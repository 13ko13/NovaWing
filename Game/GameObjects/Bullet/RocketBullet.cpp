#include "RocketBullet.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Game/GameObjects/Bullet/RocketModel.h"
#include "Manager/EffectManager.h"

namespace
{
	//プレイヤーとの距離がどのくらいの割合でホーミングをやめるか
	//プレイヤーに近づくほど1になる
	constexpr float homing_end_ratio = 0.8f;
	//どのぐらいの強さでホーミングさせるか
	constexpr float homing_strength = 0.02f;
	//跳ね返したときの攻撃力
	constexpr int reflect_attack_power = 50;
	//跳ね返したときの速度
	constexpr float reflect_speed = 60.0f;
}

RocketBullet::RocketBullet(const RocketBulletData& data) :
	EnemyBullet(data.pos, data.vel, data.attackPower,
	data.pCamera, data.pShooter, data.pEffectManager)
{
	//プレイヤー
	std::shared_ptr<Player> pPlayer = data.pShooter->GetPlayer().lock();

	//撃った位置からプレイヤーの位置までの長さを求める
	m_firstDistance = (data.pos - pPlayer->GetPos()).Length();

	//見た目はモデルで出すので、敵の弾のエフェクトは止める
	m_pEffectManager.lock()->Stop(m_effectPlayHandle);
	//ロケットのモデルを作る
	m_pModel = std::make_unique<RocketModel>(data.pCamera);
	//当たった時のヒットエフェクトを火花ではなく爆発に変える
	m_hitEffectID = ResourceLoader::EffectID::FloatingDeath;
}

RocketBullet::~RocketBullet()
{
}

int RocketBullet::GetReflectAttackPower() const
{
	return reflect_attack_power;
}

float RocketBullet::GetReflectSpeed() const
{
	return reflect_speed;
}

void RocketBullet::Update()
{
	//親クラスの更新
	EnemyBullet::Update();

	std::shared_ptr<EnemyBase> pTarget = GetShooter().lock();

	//nullじゃなければプレイヤーを追いかける
	if (pTarget && !pTarget->IsDead())
	{
		HomingPlayer(pTarget);
	}
}

void RocketBullet::Draw()
{
	//親クラスの描画
	EnemyBullet::Draw();

	//画面の中で進んで見える方向
	//プレイヤーも前に進んでいるので、プレイヤーの前に進む速度を引く
	Vector3 lookDir = m_velocity;
	std::shared_ptr<EnemyBase> pShooter = GetShooter().lock();
	if (pShooter)
	{
		std::shared_ptr<Player> pPlayer = pShooter->GetPlayer().lock();
		if (pPlayer)
		{
			lookDir = m_velocity - Vector3(0.0f, 0.0f, pPlayer->GetVel().z);
		}
	}

	//弾の位置に、進んで見える方向を向けてモデルを描画
	m_pModel->SetPos(m_pos);
	m_pModel->LookAtDir(lookDir);
	m_pModel->Draw();
}

void RocketBullet::HomingPlayer(const std::shared_ptr<EnemyBase> pTarget)
{
	//プレイヤーのポインタを取得
	std::shared_ptr<Player> pPlayer = pTarget->GetPlayer().lock();
	//プレイヤーの位置
	Vector3 playerPos;

	float currentDistance = 0.0f;
	//新しいプレイヤーまでの距離を記録
	if (pPlayer && !pPlayer->IsDead())
	{
		currentDistance = (m_pos - pPlayer->GetPos()).Length();
		//プレイヤーの位置も取得
		playerPos = pPlayer->GetPos();
	}

	//撃った位置からプレイヤーの位置までの距離の進捗を計算
	//プレイヤーに近いほど1.0fになる
	float progress = 1.0f - currentDistance / m_firstDistance;

	//今のプレイヤーまでの距離が一定以上の場合のみホーミングさせる
	if (progress < homing_end_ratio)
	{
		//プレイヤーへの方向を計算
		Vector3 toPlayerDir = playerPos - m_pos;
		toPlayerDir.Normalize();

		//プレイヤーの速度のZだけを保存
		Vector3 playerVelZ = Vector3(0.0f, 0.0f, pPlayer->GetVel().z);
		//プレイヤーから見た相対速度を計算
		Vector3 velFromPlayer = m_velocity - playerVelZ;

		//プレイヤーの方向へ少しずつ向かわせる
		Vector3 homingDir = Vector3::Lerp(
			velFromPlayer.Normalized(), toPlayerDir, homing_strength);
		//最初に相対速度として計算しているので
		//方向 * 相対速度の長さ + プレイヤーのvel
		//することでロケット自身の速度に直すことができる
		m_velocity = homingDir.Normalized() * velFromPlayer.Length() + playerVelZ;
	}
}

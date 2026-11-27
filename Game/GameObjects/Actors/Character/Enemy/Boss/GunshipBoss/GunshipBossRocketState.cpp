#include <iterator>

#include "GunshipBossRocketState.h"
#include "GunshipBoss.h"
#include "Utility/ModelAnimator.h"
#include "GunshipBossMoveState.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Manager/BulletManager.h"
#include "Game/GameObjects/Camera/CameraBase.h"

namespace
{
	//ロケットを撃つときのアニメーションの名前
	//開く
	constexpr const wchar_t* rocket_open_anim_name = L"Boss2_Armature|RocketOpen";
	//閉じる
	constexpr const wchar_t* rocket_close_anim_name = L"Boss2_Armature|RocketClose";

	//ロケットを打つ回数
	constexpr int max_rocket_fire_num = 3;
	//ロケットを打つ間隔
	constexpr int rocket_fire_interval = 30;
	//ロケットの速さ
	constexpr float rocket_move_speed = 0.005f;
	//ロケットの攻撃力
	constexpr int rocket_attack_power = 15;

	//ロケットを打つ位置のボーン名
	constexpr const wchar_t* rocket_muzzle_bone_names[] =
	{
		L"Muzzle_Rocket_0",
		L"Muzzle_Rocket_1",
		L"Muzzle_Rocket_2",
		L"Muzzle_Rocket_3"
	};
}

GunshipBossRocketState::GunshipBossRocketState(std::weak_ptr<GunshipBoss> pBoss) :
	IGunshipBossState(pBoss),
	m_animator(pBoss.lock()->GetAnimator())
{
}

GunshipBossRocketState::~GunshipBossRocketState()
{
}

void GunshipBossRocketState::Enter()
{
	//ロケットの発射口が開くアニメーションを再生
	m_animator.Play(rocket_open_anim_name, false);
}

void GunshipBossRocketState::Update()
{
	switch (m_rocketPhase)
	{
	case RocketPhase::Open:
		//発射口を開くときの更新処理
		UpdateRocketOpen();
		break;
	case RocketPhase::Fire:
		//ロケット発射時の更新処理
		UpdateRocketFire();
		break;
	case RocketPhase::Close:
		//発射口を閉じるときの更新処理
		UpdateRocketClose();
		break;

	}
}

void GunshipBossRocketState::Exit()
{
}

void GunshipBossRocketState::UpdateRocketOpen()
{
	//発射口を開き終わったらロケットを発射して
	//段階をFireに変更
	if (m_animator.IsEnd())
	{
		FireRocket();
		m_rocketPhase = RocketPhase::Fire;
	}
}

void GunshipBossRocketState::UpdateRocketFire()
{
	//ロケットを発射する間隔を計測する
	m_rocketFrame++;
	//間隔を超えたら発射
	if (m_rocketFrame > rocket_fire_interval)
	{
		FireRocket();
		m_rocketFrame = 0;
	}

	//ロケットを数個打ったら閉じるアニメーションの再生を行う
	if (m_rocketFiredNum >= max_rocket_fire_num)
	{
		//閉じるアニメーションを行う
		m_animator.Play(rocket_close_anim_name, false);
		//段階をCloseにする
		m_rocketPhase = RocketPhase::Close;
	}
}

void GunshipBossRocketState::UpdateRocketClose()
{
	//アニメーションが終わったらステートを戻す
	if (m_animator.IsEnd())
	{
		ChangeState(std::make_shared<GunshipBossMoveState>(m_pBoss));
	}
}

void GunshipBossRocketState::FireRocket()
{
	//撃った数によって0~3のボーンの位置から発射させる
	//まずボーンの番号を取得
	//発射位置のボーンの数を超えないようにする
	int boneIdx = MV1SearchFrame(
		m_pBoss.lock()->GetModelHandle(),
		rocket_muzzle_bone_names[m_rocketFiredNum % std::size(rocket_muzzle_bone_names)]
	);
	//発射ボーンの位置を取得
	Vector3 firePos;
	firePos = MV1GetFramePosition(
		m_pBoss.lock()->GetModelHandle(),
		boneIdx
	);

	std::shared_ptr<GunshipBoss> pBoss = m_pBoss.lock();
	std::shared_ptr<Player> pPlayer = pBoss->GetPlayer().lock();
	std::shared_ptr<BulletManager> pBulletManager = pBoss->GetBulletManager().lock();
	std::shared_ptr<CameraBase> pCamera = pBoss->GetCamera().lock();
	//プレイヤーの位置
	Vector3 playerPos = pPlayer->GetPos();

	//弾を撃つ方向
	Vector3 toPlayerDir = (playerPos - firePos).Normalized();
	Vector3 toPlayerVelocity = toPlayerDir * rocket_move_speed;
	//弾の管理者に弾を生成させる
	pBulletManager->CreateBullet(
		BulletManager::BulletType::RocketBullet,
		firePos, toPlayerVelocity, rocket_attack_power,
		pCamera, pBoss
	);

	//ロケットを撃った回数をインクリメント
	m_rocketFiredNum++;
}

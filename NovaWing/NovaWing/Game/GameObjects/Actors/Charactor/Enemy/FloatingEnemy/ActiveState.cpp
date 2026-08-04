#include <cmath>

#include "ActiveState.h"
#include "Utility/Vector3.h"
#include "FloatingEnemy.h"
#include "LeaveState.h"
#include "Manager/BulletManager.h"

namespace
{
	//速度
	constexpr float move_speed =8.0f;
	//周期
	constexpr float wave_speed = 0.05f;
	//ActiveStateを何フレーム続けるか
	constexpr int active_frame = 60 * 3;
	//何秒ごとに弾を発射するか
	constexpr int bullet_interval = 30;
	//弾の速度
	constexpr float bullet_speed = 7.0f;
	//弾の攻撃力
	constexpr int bullet_power = 5;
}

ActiveState::ActiveState(std::weak_ptr<FloatingEnemy> pEnemy):
	IFloatingEnemyState(pEnemy)
{

}

ActiveState::~ActiveState()
{

}

void ActiveState::Enter()
{

}

void ActiveState::Update()
{
	//フレームを更新
	m_frame++;

	//上下の揺れ
	Vector3 vel;
	vel.m_y = sinf(m_frame * wave_speed) * move_speed;
	//プレイヤーの前方向と同じ方向に一緒に進む
	std::shared_ptr<FloatingEnemy> pEnemy = m_pEnemy.lock();
	Vector3 forward = pEnemy->GetPlayerFoward();
	vel += -forward * move_speed;

	//敵の速度に適用する
	
	pEnemy->SetVel(vel);
#ifdef _DEBUG
	DrawFormatString(0, 230, 0xffffff, L"velY : %f", vel.m_y);
#endif

	//弾の発射
	//一定周期で弾を打ってくる
	if (m_frame % bullet_interval == 0)
	{
		//弾の管理者を取得する
		std::shared_ptr<BulletManager> pBulletManager = pEnemy->GetBulletManager();
		//発射位置
		Vector3 shootPos = pEnemy->GetPos();
		//速度
		//プレイヤーの位置
		Vector3 playerPos = pEnemy->GetPlayerPos();
		//敵自身の位置
		Vector3 enemyPos = shootPos;

		//プレイヤーに向かう方向ベクトル
		Vector3 toPlayerDir = playerPos - enemyPos;
		toPlayerDir = toPlayerDir.Normalized();
		//方向に速度をかける
		Vector3 shootVel = toPlayerDir * bullet_speed;
		//弾管理者に発射を依頼
		pBulletManager->CreateBullet(
			BulletManager::BulletType::EnemyBullet,//敵の弾
			shootPos, shootVel, bullet_power);//発射位置と速度と攻撃力
	}

	//Activeの時間が終了したら戦闘離脱ステートに遷移
	if (m_frame > active_frame)
	{
		//遷移
		ChangeState(std::make_shared<LeaveState>(pEnemy));
	}

}

void ActiveState::Exit()
{

}
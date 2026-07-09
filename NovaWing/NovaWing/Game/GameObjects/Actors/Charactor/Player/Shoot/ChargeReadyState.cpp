#include "ChargeReadyState.h"
#include "Manager/InputManager.h"
#include "NormalShootState.h"
#include "Manager/BulletManager.h"
#include "Charactor/Player/Player.h"

namespace
{
	//チャージ弾を打てる許容時間
	constexpr int can_shoot_frame = 60;//1秒
	//弾の速度
	constexpr float move_speed = 40.0f;
	//攻撃力
	constexpr int attack_power = 100;
}

ChargeReadyState::ChargeReadyState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager) :
	IShootState(pPlayer,pBulletManager)
{

}

ChargeReadyState::~ChargeReadyState()
{
}

void ChargeReadyState::Exit()
{
}

void ChargeReadyState::Update()
{
	//チャージ完了してからの
	//発射待ちの間の時間を計測
	m_waitFrame++;

	InputManager& input = InputManager::GetInstance();

	//ボタンが押されて、発射待ちフレームが
	//許容範囲であればチャージ弾を発射
	if (input.IsTriggered("shoot") &&
		m_waitFrame < can_shoot_frame)
	{
		//BulletManagerにチャージ弾発射を依頼する
		std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();//一時的にshared_ptrに変換
		std::shared_ptr<Player> pPlayer = m_pPlayer.lock();//一時的にshared_ptrに変換
		const Vector3 pos = pPlayer->GetPos();//プレイヤーの位置
		const Vector3 vel = -pPlayer->GetForward() * move_speed;//速度

		//プレイヤーからターゲットを受け取る
		std::weak_ptr<GameObject> pTarget = m_pPlayer.lock()->GetFocusTarget();

		pBulletManager->CreateBullet(BulletManager::BulletType::ChargeBullet,
			pos, vel, attack_power,pTarget);

		//ノーマルステートに戻す
		ChangeState(std::make_shared<NormalShootState>(m_pPlayer, m_pBulletManager));
	}
	//タイムアウトなら
	else if (m_waitFrame > can_shoot_frame)
	{
		//何もせずにノーマルステートに戻す
		ChangeState(std::make_shared<NormalShootState>(m_pPlayer, m_pBulletManager));
	}

#ifdef _DEBUG
	//チャージ完了デバッグ
	DrawFormatString(0, 350, 0xff0000, L"チャージ完了");
#endif // _DEBUG
}

void ChargeReadyState::Enter()
{
	//待機フレームを初期化
	m_waitFrame = 0;
}

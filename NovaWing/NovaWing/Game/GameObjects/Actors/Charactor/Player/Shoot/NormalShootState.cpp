#include "NormalShootState.h"
#include "Charactor/Player/Player.h"
#include "ChargeShootState.h"
#include "Manager/BulletManager.h"
#include "Manager/InputManager.h"
#include "Manager/SoundManager.h"
#include "Manager/TargetManager.h"

namespace
{
	// 弾の速度
	constexpr float move_speed = 60.0f;
	// 攻撃力
	constexpr int attack_power = 10;
	// クールタイム
	constexpr int cool_time = 5;

	// チャージ開始とみなすときの長押しフレーム数
	constexpr int charge_start_frame = 10;
}

NormalShootState::NormalShootState(
	const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<SoundManager> pSoundManager,
	std::weak_ptr<TargetManager> pTargetManager) :
	IShootState(pPlayer, pBulletManager, pSoundManager,pTargetManager)
{
}

NormalShootState::~NormalShootState()
{
}

void NormalShootState::Exit()
{
}

void NormalShootState::Update()
{
	// クールタイム更新
	m_shootCT++;

	// インプットマネージャーを借りてくる
	InputManager& input = InputManager::GetInstance();

	// shootボタンが押されたら弾を発射する
	if (input.IsTriggered(InputEvent::shoot) &&
		m_shootCT > cool_time)
	{
		// BulletManagerに弾の生成を依頼する
		std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock(); // 一時的にshared_ptrに変換
		std::shared_ptr<Player> pPlayer = m_pPlayer.lock();						 // 一時的にshared_ptrに変換
		const Vector3 pos = pPlayer->GetPos();									 // プレイヤーの位置
		const Vector3 vel = -pPlayer->GetForward() * move_speed;				 // 速度

		pBulletManager->CreateBullet(BulletManager::BulletType::PlayerBullet,
									 pos, vel, attack_power,pPlayer->GetCamera());

		// 発射音を鳴らす
		m_pSoundManager.lock()->Play(SoundManager::SoundType::NormalShoot);

		// クールタイムをリセット
		m_shootCT = 0;
	}
	// ボタンが押され続けていたら
	else if (input.IsPressed(InputEvent::shoot))
	{
		// ボタンが押されている時間を計測
		m_pressingShootButton++;
		// 一定フレーム長押しされていれば長押しとみなす
		if (m_pressingShootButton > charge_start_frame)
		{
			// ChargeShootStateに遷移する
			ChangeState(std::make_shared<ChargeShootState>(m_pPlayer, m_pBulletManager, m_pSoundManager,m_pTargetManager));
		}
	}
	// ボタンが離されたら長押しされている時間をリセット
	if (input.IsReleased(InputEvent::shoot))
	{
		m_pressingShootButton = 0;
	}
}

void NormalShootState::Enter()
{
	// ロック機能を終了する
	if (m_pTargetManager.lock() != nullptr)
	{
		m_pTargetManager.lock()->EndLock();
	}
}

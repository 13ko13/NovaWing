#include "NormalShootState.h"
#include "Manager/InputManager.h"
#include "ChargeShootState.h"
#include "Charactor/Player/Player.h"
#include "Manager/BulletManager.h"

namespace
{
	//弾の速度
	constexpr float move_speed = 60.0f;
	//攻撃力
	constexpr int attack_power = 100;
	//クールタイム
	constexpr int cool_time = 10;
}

NormalShootState::NormalShootState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager) :
	IShootState(pPlayer,pBulletManager)
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
	//クールタイム更新
	m_shootCT++;

	//インプットマネージャーを借りてくる
	InputManager& input = InputManager::GetInstance();

	//shootボタンが押されたら弾を発射する
	if (input.IsTriggered(InputEvent::shoot) &&
		m_shootCT > cool_time)
	{
		//BulletManagerに弾の生成を依頼する
		std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();//一時的にshared_ptrに変換
		std::shared_ptr<Player> pPlayer = m_pPlayer.lock();//一時的にshared_ptrに変換
		const Vector3 pos = pPlayer->GetPos();//プレイヤーの位置
		const Vector3 vel = -pPlayer->GetForward() * move_speed;//速度
		
		pBulletManager->CreateBullet(BulletManager::BulletType::PlayerBullet,
			pos, vel, attack_power);

		//クールタイムをリセット
		m_shootCT = 0;
	}
	//ボタンが押され続けていたら
	else if(input.IsPressed(InputEvent::shoot))
	{
		//ChargeShootStateに遷移する
		ChangeState(std::make_shared<ChargeShootState>(m_pPlayer, m_pBulletManager));
	}
}

void NormalShootState::Enter()
{

}

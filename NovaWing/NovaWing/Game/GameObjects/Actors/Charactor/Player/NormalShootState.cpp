#include "NormalShootState.h"
#include "Manager/InputManager.h"
#include "ChargeShootState.h"

NormalShootState::NormalShootState(const std::weak_ptr<Player> pPlayer) :
	IShootState(pPlayer)
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
	//インプットマネージャーを借りてくる
	InputManager& input = InputManager::GetInstance();

	//shootボタンが押されたら弾を発射する
	if (input.IsTriggered("shoot"))
	{
		//TODO:BulletManagerに弾の生成を依頼する

	}
	//ボタンが押され続けていたら
	else if(input.IsPressed("shoot"))
	{
		//ChargeShootStateに遷移する
		ChangeState(std::make_shared<ChargeShootState>(m_pPlayer));
	}
}

void NormalShootState::Enter()
{

}

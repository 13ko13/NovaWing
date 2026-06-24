#include "NormalShootState.h"
#include "Manager/InputManager.h"

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
		//TODO:ChargeShootStateに遷移する

	}
}

void NormalShootState::Enter()
{
}

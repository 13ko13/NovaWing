#include "Player.h"
#include "IPlayerState.h"
#include "NormalState.h"
#include "../../Utility/SmartPointerHelper.h"

Player::Player(std::shared_ptr<InputManager> inputManager,
	std::shared_ptr<BulletManager> bulletManager) :
	m_pInputManager(inputManager),
	m_pBulletManager(bulletManager)
{
	
}

Player::~Player()
{
	//処理なし
}

void Player::Init()
{
	//Normalステートに初期化
	//shared_ptrに変換
	std::shared_ptr<InputManager> pInput = WeakToShared(m_pInputManager);
	//Nullチェック
	if (pInput)
	{
		m_pCurrentState = std::make_shared<NormalState>(pInput, shared_from_this());
	}
}

void Player::Update()
{
	//もしプレイヤーの現在のステートが存在するなら
	if (m_pCurrentState)
	{
		m_pCurrentState->Update();
	}
}

void Player::Draw()
{
}

void Player::TakeDamage(int damage)
{

}

void Player::ChangeState(std::shared_ptr<IPlayerState> pNextState)
{
	//Nullチェック
	if (m_pCurrentState != nullptr)
	{
		//現在のステートの出たときの処理を呼ぶ
		m_pCurrentState->Exit();
	}

	//新しいステートに差し替える
	m_pCurrentState = pNextState;
	//そのステートの入った時の処理を呼ぶ
	m_pCurrentState->Enter();
}
#include <algorithm>

#include "Player.h"
#include "IPlayerState.h"
#include "NormalState.h"
#include "../../../../../Utility/SmartPointerHelper.h"
#include "../../../../../Utility/Quaternion.h"
#include "../../../../../Utility/Matrix4x4.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 0.3f,0.3f,0.3f };

	//移動制限範囲
	constexpr float move_limit_x = 500.0f;
	constexpr float move_limit_y = 300.0f;
}

Player::Player(
	std::shared_ptr<BulletManager> bulletManager,
	ResourceLoader::ModelID modelID):
	Charactor(modelID),
	m_pBulletManager(bulletManager)
{

}

Player::~Player()
{
	//処理なし
}

void Player::OnInit()
{
	//Normalステートに初期化
	//Nullチェック
	//shared_from_thisがPlayerではなくActor型なのでPlayerにキャストする
	m_pCurrentState = std::make_shared<NormalState>(static_pointer_cast<Player>(shared_from_this()));
}

void Player::Update()
{
	Charactor::Update();

	//もしプレイヤーの現在のステートが存在するなら
	if (m_pCurrentState)
	{
		m_pCurrentState->Update();
	}
	m_pos.m_x = std::clamp(m_pos.m_x, -move_limit_x, move_limit_x);
	m_pos.m_y = std::clamp(m_pos.m_y, -move_limit_y, move_limit_y);
}

void Player::Draw()
{
	Matrix4x4 mat;
	//拡大縮小行列
	Matrix4x4 scaleMat = Matrix4x4::Scale(model_scale);
	mat = scaleMat;
	//回転行列
	//m_rotationをMatrix4x4に変換
	Matrix4x4 rotMat = m_rotation.ToMatrix4x4();
	mat = mat * rotMat;
	//移動行列
	Matrix4x4 transMat = Matrix4x4::Translate(m_pos);
	mat = mat * transMat;
	MV1SetMatrix(m_modelHandle, mat.ToDxLib());

	//プレイヤーのモデルを描画する
	MV1DrawModel(m_modelHandle);
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
#include "FloatingEnemy.h"
#include "IEnemyState.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 1.0f,1.0f,1.0f };
}

FloatingEnemy::FloatingEnemy(const std::weak_ptr<Player> pPlayer,
	const ResourceLoader::ModelID Id) :
	Charactor(Id),
	m_pPlayer(pPlayer)
{

}

FloatingEnemy::~FloatingEnemy()
{

}

void FloatingEnemy::OnInit()
{
	//ステートを初期化
	//TODO:隠れているステートで初期化する

	//ステートに入った時の処理
	m_pState->Enter();
}

void FloatingEnemy::Update()
{
	//ステートの更新
	m_pState->Update();
	//次のステートを取得
	std::shared_ptr<IEnemyState> pState = m_pState->GetNextState();
	//次のステートがあればステートを変更する
	if (pState != nullptr)
	{
		//前ステートの出るときの処理
		m_pState->Exit();
		//ステートを変更
		m_pState = pState;
		//切り替え後の入った時の処理
		m_pState->Enter();
	}

	//キャラクタークラス共通の処理
	Charactor::Update();
}

void FloatingEnemy::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//モデルを描画
	MV1DrawModel(m_modelHandle);
}

void FloatingEnemy::TakeDamage(int damage)
{

}

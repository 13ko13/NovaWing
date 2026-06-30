#include "FloatingEnemy.h"
#include "IEnemyState.h"
#include "Charactor/Player/Player.h"
#include "HideState.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 0.3f,0.3f,0.3f };
	//敵自身の球の当たり判定の半径
	constexpr float col_radius = 132.0f;
}

FloatingEnemy::FloatingEnemy(const std::weak_ptr<Player> pPlayer,
	const ResourceLoader::ModelID Id) :
	Charactor(Id),
	m_pPlayer(pPlayer),
	m_colSphere(m_pos)
{

}

FloatingEnemy::~FloatingEnemy()
{

}

void FloatingEnemy::OnInit()
{
	//ステートを初期化
	//隠れているときのステートで初期化する
	m_pState = std::make_shared<HideState>(
		std::static_pointer_cast<FloatingEnemy>(shared_from_this())
	);

	//ステートに入った時の処理
	m_pState->Enter();
	m_pos.m_z += 900;
	//m_pos.m_x += 200;
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

	//当たり判定の更新
	m_colSphere.Update(m_pos, col_radius);
}

void FloatingEnemy::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale,m_pos, m_rotation, m_modelHandle);

	//モデルを描画
	MV1DrawModel(m_modelHandle);

#ifdef _DEBUG
	//当たり判定の描画
	m_colSphere.Draw(0xaaffff);
	//位置
	DrawFormatString(0, 320, 0xffffff, L"EPosX:%f,Y:%f,Z:%f", m_pos.m_x, m_pos.m_y, m_pos.m_z);
#endif
}

void FloatingEnemy::TakeDamage(int damage)
{

}

Vector3 FloatingEnemy::GetPlayerPos() const
{
	//shared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//位置を取得して返す
	return pPlayer->GetPos();
}

Vector3 FloatingEnemy::GetPlayerFoward() const
{
	//shared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//前方向を取得して返す
	return pPlayer->GetForward();
}

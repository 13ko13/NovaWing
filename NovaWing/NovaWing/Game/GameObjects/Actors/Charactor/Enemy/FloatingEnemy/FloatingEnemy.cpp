#include "FloatingEnemy.h"
#include "IEnemyState.h"
#include "Charactor/Player/Player.h"
#include "HideState.h"
#include "Manager/LightingManager.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 1.0f,1.0f,1.0f };
	//敵自身の球の当たり判定の半径
	constexpr float col_radius = 132.0f;
}

FloatingEnemy::FloatingEnemy(const std::weak_ptr<Player> pPlayer,
	const ResourceLoader::ModelID Id,
	const std::shared_ptr<BulletManager> pBulletManager,
	std::weak_ptr<CameraBase> camera) :
	Charactor(Id,camera),
	m_pPlayer(pPlayer),
	m_colSphere(m_pos),
	m_pBulletManager(pBulletManager)
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

	//アニメーターを作成
	m_pAnimator = std::make_shared<ModelAnimator>(m_modelHandle);

	//ステートに入った時の処理
	m_pState->Enter();
	m_pos.m_z += 900.0f;
	m_pos.m_y += 500.0f;

	//定数バッファを作成
	CreateShaderBuffers();

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
	//アニメーターの更新
	m_pAnimator->Update(1.0f);

	//キャラクタークラス共通の処理
	Charactor::Update();

	//当たり判定の更新
	m_colSphere.Update(m_pos, col_radius);
}

void FloatingEnemy::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新してから
	UpdateShaderMatrixData();

	//敵の描画
	DrawEnemy();


#ifdef _DEBUG
	//当たり判定の描画
	m_colSphere.Draw(0xffffff);
	//位置
	DrawFormatString(0, 320, 0xffffff, L"EPosX:%f,Y:%f,Z:%f", m_pos.m_x, m_pos.m_y, m_pos.m_z);
#endif
}

void FloatingEnemy::DrawEnemy()
{
	//ResourceLoaderから浮遊敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::EnemyNormalMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::EnemyEmissionMap);

	//テクスチャをシェーダにセットする
	SetUseTextureToShader(1, normGraphH);//t1
	SetUseTextureToShader(2, -1);//t2
	SetUseTextureToShader(3, emissionGraphH);//t3

	LightingManager::GetInstance().ApplyShader();
	//定数バッファをシェーダレジスタにセットする
	BindShaderBuffers();

	//モデル描画
	MV1DrawModel(m_modelHandle);

	//テクスチャを解除する
	SetUseTextureToShader(1, -1);//法線マップを解除
	SetUseTextureToShader(2, -1);//メタリックマップを解除
	SetUseTextureToShader(3, -1);//エミッションマップを解除
	//シェーダを解除
	LightingManager::GetInstance().ResetShader();
	ReleaseShaderBuffers();
}

void FloatingEnemy::TakeDamage(int damage)
{
	//体力を減らす
	m_health -= damage;
	//体力が0以下になったら死亡処理
	if (m_health <= 0)
	{
		OnDead();
	}
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

std::shared_ptr<BulletManager> FloatingEnemy::GetBulletManager() const
{
	std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();
	return pBulletManager;
}

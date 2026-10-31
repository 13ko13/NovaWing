#include "FloatingEnemy.h"
#include "IFloatingEnemyState.h"
#include "Character/Player/Player.h"
#include "HideState.h"
#include "Manager/LightingManager.h"
#include "Constants/ShaderRegister.h"
#include "Manager/DebugManager.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 2.0f,2.0f,2.0f };
}

FloatingEnemy::FloatingEnemy(const std::weak_ptr<Player> pPlayer,
	const ResourceLoader::ModelID Id,
	const std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<CameraBase> camera,
	const Vector3& pos,
	std::weak_ptr<SoundManager> pSoundManager,
	std::weak_ptr<EffectManager> pEffectManager) :
	EnemyBase(Id,camera,pPlayer,pBulletManager,pSoundManager,pEffectManager),
	m_colSphere(std::make_shared<SphereShape>(pos, 0.0f)),
	m_collider(*this, ColliderTag::Floating)
{
	//位置を反映
	SetPos(pos);
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

	//定数バッファを作成
	CreateShaderBuffers();

	//m_rotation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), DX_PI_F);
}

void FloatingEnemy::Update()
{
	//死亡待機中以外は処理を行う
	if(!m_isDying)
	{
		//ステートの更新
		m_pState->Update();
		//次のステートを取得
		std::shared_ptr<IFloatingEnemyState> pState = m_pState->GetNextState();
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
		Character::Update();

		//当たり判定の更新
		m_colSphere->Update(m_pos, m_colRadius);
	}
	//死亡待機中はフレームを数えて一定フレーム経ったら存在を削除
	else
	{
		m_dyingFrame++;
		if(m_dyingFrame > m_trueDeadFrame)
		{
			//完全死亡
			OnEnemyDead();
		}
	}
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
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		//当たり判定の描画
		m_colSphere->Draw(0xffffff);
	}
#endif
}

void FloatingEnemy::DrawEnemy()
{
	//ResourceLoaderから浮遊敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::FloatingNormalMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::FloatingEmissionMap);
	//ディゾルブ用ノイズ
    int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::DissolveNoise
    );

	//ハンドルとレジスタ番号をセットで保持しておく
    std::vector<std::pair<int,int>> textures;
    textures.push_back({ShaderRegister::tex_normal,normGraphH});//法線マップ
    textures.push_back({ShaderRegister::tex_metalic,-1});//メタリックマップはないので-1
    textures.push_back({ShaderRegister::tex_emission,emissionGraphH});//エミッション
    textures.push_back({ShaderRegister::tex_noise,noiseHandle});//ノイズテクスチャ

    //DrawWithLightingにペアの配列を渡してモデルを描画
    DrawWithLighting(textures);
}

Vector3 FloatingEnemy::GetPlayerPos() const
{
	//shared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//位置を取得して返す
	return pPlayer->GetPos();
}

Vector3 FloatingEnemy::GetPlayerVel() const
{
	//shared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//速度を取得して返す
	return pPlayer->GetVel();
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

std::vector<std::shared_ptr<SphereShape>> FloatingEnemy::GetCollisionSpheres() const
{
	return { m_colSphere };
}

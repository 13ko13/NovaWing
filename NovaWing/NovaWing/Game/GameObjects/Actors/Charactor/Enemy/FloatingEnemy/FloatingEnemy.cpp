#include <EffekseerForDXLib.h>

#include "FloatingEnemy.h"
#include "IFloatingEnemyState.h"
#include "Charactor/Player/Player.h"
#include "HideState.h"
#include "Manager/LightingManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 1.0f,1.0f,1.0f };
	//敵自身の球の当たり判定の半径
	constexpr float col_radius = 132.0f;
	//死亡待機状態から完全死亡になるまでのフレーム
	constexpr int true_dead_frame = 10;
}

FloatingEnemy::FloatingEnemy(const std::weak_ptr<Player> pPlayer,
	const ResourceLoader::ModelID Id,
	const std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<CameraBase> camera,
	const Vector3& pos,
	int health) :
	EnemyBase(Id,camera,pPlayer,pBulletManager,health),
	m_colSphere(pos)
{
	//位置を反映
	SetPos(pos);
}

FloatingEnemy::~FloatingEnemy()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);
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
		Charactor::Update();

		//当たり判定の更新
		m_colSphere.Update(m_pos, col_radius);
	}
	//死亡待機中はフレームを数えて一定フレーム経ったら存在を削除
	else
	{
		m_dyingFrame++;
		if(m_dyingFrame > true_dead_frame)
		{
			//完全死亡
			OnEnemyDead();
		}
	}

	//エフェクトの位置の調整する
		SetPosPlayingEffekseer3DEffect(
			m_effectPlayHandle, GetPos().m_x, GetPos().m_y, GetPos().m_z
		);
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

	DrawFormatString(0, 460, 0xffffff, L"ForwardX : %f,Y:%f,Z:%f", GetForward().m_x,GetForward().m_y,GetForward().m_z);
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

void FloatingEnemy::TakeDamage(int damage)
{
	//体力を減らす
	m_health -= damage;
	//体力が0以下になったら死亡処理
	if (m_health <= 0)
	{
		//死亡待機状態をtrueにする
		m_isDying = true;

		//Effekseerのエフェクト再生を呼ぶ
		m_effectPlayHandle = PlayEffekseer3DEffect(
			ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::FloatingDeath)
		);

		//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
		SetPosPlayingEffekseer3DEffect(
			m_effectPlayHandle, m_pos.m_x, m_pos.m_y, m_pos.m_z
		);
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

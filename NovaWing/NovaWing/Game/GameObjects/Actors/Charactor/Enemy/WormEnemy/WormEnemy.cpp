#include <EffekseerForDXLib.h>

#include "WormEnemy.h"
#include "Charactor/Player/Player.h"
#include "Manager/BulletManager.h"
#include "Manager/LightingManager.h"
#include "Constants/ShaderRegister.h"
#include "Manager/SoundManager.h"
#include "Game/GameObjects/Camera/CameraBase.h"

namespace
{
	//頭の移動速度
	constexpr float move_speed =18.0f;
	//螺旋状に回転するときの回転速度
	constexpr float rot_speed = 4.0f;
	//螺旋移動の時の半径
	constexpr float spiral_radius = 160.0f;

	//当たり判定球の半径
	constexpr float sphere_radius = 40.0f;

	//どのぐらいの間隔で弾を撃つか
	constexpr int shoot_interval = 60;
	//弾の速度
	constexpr float bullet_speed = 8.0f;
	//弾の攻撃力
	constexpr int bullet_power = 5;
	//胴体間の間隔フレーム数
	constexpr int spacing = 4;

	//モデルのサイズ
	const Vector3 model_scale = { 1.0f,1.0f,1.0f };
	//胴体の死亡エフェクトを出すときの間隔フレーム数
	constexpr int death_effect_interval = 13;
}

WormEnemy::WormEnemy(
	const std::weak_ptr<Player> pPlayer,//プレイヤー
	const std::weak_ptr<BulletManager> pBulletManager,//バレットマネージャー
	std::weak_ptr<CameraBase> camera,//カメラ
	const WormEnemyData& data,//ワームエネミーに必要なデータ
	std::weak_ptr<SoundManager> pSoundManager) :
	EnemyBase(data.modelID,camera,pPlayer,pBulletManager),
	m_segmentCount(data.segmentCount),
	m_headSphere(data.pos),
	m_moveDirection(data.direction),
	m_activatePlayerZ(data.activatePlayerZ),
	m_pSoundManager(pSoundManager)
{
	//位置を反映
	SetPos(data.pos);

	//螺旋の中心を保存
	m_spiralCenter = Vector2(data.pos.m_x, data.pos.m_y);
}

WormEnemy::~WormEnemy()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_deathPlayHandle);
}

void WormEnemy::OnInit()
{
	//配列の要素数確定
	m_segmentPositions.resize(m_segmentCount);
	m_segmentSpheres.resize(m_segmentCount);

	//胴体の数だけ位置と当たり判定を初期化する
	//位置の初期化
	for (Vector3& segmentPos : m_segmentPositions)
	{
		segmentPos = m_pos;
	}
	//当たり判定の初期化
	for (Sphere& sphere : m_segmentSpheres)
	{
		sphere = Sphere(m_pos);
	}
	//Y軸に180度回転する(モデルが反対を向いているので)
	Vector3 axis = Vector3(0.0f, 1.0f, 0.0f);
	//初期回転
	Quaternion initRot = Quaternion(axis, DX_PI_F);
	m_rotation = initRot;

	//定数バッファを作成
	CreateShaderBuffers();
}

void WormEnemy::Update()
{
	//プレイヤーがワームの動き出し距離まで来ていなければ処理を飛ばす
	if(m_pPlayer.lock()->GetPos().m_z < m_activatePlayerZ) return;

	//フレーム更新
	m_frame++;

	//死亡待機状態じゃなければ更新処理を行う
	if (!m_isDying)
	{
		//カメラのFar距離より奥(見えない位置)まで離れたら消す
		std::shared_ptr<CameraBase> pCamera = m_pCamera.lock();
		if (pCamera != nullptr)
		{
			float distFromCamera = (m_pos - GetCameraPos()).Length();
			if (distFromCamera > pCamera->GetFarClip())
			{
				OnEnemyDead();
				return;
			}
		}

		//螺旋状に回転させる
		m_rotationAngle += rot_speed;
		//初期回転を適用したうえで
		Quaternion initRot = Quaternion(
			Vector3(0.0f, 1.0f, 0.0f),
			DX_PI_F
		);

		//Z軸を中心に螺旋回転させる
		Quaternion rotZ = Quaternion(
				Vector3(0.0f, 0.0f, 1.0f),
				m_rotationAngle * DX_PI_F / 180.0f
		);
		//初期回転を適用したうえで螺旋回転を適用する
		m_rotation = rotZ * initRot;

		//頭の移動を螺旋状にする
		m_pos.m_z += move_speed * m_moveDirection;
		m_pos.m_x = m_spiralCenter.m_x + cosf(m_rotationAngle * DX_PI_F / 180.0f) *
			spiral_radius;
		m_pos.m_y = m_spiralCenter.m_y + sinf(m_rotationAngle * DX_PI_F / 180.0f) *
			spiral_radius;

		//頭の位置を履歴に追加
		m_headHistory.insert(m_headHistory.begin(), m_pos);

		//それぞれの胴体の位置を履歴から取得する
		for (int i = 0; i < m_segmentCount; i++)
		{
			//履歴が足りないときにクラッシュするので
			//if文でガードする
			if (static_cast<int>(m_headHistory.size()) > (i+1) * spacing)
			{
				m_segmentPositions[i] = m_headHistory[(i+1) * spacing];
			}
		}

		//各Sphereの位置を更新
		//頭の当たり判定更新
		m_headSphere.Update(m_pos, sphere_radius);
		//胴体の当たり判定更新(胴体の数だけループ)
		for (int i = 0; i < m_segmentCount; i++)
		{
			//対応する位置をそれぞれ更新する
			m_segmentSpheres[i].Update(
				m_segmentPositions[i], sphere_radius
			);
		}

		//一定間隔で弾を発射する
		if (m_frame % shoot_interval == 0)
		{
			//弾の管理者を取得する
			std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();
			//発射位置
			Vector3 shootPos = m_pos;
			//速度
			//プレイヤーの位置
			Vector3 playerPos = m_pPlayer.lock()->GetPos();
			//敵自身の位置
			Vector3 enemyPos = shootPos;

			//プレイヤーに向かう方向ベクトル
			Vector3 toPlayerDir = playerPos - enemyPos;
			toPlayerDir = toPlayerDir.Normalized();
			//方向に速度をかける
			Vector3 shootVel = toPlayerDir * bullet_speed;
			//弾管理者に発射を依頼
			pBulletManager->CreateBullet(
				BulletManager::BulletType::EnemyBullet,//敵の弾
				shootPos, shootVel, bullet_power);//発射位置と速度と攻撃力

			//発射音を鳴らす
			m_pSoundManager.lock()->Play(SoundManager::SoundType::EnemyShoot);
		}
	}

	//死亡エフェクト再生許可が出ていれば(死亡していれば)
	if(m_isCanPlayEffect)
	{
		//まだ死亡エフェクトを胴体の数 + 1(頭の数)分出し終わっていなければ
		//死亡エフェクトを一定間隔で、頭と胴体の場所に出す
		if (m_segmentCount >= m_deathEffectNum)
		{
			if (m_frame % death_effect_interval == 0)
			{
				//死亡エフェクトが未発動であれば頭の位置にエフェクトを出す
				if (m_deathEffectNum == 0)
				{
					//エフェクト再生を呼ぶ
					m_deathPlayHandle = PlayEffekseer3DEffect(
						ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::WormDeath)
					);
					//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
					SetPosPlayingEffekseer3DEffect(
						m_deathPlayHandle, m_pos.m_x, m_pos.m_y, m_pos.m_z
					);	
				}
				else
				{
					//エフェクト再生を呼ぶ
					m_deathPlayHandle = PlayEffekseer3DEffect(
						ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::WormDeath)
					);
					//再生直後に胴体の位置にセットする
					SetPosPlayingEffekseer3DEffect(
						m_deathPlayHandle,
						m_segmentPositions[m_deathEffectNum - 1].m_x,
						m_segmentPositions[m_deathEffectNum - 1].m_y,
						m_segmentPositions[m_deathEffectNum - 1].m_z
					);	
				}

				//爆発音を鳴らす
				m_pSoundManager.lock()->Play(SoundManager::SoundType::EnemyDeath);

				//エフェクトを出した数を更新
				m_deathEffectNum++;
			}
		}
		//頭+胴体の数分死亡エフェクトを出し終わっていればワームの存在を消す
		else
		{
			OnEnemyDead();
		}
	}
}

void WormEnemy::Draw()
{
	//プレイヤーがワームの動き出し距離まで来ていなければ処理を飛ばす
	if(m_pPlayer.lock()->GetPos().m_z < m_activatePlayerZ) return;

	//頭がまだ爆発していなければ描画する
	if (m_deathEffectNum == 0)
	{
		//モデルのスケール、位置、回転を適用する
		ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);
		//シェーダに渡すカメラ情報を更新してから
		UpdateShaderMatrixData();
		//頭のモデルを描画
		DrawWormHead();
	}
	
	//胴体のモデルを描画
	//胴体の数だけループを回す
	for (int i = 0; i < m_segmentCount; i++)
	{
		//その胴体がまだ爆発していなければ描画する
		if (m_deathEffectNum < i + 1)
		{
			//それぞれの胴体の位置にモデルを適用する
			ApplyMatrix(
				model_scale, 
				m_segmentPositions[i],
				m_rotation, m_modelHandle);
			//シェーダに渡すカメラ情報を更新してから
			UpdateShaderMatrixData();

			//それぞれの胴体のモデルを描画する
			DrawWormBody();
		}
	}

#ifdef _DEBUG
	//頭の当たり判定を描画
	m_headSphere.Draw(0xff0000);
	//胴体の当たり判定を描画
	for (Sphere& sphere : m_segmentSpheres)
	{
		sphere.Draw(0x00ff00);
	}
#endif
}

void WormEnemy::DrawWormHead()
{
	//ResourceLoaderから浮遊敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::WormHeadNormalMap);
	//メタリックマップを取得
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::WormHeadMetalicMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::WormHeadEmissionMap);
	//ディゾルブ用ノイズ
    int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::DissolveNoise
    );
	
	//ハンドルとレジスタ番号をセットで保持しておく
    std::vector<std::pair<int,int>> textures;
    textures.push_back({ShaderRegister::tex_normal,normGraphH});//法線マップ
    textures.push_back({ShaderRegister::tex_metalic,metalicGraphH});//メタリックマップ
    textures.push_back({ShaderRegister::tex_emission,emissionGraphH});//エミッション
    textures.push_back({ShaderRegister::tex_noise,noiseHandle});//ノイズテクスチャ

    //DrawWithLightingにペアの配列を渡してモデルを描画
    DrawWithLighting(textures);
}

void WormEnemy::DrawWormBody()
{
	//ResourceLoaderから浮遊敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::WormHeadNormalMap);
	//メタリックマップを取得
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::WormHeadMetalicMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::WormHeadEmissionMap);
	//ワームの胴体のディフューズマップを取得
	const int diffuseGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::WormBodyDiffuseMap);
	//ディゾルブ用ノイズ
    int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::DissolveNoise);

	//ハンドルとレジスタ番号をセットで保持しておく
    std::vector<std::pair<int,int>> textures;
    textures.push_back({ShaderRegister::tex_normal,normGraphH});//法線マップ
    textures.push_back({ShaderRegister::tex_metalic,metalicGraphH});//メタリックマップ
    textures.push_back({ShaderRegister::tex_emission,emissionGraphH});//エミッション
    textures.push_back({ShaderRegister::tex_noise,noiseHandle});//ノイズテクスチャ
    textures.push_back({ShaderRegister::tex_diffuse,diffuseGraphH});//ディフューズマップ

    //DrawWithLightingにペアの配列を渡してモデルを描画
    DrawWithLighting(textures);
}

void WormEnemy::TakeDamage(int damage)
{
	//体力を減らす
	m_health -= damage;
	//体力が0以下になったら死亡処理
	if (m_health <= 0)
	{
		//エフェクト再生の許可を出す
		m_isCanPlayEffect = true;
		//死亡待機状態にする
		m_isDying = true;
	}
}

std::vector<Sphere> WormEnemy::GetCollisionSpheres() const
{
	//頭と胴体の当たり判定を全部詰めて返す
	std::vector<Sphere> spheres;
	spheres.push_back(m_headSphere);
	spheres.insert(spheres.end(), m_segmentSpheres.begin(), m_segmentSpheres.end());
	return spheres;
}

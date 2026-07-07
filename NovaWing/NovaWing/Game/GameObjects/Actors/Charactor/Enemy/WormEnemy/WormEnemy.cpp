#include "WormEnemy.h"
#include "Charactor/Player/Player.h"
#include "Manager/BulletManager.h"
#include "Manager/LightingManager.h"

namespace
{
	//頭の移動速度
	constexpr float move_speed =9.0f;
	//螺旋状に回転するときの回転速度
	constexpr float rot_speed = 2.0f;
	//螺旋移動の時の半径
	constexpr float spiral_radius = 160.0f;

	//当たり判定球の半径
	constexpr float sphere_radius = 40.0f;

	//どのぐらいの間隔で弾を撃つか
	constexpr int shoot_interval = 60;
	//弾の速度
	constexpr float bullet_speed = 8.0f;
	//弾の攻撃力
	constexpr int bullet_power = 1;
	//胴体間の間隔フレーム数
	constexpr int spacing = 6;

	//モデルのサイズ
	const Vector3 model_scale = { 1.0f,1.0f,1.0f };
}

WormEnemy::WormEnemy(
	const std::weak_ptr<Player> pPlayer,
	const ResourceLoader::ModelID Id,
	const std::shared_ptr<BulletManager> pBulletManager,
	int segmentCount) :
	Charactor(Id),
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager),
	m_segmentCount(segmentCount),
	m_headSphere(m_pos)
{

}

WormEnemy::~WormEnemy()
{

}

void WormEnemy::OnInit()
{
	//配列の要素数確定
	m_segmentPositions.resize(m_segmentCount);
	m_segmentSpheres.resize(m_segmentCount);

	m_pos.m_y += 500.0f;

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

	//仮で位置決定
	m_pos.m_z -= 500.0f;//プレイヤーより後ろに配置
}

void WormEnemy::Update()
{
	//フレーム更新
	m_frame++;

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
	m_pos.m_z += move_speed;
	m_pos.m_x = cosf(m_rotationAngle * DX_PI_F / 180.0f) *
		spiral_radius;
	m_pos.m_y = sinf(m_rotationAngle * DX_PI_F / 180.0f) *
		spiral_radius + 500.0f;

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
	}
}

void WormEnemy::Draw()
{
	//モデルのスケール、位置、回転を適用する
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);
	//シェーダに渡すカメラ情報を更新してから
	UpdateShaderMatrixData(GetPlayerCameraPos());
	//頭のモデルを描画
	DrawWormHead();
	//胴体のモデルを描画
	//胴体の数だけループを回す
	for (int i = 0; i < m_segmentCount; i++)
	{
		//それぞれの胴体の位置にモデルを適用する
		ApplyMatrix(
			model_scale, 
			m_segmentPositions[i],
			m_rotation, m_modelHandle);
		//シェーダに渡すカメラ情報を更新してから
		UpdateShaderMatrixData(GetPlayerCameraPos());

		//それぞれの胴体のモデルを描画する
		DrawWormBody();
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
	

	//シェーダにテクスチャをセットする
	SetUseTextureToShader(1, normGraphH);//t1
	SetUseTextureToShader(2, metalicGraphH);//t2
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

	//シェーダにテクスチャをセットする
	SetUseTextureToShader(0, diffuseGraphH);//t0
	SetUseTextureToShader(1, normGraphH);//t1
	SetUseTextureToShader(2, metalicGraphH);//t2
	SetUseTextureToShader(3, emissionGraphH);//t3

	LightingManager::GetInstance().ApplyShader();
	//定数バッファをシェーダレジスタにセットする
	BindShaderBuffers();

	//モデル描画
	MV1DrawModel(m_modelHandle);

	//テクスチャを解除する
	SetUseTextureToShader(0, -1);//ディフューズマップを解除
	SetUseTextureToShader(1, -1);//法線マップを解除
	SetUseTextureToShader(2, -1);//メタリックマップを解除
	SetUseTextureToShader(3, -1);//エミッションマップを解除
	//シェーダを解除
	LightingManager::GetInstance().ResetShader();
	ReleaseShaderBuffers();
}

void WormEnemy::TakeDamage(int damage)
{
	//体力を減らす
	m_health -= damage;
	//体力が0以下になったら死亡処理
	if (m_health <= 0)
	{
		OnDead();
	}
}

Vector3 WormEnemy::GetPlayerCameraPos() const
{
	//shared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//カメラの位置を取得して返す
	return pPlayer->GetCameraPos();
}
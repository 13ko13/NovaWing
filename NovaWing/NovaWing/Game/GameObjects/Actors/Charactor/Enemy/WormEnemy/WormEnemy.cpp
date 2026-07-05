#include "WormEnemy.h"
#include "Charactor/Player/Player.h"
#include "Manager/BulletManager.h"

namespace
{
	//頭の移動速度
	constexpr float move_speed =8.0f;
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

	//仮で位置決定
	m_pos.m_z -= 500.0f;//プレイヤーより後ろに配置
}

void WormEnemy::Update()
{
	//フレーム更新
	m_frame++;

	//螺旋状に回転させる
	m_rotationAngle += rot_speed;
	//Z軸を中心に螺旋回転させる
	Quaternion rotZ = Quaternion(
			Vector3(0.0f, 0.0f, 1.0f),
			m_rotationAngle * DX_PI_F / 180.0f
	);
	m_rotation = rotZ;

	//頭の移動を螺旋状にする
	m_pos.m_z += move_speed;
	m_pos.m_x = cosf(m_rotationAngle * DX_PI_F / 180.0f) * spiral_radius;
	m_pos.m_y = sinf(m_rotationAngle * DX_PI_F / 180.0f) * spiral_radius;

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

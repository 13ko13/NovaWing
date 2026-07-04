#pragma once
#include <memory>
#include <vector>

#include "Charactor/Charactor.h"
#include "Utility/Sphere.h"

class Player;
class BulletManager;
class WormEnemy : public Charactor
{
public:
	/// <summary>
	/// ワームエネミーのコンストラクタ
	/// </summary>
	/// <param name="pPlayer">プレイヤーのポインタ</param>
	/// <param name="Id">モデルのID</param>
	/// <param name="pBulletManager">弾の管理者</param>
	/// <param name="segmentCount">胴体の数</param>
	WormEnemy(const std::weak_ptr<Player> pPlayer,
		const ResourceLoader::ModelID Id,
		const std::shared_ptr<BulletManager> pBulletManager,
		int segmentCount);
	~WormEnemy();

	void OnInit() override;//初期化処理
	void Update() override;//更新処理
	void Draw() override;//描画処理
	
	void TakeDamage(int damage) override;//ダメージを受ける
	//頭の当たり判定(球)を取得
	const Sphere& GetHeadSphere() const { return m_headSphere; }

	const std::vector<Sphere>& GetSegmentSpheres() const { return m_segmentSpheres; }

private:
	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
	//弾管理者
	std::weak_ptr<BulletManager> m_pBulletManager;
	//胴体の数
	int m_segmentCount = 0;
	//螺旋移動用
	float m_rotationAngle = 0.0f;
	//弾発射タイミング用のフレーム
	int m_frame = 0;

	Sphere m_headSphere;//当たり判定(球)
	std::vector<Vector3> m_segmentPositions;//胴体一つ一つの位置
	std::vector<Sphere> m_segmentSpheres;//胴体一つ一つの当たり判定球
};


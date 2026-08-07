#pragma once
#include <memory>
#include <vector>

#include "Utility/Sphere.h"
#include "Utility/Vector2.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"

class Player;
class BulletManager;
class CameraBase;
class WormEnemy : public EnemyBase
{
public:
	struct WormEnemyData
	{
		ResourceLoader::ModelID modelID;//モデルID
		int segmentCount;//胴体の数
		Vector3 pos;//初期位置
		float direction;//移動方向
		float activatePlayerZ;//プレイヤーがどのあたり(Z値)まで来たら動き出すか
	};

public:
	WormEnemy(const std::weak_ptr<Player> pPlayer,//プレイヤー
		const std::shared_ptr<BulletManager> pBulletManager,//バレットマネージャー
		std::weak_ptr<CameraBase> camera,//カメラ
		const WormEnemyData& data//ワームエネミーに必要なデータ
	);
	~WormEnemy();

	void OnInit() override;//初期化処理
	void Update() override;//更新処理
	//描画処理
	void Draw() override;
	void DrawWormHead();
	void DrawWormBody();
	
	void TakeDamage(int damage) override;//ダメージを受ける
	//頭の当たり判定(球)を取得
	const Sphere& GetHeadSphere() const { return m_headSphere; }
	//胴体の当たり判定(球)を取得
	const std::vector<Sphere>& GetSegmentSpheres() const { return m_segmentSpheres; }

private:
	//胴体の数
	int m_segmentCount = 0;
	//螺旋移動用
	float m_rotationAngle = 0.0f;
	//間隔を決めるときのフレーム
	int m_frame = 0;

	Sphere m_headSphere;//当たり判定(球)
	std::vector<Vector3> m_segmentPositions;//胴体一つ一つの位置
	std::vector<Sphere> m_segmentSpheres;//胴体一つ一つの当たり判定球

	//頭の位置を履歴として持つ
	std::vector<Vector3> m_headHistory;

	//死亡エフェクトのプレイハンドル
	int m_deathPlayHandle = -1;
	//エフェクトを再生するかどうか
	bool m_isCanPlayEffect = false;
	//胴体の死亡エフェクトを何回出したか
	int m_deathEffectNum = 0;

	//Z+方向かZ-方向かどちらに進むかの移動方向
	float m_moveDirection = 0;//移動方向(1.0:Z+方向、-1.0:Z-方向)
	//コンストラクタで受け取ったposのx,yを螺旋の中心とする
	Vector2 m_spiralCenter;
	//プレイヤーがどの位置(Z)まで来たら動き出すか
	float m_activatePlayerZ = 0.0f;
};


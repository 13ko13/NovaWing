#pragma once
#include "Game/GameObjects/Actors/Charactor/Charactor.h"
#include "Manager/ResourceLoader.h"
#include "Utility/Sphere.h"

class Player;
class BulletManager;
class BossEnemy : public Charactor
{
public:
	BossEnemy(const std::weak_ptr<Player> pPlayer,
			  const ResourceLoader::ModelID Id,
			  const std::shared_ptr<BulletManager> pBulletManager,
			  std::weak_ptr<CameraBase> camera,
			  const Vector3& pos);
	~BossEnemy();

	void OnInit() override; // 初期化処理
	void Update() override;//更新処理
	void Draw() override;//描画処理
	void TakeDamage(int damage) override;//被弾処理

private:
	//敵の描画(シェーダ適応も含めた)
	void DrawEnemy();

private:
	std::weak_ptr<Player> m_pPlayer; // プレイヤーを借りる
	std::weak_ptr<BulletManager> m_pBulletManager; // 弾の管理者
	Sphere m_colSphere;	 // 当たり判定(球)
	//モデルの足の位置(6箇所)
	std::vector<VECTOR> m_legPositions;

	enum class LegIndex
	{
		BackRight = 1,//右後ろ脚
		MiddleRight,//右の真ん中の脚
		FrontRight,//右前脚
		BackLeft,//左後ろ脚
		MiddleLeft,//左の真ん中の脚
		FrontLeft,//左前脚

		Max,//何個あるか
	};
};
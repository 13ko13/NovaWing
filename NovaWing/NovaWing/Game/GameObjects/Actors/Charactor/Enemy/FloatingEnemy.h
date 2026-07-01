#pragma once
#include "Charactor/Charactor.h"
#include "Manager/ResourceLoader.h"
#include "Utility/Sphere.h"

class Player;
class IEnemyState;
class BulletManager;
class FloatingEnemy : public Charactor
{
public:
	FloatingEnemy(const std::weak_ptr<Player> pPlayer,
		const ResourceLoader::ModelID Id,
		const std::shared_ptr<BulletManager> pBulletManager);
	~FloatingEnemy();

	void OnInit();//初期化処理
	void Update() override;//更新処理
	void Draw() override;//描画
	void TakeDamage(int damage) override;//ダメージを受ける

	//プレイヤーの位置を返す
	Vector3 GetPlayerPos() const;
	//プレイヤーの前方向
	Vector3 GetPlayerFoward() const;

	//弾の管理者を取得する
	std::shared_ptr<BulletManager> GetBulletManager() const;

	//当たり判定(球)を取得
	const Sphere& GetSphere() const { return m_colSphere; }

private:
	std::weak_ptr<Player> m_pPlayer;//プレイヤーを借りる
	std::shared_ptr<IEnemyState> m_pState;//ステート
	std::weak_ptr<BulletManager> m_pBulletManager;//弾の管理者

	Sphere m_colSphere;//当たり判定(球)
};
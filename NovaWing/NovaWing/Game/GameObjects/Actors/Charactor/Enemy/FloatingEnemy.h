#pragma once
#include "Charactor/Charactor.h"
#include "Manager/ResourceLoader.h"
#include "Utility/Sphere.h"

class Player;
class IEnemyState;
class FloatingEnemy : public Charactor
{
public:
	FloatingEnemy(const std::weak_ptr<Player> pPlayer,
		const ResourceLoader::ModelID Id);
	~FloatingEnemy();

	void OnInit();//初期化処理
	void Update() override;//更新処理
	void Draw() override;//描画
	void TakeDamage(int damage) override;//ダメージを受ける

	//プレイヤーの位置を返す
	Vector3 GetPlayerPos() const;
	//プレイヤーの前方向
	Vector3 GetPlayerFoward() const;

private:
	std::weak_ptr<Player> m_pPlayer;//プレイヤーを借りる
	std::shared_ptr<IEnemyState> m_pState;//ステート

	Sphere m_colSphere;//当たり判定(球)
};
#pragma once
#include "Charactor/Charactor.h"
#include "Manager/ResourceLoader.h"

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

private:
	std::weak_ptr<Player> m_pPlayer;//プレイヤーを借りる
	std::shared_ptr<IEnemyState> m_pState;//ステート
};
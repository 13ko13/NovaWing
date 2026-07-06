#pragma once
#include <memory>
#include <vector>

#include "Utility/Vector3.h"

class FloatingEnemy;
class WormEnemy;
class GameObject;
class Player;
class TargetManager
{
public:
	TargetManager(std::weak_ptr<Player> pPlayer);
	~TargetManager();

	void Update();//更新処理

	//浮遊敵を配列に格納する
	void RegisterFloatingEnemy(std::shared_ptr<FloatingEnemy> pFloatingEnemy);
	//ワームエネミーを配列に格納する
	void RegisterWormEnemy(std::shared_ptr<WormEnemy> pWormEnemy);

	//レティクルの位置を取得
	Position3 GetReticlePos() const { return m_reticlePos; }
	//現在フォーカス中の敵を取得
	std::weak_ptr<GameObject> GetFocusTarget() const { return m_pFocusTarget; }
	//現在フォーカス中かを取得
	bool IsFocus() const { return m_isFocus; }

	//手前のレティクルの位置を取得する
	Vector3 GetFrontReticlePos() const { return m_frontReticlePos; }

private:
	//プレイヤー
	std::weak_ptr<Player> m_pPlayer;

	//浮遊敵の配列
	std::vector<std::weak_ptr<FloatingEnemy>> m_pFloatingEnemies;
	//ワームの配列
	std::vector<std::weak_ptr<WormEnemy>> m_pWormEnemies;

	//フォーカス中の敵
	std::weak_ptr<GameObject> m_pFocusTarget;

	//奥のレティクルの位置
	Position3 m_reticlePos;

	//フォーカス中かどうか
	bool m_isFocus = false;

	//手前のレティクルの位置
	Position3 m_frontReticlePos;
};


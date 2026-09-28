#pragma once
#include <memory>
#include <vector>

#include "Utility/Vector3.h"

class EnemyBase;
class GameObject;
class Player;
class TargetManager
{
public:
	TargetManager(std::weak_ptr<Player> pPlayer);
	~TargetManager();

	void Update();//更新処理

	//敵を配列に格納
	void Register(std::shared_ptr<EnemyBase> pEnemy);

	//レティクルの位置を取得
	Position3 GetReticlePos() const { return m_reticlePos; }
	//現在フォーカス中の敵を取得
	std::weak_ptr<EnemyBase> GetFocusTarget() const { return m_pFocusTarget; }
	//現在フォーカス中かを取得
	bool IsFocus() const { return m_isForcus; }

	//手前のレティクルの位置を取得する
	Vector3 GetFrontReticlePos() const { return m_frontReticlePos; }

	//ロック機能をONにする
	void BeginLock();
	//ロック機能をOFFにする
	void EndLock();

private:
	//物体がスクリーン上にいるか
	bool IsOnScreen(const Vector3& worldPos);

private:
	//プレイヤー
	std::weak_ptr<Player> m_pPlayer;

	//敵の配列
	std::vector<std::weak_ptr<EnemyBase>> m_pEnemies;

	//フォーカス中の敵
	std::weak_ptr<EnemyBase> m_pFocusTarget;

	//奥のレティクルの位置
	Position3 m_reticlePos;

	//フォーカス中かどうか
	bool m_isForcus = false;

	//手前のレティクルの位置
	Position3 m_frontReticlePos;

	//ロック機能が動作しているか
	bool m_isLocking = false;
};


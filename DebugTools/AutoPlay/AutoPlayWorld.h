#pragma once
#ifdef _DEBUG
#include <map>
#include <memory>
#include <vector>

#include "Utility/Vector3.h"

class Player;
class BossEnemy;

/// <summary>
/// AIテストプレイ用:GameObjectManagerに登録されているオブジェクトを毎フレーム読み取り、
/// AIとログが使いやすい形にまとめる(ゲーム側には何も書き込まない)
/// </summary>
class AutoPlayWorld
{
public:
	//プレイヤーに当たるとダメージになるもの、またはカウンターできるものの種類
	enum class ThreatKind
	{
		EnemyBullet,//敵弾(カウンター可)
		BossBeam,//ボスのビームの胴体
		BossBeamTip,//ボスのビームの先端(カウンター可、当たってもダメージはない)
		Worm,//ワームの頭・胴体
		Rock,//岩
	};

	//危険物1つ分(球1つ分)
	struct Threat
	{
		ThreatKind kind = ThreatKind::Rock;
		Vector3 pos;
		Vector3 vel;//1フレームあたりの移動量
		float radius = 0.0f;
		const void* key = nullptr;//同じものを毎フレーム見分けるための値
		bool isDead = false;//このフレームに消えたもの(当たった直後の弾など)
	};

	//倒せる敵1体分(ボス以外)
	struct Enemy
	{
		Vector3 pos;
		float radius = 0.0f;
		bool isWorm = false;
	};

	AutoPlayWorld();
	virtual ~AutoPlayWorld();

	//GameObjectManagerから今の状態を読み直す
	void Refresh();
	//読み取ったオブジェクトへの参照を手放す(シーン切り替えやゲーム終了をじゃましないよう毎フレーム呼ぶ)
	void ReleaseObjects();

	//プレイヤー(ゲームシーン以外ではnullptr)
	const std::shared_ptr<Player>& GetPlayer() const { return m_pPlayer; }
	//ボス(いなければnullptr)
	const std::shared_ptr<BossEnemy>& GetBoss() const { return m_pBoss; }
	//危険物
	const std::vector<Threat>& GetThreats() const { return m_threats; }
	//倒せる敵(ボス以外)
	const std::vector<Enemy>& GetEnemies() const { return m_enemies; }
	//まだ出現していない(弾が当たらない)ものも含めた、ボス以外の全ての敵の位置(ステージ構成の記録用)
	const std::vector<Enemy>& GetAllEnemyPositions() const { return m_allEnemyPositions; }
	//岩があるか(通常ステージが始まっているかの目印)
	bool HasRocks() const { return m_hasRocks; }

private:
	//前フレームの位置から1フレームあたりの移動量を求める
	Vector3 EstimateVelocity(const void* key, const Vector3& pos);

private:
	std::shared_ptr<Player> m_pPlayer;
	std::shared_ptr<BossEnemy> m_pBoss;
	std::vector<Threat> m_threats;
	std::vector<Enemy> m_enemies;
	std::vector<Enemy> m_allEnemyPositions;
	bool m_hasRocks = false;

	//速度を求めるための前フレームの位置(キーは球のアドレス)
	std::map<const void*, Vector3> m_prevPositions;
	std::map<const void*, Vector3> m_currentPositions;
};
#endif

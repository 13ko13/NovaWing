#pragma once
#include <vector>
#include <memory>
#include "Manager/ResourceLoader.h"

class PlayerBullet;
class EnemyBullet;
class ChargeBullet;
class GameObject;
struct Vector3;
class BulletBase;
class EnemyBase;
class BulletManager
{
public:
	BulletManager();
	~BulletManager();

	//更新処理
	void Update();

	//弾の種類
	enum class BulletType
	{
		PlayerBullet,//プレイヤーの弾
		ChargeBullet,//プレイヤーのチャージ弾
		EnemyBullet,//敵の弾
	};

	//別クラスから種類を指定してもらってその弾を作成する
	//ターゲットはデフォルトではNull,チャージショットの時のみターゲットを設定する
	void CreateBullet(const BulletType bulletType, const Vector3& pos,
		const Vector3& vel, const int attackPower,
		std::weak_ptr<EnemyBase> pTarget = std::weak_ptr<EnemyBase>());

	//プレイヤー弾の配列のゲッター
	const std::vector<std::weak_ptr<PlayerBullet>>& GetPlayerBullets() const { return m_pPlayerBullets; }
	//チャージ弾の配列のゲッター
	const std::vector<std::weak_ptr<ChargeBullet>>& GetChargeBullets() const { return m_pChargeBullets; }
	//敵弾の配列のゲッター
	const std::vector<std::weak_ptr<EnemyBullet>>& GetEnemyBullets() const { return m_pEnemyBullets; }

private:
	//敵弾の配列
	std::vector<std::weak_ptr<EnemyBullet>> m_pEnemyBullets;
	//プレイヤーの弾の配列
	std::vector<std::weak_ptr<PlayerBullet>> m_pPlayerBullets;
	//チャージ弾の配列
	std::vector<std::weak_ptr<ChargeBullet>> m_pChargeBullets;

	//弾全ての配列
	std::vector<std::weak_ptr<BulletBase>> m_pAllBullets;
};
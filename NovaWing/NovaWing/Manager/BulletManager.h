#pragma once
#include <vector>
#include <memory>
#include "Manager/ResourceLoader.h"

class PlayerBullet;
class EnemyBullet;
struct Vector3;
class BulletManager
{
public:
	BulletManager();
	~BulletManager();

	//弾の種類
	enum class BulletType
	{
		PlayerBullet,//プレイヤーの弾
		EnemyBullet,//敵の弾
	};

	//別クラスから種類を指定してもらってその弾を作成する
	void CreateBullet(const BulletType bulletType, const Vector3& pos,
		const Vector3& vel, const int attackPower);

	//プレイヤー弾の配列のゲッター
	const std::vector<std::weak_ptr<PlayerBullet>>& GetPlayerBullets() const;
	//敵弾の配列のゲッター
	const std::vector<std::weak_ptr<EnemyBullet>>& GetEnemyBullets() const;

private:
	//敵弾の配列
	std::vector<std::weak_ptr<EnemyBullet>> m_pEnemyBullets;
	//プレイヤーの弾の配列
	std::vector<std::weak_ptr<PlayerBullet>> m_pPlayerBullets;
};
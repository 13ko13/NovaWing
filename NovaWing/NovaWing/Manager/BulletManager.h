#pragma once
#include <vector>
#include <memory>
#include "Manager/ResourceLoader.h"

class BulletBase;
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
	void CreateBullet(const BulletType bulletType,const Vector3& pos,
		const Vector3& vel,const int attackPower,ResourceLoader::ModelID modelId);

private:
	 //弾の配列
	std::vector < std::weak_ptr<BulletBase>> m_pBullets;
};
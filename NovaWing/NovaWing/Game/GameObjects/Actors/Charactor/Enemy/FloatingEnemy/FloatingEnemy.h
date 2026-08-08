#pragma once
#include "Manager/ResourceLoader.h"
#include "Utility/Sphere.h"
#include "Utility/ModelAnimator.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"

class Player;
class IFloatingEnemyState;
class BulletManager;
class CameraBase;
class FloatingEnemy : public EnemyBase
{
public:
	FloatingEnemy(const std::weak_ptr<Player> pPlayer,
		const ResourceLoader::ModelID Id,
		const std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<CameraBase> camera,
		const Vector3& pos);
	~FloatingEnemy();

	void OnInit() override;//初期化処理
	void Update() override;//更新処理
	//描画
	void Draw() override;
	
	void TakeDamage(int damage) override;//ダメージを受ける

	//プレイヤーの位置を返す
	Vector3 GetPlayerPos() const;
	//プレイヤーの前方向
	Vector3 GetPlayerFoward() const;

	//弾の管理者を取得する
	std::shared_ptr<BulletManager> GetBulletManager() const;

	//当たり判定(球)を取得
	const Sphere& GetSphere() const { return m_colSphere; }

	//アニメーターを取得
	//返り値の型をshared_ptrにすると
	//他のクラス(State)が呼び出すたびに
	//参照カウントが増えてModelAnimatorの
	//所有者の1人になってしまうため参照でポインタを返す
	ModelAnimator& GetAnimator() { return *m_pAnimator; }

private:
	//敵の描画(シェーダ適応も含めた)
	void DrawEnemy();

private:
	std::shared_ptr<IFloatingEnemyState> m_pState;//ステート

	Sphere m_colSphere;//当たり判定(球)

	//アニメーター
	std::shared_ptr<ModelAnimator> m_pAnimator;

	//effekseerの再生中のエフェクトのハンドル
	int m_effectPlayHandle = -1;

	//死亡状態中のフレーム計測
	int m_dyingFrame = 0;
};
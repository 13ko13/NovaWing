#pragma once
#include "IBossEnemyState.h"
#include "Utility/Vector3.h"
#include "Game/Collision/SphereShape.h"

class Player;
class BossEnemy;
class BossBeamState : public IBossEnemyState
{
public:
	BossBeamState(std::weak_ptr<BossEnemy> pBoss,
	 std::weak_ptr<Player> pPlayer);
	~BossBeamState();

	void Enter() override;
	void Update() override;
	void Exit() override;
	void Draw() override;

	//ビームの球の位置
	std::vector<std::shared_ptr<SphereShape>> GetLeftBeamSpheres() const { return m_beamSpheresL; }
	std::vector<std::shared_ptr<SphereShape>> GetRightBeamSpheres() const { return m_beamSpheresR; }
	
	//ビームを受けたときのダメージ取得
	int GetBeamDamage() const;

private:
	/// <summary>
	/// エフェクトのZ軸をビームの進行方向に向ける
	/// </summary>
	/// <param name="playH">再生中のエフェクトハンドル</param>
	/// <param name="dir">進行方向(正規化済み)</param>
	void SetBeamEffectDir(int playH, const Vector3& dir);

	//ビームの反射が開始されたときの関数
	void OnReflectLeft(const Vector3& hitPos);
	//ビームの反射が開始されたときの関数
	void OnReflectRight(const Vector3& hitPos);

private:
	//ビームのエフェクト再生ハンドル
	int m_rightBeamEffectPlayH = -1;
	int m_leftBeamEffectPlayH = -1;

	//ビームを続けるときの時間管理用
	int m_beamFrame = 0;

	//発射口の位置
	Vector3 m_muzzlePosR;//右の発射口
	Vector3 m_muzzlePosL;//左の発射口

	//ビーム位置
	Vector3 m_beamPosR;//右のビームの先端位置
	Vector3 m_beamPosL;//左のビームの先端位置
	//前フレームのビームの位置
	Vector3 m_prevBeamPosR;//前フレームの右のビームの先端位置
	Vector3 m_prevBeamPosL;//前フレームの左のビームの先端位置

	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;

	//ビームの判定用球
	std::vector<std::shared_ptr<SphereShape>> m_beamSpheresL;
	std::vector<std::shared_ptr<SphereShape>> m_beamSpheresR;

	//ビームがプレイヤーを越えた後の進む方向
	Vector3 m_beamMoveDirR;
	Vector3 m_beamMoveDirL;

	//左のビームが反射されたか
	bool m_isReflectedL = false;
	//右のビームが反射されたか
	bool m_isReflectedR = false;

#ifdef _DEBUG
	//ビームの目標地点
	std::shared_ptr<SphereShape> m_targetSphereL = std::make_shared<SphereShape>();
	std::shared_ptr<SphereShape> m_targetSphereR = std::make_shared<SphereShape>();
	//ビームの先端位置
	std::shared_ptr<SphereShape> m_beamTipSphereL = std::make_shared<SphereShape>();
	std::shared_ptr<SphereShape> m_beamTipSphereR = std::make_shared<SphereShape>();
#endif
};


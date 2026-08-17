#pragma once
#include "IBossEnemyState.h"
#include "Utility/Vector3.h"
#include "Utility/Sphere.h"

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
	std::vector<Sphere> GetLeftBeamSpheres() const { return m_beamSpheresL; }
	std::vector<Sphere> GetRightBeamSpheres() const { return m_beamSpheresR; }
	
	//ビームを受けたときのダメージ取得
	int GetBeamDamage() const;

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
	std::vector<Sphere> m_beamSpheresL;
	std::vector<Sphere> m_beamSpheresR;

	//ビームがプレイヤーを越えた後の進む方向
	Vector3 m_beamMoveDirR;
	Vector3 m_beamMoveDirL;

#ifdef _DEBUG
	//ビームの目標地点
	Sphere m_targetSphereL;
	Sphere m_targetSphereR;
	//ビームの先端位置
	Sphere m_beamTipSphereL;
	Sphere m_beamTipSphereR;
#endif
};


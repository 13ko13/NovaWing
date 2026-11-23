#pragma once
#include "IGunshipBossState.h"
#include "Utility/Vector3.h"

class ModelAnimator;
class GunshipBossNetState : public IGunshipBossState
{
public:
	GunshipBossNetState(std::weak_ptr<GunshipBoss> pBoss);
	virtual ~GunshipBossNetState();

	void Enter() override;
	void Update() override;
	void Draw() override;
	void Exit() override;
	
	private:
	//網攻撃の準備の更新処理
	void UpdateNetReady();
	//網攻撃の発射の更新処理
	void UpdateNetFire();
	//放熱開始中の更新処理
	void UpdateVentOpen();
	//放熱中の更新処理
	void UpdateVentLoop();
	//放熱終了中の更新処理
	void UpdateVentClose();
	
	//発射口のフレーム名からワールド座標をとる
	Vector3 GetMuzzlePos(const wchar_t* frameName) const;
	//網を描画する
	void DrawNetLines(unsigned int lineCol,
		const Vector3& muzzlePosLeft,
		const Vector3& muzzlePosRight
	);

private:
	//網を発射するまでの流れ
	enum class NetPhase
	{
		Ready,//準備
		Fire,//発射
		VentOpen,//放熱(後隙)開始
		VentLoop,//放熱中
		VentClose,//放熱終了
	};
	//現在の網攻撃までの段階
	NetPhase m_phase = NetPhase::Ready;

	//ボス側から受け取るアニメーター
	ModelAnimator& m_animator;

	//放熱中のフレーム計測
	int m_ventFrame = 0;

	//網攻撃の網を置いた場所の中心位置
	Vector3 m_netPos;
	//網攻撃が始まってからの経過フレーム
	int m_netFrame = 0;
};

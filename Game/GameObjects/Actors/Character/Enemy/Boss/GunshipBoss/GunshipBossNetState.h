#pragma once
#include <vector>

#include "IGunshipBossState.h"
#include "Utility/Vector3.h"

class ModelAnimator;
class CapsuleShape;
class EffectManager;
class GunshipBossNetState : public IGunshipBossState
{
public:
	GunshipBossNetState(std::weak_ptr<GunshipBoss> pBoss);
	virtual ~GunshipBossNetState();

	void Enter() override;
	void Update() override;
	void Draw() override;
	void Exit() override;

	//カプセルの配列を返す
	const std::vector<std::shared_ptr<CapsuleShape>>& GetNetCapsules() const { return m_netCapsules; }

	//判定が出ているかを返す
	bool IsNetHitActive() const;

private:
	//網攻撃の当たり判定初期化
	void InitNetCol();
	
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
	//網の予測線用のエフェクト再生ハンドル
	int m_warningEffectPlayH = -1;
	
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

	//網用にカプセルの当たり判定を配列で持つ
	std::vector<std::shared_ptr<CapsuleShape>> m_netCapsules;

	//エフェクトマネージャーの弱参照を持つ
	std::weak_ptr<EffectManager> m_pEffectManager;
};

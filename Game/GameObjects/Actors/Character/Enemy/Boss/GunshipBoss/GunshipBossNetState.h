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
	//渡されたfor文の添え字が偶数なら右、奇数なら左の発射口の位置を返す
	Vector3 GetStreakStartPos(const size_t i) const;
	//網を描画する
	void DrawNetLines(unsigned int lineCol,
		const Vector3& muzzlePosLeft,
		const Vector3& muzzlePosRight
	);

	//網の予測線のエフェクトを全部再生する
	void PlayWarningEffects();
	//網の予測線のエフェクトを全部止める
	void StopWarningEffects();
	//網の本番のエフェクトを全部再生する
	void PlayLaserEffects();
	//網の本番のエフェクトを全部止める
	void StopLaserEffects();
	//網の本番用のエフェクトを更新する
	void UpdateLaserEffects();

private:
	//網の予測線用のエフェクト再生ハンドル
	std::vector<int> m_warningEffectHandles;
	//網の本番用のエフェクト再生ハンドル
	std::vector<int> m_netLaserEffectHandles;
	//発射口から本番の線の先端へ伸びる光の筋のエフェクトの再生ハンドル(網の線と同じ順番)
	std::vector<int> m_streakEffectHandles;

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

	//割合の計算に使用するために網の線の終点を配列で持つ
	std::vector<Vector3> m_lineEndPositions;
};

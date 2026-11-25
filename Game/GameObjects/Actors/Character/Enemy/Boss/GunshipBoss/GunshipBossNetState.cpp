#include <algorithm>

#include "GunshipBossNetState.h"
#include "GunshipBoss.h"
#include "Utility/ModelAnimator.h"
#include "GunshipBossMoveState.h"
#include "Game/Collision/CapsuleShape.h"
#include "Manager/EffectManager.h"
#include "Manager/DebugManager.h"

namespace
{
	//網攻撃の準備のアニメーション名
	constexpr const wchar_t* net_ready_anim_name = L"Boss2_Armature|NetReady";
	//網攻撃の発射のアニメーション名
	constexpr const wchar_t* net_fire_anim_name = L"Boss2_Armature|NetFire";
	//放熱開始(行動後の隙)のアニメーション名
	constexpr const wchar_t* vent_open_anim_name = L"Boss2_Armature|VentOpen";
	//放熱状態のループ用アニメーション名
	constexpr const wchar_t* vent_loop_anim_name = L"Boss2_Armature|VentLoop";
	//放熱終了用のアニメーション名
	constexpr const wchar_t* vent_close_anim_name = L"Boss2_Armature|VentClose";

	//準備アニメーションの再生スピード
	constexpr float net_ready_anim_speed = 0.5f;

	//放熱をどのぐらいのフレーム行うか
	constexpr int vent_loop_frame = 180;

	//網攻撃の線の数
	constexpr int net_line_num = 6;
	//網攻撃の線をy方向にずらす量
	constexpr int net_line_interval = 400;
	//網の長さ
	constexpr float net_half_width = 2500;
	//網の中心の高さ
	constexpr float net_center_y = 500.0f;
	//網のビームを出すボーンの名前(左)
	constexpr const wchar_t* muzzle_frame_name_left = L"Muzzle_Net_L";
	//網のビームを出すボーンの名前(右)
	constexpr const wchar_t* muzzle_frame_name_right = L"Muzzle_Net_R";

	//どのぐらいの間網を出し続けるか
	constexpr int net_active_frame = 120;

	//線の判定の半径
	constexpr float net_line_radius = 30.0f;

	//網攻撃の本番のレーザーが伸びきるフレーム数
	constexpr int laser_extend_frame = 20;

	//網をボスのどのぐらい前に出すか
	constexpr float net_pos_offset_z = 1200.0f;
}

GunshipBossNetState::GunshipBossNetState(std::weak_ptr<GunshipBoss> pBoss) :
	IGunshipBossState(pBoss),
	m_animator(pBoss.lock()->GetAnimator())
{

}

GunshipBossNetState::~GunshipBossNetState()
{
}

void GunshipBossNetState::Enter()
{
	//網攻撃の準備アニメーション再生
	m_animator.Play(net_ready_anim_name, false, net_ready_anim_speed);
	//網の位置を発射時のボスの位置に置いておく
	m_netPos = m_pBoss.lock()->GetPos();

	//当たり判定を線の数ぶん初期化する
	InitNetCol();

	//エフェクトのマネージャーを保持しておく
	m_pEffectManager = m_pBoss.lock()->GetEffectManager();

	//すべての予測線のエフェクトを再生
	PlayWarningEffects();
}

void GunshipBossNetState::InitNetCol()
{
	for (int i = -net_line_num; i < net_line_num; i++)
	{
		//1回ごとに線Aと線Bの始点と終点を出す
		//Y座標は一定間隔で配置
		float lineStartY = net_center_y + net_half_width + i * net_line_interval;

		//網に使用する2本の線の始点
		Vector3 lineStartA = Vector3(
			m_netPos.x + net_half_width,
			lineStartY, m_netPos.z - net_pos_offset_z
		);
		Vector3 lineStartB = Vector3(
			m_netPos.x - net_half_width,
			lineStartY, m_netPos.z - net_pos_offset_z
		);
		//網に使用する2本の線の終点
		Vector3 lineEndA = Vector3(
			m_netPos.x - net_half_width,
			net_center_y - net_half_width + i * net_line_interval,
			m_netPos.z - net_pos_offset_z
		);
		Vector3 lineEndB = Vector3(
			m_netPos.x + net_half_width,
			net_center_y - net_half_width + i * net_line_interval,
			m_netPos.z - net_pos_offset_z
		);

		//始点と終点と半径をカプセルに渡して初期化させる
		m_netCapsules.push_back(
			std::make_shared<CapsuleShape>(lineStartA, lineEndA, net_line_radius)
		);
		m_netCapsules.push_back(
			std::make_shared<CapsuleShape>(lineStartB, lineEndB, net_line_radius)
		);
		//計算に使用するために終点を保存しておく
		m_lineEndPositions.push_back(lineEndA);
		m_lineEndPositions.push_back(lineEndB);
	}
}

void GunshipBossNetState::Update()
{
	//準備中は準備が始まってから計測
	//そのあとは発射からのフレームを計測
	m_netFrame++;

	switch (m_phase)
	{
	case NetPhase::Ready:
		//網攻撃の準備更新
		UpdateNetReady();
		break;

	case NetPhase::Fire:
		//網攻撃発射更新
		UpdateNetFire();
		break;

	case NetPhase::VentOpen:
		//放熱(後隙)開始中の更新処理
		UpdateVentOpen();
		break;

	case NetPhase::VentLoop:
		//放熱中の更新処理
		UpdateVentLoop();
		break;

	case NetPhase::VentClose:
		//放熱終了中の更新処理
		UpdateVentClose();
		break;

	}

	//本番用のエフェクトを更新する
	UpdateLaserEffects();
	//網の判定が消えたら本番用のエフェクトを止める
	if (!IsNetHitActive()) StopLaserEffects();
}

void GunshipBossNetState::Draw()
{
	//網が完成し終わったら描画しない
	if (m_phase != NetPhase::Ready && !IsNetHitActive()) return;

#ifdef _DEBUG
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		//デバッグとして当たり判定を描画
		for (const auto& capsule : m_netCapsules)
		{
			capsule->Draw(0xff0000);
		}
	}
#endif 
}

void GunshipBossNetState::DrawNetLines(unsigned int lineCol,
	const Vector3& muzzlePosLeft, const Vector3& muzzlePosRight)
{
	//現在の網の完成度を割合で計算
	float extendRate = 1.0f;
	//準備中のみアニメーションに合わせた割合で行う
	if (m_phase == NetPhase::Ready)
	{
		extendRate = m_animator.GetPlayTime() / m_animator.GetAnimLength();
	}
	if (extendRate > 1.0) extendRate = 1.0f;

	//線を描く
	//網のようにする
	//カプセルの配列を回す
	//添字を2ずつ増やしていくことで奇数か偶数かを調べずに行うことができる
	for (size_t i = 0; i < m_netCapsules.size(); i += 2)
	{
		//カプセルから始点と終点を取得
		Vector3 lineStartA;
		Vector3 lineEndA;
		Vector3 lineStartB;
		Vector3 lineEndB;
		//右の発射口
		lineStartA = m_netCapsules[i]->GetStartPos();
		lineEndA = m_netCapsules[i]->GetEndPos();
		//左の発射口
		lineStartB = m_netCapsules[i + 1]->GetStartPos();
		lineEndB = m_netCapsules[i + 1]->GetEndPos();

		//右上から左下にかけて伸びる線
		DrawLine3D(
			lineStartA.ToDxLib(),
			Vector3::Lerp(lineStartA, lineEndA, extendRate).ToDxLib(),
			lineCol
		);
		//左上から右下にかけて伸びる線
		DrawLine3D(
			lineStartB.ToDxLib(),
			Vector3::Lerp(lineStartB, lineEndB, extendRate).ToDxLib(),
			lineCol
		);
		//発射口から網まで伸びる線
		DrawLine3D(
			muzzlePosRight.ToDxLib(),
			Vector3::Lerp(lineStartA, lineEndA, extendRate).ToDxLib(), 0xffff00
		);
		DrawLine3D(
			muzzlePosLeft.ToDxLib(),
			Vector3::Lerp(lineStartB, lineEndB, extendRate).ToDxLib(), 0xffff00
		);
	}
}

void GunshipBossNetState::PlayWarningEffects()
{
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
	if (pEffectManager == nullptr) return;

	//網の予測線のエフェクトをまとめて再生する
	for (const auto& capsule : m_netCapsules)
	{
		//再生したエフェクトのハンドルを一度ローカル変数に保存
		int playHandle = pEffectManager->Play(
			ResourceLoader::EffectID::BossNetWarning, capsule->GetStartPos());

		//エフェクトがx方向に伸びるようになっているので
		//網が実際に伸びる方向を渡してz軸回りに回す
		//線の向きのベクトルを出す
		Vector3 netStartToEnd = capsule->GetEndPos() - capsule->GetStartPos();
		//atan2fを使用して角度を求める
		float angle = std::atan2f(netStartToEnd.y, netStartToEnd.x);
		pEffectManager->SetRotationAxis(playHandle, Vector3(0.0f, 0.0f, 1.0f), angle);
		//エフェクトを仮の長さではなく実際の長さで出すようにする
		float length = netStartToEnd.Length();
		//長さをエフェクトに渡す
		pEffectManager->SetDynamicInput(playHandle, 0, length);

		//回転させたエフェクトの再生ハンドルを配列にいれる
		m_warningEffectHandles.push_back(playHandle);
	}
}

void GunshipBossNetState::StopWarningEffects()
{
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
	if (pEffectManager == nullptr) return;

	//すべての網の予測線のエフェクトを止める
	for (const auto& effectHandle : m_warningEffectHandles)
	{
		pEffectManager->Stop(effectHandle);
	}
	m_warningEffectHandles.clear();
}

void GunshipBossNetState::PlayLaserEffects()
{
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
	if (pEffectManager == nullptr) return;

	//網の本番用のエフェクトをまとめて再生する
	for (const auto& capsule : m_netCapsules)
	{
		//再生したエフェクトのハンドルを一度ローカル変数に保存
		int playHandle = pEffectManager->Play(
			ResourceLoader::EffectID::BossNetLaser, capsule->GetStartPos());

		//エフェクトがx方向に伸びるようになっているので
		//網が実際に伸びる方向を渡してz軸回りに回す
		//線の向きのベクトルを出す
		Vector3 netStartToEnd = capsule->GetEndPos() - capsule->GetStartPos();
		//atan2fを使用して角度を求める
		float angle = std::atan2f(netStartToEnd.y, netStartToEnd.x);
		pEffectManager->SetRotationAxis(playHandle, Vector3(0.0f, 0.0f, 1.0f), angle);
		//エフェクトを仮の長さではなく実際の長さで出すようにする
		float length = netStartToEnd.Length();
		//レーザーの進捗をエフェクトに渡す
		pEffectManager->SetDynamicInput(playHandle, 0, 0.0f);
		//長さをエフェクトに渡す
		pEffectManager->SetDynamicInput(playHandle, 1, length);

		//回転させたエフェクトの再生ハンドルを配列にいれる
		m_netLaserEffectHandles.push_back(playHandle);
	}

	//発射口からの光の筋を出す
	for (size_t i = 0; i < m_netCapsules.size(); i++)
	{
		//光の筋のエフェクトを再生
		int playHandle = pEffectManager->Play(
			ResourceLoader::EffectID::BossNetStreak, GetStreakStartPos(i));
		//エフェクトのハンドルを保存
		m_streakEffectHandles.push_back(playHandle);
	}
}

void GunshipBossNetState::StopLaserEffects()
{
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
	if (pEffectManager == nullptr) return;

	//すべての網の本番用のエフェクトを止める
	for (const auto& effectHandle : m_netLaserEffectHandles)
	{
		pEffectManager->Stop(effectHandle);
	}
	m_netLaserEffectHandles.clear();

	//光の筋のエフェクトを全部止める
	for (const auto& effectHandle : m_streakEffectHandles)
	{
		//エフェクトを止める
		pEffectManager->Stop(effectHandle);
	}
	m_streakEffectHandles.clear();
}

void GunshipBossNetState::UpdateLaserEffects()
{
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
	if (pEffectManager == nullptr) return;

	//現在の本番用の線の進捗を0~1で計算
	float progress = static_cast<float>(m_netFrame) / laser_extend_frame;
	progress = std::clamp(progress, 0.0f, 1.0f);

	//毎フレームエフェクトに進捗を渡してあげる
	for (size_t i = 0; i < m_netLaserEffectHandles.size(); i++)
	{
		pEffectManager->SetDynamicInput(m_netLaserEffectHandles[i], 0, progress);

		//カプセルを線の進捗に合わせて伸びるようにする
		//新しい終点を計算
		Vector3 newEndPos = Vector3::Lerp(
			m_netCapsules[i]->GetStartPos(), m_lineEndPositions[i], progress);
		//終点位置を再設定
		m_netCapsules[i]->Update(m_netCapsules[i]->GetStartPos(), newEndPos, net_line_radius);

		//光の筋のエフェクトの位置を発射口の位置にする
		pEffectManager->SetPos(m_streakEffectHandles[i], GetStreakStartPos(i));
		//発射口からカプセルの終点までのベクトルを出す
		Vector3 streakVec = (newEndPos - GetStreakStartPos(i));
		//発射口からカプセルの終点までの長さを出す
		float length = streakVec.Length();
		//方向も出しておく
		Vector3 streakDir = streakVec.Normalized();

		//どの軸に回すか
		Vector3 axis = Vector3::Cross(Vector3(1.0f, 0.0f, 0.0f), streakDir);
		axis.Normalize();

		//どのぐらい回す必要があるかを計算する
		float cosAngle = Vector3::Dot(streakDir, Vector3(1.0f, 0.0f, 0.0f));
		cosAngle = std::clamp(cosAngle, -1.0f, 1.0f);
		//acosfを使ってCosθからθを取り出す
		float angle = std::acosf(cosAngle);

		//光の筋を回す
		pEffectManager->SetRotationAxis(m_streakEffectHandles[i], axis, angle);
		//光の筋の長さも設定
		pEffectManager->SetDynamicInput(m_streakEffectHandles[i], 0, length);
	}
}

bool GunshipBossNetState::IsNetHitActive() const
{
	//網の判定が出ている間かを返す
	return m_netFrame < net_active_frame && m_phase != NetPhase::Ready;
}

void GunshipBossNetState::Exit()
{
	//予測線のエフェクトを全部止める
	StopWarningEffects();
	//本番用のエフェクトを全部止める
	StopLaserEffects();
}

void GunshipBossNetState::UpdateNetReady()
{
	if (m_animator.IsEnd())
	{
		m_animator.Play(net_fire_anim_name, false);
		m_phase = NetPhase::Fire;
		//網用のフレームをリセットする
		m_netFrame = 0;
		//予測線のエフェクトを全部止める
		StopWarningEffects();
		//本番用のエフェクトを全部再生
		PlayLaserEffects();
	}
}

void GunshipBossNetState::UpdateNetFire()
{
	if (m_animator.IsEnd())
	{
		m_animator.Play(vent_open_anim_name, false);
		m_phase = NetPhase::VentOpen;
	}
}

void GunshipBossNetState::UpdateVentOpen()
{
	if (m_animator.IsEnd())
	{
		m_animator.Play(vent_loop_anim_name, true);
		m_phase = NetPhase::VentLoop;
	}
}

void GunshipBossNetState::UpdateVentLoop()
{
	//後隙中のフレーム更新
	m_ventFrame++;

	//後隙の時間が終了したら放熱を終了する
	if (m_ventFrame >= vent_loop_frame)
	{
		m_animator.Play(vent_close_anim_name, false);
		m_phase = NetPhase::VentClose;
	}
}

void GunshipBossNetState::UpdateVentClose()
{
	if (m_animator.IsEnd())
	{
		//アニメーションが終わったらMoveStateに戻す
		ChangeState(std::make_shared<GunshipBossMoveState>(m_pBoss));
	}
}

Vector3 GunshipBossNetState::GetMuzzlePos(const wchar_t* frameName) const
{
	//ボスのモデルハンドル
	int bossModelH = m_pBoss.lock()->GetModelHandle();

	//発射口のフレームの位置を取得して返す
	Vector3 muzzlePos;
	muzzlePos = MV1GetFramePosition(
	   bossModelH,
	   MV1SearchFrame(bossModelH, frameName)
	);

	return muzzlePos;
}

Vector3 GunshipBossNetState::GetStreakStartPos(const size_t i) const
{
	Vector3 streakStartPos;

	//偶数だったら右の発射口、奇数なら左の発射口
	if (i % 2 == 0)
	{
		streakStartPos = GetMuzzlePos(muzzle_frame_name_right);
	}
	else
	{
		streakStartPos = GetMuzzlePos(muzzle_frame_name_left);
	}

	return streakStartPos;
}

#include "GunshipBossNetState.h"
#include "GunshipBoss.h"
#include "Utility/ModelAnimator.h"
#include "GunshipBossMoveState.h"

namespace
{
	//網攻撃の準備のアニメーション名
	constexpr const wchar_t* net_ready_anim_name = L"Boss2_Armature|NetReady";
	//準備アニメーションの再生スピード
	constexpr float net_ready_anim_speed = 0.3f;
	//網攻撃の発射のアニメーション名
	constexpr const wchar_t* net_fire_anim_name = L"Boss2_Armature|NetFire";
	//放熱開始(行動後の隙)のアニメーション名
	constexpr const wchar_t* vent_open_anim_name = L"Boss2_Armature|VentOpen";
	//放熱状態のループ用アニメーション名
	constexpr const wchar_t* vent_loop_anim_name = L"Boss2_Armature|VentLoop";
	//放熱終了用のアニメーション名
	constexpr const wchar_t* vent_close_anim_name = L"Boss2_Armature|VentClose";

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

	//何フレームかけて網が伸び終わるか
	constexpr int net_extend_frame = 120;

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
	m_animator.Play(net_ready_anim_name, false,net_ready_anim_speed);
	//網の位置を発射時のボスの位置に置いておく
	m_netPos = m_pBoss.lock()->GetPos();
}

void GunshipBossNetState::Update()
{
	//網攻撃が始まってからのフレームを計測
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
}

void GunshipBossNetState::Draw()
{
	//網を作るためのビームの発射口のフレーム位置をとる
	Vector3 muzzlePosLeft = GetMuzzlePos(muzzle_frame_name_left);
	Vector3 muzzlePosRight = GetMuzzlePos(muzzle_frame_name_right);

	//網の色
	unsigned int lineCol = 0xffffff;
	//準備中は色を変える
	if (m_phase == NetPhase::Ready) lineCol = 0x444444;

	//網を描画する
	DrawNetLines(lineCol, muzzlePosLeft, muzzlePosRight);
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
	for (int i = -net_line_num; i < net_line_num; i++)
	{
		//Y座標は一定間隔で配置
		float lineStartY = net_center_y + net_half_width + i * net_line_interval;

		//網に使用する2本の線の始点
		Vector3 lineStartA = Vector3(
			m_netPos.x + net_half_width,
			lineStartY, m_netPos.z - 1200.0f
		);
		Vector3 lineStartB = Vector3(
			m_netPos.x - net_half_width,
			lineStartY, m_netPos.z - 1200.0f
		);
		//網に使用する2本の線の終点
		Vector3 lineEndA = Vector3(
			m_netPos.x - net_half_width,
			net_center_y - net_half_width + i * net_line_interval,
			m_netPos.z - 1200.0f
		);
		Vector3 lineEndB = Vector3(
			m_netPos.x + net_half_width,
			net_center_y - net_half_width + i * net_line_interval,
			m_netPos.z - 1200.0f
		);

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

void GunshipBossNetState::Exit()
{
}

void GunshipBossNetState::UpdateNetReady()
{
	if (m_animator.IsEnd())
	{
		m_animator.Play(net_fire_anim_name, false);
		m_phase = NetPhase::Fire;
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

#include "GunshipBossNetState.h"
#include "GunshipBoss.h"
#include "Utility/ModelAnimator.h"
#include "GunshipBossMoveState.h"

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

	//放熱をどのぐらいのフレーム行うか
	constexpr int vent_loop_frame = 180;
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
	m_animator.Play(net_ready_anim_name, false);
}

void GunshipBossNetState::Update()
{
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

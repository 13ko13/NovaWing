#include "GunshipBossRocketState.h"
#include "GunshipBoss.h"
#include "Utility/ModelAnimator.h"
#include "GunshipBossMoveState.h"

namespace
{
	//ロケットを撃つときのアニメーションの名前
	//開く
	constexpr const wchar_t* rocket_open_anim_name = L"Boss2_Armature|RocketOpen";
	//閉じる
	constexpr const wchar_t* rocket_close_anim_name = L"Boss2_Armature|RocketClose";
}

GunshipBossRocketState::GunshipBossRocketState(std::weak_ptr<GunshipBoss> pBoss) :
	IGunshipBossState(pBoss),
	m_animator(pBoss.lock()->GetAnimator())
{
}

GunshipBossRocketState::~GunshipBossRocketState()
{
}

void GunshipBossRocketState::Enter()
{
	//ロケットの発射口が開くアニメーションを再生
	m_animator.Play(rocket_open_anim_name, false);
}

void GunshipBossRocketState::Update()
{
	//現在の段階がOpenの時に開くアニメーションが
	//終わっていたら閉じるアニメーションを行う
	if (m_rocketPhase == RocketPhase::Open &&
		m_animator.IsEnd())
	{
		//閉じるアニメーションを行う
		m_animator.Play(rocket_close_anim_name, false);
		//段階をCloseにする
		m_rocketPhase = RocketPhase::Close;
	}
	//Close時にアニメーションが終わればステートをもとに戻す
	else if (m_rocketPhase == RocketPhase::Close &&
		m_animator.IsEnd())
	{
		ChangeState(std::make_shared<GunshipBossMoveState>(m_pBoss));
	}
}

void GunshipBossRocketState::Exit()
{
}

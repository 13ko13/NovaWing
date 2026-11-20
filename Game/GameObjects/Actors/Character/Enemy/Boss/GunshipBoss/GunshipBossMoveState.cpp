#define NOMINMAX
#include "GunshipBossMoveState.h"
#include "GunshipBoss.h"
#include "Utility/Easing.h"

namespace
{
	//移動にかける時間
	constexpr int move_frame = 60;
}

GunshipBossMoveState::GunshipBossMoveState(std::weak_ptr<GunshipBoss> pBoss) :
	IGunshipBossState(pBoss)
{
}

GunshipBossMoveState::~GunshipBossMoveState()
{
}

void GunshipBossMoveState::Enter()
{
	//移動の開始地点を保存
	m_startPos = m_pBoss.lock()->GetPos();
	//目標地点を入れる
	//仮で開始地点から右に1000
	m_endPos = m_startPos + Vector3(1000.0f, 0.0f, 0.0f);
	//フレームをリセット
	m_frame = 0;
}

void GunshipBossMoveState::Update()
{
	//フレームの更新
	m_frame++;

	//移動を行う
	Move();
}

void GunshipBossMoveState::Move()
{
	std::shared_ptr<GunshipBoss> pSharedBoss = m_pBoss.lock();

	//移動にかける時間から見たframeの割合を出す
	float timeRatio = static_cast<float>(m_frame) / move_frame;
	timeRatio = std::min(timeRatio, 1.0f);//1.0に制限

	//イースインアウトの関数を使って移動の割合を出す
	float moveRatio = Easing::EaseInOutQuad(timeRatio);
	
	//位置をLerpで移動の割合を使って移動させる
	Vector3 pos = Vector3::Lerp(m_startPos, m_endPos, moveRatio);
	pos.z = pSharedBoss->GetPos().z;//zだけは通常の移動
	pSharedBoss->SetPos(pos);
}

void GunshipBossMoveState::Draw()
{
}

void GunshipBossMoveState::Exit()
{
}

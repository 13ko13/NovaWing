#include <iterator>

#define NOMINMAX
#include "GunshipBossMoveState.h"
#include "GunshipBoss.h"
#include "Utility/Easing.h"

namespace
{
	//移動にかける時間
	constexpr int move_frame = 60;
	//止まっている間の時間
	constexpr int stop_frame = 120;

	//ボスが動く地点
	//時計回りに順番に回る
	const Vector3 move_point[] =
	{
		Vector3(-1400.0f,1000.0f,0.0f),//左上
		Vector3(0.0f,1100.0f,0.0f),//中央の上
		Vector3(1400.0f,1000.0f,0.0f),//右上
		Vector3(1400.0f,650.0f,0.0f),//右下
		Vector3(0.0f,600.0f,0.0f)//中央の下
	};
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
	//移動する準備を行う
	StartMove();
}

void GunshipBossMoveState::Update()
{
	//フレームの更新
	m_frame++;

	//移動している間
	if (m_isMoving)
	{
		//移動を行う
		Move();
		//移動にかける時間が過ぎたら動きを止める
		//フレームもリセット
		if (m_frame >= move_frame)
		{
			m_isMoving = false;
			m_frame = 0;
			//移動した回数を増やす
			m_moveCount++;
		}
	}
	//止まっている間
	else
	{
		//止まっている時間が終了したらまた動き出す
		//フレームもリセット
		if (m_frame >= stop_frame)
		{
			//移動する準備を行う
			StartMove();
		}
	}
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

void GunshipBossMoveState::StartMove()
{
	//開始地点を現在の位置とする
	m_startPos = m_pBoss.lock()->GetPos();

	//目標地点を入れる
	m_endPos = move_point[m_moveCount % static_cast<int>(std::size(move_point))];
	//移動する準備を行う
	m_isMoving = true;
	m_frame = 0;
}

void GunshipBossMoveState::Draw()
{
}

void GunshipBossMoveState::Exit()
{
}

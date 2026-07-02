#include "HideState.h"
#include "Utility/Vector3.h"
#include "FloatingEnemy.h"
#include "ActiveState.h"
#include "Utility/ModelAnimator.h"

namespace
{
	//出現条件の距離
	constexpr float appear_distance =500.0f;
	//待機アニメーション名前
	const wchar_t* idle_anim_name = L"01_Idle_Fly";
}

HideState::HideState(std::weak_ptr<FloatingEnemy> pEnemy):
	IEnemyState(pEnemy)
{
}

HideState::~HideState()
{
	
}

void HideState::Enter()
{
	//アニメータを取得
	ModelAnimator& animator = m_pEnemy.lock()->GetAnimator();

	//Idleアニメーション再生
	animator.Play(idle_anim_name, true);
}

void HideState::Update()
{
	//プレイヤーの位置を取得
	std::shared_ptr<FloatingEnemy> pEnemy = m_pEnemy.lock();
	Vector3 playerPos = pEnemy->GetPlayerPos();
	//敵自身の位置を取得
	Vector3 enemyPos = pEnemy->GetPos();
	//距離が一定(出現条件)以下になったらステートを変更
	//距離
	Vector3 delta = playerPos - enemyPos;
	float length = delta.Length();
	if (length < appear_distance)
	{
		//ActiveStateに変更する
		ChangeState(std::make_shared<ActiveState>(pEnemy));
	}
}

void HideState::Exit()
{
}

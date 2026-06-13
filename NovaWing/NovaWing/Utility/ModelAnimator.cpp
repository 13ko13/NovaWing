#include "ModelAnimator.h"
#include <DxLib.h>

namespace
{
	//1フレーム当たりのアニメ時間の進む量
	constexpr float anim_delta_time = 0.5f;
}

ModelAnimator::ModelAnimator(int modelHandle) :
	m_modelHandle(modelHandle)
{

}

ModelAnimator::~ModelAnimator()
{
}

void ModelAnimator::Play(int animIndex, bool isLoop,float animSpeed, float startFrame)
{
	//同じアニメーション中で、アニメーションが終了済みじゃないなら処理を飛ばす
	if (m_animIdx == animIndex && !m_isEnd) return;

	//アニメーション中なのでisEndをfalse
	m_isEnd = false;

	//再生中のアニメーション番号を更新
	m_animIdx = animIndex;
	m_isLoop = isLoop;

	//アニメーションの速度を設定
	m_animSpeed = animSpeed;

	//もしすでに前のアニメーションが残っていたら強制終了してデタッチする
	if (m_prevAttachIdx != -1)
	{
		MV1DetachAnim(m_modelHandle, m_prevAttachIdx);
	}

	//現在再生中のアニメーションを一つ前のアニメーションとする
	m_prevAttachIdx = m_currentAttachIdx;//現在のアニメーションを保存する
	m_prevTime = m_currentTime;//現在のアニメーションのカウントを保存する

	//アニメーションをアタッチ
	m_currentAttachIdx = MV1AttachAnim(m_modelHandle, animIndex);
	m_currentTime = startFrame;
	MV1SetAttachAnimTime(m_modelHandle, m_currentAttachIdx, m_currentTime);

	//ブレンド中のタイマーをリセット
	m_blendTimer = 0.0f;

	//前のがあればブレンド開始、なければ（初回再生なら）即100%
	if (m_prevAttachIdx != -1) {
		m_isBlending = true;
		//新アニメは0%から、前アニメは100%からブレンドを開始する
		MV1SetAttachAnimBlendRate(m_modelHandle, m_currentAttachIdx, 0.0f);
		MV1SetAttachAnimBlendRate(m_modelHandle, m_prevAttachIdx, 1.0f);
	}
	else {
		m_isBlending = false;
		MV1SetAttachAnimBlendRate(m_modelHandle, m_currentAttachIdx, 1.0f);
	}
}

void ModelAnimator::Update(float blendTime)
{
	//アニメーションなし状態なら処理をスキップ
	if (m_currentAttachIdx < 0) return;

	m_currentTime += anim_delta_time * m_animSpeed;//アニメーションの時間を更新
	m_prevTime += anim_delta_time * m_animSpeed;

	float totalTime = MV1GetAttachAnimTotalTime(m_modelHandle, m_currentAttachIdx);
	//アニメーションがループされていて、
	//アニメーションのカウントが総カウントを越えたら
	//アニメーションのカウントをリセットする
	if (m_isLoop)
	{
		if (m_currentTime >= totalTime)
		{
			m_currentTime -= totalTime;
		}
	}
	else
	{
		//ループじゃない場合は
		//一回しか再生しない
		if (m_currentTime >= totalTime)
		{
			m_currentTime = totalTime;//クランプ
			m_isEnd = true;
		}
	}

	//アニメーションのカウントをアニメーションに反映させる
	MV1SetAttachAnimTime(m_modelHandle, m_currentAttachIdx, m_currentTime);

	//ブレンドフラグが立っていたら
	if (m_isBlending && m_prevAttachIdx != -1)
	{
		//ブレンドタイマーを更新
		m_blendTimer++;

		//ブレンド率を計算する
		float blendRate = m_blendTimer / blendTime;
		//ブレンドが終了したら
		if (blendRate >= 1.0f)
		{
			blendRate = 1.0f;//クランプ
			//フラグを降ろす
			m_isBlending = false;
			//ブレンド前アニメーションをデタッチ
			MV1DetachAnim(m_modelHandle, m_prevAttachIdx);
		}

		//計算したブレンド率をもとにアニメーションをブレンドする
		MV1SetAttachAnimBlendRate(m_modelHandle, m_currentAttachIdx, blendRate);
		MV1SetAttachAnimBlendRate(m_modelHandle, m_prevAttachIdx, 1.0f - blendRate);

		// 前アニメの時間反映（総再生時間を超えないようにクランプする）
		float prevTotalTime = MV1GetAttachAnimTotalTime(m_modelHandle, m_prevAttachIdx);
		float clampedPrevTime = (m_prevTime > prevTotalTime) ? prevTotalTime : m_prevTime;
		MV1SetAttachAnimTime(m_modelHandle, m_prevAttachIdx, clampedPrevTime);
	}
}

bool const ModelAnimator::IsEnd() const
{
	//現在再生中のアタッチ番号が無ければfalseを返す
	if (m_currentAttachIdx < 0) return false;

	return m_isEnd;
}

float ModelAnimator::GetAnimLength() const
{
	return MV1GetAttachAnimTotalTime(m_modelHandle, m_currentAttachIdx);
}

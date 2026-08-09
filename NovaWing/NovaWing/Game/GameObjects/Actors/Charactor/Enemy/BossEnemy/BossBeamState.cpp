#include <DxLib.h>
#include <algorithm>

#include "BossBeamState.h"
#include "Manager/ResourceLoader.h"
#include "BossEnemy.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "BossIdleState.h"

namespace
{
	//発射口のボーンの名前
	constexpr const wchar_t* muzzle_frame_name_r = L"ShotgunTop_R";//右
	constexpr const wchar_t* muzzle_frame_name_l = L"ShotgunTop_L";//左 
	//どのぐらいの割合で線形補間するか
	constexpr float beam_pos_ratio = 0.01f;

	//割合計算の際に0除算を避けるための閾値
	constexpr float z_diff_threshould = 0.001f;

	//ビームの当たり判定球の半径
	constexpr float beam_sphere_radius = 70.0f;

	//ビームの目標をどのぐらいプレイヤーから離すのか
	constexpr float target_offset = 500.0f;

	//何フレームビームを続けるか
	constexpr int beam_end_frame = 60 * 7;
}

BossBeamState::BossBeamState(std::weak_ptr<BossEnemy> pBoss,
	 std::weak_ptr<Player> pPlayer):
	IBossEnemyState(pBoss),
	m_pPlayer(pPlayer)
{
}

BossBeamState::~BossBeamState()
{
}

void BossBeamState::Enter()
{
	//ボスのモデルハンドル取得
	int handle = m_pBoss.lock()->GetModelHandle();

	//ボスの発射口を取得する
	//右の発射口
	m_muzzlePosR = MV1GetFramePosition(
		handle, MV1SearchFrame(handle, muzzle_frame_name_r));
	//左の発射口
	m_muzzlePosL = MV1GetFramePosition(
		handle, MV1SearchFrame(handle, muzzle_frame_name_l));

	//ビームの先端位置を発射口に設置
	m_beamPosL = m_muzzlePosL;
	m_beamPosR = m_muzzlePosR;

	//ビームの時間を初期化
	m_beamFrame = 0;
}

void BossBeamState::Update()
{
	//プレイヤーの位置を取得
	Vector3 playerPos = m_pPlayer.lock()->GetPos();
	//プレイヤーよりも後ろの方を狙わせたいのでオフセット計算
	Vector3 targetPos = playerPos +
		m_pPlayer.lock()->GetVisualBack() * target_offset;

	//ビームの先端位置からプレイヤーまでを線形補完する
	m_beamPosL = Vector3::Lerp(m_beamPosL, targetPos, beam_pos_ratio);//左
	//ビームの先端位置からプレイヤーまでを線形補完する
	m_beamPosR = Vector3::Lerp(m_beamPosR, targetPos, beam_pos_ratio);//右

	//当たり判定はプレイヤーと同じＺライン上にある
	//ビームの場所に出現させる
	//まずプレイヤーが発射口とビームの先端から見て
	//どの位置にいるかの割合を計算する(進行度の計算)
	//発射口とビームの先端の差を計算
	float leftZDiff = m_beamPosL.m_z - m_muzzlePosL.m_z;
	float rightZDiff = m_beamPosR.m_z - m_muzzlePosR.m_z;
	float playerPosRatioL = 0.0f;//左のビームから見たプレイヤーの位置割合
	float playerPosRatioR = 0.0f;//右のビームから見たプレイヤーの位置割合

	if (std::abs(leftZDiff) > z_diff_threshould)//0除算対策
	{
		playerPosRatioL = 
			(playerPos.m_z - m_muzzlePosL.m_z) / leftZDiff;//左
	}
	if (std::abs(rightZDiff) > z_diff_threshould)//0除算対策
	{
		playerPosRatioR = 
			(playerPos.m_z - m_muzzlePosR.m_z) / rightZDiff;//右
	}
	//割合を0～1にクランプ
	playerPosRatioL = std::clamp(playerPosRatioL, 0.0f, 1.0f);
	playerPosRatioR = std::clamp(playerPosRatioR, 0.0f, 1.0f);

	//当たり判定の位置を計算
	Vector3 spherePosL = Vector3::Lerp(
		m_muzzlePosL, m_beamPosL, playerPosRatioL);
	Vector3 spherePosR = Vector3::Lerp(
		m_muzzlePosR, m_beamPosR, playerPosRatioR);
	//球の位置更新
	m_beamSphereL.Update(spherePosL, beam_sphere_radius);
	m_beamSphereR.Update(spherePosR, beam_sphere_radius);

#ifdef _DEBUG
	//ビームの目標地点の球の更新
	m_targetSphereL.Update(targetPos, beam_sphere_radius);
	m_targetSphereR.Update(targetPos, beam_sphere_radius);
	m_beamTipSphereL.Update(m_beamPosL, beam_sphere_radius);
	m_beamTipSphereR.Update(m_beamPosR, beam_sphere_radius);
#endif

	//ビームは時間で終了させる
	m_beamFrame++;
	if (m_beamFrame > beam_end_frame)
	{
		//ステートをアイドルに戻す
		ChangeState(std::make_shared<BossIdleState>(m_pBoss,m_pPlayer));
	}
}

void BossBeamState::Exit()
{
}

void BossBeamState::Draw()
{
#ifdef _DEBUG
	//球のデバッグ描画
	m_beamSphereL.Draw(0x00ff00);
	m_beamSphereR.Draw(0x00ff00);
	m_targetSphereL.Draw(0x00ff00);
	m_targetSphereR.Draw(0x00ff00);
	m_beamTipSphereL.Draw(0x00ff00);
	m_beamTipSphereR.Draw(0x00ff00);
#endif
}

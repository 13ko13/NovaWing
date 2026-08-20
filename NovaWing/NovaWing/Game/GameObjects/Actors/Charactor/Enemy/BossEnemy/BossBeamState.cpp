#include <DxLib.h>
#include <algorithm>
#include <EffekseerForDXLib.h>

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
	constexpr float target_offset = 100.0f;

	//何フレームビームを続けるか
	constexpr int beam_end_frame = 60 * 7;

	//ビームがターゲットまで進むときの速度
	constexpr float beam_speed = 20.0f;

	//何フレームに一回当たり判定を出すか
	constexpr int beam_col_interval = 10;

	//ビームの当たり判定の終わりをプレイヤーのどれだけ後ろに出すか
	const Vector3 beam_hit_end_offset = Vector3(0.0f, 0.0f, -120.0f);

	//ボスのビームのダメージ
	constexpr int beam_damage = 1;
}

BossBeamState::BossBeamState(std::weak_ptr<BossEnemy> pBoss,
	 std::weak_ptr<Player> pPlayer) :
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

	int effectHandle = ResourceLoader::GetInstance().GetEffect(
		ResourceLoader::EffectID::BossBeam);

	//ビームのエフェクトを再生
	//左
	m_leftBeamEffectPlayH = PlayEffekseer3DEffect(effectHandle);
	SetPosPlayingEffekseer3DEffect(
		m_leftBeamEffectPlayH,
		m_beamPosL.m_x,
		m_beamPosL.m_y,
		m_beamPosL.m_z
	);
	//右
	m_rightBeamEffectPlayH = PlayEffekseer3DEffect(effectHandle);
	SetPosPlayingEffekseer3DEffect(
		m_rightBeamEffectPlayH,
		m_beamPosR.m_x,
		m_beamPosR.m_y,
		m_beamPosR.m_z
	);

	//ビームの進む方向初期化　
	//プレイヤーをshared_ptrに変換
	std::shared_ptr<Player> pSharedPlayer = m_pPlayer.lock();

	//プレイヤーの位置を取得
	Vector3 playerPos = pSharedPlayer->GetPos();
	//プレイヤーよりも後ろの方を狙わせたいのでオフセット計算
	Vector3 targetPos = playerPos +
		pSharedPlayer->GetVisualBack() * target_offset;
	//ビームの先端からターゲットまでの方向を計算
	Vector3 leftToTargetDir = Vector3(targetPos - m_beamPosL).Normalized();//左
	Vector3 rihgtToTargetDir = Vector3(targetPos - m_beamPosR).Normalized();//右

	//進む方向をメンバ変数にも保存しておく(Update()の追い越し判定で使うため)
	m_beamMoveDirL = leftToTargetDir;
	m_beamMoveDirR = rihgtToTargetDir;

	//ビームの先端をターゲットに向けて一定速度で進ませる 
	m_beamPosL += leftToTargetDir * beam_speed;
	m_beamPosR += rihgtToTargetDir * beam_speed;

	//配列をクリアする
	m_beamSpheresL.clear();
	m_beamSpheresR.clear();
}

void BossBeamState::Update()
{
	//前フレームのビームの先端位置を保存
	m_prevBeamPosL = m_beamPosL;
	m_prevBeamPosR = m_beamPosR;

	//プレイヤーをshared_ptrに変換
	std::shared_ptr<Player> pSharedPlayer = m_pPlayer.lock();

	//プレイヤーの位置を取得
	Vector3 playerPos = pSharedPlayer->GetPos();
	//プレイヤーよりも後ろの方を狙わせたいのでオフセット計算
	Vector3 targetPos = playerPos +
		pSharedPlayer->GetVisualBack() * target_offset;

	//もしターゲットを越えたらそこからはプレイヤーを追いかけずにその方向に進む
	if (pSharedPlayer->GetPos().m_z < m_beamPosL.m_z)
	{
		//ビームの先端からターゲットまでの方向を計算
		Vector3 leftToTargetDir = Vector3(targetPos - m_beamPosL).Normalized();//左
		//進む方向を保存しておく
		m_beamMoveDirL = leftToTargetDir;
	}
	if (pSharedPlayer->GetPos().m_z < m_beamPosR.m_z)
	{
		//ビームの先端からターゲットまでの方向を計算
		Vector3 rightToTargetDir = Vector3(targetPos - m_beamPosR).Normalized();//右
		//進む方向を保存しておく
		m_beamMoveDirR = rightToTargetDir;
	}
	//ビームの先端をターゲットに向けて一定速度で進ませる 
	//越えている場合はターゲットまでの方向が更新されないので
	//越える前までの方向が入る
	m_beamPosL += m_beamMoveDirL * beam_speed;
	m_beamPosR += m_beamMoveDirR * beam_speed;

	//プレイヤーに追いつくまでは球を数フレームに一回出し続ける
	//プレイヤーより少し後ろまで当たり判定は出しておきたいので
	//オフセットを足しておく
	Vector3 hitEndPos = playerPos + beam_hit_end_offset;

	if (m_beamPosL.m_z >= hitEndPos.m_z)
	{
		if (m_beamFrame % beam_col_interval == 0)
		{
			//球生成
			Sphere col = Sphere(m_beamPosL, beam_sphere_radius);
			//配列にいれる
			m_beamSpheresL.push_back(col);
		}
	}
	if (m_beamPosR.m_z >= hitEndPos.m_z)
	{
		if (m_beamFrame % beam_col_interval == 0)
		{
			//球生成
			Sphere col = Sphere(m_beamPosR, beam_sphere_radius);
			//配列にいれる
			m_beamSpheresR.push_back(col);
		}
	}

	//hitEndPosより後ろにある球は削除する
	m_beamSpheresL.erase(
		std::remove_if(
			m_beamSpheresL.begin(),
			m_beamSpheresL.end(),
			[hitEndPos](const Sphere sphere)
			{
				return sphere.GetPos().m_z < hitEndPos.m_z;
			}),
		m_beamSpheresL.end()
	);
	m_beamSpheresR.erase(
		std::remove_if(
			m_beamSpheresR.begin(),
			m_beamSpheresR.end(),
			[hitEndPos](const Sphere sphere)
			{
				return sphere.GetPos().m_z < hitEndPos.m_z;
			}),
		m_beamSpheresR.end()
	);

	//ビームエフェクトの位置更新
	SetPosPlayingEffekseer3DEffect(
		m_leftBeamEffectPlayH,
		m_beamPosL.m_x,
		m_beamPosL.m_y,
		m_beamPosL.m_z
	);
	SetPosPlayingEffekseer3DEffect(
		m_rightBeamEffectPlayH,
		m_beamPosR.m_x,
		m_beamPosR.m_y,
		m_beamPosR.m_z
	);

#ifdef _DEBUG
	//ビームの目標地点の球の更新
	m_targetSphereL.Update(targetPos, beam_sphere_radius);
	m_targetSphereR.Update(targetPos, beam_sphere_radius);
	/*m_beamTipSphereL.Update(m_beamPosL, beam_sphere_radius);
	m_beamTipSphereR.Update(m_beamPosR, beam_sphere_radius);*/
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
	for (Sphere& col : m_beamSpheresL)
	{
		col.Draw(0x00ff00);
	}
	for (Sphere& col : m_beamSpheresR)
	{
		col.Draw(0x00ff00);
	}
	m_targetSphereL.Draw(0x00ff00);
	m_targetSphereR.Draw(0x00ff00);
	/*m_beamTipSphereL.Draw(0x00ff00);
	m_beamTipSphereR.Draw(0x00ff00);*/
#endif
}

int BossBeamState::GetBeamDamage() const
{
	return beam_damage;
}

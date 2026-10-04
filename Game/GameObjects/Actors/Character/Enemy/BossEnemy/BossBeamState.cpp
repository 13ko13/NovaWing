#include <DxLib.h>
#include <algorithm>
#include <cmath>

#include "BossBeamState.h"
#include "Manager/ResourceLoader.h"
#include "Manager/SoundManager.h"
#include "Manager/EffectManager.h"
#include "BossEnemy.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "BossIdleState.h"
#include "Manager/DebugManager.h"

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
	constexpr int beam_damage = 20;

	//回転軸を求める際に0除算を避けるための閾値
	constexpr float rot_axis_threshould = 0.0001f;

	//ビーム反射後の1フレームあたりの最大旋回角(0.5度)
	constexpr float one_frame_turn_angle = DX_PI_F / 360.0f;

	//ボスに当たったビームが消えるまでのフレーム数
	constexpr int hit_boss_fade_frame = 30;

	//エフェクトの色の最大値(アルファのフェードの基準)
	constexpr int color_max = 255;
}

BossBeamState::BossBeamState(std::weak_ptr<BossEnemy> pBoss,
	 std::weak_ptr<Player> pPlayer) :
	IBossEnemyState(pBoss),
	m_pPlayer(pPlayer)
{
}

BossBeamState::~BossBeamState()
{
	//エフェクトを止める
	//(ボスに当たって薄くなったビームは既に止めてあり、ハンドルが-1になっている)
	//(シーン終了でマネージャーが先に消えている場合は止める必要がない)
	if (std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock())
	{
		if (m_leftBeamEffectPlayH != -1)
		{
			pEffectManager->Stop(m_leftBeamEffectPlayH);
		}
		if (m_rightBeamEffectPlayH != -1)
		{
			pEffectManager->Stop(m_rightBeamEffectPlayH);
		}
	}
}

void BossBeamState::Enter()
{
	//エフェクトのマネージャーを保持しておく
	m_pEffectManager = m_pBoss.lock()->GetEffectManager();

	//ビーム発射音を鳴らす
	m_pBoss.lock()->GetSoundManager().lock()->Play(SoundManager::SoundType::BossBeam);

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
	Vector3 rightToTargetDir = Vector3(targetPos - m_beamPosR).Normalized();//右

	//進む方向をメンバ変数にも保存しておく(Update()の追い越し判定で使うため)
	m_beamMoveDirL = leftToTargetDir;
	m_beamMoveDirR = rightToTargetDir;

	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();

	//ビームのエフェクトを再生(発射口から)
	//左
	m_leftBeamEffectPlayH = pEffectManager->Play(
		ResourceLoader::EffectID::BossBeam, m_beamPosL);
	SetBeamEffectDir(m_leftBeamEffectPlayH, m_beamMoveDirL);
	//右
	m_rightBeamEffectPlayH = pEffectManager->Play(
		ResourceLoader::EffectID::BossBeam, m_beamPosR);
	SetBeamEffectDir(m_rightBeamEffectPlayH, m_beamMoveDirR);

	//ビームの先端をターゲットに向けて一定速度で進ませる 
	m_beamPosL += leftToTargetDir * beam_speed;
	m_beamPosR += rightToTargetDir * beam_speed;

	//配列をクリアする
	m_beamSpheresL.clear();
	m_beamSpheresR.clear();

	//反射情報を初期化
	m_isReflectedL = false;
	m_isReflectedR = false;

	//ボスに当たったかの情報を初期化
	m_isHitBossL = false;
	m_isHitBossR = false;
	m_hitBossFrameL = 0;
	m_hitBossFrameR = 0;
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
	//反射された方はプレイヤーを追いかける処理を行わない
	if (!m_isReflectedL)
	{
		if (pSharedPlayer->GetPos().z < m_beamPosL.z)
		{
			//ビームの先端からターゲットまでの方向
			Vector3 leftToTargetDir = Vector3(targetPos - m_beamPosL).Normalized();//左
			//進む方向を保存しておく
			m_beamMoveDirL = leftToTargetDir;
		}
	}
	else if (!m_isHitBossL)
	{
		//反射した側はボスへのダメージ球へ曲げる
		//(ボスに当たった後は向きを変えない)
		//ボスへのベクトルをもとめる
		Vector3 toBossDir = (m_pBoss.lock()->GetDamageSphere()->GetPos() - m_beamPosL).Normalized();
		//角度を求める
		//acosは0~180度しか返さないので、dotの結果をclampしておく
		float angle = acosf(std::clamp(Vector3::Dot(m_beamMoveDirL, toBossDir), -1.0f, 1.0f));
		//上限以下だったら
		if(angle < one_frame_turn_angle)
		{
			//そのままボスの方向に向ける
			m_beamMoveDirL = toBossDir;
		}
		else
		{
			//上限より大きかったら、上限の角度だけ回転させる
			Quaternion rot = Quaternion(Vector3::Cross(m_beamMoveDirL, toBossDir).Normalized(), one_frame_turn_angle);
			m_beamMoveDirL = (rot * m_beamMoveDirL).Normalized();
		}

	}
	if (!m_isReflectedR)
	{
		if (pSharedPlayer->GetPos().z < m_beamPosR.z)
		{
			//ビームの先端からターゲットまでの方向
			Vector3 rightToTargetDir = Vector3(targetPos - m_beamPosR).Normalized();//右
			//進む方向を保存しておく
			m_beamMoveDirR = rightToTargetDir;
		}
	}
	else if (!m_isHitBossR)
	{
		//反射した側はボスへのダメージ球へ曲げる
		//(ボスに当たった後は向きを変えない)
		//ボスへのベクトルをもとめる
		Vector3 toBossDir = (m_pBoss.lock()->GetDamageSphere()->GetPos() - m_beamPosR).Normalized();
		//角度を求める
		//acosは0~180度しか返さないので、dotの結果をclampしておく
		float angle = acosf(std::clamp(Vector3::Dot(m_beamMoveDirR, toBossDir), -1.0f, 1.0f));
		//上限以下だったら
		if(angle < one_frame_turn_angle)
		{
			//そのままボスの方向に向ける
			m_beamMoveDirR = toBossDir;
		}
		else
		{
			//上限より大きかったら、上限の角度だけ回転させる
			Quaternion rot = Quaternion(Vector3::Cross(m_beamMoveDirR, toBossDir).Normalized(), one_frame_turn_angle);
			m_beamMoveDirR = (rot * m_beamMoveDirR).Normalized();
		}
	}
	
	//ビームの先端をターゲットに向けて一定速度で進ませる 
	//越えている場合はターゲットまでの方向が更新されないので
	//越える前までの方向が入る
	//ボスに当たった側は先端をその場で止める
	if (!m_isHitBossL)
	{
		m_beamPosL += m_beamMoveDirL * beam_speed;
	}
	if (!m_isHitBossR)
	{
		m_beamPosR += m_beamMoveDirR * beam_speed;
	}

	//先端の球の更新
	m_beamTipSphereL->Update(m_beamPosL, beam_sphere_radius);
	m_beamTipSphereR->Update(m_beamPosR,beam_sphere_radius);

	//プレイヤーに追いつくまでは球を数フレームに一回出し続ける
	//プレイヤーより少し後ろまで当たり判定は出しておきたいので
	//オフセットを足しておく
	Vector3 hitEndPos = playerPos + beam_hit_end_offset;

	//反射されてなかったら、ビームの先端がプレイヤーを追い越すまでは球を出す
	if (!m_isReflectedL)
	{
		if (m_beamPosL.z >= hitEndPos.z)
		{
			if (m_beamFrame % beam_col_interval == 0 )
			{
				//球生成
				std::shared_ptr<SphereShape> col = std::make_shared<SphereShape>(m_beamPosL, beam_sphere_radius);
				//配列にいれる
				m_beamSpheresL.push_back(col);
			}
		}
	}
	if(!m_isReflectedR)
	{
		if (m_beamPosR.z >= hitEndPos.z)
		{
			if (m_beamFrame % beam_col_interval == 0)
			{
				//球生成
			std::shared_ptr<SphereShape> col = std::make_shared<SphereShape>(m_beamPosR, beam_sphere_radius);
				//配列にいれる
				m_beamSpheresR.push_back(col);
			}
		}
	}

	//hitEndPosより後ろにある球は削除する
	m_beamSpheresL.erase(
		std::remove_if(
			m_beamSpheresL.begin(),
			m_beamSpheresL.end(),
			[hitEndPos](const std::shared_ptr<SphereShape>& sphere)
			{
				return sphere->GetPos().z < hitEndPos.z;
			}),
		m_beamSpheresL.end()
	);
	m_beamSpheresR.erase(
		std::remove_if(
			m_beamSpheresR.begin(),
			m_beamSpheresR.end(),
			[hitEndPos](const std::shared_ptr<SphereShape>& sphere)
			{
				return sphere->GetPos().z < hitEndPos.z;
			}),
		m_beamSpheresR.end()
	);

	//ビームエフェクトの位置と向きの更新
	//(薄くなって止めたエフェクトのハンドルは-1なので触らない)
	if (m_leftBeamEffectPlayH != -1)
	{
		m_pEffectManager.lock()->SetPos(m_leftBeamEffectPlayH, m_beamPosL);
		SetBeamEffectDir(m_leftBeamEffectPlayH, m_beamMoveDirL);
	}

	if (m_rightBeamEffectPlayH != -1)
	{
		m_pEffectManager.lock()->SetPos(m_rightBeamEffectPlayH, m_beamPosR);
		SetBeamEffectDir(m_rightBeamEffectPlayH, m_beamMoveDirR);
	}

	//ボスに当たったビームのエフェクトを薄くしていく
	if (m_isHitBossL)
	{
		FadeOutHitBeam(m_leftBeamEffectPlayH, m_hitBossFrameL);
	}
	if (m_isHitBossR)
	{
		FadeOutHitBeam(m_rightBeamEffectPlayH, m_hitBossFrameR);
	}

#ifdef _DEBUG
	//ビームの目標地点の球の更新
	m_targetSphereL->Update(targetPos, beam_sphere_radius);
	m_targetSphereR->Update(targetPos, beam_sphere_radius);
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
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		//球のデバッグ描画
		for (std::shared_ptr<SphereShape>& col : m_beamSpheresL)
		{
			col->Draw(0x00ff00);
		}
		for (std::shared_ptr<SphereShape>& col : m_beamSpheresR)
		{
			col->Draw(0x00ff00);
		}
		m_targetSphereL->Draw(0x00ff00);
		m_targetSphereR->Draw(0x00ff00);
		m_beamTipSphereL->Draw(0x00ff00);
		m_beamTipSphereR->Draw(0x00ff00);
	}
#endif
}

int BossBeamState::GetBeamDamage() const
{
	return beam_damage;
}

void BossBeamState::OnReflectLeft(const Vector3& hitPos)
{
	//ビームの先端位置
	Vector3 beamTipPos = m_beamTipSphereL->GetPos();
	
	//カウンター球の法線法線(当たった位置からビームの先端へのベクトル)
	Vector3 normal = (beamTipPos - hitPos).Normalized();

	//反射方向を計算
	m_beamMoveDirL = m_beamMoveDirL - normal * (2 * Vector3::Dot(m_beamMoveDirL, normal));
	//反射したことを記録
	m_isReflectedL = true;
}

void BossBeamState::OnReflectRight(const Vector3& hitPos)
{
	Vector3 beamTipPos = m_beamTipSphereR->GetPos();
	//カウンター球の法線(当たった位置からビームの先端へのベクトル)
	Vector3 normal = (beamTipPos - hitPos).Normalized();

	//反射方向を計算
	m_beamMoveDirR = m_beamMoveDirR - normal * (2 * Vector3::Dot(m_beamMoveDirR, normal));
	//反射したことを記録
	m_isReflectedR = true;
}

std::shared_ptr<SphereShape> BossBeamState::GetTipSphereL() const
{
	return  m_beamTipSphereL;
}

std::shared_ptr<SphereShape> BossBeamState::GetTipSphereR() const
{
	return m_beamTipSphereR;
}

bool BossBeamState::IsReflectedL() const
{
	return m_isReflectedL;
}

bool BossBeamState::IsReflectedR() const
{
	return m_isReflectedR;
}

void BossBeamState::FadeOutHitBeam(int& playH, int& fadeFrame)
{
	//既に止めている場合は何もしない
	if (playH == -1) return;

	fadeFrame++;

	//経過に応じてアルファを下げる(0〜color_max)
	float remainRate = 1.0f - static_cast<float>(fadeFrame) / hit_boss_fade_frame;
	int alpha = static_cast<int>(color_max * std::clamp(remainRate, 0.0f, 1.0f));
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
	pEffectManager->SetColor(playH, color_max, color_max, color_max, alpha);

	//完全に消えたらエフェクトを止める
	if (fadeFrame >= hit_boss_fade_frame)
	{
		pEffectManager->Stop(playH);
		playH = -1;
	}
}

void BossBeamState::SetBeamEffectDir(int playH, const Vector3& dir)
{
	//らせんは軸の向きの正負が関係ないのでZ成分を正にそろえる
		//(180度回転の特異点も避けられる)
		Vector3 d = (dir.z < 0.0f) ? dir * -1.0f : dir;

		//(0,0,1)からdへの回転軸 = cross((0,0,1), d) = (-d.y, d.x, 0)
		float s = std::sqrt(d.x * d.x + d.y * d.y);
		if (s < rot_axis_threshould)
		{
			//ほぼZ軸方向なので回転なし
			m_pEffectManager.lock()->SetRotation(playH, Vector3(0.0f, 0.0f, 0.0f));
			return;
		}
		Vector3 axis(-d.y / s, d.x / s, 0.0f);
		float angle = std::acos(std::clamp(d.z, -1.0f, 1.0f));

		//軸が逆に傾く場合はangleを-angleにする
		m_pEffectManager.lock()->SetRotationAxis(playH, axis, angle);
}


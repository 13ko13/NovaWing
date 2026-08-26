#include <EffekseerForDXLib.h>

#include "BossEnemy.h"
#include "Constants/ShaderRegister.h"
#include "BossIdleState.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "BossBeamState.h"
#include "Manager/InputManager.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 3.0f,3.0f,3.0f };
	//フレームの番号がいくつ進むごとに足のフレームがあるか
	constexpr int leg_frame_stride = 6;
	//右脚の最初のフレーム番号
	constexpr int first_left_leg_idx = 30;
	//片側の足の本数
	constexpr int one_side_leg_num = 3;
	//海の高さ
	constexpr float water_y = 0.0f;
	//モデルの名前
	const std::wstring model_name = L"DeformationSystem|";
	//後ろ移動アニメーションの名前
	const std::wstring idle_anim_name = model_name + L"A_MechISO_WalkBackward";
	//死亡アニメーションの名前
	const std::wstring death_anim_name = model_name + L"A_MechISO_Death|BaseLayer";

	//アニメーションブレンドにかける時間
	constexpr float anim_blend_time = 20.0f;
	//アイドルアニメーションの再生スピード
	constexpr float idle_anim_speed = 0.3f;

	//無敵判定用球の半径
	constexpr float invincible_col_radius = 1500.0f;
	//無敵判定球のY座標オフセット
	constexpr float invincible_col_offset_y = 350.0f;

	//ダメージ判定球のオフセット位置
	const Vector3 damage_col_offset = Vector3(0.0f, 950.0f, -1300.0f);
	//ダメージ判定用球の半径
	constexpr float damage_col_radius = 310.0f;

	//死亡待機状態から完全死亡になるまでの時間
	constexpr int true_dead_frame = 60 * 5 + 30;

	//死亡エフェクトのオフセット
	const Vector3 death_effect_offset = Vector3(0.0f, 1000.0f, 0.0f);

	//死亡アニメーションの再生速度
	constexpr float death_anim_speed = 0.5f;

	//回復するときに、食らったダメージの何割を回復するか
	constexpr float recovery_rate = 0.4f;
}

BossEnemy::BossEnemy(BossEnemyData& data) :
	EnemyBase(data.Id, data.pCamera, data.pPlayer,
	data.pBulletManager,data.health),
	m_animator(m_modelHandle)
{
	//位置を反映
	SetPos(data.pos);
}

BossEnemy::~BossEnemy()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);
	StopEffekseer3DEffect(m_shieldEffectPlayH);
	StopEffekseer3DEffect(m_deathEffectPlayH);
}

void BossEnemy::OnInit()
{
	//シェーダーに渡す定数バッファを作成
	CreateShaderBuffers();

	//最初の右足の位置(3本)を取得
	for (int i = 1; i < static_cast<int>(LegIndex::BackLeft); i++)
	{
		//脚の位置を取得
		VECTOR rightLegPos = MV1GetFramePosition(
			m_modelHandle, leg_frame_stride * i
		);
		//配列に格納
		m_prevLegPositions.push_back(rightLegPos);
		// 配列に格納
		m_currentLegPositions.push_back(rightLegPos);
	}
	//最初の左足の位置(3本)を取得
	for (int i = 0; i < static_cast<int>(LegIndex::FrontRight); i++)
	{
		//脚の位置を取得
		VECTOR leftLegPos = MV1GetFramePosition(
			m_modelHandle, first_left_leg_idx + leg_frame_stride * i
		);
		//配列に格納
		m_prevLegPositions.push_back(leftLegPos);
		//配列に格納
		m_currentLegPositions.push_back(leftLegPos);
	}

	//後ろ移動アニメーションを再生
	m_animator.Play(MV1GetAnimIndex(m_modelHandle, idle_anim_name.c_str()), true, idle_anim_speed);

	//ステートを初期化
	m_pState = std::make_shared<BossIdleState>(
		std::static_pointer_cast<BossEnemy>(shared_from_this()),
		m_pPlayer
	);
	m_pState->Enter();

	//無敵判定球初期化
	Vector3 invincibleColPos = m_pos;
	invincibleColPos.m_y += invincible_col_offset_y;//Y座標のみ補正する
	m_invincibleHitCol = Sphere(invincibleColPos, invincible_col_radius);

	
	//ダメージ判定球初期化
	Vector3 damageColPos = m_pos + damage_col_offset;
	m_damageCol = Sphere(damageColPos, damage_col_radius);
}

void BossEnemy::Update()
{
	
	//着地が終了していない間に着地したら
	if(m_velocity.m_y < 0.0f)
	{
		if (m_pos.m_y < water_y)
		{
			//着地完了とする
			m_isFirstLanding = true;
		}
	}

	//海より下に行かないようにする
	if (m_pos.m_y < water_y)
	{
		m_pos.m_y = water_y;
	}

	//死亡待機状態じゃない場合の処理
	if (!m_isDying)
	{
		if (m_isAppear)
		{
			//プレイヤーと同じ速度で移動する
			Vector3 myVel = m_pPlayer.lock()->GetVel();
			//zのみコピー
			myVel.m_y = 0.0f;
			myVel.m_x = 0.0f;
			SetVel(myVel);

			//ステートの更新
			m_pState->Update();
			//次のステートを取得
			std::shared_ptr<IBossEnemyState> pState = m_pState->GetNextState();
			//次のステートがあればステートを変更する
			if (pState != nullptr)
			{
				//前ステートの出るときの処理
				m_pState->Exit();
				//ステートを変更
				m_pState = pState;
				//切り替え後の入った時の処理
				m_pState->Enter();
			}
		}
	

		//キャラクタークラス共通の処理
		Charactor::Update();

		//当たり判定の更新
		Vector3 hitColPos = m_pos;
		hitColPos.m_y += invincible_col_offset_y;//Y座標のみ補正する
		m_invincibleHitCol.Update(hitColPos, invincible_col_radius);

		//ダメージ判定更新
		Vector3 damageColPos = m_pos + damage_col_offset;
		m_damageCol.Update(damageColPos, damage_col_radius);
	}
	//死亡待機状態になった場合
	else
	{
		m_dyingFrame++;
		if (m_dyingFrame > true_dead_frame)
		{
			//完全死亡
			OnEnemyDead();
		}
	}

	//今の足の位置を前の足の位置にコピーする
	m_prevLegPositions = m_currentLegPositions;

	//脚の位置を取得するが、インデックスの途中で不要なものが入っているので
	//二回に分けて取得する
	//アニメーターの更新
	m_animator.Update(anim_blend_time);

	//最初の右足の位置(3本)を取得
	for (int i = 1; i < static_cast<int>(LegIndex::BackLeft); i++)
	{
		//脚の位置を取得
		VECTOR rightLegPos = MV1GetFramePosition(
			m_modelHandle, leg_frame_stride * i
		);
		//配列に格納
		m_currentLegPositions[i - 1] = rightLegPos;
	}
	//最初の左足の位置(3本)を取得
	for (int i = 0; i < static_cast<int>(LegIndex::FrontRight); i++)
	{
		//脚の位置を取得
		VECTOR leftLegPos = MV1GetFramePosition(
			m_modelHandle, first_left_leg_idx + leg_frame_stride * i
		);
		//配列に格納
		m_currentLegPositions[i + one_side_leg_num] = leftLegPos;
	}

	//前フレーム海に浸かっていなくて、今フレーム海に浸かっている場合のみエフェクトを出す
	//判定は同じ添え字のみを行う
	for (size_t i = 0; i < m_prevLegPositions.size(); i++)
	{
		if (m_prevLegPositions[i].y > water_y &&
			m_currentLegPositions[i].y < water_y)
		{
			//水しぶきエフェクトを再生する
			//Effekseerのエフェクト再生を呼ぶ
			m_effectPlayHandle = PlayEffekseer3DEffect(
				ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::Splash)
			);
			//エフェクトの位置の調整する
			SetPosPlayingEffekseer3DEffect(
				m_effectPlayHandle,
				m_currentLegPositions[i].x,
				m_currentLegPositions[i].y,
				m_currentLegPositions[i].z
			);
		}
	}

#ifdef _DEBUG
	//ボスをワンボタンキル
	if (InputManager::GetInstance().IsTriggered("killBoss"))
	{
		m_health -= m_maxHealth;
	}
#endif
}

void BossEnemy::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新してから
	UpdateShaderMatrixData();

	//モデル描画
	DrawEnemy();

	//ステートごとの描画
	m_pState->Draw();

#ifdef _DEBUG
	//HP
	DrawFormatString(800, 15, 0xff0000, L"BossHP : %d",m_health);

	//無敵判定球の描画
	m_invincibleHitCol.Draw(0xff0000);
	//ダメージ判定球の描画
	m_damageCol.Draw(0xff00ff);
#endif
}

void BossEnemy::TakeDamage(int damage)
{
	// HPを減らす
	m_health -= damage;

	// HP0以下になったら死亡処理を行う
	if (m_health <= 0)
	{
		//死亡待機状態をtrueにする
		m_isDying = true;

		//死亡アニメーション再生
		m_animator.Play(death_anim_name.c_str(), false, death_anim_speed);

		//死亡エフェクトを出現させる
		int effectH = ResourceLoader::GetInstance().GetEffect(
			ResourceLoader::EffectID::BossDeath
		);
		m_deathEffectPlayH = PlayEffekseer3DEffect(effectH);

		//位置をセット
		Vector3 effectPos = m_pos + death_effect_offset;
		SetPosPlayingEffekseer3DEffect(
			m_deathEffectPlayH,
			effectPos.m_x,
			effectPos.m_y,
			effectPos.m_z
		);
	}
}

std::vector<Sphere> BossEnemy::GetBeamSphereL() const
{
	//左のビームの球を返す
	return std::dynamic_pointer_cast<BossBeamState>(m_pState)->GetLeftBeamSpheres();
}

std::vector<Sphere> BossEnemy::GetBeamSphereR() const
{
	//右のビームの球を返す
	return std::dynamic_pointer_cast<BossBeamState>(m_pState)->GetRightBeamSpheres();
}

void BossEnemy::OnHitInvincibleCol(const Position3& effectPos,const int attackPower)
{
	//シールドのエフェクト生成
	int effectHandle = ResourceLoader::GetInstance().GetEffect(
		ResourceLoader::EffectID::BossShield
	);
	m_shieldEffectPlayH = PlayEffekseer3DEffect(effectHandle);

	//エフェクトの位置をセット
	SetPosPlayingEffekseer3DEffect(
		m_shieldEffectPlayH,
		effectPos.m_x,
		effectPos.m_y,
		effectPos.m_z
	);

	//ボスを回復させる
	//食らったダメージの5分の1回復する
	m_health += static_cast<float>(attackPower) * recovery_rate;
}

void BossEnemy::DrawEnemy()
{
	//ResourceLoaderからボスの法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::BossNormal);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::BossEmission);

	//ハンドルとレジスタ番号をセットで保持しておく
	std::vector<std::pair<int, int>> textures;
	textures.push_back({ ShaderRegister::tex_normal,normGraphH });//法線マップ
	textures.push_back({ ShaderRegister::tex_metalic,-1 });//メタリックマップはないので-1
	textures.push_back({ ShaderRegister::tex_emission,emissionGraphH });//エミッション
	textures.push_back({ ShaderRegister::tex_noise,-1 });//ノイズテクスチャも途切れることがないのでなし

	//DrawWithLightingにペアの配列を渡してモデルを描画
	DrawWithLighting(textures, true);
}

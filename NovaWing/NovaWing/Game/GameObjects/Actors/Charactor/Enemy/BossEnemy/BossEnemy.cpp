#include <EffekseerForDXLib.h>

#include "BossEnemy.h"
#include "Constants/ShaderRegister.h"
#include "BossIdleState.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"

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

	//アニメーションブレンドにかける時間
	constexpr float anim_blend_time = 20.0f;
	//アイドルアニメーションの再生スピード
	constexpr float idle_anim_speed = 0.3f;

	//プレイヤーの弾との当たり判定用球の半径
	constexpr float hit_col_radius = 1500.0f;
	//球のY座標オフセット
	constexpr float hit_col_offset_y = 350.0f;
}

BossEnemy::BossEnemy(BossEnemyData& data) :
	EnemyBase(data.Id, data.pCamera, data.pPlayer, 
	data.pBulletManager),
	m_animator(m_modelHandle)
{
	//位置を反映
	SetPos(data.pos);
}

BossEnemy::~BossEnemy()
{
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

	//当たり判定初期化
	Vector3 hitColPos = m_pos;
	hitColPos.m_y += hit_col_offset_y;//Y座標のみ補正する
	m_hitCol = Sphere(hitColPos, hit_col_radius);
}

void BossEnemy::Update()
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

	//キャラクタークラス共通の処理
	Charactor::Update();

	//当たり判定の更新
	Vector3 hitColPos = m_pos;
	hitColPos.m_y += hit_col_offset_y;//Y座標のみ補正する
	m_hitCol.Update(hitColPos, hit_col_radius);

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
	//プレイヤーの速度
	//DrawFormatString(0, 230, 0xff0000, L"PlayerVelZ : %f", m_pPlayer.lock() ->GetVel().m_z);

	//当たり判定の描画
	m_hitCol.Draw(0xff0000);
#endif
}

void BossEnemy::TakeDamage(int damage)
{
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
	DrawWithLighting(textures,true);
}

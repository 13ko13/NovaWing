#define NOMINMAX
#include "RushEnemy.h"
#include "Game/Collision/EnemyCollider.h"
#include "Game/Collision/SphereShape.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Main/Application.h"
#include "Manager/DebugManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//出現中
	//出現にかけるフレーム数
	constexpr float appear_frame = 60.0f;
	//画面外のどれだけ外側から入ってくるか
	constexpr float appear_distance =6000.0f;

	//予備動作中
	//予備動作にかけるフレーム数
	constexpr float charge_frame = 30.0f;
	//震える幅
	constexpr int shake_width = 15;

	//突進中
	//突進の速さ
	constexpr float rush_speed = 60.0f;
	//プレイヤーの後ろにどれだけ進んだら消すか
	constexpr float delete_distance = 500.0f;

	//モデルの大きさ
	const Vector3 model_scale = Vector3(2.0f, 2.0f, 2.0f);

	//モデルの正面の向き
	const Vector3 model_forward = Vector3(0.0f, -1.0f, 0.0f);

	//モデルの正面と向きたい方向がほぼ同じかの閾値
	constexpr float forward_and_dir_threshold = 0.9999f;
}

RushEnemy::RushEnemy(const RushEnemyData& data) :
	EnemyBase(data.modelID, data.pCamera, data.pPlayer, data.pBulletManager,
		data.pSoundManager, data.pEffectManager)
{
	//当たり判定の生成
	m_colSphere = std::make_shared<SphereShape>(data.pos, 0.0f);
	m_pCollider = std::make_unique<EnemyCollider>(*this, ColliderTag::RushEnemy);


	//待機位置のずれ
	//プレイヤーの位置からどれだけ離れた位置に待機するか
	Vector3 playerPos = m_pPlayer.lock()->GetPos();//プレイヤーの位置
	m_standbyOffset = data.pos - playerPos;

	//初期位置設定
	//最初は待機位置から画面の外側に離れたところに置く
	Vector3 outward = Vector3(m_standbyOffset.x, m_standbyOffset.y, 0.0f);
	if (outward.Length() == 0.0f) outward = Vector3(0.0f, 1.0f, 0.0f);
	else outward.Normalize();
	SetPos(data.pos + outward * appear_distance);
}

RushEnemy::~RushEnemy()
{
}

void RushEnemy::OnInit()
{
	//シェーダーで描くための準備を行う
	CreateShaderBuffers();
	//今のプレイヤーのZを前のフレームのZ座標にいれる
	Vector3 playerPos = m_pPlayer.lock()->GetPos();
	m_prevPlayerZ = playerPos.z;
}

void RushEnemy::Update()
{
	//死亡待機中なら管理用のフレームを更新
	if (m_isDying)
	{
		m_dyingFrame++;
		//死亡待機が一定フレームを超えたら死亡
		if (m_dyingFrame > m_trueDeadFrame)
		{
			//完全死亡
			OnEnemyDead();
		}
	}
	else
	{
		//ステートの更新(管理用のフレームを更新)
		float timeScale = Application::GetInstance().GetTimeScale();
		m_stateTimer += timeScale;//タイムスケール分足す

		//プレイヤーの位置を一回だけ取る
		Vector3 playerPos = m_pPlayer.lock()->GetPos();

		//ステートによって行う処理を変える
		switch (m_state)
		{
		case State::Appear:
			UpdateAppear(playerPos);
			break;
		case State::Charge:
			UpdateCharge(playerPos);
			break;
		case State::Rush:
			UpdateRush(playerPos);
			break;
		}

		//もし死んでたら処理を止める
		if (IsDead()) return;

		//今のプレイヤーのZを前のフレームのZに保存
		m_prevPlayerZ = playerPos.z;

		//当たり判定の球を今の位置に合わせる
		m_colSphere->Update(m_pos, m_colRadius);
	}
}

void RushEnemy::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新してから
	UpdateShaderMatrixData();

	//敵の描画
	DrawEnemy();


#ifdef _DEBUG
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		//当たり判定の描画
		m_colSphere->Draw(0xffffff);
	}
#endif
}

std::vector<std::shared_ptr<SphereShape>> RushEnemy::GetCollisionSpheres() const
{
	if (m_isDying) return {};
	else return { m_colSphere };
}

std::vector<ICollider*> RushEnemy::GetColliders()
{
	//生ポインタを取得して返す
	return { m_pCollider.get() };
}

void RushEnemy::OnHitPlayer()
{
	//突進して爆発して死ぬ感じにする
	TakeDamage(m_health);
}

void RushEnemy::UpdateAppear(const Vector3& playerPos)
{
	//進行度
	float progress = m_stateTimer / appear_frame;
	progress = std::min(progress, 1.0f);
	//だんだんゆっくり止まるようにする
	float easeOut = 1.0f - (1.0f - progress) * (1.0f - progress);

	//画面外側への方向を計算
	//前後を使わず画面の上下左右だけとする
	Vector3 outward = Vector3(m_standbyOffset.x, m_standbyOffset.y, 0.0f);
	//もし長さが0なら上に行かせる
	if (outward.Length() == 0.0f) outward = Vector3(0.0f, 1.0f, 0.0f);
	//そうでなければ長さを1にする
	else outward.Normalize();

	//位置を計算
	//最初はappear_distanceで、進むほど待機場所に近づく
	float remainingDist = appear_distance * (1.0f - easeOut);//残りの距離を出す
	m_pos = playerPos + m_standbyOffset + outward * remainingDist;

	//進行度が1以上になったらステートをチャージ状態にする
	if (progress >= 1.0f)
	{
		m_state = State::Charge;
		//ステートの更新用フレームもリセットする
		m_stateTimer = 0.0f;
		//震えの中心を現在の出現完了後の位置とする
		m_chargeCenter = m_pos;
	}
	//プレイヤーへの向きを渡してモデルの正面を
	//プレイヤーに向けるようにする
	LookAtDir(playerPos - m_pos);
}

void RushEnemy::UpdateCharge(const Vector3& playerPos)
{
	//震えの量を毎フレームランダムに決める
	float shakeAmountX = static_cast<float>(GetRand(shake_width * 2) - shake_width);//-shake_widthからshake_width
	float shakeAmountY = static_cast<float>(GetRand(shake_width * 2) - shake_width);//-shake_widthからshake_width
	Vector3 shakeAmount = Vector3(shakeAmountX, shakeAmountY, 0.0f);//Vector3にまとめる

	//前後だけプレイヤーが進んだ分ついていく
	m_chargeCenter.z += playerPos.z - m_prevPlayerZ;
	m_pos = m_chargeCenter + shakeAmount;

	//チャージの時間が終了したら
	if (m_stateTimer >= charge_frame)
	{
		//突進する方向を計算
		m_rushDir = (playerPos - m_pos).Normalized();
		//ステートを突進に変更
		m_state = State::Rush;
		//ステートタイマーリセット
		m_stateTimer = 0.0f;
	}

	//プレイヤーへの向きを渡してモデルの正面を
	//プレイヤーに向けるようにする
	LookAtDir(playerPos - m_pos);
}

void RushEnemy::UpdateRush(const Vector3& playerPos)
{
	//予備動作中に覚えた方向に進む
	//タイムスケールを取る
	float timeScale = Application::GetInstance().GetTimeScale();
	//位置を更新する(タイムスケールも交えて)
	m_pos += m_rushDir * rush_speed * timeScale;
	//プレイヤーが前に進んだ分だけ、一緒に前に進む
	m_pos.z += playerPos.z - m_prevPlayerZ;

	//モデルが突進方向を向くようにする
	LookAtDir(m_rushDir);

	//プレイヤーの後ろまで行った値が閾値を超えたら消す
	if (m_pos.z < playerPos.z - delete_distance) OnDead();
}

void RushEnemy::DrawEnemy()
{
	//ResourceLoaderから突進敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RushEnemyNormalMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RushEnemyEmissionMap);
	//メタリック
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RushEnemyMetalicMap);

	//ディゾルブ用ノイズ
	int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::DissolveNoise
	);

	//ハンドルとレジスタ番号をセットで保持しておく
	std::vector<std::pair<int, int>> textures;
	textures.push_back({ ShaderRegister::tex_normal,normGraphH });//法線マップ
	textures.push_back({ ShaderRegister::tex_metalic,metalicGraphH });//メタリックマップ
	textures.push_back({ ShaderRegister::tex_emission,emissionGraphH });//エミッション
	textures.push_back({ ShaderRegister::tex_noise,noiseHandle });//ノイズテクスチャ

	//DrawWithLightingにペアの配列を渡してモデルを描画
	DrawWithLighting(textures);
}

void RushEnemy::LookAtDir(const Vector3& dir)
{
	//向きたい方向
	Vector3 to = dir.Normalized();
	//正面と向きたい方向がどれだけ同じ向きか
	float dot = Vector3::Dot(model_forward, to);

	//正面と向きたい方向がほぼ同じ向きなら回さない
	if (dot > forward_and_dir_threshold)
	{
		m_rotation = Quaternion();
		return;
	}
	//真逆だと外積が0になって位置が決まらないので
	//正面と直角なX軸で180度回転する
	if (dot < -forward_and_dir_threshold)
	{
		m_rotation = Quaternion(Vector3(1.0f, 0.0f, 0.0f), DX_PI_F);
		return;
	}

	//2本のベクトルに直角な軸を中心に
	//間の角度だけ回す
	//dotは長さ1のベクトル同士だと2本の間の角度のcosとなるので
	//cosの逆の計算をしてcosから角度を計算
	Vector3 axis = Vector3::Cross(model_forward, to).Normalized();
	float angle = std::acosf(dot);
	m_rotation = Quaternion(axis, angle);
}

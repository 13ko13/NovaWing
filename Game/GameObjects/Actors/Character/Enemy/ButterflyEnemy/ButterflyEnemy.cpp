#define NOMINMAX

#include "ButterflyEnemy.h"
#include "Game/Collision/EnemyCollider.h"
#include "Game/Collision/SphereShape.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Main/Application.h"
#include "Manager/DebugManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//画面外のどれだけ外側から入ってくるか
	// constexpr float appear_distance =6000.0f;
	constexpr float appear_distance = 6000.0f;
	//モデルの大きさ
	const Vector3 model_scale = Vector3(2.0f, 2.0f, 2.0f);

	//出現にかける時間
	constexpr float appear_frame = 80.0f;

	//何フレーム停止するか
	constexpr float stop_frame = 60.0f;
	//ダッシュで移動する最低距離
	constexpr float dash_min_distance = 500.0f;
	//ダッシュで移動する最大値
	constexpr float dash_max_distance = 800.0f;
	//ダッシュする方向が何方向あるか
	constexpr int dash_dir_num = 8;

	//ダッシュにかけるフレーム数
	constexpr float dash_frame = 6.0f;

	//ダッシュの回数がこの回数に達したら戦線離脱する
	constexpr int leave_dash_num = 6;
}

ButterflyEnemy::ButterflyEnemy(const ButterflyEnemyData& data) :
	EnemyBase(data.modelID, data.pCamera, data.pPlayer,
		data.pBulletManager, data.pSoundManager, data.pEffectManager)
{

	//当たり判定の生成
	m_colSphere = std::make_shared<SphereShape>(data.pos, 0.0f);
	m_pCollider = std::make_unique<EnemyCollider>(*this, ColliderTag::ButterflyEnemy);


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

ButterflyEnemy::~ButterflyEnemy()
{
}

void ButterflyEnemy::OnInit()
{
	//シェーダーで描くための準備を行う
	CreateShaderBuffers();
	//今のプレイヤーのZを前のフレームのZ座標にいれる
	Vector3 playerPos = m_pPlayer.lock()->GetPos();
	m_prevPlayerZ = playerPos.z;
}

void ButterflyEnemy::Update()
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
		case State::Stop:
			UpdateStop(playerPos);
			break;
		case State::Dash:
			UpdateDash(playerPos);
			break;
		}

		//もし死んでたら処理を止める
		if (IsDead()) return;

		//今のプレイヤーのZを前のフレームのZに保存
		m_prevPlayerZ = playerPos.z;

		//当たり判定の球を今の位置に合わせる
		m_colSphere->Update(m_pos, m_colRadius);

		//プレイヤーの方にモデルを向かせる
		LookAtDir((m_pPlayer.lock()->GetPos() - m_pos).Normalized());
	}
}

void ButterflyEnemy::Draw()
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

std::vector<std::shared_ptr<SphereShape>> ButterflyEnemy::GetCollisionSpheres() const
{
	if (m_isDying) return {};
	else return { m_colSphere };
}

std::vector<ICollider*> ButterflyEnemy::GetColliders()
{
	//生ポインタを取得して返す
	return { m_pCollider.get() };
}

void ButterflyEnemy::DrawEnemy()
{
	//ResourceLoaderから蝶型の敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::ButterflyEnemyNormalMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::ButterflyEnemyEmissionMap);
	//メタリック
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::ButterflyEnemyMetalicMap);

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

void ButterflyEnemy::UpdateAppear(const Vector3& playerPos)
{
	//進行度を計算
	float progress = m_stateTimer / appear_frame;
	//進行度を0〜1に収める
	progress = std::min(progress, 1.0f);

	//だんだんゆっくり止まるようにする
	float easeOut = 1.0f - (1.0f - progress) * (1.0f - progress);

	//画面外側への方向を計算する
	Vector3 outwardVec = Vector3(m_standbyOffset.x, m_standbyOffset.y, 0.0f);
	//もし長さが0なら上に行かせる
	if (outwardVec.Length() == 0.0f) outwardVec = Vector3(0.0f, 1.0f, 0.0f);
	//そうでなければ長さを1にする
	else outwardVec.Normalize();

	//最初はappear_distanceで、進むほど待機場所に近づく
	float remainingDist = appear_distance * (1.0f - easeOut);
	//位置を計算
	//Zしかプレイヤーの位置を参照しない
	Vector3 playerZ = Vector3(0.0f, 0.0f, playerPos.z);
	m_pos = playerZ + m_standbyOffset + outwardVec * remainingDist;

	//進行度が1になったらステートをStopにする
	if (progress >= 1.0f)
	{
		m_state = State::Stop;
		m_stateTimer = 0.0f;
	}
}

void ButterflyEnemy::UpdateStop(const Vector3& playerPos)
{
	//位置を計算
	//Zしかプレイヤーの位置を参照しない
	Vector3 playerZ = Vector3(0.0f, 0.0f, playerPos.z);
	m_pos = playerZ + m_standbyOffset;

	//ダッシュした回数がleave_dash_num以上になったら戦線離脱する
	if(m_dashNum > leave_dash_num)
	{
		//ステートを離脱に遷移
		m_state = State::Leave;
		m_stateTimer = 0.0f;
	}

	//停止時間が終了したら
	if (m_stateTimer >= stop_frame)
	{
		m_state = State::Dash;
		m_stateTimer = 0.0f;

		//ダッシュの出発点を保存
		m_dashStartPos = m_standbyOffset;
		//ダッシュの移動先を決める
		//どれだけ移動するかを計算
		float dashDist = dash_min_distance +
			GetRand(static_cast<int>(dash_max_distance) - static_cast<int>(dash_min_distance));
		//8方向のうち何番目の方向に移動するか
		int dirIndex = GetRand(dash_dir_num - 1);
		//1周を8分割すると45度
		//それにランダムで決めた数をかけると8方向のうちどれを指すかが決まる
		float angle = DX_TWO_PI_F / dash_dir_num * dirIndex;
		//角度を方向にする
		Vector3 dashDir = Vector3(std::cosf(angle), std::sinf(angle), 0.0f);

		//ダッシュの目標地点を計算
		m_dashEndPos = m_dashStartPos + dashDir * dashDist;

	}
}

void ButterflyEnemy::UpdateDash(const Vector3& playerPos)
{
	//進行度
	float progress = m_stateTimer / dash_frame;
	//だんだんゆっくり止まるようにする
	float easeOut = 1.0f - (1.0f - progress) * (1.0f - progress);
	//プレイヤーから見た位置を動かす
	m_standbyOffset = m_dashStartPos + (m_dashEndPos - m_dashStartPos) * easeOut;
	//位置を更新
	//Zしかプレイヤーの位置を参照しない
	Vector3 playerZ = Vector3(0.0f, 0.0f, playerPos.z);
	m_pos = playerZ + m_standbyOffset;

	//進行度が1になったら停止
	if (progress >= 1.0f)
	{
		//状態を停止に変更
		m_state = State::Stop;
		m_stateTimer = 0.0f;
		//ダッシュした回数をカウント
		m_dashNum++;
	}
}

void ButterflyEnemy::LookAtDir(const Vector3& dir)
{
	//先に寝ているモデルを起こすQuatenionを作成
	//X軸に90度回転
	Quaternion standUp = Quaternion(Vector3(1.0f, 0.0f, 0.0f), DX_PI_F / 2.0f);

	//Dirの方を向く
	//上下の回転
	float pitchAngle = -atan2(dir.y, sqrt(dir.x * dir.x + dir.z * dir.z));
	Quaternion pitch = Quaternion(Vector3(1.0f, 0.0f, 0.0f), pitchAngle);
	//左右の回転
	float yawAngle = atan2(dir.x, dir.z);
	Quaternion yaw = Quaternion(Vector3(0.0f, 1.0f, 0.0f), yawAngle);
	m_rotation = yaw * pitch * standUp;
}
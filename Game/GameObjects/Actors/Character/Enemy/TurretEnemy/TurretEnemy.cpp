#include "TurretEnemy.h"
#include "Game/Collision/EnemyCollider.h"
#include "Manager/DebugManager.h"
#include "Constants/ShaderRegister.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Manager/BulletManager.h"
#include "Manager/SoundManager.h"
#include "Main/Application.h"

namespace
{
	//モデルの大きさ
	const Vector3 model_scale = Vector3(1.0f, 1.0f, 1.0f);
	//当たり判定のずれ
	const Vector3 col_offset = Vector3(0.0f, 150.0f, 0.0f);

	//砲塔のフレーム名
	constexpr const wchar_t* turret_frame_name = L"Turret";
	//発射口のフレーム名
	constexpr const wchar_t* barrel_frame_name = L"Barrel";
	//弾の速度
	constexpr float bullet_speed = 15.0f;
	//弾の攻撃力
	constexpr int attack_power = 5;
	//攻撃可能距離
	constexpr float can_attack_distance = 4000.0f;
	//攻撃の間隔
	constexpr int attack_interval = 70;
	//予備動作をどのぐらいの時間かけて行うか
	constexpr int anticipation_frame = 180;

	//砲身の軸の高さ
	constexpr float barrel_axis_offset_y = 179.0f;
	//砲身の角度の上限
	constexpr float barrel_angle_limit = 50.0f * DX_PI_F / 180.0f;//+-50まで

	//1フレームで回れる角度の上限
	constexpr float one_frame_turn_angle = 0.5f * DX_PI_F / 180.0f;//0.1度

	//狙いが決まってから弾を撃つまでの時間
	constexpr int aim_lock_frame = 10;

	//砲身の付け根から銃口までの距離
	constexpr float barrel_to_muzzle_distance = 132.0f;
}

TurretEnemy::TurretEnemy(const TurretEnemyData& data) :
	EnemyBase(data.modelID, data.pCamera, data.pPlayer, data.pBulletManager,
	data.pSoundManager, data.pEffectManager)
{
	//当たり判定の生成
	m_colSphere = std::make_shared<SphereShape>(data.pos + col_offset, m_colRadius);
	m_pCollider = std::make_unique<EnemyCollider>(*this, ColliderTag::Floating);

	//位置を設定
	SetPos(data.pos);
}

TurretEnemy::~TurretEnemy()
{
}

void TurretEnemy::OnInit()
{
	//シェーダーを使用する準備
	CreateShaderBuffers();
}

void TurretEnemy::Update()
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
	//生きていたら
	else
	{
		//タイムスケール分ステートタイマーを更新
		float timeScale = Application::GetInstance().GetTimeScale();
		m_stateTimer += timeScale;

		//プレイヤーの位置取得
		Vector3 playerPos = m_pPlayer.lock()->GetPos();

		//ステートごとの処理
		switch (m_state)
		{
		case State::Wait:
			UpdateWait(playerPos);
			break;
		case State::Aim:
			UpdateAim(playerPos);
			break;
		}
	}
}

void TurretEnemy::Draw()
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

std::vector<std::shared_ptr<SphereShape>> TurretEnemy::GetCollisionSpheres() const
{
	if (m_isDying) return {};
	else return { m_colSphere };
}

std::vector<ICollider*> TurretEnemy::GetColliders()
{
	//生ポインタを取得して返す
	return { m_pCollider.get() };
}

void TurretEnemy::UpdateWait(const Vector3& playerPos)
{
	//ステートのタイマーが待機時間を過ぎていて
	//CanAttackがtrueならステートをAimに変更してタイマーリセット
	if (m_stateTimer > attack_interval &&
		CanAttack(playerPos))
	{
		m_state = State::Aim;
		m_stateTimer = 0.0f;
	}
}

void TurretEnemy::UpdateAim(const Vector3& playerPos)
{
	//攻撃不可能だったらWaitに戻してタイマーをリセット
	if (!CanAttack(playerPos))
	{
		m_state = State::Wait;
		m_stateTimer = 0.0f;
		return;
	}

	//ステートタイマーが予備動作から狙いが決まるまでの時間の間だけ
	//回転を行う
	if (m_stateTimer < anticipation_frame - aim_lock_frame)
	{
		//狙う位置をプレイヤーの位置とする
		m_aimPos = playerPos;
		//回転を行う
		UpdateAimAngle(playerPos);
		//角度をフレームに反映する
		ApplyFrameRotation();
	}

	//予備動作の時間が過ぎたら弾を発射
	if (m_stateTimer > anticipation_frame)
	{
		Shoot(m_aimPos);
		m_state = State::Wait;
		m_stateTimer = 0.0f;
	}
}

bool TurretEnemy::CanAttack(const Vector3& playerPos) const
{
	//カメラ内にタレットが入っているかつ
	//攻撃する距離よりもプレイヤーとタレットの距離が短ければtrue
	return (can_attack_distance > (playerPos - m_pos).Length()) &&
		!CheckCameraViewClip(m_pos.ToDxLib());
}

void TurretEnemy::Shoot(const Vector3& playerPos)
{
	int barrelFrameIndex = MV1SearchFrame(m_modelHandle, barrel_frame_name);

	//弾がちゃんと銃口から出るようにしたいので
	//砲身の行列を取得
	MATRIX barrelMatrix = MV1GetFrameLocalWorldMatrix(m_modelHandle, barrelFrameIndex);
	//銃口までの距離
	//モデルの作り上砲身の正面が-Zなので-を付ける
	VECTOR barrelToMuzzle = VGet(0.0f, 0.0f, -barrel_to_muzzle_distance);

	//発射位置
	Vector3 shootPos;
	//砲身の行列を使って銃口までのベクトルを変換する
	shootPos = VTransform(barrelToMuzzle, barrelMatrix);

	//弾を撃つ方向
	Vector3 toPlayerDir = (playerPos - shootPos).Normalized();
	Vector3 toPlayerVelocity = toPlayerDir * bullet_speed;
	//プレイヤーが進んだ量分弾も進ませる
	toPlayerVelocity.z += m_pPlayer.lock()->GetVel().z;
	//弾の管理者に弾を生成させる
	m_pBulletManager.lock()->CreateBullet(
		BulletManager::BulletType::EnemyBullet,
		shootPos, toPlayerVelocity, attack_power,
		m_pCamera, std::static_pointer_cast<EnemyBase>(shared_from_this())
	);

	//音を鳴らす
	m_pSoundManager.lock()->Play(
		SoundManager::SoundType::EnemyShoot
	);
}

void TurretEnemy::DrawEnemy()
{
	//ResourceLoaderから固定砲台の敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::TurretEnemyNormalMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::TurretEnemyEmissionMap);
	//メタリック
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::TurretEnemyMetalicMap);
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

	//DrawFrameWithLightingにペアの配列を渡してフレームごとに描画していく
	//フレームごとにしなければ全部のフレームが原点に集まってしまうため
	DrawFrameWithLighting(textures);
}

void TurretEnemy::UpdateAimAngle(const Vector3& playerPos)
{
	//砲身の軸の中心位置
	Vector3 barrelAxisPos = Vector3(m_pos.x, m_pos.y + barrel_axis_offset_y, m_pos.z);

	//プレイヤーへの方向
	Vector3 toPlayerDir = (playerPos - barrelAxisPos);
	
	//目標角度
	float targetTurretAngleY;
	float targetBarrelAngleX;

	//砲塔の目標角度を求める
	//モデルの正面が-Zだから両方に-を付ける
	targetTurretAngleY = atan2(-toPlayerDir.x, -toPlayerDir.z);


	//砲身の角度を求める
	//そのためにプレイヤーまでの距離を測る
	float toPlayerDistance = std::sqrtf(toPlayerDir.x * toPlayerDir.x + toPlayerDir.z * toPlayerDir.z);
	targetBarrelAngleX = atan2(toPlayerDir.y, toPlayerDistance);
	//砲身は見た目上角度に上限があるため+-barrel_angle_limitまでとする
	targetBarrelAngleX = std::clamp(targetBarrelAngleX, -barrel_angle_limit, barrel_angle_limit);

	//タイムスケール
	float timeScale = Application::GetInstance().GetTimeScale();

	//砲塔の、今の角度と目標角度の差を出す
	float turretAngleDelta = targetTurretAngleY - m_turretAngleY;
	//今回回ることができる量
	float turnAngle = one_frame_turn_angle * timeScale;

	//差が180度より大きければ360度引く
	if (turretAngleDelta > DX_PI_F) turretAngleDelta -= DX_TWO_PI_F;
	if (turretAngleDelta < -DX_PI_F) turretAngleDelta += DX_TWO_PI_F;
	//回転をゆっくり行うために差を1フレームに回れる量にクランプする
	turretAngleDelta = std::clamp(turretAngleDelta, -turnAngle, turnAngle);
	m_turretAngleY += turretAngleDelta;

	//砲身も同様に差を出す
	float barrelAngleDelta = targetBarrelAngleX - m_barrelAngleX;
	//回転をゆっくり行うために差を1フレームに回れる量にクランプする
	barrelAngleDelta = std::clamp(barrelAngleDelta, -turnAngle, turnAngle);
	m_barrelAngleX += barrelAngleDelta;
}

void TurretEnemy::ApplyFrameRotation()
{
	int turretFrameIndex = MV1SearchFrame(m_modelHandle, turret_frame_name);
	int barrelFrameIndex = MV1SearchFrame(m_modelHandle, barrel_frame_name);

	//初期状態の行列を取得
	//砲塔
	MATRIX turretBaseMatrix = MV1GetFrameBaseLocalMatrix(
		m_modelHandle,turretFrameIndex
	);
	//砲身
	MATRIX barrelBaseMatrix = MV1GetFrameBaseLocalMatrix(
		m_modelHandle,barrelFrameIndex
	);

	MATRIX rotateTurretMatrix;
	MATRIX rotateBarrelMatrix;

	//元の行列と回転行列を合成
	rotateTurretMatrix = MMult(MGetRotY(m_turretAngleY), turretBaseMatrix);
	rotateBarrelMatrix = MMult(MGetRotX(m_barrelAngleX), barrelBaseMatrix);

	MV1SetFrameUserLocalMatrix(m_modelHandle, turretFrameIndex, rotateTurretMatrix);
	MV1SetFrameUserLocalMatrix(m_modelHandle, barrelFrameIndex, rotateBarrelMatrix);
}

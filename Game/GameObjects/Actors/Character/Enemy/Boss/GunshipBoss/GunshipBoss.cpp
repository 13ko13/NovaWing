#include <cassert>

#include "GunshipBoss.h"
#include "Manager/ResourceLoader.h"
#include "Constants/ShaderRegister.h"
#include "Utility/ModelAnimator.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "IGunshipBossState.h"
#include "GunshipBossMoveState.h"
#include "Manager/DebugManager.h"
#include "Game/Collision/SphereShape.h"
#include "Game/Collision/GunshipBossWeakCollider.h"
#include "GunshipBossNetState.h"
#include "Game/Collision/CapsuleShape.h"
#include "Game/Collision/GunshipBossBodyCollider.h"

namespace
{
	//第二形態になるときに表示する壊れたポッドのフレーム名
	constexpr const wchar_t* broken_pod_frame_name = L"Boss2_PodR_Broken";
	//待機アニメーション名
	constexpr const wchar_t* idle_anim_name = L"Boss2_Armature|Idle";

	//左右の弱点のフレーム名
	//左の弱点
	constexpr const wchar_t* weak_frame_name_left = L"Weak_L";
	//右の弱点
	constexpr const wchar_t* weak_frame_name_right = L"Weak_R";

	//アニメーションのブレンドの速さ
	constexpr float anim_blend_frame = 16.0f;

	//弱点の球の半径
	constexpr float weak_sphere_radius = 200.0f;

	//胴体のフレーム名(原点は胴体の中心)
	constexpr const wchar_t* body_frame_name = L"Body";
	//胴体のカプセルの、中心から両端までの横の長さ(腕の付け根が±400あたり)
	constexpr float body_capsule_half_length = 250.0f;
	//胴体のカプセルの半径(胴体の高さ約650を覆う)
	constexpr float body_capsule_radius = 330.0f;
	//胴体のカプセルを胴体の中心からどれだけ前(-Z、プレイヤー側)に出すか
	//中心のままだと前の面がz=-330で、顔(前面)より奥で弾が当たって見えるため
	constexpr float body_capsule_front_offset = 250.0f;
}

GunshipBoss::GunshipBoss(GunshipBossData& data) :
	BossBase(data),
	m_netCol(*this)
{
	//位置を設定
	SetPos(data.pos);
	//アニメーターのポインタを作成
	m_pAnimator = std::make_unique<ModelAnimator>(m_modelHandle);
	//弱点のコライダーを作成
	m_pWeakCol = std::make_unique<GunshipBossWeakCollider>(*this);
	//胴体のコライダーを作成
	m_pBodyCol = std::make_unique<GunshipBossBodyCollider>(*this);
}

GunshipBoss::~GunshipBoss()
{
}

void GunshipBoss::OnInit()
{
	//シェーダーに渡す定数バッファを作成(これがないと描画時に空のバッファを使ってしまう)
	CreateShaderBuffers();

	//第二形態に入ってから表示する壊れたポッドを非表示にしておく
	m_brokenPodFrameIdx = MV1SearchFrame(m_modelHandle, broken_pod_frame_name);
	assert(m_brokenPodFrameIdx >= 0 && "ポッドのフレーム名を間違えている");
	MV1SetFrameVisible(m_modelHandle, m_brokenPodFrameIdx, FALSE);

	//弱点の球の初期化
	InitWeakSphere();
	//胴体のカプセルの初期化
	InitBodyCapsule();

	//待機アニメーションを再生
	m_pAnimator->Play(idle_anim_name, true);

	//移動ステートのポインタを作成
	//コンストラクタ内で作ろうとするとまだGunshipBossのポインタが作られていないのでOnInitで呼ぶ
	m_pState = std::make_shared<GunshipBossMoveState>(
		std::static_pointer_cast<GunshipBoss>(shared_from_this()));
	//ステートの開始処理を呼ぶ
	m_pState->Enter();
}

void GunshipBoss::InitWeakSphere()
{
	//弱点の位置を取得
	m_weakFrameIdxLeft = MV1SearchFrame(m_modelHandle, weak_frame_name_left);
	m_weakFrameIdxRight = MV1SearchFrame(m_modelHandle, weak_frame_name_right);
	assert(m_weakFrameIdxLeft >= 0 && "弱点のフレーム名を間違えている");
	assert(m_weakFrameIdxRight >= 0 && "弱点のフレーム名を間違えている");

	Vector3 spherePosLeft;
	Vector3 spherePosRight;
	spherePosLeft = MV1GetFramePosition(m_modelHandle, m_weakFrameIdxLeft);
	spherePosRight = MV1GetFramePosition(m_modelHandle, m_weakFrameIdxRight);

	//弱点の位置に球を配置
	m_pWeakSphereLeft = std::make_shared<SphereShape>(
		spherePosLeft, weak_sphere_radius);
	m_pWeakSphereRight = std::make_shared<SphereShape>(
		spherePosRight, weak_sphere_radius);
}

void GunshipBoss::InitBodyCapsule()
{
	//胴体のフレームを探す
	m_bodyFrameIdx = MV1SearchFrame(m_modelHandle, body_frame_name);
	assert(m_bodyFrameIdx >= 0 && "胴体のフレーム名を間違えている");

	//位置はUpdateBodyCapsuleで合わせるので、ここでは形だけ作る
	m_pBodyCapsule = std::make_shared<CapsuleShape>(
		Vector3(), Vector3(), body_capsule_radius);
	UpdateBodyCapsule();
}

void GunshipBoss::UpdateBodyCapsule()
{
	//胴体の中心(アニメーションでのけぞると動くのでフレームから取る)
	Vector3 bodyPos;
	bodyPos = MV1GetFramePosition(m_modelHandle, m_bodyFrameIdx);
	//顔(前面)で弾が当たるように、前(-Z)に出す
	bodyPos.z -= body_capsule_front_offset;

	//胴体は横に長いので、中心から左右に伸ばしたカプセルにする
	Vector3 halfLength = Vector3(body_capsule_half_length, 0.0f, 0.0f);
	m_pBodyCapsule->Update(bodyPos - halfLength, bodyPos + halfLength, body_capsule_radius);
}

void GunshipBoss::Update()
{
	if (m_isAppear)
	{
		//プレイヤーと同じ速度で移動する
		Vector3 myVel = m_pPlayer.lock()->GetVel();
		//zのみコピー
		myVel.y = 0.0f;
		myVel.x = 0.0f;
		SetVel(myVel);

		//ステートの更新
		UpdateState();
	}
	//BossBase(Character)の更新処理を呼ぶ
	BossBase::Update();
	//アニメーターを更新させる
	m_pAnimator->Update(anim_blend_frame);

	Vector3 weakFramePosL;
	Vector3 weakFramePosR;
	weakFramePosL = MV1GetFramePosition(m_modelHandle, m_weakFrameIdxLeft);
	weakFramePosR = MV1GetFramePosition(m_modelHandle, m_weakFrameIdxRight);
	//弱点の球の更新
	m_pWeakSphereLeft->Update(weakFramePosL, weak_sphere_radius);
	m_pWeakSphereRight->Update(weakFramePosR, weak_sphere_radius);
	//胴体のカプセルの更新
	UpdateBodyCapsule();
}

std::vector<ICollider*> GunshipBoss::GetColliders()
{
	//ICollider*に変換して返す
	//弱点は胴体より先に並べる(胴体と弱点は離れているが、念のため弱点を優先して判定する)
	return { &m_netCol, m_pWeakCol.get(), m_pBodyCol.get() };
}

bool GunshipBoss::IsWeakOpen() const
{
	std::shared_ptr<GunshipBossNetState> netState =
		std::dynamic_pointer_cast<GunshipBossNetState>(m_pState);
	bool isVentOpen = false;
	if (netState != nullptr)
	{
		//弱点が開いているか
		isVentOpen = netState->IsVentOpen();
	}

	//nullではなく弱点が開いていたらtrueを返す
	return netState != nullptr && isVentOpen;
}

void GunshipBoss::UpdateState()
{
	//ステートの更新を呼ぶ
	m_pState->Update();

	//次のステートを取得する
	std::shared_ptr<IGunshipBossState> nextState = m_pState->GetNextState();
	//次のステートがあれば今のステートの終了処理をしてそのステートに切り替える
	if (nextState != nullptr)
	{
		m_pState->Exit();//終了処理
		//次のステートに切り替える
		m_pState = nextState;
		m_pState->Enter();//初期化
	}
}

void GunshipBoss::Draw()
{
	//モデルに行列を適用(大きさはActorのm_scale、2面のボスは1倍)
	ApplyMatrix(m_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新
	UpdateShaderMatrixData();

	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::Boss2Normal);
	//メタリックマップ
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::Boss2Metallic);
	//エミッションマップ
	int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::Boss2Emission);
	//現在弱点が開いているなら通常とは違うエミッション画像を渡す
	if (IsWeakOpen())
	{
		emissionGraphH = resourceLoader.GetGraphic(
			ResourceLoader::GraphicID::Boss2EmissionVent
		);
	}

	//ハンドルとレジスタ番号をセットで保持しておく
	std::vector<std::pair<int, int>> textures;
	textures.push_back({ ShaderRegister::tex_normal,normGraphH });//法線マップ
	textures.push_back({ ShaderRegister::tex_metalic,metalicGraphH });//メタリックマップ
	textures.push_back({ ShaderRegister::tex_emission,emissionGraphH });//エミッションマップ
	textures.push_back({ ShaderRegister::tex_noise,-1 });//ノイズテクスチャもなし

	//スキンメッシュ(ボーンで部品を動かす)なのでtrueで描画
	DrawWithLighting(textures, true);

	//ステートごとの描画
	m_pState->Draw();

#ifdef _DEBUG
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		m_pWeakSphereLeft->Draw(0xff0000);
		m_pWeakSphereRight->Draw(0xff0000);
		//胴体の判定は黄色
		m_pBodyCapsule->Draw(0xffff00);
	}
#endif
}

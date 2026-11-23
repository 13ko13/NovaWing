#include <vector>
#include <cassert>

#include "GunshipBoss.h"
#include "Manager/ResourceLoader.h"
#include "Constants/ShaderRegister.h"
#include "Utility/ModelAnimator.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "IGunshipBossState.h"
#include "GunshipBossMoveState.h"

namespace
{
	//第二形態になるときに表示する壊れたポッドのフレーム名
	constexpr const wchar_t* broken_pod_frame_name = L"Boss2_PodR_Broken";
	//待機アニメーション名
	constexpr const wchar_t* idle_anim_name = L"Boss2_Armature|Idle";

	//アニメーションのブレンドの速さ
	constexpr float anim_blend_frame = 16.0f;
}

GunshipBoss::GunshipBoss(GunshipBossData& data) :
	BossBase(data)
{
	//位置を設定
	SetPos(data.pos);
	//アニメーターのポインタを作成
	m_pAnimator = std::make_unique<ModelAnimator>(m_modelHandle);
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

	//待機アニメーションを再生
	m_pAnimator->Play(idle_anim_name, true);

	//移動ステートのポインタを作成
	//コンストラクタ内で作ろうとするとまだGunshipBossのポインタが作られていないのでOnInitで呼ぶ
	m_pState = std::make_shared<GunshipBossMoveState>(
		std::static_pointer_cast<GunshipBoss>(shared_from_this()));
	//ステートの開始処理を呼ぶ
	m_pState->Enter();
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
	//メタリックマップ(滑らかさ。-1で渡すとツヤが広く強く出て、正面が白く飛ぶ)
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::Boss2Metallic);
	//エミッションマップ(通常。放熱中の赤い版Boss2EmissionVentは、放熱を作るときに切り替える)
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::Boss2Emission);

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
}

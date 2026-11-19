#include <vector>
#include <cassert>

#include "GunshipBoss.h"
#include "Manager/ResourceLoader.h"
#include "Constants/ShaderRegister.h"
#include "Utility/ModelAnimator.h"

namespace
{
	//第二形態になるときに表示する壊れたポッドのフレーム名
	constexpr const wchar_t* broken_pod_frame_name = L"Boss2_PodR_Broken";
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
	//エミッションマップ(通常。放熱中の赤い版Boss2EmissionVentは、放熱を作るときに切り替える)
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::Boss2Emission);

	//ハンドルとレジスタ番号をセットで保持しておく
	std::vector<std::pair<int, int>> textures;
	textures.push_back({ ShaderRegister::tex_normal,normGraphH });//法線マップ
	textures.push_back({ ShaderRegister::tex_metalic,-1 });//メタリックマップはないので-1
	textures.push_back({ ShaderRegister::tex_emission,emissionGraphH });//エミッションマップ
	textures.push_back({ ShaderRegister::tex_noise,-1 });//ノイズテクスチャもなし

	//スキンメッシュ(ボーンで部品を動かす)なのでtrueで描画
	DrawWithLighting(textures, true);
}

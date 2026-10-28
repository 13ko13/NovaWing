#include <DxLib.h>

#include "Scenery.h"
#include <cmath>

#include "Manager/WaterRevealManager.h"

namespace
{
	//夜の雰囲気を残しつつ、岩のテクスチャが暗くつぶれないようにするための明るさの設定
	const COLOR_F emissive_color = { 0.35f, 0.35f, 0.39f, 1.0f };//光が当たらない面にも足す自発光
	const COLOR_F diffuse_color = { 1.4f, 1.4f, 1.4f, 1.0f };//光が当たる面の明るさ
	const COLOR_F ambient_color = { 0.55f, 0.55f, 0.55f, 1.0f };//環境光の受けやすさ

	//景色をループさせる1周の長さ(Scenery.csvのz座標が0〜この値の範囲で、端がつながるように作ってある)
	const float loop_length = 24000.0f;
	//カメラがこの距離より先に進んだ景色は、1周分前方へ移して使い回す
	const float recycle_behind_distance = 3000.0f;
}

Scenery::Scenery(std::weak_ptr<CameraBase> pCamera, const SceneryData& data) :
	Actor(data.modelId, pCamera),
	m_baseZ(data.pos.z)
{
	//位置と回転をセット
	SetPos(data.pos);
	m_rotation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), data.rotYRadian);

	//大きさを設定
	m_scale = data.scale;

	//マテリアルごとに明るさを設定する
	int matNum = MV1GetMaterialNum(m_modelHandle);
	for (int i = 0; i < matNum; i++)
	{
		MV1SetMaterialEmiColor(m_modelHandle, i, emissive_color);
		MV1SetMaterialDifColor(m_modelHandle, i, diffuse_color);
		MV1SetMaterialAmbColor(m_modelHandle, i, ambient_color);
	}
}

void Scenery::OnInit()
{
	//キャプチャ時に使うシェーダ用の定数バッファを作成
	CreateShaderBuffers();
}

void Scenery::Update()
{
	//海と同じく途切れないように、カメラの少し後ろより前方で、
	//一番手前になる位置(CSVのz + 周回数 * 1周の長さ)に置き直す
	float minZ = GetCameraPos().z - recycle_behind_distance;
	float loopCount = std::ceil((minZ - m_baseZ) / loop_length);
	m_pos.z = m_baseZ + loopCount * loop_length;
}

void Scenery::Draw()
{
	//行列適用
	ApplyMatrix(m_scale, m_pos, m_rotation, m_modelHandle);

	if (WaterRevealManager::GetInstance().IsCaptureMode())
	{
		//水面越しに透かして見せるため、キャプチャ時は岩などと同じ
		//専用のシェーダ(色とカメラからの距離を書き込む)で描画する
		UpdateShaderMatrixData();
		DrawWithLighting({});
	}
	else
	{
		//通常時はモデルのテクスチャをDxLib標準の描画でそのまま描画する
		MV1DrawModel(m_modelHandle);
	}
}
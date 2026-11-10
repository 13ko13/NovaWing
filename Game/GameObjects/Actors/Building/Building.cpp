#include <cmath>

#include "Building.h"
#include "Utility/Quaternion.h"
#include "Game/Collision/BuildingCollider.h"
#include "Manager/DebugManager.h"

namespace
{
	//カメラがこの距離より先に進んだ景色は、1周分前方へ移して使い回す
	const float recycle_behind_distance = 1000.0f;
}

Building::Building(std::weak_ptr<CameraBase> pCamera, const BuildingData& data) :
	Actor(data.modelId, pCamera)
{
	//モデルの大きさ、位置、回転を設定する
	m_scale = data.scale;
	SetPos(data.pos);
	m_rotation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), data.rotYRadian);

	//受け取った当たり判定モデルIDをもとにモデルを複製してハンドルを取得する
	m_colModelHandle = MV1DuplicateModel(
		ResourceLoader::GetInstance().GetModel(data.colModelID));
	//当たり判定用モデルに行列を適用
	ApplyMatrix(m_scale, m_pos, m_rotation, m_colModelHandle);
	//当たり判定の情報を設定する
	MV1SetupCollInfo(m_colModelHandle);
	//当たり判定を生成する
	m_pCollider = std::make_unique<BuildingCollider>(*this, m_colModelHandle);
	//ループするかを設定
	m_isLoop = data.isLoop;
	//ボスの登場時のからのステージ1周分の長さを受け取る
	m_oneLapDistance = data.oneLapDistance;
	//CSVに書かれていたZ座標を受け取る
	//元のZ座標
	m_baseZ = data.pos.z;
}

Building::~Building()
{
	MV1DeleteModel(m_colModelHandle);
}

void Building::OnInit()
{
}

void Building::Update()
{
	//ループしないビルなら処理をスキップ
	if (!m_isLoop) return;

	//カメラの後ろ側の境目を求める
	float minZ = GetCameraPos().z - recycle_behind_distance;

	//最初に配置したZ座標がカメラの境目より奥にあるなら
	//まだ使いまわしの対象ではないので処理をスキップ
	if (m_baseZ > minZ) return;

	//現在の周回数を求める
	//ceilは小数点以下の数値を切り上げる
	int loopNum = static_cast<int>(std::ceil((minZ - m_baseZ) / m_oneLapDistance));
	//使いまわしたときの新しいZ値
	float recycledZ = m_baseZ + loopNum * m_oneLapDistance;

	//新しいZ値が現在の位置のZ値と同じなら何もしない
	if (recycledZ == m_pos.z) return;
	else
	{
		//違うときだけ新しいZを設定して当たり判定用のモデルの位置を再設定
		m_pos.z = recycledZ;
		ApplyMatrix(m_scale, m_pos, m_rotation, m_colModelHandle);
		//当たり判定の情報を新しい位置で作り直す
		MV1RefreshCollInfo(m_colModelHandle);
	}
}

void Building::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(m_scale, m_pos, m_rotation, m_modelHandle);
	MV1DrawModel(m_modelHandle);

#ifdef _DEBUG
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		for (auto& shape : m_pCollider->GetCollision())
		{
			shape->Draw(0xff0000);
		}
	}
#endif
}

ICollider& Building::GetCollider()
{
	return *m_pCollider;
}

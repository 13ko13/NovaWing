#include "Building.h"
#include "Utility/Quaternion.h"
#include "Game/Collision/BuildingCollider.h"
#include "Manager/DebugManager.h"

Building::Building(std::weak_ptr<CameraBase> pCamera,const BuildingData& data) :
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

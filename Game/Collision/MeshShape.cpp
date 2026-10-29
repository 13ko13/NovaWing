#include <DxLib.h>

#include "MeshShape.h"

MeshShape::MeshShape(int modelHandle) :
	ColliderShape(Shape::Mesh),
	m_colModelHandle(modelHandle)
{
}

MeshShape::~MeshShape()
{
}

void MeshShape::Draw(unsigned int color, bool isFill)
{
	//モデルをワイヤーフレーム描画する
	MV1SetWireFrameDrawFlag(m_colModelHandle, TRUE);
	//モデル描画
	MV1DrawModel(m_colModelHandle);
	//ワイヤーフレーム描画を停止
	MV1SetWireFrameDrawFlag(m_colModelHandle, FALSE);
}

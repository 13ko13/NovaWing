#include <DxLib.h>

#include "Actor.h"
#include "Utility/Matrix4x4.h"

Actor::Actor(ResourceLoader::ModelID modelID) 
{
	//受け取ったモデルIDをもとにモデルを複製してハンドルを取得する
	m_modelHandle = MV1DuplicateModel(
		ResourceLoader::GetInstance().GetModel(modelID));
}

Actor::~Actor()
{
	//処理なし
}

void Actor::ApplyMatrix(const Vector3& scale, const Vector3& pos,
	Quaternion& rotation, int modelHandle)
{
	Matrix4x4 mat;
	//拡大縮小行列
	Matrix4x4 scaleMat = Matrix4x4::Scale(scale);
	mat = scaleMat;
	//回転行列
	//m_rotationをMatrix4x4に変換
	Matrix4x4 rotMat = rotation.ToMatrix4x4();
	mat = mat * rotMat;//合成
	//移動行列
	Matrix4x4 transMat = Matrix4x4::Translate(pos);
	mat = mat * transMat;//合成
	//モデルに適用
	MV1SetMatrix(modelHandle, mat.ToDxLib());
}
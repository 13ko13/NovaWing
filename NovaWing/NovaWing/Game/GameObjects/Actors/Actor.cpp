#include <DxLib.h>

#include "Actor.h"

Actor::Actor(ResourceLoader::ModelID modelId) :
	m_velocity(0.0f, 0.0f, 0.0f)
{
	//受け取ったモデルIDをもとにモデルを複製してハンドルを取得する
	m_modelHandle = MV1DuplicateModel(
		ResourceLoader::GetInstance().GetModel(modelId));
}

Actor::~Actor()
{
	//処理なし
}
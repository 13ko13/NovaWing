#include <DxLib.h>

#include "WaterManager.h"
#include "Utility/Size.h"

namespace
{
	//グリッドの縦の分割数
	constexpr int vertical_grid_num = 11;
	//グリッドの横の分割数
	constexpr int horizontal_grid_num = 11;

	//グリッド全体の広さ
	constexpr Size grid_size = { 3000.0f,3000.0f };
}

WaterManager::WaterManager()
{
}

WaterManager::~WaterManager()
{
}

void WaterManager::Init()
{
	//頂点データを作成
	CreateVertexData();

	//インデックスデータを生成
	//四角形の数分ループしたいので、
	//頂点数-1をする
	for (int i = 0; i < vertical_grid_num - 1; i++)
	{
		for (int j = 0; j < horizontal_grid_num - 1; j++)
		{
			unsigned short leftTopIndex = i * horizontal_grid_num + j;//左上
			unsigned short rightTopIndex = i * horizontal_grid_num + j + 1;//右上
			unsigned short leftBotIndex = (i + 1) * horizontal_grid_num + j;//左下
			unsigned short rightBotIndex = (i + 1) * horizontal_grid_num + j + 1;//右下
			
			//この4つの頂点を使って四角形を作成
			m_indices.push_back(leftTopIndex);
			m_indices.push_back(rightTopIndex);
			m_indices.push_back(leftBotIndex);
			m_indices.push_back(leftBotIndex);
			m_indices.push_back(rightTopIndex);
			m_indices.push_back(rightBotIndex);
		}
	}
}

void WaterManager::CreateVertexData()
{
	//グリッド数分の頂点データを生成
	for (int i = 0; i < vertical_grid_num; i++)
	{
		for (int j = 0; j < horizontal_grid_num; j++)
		{
			VERTEX3D vertexData;
			//頂点の座標
			vertexData.pos.x =
				j * (grid_size.m_width / horizontal_grid_num);
			vertexData.pos.y = 0.0f;//高さは波の高さなのでシェーダ側で動かす
			vertexData.pos.z = i * (grid_size.m_height / vertical_grid_num);

			//頂点の法線
			//真上を向かせる
			vertexData.norm.x = 0.0f;
			vertexData.norm.y = 1.0f;
			vertexData.norm.z = 0.0f;

			//UV
			//0～1
			//グリッド全体に対してどの位置にあるかを表す
			vertexData.u = static_cast<float>(j) /
				static_cast<float>(horizontal_grid_num);//U
			vertexData.v = static_cast<float>(i) /
				static_cast<float>(vertical_grid_num);//V

			//頂点データを収めている配列に入れる
			m_vertexData.push_back(vertexData);
		}
	}
}
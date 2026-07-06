#pragma once
#include <vector>

class WaterManager
{
public:
	WaterManager();
	~WaterManager();

	void Init();//初期化処理

	//頂点データを生成する
	void CreateVertexData();

private:
	//フレームカウンター
	int m_frame = 0;

	//↓なぜunsigned shortなのか
	//intは32ビット、unsigned shordは16ビット
	//海のグリッド程度ならばそこまで頂点数は多くならないので、
	//unsigned shortを使用する
	std::vector<unsigned short> m_indices;

	//グリッドのサイズの頂点数分の頂点データを作成する
	std::vector<VERTEX3D> m_vertexData;

	//頂点バッファのハンドル
	int m_vertexBufferH;
	//インデックスバッファのハンドル
	int m_indexBufferH;
};


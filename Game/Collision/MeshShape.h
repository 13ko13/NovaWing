#pragma once

#include "ColliderShape.h"

class MeshShape : public ColliderShape
{
public:
	MeshShape(int modelHandle);
	virtual ~MeshShape();
	//描画
	void Draw(unsigned int color, bool isFill) override;

	//モデルハンドル取得
	int GetModelHandle() const { return m_colModelHandle; }

private:
	int m_colModelHandle = -1;
};

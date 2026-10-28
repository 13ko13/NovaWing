#pragma once

#include "../Game/Collision/ColliderShape.h"

class MeshShape : public ColliderShape
{
public:
	MeshShape();
	virtual ~MeshShape();
	void Draw(unsigned int color, bool isFill) override;

private:
	int m_colModelHandle = -1;
};

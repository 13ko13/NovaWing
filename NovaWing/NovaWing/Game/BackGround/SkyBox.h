#pragma once
#include "Utility/Vector3.h"

class SkyBox
{
public:
	SkyBox(const int flontHandle,
		const int backHandle,
		const int leftHandle,
		const int rightHandle,
		const int upHandle,
		const int downHandle);
	~SkyBox();

	void Draw(const Vector3& cameraPos);

private:
	enum class Face
	{
		Front = 0,
		Back,
		Left,
		Right,
		Up,
		Down,
		
		FaceNum,
	};

	int m_skyTextures[static_cast<int>(Face::FaceNum)]; //空のテクスチャIDを格納する配列
	
	void DrawFace(
		const Vector3& leftTop,
		const Vector3& rightTop,
		const Vector3& rightBottom,
		const Vector3& leftBottom,
		int graphHandle);
};
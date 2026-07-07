#include <DxLib.h>
#include <cassert>

#include "SkyBox.h"

namespace
{
	//1つの壁の頂点の数
	constexpr int verticesNum = 6;

	//頂点のインデックス
	constexpr int LeftTop = 0;
	constexpr int RightTop1 = 1;
	constexpr int LeftBottom1 = 2;
	constexpr int RightTop2 = 3;
	constexpr int RightBottom = 4;
	constexpr int LeftBottom2 = 5;

	//カメラからスカイボックスまでの距離
	constexpr float distance = 1500.0f;

	//1つの壁に対するポリゴンの数
	constexpr int polygonsNum = 2;
}

SkyBox::SkyBox(const int flontHandle,
		const int backHandle,
		const int leftHandle,
		const int rightHandle,
		const int upHandle,
		const int downHandle)
{
	//テクスチャの読み込み
	m_skyTextures[static_cast<int>(Face::Front)] = flontHandle;//前面
	assert(m_skyTextures[static_cast<int>(Face::Front)] != -1);
	m_skyTextures[static_cast<int>(Face::Back)] = backHandle;//背面
	assert(m_skyTextures[static_cast<int>(Face::Back)] != -1);
	m_skyTextures[static_cast<int>(Face::Left)] = leftHandle;//左面
	assert(m_skyTextures[static_cast<int>(Face::Left)] != -1);
	m_skyTextures[static_cast<int>(Face::Right)] = rightHandle;//右面
	assert(m_skyTextures[static_cast<int>(Face::Right)] != -1);
	m_skyTextures[static_cast<int>(Face::Up)] = upHandle;//上面
	assert(m_skyTextures[static_cast<int>(Face::Up)] != -1);
	m_skyTextures[static_cast<int>(Face::Down)] = downHandle;//下面
	assert(m_skyTextures[static_cast<int>(Face::Down)] != -1);
}

SkyBox::~SkyBox()
{
	//テクスチャの解放はSceneMainで行うためここでは何もしない
}

void SkyBox::Draw(const Vector3& cameraPos)
{
	//スカイボックス描画用の設定
	SetUseLighting(FALSE);		//ライティング無効（頂点法線不要に）
	SetUseZBuffer3D(FALSE);		//Zバッファテスト無効
	SetWriteZBuffer3D(FALSE);	//Zバッファ書き込み無効

	//カメラの位置を中心にして、各面を描画する
	//前面
	DrawFace(
		cameraPos + Vector3(-distance, distance, distance),	//左上
		cameraPos + Vector3(distance, distance, distance),	//右上
		cameraPos + Vector3(distance, -distance, distance),	//右下
		cameraPos + Vector3(-distance, -distance, distance),	//左下
		m_skyTextures[static_cast<int>(Face::Front)]);

	//背面(内側から-Z方向を見たとき、左が+X、右が-X)
	DrawFace(
		cameraPos + Vector3(distance, distance, -distance),//左上
		cameraPos + Vector3(-distance, distance, -distance),//右上
		cameraPos + Vector3(-distance, -distance, -distance),//右下
		cameraPos + Vector3(distance, -distance, -distance),//左下
		m_skyTextures[static_cast<int>(Face::Back)]);

	//左面
	DrawFace(
		cameraPos + Vector3(-distance, distance, -distance),//左上
		cameraPos + Vector3(-distance, distance, distance),//右上
		cameraPos + Vector3(-distance, -distance, distance),//右下
		cameraPos + Vector3(-distance, -distance, -distance),//左下
		m_skyTextures[static_cast<int>(Face::Left)]);

	//右面
	DrawFace(
		cameraPos + Vector3(distance, distance, distance),//左上
		cameraPos + Vector3(distance, distance, -distance),//右上
		cameraPos + Vector3(distance, -distance, -distance),//右下
		cameraPos + Vector3(distance, -distance, distance),//左下
		m_skyTextures[static_cast<int>(Face::Right)]);

	//上面
	DrawFace(
		cameraPos + Vector3(-distance, distance, -distance),//左上
		cameraPos + Vector3(distance, distance, -distance),//右上
		cameraPos + Vector3(distance, distance, distance),//右下
		cameraPos + Vector3(-distance, distance, distance),//左下
		m_skyTextures[static_cast<int>(Face::Up)]);

	//下面(内側から-Y方向を見たとき、左が-X、右が+X、上が+Z、下が-Z)
	DrawFace(
		cameraPos + Vector3(-distance, -distance, distance),//左上
		cameraPos + Vector3(distance, -distance, distance),//右上
		cameraPos + Vector3(distance, -distance, -distance),//右下
		cameraPos + Vector3(-distance, -distance, -distance),//左下
		m_skyTextures[static_cast<int>(Face::Down)]);

	//描画設定を元に戻す
	SetUseLighting(TRUE);
	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);
}

void SkyBox::DrawFace(
	const Vector3& leftTop,
	const Vector3& rightTop,
	const Vector3& rightBottom,
	const Vector3& leftBottom,
	int graphHandle)
{
	VERTEX3D vertices[verticesNum];
	// 3頂点がそれぞれ異なる座標を持つようにする
	// 左上・右上・左下の三角形
	vertices[LeftTop].pos = leftTop.ToDxLib();
	vertices[LeftTop].norm = VGet(0.0f, 0.0f, 0.0f);
	vertices[LeftTop].dif = GetColorU8(255, 255, 255, 255);
	vertices[LeftTop].spc = GetColorU8(0, 0, 0, 0);
	vertices[LeftTop].u = 0.0f;
	vertices[LeftTop].v = 0.0f;

	vertices[RightTop1].pos = rightTop.ToDxLib();
	vertices[RightTop1].norm = VGet(0.0f, 0.0f, 0.0f);
	vertices[RightTop1].dif = GetColorU8(255, 255, 255, 255);
	vertices[RightTop1].spc = GetColorU8(0, 0, 0, 0);
	vertices[RightTop1].u = 1.0f;
	vertices[RightTop1].v = 0.0f;

	vertices[LeftBottom1].pos = leftBottom.ToDxLib();
	vertices[LeftBottom1].norm = VGet(0.0f, 0.0f, 0.0f);
	vertices[LeftBottom1].dif = GetColorU8(255, 255, 255, 255);
	vertices[LeftBottom1].spc = GetColorU8(0, 0, 0, 0);
	vertices[LeftBottom1].u = 0.0f;
	vertices[LeftBottom1].v = 1.0f;

	//右上・右下・左下の三角形
	vertices[RightTop2].pos = rightTop.ToDxLib();
	vertices[RightTop2].norm = VGet(0.0f, 0.0f, 0.0f);
	vertices[RightTop2].dif = GetColorU8(255, 255, 255, 255);
	vertices[RightTop2].spc = GetColorU8(0, 0, 0, 0);
	vertices[RightTop2].u = 1.0f;
	vertices[RightTop2].v = 0.0f;
	
	vertices[RightBottom].pos = rightBottom.ToDxLib();
	vertices[RightBottom].norm = VGet(0.0f, 0.0f, 0.0f);
	vertices[RightBottom].dif = GetColorU8(255, 255, 255, 255);
	vertices[RightBottom].spc = GetColorU8(0, 0, 0, 0);
	vertices[RightBottom].u = 1.0f;
	vertices[RightBottom].v = 1.0f;

	vertices[LeftBottom2].pos = leftBottom.ToDxLib();
	vertices[LeftBottom2].norm = VGet(0.0f, 0.0f, 0.0f);
	vertices[LeftBottom2].dif = GetColorU8(255, 255, 255, 255);
	vertices[LeftBottom2].spc = GetColorU8(0, 0, 0, 0);
	vertices[LeftBottom2].u = 0.0f;
	vertices[LeftBottom2].v = 1.0f;

	DrawPolygon3D(vertices, polygonsNum, graphHandle, false);
}


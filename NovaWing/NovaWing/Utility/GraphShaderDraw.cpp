#include <DxLib.h>

#include "GraphShaderDraw.h"
#include "Utility/SizeF.h"
#include "Utility/Size.h"
#include "Utility/Vector2.h"
#include "Constants/ShaderRegister.h"

namespace
{
    //画像の頂点数
    constexpr int vertex_num = 4;
    //頂点
    constexpr int vertex_left_top = 0;//左上
    constexpr int vertex_right_top = 1;//右上
    constexpr int vertex_left_bottom = 2;//左下
    constexpr int vertex_right_bottom = 3;//右下

    //頂点を組み合わせて三角形を作るときのインデックス数
    constexpr int indices_num = 6;
    //画像を表示するときのポリゴンの数
    constexpr int polygon_num = 2;
}

void DrawRectHorizontalGraphToShader(
    float left, float top,
    const SizeF& size,
    float uvMaxU, int texH)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

    //4頂点分の頂点データ
    VERTEX2DSHADER vertexDatas[vertex_num];
    //1つ目の頂点のデータ
    //位置
    vertexDatas[vertex_left_top].pos = Vector2(left, top).ToDxLib();//左上
    vertexDatas[vertex_right_top].pos = Vector2(left + size.m_width * uvMaxU, top).ToDxLib();//右上
    vertexDatas[vertex_left_bottom].pos = Vector2(left, top + size.m_height).ToDxLib();//左下
    vertexDatas[vertex_right_bottom].pos = Vector2(left + size.m_width * uvMaxU, top + size.m_height).ToDxLib();//右下

    //UV
    //左上
    //右上と右下はHPが減るので割合位置をかける
    vertexDatas[vertex_left_top].u = 0.0f;
    vertexDatas[vertex_left_top].v = 0.0f;
    //右上
    vertexDatas[vertex_right_top].u = 1.0f * uvMaxU;
    vertexDatas[vertex_right_top].v = 0.0f;
    //左下
    vertexDatas[vertex_left_bottom].u = 0.0f;
    vertexDatas[vertex_left_bottom].v = 1.0f;
    //右下
    vertexDatas[vertex_right_bottom].u = 1.0f * uvMaxU;
    vertexDatas[vertex_right_bottom].v = 1.0f;

    //スペキュラとディフューズとrhwとsuvを設定
    for (int i = 0; i < vertex_num; i++)
    {
        vertexDatas[i].spc = GetColorU8(0, 0, 0, 0);
        vertexDatas[i].dif = GetColorU8(255, 255, 255, 255);
        vertexDatas[i].rhw = 1.0f;
        vertexDatas[i].su = vertexDatas[i].u;
        vertexDatas[i].sv = vertexDatas[i].v;
    }

    //頂点インデックスの配列
    unsigned short indices[indices_num];
    //1つ目の三角形
    indices[0] = vertex_left_top;//左上
    indices[1] = vertex_right_top;//右上
    indices[2] = vertex_left_bottom;//左下

    //2つ目の三角形
    indices[3] = vertex_left_bottom;//左下
    indices[4] = vertex_right_top;//右上
    indices[5] = vertex_right_bottom;//右下
    //テクスチャを渡す
    SetUseTextureToShader(ShaderRegister::ui_tex_diffuse, texH);
    //画像を描画する
    DrawPolygonIndexed2DToShader(vertexDatas, vertex_num, indices, polygon_num);
    //テクスチャを解除
    SetUseTextureToShader(ShaderRegister::ui_tex_diffuse, -1);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void DrawRectVerticalGraphToShader(
    float left, float top,
    const SizeF& size, float uvMaxV, int texH)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

    //4頂点分の頂点データ
    VERTEX2DSHADER vertexDatas[vertex_num];
    //1つ目の頂点のデータ
    //位置
    vertexDatas[vertex_left_top].pos = Vector2(left, top + 
         size.m_height * (1.0f - uvMaxV)).ToDxLib();//左上
    vertexDatas[vertex_right_top].pos = Vector2(left + size.m_width, top +
         size.m_height * (1.0f - uvMaxV)).ToDxLib();//右上
    vertexDatas[vertex_left_bottom].pos = Vector2(left, top + size.m_height).ToDxLib();//左下
    vertexDatas[vertex_right_bottom].pos = Vector2(left + size.m_width, top + size.m_height).ToDxLib();//右下

    //UV
    //左上と右上はHPが減るので割合位置をかける
    //左上
    vertexDatas[vertex_left_top].u = 0.0f;
    vertexDatas[vertex_left_top].v = 1.0f - uvMaxV;
    //右上
    vertexDatas[vertex_right_top].u = 1.0f;
    vertexDatas[vertex_right_top].v = 1.0f - uvMaxV;
    //左下
    vertexDatas[vertex_left_bottom].u = 0.0f;
    vertexDatas[vertex_left_bottom].v = 1.0f;
    //右下
    vertexDatas[vertex_right_bottom].u = 1.0f;
    vertexDatas[vertex_right_bottom].v = 1.0f;

    //スペキュラとディフューズとrhwとsuvを設定
    for (int i = 0; i < vertex_num; i++)
    {
        vertexDatas[i].spc = GetColorU8(0, 0, 0, 0);
        vertexDatas[i].dif = GetColorU8(255, 255, 255, 255);
        vertexDatas[i].rhw = 1.0f;
        vertexDatas[i].su = vertexDatas[i].u;
        vertexDatas[i].sv = vertexDatas[i].v;
    }

    //頂点インデックスの配列
    unsigned short indices[indices_num];
    //1つ目の三角形
    indices[0] = vertex_left_top;//左上
    indices[1] = vertex_right_top;//右上
    indices[2] = vertex_left_bottom;//左下

    //2つ目の三角形
    indices[3] = vertex_left_bottom;//左下
    indices[4] = vertex_right_top;//右上
    indices[5] = vertex_right_bottom;//右下
    //テクスチャを渡す
    SetUseTextureToShader(ShaderRegister::ui_tex_diffuse, texH);
    //画像を描画する
    DrawPolygonIndexed2DToShader(vertexDatas, vertex_num, indices, polygon_num);
    //テクスチャを解除
    SetUseTextureToShader(ShaderRegister::ui_tex_diffuse, -1);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void DrawGraphToShaderByCenter(
    float centerX, float centerY,
     double scale, int texH, float uvMaxU)
{
    //画像の幅と高さを取得
	Size texSize;
	GetGraphSize(
		texH,
		&texSize.m_width,
		&texSize.m_height
	);
	//float型のSizeに変換
	SizeF texSizeSizeF = 
	{
		texSize.m_width * scale,
		texSize.m_height * scale
	};

	//左上座標を計算
	Vector2 leftTopPos = Vector2(
		centerX - texSizeSizeF.m_width / 2,
		centerY - texSizeSizeF.m_height / 2
	);

	//画像をシェーダーを通して描画
	DrawRectHorizontalGraphToShader(
		leftTopPos.m_x,
		leftTopPos.m_y,
		texSizeSizeF,uvMaxU,texH
	);
}

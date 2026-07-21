#pragma once

struct SizeF;
//フリーの共通関数
//矩形1枚をシェーダ付きで描画する
void DrawGraphToShader(
        float left, float top,
        const SizeF& size, float uvMaxU, int texH
);

//中心座標+スケールを指定して、シェーダ付きで描画する
void DrawGraphToShaderByCenter(
    float centerX,float centerY,
    double scale,int texH,float uvMaxU = 1.0f
);


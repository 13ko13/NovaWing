#pragma once

struct SizeF;
//フリーの共通関数
//矩形1枚をシェーダ付きで描画する
void DrawRectHorizontalGraphToShader(
    float left, float top,
    const SizeF& size, float uvMaxU, 
    int texH, float alpha = 1.0f,float uvMinU = 0.0f
);

void DrawRectVerticalGraphToShader(
        float left, float top,
        const SizeF& size, float uvMaxV, int texH
);

//中心座標+スケールを指定して、シェーダ付きで描画する
void DrawGraphToShaderByCenter(
    float centerX, float centerY,
    double scale, int texH, float alpha = 1.0f, float uvMaxU = 1.0f, float uvMinU = 0.0f
);


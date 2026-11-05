// 新規ファイルの雛形(リポジトリの既存コードに合わせる: C++ はタブ + オールマン、HLSL は空白4つ)

/**
 * @param name クラス名
 * @param base 継承元 { name, includePath } (継承しないなら undefined。includePath は見つからなければ undefined)
 */
const classHeader = (name, base) => {
    const lines = ['#pragma once'];
    if (base?.includePath) lines.push(`#include "${base.includePath}"`);
    lines.push(
        '',
        base ? `class ${name} : public ${base.name}` : `class ${name}`,
        '{',
        'public:',
        `\t${name}();`,
        base ? `\tvirtual ~${name}();` : `\t~${name}();`,
        '};',
    );
    return lines;
};

const classSource = (name) => [
    `#include "${name}.h"`,
    '',
    `${name}::${name}()`,
    '{',
    '}',
    '',
    `${name}::~${name}()`,
    '{',
    '}',
];

// 3Dモデルに使う頂点シェーダーの出力 = ピクセルシェーダーの入力(LightingVS と同じ並び)
const modelPixelInput = [
    'struct PS_INPUT',
    '{',
    '    float4 pos : SV_Position; //スクリーン空間の位置',
    '    float2 uv : TEXCOORD0; //UV座標',
    '    float3 normalWS : NORMAL; //ワールド空間の法線',
    '    float3 worldPos : TEXCOORD1; //ワールド座標',
    '    float3 tangentWS : TANGENT; //ワールド空間の接線',
    '};',
];

const modelPixelShader = () => [
    'Texture2D<float4> tex : register(t0);',
    'SamplerState smp : register(s0);',
    '',
    ...modelPixelInput,
    '',
    'cbuffer CameraBuffer : register(b6)',
    '{',
    '    float3 cameraPos; //カメラの位置',
    '    float padding; //16バイトアライメント',
    '}',
    '',
    'float4 main(PS_INPUT input) : SV_TARGET',
    '{',
    '    return tex.Sample(smp, input.uv);',
    '}',
];

const screenPixelShader = () => [
    'Texture2D<float4> tex : register(t0);',
    'SamplerState smp : register(s0);',
    '',
    'struct PS_INPUT',
    '{',
    '    float4 pos : SV_Position;',
    '    float2 uv : TEXCOORD0;',
    '};',
    '',
    'float4 main(PS_INPUT input) : SV_TARGET',
    '{',
    '    return tex.Sample(smp, input.uv);',
    '}',
];

const modelVertexOutput = [
    'struct VS_OUTPUT',
    '{',
    '    float4 pos : SV_POSITION; //スクリーン空間の位置',
    '    float2 uv : TEXCOORD0; //UV座標',
    '    float3 normal : NORMAL; //ワールド空間の法線',
    '    float3 worldPos : TEXCOORD1; //ピクセルシェーダーに渡すワールド座標',
    '    float3 tangent : TANGENT; //ワールド空間の接線',
    '};',
];

const matrixAndCameraBuffers = [
    'cbuffer MatrixBuffer : register(b5)',
    '{',
    '    float4x4 world; //ワールド行列',
    '    float4x4 view; //カメラ行列',
    '    float4x4 proj; //プロジェクション行列',
    '}',
    '',
    'cbuffer CameraBuffer : register(b6)',
    '{',
    '    float3 cameraPos; //カメラの位置',
    '    float padding; //16バイトアライメント',
    '}',
];

const modelVertexShader = () => [
    'struct VS_INPUT',
    '{',
    '    float4 pos : POSITION; //モデルの頂点座標',
    '    float3 normal : NORMAL0; //モデルの法線方向',
    '    float4 diffuse : COLOR0; //この二つはないとうまく値が入らない',
    '    float4 specular : COLOR1;',
    '    float2 uv : TEXCOORD0; //ピクセルuv',
    '    float3 tangent : TANGENT; //接線',
    '};',
    '',
    ...modelVertexOutput,
    '',
    ...matrixAndCameraBuffers,
    '',
    'VS_OUTPUT main(VS_INPUT input)',
    '{',
    '    VS_OUTPUT output;',
    '',
    '    //ローカル座標→ワールド座標→ビュー座標→スクリーン座標',
    '    float4 worldPos = mul(input.pos, transpose(world));',
    '    float4 viewPos = mul(worldPos, transpose(view));',
    '    float4 screenPos = mul(viewPos, transpose(proj));',
    '',
    '    //wの値を0にすることで、方向として変換する',
    '    float3 worldNormal = normalize(mul(float4(input.normal.xyz, 0.0f), transpose(world)).xyz);',
    '    float3 worldTangent = normalize(mul(float4(input.tangent.xyz, 0.0f), transpose(world)).xyz);',
    '',
    '    output.pos = screenPos;',
    '    output.normal = worldNormal;',
    '    output.tangent = worldTangent;',
    '    output.uv = input.uv;',
    '    output.worldPos = worldPos.xyz;',
    '',
    '    return output;',
    '}',
];

const skinnedVertexShader = () => [
    'struct VS_INPUT',
    '{',
    '    float3 pos : POSITION;',
    '    float3 normal : NORMAL0;',
    '    float4 diffuse : COLOR0;',
    '    float4 specular : COLOR1;',
    '    float4 uv : TEXCOORD0;',
    '    float4 suv : TEXCOORD1;',
    '    float3 tangent : TANGENT;',
    '    float3 bin : BINORMAL0;',
    '',
    '    int4 blendIndices : BLENDINDICES0; //影響を受けるボーン番号(最大4本)',
    '    float4 blendWeight : BLENDWEIGHT0; //各ボーンの影響度',
    '};',
    '',
    ...modelVertexOutput,
    '',
    ...matrixAndCameraBuffers,
    '',
    'cbuffer LocalWorldMatrixBuffer : register(b3)',
    '{',
    '    float4 localWorldMatrix[162]; //1ボーンにつきfloat4が3行、54ボーン分',
    '}',
    '',
    'VS_OUTPUT main(VS_INPUT input)',
    '{',
    '    //スキニング行列の合成',
    '    float4 skinRow0 = float4(0, 0, 0, 0);',
    '    float4 skinRow1 = float4(0, 0, 0, 0);',
    '    float4 skinRow2 = float4(0, 0, 0, 0);',
    '    [unroll]',
    '    for (int i = 0; i < 4; ++i)',
    '    {',
    '        int bone = input.blendIndices[i];',
    '        float weight = input.blendWeight[i];',
    '        skinRow0 += localWorldMatrix[bone + 0] * weight;',
    '        skinRow1 += localWorldMatrix[bone + 1] * weight;',
    '        skinRow2 += localWorldMatrix[bone + 2] * weight;',
    '    }',
    '',
    '    //頂点の変換(ローカル→ワールド)',
    '    float4 localPos = float4(input.pos.xyz, 1.0f);',
    '    float4 worldPos = float4(dot(localPos, skinRow0), dot(localPos, skinRow1), dot(localPos, skinRow2), 1.0f);',
    '    float4 viewPos = mul(worldPos, transpose(view));',
    '    float4 screenPos = mul(viewPos, transpose(proj));',
    '',
    '    //法線・接線のワールド変換(wを0にして方向として変換する)',
    '    float4 normalVec = float4(input.normal.xyz, 0.0f);',
    '    float3 worldNormal = normalize(float3(dot(normalVec, skinRow0), dot(normalVec, skinRow1), dot(normalVec, skinRow2)));',
    '    float4 tangentVec = float4(input.tangent.xyz, 0.0f);',
    '    float3 worldTangent = normalize(float3(dot(tangentVec, skinRow0), dot(tangentVec, skinRow1), dot(tangentVec, skinRow2)));',
    '',
    '    VS_OUTPUT output;',
    '    output.pos = screenPos;',
    '    output.normal = worldNormal;',
    '    output.tangent = worldTangent;',
    '    output.uv = input.uv.xy;',
    '    output.worldPos = worldPos.xyz;',
    '    return output;',
    '}',
];

const shaderInclude = () => [
    '//共通で使う関数・定数をここに書く',
];

// HLSL の種類。suffix はファイル名の末尾(vcxproj の PS/VS 判定にも使われる)
const SHADER_KINDS = [
    { label: 'ピクセルシェーダー: 3Dモデル用', suffix: 'PS', ext: '.hlsl', lines: modelPixelShader },
    { label: 'ピクセルシェーダー: 画面全体エフェクト用', suffix: 'PS', ext: '.hlsl', lines: screenPixelShader },
    { label: '頂点シェーダー: 3Dモデル用', suffix: 'VS', ext: '.hlsl', lines: modelVertexShader },
    { label: '頂点シェーダー: スキンメッシュ(ボーン付き)用', suffix: 'VS', ext: '.hlsl', lines: skinnedVertexShader },
    { label: '共通インクルード (.hlsli)', suffix: '', ext: '.hlsli', lines: shaderInclude },
];

module.exports = { classHeader, classSource, SHADER_KINDS };

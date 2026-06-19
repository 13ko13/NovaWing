#include <DxLib.h>
#include <cassert>

#include "LightingManager.h"

LightingManager& LightingManager::GetInstance()
{
	static LightingManager instance;
	return instance;
}

LightingManager::LightingManager()
{
	//シェーダをロード
	LoadShader();
}

void LightingManager::ApplyShader()
{
	MV1SetUseOrigShader(true);
	//ライトの方向をセットする
	m_pCBuffLightData->lightDir = m_lightDir;
	//情報を入れたので定数バッファが変わったことをシェーダに知らせる
	UpdateShaderConstantBuffer(m_cbufferLightInfo);
	//頂点シェーダをセット
	SetUseVertexShader(m_lightingVSH);
	//ピクセルシェーダをセットする
	SetUsePixelShader(m_lightingPSH);
	//定数バッファをシェーダにセットする
	SetShaderConstantBuffer(m_cbufferLightInfo, DX_SHADERTYPE_PIXEL, 1);
}

void LightingManager::ResetShader()
{
	//シェーダを解除してデフォルトに戻す
	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 1);
	MV1SetUseOrigShader(false);
}

void LightingManager::LoadShader()
{
	//頂点シェーダとピクセルシェーダのロード
	m_lightingPSH = LoadPixelShader(L"LightingPS.pso");
	m_lightingVSH = LoadVertexShader(L"LightingVS.vso");

	//ロードに失敗していたらクラッシュ
	assert(m_lightingPSH >= 0);
	assert(m_lightingVSH >= 0);

	//バッファを作成する
	m_cbufferLightInfo = CreateShaderConstantBuffer(sizeof(LightBuffer));
	//バッファのアドレスを取得
	//void*なのでLightBuffer*でキャストする
	m_pCBuffLightData = static_cast<LightBuffer*>(GetBufferShaderConstantBuffer(m_cbufferLightInfo));
}
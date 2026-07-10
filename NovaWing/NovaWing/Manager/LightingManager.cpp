#include <DxLib.h>
#include <cassert>

#include "LightingManager.h"
#include "WaterRevealManager.h"

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

void LightingManager::SetLightDirection(const Vector3& lightDir)
{
	//独自シェーダのライティングを設定
	m_lightDir = lightDir;
	ChangeLightTypeDir(lightDir.ToDxLib());
	SetLightDifColor(GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
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

	//WaterRevealManagerのインスタンスを受け取る
	WaterRevealManager& revealManager = WaterRevealManager::GetInstance();
	//キャプチャ用のピクセルシェーダハンドルを受け取る
	int capturePSH = revealManager.GetCapturePSHandle();

	//キャプチャ中の場合はCapturePS,そうじゃない場合はLightingPSを呼ぶ
	if (revealManager.IsCaptureMode())
	{
		//キャプチャ用のシェーダをセット
		SetUsePixelShader(capturePSH);
	}
	else
	{
		//普通のライティング用のシェーダをセット
		SetUsePixelShader(m_lightingPSH);
		//定数バッファをシェーダにセットする
		SetShaderConstantBuffer(m_cbufferLightInfo, DX_SHADERTYPE_PIXEL, 4);
	}
}

void LightingManager::ResetShader()
{
	//シェーダを解除してデフォルトに戻す
	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 4);
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
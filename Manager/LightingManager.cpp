#include <DxLib.h>
#include <cassert>

#include "LightingManager.h"
#include "WaterRevealManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//プレイヤーとカメラの間の距離がどのぐらいの比率になったら完全に消すか
	constexpr float comp_disolve_ratio = 0.55f;
	//消し始める距離
	constexpr float start_disolve_dist = 400.0f;
}

LightingManager& LightingManager::GetInstance()
{
	static LightingManager instance;
	return instance;
}

LightingManager::LightingManager()
{
	//シェーダをロード
	LoadShader();
	m_startDisolveDist = start_disolve_dist;
}

void LightingManager::SetLightDirection(const Vector3& lightDir)
{
	//独自シェーダのライティングを設定
	m_lightDir = lightDir;
	ChangeLightTypeDir(lightDir.ToDxLib());
	SetLightDifColor(GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
}

void LightingManager::ApplyShader(bool isSkinning,float isDisolve)
{
	MV1SetUseOrigShader(true);
	//ライトの方向をセットする
	m_pCBuffLightData->lightDir = m_lightDir;
	m_pCBuffLightData->isDisolveShader = isDisolve;
	//薄れ始める距離をセットする
	m_pCBuffLightData->startDisolveDist = m_startDisolveDist;
	//完全にディゾルブで消える距離をセットする
	m_pCBuffLightData->compDisolveDist = m_startDisolveDist * comp_disolve_ratio;


	//情報を入れたので定数バッファが変わったことをシェーダに知らせる
	UpdateShaderConstantBuffer(m_cbufferLightInfo);

	//頂点シェーダをセット
	//スキニングが必要な時はスキニング用のライティングシェーダを使用する
	if (isSkinning)
	{
		SetUseVertexShader(m_skinnedLightingVSH);
	}
	else
	{
		SetUseVertexShader(m_lightingVSH);
	}

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
		SetShaderConstantBuffer(m_cbufferLightInfo, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_light);
	}
}

void LightingManager::ResetShader()
{
	//シェーダを解除してデフォルトに戻す
	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_light);
	MV1SetUseOrigShader(false);
}

void LightingManager::LoadShader()
{
	//頂点シェーダとピクセルシェーダのロード
	m_lightingPSH = LoadPixelShader(L"LightingPS.pso");
	m_lightingVSH = LoadVertexShader(L"LightingVS.vso");
	m_skinnedLightingVSH = LoadVertexShader(L"SkinnedLightingVS.vso");

	//ロードに失敗していたらクラッシュ
	assert(m_lightingPSH >= 0);
	assert(m_lightingVSH >= 0);
	assert(m_skinnedLightingVSH >= 0);

	//バッファを作成する
	m_cbufferLightInfo = CreateShaderConstantBuffer(sizeof(LightBuffer));
	//バッファのアドレスを取得
	//void*なのでLightBuffer*でキャストする
	m_pCBuffLightData = static_cast<LightBuffer*>(GetBufferShaderConstantBuffer(m_cbufferLightInfo));
}
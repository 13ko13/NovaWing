#pragma once
#include "Utility/Vector3.h"

class LightingManager
{
public:
	//インスタンスを取得
	static LightingManager& GetInstance();
	//コピーと代入を禁止する(消す)
	LightingManager(const LightingManager&) = delete;
	LightingManager& operator=(const LightingManager&) = delete;

	//ライトの方向ベクトルをセットする
	void SetLightDirection(const Vector3& lightDir);
	//シェーダーを適用&定数バッファに値を渡す
	void ApplyShader(bool isSkinning = false);
	//シェーダーを解除する
	void ResetShader();
	//光の情報を持ってる定数バッファのハンドルを返す
	int GetLightCBuffer() const { return m_cbufferLightInfo; }

private:
	//コンストラクタ
	LightingManager();

	//シェーダーのロード
	void LoadShader();

private:
	struct LightBuffer
	{
		Vector3 lightDir;//光の方向ベクトル
		float padding;//16バイトアライメント(16の倍数にそろえる)のため
	};
	int m_cbufferLightInfo = -1;//バッファのハンドル
	LightBuffer* m_pCBuffLightData = nullptr;//光の情報バッファへのポインタ

	//ピクセルシェーダと頂点シェーダのコンパイル済みシェーダハンドル
	int m_lightingPSH = -1;
	int m_lightingVSH = -1;
	//スキニングを行う際の頂点シェーダハンドル
	int m_skinnedLightingVSH = -1;

	//光の方向ベクトル
	Vector3 m_lightDir;
};
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
	void ApplyShader(bool isSkinning = false, float isDisolve = 1.0f);
	//シェーダーを解除する
	void ResetShader();
	//光の情報を持ってる定数バッファのハンドルを返す
	int GetLightCBuffer() const { return m_cbufferLightInfo; }

	//ディゾルブの基準距離を設定する
	void SetStartDisolveDist(float cameraToPlayerDist) { m_startDisolveDist = cameraToPlayerDist; }

private:
	//コンストラクタ
	LightingManager();

	//シェーダーのロード
	void LoadShader();

private:
	struct LightBuffer
	{
		Vector3 lightDir;//光の方向ベクトル
		float isDisolveShader;//ディゾルブ対象かどうか(0:false,1:true)
		float startDisolveDist;//ディゾルブを開始する距離
		float compDisolveDist;//ディゾルブで完全に消える距離
		float padding[2];//16バイトアライメント
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

	//ディゾルブを開始する距離
	float m_startDisolveDist;
};
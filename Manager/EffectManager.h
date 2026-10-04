#pragma once
#include "Manager/ResourceLoader.h"
#include "Utility/Vector3.h"

//Effekseerのエフェクトの再生・更新・描画をまとめて管理する
//再生ハンドルは再生したクラスが持ち、位置の更新や停止はこのクラス経由で行う
class EffectManager
{
public:
	EffectManager();
	~EffectManager();

	//更新処理(Effekseerの同期と更新)
	void Update();

	//描画処理
	void Draw();

	/// <summary>
	/// エフェクトを再生して再生ハンドルを返す(位置の更新や停止を行いたいとき用)
	/// </summary>
	/// <param name="id">再生するエフェクトの種類</param>
	/// <param name="pos">再生位置</param>
	/// <returns>再生ハンドル</returns>
	int Play(ResourceLoader::EffectID id, const Vector3& pos);

	/// <summary>
	/// エフェクトを再生して放置する(被弾・死亡など再生しっぱなしでよいとき用)
	/// </summary>
	/// <param name="id">再生するエフェクトの種類</param>
	/// <param name="pos">再生位置</param>
	void PlayOneShot(ResourceLoader::EffectID id, const Vector3& pos);

	//再生中のエフェクトの位置を設定
	void SetPos(int playH, const Vector3& pos);
	//再生中のエフェクトの回転(ラジアン)を設定
	void SetRotation(int playH, const Vector3& rotation);
	//再生中のエフェクトの回転を軸と角度(ラジアン)で設定
	void SetRotationAxis(int playH, const Vector3& axis, float angle);
	//再生中のエフェクトの大きさを設定
	void SetScale(int playH, const Vector3& scale);
	//再生中のエフェクトの色を設定(0～255)
	void SetColor(int playH, int r, int g, int b, int a);
	//再生中のエフェクトの動的パラメーターを設定
	void SetDynamicInput(int playH, int index, float value);

	//エフェクトが再生中か
	bool IsPlaying(int playH) const;

	//指定のエフェクトを止める
	void Stop(int playH);

	//再生中のエフェクトを全て止める(シーンの終了時などに残さないため)
	void StopAll();
};

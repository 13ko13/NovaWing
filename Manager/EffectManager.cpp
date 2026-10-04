#include <EffekseerForDXLib.h>

#include "EffectManager.h"

EffectManager::EffectManager()
{
}

EffectManager::~EffectManager()
{
	//このマネージャーが消えるときにエフェクトを残さない
	StopAll();
}

void EffectManager::Update()
{
	//DxLibのカメラ設定などをEffekseer側に同期してから更新する
	Effekseer_Sync3DSetting();
	UpdateEffekseer3D();
}

void EffectManager::Draw()
{
	DrawEffekseer3D();
}

int EffectManager::Play(ResourceLoader::EffectID id, const Vector3& pos)
{
	int playH = PlayEffekseer3DEffect(ResourceLoader::GetInstance().GetEffect(id));

	//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
	SetPos(playH, pos);

	return playH;
}

void EffectManager::PlayOneShot(ResourceLoader::EffectID id, const Vector3& pos)
{
	//ハンドルは使わない(再生が終わればEffekseer側で破棄される)
	Play(id, pos);
}

void EffectManager::SetPos(int playH, const Vector3& pos)
{
	SetPosPlayingEffekseer3DEffect(playH, pos.x, pos.y, pos.z);
}

void EffectManager::SetRotation(int playH, const Vector3& rotation)
{
	SetRotationPlayingEffekseer3DEffect(playH, rotation.x, rotation.y, rotation.z);
}

void EffectManager::SetRotationAxis(int playH, const Vector3& axis, float angle)
{
	//DxLibのラッパーに軸と角度で回す関数がないのでEffekseerのマネージャーを直接呼ぶ
	GetEffekseer3DManager()->SetRotation(
		playH, Effekseer::Vector3D(axis.x, axis.y, axis.z), angle);
}

void EffectManager::SetScale(int playH, const Vector3& scale)
{
	SetScalePlayingEffekseer3DEffect(playH, scale.x, scale.y, scale.z);
}

void EffectManager::SetColor(int playH, int r, int g, int b, int a)
{
	SetColorPlayingEffekseer3DEffect(playH, r, g, b, a);
}

void EffectManager::SetDynamicInput(int playH, int index, float value)
{
	SetDynamicInput3DEffect(playH, index, value);
}

bool EffectManager::IsPlaying(int playH) const
{
	//DxLibの関数は再生中なら0、再生していなければ-1を返す
	return IsEffekseer3DEffectPlaying(playH) == 0;
}

void EffectManager::Stop(int playH)
{
	StopEffekseer3DEffect(playH);
}

void EffectManager::StopAll()
{
	//アプリ終了時などEffekseerが既に終了していたら何もしない
	if (Effekseer::ManagerRef manager = GetEffekseer3DManager())
	{
		manager->StopAllEffects();
	}
}

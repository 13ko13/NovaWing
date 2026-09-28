#pragma once
#include "Actor.h"
#include "Manager/ResourceLoader.h"

class TitlePlayer : public Actor
{
public:
	TitlePlayer(ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> pCamera);
	~TitlePlayer();

	//初期化処理
	void OnInit() override;
	//更新処理
	void Update() override;
	//描画
	void Draw() override;

	//前進から宙返りへ進む
	void StartSomersault();
	//加速フェーズへ進める
	void StartBoost();

	//宙返りが終了しているかを返す
	bool IsSomersaultEnd();

	//見た目通りの前方向・後方向が欲しいときはこちらを使う
	Vector3 GetVisualForward() const { return GetBack(); }

private:
	enum class Phase
	{
		Forward,//前進
		Somersault,//宙返り
		Boost,//加速
	};
	Phase m_phase = Phase::Forward;

	//宙返りの時間管理用
	int m_somersaultFrame = 0;

	int m_boostPlayEffect = -1;
};
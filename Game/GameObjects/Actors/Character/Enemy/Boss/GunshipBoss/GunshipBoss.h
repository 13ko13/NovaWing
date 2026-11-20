#pragma once
#include <memory>

#include "Game/GameObjects/Actors/Character/Enemy/Boss/BossBase.h"

class ModelAnimator;
class IGunshipBossState;
class GunshipBoss : public BossBase
{
public:
	struct GunshipBossData : public BossBaseData
	{
		Vector3 pos;
	};
	
	GunshipBoss(GunshipBossData& data);
	virtual ~GunshipBoss();

	//初期化処理
	void OnInit() override;
	//更新
	void Update() override;
	//描画
	void Draw() override;

private:
	//第二形態になるときに表示する壊れたポッドのフレーム番号
	int m_brokenPodFrameIdx = -1;
	//アニメーター
	std::unique_ptr<ModelAnimator> m_pAnimator;
	//現在のステート
	std::shared_ptr<IGunshipBossState> m_pState;
};
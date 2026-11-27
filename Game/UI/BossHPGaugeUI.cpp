#include "BossHPGaugeUI.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/BossBase.h"
#include "Manager/ResourceLoader.h"
#include "Utility/Size.h"
#include "Utility/Vector2.h"
#include "Main/Application.h"

namespace
{
	//HP枠画像の位置
	const Vector2 special_frame_pos_ratio = { 0.9f,0.2f };
	//ゲージを置く位置
	const Vector2 gauge_pos_in_frame = { 30.0f,62.0f };
	//スキャンラインを入れる周期
	constexpr float scanline_frequency = 655.0f;

	//攻撃を受けたときに何フレームゲージを揺らすか
	constexpr int shake_frame = 10;
}

BossHPGaugeUI::BossHPGaugeUI(std::weak_ptr<BossBase> pBoss) :
	m_pBoss(pBoss)
{
	//スキャンラインを入れる周期をシェーダに渡す
	m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
	UpdateShaderConstantBuffer(m_cbufferGlitch);
}

BossHPGaugeUI::~BossHPGaugeUI()
{

}

void BossHPGaugeUI::Update()
{
	//基底クラスの更新処理
	GaugeUIBase::Update();

	//合計ダメージが増えていたら揺れ始める
	float totalHitDamage = m_pBoss.lock()->GetTotalHitDamage();
	if (m_prevTotalHitDamage < totalHitDamage)
	{
		m_shakeFrame = shake_frame;
	}
	//揺れのフレームを更新
	m_shakeFrame--;

	m_prevTotalHitDamage = totalHitDamage;
}

void BossHPGaugeUI::Draw()
{
	//ボスの出現が完了していたら描画する
	if (m_pBoss.lock()->IsBossAppear())
	{
		//HPの枠画像を取得
		int hpFrameHandle = ResourceLoader::GetInstance().GetGraphic(
			ResourceLoader::GraphicID::BossHPFrame
		);
		//HPゲージの画像を取得
		int hpGaugeHandle = ResourceLoader::GetInstance().GetGraphic(
			ResourceLoader::GraphicID::BossHPGauge
		);

		//ウィンドウサイズ
		const Size& wsize = Application::GetInstance().GetWindowSize();
		//揺れのフレームが残っているなら位置をランダムにずらす
		Vector2 frameDrawPos;
		if (m_shakeFrame > 0)
		{
			frameDrawPos = Vector2(
				wsize.width * special_frame_pos_ratio.x + GetRand(10) - 5,
				wsize.height * special_frame_pos_ratio.y + GetRand(10) - 5
			);
		}
		else
		{
			frameDrawPos = Vector2(
				wsize.width * special_frame_pos_ratio.x,
				wsize.height * special_frame_pos_ratio.y
			);
		}

		//HPの割合から、切り取り位置を計算
		float ratio =
			static_cast<float>(m_pBoss.lock()->GetHealth()) /
			static_cast<float>(m_pBoss.lock()->GetMaxHealth());

		//どちらもシェーダを通して描画する
		DrawGauge(
			hpFrameHandle,
			hpGaugeHandle,
			frameDrawPos,
			gauge_pos_in_frame,
			ratio,
			true
		);
	}

#ifdef _DEBUG

#endif
}
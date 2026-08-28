#define NOMINMAX

#include "ReticleUI.h"
#include "Charactor/Player/Player.h"
#include "Game/GameObjects/GameObject.h"
#include "Manager/ResourceLoader.h"
#include "Manager/TargetManager.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"

namespace
{
	// 手前のレティクルの大きさ
	constexpr double front_reticle_size = 0.2;
	// 奥のレティクルの大きさ
	constexpr double depth_reticle_size = 0.1;
	// チャージレティクルの大きさ
	constexpr double charge_reticle_size = 0.2;

	// フォーカスのアルファ値が1フレームでどれぐらい変化するか
	constexpr float focus_alpha_speed = 25.0f;

	// アニメーションスピード
	constexpr float anim_speed = 1.0f / 20.0f;
	// チャージレティクルの最大サイズ
	constexpr double charge_reticle_max_size = 0.8;
	// 最大回転角
	constexpr double max_rotation = DX_TWO_PI_F;
} // namespace

ReticleUI::ReticleUI(
	std::weak_ptr<TargetManager> pTargetManager,
	std::weak_ptr<Player> pPlayer) : 
	m_pTargetManager(pTargetManager),
	m_pPlayer(pPlayer)
{
}

ReticleUI::~ReticleUI()
{
}

void ReticleUI::Draw()
{
	// ターゲットマネージャーをshared_ptrに変換
	std::shared_ptr<TargetManager> pTargetManager = m_pTargetManager.lock();
	// ターゲットマネージャーから奥のレティクル位置を取得
	Vector3 reticlePos = pTargetManager->GetReticlePos();
	// 手前のレティクルの位置
	Vector3 frontReticlePos = pTargetManager->GetFrontReticlePos();

	// まだレティクル位置が3D座標なのでスクリーン座標
	VECTOR reticleScreenPos = ConvWorldPosToScreenPos(reticlePos.ToDxLib());
	VECTOR frontScreenPos = ConvWorldPosToScreenPos(frontReticlePos.ToDxLib());

	// レティクルの画像を取得
	int normalH = ResourceLoader::GetInstance().GetGraphic(ResourceLoader::GraphicID::NormalReticle);

	// チャージレティクルの画像を取得
	int chargeH = ResourceLoader::GetInstance().GetGraphic(ResourceLoader::GraphicID::ChargeReticle);

	// 通常レティクル二枚を描画
	DrawRotaGraph(frontScreenPos.x, frontScreenPos.y, front_reticle_size, 0.0, normalH, true);	   // 手前のレティクル
	DrawRotaGraph(reticleScreenPos.x, reticleScreenPos.y, depth_reticle_size, 0.0, normalH, true); // 奥のレティクル

	// フォーカス中なら
	if (pTargetManager->IsFocus())
	{
		// アニメーションを進める
		m_animProgress += anim_speed;
		m_animProgress = std::fmin(m_animProgress, 1.0f);
		// スケールをLerpで計算
		double scale = charge_reticle_max_size * (1.0f - m_animProgress) +
					   charge_reticle_size * m_animProgress;
		// 回転角を計算
		double rotation = max_rotation * m_animProgress;

		// フォーカス時のレティクルのアルファを上げる
		m_focusAlpha += focus_alpha_speed;
		m_focusAlpha = std::fmin(m_focusAlpha, 255.0f);

		// フォーカスしているオブジェクトの位置
		std::weak_ptr<EnemyBase> target = pTargetManager->GetFocusTarget();
		std::shared_ptr<EnemyBase> sharedTarget = target.lock();
		// ターゲットが死んでいたら処理を行わない
		if (sharedTarget && !sharedTarget->IsDead())
		{
			Vector3 targetPos = sharedTarget->GetPos();
			// それをスクリーン座標に直す
			VECTOR targetScreenPos = ConvWorldPosToScreenPos(targetPos.ToDxLib());

			// アルファを適用
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(m_focusAlpha));
			// チャージレティクル描画
			DrawRotaGraph(targetScreenPos.x, targetScreenPos.y, scale, rotation, chargeH, true); // 奥のレティクル
			// ブレンドを終了
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
	}
	// フォーカス中じゃなければ
	else
	{
		// レティクルのアルファを下げる
		m_focusAlpha = 0.0f;

		// アニメーション進行度リセット
		m_animProgress = 0.0f;
	}
}

void ReticleUI::Update()
{
	// 処理なし
}

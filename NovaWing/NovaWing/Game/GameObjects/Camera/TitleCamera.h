#pragma once
#include "CameraBase.h"

class TitlePlayer;
class TitleCamera : public CameraBase
{
public:
	TitleCamera(std::shared_ptr<TitlePlayer> pPlayer);
	~TitleCamera();

	//カメラの位置更新
	void UpdatePosition() override;
	//描画
	void Draw() override;
	
	//プレイヤー追従をやめさせる関数
	void StopFollowing();

private:
	//プレイヤーへのポインタ
	std::shared_ptr<TitlePlayer> m_pPlayer;

	//プレイヤーを追従するか
	//最初は追従から始まるのでtrue
	bool m_isFollowing = true;
};
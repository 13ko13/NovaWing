#include "SceneMain.h"
#include "DxLib.h"

SceneMain::SceneMain() :
	m_frameCount(0),
	m_fov(DX_PI_F / 3.0f)//60度
{
}

SceneMain::~SceneMain()
{
}

void SceneMain::Init()
{
	//原点(0,0,0)が画面中央に来るようにカメラを設定
	SetCameraPositionAndTarget_UpVecY(VGet(0.0f, 300.0f, -500.0f), VGet(0.0f, 0.0f, 0.0f));

	//視野角を設定する
	SetupCamera_Perspective(m_fov);
	//SetupCamera_Perspective(DX_PI_F / 3.0f);	//60度 多分Dxライブラリのデフォルトの視野角
	//SetupCamera_Perspective(DX_PI_F / 12.0f);	//15度 大きく表示されるようになった
	//SetupCamera_Perspective(DX_PI_F * 2.0f / 3.0f);	//120度 小さく表示されるようになった

	//カメラのNear,Farを設定する
	//特にFarは大きすぎる数値を設定しないようにする
	//できる限り使う範囲のみを収めるように設定する
	SetCameraNearFar(0.1f, 1500.0f);
}

void SceneMain::Update()
{
	m_frameCount++;

	//パッドのボタンでカメラの視野角を変更可能にする
	int pad = GetJoypadInputState(DX_INPUT_KEY_PAD1);
	if (pad & PAD_INPUT_1) m_fov += 0.01f;
	if (pad & PAD_INPUT_2) m_fov -= 0.01f;

	//視野角を設定する
	//カメラのズームインアウトは視野角の変更で行われることが多い
	SetupCamera_Perspective(m_fov);	
}

void SceneMain::Draw()
{
	//直線の始点と終点
	VECTOR startPos;
	VECTOR endPos;

	//横方向の線
	for (int z = -200; z <= 200; z += 100)
	{
		startPos = VGet(-200.0f, 0.0f, static_cast<float>(z));
		endPos = VGet(200.0f, 0.0f, static_cast<float>(z));
		DrawLine3D(startPos, endPos,0xff0000);
	}
	//奥行方向の線
	for (int x = -200; x <= 200; x += 100)
	{
		startPos = VGet(static_cast<float>(x), 0.0f, -200.0f);
		endPos = VGet(static_cast<float>(x), 0.0f, 200.0f);
		DrawLine3D(startPos, endPos, 0x0000ff);
	}

	//球を表示するテスト
	//DrawSphere3D(VGet(0.0f, 0.0f, 0.0f),120.0f, 8, 0xff0000, 0xff0000, false);
	//カプセルを表示するテスト
	//DrawCapsule3D(VGet(320.0f, 100.0f, 0.0f), VGet(320.0f, 300.0f, 0.0f) , 40.0f, 8, 0x00ff00, 0x00ff00, false);

	DrawString(0, 0, L"SceneMain", GetColor(255, 255, 255));
	DrawFormatString(0, 16, GetColor(255, 255, 255), L"FRAME:%d", m_frameCount);

	DrawFormatString(0, 32, 0xffffff, L"fov %f度", m_fov / DX_PI_F * 180.0f);
}

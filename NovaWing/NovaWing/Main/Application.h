#pragma once
#include <vector>
#include "../Utility/Size.h"
//#include "../SoundManager.h"

/// <summary>
/// アプリケーション全体を管理する
/// シングルトンクラス
/// </summary>
class Application
{
public:
	~Application();

	/// <summary>
	/// Applicationのシングルトンインスタンスを取得する
	/// </summary>
	/// <returns>Applicationのインスタンス</returns>
	/// <note>Applicationの実体はこの関数内部で生成され、ずっととどまり続ける</note>
	static Application& GetInstance();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <returns>初期化成功:true / 初期化失敗:false</returns>
	bool Init();

	/// <summary>
	/// アプリケーションをスタートする
	/// ゲームループを内包する
	/// </summary>
	void Run();

	/// <summary>
	/// アプリケーション終了処理
	/// </summary>
	void Terminate();

	/// <summary>
	/// ウィンドウサイズを取得する
	/// </summary>
	/// <returns></returns>
	const Size& GetWindowSize() const;

	//ゲーム終了をリクエストされるときに呼んでもらう
	void RequestExit() { m_isExitRequested = true; }

private:
	Size m_windowSize;//ウィンドウサイズ
private:
	Application();//newも変数宣言もできなくなる
	Application(const Application& app) = delete;//コピーコンストラクタを無効
	void operator=(const Application& app) = delete;//代入を禁止
	//終了リクエストをもらったか
	bool m_isExitRequested = false;

	//void SoundLoad(SoundManager& sm);
};
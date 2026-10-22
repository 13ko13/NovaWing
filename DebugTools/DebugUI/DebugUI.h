#pragma once

#ifdef _DEBUG

#include <functional>
#include <string>
#include <vector>

/// <summary>
/// Dear ImGuiを使ったデバッグ用ウィンドウを管理するシングルトンクラス(_DEBUGビルドのみ)
/// ImGuiの型はDebugUI.cppの中に閉じ込めて、ゲーム側のコードがimgui.hを知らなくて済むようにしている
/// 各シーンやオブジェクトは、自分のInitなどで項目を登録し、破棄されるときにRemoveOwnerで解除する
/// </summary>
class DebugUI
{
private:
	DebugUI();

public:
	/// <summary>
	/// DebugUIのインスタンスを取得する
	/// </summary>
	/// <returns></returns>
	static DebugUI& GetInstance();

	//コピーと代入を禁止する(消す)
	DebugUI(const DebugUI&) = delete;
	DebugUI& operator=(const DebugUI&) = delete;

	/// <summary>
	/// ImGuiを初期化する(DxLib_Initの後に呼ぶ)
	/// </summary>
	/// <returns>成功:true / 失敗:false</returns>
	bool Init();

	/// <summary>
	/// ImGuiを終了する(DxLib_Endの前に呼ぶ)
	/// </summary>
	void Terminate();

	/// <summary>
	/// フレームの先頭で呼ぶ。UIを組み立てて、ボタンなどに登録された処理を実行する
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// ScreenFlipの直前で呼ぶ。組み立てたUIをバックバッファへ描画する
	/// </summary>
	void Render();

	/// <summary>
	/// ImGuiがキーボード入力を使っている(文字入力中など)かどうか
	/// trueの間はゲーム側のキーボード入力を無効にする
	/// </summary>
	/// <returns>使っている:true / 使っていない:false</returns>
	bool WantCaptureKeyboard() const;

	/// <summary>
	/// ボタンを登録する
	/// </summary>
	/// <param name="owner">登録者(RemoveOwnerで解除するための目印。通常はthis)</param>
	/// <param name="section">表示するセクション名(同じ名前の項目は同じ見出しにまとまる)</param>
	/// <param name="label">ボタンの文字</param>
	/// <param name="onClick">押されたときの処理</param>
	void AddButton(
		const void* owner,
		const std::wstring& section,
		const std::wstring& label,
		std::function<void()> onClick);

	/// <summary>
	/// チェックボックスを登録する
	/// </summary>
	/// <param name="value">切り替える対象(登録者が生きている間は有効であること)</param>
	void AddCheckbox(
		const void* owner,
		const std::wstring& section,
		const std::wstring& label,
		bool* value);

	/// <summary>
	/// 毎フレーム文字列を取得して表示する項目を登録する(座標などの確認用)
	/// </summary>
	/// <param name="getText">表示する文字列を返す処理</param>
	void AddText(
		const void* owner,
		const std::wstring& section,
		std::function<std::wstring()> getText);

	/// <summary>
	/// スライダーとボタンのセットを登録する。ボタンを押したときにスライダーの値を渡して処理を呼ぶ
	/// </summary>
	/// <param name="label">スライダーの名前</param>
	/// <param name="min">スライダーの最小値</param>
	/// <param name="max">スライダーの最大値</param>
	/// <param name="buttonLabel">ボタンの文字</param>
	/// <param name="onExecute">ボタンが押されたときの処理(引数はスライダーの値)</param>
	void AddSliderAction(
		const void* owner,
		const std::wstring& section,
		const std::wstring& label,
		float min,
		float max,
		const std::wstring& buttonLabel,
		std::function<void(float)> onExecute);

	/// <summary>
	/// ownerが登録した項目をすべて解除する
	/// </summary>
	void RemoveOwner(const void* owner);

private:
	//登録された項目1つぶん
	struct Item
	{
		const void* owner;//登録者
		std::wstring section;//所属するセクション名
		std::function<void()> draw;//ImGuiで描画する処理(押された処理はm_pendingActionsへ積む)
	};

	/// <summary>
	/// 組み込みのSystemセクション(FPSやデバッグ表示の切り替え)を描画する
	/// </summary>
	void DrawSystemSection();

	/// <summary>
	/// 登録された項目をセクションごとにまとめて描画する
	/// </summary>
	void DrawItems();

private:
	//初期化済みか
	bool m_isInitialized = false;
	//ウィンドウを表示するか(F1キーで切り替え)
	bool m_isVisible = true;
	//前のフレームでF1キーが押されていたか
	bool m_wasToggleKeyDown = false;

	//登録された項目
	std::vector<Item> m_items;

	//UIを組み立てている間に押されたボタンの処理
	//組み立て中に実行すると、登録の解除などでm_itemsが壊れるので、組み立てが終わってから実行する
	std::vector<std::function<void()>> m_pendingActions;
};

#endif

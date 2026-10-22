#ifdef _DEBUG

#include "DebugUI.h"

#define NOMINMAX
#include <DxLib.h>
#include <d3d11.h>
#include <algorithm>
#include <memory>

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../../Manager/DebugManager.h"

//imgui_impl_win32.cppで定義されているメッセージ処理(ヘッダには宣言がないのでここで宣言する)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	//日本語を表示するためのフォント(Windows標準のメイリオ)
	const char* const font_path = "C:/Windows/Fonts/meiryo.ttc";
	constexpr float font_size = 18.0f;

	//ウィンドウ位置などを保存するファイル
	const char* const ini_file_path = "DebugTools/ImGui/imgui.ini";

	//ウィンドウの初期サイズ
	constexpr float window_width = 380.0f;
	constexpr float window_height = 520.0f;
	constexpr float window_margin = 10.0f;

	//ウィンドウの表示/非表示を切り替えるキー
	constexpr int toggle_key = KEY_INPUT_F1;

	//ImGuiは文字列をUTF-8で受け取るので、プロジェクトで使っているワイド文字列から変換する
	std::string ToUtf8(const std::wstring& wide)
	{
		if (wide.empty())
		{
			return std::string();
		}
		const int size = WideCharToMultiByte(
			CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
		std::string utf8(size, '\0');
		WideCharToMultiByte(
			CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), utf8.data(), size, nullptr, nullptr);
		return utf8;
	}

	//DxLibのウィンドウメッセージをImGuiにも渡すためのフック
	LRESULT CALLBACK HookWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		//戻り値は使わず、このあとDxLib側の処理も通常どおり行わせる
		SetUseHookWinProcReturnValue(FALSE);
		return 0;
	}
}

DebugUI::DebugUI()
{
}

DebugUI& DebugUI::GetInstance()
{
	//staticで宣言してそれを返す
	static DebugUI instance;
	return instance;
}

bool DebugUI::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = ini_file_path;
	//ゲームのウィンドウの外へ出したウィンドウを、OSの別ウィンドウとして表示できるようにする
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	//別ウィンドウにしたときにタスクバーへアイコンが増えないようにする
	io.ConfigViewportsNoTaskBarIcon = true;
	//日本語が表示できるフォントを読み込む(読み込めなければImGui標準のフォントのまま)
	io.Fonts->AddFontFromFileTTF(font_path, font_size, nullptr, io.Fonts->GetGlyphRangesJapanese());
	ImGui::StyleColorsDark();
	//別ウィンドウになったときに角が丸いと見た目がずれるので、角を丸めず背景も不透明にする
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;

	//DxLibが使っているウィンドウとDirect3D11のデバイスをImGuiに教える
	auto pDevice = static_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
	auto pContext = static_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));
	if (!ImGui_ImplWin32_Init(GetMainWindowHandle()))
	{
		return false;
	}
	if (!ImGui_ImplDX11_Init(pDevice, pContext))
	{
		return false;
	}
	SetHookWinProc(HookWinProc);

	//ImGuiを操作するためにマウスカーソルを表示する
	SetMouseDispFlag(TRUE);
	//別ウィンドウを操作してゲームのウィンドウが非アクティブになっても、処理を止めない
	//(止まるとImGuiの別ウィンドウも描画されず固まってしまう)
	SetAlwaysRunFlag(TRUE);

	m_isInitialized = true;
	return true;
}

void DebugUI::Terminate()
{
	if (!m_isInitialized)
	{
		return;
	}
	SetHookWinProc(nullptr);
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	m_isInitialized = false;
}

void DebugUI::BeginFrame()
{
	if (!m_isInitialized)
	{
		return;
	}

	//F1キーが押された瞬間だけ表示/非表示を切り替える
	const bool isToggleKeyDown = (CheckHitKey(toggle_key) != 0);
	if (isToggleKeyDown && !m_wasToggleKeyDown)
	{
		m_isVisible = !m_isVisible;
	}
	m_wasToggleKeyDown = isToggleKeyDown;

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (m_isVisible)
	{
		ImGui::SetNextWindowPos(ImVec2(window_margin, window_margin), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Debug"))
		{
			DrawSystemSection();
			DrawItems();
		}
		ImGui::End();
	}

	//UIの組み立てが終わってから、押されたボタンの処理を実行する
	//(処理の中でシーンが入れ替わって登録が解除されても、m_itemsを触っている途中にならない)
	std::vector<std::function<void()>> actions;
	actions.swap(m_pendingActions);
	for (const auto& action : actions)
	{
		action();
	}
}

void DebugUI::Render()
{
	if (!m_isInitialized)
	{
		return;
	}

	ImGui::Render();

	//DxLibが溜め込んでいる描画を先に出し切る
	RenderVertex();

	//ImGuiの描画先をバックバッファにする
	auto pContext = static_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));
	auto pBackBufferRTV = static_cast<ID3D11RenderTargetView*>(
		const_cast<void*>(GetUseDirect3D11BackBufferRenderTargetView()));
	pContext->OMSetRenderTargets(1, &pBackBufferRTV, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	//ゲームのウィンドウの外へ出したウィンドウ(別ウィンドウ)を更新して描画する
	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();

	//ImGuiが変えたDirect3Dの設定を、DxLibが期待する状態に戻す
	RefreshDxLibDirect3DSetting();
}

bool DebugUI::WantCaptureKeyboard() const
{
	if (!m_isInitialized)
	{
		return false;
	}
	return ImGui::GetIO().WantCaptureKeyboard;
}

void DebugUI::AddButton(
	const void* owner,
	const std::wstring& section,
	const std::wstring& label,
	std::function<void()> onClick)
{
	const std::string utf8Label = ToUtf8(label);
	m_items.push_back({ owner, section, [this, utf8Label, onClick]()
	{
		if (ImGui::Button(utf8Label.c_str()))
		{
			m_pendingActions.push_back(onClick);
		}
	} });
}

void DebugUI::AddCheckbox(
	const void* owner,
	const std::wstring& section,
	const std::wstring& label,
	bool* value)
{
	const std::string utf8Label = ToUtf8(label);
	m_items.push_back({ owner, section, [utf8Label, value]()
	{
		ImGui::Checkbox(utf8Label.c_str(), value);
	} });
}

void DebugUI::AddText(
	const void* owner,
	const std::wstring& section,
	std::function<std::wstring()> getText)
{
	m_items.push_back({ owner, section, [getText]()
	{
		ImGui::TextUnformatted(ToUtf8(getText()).c_str());
	} });
}

void DebugUI::AddSliderAction(
	const void* owner,
	const std::wstring& section,
	const std::wstring& label,
	float min,
	float max,
	const std::wstring& buttonLabel,
	std::function<void(float)> onExecute)
{
	const std::string utf8Label = ToUtf8(label);
	const std::string utf8ButtonLabel = ToUtf8(buttonLabel);
	//スライダーの値はこの項目が持つ(ラムダの中で共有する)
	auto value = std::make_shared<float>(min);
	m_items.push_back({ owner, section, [this, utf8Label, utf8ButtonLabel, min, max, value, onExecute]()
	{
		ImGui::SliderFloat(utf8Label.c_str(), value.get(), min, max);
		if (ImGui::Button(utf8ButtonLabel.c_str()))
		{
			const float executeValue = *value;
			m_pendingActions.push_back([onExecute, executeValue]() { onExecute(executeValue); });
		}
	} });
}

void DebugUI::RemoveOwner(const void* owner)
{
	m_items.erase(
		std::remove_if(m_items.begin(), m_items.end(),
			[owner](const Item& item) { return item.owner == owner; }),
		m_items.end());
}

void DebugUI::DrawSystemSection()
{
	if (!ImGui::CollapsingHeader("System", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	DebugManager& debugManager = DebugManager::GetInstance();
	bool isDebugDrawEnabled = debugManager.IsDebugDrawEnabled();
	if (ImGui::Checkbox(ToUtf8(L"デバッグ表示(当たり判定など)").c_str(), &isDebugDrawEnabled))
	{
		debugManager.SetDebugDrawEnabled(isDebugDrawEnabled);
	}
	bool isPlayerInvincible = debugManager.IsPlayerInvincible();
	if (ImGui::Checkbox(ToUtf8(L"プレイヤー無敵").c_str(), &isPlayerInvincible))
	{
		debugManager.SetPlayerInvincible(isPlayerInvincible);
	}
	ImGui::TextDisabled(ToUtf8(L"F1キーでこのウィンドウを表示/非表示").c_str());
}

void DebugUI::DrawItems()
{
	//セクションは、最初に登録された順に見出しを出して、同じセクションの項目をまとめて描画する
	std::vector<std::wstring> drawnSections;
	for (size_t i = 0; i < m_items.size(); ++i)
	{
		const std::wstring& section = m_items[i].section;
		if (std::find(drawnSections.begin(), drawnSections.end(), section) != drawnSections.end())
		{
			continue;
		}
		drawnSections.push_back(section);

		if (!ImGui::CollapsingHeader(ToUtf8(section).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			continue;
		}
		for (size_t j = i; j < m_items.size(); ++j)
		{
			if (m_items[j].section != section)
			{
				continue;
			}
			//同じ文字のボタンが複数あっても区別できるように、項目の位置をIDにする
			ImGui::PushID(static_cast<int>(j));
			m_items[j].draw();
			ImGui::PopID();
		}
	}
}

#endif

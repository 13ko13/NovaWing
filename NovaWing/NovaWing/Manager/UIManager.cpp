#include "UIManager.h"
#include "Game/UI/UIBase.h"

void UIManager::Register(std::shared_ptr<UIBase> pUI)
{
	//UIの配列にいれる
	m_pUIs.push_back(pUI);
}

void UIManager::Draw()
{
	//全てのUIの描画を行う
	for (std::shared_ptr<UIBase>& pUI : m_pUIs)
	{
		pUI->Draw();
	}
}
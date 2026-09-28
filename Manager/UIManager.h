#pragma once
#include <vector>
#include <memory>

class UIBase;
class UIManager
{
public:
	//生成されたUIを登録する
	void Register(std::shared_ptr<UIBase> pUI);

	//全てのUIの更新処理を呼ぶ
	void Update();
	//すべてのUIの描画処理を呼ぶ
	void Draw();

private:
	std::vector<std::shared_ptr<UIBase>> m_pUIs;
};


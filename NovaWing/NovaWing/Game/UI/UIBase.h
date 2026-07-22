#pragma once
class UIBase
{
public:
	virtual ~UIBase();

	//描画
	virtual void Draw() = 0;
	//更新
	virtual void Update() = 0;
};

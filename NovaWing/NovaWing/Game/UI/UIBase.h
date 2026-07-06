#pragma once
class UIBase
{
public:
	virtual ~UIBase();

	//描画
	virtual void Draw() = 0;
};

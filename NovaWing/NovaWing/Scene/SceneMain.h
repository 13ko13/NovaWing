#pragma once
class SceneMain
{
public:
	SceneMain();
	~SceneMain();

	void Init();
	void Update();
	void Draw();

private:
	int m_frameCount;

	//ƒJƒƒ‰‚Ì‹–ìŠp
	float m_fov;	//field of view(‹–ìŠp)
};
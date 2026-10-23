#pragma once
#include <string>

class StageConfig
{
public:
	//ステージ番号を受け取る
	StageConfig(int stageNo);
	~StageConfig();

	//ステージに使用するデータが入ったcsvがあるフォルダの名前を取得
	const std::wstring& GetCSVFolderName() const { return m_csvFolderName; }
	//チュートリアルがあるかを取得
	bool HasTutorial() const { return m_hasTutorial; }
	//ボスが出現するZ座標
	float GetBossAppearZ() const { return m_bossAppearZ; }
	//最後のステージかどうか
	bool IsLastStage() const;
	//ステージ番号を返す
	int GetStageNo() const { return m_stageNo; }

private:
	//ステージに使用するデータが入ったcsvのフォルダ名
	std::wstring m_csvFolderName;
	//チュートリアルがあるか
	bool m_hasTutorial = false;
	//ボスが出現するZ座標
	float m_bossAppearZ = 0.0f;
	//ステージの番号
	int m_stageNo = 0;
};

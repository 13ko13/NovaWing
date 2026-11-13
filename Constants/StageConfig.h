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
	//海ありのステージかを返す
	bool HasSea() const { return m_hasSea; }
	//ビルのループが始まるZ座標を取得
	float GetBuildingLoopStartZ() const { return m_buildingLoopStartZ; }
	//ボス登場時からのステージ1周の長さ
	float GetOneLapDistance() const { return m_oneLapDistance; }
	//雨のステージなのか
	bool IsRainStage() const { return m_isRainStage; }

private:
	//ステージに使用するデータが入ったcsvのフォルダ名
	std::wstring m_csvFolderName;
	//チュートリアルがあるか
	bool m_hasTutorial = false;
	//ボスが出現するZ座標
	float m_bossAppearZ = 0.0f;
	//ステージの番号
	int m_stageNo = 0;
	//海ありか
	bool m_hasSea = false;
	//ビルのループが開始するZ座標
	float m_buildingLoopStartZ = 0.0f;
	//ボス登場時からのステージ1周の長さ
	float m_oneLapDistance = 0.0f;
	//雨のステージか
	bool m_isRainStage = false;
};

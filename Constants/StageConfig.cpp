#include <cassert>
#include <iterator>

#include "StageConfig.h"

namespace
{
	//ステージ情報
	struct StageData
	{
		//ステージのcsvファイルが入ったフォルダの名前
		const wchar_t* csvFolderName;
		//チュートリアルがあるか
		bool hasTutorial;
		//ボス出現Z座標
		float bossAppearZ;
		//海があるか
		bool hasSea;
		//ビルのループが開始するZ座標
		float buildingLoopStartZ;
		//1周の長さ
		float oneLapDistance;
	};

	//ステージごとの設定(添字=ステージ番号(1面=0))
	constexpr StageData stage_datas[] =
	{
		{ L"Stage1",true,58700.0f ,true,0.0f,0.0f },//1面
		{ L"Stage2",false,56300.0f,false, 34000.0f,16000.0f },//2面
	};
}

StageConfig::StageConfig(int stageNo) :
	m_stageNo(stageNo)
{
	//ステージ番号をstage_datasに渡して対応するステージ情報を受け取る
	//範囲外の番号ならassert
	assert(
		stageNo >= 0 &&
		stageNo < static_cast<int>(std::size(stage_datas)) &&
		"存在しないステージ番号です"
	);
	const StageData& data = stage_datas[stageNo];
	m_csvFolderName = data.csvFolderName;
	m_hasTutorial = data.hasTutorial;
	m_bossAppearZ = data.bossAppearZ;
	m_hasSea = data.hasSea;
	m_buildingLoopStartZ = data.buildingLoopStartZ;
	m_oneLapDistance = data.oneLapDistance;
}

StageConfig::~StageConfig()
{
}

bool StageConfig::IsLastStage() const
{
	//現在のステージナンバーがstage_datasの最大要素番号と同じであれば最後のステージ
	return m_stageNo == std::size(stage_datas) - 1;
}

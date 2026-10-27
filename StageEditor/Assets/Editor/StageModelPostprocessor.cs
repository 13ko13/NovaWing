using UnityEditor;

// Assets/Models に入れたゲームのモデル(FBX/OBJ)の読み込み設定。
// ゲーム(DxLib)と同じく、ファイルの数値をそのままの大きさで読む(単位の変換をしない)。
// マテリアルは StageModelCatalog で作るので、モデルからは作らない。
//このコードはAIを使用しています。
public class StageModelPostprocessor : AssetPostprocessor
{
    void OnPreprocessModel()
    {
        if (!assetPath.StartsWith("Assets/Models/")) return;

        var importer = (ModelImporter)assetImporter;
        importer.useFileScale = false;
        importer.globalScale = 1f;
        importer.materialImportMode = ModelImporterMaterialImportMode.None;
        importer.importAnimation = false;
        importer.animationType = ModelImporterAnimationType.None;
        importer.importCameras = false;
        importer.importLights = false;
    }
}

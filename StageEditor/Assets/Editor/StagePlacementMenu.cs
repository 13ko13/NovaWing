using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

// ステージを作るためのメニュー。
// ・NovaWing/Create Stage2 Scene : 2面の配置用シーン(Assets/Scenes/Stage2.unity)を作る。
//   書き出し(Export Stage CSV)はシーン名のフォルダ(Data/CSV/Stage2)に出る。
// ・NovaWing/Add/... : シーンビューの真ん中に、ゲームのモデルの見た目付きでオブジェクトを置く。
//このコードはAIを使用しています。
public static class StagePlacementMenu
{
    // ビルを置いたときの大きさ(CSVのscaleになる)。ビルのモデルは幅100〜180程度なので、
    // プレイヤーの動ける範囲(幅約3200)に対して見合う大きさにしている。置いたあと自由に変えてよい
    const float DefaultBuildingScale = 10f;

    [MenuItem("NovaWing/Create Stage2 Scene (2面の配置用シーンを作る)")]
    static void CreateStage2Scene()
    {
        const string path = "Assets/Scenes/Stage2.unity";
        if (AssetDatabase.LoadAssetAtPath<SceneAsset>(path) != null)
        {
            if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
            EditorSceneManager.OpenScene(path);
            Debug.Log("[StagePlacement] Stage2 シーンは既にあるので開きました。");
            return;
        }
        if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;
        var scene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Single);
        EditorSceneManager.SaveScene(scene, path);
        Debug.Log("[StagePlacement] Stage2 シーンを作りました。NovaWing/Add からビルや敵を置いて、Export Stage CSV で Data/CSV/Stage2 に書き出せます。");
    }

    [MenuItem("NovaWing/Add/Building/AntennaTower")] static void AddAntennaTower() => AddBuilding("Building_AntennaTower");
    [MenuItem("NovaWing/Add/Building/BridgeWreck")] static void AddBridgeWreck() => AddBuilding("Building_BridgeWreck");
    [MenuItem("NovaWing/Add/Building/Collapsed")] static void AddCollapsed() => AddBuilding("Building_Collapsed");
    [MenuItem("NovaWing/Add/Building/Leaning")] static void AddLeaning() => AddBuilding("Building_Leaning");
    [MenuItem("NovaWing/Add/Building/OfficeGutted")] static void AddOfficeGutted() => AddBuilding("Building_OfficeGutted");
    [MenuItem("NovaWing/Add/Building/TallBroken")] static void AddTallBroken() => AddBuilding("Building_TallBroken");

    [MenuItem("NovaWing/Add/Floating Enemy")] static void AddFloating() => Add(PlacedObject.Kind.FloatingEnemy, "FloatingEnemy", 1f, false);
    [MenuItem("NovaWing/Add/Worm")] static void AddWorm() => Add(PlacedObject.Kind.WormEnemy, "WormHead", 1f, false);
    [MenuItem("NovaWing/Add/Rock/Rock1")] static void AddRock1() => Add(PlacedObject.Kind.Rock, "Rock1", 1f, true);
    [MenuItem("NovaWing/Add/Rock/Rock2")] static void AddRock2() => Add(PlacedObject.Kind.Rock, "Rock2", 1f, true);
    [MenuItem("NovaWing/Add/Rock/Rock3")] static void AddRock3() => Add(PlacedObject.Kind.Rock, "Rock3", 1f, true);
    [MenuItem("NovaWing/Add/Boss")] static void AddBoss() => Add(PlacedObject.Kind.Boss, "Boss", 1f, false);

    static void AddBuilding(string id) => Add(PlacedObject.Kind.Building, id, DefaultBuildingScale, true);

    // シーンビューの真ん中(注視点)に置く。地面に置くもの(ビル・岩)は高さ0にする
    static void Add(PlacedObject.Kind kind, string modelID, float scale, bool onGround)
    {
        Vector3 pos = SceneView.lastActiveSceneView != null ? SceneView.lastActiveSceneView.pivot : Vector3.zero;
        if (onGround) pos.y = 0f;

        var go = new GameObject($"{modelID}_{Object.FindObjectsOfType<PlacedObject>().Length + 1:00}");
        Undo.RegisterCreatedObjectUndo(go, "Add " + modelID);
        go.transform.position = pos;
        go.transform.localScale = Vector3.one * scale;

        var p = go.AddComponent<PlacedObject>();
        p.kind = kind;
        p.modelID = modelID;

        StageModelCatalog.RebuildVisual(p);
        Selection.activeGameObject = go;
    }
}

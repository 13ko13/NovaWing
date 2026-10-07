using System.Collections.Generic;
using System.Globalization;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

// Data/CSV/<シーン名>/ の新形式CSV(StageCsvExporterの出力と同じ形)を読んで、PlacedObject付きのオブジェクトとしてシーンに置く。
// CSVを直接書き換えたあと、Unityのシーンを合わせるために使う。座標はゲーム単位 ÷ UnityToGame(StageCsvExporterの逆)。
// 実行するたびに、シーン内の既存のPlacedObjectをすべて消して作り直す(Ctrl+Zで戻せる)。
//このコードはAIを使用しています。
public static class StageCsvImporter
{
    const float UnityToGame = 100f;
    const string RootName = "Imported";

    static readonly string CsvRoot = Path.GetFullPath(Path.Combine(Application.dataPath, "..", "..", "Data", "CSV"));

    [MenuItem("NovaWing/Import Stage CSV (CSVをシーンに置く)")]
    static void Import()
    {
        string stageName = SceneManager.GetActiveScene().name;
        string dir = Path.Combine(CsvRoot, stageName);
        if (!Directory.Exists(dir))
        {
            Debug.LogError($"[StageCsvImporter] フォルダが見つかりません: {dir}");
            return;
        }

        // 前回の取り込みや、手で置いた古い配置をすべて消す
        foreach (var old in Object.FindObjectsOfType<PlacedObject>())
        {
            if (old != null) Undo.DestroyObjectImmediate(old.gameObject);
        }
        var oldRoot = GameObject.Find(RootName);
        if (oldRoot != null) Undo.DestroyObjectImmediate(oldRoot);

        var root = new GameObject(RootName);
        Undo.RegisterCreatedObjectUndo(root, "Import Stage CSV");

        int count = 0;
        foreach (PlacedObject.Kind kind in System.Enum.GetValues(typeof(PlacedObject.Kind)))
        {
            foreach (var row in ReadRows(Path.Combine(dir, kind + ".csv")))
            {
                Create(root, kind, row);
                count++;
            }
        }

        Debug.Log($"[StageCsvImporter] {stageName}: {count}個を{RootName}の下に置きました。シーンを保存してください。");
    }

    // コマンドラインから「シーンを開く→取り込む→保存」を行う入口。
    // Unity.exe -batchmode -quit -projectPath StageEditor -executeMethod StageCsvImporter.ImportAndSaveBatch
    public static void ImportAndSaveBatch()
    {
        var scene = UnityEditor.SceneManagement.EditorSceneManager.OpenScene("Assets/Scenes/Stage1.unity");
        Import();
        UnityEditor.SceneManagement.EditorSceneManager.SaveScene(scene);
    }

    static void Create(GameObject root, PlacedObject.Kind kind, Dictionary<string, string> row)
    {
        string modelID = row["modelID"];
        var go = new GameObject($"{modelID}_{root.transform.childCount + 1:00}");
        go.transform.SetParent(root.transform);
        go.transform.position = new Vector3(F(row, "posX"), F(row, "posY"), F(row, "posZ")) / UnityToGame;
        go.transform.eulerAngles = new Vector3(F(row, "rotX"), F(row, "rotY"), F(row, "rotZ"));
        go.transform.localScale = new Vector3(F(row, "scaleX"), F(row, "scaleY"), F(row, "scaleZ"));

        var p = go.AddComponent<PlacedObject>();
        p.kind = kind;
        p.modelID = modelID;

        if (kind == PlacedObject.Kind.WormEnemy)
        {
            p.segmentCount = int.Parse(row["segmentCount"], CultureInfo.InvariantCulture);
            p.direction = int.Parse(row["direction"], CultureInfo.InvariantCulture);
            p.activatePlayerZ = F(row, "activatePlayerZ");
        }

        // 見た目用の仮の形は、親の大きさに影響されないよう子として置く(LegacyCsvImporterと同じ)
        var (shape, size) = VisualOf(kind);
        var visual = GameObject.CreatePrimitive(shape);
        visual.name = "Visual";
        Object.DestroyImmediate(visual.GetComponent<Collider>());
        visual.transform.SetParent(go.transform, false);
        visual.transform.localScale = Vector3.one * size;
    }

    static (PrimitiveType, float) VisualOf(PlacedObject.Kind kind)
    {
        switch (kind)
        {
            case PlacedObject.Kind.FloatingEnemy: return (PrimitiveType.Sphere, 2.6f);
            case PlacedObject.Kind.WormEnemy: return (PrimitiveType.Capsule, 2f);
            case PlacedObject.Kind.Boss: return (PrimitiveType.Cube, 20f);
            default: return (PrimitiveType.Cube, 3f);
        }
    }

    static float F(Dictionary<string, string> row, string key) => float.Parse(row[key], CultureInfo.InvariantCulture);

    // ヘッダー行の名前で引ける形にして返す。ファイルが無い種類は何も返さない(敵だけのステージなどがあるため)
    static IEnumerable<Dictionary<string, string>> ReadRows(string path)
    {
        if (!File.Exists(path)) yield break;

        var lines = File.ReadAllLines(path);
        string[] header = lines[0].Trim().Split(',');
        for (int i = 1; i < lines.Length; i++)
        {
            if (string.IsNullOrWhiteSpace(lines[i])) continue;
            string[] cols = lines[i].Trim().Split(',');
            var row = new Dictionary<string, string>();
            for (int c = 0; c < header.Length; c++) row[header[c]] = cols[c];
            yield return row;
        }
    }
}

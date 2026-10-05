using System.Globalization;
using System.IO;
using UnityEditor;
using UnityEngine;

// 旧形式のCSV(Data/CSV直下の4ファイル)を読んで、PlacedObject付きのオブジェクトとしてシーンに置く。
// 移行用に1回使うためのもの。座標はゲーム単位 ÷ UnityToGame でUnity単位に戻す(StageCsvExporterの逆)。
// 実行するたびに前回のぶん(Legacyオブジェクト)を消して作り直す。
//このコードはAIを使用しています。
public static class LegacyCsvImporter
{
    const float UnityToGame = 100f;
    const string RootName = "Legacy";

    static readonly string CsvRoot = Path.GetFullPath(Path.Combine(Application.dataPath, "..", "..", "Data", "CSV"));

    [MenuItem("NovaWing/Import Legacy CSV (旧配置をシーンに置く)")]
    static void Import()
    {
        var old = GameObject.Find(RootName);
        if (old != null) Undo.DestroyObjectImmediate(old);

        var root = new GameObject(RootName);
        Undo.RegisterCreatedObjectUndo(root, "Import Legacy CSV");

        int count = 0;
        // FloatingEnemyData: modelID,x,y,z,hp
        foreach (var c in ReadRows("FloatingEnemyData.csv"))
        {
            var p = Create(root, PlacedObject.Kind.FloatingEnemy, c[0], c[1], c[2], c[3], PrimitiveType.Sphere, 2.6f);
            count++;
        }
        // WormEnemyData: modelID,x,y,z,segmentCount,direction,activatePlayerZ
        foreach (var c in ReadRows("WormEnemyData.csv"))
        {
            var p = Create(root, PlacedObject.Kind.WormEnemy, c[0], c[1], c[2], c[3], PrimitiveType.Capsule, 2f);
            p.segmentCount = int.Parse(c[4], CultureInfo.InvariantCulture);
            p.direction = int.Parse(c[5], CultureInfo.InvariantCulture);
            p.activatePlayerZ = Parse(c[6]);
            count++;
        }
        // BossEnemyData: modelID,x,y,z,hp
        foreach (var c in ReadRows("BossEnemyData.csv"))
        {
            Create(root, PlacedObject.Kind.Boss, c[0], c[1], c[2], c[3], PrimitiveType.Cube, 20f);
            count++;
        }
        // RockData: modelID,modelX,modelY,modelZ,sphere...(球はPlacedObjectに持たない)
        foreach (var c in ReadRows("RockData.csv"))
        {
            Create(root, PlacedObject.Kind.Rock, c[0], c[1], c[2], c[3], PrimitiveType.Cube, 3f);
            count++;
        }

        Debug.Log($"[LegacyCsvImporter] {count}個を{RootName}の下に置きました。");
    }

    // 親(大きさ1)にPlacedObjectを付け、見た目用の仮の形を子として置く
    static PlacedObject Create(GameObject root, PlacedObject.Kind kind, string modelID,
        string x, string y, string z, PrimitiveType shape, float visualSize)
    {
        var go = new GameObject($"{modelID}_{root.transform.childCount + 1:00}");
        go.transform.SetParent(root.transform);
        go.transform.position = new Vector3(Parse(x), Parse(y), Parse(z)) / UnityToGame;

        var p = go.AddComponent<PlacedObject>();
        p.kind = kind;
        p.modelID = modelID;

        var visual = GameObject.CreatePrimitive(shape);
        visual.name = "Visual";
        Object.DestroyImmediate(visual.GetComponent<Collider>());
        visual.transform.SetParent(go.transform, false);
        visual.transform.localScale = Vector3.one * visualSize;
        return p;
    }

    static float Parse(string s) => float.Parse(s, CultureInfo.InvariantCulture);

    // ヘッダー行を飛ばして、空でない行をカンマで分解して返す
    static System.Collections.Generic.IEnumerable<string[]> ReadRows(string fileName)
    {
        string path = Path.Combine(CsvRoot, fileName);
        if (!File.Exists(path))
        {
            Debug.LogWarning($"[LegacyCsvImporter] ファイルが見つかりません: {path}");
            yield break;
        }
        var lines = File.ReadAllLines(path);
        for (int i = 1; i < lines.Length; i++)
        {
            if (string.IsNullOrWhiteSpace(lines[i])) continue;
            yield return lines[i].Trim().Split(',');
        }
    }
}

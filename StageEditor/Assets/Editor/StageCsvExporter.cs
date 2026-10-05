using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

// 開いているシーンの PlacedObject を集めて、Data/CSV/<シーン名>/<種類>.csv に書き出す。
// 座標はゲーム単位(Unity座標 × UnityToGame)に変換して書く。変換はここだけで行い、ゲーム側は変換しない。
// 回転(度)・大きさ(倍率)は単位が同じなのでそのまま書く。
//このコードはAIを使用しています。
public static class StageCsvExporter
{
    // このファイルは StageEditor/Assets/Editor にあるので、リポジトリ直下はdataPathの2つ上
    static readonly string CsvRoot = Path.GetFullPath(Path.Combine(Application.dataPath, "..", "..", "Data", "CSV"));

    // Unityの1単位 = ゲームの何単位か(Unityで80の位置 = ゲームのZ 8000)
    const float UnityToGame = 100f;

    const string CommonHeader = "modelID,posX,posY,posZ,rotX,rotY,rotZ,scaleX,scaleY,scaleZ";

    [MenuItem("NovaWing/Export Stage CSV")]
    static void Export()
    {
        string stageName = SceneManager.GetActiveScene().name;
        string outDir = Path.Combine(CsvRoot, stageName);
        Directory.CreateDirectory(outDir);

        var placed = Object.FindObjectsOfType<PlacedObject>();
        // 並びを毎回同じにする(CSVの差分を見やすくするため)
        var sorted = placed.OrderBy(p => p.transform.position.z).ThenBy(p => p.transform.position.x);

        int fileCount = 0;
        foreach (PlacedObject.Kind kind in System.Enum.GetValues(typeof(PlacedObject.Kind)))
        {
            var items = sorted.Where(p => p.kind == kind).ToList();
            if (items.Count == 0) continue;

            var sb = new StringBuilder();
            sb.Append(CommonHeader);
            if (kind == PlacedObject.Kind.WormEnemy) sb.Append(",segmentCount,direction,activatePlayerZ");
            sb.Append('\n');

            foreach (var p in items)
            {
                sb.Append(MakeRow(p, kind));
                sb.Append('\n');
            }

            // BOMなしUTF-8(ゲーム側の読み込みはUTF-8前提)
            File.WriteAllText(Path.Combine(outDir, kind + ".csv"), sb.ToString(), new UTF8Encoding(false));
            fileCount++;
        }

        Debug.Log($"[StageCsvExporter] {stageName}: {placed.Length}個を{fileCount}ファイルに書き出しました → {outDir}");
    }

    static string MakeRow(PlacedObject p, PlacedObject.Kind kind)
    {
        Transform t = p.transform;
        Vector3 pos = t.position * UnityToGame;
        Vector3 rot = t.eulerAngles;
        Vector3 scale = t.lossyScale;

        var cols = new List<string>
        {
            p.modelID,
            F(pos.x), F(pos.y), F(pos.z),
            F(rot.x), F(rot.y), F(rot.z),
            F(scale.x), F(scale.y), F(scale.z),
        };

        if (kind == PlacedObject.Kind.WormEnemy)
        {
            cols.Add(p.segmentCount.ToString(CultureInfo.InvariantCulture));
            cols.Add(p.direction.ToString(CultureInfo.InvariantCulture));
            cols.Add(F(p.activatePlayerZ));
        }
        return string.Join(",", cols);
    }

    // 小数点は必ずピリオド。誤差の細かい桁は丸める。
    static string F(float v) => System.Math.Round(v, 4).ToString("0.####", CultureInfo.InvariantCulture);
}

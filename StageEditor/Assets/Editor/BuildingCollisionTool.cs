using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

// ビルの当たり判定用メッシュ(ProBuilderで作る)を、ゲームが読めるFBXとして書き出すための道具。
// 1. メニュー「NovaWing/Building Collision/Edit ...」でビルを原点に置いた作業用シーンを開く
// 2. 「Collision」の子として、ProBuilderの箱などでビルの形をなぞる
// 3. メニュー「NovaWing/Building Collision/Export」で Data/Model/Building/<ビル名>_Col.x に書き出す
//    (「Auto Generate All」なら、ビルの形を輪切りにした箱を自動で作って全部書き出す)
//
// 大きさ: ビルは他のシーンと同じく1/100(Unityの1 = ゲームの100)で置き、書き出すときに100倍して
// ビルのモデルのファイルの数値と同じ単位にする。ゲーム(DxLib)はFBX Exporterの出すFBXを読めないので .x で書く。
//このコードはAIを使用しています。
public static class BuildingCollisionTool
{
    const string CollisionRootName = "Collision";
    const string ScenePath = "Assets/Scenes/BuildingCollision.unity";
    static readonly string OutputDir = Path.GetFullPath(Path.Combine(Application.dataPath, "..", "..", "Data", "Model", "Building"));

    [MenuItem("NovaWing/Building Collision/Edit AntennaTower")] static void EditAntennaTower() => Edit("Building_AntennaTower");
    [MenuItem("NovaWing/Building Collision/Edit BridgeWreck")] static void EditBridgeWreck() => Edit("Building_BridgeWreck");
    [MenuItem("NovaWing/Building Collision/Edit Collapsed")] static void EditCollapsed() => Edit("Building_Collapsed");
    [MenuItem("NovaWing/Building Collision/Edit Leaning")] static void EditLeaning() => Edit("Building_Leaning");
    [MenuItem("NovaWing/Building Collision/Edit OfficeGutted")] static void EditOfficeGutted() => Edit("Building_OfficeGutted");
    [MenuItem("NovaWing/Building Collision/Edit TallBroken")] static void EditTallBroken() => Edit("Building_TallBroken");

    // ビルを原点に置いた作業用シーンを作る(前の作業内容は保存を確認してから閉じる)
    static void Edit(string buildingID)
    {
        if (!EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) return;

        var scene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Single);

        // ビルの見た目(Stageのシーンと同じ作り方・大きさ)
        var building = StageModelCatalog.CreateVisual(buildingID, PlacedObject.Kind.Building);
        building.name = buildingID;

        // 当たり判定の箱を入れる入れ物(原点・回転なし・大きさ1のまま使う)
        var root = new GameObject(CollisionRootName);
        root.AddComponent<BuildingCollisionMarker>().buildingID = buildingID;

        // 既に書き出したものがあれば、続きから直せるように読み込む
        string existing = "Assets/Models/" + buildingID + "_Col.fbx";
        var prev = AssetDatabase.LoadAssetAtPath<GameObject>(existing);
        if (prev != null)
        {
            var copy = (GameObject)Object.Instantiate(prev);
            copy.transform.SetParent(root.transform, false);
            // Assets/Models のモデルはファイルの数値のまま読む設定なので、1/100にしてビルと合わせる
            copy.transform.localScale = Vector3.one / 100f;
            copy.name = "前回の当たり判定(作り直すなら消してよい)";
        }

        EditorSceneManager.SaveScene(scene, ScenePath);
        Selection.activeGameObject = root;
        SceneView.lastActiveSceneView?.FrameSelected();
        Debug.Log($"[BuildingCollision] {buildingID} を原点に置きました。「{CollisionRootName}」の子に ProBuilder の形を作って、NovaWing/Building Collision/Export で書き出してください。");
    }

    static readonly string[] BuildingIDs =
    {
        "Building_AntennaTower", "Building_BridgeWreck", "Building_Collapsed",
        "Building_Leaning", "Building_OfficeGutted", "Building_TallBroken",
    };

    // 自動で作る箱の細かさ(ファイルの数値。ビルは大きさ6〜10で置くので、ゲームでは60〜100)。
    // ビルの表面をこの大きさの立方体(ボクセル)で拾うので、小さいほど見た目に近くなり、箱の数は増える
    const float AutoVoxelSize = 8f;

    // 全部のビルについて、形から当たり判定の箱を自動で作って書き出す(作業用シーンは使わない)
    [MenuItem("NovaWing/Building Collision/Auto Generate All (形から自動で作る)")]
    static void AutoGenerateAll()
    {
        foreach (var id in BuildingIDs)
        {
            var root = new GameObject(CollisionRootName);
            int boxes = BuildAutoBoxes(id, root.transform);
            if (boxes > 0)
            {
                ExportRoot(root, id);
                Debug.Log($"[BuildingCollision] {id}: 箱{boxes}個で書き出しました");
            }
            Object.DestroyImmediate(root);
        }
        AssetDatabase.Refresh();
    }

    // バッチ用の入口
    public static void AutoGenerateAllBatch() => AutoGenerateAll();

    // ビルの形(ファイルの数値)から、見た目に沿った箱を root の子に作る。箱の大きさは1/100。
    // 1. 三角形の表面を細かく拾って、表面が通るボクセルに印を付ける
    // 2. 高さの段ごとに、外側から届かないボクセル(壁に囲まれた中身)も埋める
    // 3. 段ごとに、埋まったボクセルを長方形にまとめ、上下の段で同じ長方形はつなげて1つの箱にする
    // 以前は段ごとに全部を囲む箱1つにしていたため、L字・飛び出し・傾きの分だけ見た目より大きかった
    static int BuildAutoBoxes(string buildingID, Transform root)
    {
        var model = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Models/" + buildingID + ".fbx");
        if (model == null) { Debug.LogError($"[BuildingCollision] モデルがありません: {buildingID}"); return 0; }

        // ファイルの数値のままの三角形(原点・大きさ1で置いたときのワールド座標)を集める
        var inst = (GameObject)Object.Instantiate(model);
        var tris = new System.Collections.Generic.List<Vector3>();
        foreach (var mf in inst.GetComponentsInChildren<MeshFilter>())
        {
            var mv = mf.sharedMesh.vertices;
            foreach (int t in mf.sharedMesh.triangles) tris.Add(mf.transform.TransformPoint(mv[t]));
        }
        Object.DestroyImmediate(inst);
        if (tris.Count == 0) return 0;

        Vector3 min = tris[0], max = tris[0];
        foreach (var p in tris) { min = Vector3.Min(min, p); max = Vector3.Max(max, p); }
        float v = AutoVoxelSize;
        // 外側を1マスずつ空けておく(外から塗るときの出発点にする)
        int nx = Mathf.CeilToInt((max.x - min.x) / v) + 2;
        int ny = Mathf.CeilToInt((max.y - min.y) / v);
        int nz = Mathf.CeilToInt((max.z - min.z) / v) + 2;
        ny = Mathf.Max(ny, 1);
        var solid = new bool[nx, ny, nz];

        // 1. 表面を拾う(三角形の上を、ボクセルの半分より細かい間隔でなぞる)
        for (int i = 0; i + 2 < tris.Count; i += 3)
        {
            Vector3 a = tris[i], b = tris[i + 1], c = tris[i + 2];
            float longest = Mathf.Max((b - a).magnitude, (c - a).magnitude, (c - b).magnitude);
            int n = Mathf.Max(1, Mathf.CeilToInt(longest / (v * 0.5f)));
            for (int s = 0; s <= n; s++)
            {
                for (int t = 0; t <= n - s; t++)
                {
                    Vector3 p = a + (b - a) * (s / (float)n) + (c - a) * (t / (float)n);
                    int ix = Mathf.Clamp(Mathf.FloorToInt((p.x - min.x) / v) + 1, 1, nx - 2);
                    int iy = Mathf.Clamp(Mathf.FloorToInt((p.y - min.y) / v), 0, ny - 1);
                    int iz = Mathf.Clamp(Mathf.FloorToInt((p.z - min.z) / v) + 1, 1, nz - 2);
                    solid[ix, iy, iz] = true;
                }
            }
        }

        // 2. 段ごとに、外側から空いているマスをたどり、たどれなかった所(中身)を埋める
        var outside = new bool[nx, nz];
        var queue = new System.Collections.Generic.Queue<Vector2Int>();
        for (int y = 0; y < ny; y++)
        {
            System.Array.Clear(outside, 0, outside.Length);
            queue.Clear();
            for (int x = 0; x < nx; x++) { queue.Enqueue(new Vector2Int(x, 0)); queue.Enqueue(new Vector2Int(x, nz - 1)); }
            for (int z = 0; z < nz; z++) { queue.Enqueue(new Vector2Int(0, z)); queue.Enqueue(new Vector2Int(nx - 1, z)); }
            while (queue.Count > 0)
            {
                var q = queue.Dequeue();
                if (q.x < 0 || q.y < 0 || q.x >= nx || q.y >= nz) continue;
                if (outside[q.x, q.y] || solid[q.x, y, q.y]) continue;
                outside[q.x, q.y] = true;
                queue.Enqueue(new Vector2Int(q.x + 1, q.y)); queue.Enqueue(new Vector2Int(q.x - 1, q.y));
                queue.Enqueue(new Vector2Int(q.x, q.y + 1)); queue.Enqueue(new Vector2Int(q.x, q.y - 1));
            }
            for (int x = 0; x < nx; x++)
                for (int z = 0; z < nz; z++)
                    if (!outside[x, z]) solid[x, y, z] = true;
        }

        // 3. 段ごとに長方形にまとめ、上下で同じ長方形をつなげる(キー: x0,z0,x1,z1 → 始まりの段)
        var open = new System.Collections.Generic.Dictionary<(int, int, int, int), int>();
        var boxes = new System.Collections.Generic.List<Bounds>();
        System.Action<(int x0, int z0, int x1, int z1), int, int> addBox = (r, y0, y1) =>
        {
            var bmin = new Vector3(min.x + (r.x0 - 1) * v, min.y + y0 * v, min.z + (r.z0 - 1) * v);
            var bmax = new Vector3(min.x + r.x1 * v, min.y + y1 * v, min.z + r.z1 * v);
            var bb = new Bounds(); bb.SetMinMax(bmin, bmax);
            boxes.Add(bb);
        };
        var used = new bool[nx, nz];
        for (int y = 0; y < ny; y++)
        {
            System.Array.Clear(used, 0, used.Length);
            var rects = new System.Collections.Generic.HashSet<(int, int, int, int)>();
            for (int z = 0; z < nz; z++)
            {
                for (int x = 0; x < nx; x++)
                {
                    if (!solid[x, y, z] || used[x, z]) continue;
                    int x1 = x;
                    while (x1 + 1 < nx && solid[x1 + 1, y, z] && !used[x1 + 1, z]) x1++;
                    int z1 = z;
                    while (z1 + 1 < nz)
                    {
                        bool row = true;
                        for (int k = x; k <= x1; k++) if (!solid[k, y, z1 + 1] || used[k, z1 + 1]) { row = false; break; }
                        if (!row) break;
                        z1++;
                    }
                    for (int k = x; k <= x1; k++) for (int m = z; m <= z1; m++) used[k, m] = true;
                    rects.Add((x, z, x1 + 1, z1 + 1));
                }
            }
            // 前の段から続かなかった長方形は、そこで箱として閉じる
            var closing = new System.Collections.Generic.List<(int, int, int, int)>();
            foreach (var kv in open) if (!rects.Contains(kv.Key)) closing.Add(kv.Key);
            foreach (var key in closing) { addBox(key, open[key], y); open.Remove(key); }
            foreach (var r in rects) if (!open.ContainsKey(r)) open[r] = y;
        }
        foreach (var kv in open) addBox(kv.Key, kv.Value, ny);

        // 箱を作る(1/100)
        int index = 0;
        foreach (var b in boxes)
        {
            var box = GameObject.CreatePrimitive(PrimitiveType.Cube);
            box.name = "Box" + (++index);
            Object.DestroyImmediate(box.GetComponent<Collider>());
            box.transform.SetParent(root, false);
            box.transform.localPosition = b.center / 100f;
            box.transform.localScale = b.size / 100f;
        }
        return boxes.Count;
    }

    // root を原点に戻して、ゲーム用の .x(Data/Model/Building)と、Unityで続きを直す用の FBX(Assets/Models)を書き出す。
    // DxLib(ゲーム)はFBX Exporterの出すFBXを読めないので、ゲームには .x を使う。
    static void ExportRoot(GameObject root, string buildingID)
    {
        root.transform.SetPositionAndRotation(Vector3.zero, Quaternion.identity);
        root.transform.localScale = Vector3.one;

        Directory.CreateDirectory(OutputDir);
        WriteXFile(root, Path.Combine(OutputDir, buildingID + "_Col.x"));

        string unityCopy = Path.Combine(Application.dataPath, "Models", buildingID + "_Col.fbx");
        UnityEditor.Formats.Fbx.Exporter.ModelExporter.ExportObject(unityCopy, root);
    }

    // root の子のメッシュを全部まとめて、DirectXの .x(テキスト)に書く。
    // 座標は「Unityの位置 × 100」= ビルのモデルのファイルの数値と同じ単位(ビルは1/100で置いているため)。
    // .x はUnity・DxLibと同じ左手系なので、向きや三角形の順番はそのまま。
    static void WriteXFile(GameObject root, string path)
    {
        var verts = new System.Collections.Generic.List<Vector3>();
        var normals = new System.Collections.Generic.List<Vector3>();
        var tris = new System.Collections.Generic.List<int>();
        foreach (var mf in root.GetComponentsInChildren<MeshFilter>())
        {
            var mesh = mf.sharedMesh;
            if (mesh == null) continue;
            int baseIndex = verts.Count;
            var mv = mesh.vertices;
            var mn = mesh.normals;
            for (int i = 0; i < mv.Length; i++)
            {
                verts.Add(mf.transform.TransformPoint(mv[i]) * 100f);
                normals.Add(mn.Length == mv.Length ? mf.transform.TransformDirection(mn[i]).normalized : Vector3.up);
            }
            foreach (int t in mesh.triangles) tris.Add(baseIndex + t);
        }

        var ci = System.Globalization.CultureInfo.InvariantCulture;
        var sb = new System.Text.StringBuilder();
        sb.AppendLine("xof 0302txt 0064");
        sb.AppendLine("Mesh Collision {");
        sb.AppendLine($" {verts.Count};");
        for (int i = 0; i < verts.Count; i++)
            sb.AppendLine(string.Format(ci, " {0:0.####};{1:0.####};{2:0.####};{3}", verts[i].x, verts[i].y, verts[i].z, i == verts.Count - 1 ? ";" : ","));
        int triCount = tris.Count / 3;
        sb.AppendLine($" {triCount};");
        for (int i = 0; i < triCount; i++)
            sb.AppendLine($" 3;{tris[i * 3]},{tris[i * 3 + 1]},{tris[i * 3 + 2]};{(i == triCount - 1 ? ";" : ",")}");
        sb.AppendLine(" MeshNormals {");
        sb.AppendLine($"  {normals.Count};");
        for (int i = 0; i < normals.Count; i++)
            sb.AppendLine(string.Format(ci, "  {0:0.####};{1:0.####};{2:0.####};{3}", normals[i].x, normals[i].y, normals[i].z, i == normals.Count - 1 ? ";" : ","));
        sb.AppendLine($"  {triCount};");
        for (int i = 0; i < triCount; i++)
            sb.AppendLine($"  3;{tris[i * 3]},{tris[i * 3 + 1]},{tris[i * 3 + 2]};{(i == triCount - 1 ? ";" : ",")}");
        sb.AppendLine(" }");
        sb.AppendLine(" MeshMaterialList {");
        sb.AppendLine("  1;");
        sb.AppendLine($"  {triCount};");
        for (int i = 0; i < triCount; i++) sb.AppendLine($"  0{(i == triCount - 1 ? ";" : ",")}");
        sb.AppendLine("  Material {");
        sb.AppendLine("   1.000000;1.000000;1.000000;1.000000;;");
        sb.AppendLine("   0.000000;");
        sb.AppendLine("   0.000000;0.000000;0.000000;;");
        sb.AppendLine("   0.000000;0.000000;0.000000;;");
        sb.AppendLine("  }");
        sb.AppendLine(" }");
        sb.AppendLine("}");
        File.WriteAllText(path, sb.ToString(), new System.Text.UTF8Encoding(false));
    }

    [MenuItem("NovaWing/Building Collision/Export")]
    static void Export()
    {
        var marker = Object.FindObjectOfType<BuildingCollisionMarker>();
        if (marker == null)
        {
            Debug.LogError("[BuildingCollision] 作業用シーンではありません。先に NovaWing/Building Collision/Edit ... を開いてください。");
            return;
        }
        var root = marker.gameObject;
        if (root.transform.childCount == 0)
        {
            Debug.LogError("[BuildingCollision] 「Collision」の子に当たり判定の形がありません。");
            return;
        }
        // 入れ物は原点・回転なし・大きさ1でないと、ゲームでビルとずれる(ExportRootで戻す)。
        // 次に開いたとき続きから直せるよう、Unity側にも同じものを置いておく
        ExportRoot(root, marker.buildingID);
        AssetDatabase.Refresh();

        EditorSceneManager.SaveScene(root.scene);
        Debug.Log($"[BuildingCollision] 書き出しました: {Path.Combine(OutputDir, marker.buildingID + "_Col.x")}");
    }
}

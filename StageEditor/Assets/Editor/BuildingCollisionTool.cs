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

    // 自動で作る箱: ビルを高さ方向に何段に切るか / 隣の段と大きさの差がこの割合以内なら1つの箱にまとめる
    const int AutoSliceCount = 10;
    const float AutoMergeTolerance = 0.15f;

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

    // ビルの頂点(ファイルの数値)を高さ方向に輪切りにして、段ごとに収まる箱を root の子に作る。箱の大きさは1/100
    static int BuildAutoBoxes(string buildingID, Transform root)
    {
        var model = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Models/" + buildingID + ".fbx");
        if (model == null) { Debug.LogError($"[BuildingCollision] モデルがありません: {buildingID}"); return 0; }

        // ファイルの数値のままの頂点(原点・大きさ1で置いたときのワールド座標)を集める
        var inst = (GameObject)Object.Instantiate(model);
        var points = new System.Collections.Generic.List<Vector3>();
        foreach (var mf in inst.GetComponentsInChildren<MeshFilter>())
        {
            foreach (var v in mf.sharedMesh.vertices) points.Add(mf.transform.TransformPoint(v));
        }
        Object.DestroyImmediate(inst);
        if (points.Count == 0) return 0;

        float minY = float.MaxValue, maxY = float.MinValue;
        foreach (var p in points) { minY = Mathf.Min(minY, p.y); maxY = Mathf.Max(maxY, p.y); }
        float step = (maxY - minY) / AutoSliceCount;

        // 段ごとの箱(XZの範囲)
        var slices = new System.Collections.Generic.List<Bounds>();
        for (int i = 0; i < AutoSliceCount; i++)
        {
            float y0 = minY + step * i, y1 = y0 + step;
            bool found = false;
            Bounds b = new Bounds();
            foreach (var p in points)
            {
                if (p.y < y0 || p.y > y1) continue;
                if (!found) { b = new Bounds(new Vector3(p.x, (y0 + y1) * 0.5f, p.z), Vector3.zero); found = true; }
                else b.Encapsulate(new Vector3(p.x, (y0 + y1) * 0.5f, p.z));
            }
            if (!found) continue;
            // 高さは段の高さそのもの
            b.center = new Vector3(b.center.x, (y0 + y1) * 0.5f, b.center.z);
            b.size = new Vector3(b.size.x, step, b.size.z);
            slices.Add(b);
        }

        // 上下の段の箱がほぼ同じなら1つにまとめる
        var merged = new System.Collections.Generic.List<Bounds>();
        foreach (var s in slices)
        {
            if (merged.Count > 0)
            {
                var last = merged[merged.Count - 1];
                float tol = AutoMergeTolerance * Mathf.Max(last.size.x, last.size.z, 1f);
                bool touching = Mathf.Abs(last.max.y - s.min.y) < step * 0.01f + 0.001f;
                if (touching &&
                    Mathf.Abs(last.min.x - s.min.x) < tol && Mathf.Abs(last.max.x - s.max.x) < tol &&
                    Mathf.Abs(last.min.z - s.min.z) < tol && Mathf.Abs(last.max.z - s.max.z) < tol)
                {
                    // XZは両方を含む範囲、Yはつなげる
                    var min = Vector3.Min(last.min, s.min);
                    var max = Vector3.Max(last.max, s.max);
                    var m = new Bounds(); m.SetMinMax(min, max);
                    merged[merged.Count - 1] = m;
                    continue;
                }
            }
            merged.Add(s);
        }

        // 箱を作る(1/100)
        int index = 0;
        foreach (var b in merged)
        {
            var box = GameObject.CreatePrimitive(PrimitiveType.Cube);
            box.name = "Box" + (++index);
            Object.DestroyImmediate(box.GetComponent<Collider>());
            box.transform.SetParent(root, false);
            box.transform.localPosition = b.center / 100f;
            box.transform.localScale = b.size / 100f;
        }
        return merged.Count;
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

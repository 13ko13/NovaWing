using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;

// 配置したオブジェクト(PlacedObject)の見た目に、ゲームと同じモデルを使うための一覧。
// modelID(CSVの1列目)から、モデル・テクスチャ・大きさを引く。
// 大きさ = ゲーム側の model_scale ÷ 100(Unityの1単位 = ゲームの100単位)。
// 一覧にないmodelIDは、種類ごとの色を付けた仮の形にする。
//このコードはAIを使用しています。
public static class StageModelCatalog
{
    const string ModelDir = "Assets/Models/";
    const string MaterialDir = "Assets/Models/Materials/";
    const float UnityToGame = 100f;

    struct Entry
    {
        public string model;   // Assets/Models/ からのファイル名
        public string texture; // Assets/Models/ からのファイル名
        public Vector3 scale;  // ゲーム側の model_scale
        public Entry(string model, string texture, Vector3 scale) { this.model = model; this.texture = texture; this.scale = scale; }
    }

    // ワームの頭はゲームの大きさのままだと小さすぎて見えにくいので、Unityの表示だけ大きくする倍率。
    // 見た目だけの話で、書き出すCSVやゲームには関係しない(枠・螺旋・胴体の球はゲームと同じ大きさのまま)
    const float WormDisplayScale = 6f;

    // ゲーム側の大きさ(model_scale)は各クラスの .cpp の値に合わせる
    // 浮遊敵・ワーム・岩はゲームの独自シェーダ(DrawWithLighting)がモデル内の部品ごとの拡大を使わないので、
    // DxLibで部品ごとの拡大なしの形を書き出したOBJを使う(FBXのままだと浮遊敵・ワームは2.54倍、岩は3〜8倍大きく見える)。
    // ボス(スキンメッシュ)とビル(部品ごとの拡大なし)はFBXのままで合う。
    static readonly Dictionary<string, Entry> Entries = new Dictionary<string, Entry>
    {
        { "FloatingEnemy", new Entry("Drone_fix.obj", "Drone_Albedo.png", Vector3.one * 2f) },      // FloatingEnemy.cpp
        { "WormHead",      new Entry("Worm_fix.obj", "T_Space_Drone_D_Red.png", Vector3.one) },     // WormEnemy.cpp
        { "Boss",          new Entry("Boss.fbx", "T_Mech_LOD2_B.png", Vector3.one * 3f) },          // BossEnemy.cpp
        { "Rock1",         new Entry("Rock1.obj", "rocks_diff_spec_new.png", new Vector3(3f, 5f, 3f)) }, // Rock.cpp
        { "Rock2",         new Entry("Rock2.obj", "rocks_diff_spec_new.png", new Vector3(3f, 5f, 3f)) },
        { "Rock3",         new Entry("Rock3.obj", "rocks_diff_spec_new.png", new Vector3(3f, 5f, 3f)) },
        { "Building_AntennaTower", new Entry("Building_AntennaTower.fbx", "Building_AntennaTower.jpg", Vector3.one) },
        { "Building_BridgeWreck",  new Entry("Building_BridgeWreck.fbx",  "Building_BridgeWreck.jpg",  Vector3.one) },
        { "Building_Collapsed",    new Entry("Building_Collapsed.fbx",    "Building_Collapsed.jpg",    Vector3.one) },
        { "Building_Leaning",      new Entry("Building_Leaning.fbx",      "Building_Leaning.jpg",      Vector3.one) },
        { "Building_OfficeGutted", new Entry("Building_OfficeGutted.fbx", "Building_OfficeGutted.jpg", Vector3.one) },
        { "Building_TallBroken",   new Entry("Building_TallBroken.fbx",   "Building_TallBroken.jpg",   Vector3.one) },
    };

    // 見た目のオブジェクトを作って返す(親は呼ぶ側で設定する)
    public static GameObject CreateVisual(string modelID, PlacedObject.Kind kind)
    {
        if (modelID != null && Entries.TryGetValue(modelID, out var e))
        {
            var model = AssetDatabase.LoadAssetAtPath<GameObject>(ModelDir + e.model);
            if (model != null)
            {
                var go = (GameObject)Object.Instantiate(model);
                go.name = "Visual";
                foreach (var c in go.GetComponentsInChildren<Collider>()) Object.DestroyImmediate(c);
                var mat = GetMaterial(modelID, e.texture, Color.white);
                foreach (var r in go.GetComponentsInChildren<Renderer>())
                {
                    var mats = new Material[r.sharedMaterials.Length];
                    for (int i = 0; i < mats.Length; i++) mats[i] = mat;
                    r.sharedMaterials = mats;
                }
                go.transform.localScale = e.scale / UnityToGame;
                // ワームはUnityの表示だけ大きくするので、ゲームの当たり判定とは合わせられない。
                // 置いた位置(螺旋の中心)にモデルの中心が来るように置く(モデルの原点は中心から上にずれているため)
                if (kind == PlacedObject.Kind.WormEnemy)
                {
                    go.transform.localScale *= WormDisplayScale;
                    go.transform.localPosition = -BoundsCenter(go);
                }
                return go;
            }
            Debug.LogWarning($"[StageModelCatalog] モデルが見つかりません: {ModelDir + e.model}");
        }

        // 一覧にないものは、種類ごとの色の仮の形
        var (shape, size) = PrimitiveOf(kind);
        var prim = GameObject.CreatePrimitive(shape);
        prim.name = "Visual";
        Object.DestroyImmediate(prim.GetComponent<Collider>());
        prim.transform.localScale = Vector3.one * size;
        prim.GetComponent<Renderer>().sharedMaterial = GetMaterial("Kind_" + kind, null, PlacedObject.KindColor(kind));
        return prim;
    }

    // 原点に置いた状態での、モデル全体の見た目の中心(大きさは掛かった状態)
    static Vector3 BoundsCenter(GameObject go)
    {
        var rs = go.GetComponentsInChildren<Renderer>();
        if (rs.Length == 0) return Vector3.zero;
        Bounds b = rs[0].bounds;
        foreach (var r in rs) b.Encapsulate(r.bounds);
        return b.center - go.transform.position;
    }

    // 見た目を作り直して、placedの子に付け替える(古い Visual は消す)
    public static void RebuildVisual(PlacedObject placed)
    {
        var old = placed.transform.Find("Visual");
        if (old != null) Undo.DestroyObjectImmediate(old.gameObject);

        var visual = CreateVisual(placed.modelID, placed.kind);
        Undo.RegisterCreatedObjectUndo(visual, "Rebuild Visual");
        visual.transform.SetParent(placed.transform, false);
    }

    [MenuItem("NovaWing/Refresh Visuals (見た目をゲームのモデルにする)")]
    static void RefreshAll()
    {
        int count = 0;
        foreach (var p in Object.FindObjectsOfType<PlacedObject>())
        {
            RebuildVisual(p);
            count++;
        }
        Debug.Log($"[StageModelCatalog] {count}個の見た目を作り直しました。シーンを保存してください。");
    }

    // コマンドラインから「シーンを開く→見た目を作り直す→保存」を行う入口
    // Unity.exe -batchmode -quit -projectPath StageEditor -executeMethod StageModelCatalog.RefreshStage1Batch
    public static void RefreshStage1Batch()
    {
        var scene = UnityEditor.SceneManagement.EditorSceneManager.OpenScene("Assets/Scenes/Stage1.unity");
        RefreshAll();
        UnityEditor.SceneManagement.EditorSceneManager.SaveScene(scene);
    }

    static (PrimitiveType, float) PrimitiveOf(PlacedObject.Kind kind)
    {
        switch (kind)
        {
            case PlacedObject.Kind.FloatingEnemy: return (PrimitiveType.Sphere, 2.6f);
            case PlacedObject.Kind.WormEnemy: return (PrimitiveType.Capsule, 2f);
            case PlacedObject.Kind.Boss: return (PrimitiveType.Cube, 20f);
            default: return (PrimitiveType.Cube, 3f);
        }
    }

    // modelIDごとのマテリアル(無ければ作って保存する)
    static Material GetMaterial(string name, string texture, Color color)
    {
        string path = MaterialDir + name + ".mat";
        var mat = AssetDatabase.LoadAssetAtPath<Material>(path);
        if (mat != null) return mat;

        if (!AssetDatabase.IsValidFolder("Assets/Models/Materials"))
        {
            AssetDatabase.CreateFolder("Assets/Models", "Materials");
        }
        mat = new Material(Shader.Find("Standard")) { color = color };
        if (texture != null)
        {
            mat.mainTexture = AssetDatabase.LoadAssetAtPath<Texture2D>(ModelDir + texture);
        }
        mat.SetFloat("_Glossiness", 0.1f);
        AssetDatabase.CreateAsset(mat, path);
        return mat;
    }

    // バッチ用: 見た目の確認のため、各モデルの読み込み後の大きさ(ファイルの数値のまま)をログに出す
    public static void LogBoundsBatch()
    {
        AssetDatabase.Refresh();
        foreach (var kv in Entries)
        {
            var model = AssetDatabase.LoadAssetAtPath<GameObject>(ModelDir + kv.Value.model);
            if (model == null) { Debug.Log($"[Bounds] {kv.Key}: not found"); continue; }
            var go = (GameObject)Object.Instantiate(model);
            var rs = go.GetComponentsInChildren<Renderer>();
            if (rs.Length == 0) { Debug.Log($"[Bounds] {kv.Key}: no renderer"); Object.DestroyImmediate(go); continue; }
            Bounds b = rs[0].bounds;
            foreach (var r in rs) b.Encapsulate(r.bounds);
            Debug.Log($"[Bounds] {kv.Key}: min{b.min.ToString("F2")} max{b.max.ToString("F2")}");
            Object.DestroyImmediate(go);
        }
    }
}

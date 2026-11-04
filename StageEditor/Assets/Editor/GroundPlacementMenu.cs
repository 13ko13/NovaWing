using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEngine;

// 2ステージ目の地面(Data/Model/Road)を置くためのメニュー。
// 地面はビルと同じ種類(Building)で置くので、Export Stage CSV で Building.csv に書き出される。
// ・NovaWing/Ground/Lay Straight Rows : 地面をZ方向にまっすぐ敷き詰める(親「Ground」の下にまとめる)
// ・NovaWing/Ground/Add/...           : シーンビューの真ん中に1枚置く(Zは1枚の長さ、高さは400の区切りにそろえる)。
//                                       坂と壁の置き方は Data/Model/Road/README_License.txt の「置き方」を参照
// ・NovaWing/Ground/Change Selected To/... : 選んでいる地面の種類を変える(位置はそのまま)
// ・NovaWing/Ground/Rebuild Walls     : 隣どうしの地面の高さの違いを見て、横の段差に擁壁を置き直す
//このコードはAIを使用しています。
public static class GroundPlacementMenu
{
    // Unityの1単位 = ゲームの100単位
    const float UnityToGame = 100f;
    // 1枚の長さ(ゲーム単位)。README_License.txt の「1枚の長さはZ方向にちょうど2000」
    const float TileLength = 2000f;
    // 敷き詰める範囲(ゲーム単位)。プレイヤーの開始位置(Z=0)より手前から、ボス(Z=34800)の先のfar(5500)まで
    const float LayStartZ = -2000f;
    const float LayEndZ = 42000f;
    // 敷き詰める高さ(ゲーム単位)。プレイヤーは高さ100・当たり判定の半径50なので、当たり判定の下の端は50。
    // -10にすると、プレイヤーが通る帯の線路(上面55→45)・道路(20→10)には一番低く飛んでも当たらない
    public const float LayY = -10f;
    const string GroundRootName = "Ground";

    // 1列に並べる地面と、そのX(ゲーム単位)。
    // 真ん中の3列(線路-1600・道路0・線路+1600、X -2100〜2100)がプレイヤーの通る帯(プレイヤーが動けるのはX±1600)。
    // 帯は平らにして、出っ張りの高い空き地や壊れた地面は使わない。高低差はその外側の4列(±2600・±3600)で付ける
    static readonly (string id, float x)[] RowLayout =
    {
        ("Ground_Side_Left", -4100f),
        ("Ground_Rubble", -3600f),
        ("Ground_Rubble", -2600f),
        ("Rail_Straight", -1600f),
        ("Road_Straight", 0f),
        ("Rail_Straight", 1600f),
        ("Rail_Straight", 2600f),
        ("Ground_Rubble", 3600f),
        ("Ground_Side_Right", 4100f),
    };

    [MenuItem("NovaWing/Ground/Lay Straight Rows (地面をまっすぐ敷き詰める)")]
    static void LayStraightRows()
    {
        var old = GameObject.Find(GroundRootName);
        if (old != null)
        {
            if (!EditorUtility.DisplayDialog("地面を敷き直す",
                $"「{GroundRootName}」の下の地面を全部消して、まっすぐな地面を敷き直します。", "敷き直す", "やめる")) return;
            Undo.DestroyObjectImmediate(old);
        }

        var root = new GameObject(GroundRootName);
        Undo.RegisterCreatedObjectUndo(root, "Lay Ground");

        int count = 0;
        for (float z = LayStartZ; z < LayEndZ; z += TileLength)
        {
            foreach (var (id, x) in RowLayout)
            {
                Create(id, new Vector3(x, LayY, z) / UnityToGame, root.transform);
                count++;
            }
        }
        Selection.activeGameObject = root;
        Debug.Log($"[GroundPlacement] 地面を{count}枚敷きました(Z {LayStartZ}〜{LayEndZ})。種類を変えたい所は選んで NovaWing/Ground/Change Selected To から変えられます。");
    }

    [MenuItem("NovaWing/Ground/Add/Road_Straight")] static void AddRoadStraight() => Add("Road_Straight");
    [MenuItem("NovaWing/Ground/Add/Road_Damaged")] static void AddRoadDamaged() => Add("Road_Damaged");
    [MenuItem("NovaWing/Ground/Add/Road_Debris")] static void AddRoadDebris() => Add("Road_Debris");
    [MenuItem("NovaWing/Ground/Add/Rail_Straight")] static void AddRailStraight() => Add("Rail_Straight");
    [MenuItem("NovaWing/Ground/Add/Rail_Damaged")] static void AddRailDamaged() => Add("Rail_Damaged");
    [MenuItem("NovaWing/Ground/Add/Ground_Rubble")] static void AddGroundRubble() => Add("Ground_Rubble");
    [MenuItem("NovaWing/Ground/Add/Ground_Side_Left")] static void AddGroundSideLeft() => Add("Ground_Side_Left");
    [MenuItem("NovaWing/Ground/Add/Ground_Side_Right")] static void AddGroundSideRight() => Add("Ground_Side_Right");
    [MenuItem("NovaWing/Ground/Add/Slope/Road_Slope_Up400")] static void AddRoadUp() => Add("Road_Slope_Up400");
    [MenuItem("NovaWing/Ground/Add/Slope/Road_Slope_Down400")] static void AddRoadDown() => Add("Road_Slope_Down400");
    [MenuItem("NovaWing/Ground/Add/Slope/Rail_Slope_Up400")] static void AddRailUp() => Add("Rail_Slope_Up400");
    [MenuItem("NovaWing/Ground/Add/Slope/Rail_Slope_Down400")] static void AddRailDown() => Add("Rail_Slope_Down400");
    [MenuItem("NovaWing/Ground/Add/Slope/Ground_Rubble_Slope_Up400")] static void AddRubbleUp() => Add("Ground_Rubble_Slope_Up400");
    [MenuItem("NovaWing/Ground/Add/Slope/Ground_Rubble_Slope_Down400")] static void AddRubbleDown() => Add("Ground_Rubble_Slope_Down400");
    [MenuItem("NovaWing/Ground/Add/Wall/Wall_Side")] static void AddWallSide() => Add("Wall_Side");
    [MenuItem("NovaWing/Ground/Add/Wall/Wall_Side_Slope")] static void AddWallSideSlope() => Add("Wall_Side_Slope");
    [MenuItem("NovaWing/Ground/Add/Wall/Wall_Front")] static void AddWallFront() => Add("Wall_Front");
    // 帯の上の構造物。原点は地面の高さなので、通る帯の地面(LayY)に置いてから位置を直す
    [MenuItem("NovaWing/Ground/Add/Structure/Overpass_Cross")] static void AddOverpass() => Add("Overpass_Cross");
    [MenuItem("NovaWing/Ground/Add/Structure/Overpass_Cross_Broken")] static void AddOverpassBroken() => Add("Overpass_Cross_Broken");
    [MenuItem("NovaWing/Ground/Add/Structure/Sign_Gantry")] static void AddSignGantry() => Add("Sign_Gantry");

    [MenuItem("NovaWing/Ground/Change Selected To/Road_Straight")] static void ToRoadStraight() => ChangeSelected("Road_Straight");
    [MenuItem("NovaWing/Ground/Change Selected To/Road_Damaged")] static void ToRoadDamaged() => ChangeSelected("Road_Damaged");
    [MenuItem("NovaWing/Ground/Change Selected To/Road_Debris")] static void ToRoadDebris() => ChangeSelected("Road_Debris");
    [MenuItem("NovaWing/Ground/Change Selected To/Rail_Straight")] static void ToRailStraight() => ChangeSelected("Rail_Straight");
    [MenuItem("NovaWing/Ground/Change Selected To/Rail_Damaged")] static void ToRailDamaged() => ChangeSelected("Rail_Damaged");
    [MenuItem("NovaWing/Ground/Change Selected To/Ground_Rubble")] static void ToGroundRubble() => ChangeSelected("Ground_Rubble");
    [MenuItem("NovaWing/Ground/Change Selected To/Slope/Road_Slope_Up400")] static void ToRoadUp() => ChangeSelected("Road_Slope_Up400");
    [MenuItem("NovaWing/Ground/Change Selected To/Slope/Road_Slope_Down400")] static void ToRoadDown() => ChangeSelected("Road_Slope_Down400");
    [MenuItem("NovaWing/Ground/Change Selected To/Slope/Rail_Slope_Up400")] static void ToRailUp() => ChangeSelected("Rail_Slope_Up400");
    [MenuItem("NovaWing/Ground/Change Selected To/Slope/Rail_Slope_Down400")] static void ToRailDown() => ChangeSelected("Rail_Slope_Down400");
    [MenuItem("NovaWing/Ground/Change Selected To/Slope/Ground_Rubble_Slope_Up400")] static void ToRubbleUp() => ChangeSelected("Ground_Rubble_Slope_Up400");
    [MenuItem("NovaWing/Ground/Change Selected To/Slope/Ground_Rubble_Slope_Down400")] static void ToRubbleDown() => ChangeSelected("Ground_Rubble_Slope_Down400");

    // 地面の高さの刻み(ゲーム単位)。坂1枚で上がり下がりする高さ
    const float HeightStep = 400f;

    // シーンビューの真ん中に置く。高さは400の刻み、Zは1枚の長さの区切りにそろえる(並べたときに継ぎ目が合うように)
    static void Add(string id)
    {
        Vector3 pos = SceneView.lastActiveSceneView != null ? SceneView.lastActiveSceneView.pivot : Vector3.zero;
        float step = HeightStep / UnityToGame;
        pos.y = Mathf.Round(pos.y / step) * step;
        float tile = TileLength / UnityToGame;
        pos.z = Mathf.Round(pos.z / tile) * tile;

        var root = GameObject.Find(GroundRootName);
        var go = Create(id, pos, root != null ? root.transform : null);
        Selection.activeGameObject = go;
    }

    static GameObject Create(string id, Vector3 unityPos, Transform parent)
    {
        var go = new GameObject($"{id}_{unityPos.z * UnityToGame:0}");
        Undo.RegisterCreatedObjectUndo(go, "Add " + id);
        if (parent != null) go.transform.SetParent(parent, false);
        go.transform.position = unityPos;

        var p = go.AddComponent<PlacedObject>();
        p.kind = PlacedObject.Kind.Building;
        p.modelID = id;

        StageModelCatalog.RebuildVisual(p);
        return go;
    }

    // 選んでいる地面の種類を変える。道路どうし・線路どうしで変えないと幅が合わないので、違うときは知らせる
    static void ChangeSelected(string id)
    {
        int count = 0;
        foreach (var obj in Selection.gameObjects)
        {
            var p = obj.GetComponentInParent<PlacedObject>();
            if (p == null || p.kind != PlacedObject.Kind.Building || p.modelID == null || p.modelID.StartsWith("Building_")) continue;

            if (Width(p.modelID) != Width(id))
            {
                Debug.LogWarning($"[GroundPlacement] {p.name}: {p.modelID} と {id} は幅が違うので変えませんでした。");
                continue;
            }
            Undo.RecordObject(p, "Change Ground");
            p.modelID = id;
            Undo.RecordObject(p.gameObject, "Change Ground");
            p.gameObject.name = $"{id}_{p.transform.position.z * UnityToGame:0}";
            StageModelCatalog.RebuildVisual(p);
            count++;
        }
        Debug.Log($"[GroundPlacement] {count}枚を {id} に変えました。");
    }

    // 幅(ゲーム単位)。道路2200、外側の地面8000、線路・空き地1000。壁と構造物は地面と入れ替えないので-1
    static float Width(string id)
    {
        if (id.StartsWith("Wall_") || id.StartsWith("Overpass_") || id.StartsWith("Sign_")) return -1f;
        if (id.StartsWith("Road_")) return 2200f;
        if (id.StartsWith("Ground_Side_")) return 8000f;
        return 1000f;
    }

    // ---------- 擁壁の自動配置 ----------

    // 地面1枚の情報(ゲーム単位)
    struct Tile
    {
        public float minX, maxX; // 横の範囲
        public float z;          // 手前の端
        public float h0, h1;     // 手前の端と奥の端の地面の高さ
    }

    [MenuItem("NovaWing/Ground/Rebuild Walls (段差に壁を置き直す)")]
    static void RebuildWallsMenu()
    {
        var root = GameObject.Find(GroundRootName);
        if (root == null) { Debug.LogError($"[GroundPlacement] 「{GroundRootName}」がありません。"); return; }
        int count = RebuildWalls(root.transform);
        Debug.Log($"[GroundPlacement] 擁壁を{count}個置き直しました。");
    }

    // rootの下の擁壁(Wall_)を全部消し、同じZの列で隣り合う地面の高さが違う所に Wall_Side / Wall_Side_Slope を置く。
    // 置き方は README の「置き方」と同じ(壁は低い側、高い側の地面の端から50外に中心)。
    // 手前と奥の段差(Wall_Front)は置かないので、Zの方向の高さの変化は坂でつなぐこと
    public static int RebuildWalls(Transform root)
    {
        foreach (var p in root.GetComponentsInChildren<PlacedObject>())
        {
            if (p.modelID != null && p.modelID.StartsWith("Wall_")) Undo.DestroyObjectImmediate(p.gameObject);
        }

        var tiles = new List<Tile>();
        foreach (var p in root.GetComponentsInChildren<PlacedObject>())
        {
            if (TryGetTile(p, out var t)) tiles.Add(t);
        }

        int count = 0;
        foreach (var row in tiles.GroupBy(t => Mathf.Round(t.z)))
        {
            var sorted = row.OrderBy(t => t.minX).ToList();
            for (int i = 0; i + 1 < sorted.Count; i++)
            {
                Tile left = sorted[i], right = sorted[i + 1];
                if (Mathf.Abs(left.maxX - right.minX) > 1f) continue; // 隣り合っていない
                count += PlaceWallsBetween(root, left, right, left.maxX);
            }
        }
        return count;
    }

    static bool TryGetTile(PlacedObject p, out Tile t)
    {
        t = default;
        string id = p.modelID;
        // 地面(道路・線路・空き地・外側の地面)だけ。ビル・擁壁・陸橋などの構造物は数えない
        if (p.kind != PlacedObject.Kind.Building || id == null) return false;
        if (!id.StartsWith("Road_") && !id.StartsWith("Rail_") && !id.StartsWith("Ground_")) return false;

        Vector3 pos = p.transform.position * UnityToGame;
        float w = Width(id);
        if (id == "Ground_Side_Left") { t.minX = pos.x - w; t.maxX = pos.x; }
        else if (id == "Ground_Side_Right") { t.minX = pos.x; t.maxX = pos.x + w; }
        else { t.minX = pos.x - w * 0.5f; t.maxX = pos.x + w * 0.5f; }
        t.z = pos.z;
        t.h0 = pos.y;
        t.h1 = pos.y + (id.Contains("Slope_Up400") ? HeightStep : id.Contains("Slope_Down400") ? -HeightStep : 0f);
        return true;
    }

    // 2枚の境目(boundaryX)に壁を置く。置いた数を返す
    static int PlaceWallsBetween(Transform root, Tile left, Tile right, float boundaryX)
    {
        const float eps = 0.5f;
        if (Mathf.Abs(left.h0 - right.h0) < eps && Mathf.Abs(left.h1 - right.h1) < eps) return 0;

        bool leftHigh = left.h0 >= right.h0 - eps && left.h1 >= right.h1 - eps;
        bool rightHigh = right.h0 >= left.h0 - eps && right.h1 >= left.h1 - eps;
        if (!leftHigh && !rightHigh)
        {
            Debug.LogWarning($"[GroundPlacement] Z={left.z} X={boundaryX}: 高さが途中で入れ替わるので壁を置けません。");
            return 0;
        }
        Tile high = leftHigh ? left : right, low = leftHigh ? right : left;
        // 壁の中心は、高い側の端から50だけ低い側に寄せる
        float x = boundaryX + (leftHigh ? 50f : -50f);
        float lowMin = Mathf.Min(low.h0, low.h1);

        int count = 0;
        float bottom;
        if (Mathf.Abs(high.h1 - high.h0) < eps)
        {
            Create("Wall_Side", new Vector3(x, high.h0, high.z) / UnityToGame, root);
            bottom = high.h0 - HeightStep;
        }
        else if (high.h1 > high.h0)
        {
            Create("Wall_Side_Slope", new Vector3(x, high.h0, high.z) / UnityToGame, root);
            bottom = high.h0 - HeightStep;
        }
        else
        {
            // 下りは上りの壁をY軸で180°回して使う(回すとZが-2000〜0に広がるので、奥の端に置く)
            var go = Create("Wall_Side_Slope", new Vector3(x, high.h1, high.z + TileLength) / UnityToGame, root);
            go.transform.rotation = Quaternion.Euler(0f, 180f, 0f);
            bottom = high.h1 - HeightStep;
        }
        count++;

        // 低い側の地面まで届かなければ、下に Wall_Side を重ねる
        while (bottom > lowMin + eps)
        {
            Create("Wall_Side", new Vector3(x, bottom, high.z) / UnityToGame, root);
            bottom -= HeightStep;
            count++;
        }
        return count;
    }
}

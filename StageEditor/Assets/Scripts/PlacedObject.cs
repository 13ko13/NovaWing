using UnityEngine;

// ステージに置くオブジェクトの目印。
// StageCsvExporter がシーン内のこのコンポーネントを集めて、種類(kind)ごとにCSVへ書き出す。
public class PlacedObject : MonoBehaviour
{
    public enum Kind
    {
        Rock,
        FloatingEnemy,
        WormEnemy,
        Boss,
        // 2面のビル(障害物)。並びの数字がシーンに保存されているので、新しい種類は必ず末尾に足す
        Building,
    }

    [Tooltip("CSVの1列目の名前(ゲーム側のモデルID。例: Rock1, FloatingEnemy, WormHead, Boss, Building_Leaning)")]
    public string modelID;

    [Tooltip("どのCSVに書き出すか")]
    public Kind kind;

    [Header("WormEnemy 専用")]
    public int segmentCount = 5;
    [Tooltip("1 か -1")]
    public int direction = 1;
    [Tooltip("プレイヤーのZ座標がこの値を超えたら動き出す(ゲーム単位。Unity単位ではない)")]
    public float activatePlayerZ = 3000f;

#if UNITY_EDITOR
    // 名前の文字を出す、シーンビューのカメラからの距離(Unity単位)。遠くの文字が重なって読めなくなるのを防ぐ
    const float LabelDistance = 40f;
    // Unityの1単位 = ゲームの100単位
    const float UnityToGame = 100f;

    // ワームの螺旋の半径(WormEnemy.cppのspiral_radiusと同じ値)
    const float WormSpiralRadius = 160f;

    // 当たり判定の半径(ゲーム単位。EnemyStatus.csvのcolRadius)。枠の大きさに使う
    static float CollisionRadius(Kind k)
    {
        switch (k)
        {
            case Kind.FloatingEnemy: return 132f;
            case Kind.WormEnemy: return 40f;
            default: return 150f;
        }
    }

    // シーンビューで「何の敵か」が分かるように、種類ごとの色の枠と名前を出す(エディタ専用)
    void OnDrawGizmos()
    {
        Gizmos.color = KindColor(kind);
        if (kind == Kind.WormEnemy)
        {
            DrawWormGizmo();
        }
        else if (kind != Kind.Boss && kind != Kind.Building)
        {
            Gizmos.DrawWireSphere(transform.position, CollisionRadius(kind) / UnityToGame);
        }

        // 名前は、選んでいるものか、カメラの近くにあるものだけ出す
        bool isSelected = UnityEditor.Selection.Contains(gameObject);
        var cam = UnityEditor.SceneView.currentDrawingSceneView != null ? UnityEditor.SceneView.currentDrawingSceneView.camera : null;
        bool isNear = cam != null && Vector3.Distance(cam.transform.position, transform.position) < LabelDistance;
        if (!isSelected && !isNear) return;

        string label = JapaneseName(kind, modelID);
        if (kind == Kind.WormEnemy && isSelected) label += $"\n胴{segmentCount} 向き{direction} 起動Z{activatePlayerZ}";

        var style = new GUIStyle(UnityEditor.EditorStyles.boldLabel);
        style.normal.textColor = KindColor(kind);
        UnityEditor.Handles.Label(transform.position + Vector3.up * 2f, label, style);
    }

    // ワーム: 置いた位置(螺旋の中心)を囲む、螺旋の円(ワームが通る太さの目安)だけを描く。
    // 頭のモデルはUnityの表示だけ大きくしているので、当たり判定の枠は描かない
    void DrawWormGizmo()
    {
        UnityEditor.Handles.color = KindColor(kind);
        UnityEditor.Handles.DrawWireDisc(transform.position, Vector3.forward, WormSpiralRadius / UnityToGame);
    }

    // シーンビューに出す短い名前
    static string JapaneseName(Kind k, string id)
    {
        switch (k)
        {
            case Kind.FloatingEnemy: return "浮遊敵";
            case Kind.WormEnemy: return "ワーム";
            case Kind.Boss: return "ボス";
            case Kind.Rock: return "岩" + (id != null && id.StartsWith("Rock") ? id.Substring(4) : "");
            case Kind.Building: return id != null && id.StartsWith("Building_") ? id.Substring(9) : "ビル";
            default: return id;
        }
    }
#endif

    // 種類ごとの色(シーンビューの枠・文字と、モデルが無いときの仮の形に使う)
    public static Color KindColor(Kind k)
    {
        switch (k)
        {
            case Kind.Rock: return Color.gray;
            case Kind.FloatingEnemy: return Color.cyan;
            case Kind.WormEnemy: return Color.magenta;
            case Kind.Boss: return Color.red;
            case Kind.Building: return Color.yellow;
            default: return Color.white;
        }
    }
}

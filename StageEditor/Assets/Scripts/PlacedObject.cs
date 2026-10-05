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
    }

    [Tooltip("CSVの1行目の名前(ゲーム側のモデルID。例: Rock1, FloatingEnemy, WormHead, Boss)")]
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
    // シーンビューで「何の敵か」が分かるように、種類ごとの色の枠と文字を出す(エディタ専用)
    void OnDrawGizmos()
    {
        Gizmos.color = KindColor(kind);
        Gizmos.DrawWireSphere(transform.position, 1.5f);

        string label = $"{kind}\n{modelID}";
        if (kind == Kind.WormEnemy) label += $"\n胴{segmentCount} 向き{direction} 起動Z{activatePlayerZ}";

        var style = new GUIStyle(UnityEditor.EditorStyles.boldLabel);
        style.normal.textColor = KindColor(kind);
        UnityEditor.Handles.Label(transform.position + Vector3.up * 2f, label, style);
    }

    static Color KindColor(Kind k)
    {
        switch (k)
        {
            case Kind.Rock: return Color.gray;
            case Kind.FloatingEnemy: return Color.cyan;
            case Kind.WormEnemy: return Color.magenta;
            case Kind.Boss: return Color.red;
            default: return Color.white;
        }
    }
#endif
}

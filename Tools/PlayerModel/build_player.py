# 自機のモデルを作るスクリプト(Blender 5.x)
# 単位: Blenderの1m = ゲームの100。正面(機首)はBlenderの -Y、上は +Z。+X = パイロットから見て左。
# 何度実行しても同じものができる(前に作った Player のものは消して作り直す)。
#
# 形(ブロックアウトのB案「鷹」):
# - 細長い胴(とがった機首、横に稜線)と、両肩の空気取り入れ口のふくらみ
# - 付け根で上がり、折れ目から翼端へ下がるガル翼(後ろから見て「M」の字)。翼端が一番低い
# - 背中の1枚の垂直尾翼と、下に外へ開く小さなひれ2枚
# - 後ろに噴射口1つ(後ろの端が +Y 6.5m = ゲームの +Z 650)
# 色: 白が主、背骨と翼の上の内側のパネル・翼端・尾翼が濃いグレー。噴射口と細い光るライン(水色)。
# 翼端のフレーム: Collider7(+X、パイロットの左)/ Collider6(-X、右)。翼端の後ろの下の角。
# 色・滑らかさ・発光・法線はマテリアルの出力ノード(OUT_COLOR / OUT_SMOOTH / OUT_EMIT / OUT_NORMAL)で決め、
# bake_player.py が画像に焼き付ける。色の値は「PNGに入る値(ゲームのシェーダーがそのまま計算に使う値)」で書く。

import bpy
import bmesh
import math
from mathutils import Vector, Matrix

COLL_NAME = "Player"
OBJ_NAME = "Player_Body"
COLLIDER_L = "Collider7"   # +X 側(パイロットから見て左)
COLLIDER_R = "Collider6"   # -X 側(パイロットから見て右)


# ------------------------------------------------------------
# 片付け
# ------------------------------------------------------------
def cleanup():
    for name in ("Cube", "Camera", "Light"):
        ob = bpy.data.objects.get(name)
        if ob is not None:
            bpy.data.objects.remove(ob, do_unlink=True)
    coll = bpy.data.collections.get(COLL_NAME)
    if coll is not None:
        for ob in list(coll.all_objects):
            bpy.data.objects.remove(ob, do_unlink=True)
        bpy.data.collections.remove(coll)
    # 名前に .001 が付かないよう、同じ名前で始まるものは全部消す
    for ob in list(bpy.data.objects):
        if ob.name.startswith(("Player_", "Pp_", "Collider")):
            bpy.data.objects.remove(ob, do_unlink=True)
    for m in list(bpy.data.materials):
        if m.name.startswith("P_"):
            bpy.data.materials.remove(m)
    for g in list(bpy.data.node_groups):
        if g.name.startswith("P_"):
            bpy.data.node_groups.remove(g)
    for me in list(bpy.data.meshes):
        if me.users == 0:
            bpy.data.meshes.remove(me)


def get_collection():
    coll = bpy.data.collections.new(COLL_NAME)
    bpy.context.scene.collection.children.link(coll)
    return coll


# ------------------------------------------------------------
# ノードを組む道具(2面のボスと同じ)
# ------------------------------------------------------------
class G:
    def __init__(self, nt):
        self.nt = nt

    def node(self, t, **props):
        n = self.nt.nodes.new(t)
        for k, v in props.items():
            setattr(n, k, v)
        return n

    def set(self, sock, v):
        if v is None:
            return
        if isinstance(v, bpy.types.NodeSocket):
            self.nt.links.new(v, sock)
        else:
            sock.default_value = v

    def math(self, op, a, b=None, c=None, clamp=False):
        n = self.node("ShaderNodeMath", operation=op, use_clamp=clamp)
        self.set(n.inputs[0], a)
        self.set(n.inputs[1], b)
        if c is not None:
            self.set(n.inputs[2], c)
        return n.outputs[0]

    def vm(self, op, a, b=None, scale=None):
        n = self.node("ShaderNodeVectorMath", operation=op)
        self.set(n.inputs[0], a)
        self.set(n.inputs[1], b)
        if scale is not None:
            self.set(n.inputs[3], scale)
        if op in ("DOT_PRODUCT", "LENGTH", "DISTANCE"):
            return n.outputs["Value"]
        return n.outputs["Vector"]

    def smooth(self, x, e0, e1, t0=0.0, t1=1.0):
        n = self.node("ShaderNodeMapRange", data_type="FLOAT", interpolation_type="SMOOTHSTEP", clamp=True)
        self.set(n.inputs["Value"], x)
        self.set(n.inputs["From Min"], e0)
        self.set(n.inputs["From Max"], e1)
        self.set(n.inputs["To Min"], t0)
        self.set(n.inputs["To Max"], t1)
        return n.outputs["Result"]

    def lerp(self, a, b, f):
        d = self.vm("SUBTRACT", b, a)
        return self.vm("ADD", a, self.vm("SCALE", d, scale=f))

    def xyz(self, x, y, z):
        n = self.node("ShaderNodeCombineXYZ")
        self.set(n.inputs[0], x)
        self.set(n.inputs[1], y)
        self.set(n.inputs[2], z)
        return n.outputs[0]

    def sep(self, v):
        n = self.node("ShaderNodeSeparateXYZ")
        self.set(n.inputs[0], v)
        return n.outputs[0], n.outputs[1], n.outputs[2]

    def clamp01(self, x):
        return self.math("ADD", x, 0.0, clamp=True)


def new_group(name, inputs, outputs):
    ng = bpy.data.node_groups.new(name, "ShaderNodeTree")
    for n, t in inputs:
        ng.interface.new_socket(name=n, in_out="INPUT", socket_type=t)
    for n, t in outputs:
        ng.interface.new_socket(name=n, in_out="OUTPUT", socket_type=t)
    gi = ng.nodes.new("NodeGroupInput")
    go = ng.nodes.new("NodeGroupOutput")
    return ng, gi, go


def build_panel_group():
    """面の上の2次元の座標(U,V)から、長方形のパネルの継ぎ目・パネルごとの乱数を作る"""
    F = "NodeSocketFloat"
    ng, gi, go = new_group("P_Panels2D",
                           [("U", F), ("V", F), ("W", F), ("H", F), ("Mortar", F), ("Offset", F), ("Seed", F)],
                           [("Seam", F), ("Rand", "NodeSocketColor")])
    g = G(ng)
    U, V, W, H, M, OFF, SEED = (gi.outputs[i] for i in range(7))
    row = g.math("FLOOR", g.math("DIVIDE", V, H))
    u2 = g.math("MULTIPLY_ADD", row, g.math("MULTIPLY", OFF, W), U)
    col = g.math("FLOOR", g.math("DIVIDE", u2, W))
    fu = g.math("MULTIPLY", g.math("FRACT", g.math("DIVIDE", u2, W)), W)
    du = g.math("MINIMUM", fu, g.math("SUBTRACT", W, fu))
    fv = g.math("MULTIPLY", g.math("FRACT", g.math("DIVIDE", V, H)), H)
    dv = g.math("MINIMUM", fv, g.math("SUBTRACT", H, fv))
    d = g.math("MINIMUM", du, dv)
    half = g.math("MULTIPLY", M, 0.5)
    seam = g.smooth(d, half, g.math("ADD", half, 0.01), 1.0, 0.0)
    wn = g.node("ShaderNodeTexWhiteNoise", noise_dimensions="3D")
    g.set(wn.inputs["Vector"], g.xyz(col, row, SEED))
    ng.links.new(seam, go.inputs["Seam"])
    ng.links.new(wn.outputs["Color"], go.inputs["Rand"])
    return ng


# ------------------------------------------------------------
# 細かい凹凸(ハッチ・通気口・リベット・動翼の溝)
# 機体の座標に場所を決めて描く(ポリゴンは増やさない)。ゲームの距離でも見えるよう、
# 大きさ0.3m以上・溝の幅3cm前後と大きめにする(CGは情報が減るので誇張する)。
# 投影の向き: side = 横から(u=y, v=z, 奥行き=|x|)、top / bottom = 上下から(u=y, v=|x|, 奥行き=z)
# 書式: (種類, 投影, 中心u, 中心v, 半幅u, 半幅v, 奥行きの範囲(lo, hi), 種類ごとの値)
# ------------------------------------------------------------
DETAILS = [
    # 機首の横: 点検口2つとリベットの列
    ("hatch", "side", -3.55, -0.08, 0.5, 0.16, (0.25, 0.85)),
    ("hatch", "side", -5.0, 0.0, 0.22, 0.09, (0.1, 0.5)),
    ("rivets", "side", -5.3, -0.24, -2.5, -0.27, (0.2, 0.95), 0.13),
    # 機首の上(キャノピーの前)と下
    ("hatch", "top", -5.35, 0.0, 0.3, 0.1, (0.0, 0.5)),
    ("hatch", "bottom", -2.2, 0.0, 0.6, 0.24, (-0.8, 0.0)),
    ("hatch", "bottom", 1.4, 0.0, 0.7, 0.3, (-0.9, 0.0)),
    ("vent", "bottom", 3.6, 0.0, 0.45, 0.2, (-0.9, 0.0), 0.08),
    ("rivets", "bottom", -3.2, 0.45, 4.6, 0.45, (-0.9, 0.0), 0.16),
    # 背骨(濃いグレー): 通気口と点検口
    ("vent", "top", 1.2, 0.0, 0.42, 0.16, (0.4, 1.0), 0.085),
    ("hatch", "top", 3.3, 0.0, 0.45, 0.17, (0.4, 1.0)),
    ("vent", "top", -0.25, 0.0, 0.2, 0.12, (0.4, 1.0), 0.06),
    # 肩の外側: 点検口・通気口・リベット
    ("hatch", "side", 0.9, 0.06, 0.75, 0.2, (1.15, 1.5)),
    ("vent", "side", 3.5, 0.05, 0.55, 0.17, (1.15, 1.5), 0.09),
    ("rivets", "side", -1.3, -0.25, 4.6, -0.25, (1.15, 1.5), 0.15),
    # 翼の上: 点検口・動翼の溝・リベット
    ("hatch", "top", 3.7, 1.95, 0.55, 0.28, (-0.2, 1.0)),
    ("hatch", "top", 4.55, 3.05, 0.35, 0.18, (-0.6, 0.6)),
    ("line", "top", 4.95, 1.3, 5.08, 4.05, (-0.8, 1.0), 0.0),
    ("line", "top", 4.98, 2.55, 5.6, 2.55, (-0.8, 1.0), 0.0),
    ("line", "top", 5.04, 3.55, 5.7, 3.55, (-0.8, 1.0), 0.0),
    ("rivets", "top", 1.6, 2.0, 4.85, 2.0, (-0.2, 1.0), 0.17),
    ("rivets", "top", 2.9, 1.35, 4.85, 1.35, (-0.2, 1.0), 0.17),
    # 翼の下
    ("hatch", "bottom", 3.4, 2.1, 0.6, 0.3, (-1.0, 0.8)),
    ("line", "bottom", 4.95, 1.3, 5.08, 4.05, (-1.0, 0.8), 0.0),
    ("line", "bottom", 4.98, 2.55, 5.6, 2.55, (-1.0, 0.8), 0.0),
    # 垂直尾翼: 方向舵の溝と点検口
    ("line", "side", 5.0, 0.8, 5.72, 2.45, (0.0, 0.12), 0.0),
    ("line", "side", 5.0, 0.8, 5.65, 0.8, (0.0, 0.12), 0.0),
    ("hatch", "side", 3.55, 1.2, 0.32, 0.15, (0.0, 0.12)),
    ("rivets", "side", 2.3, 0.72, 5.0, 0.72, (0.0, 0.12), 0.14),
]


def build_details(g, x, y, z, ax, nx, ny, nz):
    """DETAILS を高さ(h、m)と暗さ(dark、0〜1)にする"""
    GROOVE_W = 0.016   # 溝の幅の半分
    GROOVE_D = -0.012  # 溝の深さ
    proj = {
        "side": (y, z, ax, g.math("ABSOLUTE", nx)),
        "top": (y, ax, z, nz),
        "bottom": (y, ax, z, g.math("MULTIPLY", nz, -1.0)),
    }
    h_t = None
    d_t = None

    def acc(h, d):
        nonlocal h_t, d_t
        h_t = h if h_t is None else g.math("ADD", h_t, h)
        d_t = d if d_t is None else g.math("MAXIMUM", d_t, d)

    for det in DETAILS:
        kind, pj, cu, cv, a, b, (lo, hi) = det[:7]
        u, v, w, n = proj[pj]
        # 面の向きと奥行きで、その部品の面だけに描く
        sel = g.math("MULTIPLY", g.smooth(n, 0.45, 0.6),
                     g.math("MULTIPLY", g.smooth(w, lo - 0.01, lo + 0.01), g.smooth(w, hi + 0.01, hi - 0.01)))
        if kind in ("hatch", "vent"):
            du = g.math("SUBTRACT", g.math("ABSOLUTE", g.math("SUBTRACT", u, cu)), a)
            dv = g.math("SUBTRACT", g.math("ABSOLUTE", g.math("SUBTRACT", v, cv)), b)
            dbox = g.math("MAXIMUM", du, dv)   # 長方形の縁からの距離(内側が負)
            groove = g.smooth(g.math("ABSOLUTE", dbox), GROOVE_W, GROOVE_W + 0.01, 1.0, 0.0)
            inside = g.smooth(dbox, 0.0, -0.01)
            h = g.math("MULTIPLY", groove, GROOVE_D)
            dark = g.math("MULTIPLY", groove, 0.8)
            if kind == "hatch":
                # 少しへこんだ蓋と、四隅の留め具
                h = g.math("MULTIPLY_ADD", inside, -0.003, h)
                cu2 = g.math("SUBTRACT", g.math("ABSOLUTE", g.math("SUBTRACT", u, cu)), a - 0.06)
                cv2 = g.math("SUBTRACT", g.math("ABSOLUTE", g.math("SUBTRACT", v, cv)), b - 0.06)
                dist = g.vm("LENGTH", g.xyz(cu2, cv2, 0.0))
                bolt = g.smooth(dist, 0.018, 0.026, 1.0, 0.0)
                h = g.math("MULTIPLY_ADD", bolt, 0.006, h)
                dark = g.math("MAXIMUM", dark, g.math("MULTIPLY", bolt, 0.25))
            else:
                # 通気口: 中が暗く、斜めの羽板が並ぶ(u の向きに)
                pitch = det[7]
                t = g.math("FRACT", g.math("DIVIDE", g.math("SUBTRACT", u, cu), pitch))
                slat = g.math("MULTIPLY", inside, g.math("SUBTRACT", 1.0, t))
                h = g.math("MULTIPLY_ADD", slat, -0.02, h)
                h = g.math("MULTIPLY_ADD", inside, -0.004, h)
                dark = g.math("MAXIMUM", dark, g.math("MULTIPLY", inside,
                                                      g.math("MULTIPLY_ADD", g.math("SUBTRACT", 1.0, t), 0.5, 0.35)))
        else:
            # 線分 (cu,cv)→(a,b) までの距離。rivets は間隔ごとの点、line は溝
            p0 = (cu, cv)
            p1 = (a, b)
            L = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
            dx, dy = (p1[0] - p0[0]) / L, (p1[1] - p0[1]) / L
            ru = g.math("SUBTRACT", u, p0[0])
            rv = g.math("SUBTRACT", v, p0[1])
            t = g.math("ADD", g.math("MULTIPLY", ru, dx), g.math("MULTIPLY", rv, dy))
            if kind == "rivets":
                sp = det[7]
                t = g.math("MULTIPLY", g.math("ROUND", g.math("DIVIDE", t, sp)), sp)
            t = g.math("MINIMUM", g.math("MAXIMUM", t, 0.0), L)
            qu = g.math("SUBTRACT", ru, g.math("MULTIPLY", t, dx))
            qv = g.math("SUBTRACT", rv, g.math("MULTIPLY", t, dy))
            dist = g.vm("LENGTH", g.xyz(qu, qv, 0.0))
            if kind == "rivets":
                dot = g.smooth(dist, 0.014, 0.022, 1.0, 0.0)
                h = g.math("MULTIPLY", dot, 0.005)
                dark = g.math("MULTIPLY", dot, 0.3)
            else:
                groove = g.smooth(dist, GROOVE_W, GROOVE_W + 0.01, 1.0, 0.0)
                h = g.math("MULTIPLY", groove, GROOVE_D)
                dark = g.math("MULTIPLY", groove, 0.8)
        acc(g.math("MULTIPLY", h, sel), g.math("MULTIPLY", dark, sel))
    return h_t, d_t


def build_surface_group(panel_ng):
    """金属の表面: パネル割り(3方向から投影)・すき間の汚れ(AO)・角の丸み・塗装はがれ・細かい凹凸。
    Extra: 形に合わせて引いた溝(パネルの色の境目)。継ぎ目と同じように溝にする
    BevelR: 角を丸めて見せる半径(法線だけ。縁に柔らかいハイライトが出る)"""
    F = "NodeSocketFloat"
    ng, gi, go = new_group("P_Surface",
                           [("BaseColor", "NodeSocketColor"), ("Panel", F), ("Wear", F), ("Dirt", F),
                            ("PanelScale", F), ("Extra", F), ("BevelR", F), ("Smooth", F), ("ExtraH", F)],
                           [("Color", "NodeSocketColor"), ("Normal", "NodeSocketVector"), ("Smooth", F)])
    g = G(ng)
    BASE, PANEL, WEAR, DIRT, PSCALE, EXTRA, BEVR, SMOOTH, EXTRAH = (gi.outputs[i] for i in range(9))
    tc = g.node("ShaderNodeTexCoord")
    obj = tc.outputs["Object"]
    x, y, z = g.sep(obj)
    geo = g.node("ShaderNodeNewGeometry")
    nx, ny, nz = g.sep(geo.outputs["Normal"])
    ws = [g.math("POWER", g.math("ABSOLUTE", c), 6.0) for c in (nx, ny, nz)]
    wsum = g.math("ADD", g.math("ADD", ws[0], ws[1]), ws[2])
    ws = [g.math("DIVIDE", w, wsum) for w in ws]
    # 左右対称にする(|x|で割る)。機体の中心線に継ぎ目が来るよう、x はそのまま0から
    ax = g.math("ABSOLUTE", x)

    def panels(u, v, w, h, m, off, seed):
        n = g.node("ShaderNodeGroup")
        n.node_tree = panel_ng
        for i, val in enumerate((u, v, w, h, m, off, seed)):
            g.set(n.inputs[i], val)
        return n.outputs["Seam"], n.outputs["Rand"]

    # 戦闘機なので細長いパネル(前後に長い)。小さいパネル(細い継ぎ目)と大きいパネル(太い継ぎ目)
    seam_t = None
    rand_t = None
    for k, (u, v) in enumerate(((y, z), (ax, z), (ax, y))):
        # u,v の組で「前後(y)」がどちらにあるか: (y,z)は u、(ax,y)は v
        if k == 0:
            ws_s, hs_s, ws_b, hs_b = 1.6, 0.7, 3.4, 1.5
        elif k == 1:
            ws_s, hs_s, ws_b, hs_b = 0.7, 0.7, 1.5, 1.5
        else:
            ws_s, hs_s, ws_b, hs_b = 0.75, 1.7, 1.6, 3.6
        s1, r1 = panels(u, v, g.math("MULTIPLY", PSCALE, ws_s), g.math("MULTIPLY", PSCALE, hs_s),
                        0.022, 0.37, float(k))
        s2, _ = panels(g.math("ADD", u, 0.23), g.math("ADD", v, 0.41), g.math("MULTIPLY", PSCALE, ws_b),
                       g.math("MULTIPLY", PSCALE, hs_b), 0.036, 0.0, float(k + 10))
        seam = g.math("MAXIMUM", g.math("MULTIPLY", s1, 0.55), s2)
        w = ws[k]
        seam_w = g.math("MULTIPLY", seam, w)
        rand_w = g.vm("SCALE", r1, scale=w)
        seam_t = seam_w if seam_t is None else g.math("ADD", seam_t, seam_w)
        rand_t = rand_w if rand_t is None else g.vm("ADD", rand_t, rand_w)
    rr, rg, rb = g.sep(rand_t)

    ao = g.node("ShaderNodeAmbientOcclusion", samples=16, only_local=False)
    ao.inputs["Distance"].default_value = 0.4
    cav = g.smooth(g.math("SUBTRACT", 1.0, ao.outputs["AO"]), 0.08, 0.7)
    bev = g.node("ShaderNodeBevel", samples=16)
    g.set(bev.inputs["Radius"], BEVR)
    edge = g.math("SUBTRACT", 1.0, g.vm("DOT_PRODUCT", bev.outputs["Normal"], geo.outputs["Normal"]))
    edge = g.smooth(edge, 0.002, 0.05)
    n_small = g.node("ShaderNodeTexNoise")
    n_small.inputs["Scale"].default_value = 9.0
    n_small.inputs["Detail"].default_value = 4.0
    g.set(n_small.inputs["Vector"], obj)
    chip = g.smooth(n_small.outputs["Fac"], 0.52, 0.62)
    wear = g.clamp01(g.math("MULTIPLY", g.math("MULTIPLY", g.math("MULTIPLY_ADD", chip, 0.75, 0.25), edge), WEAR))
    n_big = g.node("ShaderNodeTexNoise")
    n_big.inputs["Scale"].default_value = 0.5
    n_big.inputs["Detail"].default_value = 5.0
    g.set(n_big.inputs["Vector"], obj)
    # 噴射口の後ろの、すすけた汚れ(+Y の後ろほど、下側ほど)
    soot = g.smooth(y, 3.5, 6.5)
    # 風で後ろへ流れた汚れの筋(前後=yに長く伸ばしたノイズ)
    n_st = g.node("ShaderNodeTexNoise")
    n_st.inputs["Scale"].default_value = 1.6
    n_st.inputs["Detail"].default_value = 4.0
    g.set(n_st.inputs["Vector"], g.xyz(g.math("MULTIPLY", x, 7.0), g.math("MULTIPLY", y, 0.22),
                                         g.math("MULTIPLY", z, 7.0)))
    streak = g.smooth(n_st.outputs["Fac"], 0.56, 0.76)
    # しみ(まだらに曇った所)
    n_sm = g.node("ShaderNodeTexNoise")
    n_sm.inputs["Scale"].default_value = 1.3
    n_sm.inputs["Detail"].default_value = 6.0
    n_sm.inputs["Roughness"].default_value = 0.6
    g.set(n_sm.inputs["Vector"], g.vm("ADD", obj, (3.1, 7.7, 1.3)))
    smudge = g.smooth(n_sm.outputs["Fac"], 0.52, 0.72)
    # 磨いた金属の細かい筋(前後に流れる。ツヤの鋭さだけを揺らす)
    n_br = g.node("ShaderNodeTexNoise")
    n_br.inputs["Scale"].default_value = 5.0
    n_br.inputs["Detail"].default_value = 3.0
    g.set(n_br.inputs["Vector"], g.xyz(g.math("MULTIPLY", x, 9.0), g.math("MULTIPLY", y, 0.35),
                                         g.math("MULTIPLY", z, 9.0)))
    dirt = g.math("MULTIPLY_ADD", cav, 0.8, g.math("MULTIPLY", g.math("SUBTRACT", n_big.outputs["Fac"], 0.45), 0.5))
    dirt = g.math("MULTIPLY_ADD", soot, 0.25, dirt)
    dirt = g.math("MULTIPLY_ADD", streak, 0.8, dirt)
    dirt = g.math("MULTIPLY_ADD", smudge, 0.4, dirt)
    dirt = g.clamp01(g.math("MULTIPLY", g.clamp01(dirt), DIRT))

    tint = g.lerp((0.975, 0.985, 1.0), (1.015, 1.005, 0.985), rr)
    tint = g.vm("SCALE", tint, scale=g.math("MULTIPLY_ADD", rg, 0.05, 0.975))
    tint = g.lerp((1.0, 1.0, 1.0), tint, PANEL)
    c = g.vm("MULTIPLY", BASE, tint)
    # 大きな明るさのむら(パネル割りの代わり。形と関係ない線は出さない)
    c = g.vm("SCALE", c, scale=g.math("MULTIPLY_ADD", n_big.outputs["Fac"], 0.24, 0.88))
    c = g.vm("SCALE", c, scale=g.math("SUBTRACT", 1.0, g.math("MULTIPLY", dirt, 0.5)))
    c = g.lerp(c, g.vm("MULTIPLY", c, (0.93, 0.9, 0.85)), g.math("MULTIPLY", dirt, 0.5))
    seam_p = g.math("MAXIMUM", g.math("MULTIPLY", seam_t, PANEL), EXTRA)
    # 溝は凹凸(法線)で見せ、色はわずかに暗くするだけ(黒い線にすると安っぽく見えるため)
    c = g.vm("SCALE", c, scale=g.math("SUBTRACT", 1.0, g.math("MULTIPLY", seam_p, 0.18)))
    c = g.lerp(c, (0.55, 0.56, 0.58), wear)
    det_h, det_dark = build_details(g, x, y, z, ax, nx, ny, nz)
    c = g.vm("SCALE", c, scale=g.math("SUBTRACT", 1.0, g.math("MULTIPLY", det_dark, 0.2)))
    h = g.math("MULTIPLY", seam_p, -0.01)
    h = g.math("ADD", h, g.math("MULTIPLY", g.math("SUBTRACT", rb, 0.5), g.math("MULTIPLY", PANEL, 0.003)))
    h = g.math("ADD", h, det_h)
    h = g.math("ADD", h, EXTRAH)
    bump = g.node("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 1.0
    bump.inputs["Distance"].default_value = 1.0
    g.set(bump.inputs["Height"], h)
    g.set(bump.inputs["Normal"], bev.outputs["Normal"])
    # 滑らかさ(ゲームのツヤの鋭さ = 1 + R*34)。前の機体の画像と同じ考え方で、場所ごとに揺らす:
    #   磨いた筋で細かく揺れ、汚れの筋・しみ・溝では下がり(ツヤが広く鈍くなる)、塗装がはげた角では上がる
    s = g.math("MULTIPLY_ADD", g.math("SUBTRACT", n_br.outputs["Fac"], 0.5), 0.7, SMOOTH)
    s = g.math("MULTIPLY_ADD", streak, -0.4, s)
    s = g.math("MULTIPLY_ADD", smudge, -0.45, s)
    s = g.math("MULTIPLY_ADD", dirt, -0.15, s)
    s = g.math("MULTIPLY_ADD", wear, 0.4, s)
    s = g.math("MULTIPLY_ADD", seam_p, -0.5, s)
    s = g.math("MULTIPLY_ADD", det_dark, -0.6, s)
    s = g.math("MAXIMUM", g.math("MINIMUM", s, 1.0), 0.05)
    ng.links.new(c, go.inputs["Color"])
    ng.links.new(bump.outputs["Normal"], go.inputs["Normal"])
    ng.links.new(s, go.inputs["Smooth"])
    return ng


# ------------------------------------------------------------
# マテリアル(焼き付けの出力を4つ持つ)
# ------------------------------------------------------------
SURF = {}


def make_mat(name, base=None, panel=0.0, wear=0.8, dirt=0.85, pscale=1.0, emit=None, base_fn=None,
             surface=True, smooth=0.5, bevel_r=0.06, smooth_dark=None):
    """base: PNGに入るベース色。emit: 発光(影の中でも足し算で光る)。
    panel: 形と関係なく格子状に入れるパネルの継ぎ目の強さ。既定は0(なし。翼の上でマス目に見えて安っぽいため)。
           溝は形に合わせて場所を決めたもの(hull_fn の色の境目と DETAILS)だけにする。
    smooth: 滑らかさの基準(StarSparrow_MetallicSmoothness.png の R)。ゲームのシェーダーで ツヤの鋭さ = 1 + R*34。
            surface=True のときは P_Surface が汚れ・筋・溝に合わせて場所ごとに揺らす
            このシェーダーでは値が低いほどツヤが広く出る。白い所はツヤが乗っても飛ぶだけで見えないので、
            ハイライトを見せたいのは暗い所(前の機体も濃いグレーの所が0.3前後で、そこが大きく光っていた)
    smooth_dark: base_fn が返す濃いグレーの所の滑らかさ(省略時は smooth のまま)
    base_fn(g): (色, 溝) をノードで作る関数(色分けのパネルと、その境目の溝)"""
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    g = G(nt)
    base_sock, extra, dark_mask, extra_h = (base_fn(g) if base_fn is not None else (None, None, None, None))
    if surface:
        sn = g.node("ShaderNodeGroup")
        sn.node_tree = SURF["surface"]
        g.set(sn.inputs["BaseColor"], base_sock if base_sock is not None else (*base, 1.0))
        sn.inputs["Panel"].default_value = panel
        sn.inputs["Wear"].default_value = wear
        sn.inputs["Dirt"].default_value = dirt
        sn.inputs["PanelScale"].default_value = pscale
        g.set(sn.inputs["Extra"], extra if extra is not None else 0.0)
        g.set(sn.inputs["ExtraH"], extra_h if extra_h is not None else 0.0)
        sn.inputs["BevelR"].default_value = bevel_r
        if dark_mask is not None and smooth_dark is not None:
            g.set(sn.inputs["Smooth"], g.math("MULTIPLY_ADD", dark_mask, smooth_dark - smooth, smooth))
        else:
            sn.inputs["Smooth"].default_value = smooth
        col, nrm, smo = sn.outputs["Color"], sn.outputs["Normal"], sn.outputs["Smooth"]
    else:
        col = base_sock
        nrm = None
        smo = None

    oc = g.node("ShaderNodeEmission", name="OUT_COLOR")
    g.set(oc.inputs["Color"], col if col is not None else (*base, 1.0))
    osm = g.node("ShaderNodeEmission", name="OUT_SMOOTH")
    if smo is not None:
        g.set(osm.inputs["Color"], g.xyz(smo, smo, smo))
    else:
        osm.inputs["Color"].default_value = (smooth, smooth, smooth, 1.0)
    oe = g.node("ShaderNodeEmission", name="OUT_EMIT")
    oe.inputs["Color"].default_value = (*emit, 1.0) if emit is not None else (0.0, 0.0, 0.0, 1.0)
    on = g.node("ShaderNodeBsdfPrincipled", name="OUT_NORMAL")
    if nrm is not None:
        g.set(on.inputs["Normal"], nrm)
    g.set(on.inputs["Base Color"], col if col is not None else (*base, 1.0))
    on.inputs["Roughness"].default_value = 0.5
    out = g.node("ShaderNodeOutputMaterial", name="MatOut")
    nt.links.new(on.outputs[0], out.inputs["Surface"])
    m.diffuse_color = (*(base if base is not None else (0.8, 0.8, 0.82)), 1.0)
    return m


# 白と濃いグレー
# 白は少し抑える(前の機体と同じくらい)。明るすぎると、光が当たったときに1を超えて飛び、ツヤが見えない
WHITE = (0.73, 0.745, 0.77)
DARK = (0.2, 0.21, 0.23)
GLOW = (0.25, 0.78, 1.0)       # 光るライン(水色)。白く飛ばない強さ
NOZZLE_GLOW = (0.4, 0.8, 1.0)  # 噴射口の奥(明るい水色)


def hull_fn(g):
    """白い装甲の上に、濃いグレーのパネル(背骨・翼の上の内側・翼端)を塗り分ける。
    座標は機体の座標(|x|, y)。境目には溝を引く"""
    tc = g.node("ShaderNodeTexCoord")
    x, y, z = g.sep(tc.outputs["Object"])
    ax = g.math("ABSOLUTE", x)
    geo = g.node("ShaderNodeNewGeometry")
    _, _, nz = g.sep(geo.outputs["Normal"])
    top = g.smooth(nz, 0.2, 0.35)
    W = 0.012

    def half(a, b, c, e0=-W, e1=W):
        """a*|x| + b*y + c > 0 の側が1"""
        d = g.math("ADD", g.math("ADD", g.math("MULTIPLY", ax, a), g.math("MULTIPLY", y, b)), c)
        return g.smooth(d, e0, e1)

    def region(planes, top_only, e0=-W, e1=W):
        m = None
        for a, b, c in planes:
            h = half(a, b, c, e0, e1)
            m = h if m is None else g.math("MULTIPLY", m, h)
        if top_only:
            m = g.math("MULTIPLY", m, top)
        return m

    SPINE = [(-1.0, -0.02, 0.36), (0.0, 1.0, 0.7), (0.0, -1.0, 5.55)]
    # 翼の上の内側のパネル: 前縁(内側 y=2.333|x|-3.767、外側 y=1.674|x|-2.348)から0.55下がった所から。
    # 後ろは後縁の手前まで
    WINGP = [(-1.674, 1.0, 2.348 - 0.55), (-2.333, 1.0, 3.767 - 0.55), (1.0, 0.0, -1.25),
             (-1.0, 0.0, 3.35), (0.0, -1.0, 4.95)]
    masks = [
        # 背骨: キャノピーの後ろから噴射口の手前まで、細くなりながら
        region(SPINE, True),
        region(WINGP, True),
        # 翼端(上下とも)
        region([(1.0, 0.0, -3.72)], False),
    ]
    mask = masks[0]
    for m in masks[1:]:
        mask = g.math("MAXIMUM", mask, m)
    col = g.lerp(WHITE, DARK, mask)
    # 溝: 塗り分けの境目(マスクが0.5の所)
    groove = None
    for m in masks:
        e = g.math("MULTIPLY", g.math("MULTIPLY", m, g.math("SUBTRACT", 1.0, m)), 4.0)
        groove = e if groove is None else g.math("MAXIMUM", groove, e)
    # 濃いグレーのパネルは、縁が丸く盛り上がった板にする(法線だけ)。
    # このゲームの光は後ろ上から前へ当たり、カメラも後ろにいるので、ツヤが映るのは後ろ上を向いた面だけ。
    # 平らな面ではツヤが出ないので、縁の丸み(幅12cm・高さ8cm。後ろ上を向く45°前後の傾き)にツヤの帯を出す
    PW = 0.12
    pillow = g.math("MAXIMUM", region(SPINE, True, 0.0, PW), region(WINGP, True, 0.0, PW))
    height = g.math("MULTIPLY", pillow, 0.08)
    # 3つ目: 濃いグレーの所(make_mat が滑らかさを smooth_dark に切り替える)、4つ目: 盛り上がりの高さ
    return col, groove, mask, height


def build_materials():
    SURF["panels"] = build_panel_group()
    SURF["surface"] = build_surface_group(SURF["panels"])
    return {
        # 白い装甲(濃いグレーのパネルを塗り分ける)
        # 角の丸め(bevel_r)は、ゲームの距離でも縁にハイライトが見える大きさ。薄い尾翼・金属は小さめ
        # 滑らかさの基準は前の機体の画像(地が0.68前後)に合わせる
        # 濃いグレーの所(背骨・翼の上のパネル・翼端)は 0.3 にして、ツヤを広く出す
        "hull": make_mat("P_Hull", WHITE, base_fn=hull_fn, smooth=0.68, smooth_dark=0.3, bevel_r=0.12),
        # 濃いグレー(尾翼・ひれ)
        "dark": make_mat("P_Dark", DARK, smooth=0.3, bevel_r=0.07),
        # 噴射口の金属
        "metal": make_mat("P_Metal", (0.3, 0.305, 0.32), wear=0.9, dirt=0.9, pscale=0.6, smooth=0.35,
                          bevel_r=0.06),
        # 空気取り入れ口の奥(黒)
        "intake": make_mat("P_Intake", (0.035, 0.037, 0.04), surface=False, smooth=0.3),
        # キャノピー(青みのある黒。ツヤを鋭く)
        "canopy": make_mat("P_Canopy", (0.07, 0.1, 0.13), surface=False, smooth=0.85),
        # 光るライン。光る所は滑らかさを最大にして、ツヤが色にかぶって白くならないようにする
        # ベース色は暗くして、色のほとんどを発光で出す(ベース色×明るさ+発光 が1を超えて白く飛ばないように)
        "glow": make_mat("P_Glow", (0.05, 0.12, 0.15), emit=GLOW, surface=False, smooth=1.0),
        "nozzle_glow": make_mat("P_NozzleGlow", (0.06, 0.1, 0.13), emit=NOZZLE_GLOW, surface=False, smooth=1.0),
    }


# ------------------------------------------------------------
# 形を作る道具
# ------------------------------------------------------------
def ring_loft(bm, rings, cap0=True, cap1=True):
    vr = [[bm.verts.new(Vector(p)) for p in r] for r in rings]
    n = len(vr[0])
    for a, b in zip(vr[:-1], vr[1:]):
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((a[i], a[j], b[j], b[i]))
    if cap0:
        bm.faces.new(list(reversed(vr[0])))
    if cap1:
        bm.faces.new(vr[-1])
    return vr


def lerp_table(table, t):
    """table: [(t, 値...), ...](t は昇順)。t で直線補間"""
    if t <= table[0][0]:
        return table[0][1:]
    for a, b in zip(table[:-1], table[1:]):
        if t <= b[0]:
            k = (t - a[0]) / (b[0] - a[0])
            return tuple(va + (vb - va) * k for va, vb in zip(a[1:], b[1:]))
    return table[-1][1:]


# 胴の断面(右半分を上から下へ。x=1 の所が横の稜線)
BODY_HALF = [(0.0, 1.0), (0.2, 0.99), (0.38, 0.96), (0.55, 0.89), (0.7, 0.78), (0.82, 0.63), (0.9, 0.45),
             (0.96, 0.25), (1.0, 0.04), (0.95, -0.14), (0.88, -0.32), (0.75, -0.53), (0.6, -0.72), (0.43, -0.86),
             (0.26, -0.96), (0.0, -1.0)]
BODY_PROF = BODY_HALF + [(-x, z) for (x, z) in reversed(BODY_HALF[1:-1])]
# 胴の大きさ: (y, 幅の半分, 高さの半分, 中心の高さ)
BODY = [(-6.5, 0.015, 0.012, -0.02), (-6.1, 0.12, 0.09, 0.0), (-5.5, 0.27, 0.19, 0.03), (-4.7, 0.43, 0.3, 0.06),
        (-3.8, 0.58, 0.4, 0.09), (-2.8, 0.7, 0.47, 0.11), (-1.6, 0.8, 0.53, 0.12), (0.0, 0.88, 0.57, 0.12),
        (1.8, 0.92, 0.59, 0.12), (3.4, 0.9, 0.58, 0.11), (4.6, 0.84, 0.55, 0.09), (5.3, 0.76, 0.51, 0.06),
        (5.55, 0.7, 0.48, 0.05)]


def body_point(y, xn, upper=True):
    """胴の表面の点(xn = x / 幅の半分、0〜1)の高さ"""
    w, h, cz = lerp_table(BODY, y)
    pts = sorted((x, z) for (x, z) in BODY_HALF if (z >= 0.0) == upper or x == 1.0)
    z = lerp_table(pts, xn)[0]
    return w, cz + z * h


# 翼の断面(前縁からの割合, 厚さの半分の割合)
AIRFOIL = [(0.0, 0.0), (0.06, 0.3), (0.2, 0.47), (0.4, 0.5), (0.62, 0.4), (0.82, 0.22), (1.0, 0.04)]


def airfoil_ring(le, te, t, n):
    le, te, n = Vector(le), Vector(te), Vector(n).normalized()
    c = te - le
    top = [le + c * f + n * (t * k) for f, k in AIRFOIL[1:]]
    bot = [le + c * f - n * (t * k) for f, k in reversed(AIRFOIL[1:])]
    return [le] + top + bot


def surf(bm, sections):
    """sections: (前縁, 後縁, 厚さ, 厚さの向き) をつないだ翼(両端はふさぐ)"""
    ring_loft(bm, [airfoil_ring(*s) for s in sections])


# ガル翼: (|x|, 前縁y, 後縁y, 厚さ)。z は wing_z(x)
WING = [(0.5, -2.6, 5.3, 0.44), (1.25, -0.85, 5.38, 0.38), (2.0, 0.9, 5.48, 0.3), (2.8, 2.24, 5.58, 0.22),
        (3.6, 3.58, 5.68, 0.15), (4.3, 4.75, 5.8, 0.09)]
GULL_KINK = 2.0
GULL_UP = math.radians(14)
GULL_DOWN = math.radians(24)


def wing_z(x):
    if x <= GULL_KINK:
        return 0.2 + (x - 0.5) * math.tan(GULL_UP)
    return 0.2 + (GULL_KINK - 0.5) * math.tan(GULL_UP) - (x - GULL_KINK) * math.tan(GULL_DOWN)


def wing_normal(x):
    a = GULL_UP if x < GULL_KINK else (-GULL_DOWN if x > GULL_KINK else 0.0)
    return Vector((-math.sin(a), 0.0, math.cos(a)))


def wing_sections(sx):
    out = []
    for (x, yl, yt, t) in WING:
        n = wing_normal(x)
        out.append(((sx * x, yl, wing_z(x)), (sx * x, yt, wing_z(x)), t, Vector((sx * n.x, 0.0, n.z))))
    return out


def wing_surface(x, f, upper=True):
    """翼の上(下)の面の、|x|・前縁からの割合 f の点(+X側)。ゲームの翼と同じ折れ線で補間する"""
    yl, yt, t = lerp_table([(w[0], w[1], w[2], w[3]) for w in WING], x)
    k = lerp_table(AIRFOIL, f)[0]
    n = wing_normal(x)
    p = Vector((x, yl + (yt - yl) * f, wing_z(x)))
    return p + n * (t * k) * (1 if upper else -1)


def fin(bm, sx, base, root, tip, h, cant, t0, t1):
    """base: 付け根の(|x|, z)。root/tip: (前縁y, 後縁y)。h: 高さ(負なら下へ)。cant: 外への傾き(度)"""
    c = math.radians(cant)
    d = Vector((sx * math.sin(c), 0.0, math.cos(c)))
    n = Vector((math.cos(c), 0.0, -sx * math.sin(c)))
    b = Vector((sx * base[0], 0.0, base[1]))
    top = b + d * h
    surf(bm, [((b.x, root[0], b.z), (b.x, root[1], b.z), t0, n),
              ((top.x, tip[0], top.z), (top.x, tip[1], top.z), t1, n)])


def strip(bm, pts, width, side, up, depth=0.03, lift=0.012):
    """細い帯(光るライン): 中心線 pts に沿って、side 方向に幅、up 方向に lift だけ浮かせる(下は depth だけ埋める)"""
    rings = []
    for p in pts:
        p = Vector(p)
        s = Vector(side).normalized() * (width / 2)
        u = Vector(up).normalized()
        rings.append([p - s + u * lift, p + s + u * lift, p + s - u * depth, p - s - u * depth])
    ring_loft(bm, rings)


def circle(y, r, cz, seg, phase=0.0, rfn=None):
    pts = []
    for i in range(seg):
        a = 2 * math.pi * (i + phase) / seg
        rr = r if rfn is None else rfn(i, r)
        pts.append((rr * math.cos(a), y, cz + rr * math.sin(a)))
    return pts


def box(bm, center, size, rot=None):
    mat = Matrix.Translation(center)
    if rot is not None:
        mat = mat @ rot
    mat = mat @ Matrix.Diagonal((size[0], size[1], size[2], 1.0))
    bmesh.ops.create_cube(bm, size=1.0, matrix=mat)


def finish_object(coll, name, bm, mat, bevel=0.0, segs=1, smooth_angle=30.0):
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    coll.objects.link(ob)
    me.materials.append(mat)
    if bevel > 0.0:
        mod = ob.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = segs
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(40)
        mod.harden_normals = False
        dg = bpy.context.evaluated_depsgraph_get()
        new_me = bpy.data.meshes.new_from_object(ob.evaluated_get(dg))
        ob.modifiers.clear()
        old = ob.data
        ob.data = new_me
        bpy.data.meshes.remove(old)
        new_me.name = name
    # 5角形以上の面は三角形に分ける(FBXで接線を書き出すため)
    bm3 = bmesh.new()
    bm3.from_mesh(ob.data)
    ngons = [f for f in bm3.faces if len(f.verts) > 4]
    if ngons:
        bmesh.ops.triangulate(bm3, faces=ngons, quad_method="BEAUTY", ngon_method="EAR_CLIP")
    bm3.to_mesh(ob.data)
    bm3.free()
    # 曲面はなめらかに、角(smooth_angle より折れている所)はとがらせる
    for poly in ob.data.polygons:
        poly.use_smooth = True
    ob.data.set_sharp_from_angle(angle=math.radians(smooth_angle))
    return ob


class Parts:
    def __init__(self, coll):
        self.coll = coll
        self.list = []

    def add(self, fn, mat, bevel=0.0, segs=1, mirror=False, smooth_angle=30.0):
        """fn(bm, sx) で形を作る。mirror=True なら sx=+1 と -1 の両方を作る"""
        for sx in ((1, -1) if mirror else (1,)):
            bm = bmesh.new()
            fn(bm, sx)
            self.list.append(finish_object(self.coll, "Pp_%03d" % (len(self.list) + 1), bm, mat,
                                           bevel=bevel, segs=segs, smooth_angle=smooth_angle))


# ------------------------------------------------------------
# 部品
# ------------------------------------------------------------
NOZZLE_Y0 = 5.2    # 噴射口の付け根
NOZZLE_END = 6.5   # 噴射口の後ろの端(ゲームの +Z 650)
NOZZLE_CZ = 0.06
TIP_X = WING[-1][0]
COLLIDER_POS = Vector((TIP_X, WING[-1][2] - 0.05, 0.0))


def build(P, M):
    # 胴
    P.add(lambda bm, sx: ring_loft(bm, [[(x * w, y, cz + z * h) for (x, z) in BODY_PROF] for (y, w, h, cz) in BODY]),
          M["hull"], smooth_angle=34)

    # キャノピー: 胴の上に乗る、前がとがった涙型
    can_half = [(0.0, 1.0), (0.45, 0.88), (0.8, 0.55), (1.0, 0.0), (0.8, -0.6), (0.0, -0.9)]
    can_prof = can_half + [(-x, z) for (x, z) in reversed(can_half[1:-1])]
    P.add(lambda bm, sx: ring_loft(bm, [[(x * w, y, cz + z * h) for (x, z) in can_prof] for (y, w, h, cz) in [
        (-4.75, 0.03, 0.02, 0.33), (-4.2, 0.24, 0.17, 0.4), (-3.4, 0.4, 0.29, 0.47), (-2.5, 0.46, 0.34, 0.52),
        (-1.6, 0.42, 0.31, 0.55), (-0.8, 0.28, 0.2, 0.58), (-0.2, 0.06, 0.04, 0.6)]]), M["canopy"], smooth_angle=50)
    # キャノピーの枠(前と中ほどの白い細い帯)
    def canopy_frame(bm, sx):
        for y, w, h, cz in ((-3.75, 0.36, 0.25, 0.45), (-2.05, 0.46, 0.345, 0.535)):
            pts = [(x * (w + 0.015), y, cz + z * (h + 0.015)) for (x, z) in can_half[:4]]
            pts2 = [(-p[0], p[1], p[2]) for p in reversed(pts[1:])]
            line = pts2 + pts
            strip(bm, line, 0.07, (0, 1, 0), (0, 0, 1), depth=0.02, lift=0.0)
    P.add(canopy_frame, M["hull"], smooth_angle=60)

    # 肩(空気取り入れ口のふくらみ)。前の口は斜めに切り、奥は黒
    SH = [(0.0, 1.0), (0.3, 0.97), (0.55, 0.88), (0.8, 0.68), (0.95, 0.4), (1.0, 0.12), (0.97, -0.25),
          (0.8, -0.7), (0.55, -0.9), (0.3, -1.0), (-0.3, -1.0), (-0.55, -0.9), (-0.8, -0.7), (-0.97, -0.25),
          (-1.0, 0.12), (-0.95, 0.4), (-0.8, 0.68), (-0.55, 0.88), (-0.3, 0.97)]
    SH_X = 0.98
    SH_SEC = [(-2.15, 0.3, 0.3, -0.05), (-1.5, 0.4, 0.38, -0.03), (0.3, 0.46, 0.43, 0.0), (2.6, 0.46, 0.43, 0.0),
              (4.4, 0.42, 0.38, 0.0), (5.0, 0.3, 0.28, 0.01), (5.45, 0.08, 0.08, 0.02)]

    def shoulder(bm, sx):
        rings = []
        for i, (y, w, h, cz) in enumerate(SH_SEC):
            r = []
            for (x, z) in SH:
                yy = y
                if i == 0:
                    # 口を斜めに切る(上が前に出る)
                    yy = y - 0.35 * z
                r.append((sx * (SH_X + x * w), yy, cz + z * h))
            rings.append(r)
        ring_loft(bm, rings, cap0=False)
        # 口のふち(内側の壁)と奥
        inner = [(sx * (SH_X + x * w * 0.78), y - 0.35 * z + 0.02, cz + z * h * 0.78)
                 for (x, z), (y, w, h, cz) in zip(SH, [SH_SEC[0]] * len(SH))]
        outer = rings[0]
        vo = [bm.verts.new(Vector(p)) for p in outer]
        vi = [bm.verts.new(Vector(p)) for p in inner]
        n = len(vo)
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((vo[j], vo[i], vi[i], vi[j]))
    P.add(shoulder, M["hull"], mirror=True, smooth_angle=40)

    def intake_hole(bm, sx):
        (y, w, h, cz) = SH_SEC[0]
        ring = [(sx * (SH_X + x * w * 0.78), y - 0.35 * z + 0.02, cz + z * h * 0.78) for (x, z) in SH]
        deep = [(sx * (SH_X + x * w * 0.7), y + 0.45, cz + z * h * 0.7) for (x, z) in SH]
        ring_loft(bm, [ring, deep], cap0=False, cap1=True)
    P.add(intake_hole, M["intake"], mirror=True, smooth_angle=60)

    # ガル翼
    P.add(lambda bm, sx: surf(bm, wing_sections(sx)), M["hull"], mirror=True, bevel=0.012, smooth_angle=25)

    # 背中の垂直尾翼(後ろへ大きく流す)
    P.add(lambda bm, sx: fin(bm, 1, (0.0, 0.55), (1.9, 5.6), (5.0, 6.2), 2.05, 0, 0.17, 0.06),
          M["dark"], bevel=0.012, smooth_angle=30)
    # 下の小さなひれ(外へ開く)
    P.add(lambda bm, sx: fin(bm, sx, (0.5, -0.3), (3.7, 5.55), (4.9, 5.85), -0.78, -32, 0.11, 0.05),
          M["dark"], mirror=True, bevel=0.01, smooth_angle=30)

    # 噴射口: 外の筒(後ろの縁をぎざぎざにして、後ろから見たときの特徴にする)→ 内の壁 → 奥
    SEG = 32

    def nozzle(bm, sx):
        teeth = lambda i, r: r + (0.035 if i % 2 == 0 else -0.01)
        rings = [circle(NOZZLE_Y0, 0.68, NOZZLE_CZ, SEG), circle(5.75, 0.74, NOZZLE_CZ, SEG),
                 circle(6.3, 0.75, NOZZLE_CZ, SEG, rfn=teeth), circle(NOZZLE_END, 0.71, NOZZLE_CZ, SEG, rfn=teeth),
                 circle(NOZZLE_END, 0.6, NOZZLE_CZ, SEG), circle(6.05, 0.56, NOZZLE_CZ, SEG)]
        ring_loft(bm, rings, cap0=True, cap1=False)
    # ぎざぎざの縁は角をとがらせる(なめらかにすると、ぎざぎざに沿って変な陰が出る)
    P.add(nozzle, M["metal"], smooth_angle=12)
    # 噴射口の縁の段(1本の溝のように見せる細い輪)
    P.add(lambda bm, sx: ring_loft(bm, [circle(5.95, 0.752, NOZZLE_CZ, SEG), circle(6.0, 0.752, NOZZLE_CZ, SEG),
                                        circle(6.0, 0.73, NOZZLE_CZ, SEG), circle(5.95, 0.73, NOZZLE_CZ, SEG)]),
          M["dark"], smooth_angle=40)
    # 奥: 光る面と、真ん中の暗い円すい
    P.add(lambda bm, sx: ring_loft(bm, [circle(6.02, 0.565, NOZZLE_CZ, SEG), circle(6.05, 0.565, NOZZLE_CZ, SEG)]),
          M["nozzle_glow"], smooth_angle=80)
    P.add(lambda bm, sx: ring_loft(bm, [circle(6.0, 0.21, NOZZLE_CZ, 12), circle(6.3, 0.04, NOZZLE_CZ, 12)],
                                   cap0=True, cap1=True), M["metal"], smooth_angle=60)

    # 光るライン
    # 1) 背骨の両脇(上から・斜め後ろから見える)
    def spine_line(bm, sx):
        pts = []
        xn = 0.4
        for y in [-0.4, 0.0, 1.8, 3.4, 4.6, 5.1]:
            w, z = body_point(y, xn)
            pts.append((sx * w * xn, y, z))
        strip(bm, pts, 0.05, (1, 0, 0), (0, 0, 1))
    P.add(spine_line, M["glow"], mirror=True, smooth_angle=60)
    # 2) 翼の上: 折れ目から翼端へ、前縁と平行に
    def wing_line(bm, sx):
        pts = []
        xs = [2.15, 2.8, 3.6, 4.05]
        for x in xs:
            p = wing_surface(x, 0.3)
            pts.append((sx * p.x, p.y, p.z))
        n = wing_normal(3.0)
        strip(bm, pts, 0.05, (0, 1, 0), (sx * n.x, 0, n.z))
    P.add(wing_line, M["glow"], mirror=True, smooth_angle=60)
    # 3) 垂直尾翼の前縁に沿って、両面に1本
    def fin_line(bm, sx):
        # 前縁より少し後ろ(前縁: 付け根 y1.9 z0.55 → 先 y5.0 z2.6)。尾翼の表面の厚さに合わせて少し外へ
        pts = [(0.075, 2.75, 0.95), (0.06, 3.8, 1.65), (0.045, 4.85, 2.35)]
        strip(bm, [(sx * a, y, z) for a, y, z in pts], 0.05, (0, -0.55, 0.83), (sx, 0, 0), depth=0.04, lift=0.0)
    P.add(fin_line, M["glow"], mirror=True, smooth_angle=60)


# ------------------------------------------------------------
# まとめる
# ------------------------------------------------------------
def join(parts, name):
    target = parts[0]
    with bpy.context.temp_override(active_object=target, object=target,
                                   selected_objects=parts, selected_editable_objects=parts):
        bpy.ops.object.join()
    target.name = name
    target.data.name = name
    return target


def view3d_override():
    for win in bpy.context.window_manager.windows:
        for area in win.screen.areas:
            if area.type == "VIEW_3D":
                for region in area.regions:
                    if region.type == "WINDOW":
                        return dict(window=win, area=area, region=region)
    return {}


def make_uvs(ob):
    """重ならないUVを1組作る(テクスチャ1枚に焼き付けるため)"""
    ov = view3d_override()
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    with bpy.context.temp_override(**ov, active_object=ob, object=ob, selected_objects=[ob],
                                   selected_editable_objects=[ob]):
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(50), island_margin=0.0,
                                 area_weight=0.0, correct_aspect=True, scale_to_bounds=False)
        # 余白は固定の幅(6画素)。発光が隣の島ににじまないように
        bpy.ops.uv.pack_islands(margin=0.003, margin_method="ADD", rotate=True, shape_method="CONCAVE")
        bpy.ops.object.mode_set(mode="OBJECT")
    ob.select_set(False)


def make_collider(coll, name, pos, parent):
    e = bpy.data.objects.new(name, None)
    e.empty_display_type = "PLAIN_AXES"
    e.empty_display_size = 0.3
    coll.objects.link(e)
    e.parent = parent
    e.location = pos
    if e.name != name:
        raise RuntimeError("名前が %s になった(同じ名前のものが残っている)" % e.name)
    return e


def main():
    cleanup()
    coll = get_collection()
    M = build_materials()
    P = Parts(coll)
    build(P, M)
    ob = join(P.list, OBJ_NAME)
    make_uvs(ob)

    # 翼端のフレーム: 翼端の後ろの、下の面(一番海面に近づく所)
    tip_z = wing_z(TIP_X) - 0.04
    l = make_collider(coll, COLLIDER_L, (TIP_X, COLLIDER_POS.y, tip_z), ob)
    r = make_collider(coll, COLLIDER_R, (-TIP_X, COLLIDER_POS.y, tip_z), ob)
    bpy.context.view_layer.update()

    ob.data.calc_loop_triangles()
    co = [ob.matrix_world @ v.co for v in ob.data.vertices]
    mn = [min(p[i] for p in co) for i in range(3)]
    mx = [max(p[i] for p in co) for i in range(3)]
    return {"triangles": len(ob.data.loop_triangles),
            "min": [round(a, 3) for a in mn], "max": [round(a, 3) for a in mx],
            "size": [round(b - a, 3) for a, b in zip(mn, mx)],
            COLLIDER_L: [round(a, 3) for a in l.matrix_world.translation],
            COLLIDER_R: [round(a, 3) for a in r.matrix_world.translation],
            "materials": [m.name for m in ob.data.materials]}


result = main()

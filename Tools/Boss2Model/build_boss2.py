# 2面のボスのモデルを作るスクリプト(Blender 5.x) 第2版
# 仕様は BOSS2_SPEC.md の「5. モデル」と「8. 作ったモデル」。
# 単位: Blenderの1m = ゲームの100。正面(プレイヤーを向く側)はBlenderの -Y、上は +Z。
# 何度実行しても同じものができる(前に作ったBoss2のものは消して作り直す)。
#
# 第2版の考え方(NOTES.md「2026-11-19 2面のボスのモデル第2版」):
# - 正面のシルエット: 下へとがる盾形の胴、いからせた肩当てと角、背中のトサカ、
#   ひじで折れる腕と、その下に垂れる3枚の刃、ポッドの上のV字の角と下の爪。
# - ポッド・胴の下のハッチを「あご」にそろえる(開く=攻撃か隙)。
# - 色はこのスクリプトのマテリアルで決め、bake_boss2.py が5種類の出力
#   (OUT_COLOR / OUT_SPEC / OUT_EMIT / OUT_EMIT_VENT / OUT_NORMAL)を画像に焼き付ける。
#   色の値は「PNGに入る値(ゲームのシェーダーがそのまま計算に使う値)」で書く。

import bpy
import bmesh
import math
from mathutils import Vector, Matrix

COLL_NAME = "Boss2"
ARM_NAME = "Boss2_Armature"
FPS = 60


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
    for ob in list(bpy.data.objects):
        if ob.name.startswith("Boss2") or ob.name.startswith("B2p_"):
            bpy.data.objects.remove(ob, do_unlink=True)
    names = ("Idle", "NetReady", "NetFire", "VentOpen", "VentLoop", "VentClose",
             "RocketOpen", "RocketClose", "Stagger", "ArmatureAction", "Boss2_ArmatureAction")
    for a in list(bpy.data.actions):
        if a.name.split(".")[0] in names:
            bpy.data.actions.remove(a)
    for m in list(bpy.data.materials):
        if m.name.startswith("B2_"):
            bpy.data.materials.remove(m)
    for g in list(bpy.data.node_groups):
        if g.name.startswith("B2_"):
            bpy.data.node_groups.remove(g)
    for coll_data in (bpy.data.meshes, bpy.data.armatures, bpy.data.actions):
        for d in list(coll_data):
            if d.users == 0:
                coll_data.remove(d)


def get_collection():
    coll = bpy.data.collections.new(COLL_NAME)
    bpy.context.scene.collection.children.link(coll)
    return coll


# ------------------------------------------------------------
# ノードを組む道具
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
        """ベクトル(色)の a + (b - a) * f"""
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
    """面の上の2次元の座標(U,V)から、長方形のパネルの継ぎ目・パネルごとの乱数・ボルトを作る"""
    F = "NodeSocketFloat"
    ng, gi, go = new_group("B2_Panels2D",
                           [("U", F), ("V", F), ("W", F), ("H", F), ("Mortar", F), ("Offset", F), ("Seed", F)],
                           [("Seam", F), ("Rand", "NodeSocketColor"), ("Bolt", F)])
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
    seam = g.smooth(d, half, g.math("ADD", half, 0.012), 1.0, 0.0)
    wn = g.node("ShaderNodeTexWhiteNoise", noise_dimensions="3D")
    g.set(wn.inputs["Vector"], g.xyz(col, row, SEED))
    # ボルト: 横の継ぎ目の両側に、内側へ少し入った所に並べる
    S = 0.34
    bx = g.math("MULTIPLY", g.math("SUBTRACT", g.math("FRACT", g.math("DIVIDE", u2, S)), 0.5), S)
    by = g.math("SUBTRACT", dv, 0.09)
    dist = g.math("SQRT", g.math("ADD", g.math("MULTIPLY", bx, bx), g.math("MULTIPLY", by, by)))
    bolt = g.smooth(dist, 0.022, 0.034, 1.0, 0.0)
    bolt = g.math("MULTIPLY", bolt, g.smooth(du, 0.06, 0.12))
    ng.links.new(seam, go.inputs["Seam"])
    ng.links.new(wn.outputs["Color"], go.inputs["Rand"])
    ng.links.new(bolt, go.inputs["Bolt"])
    return ng


def build_surface_group(panel_ng):
    """金属の表面: パネル割り(3方向から投影)・汚れ(AO)・雨だれ・角の塗装はがれ・凹凸"""
    F = "NodeSocketFloat"
    ng, gi, go = new_group("B2_Surface",
                           [("BaseColor", "NodeSocketColor"), ("Spec", F), ("Panel", F), ("Wear", F),
                            ("Dirt", F), ("PanelScale", F)],
                           [("Color", "NodeSocketColor"), ("Spec", F), ("Normal", "NodeSocketVector")])
    g = G(ng)
    BASE, SPEC, PANEL, WEAR, DIRT, PSCALE = (gi.outputs[i] for i in range(6))
    tc = g.node("ShaderNodeTexCoord")
    obj = tc.outputs["Object"]
    x, y, z = g.sep(obj)
    geo = g.node("ShaderNodeNewGeometry")
    nx, ny, nz = g.sep(geo.outputs["Normal"])
    ws = [g.math("POWER", g.math("ABSOLUTE", c), 6.0) for c in (nx, ny, nz)]
    wsum = g.math("ADD", g.math("ADD", ws[0], ws[1]), ws[2])
    ws = [g.math("DIVIDE", w, wsum) for w in ws]

    def panels(u, v, w, h, m, off, seed):
        n = g.node("ShaderNodeGroup")
        n.node_tree = panel_ng
        for i, val in enumerate((u, v, w, h, m, off, seed)):
            g.set(n.inputs[i], val)
        return n.outputs["Seam"], n.outputs["Rand"], n.outputs["Bolt"]

    # 小さいパネル(細い継ぎ目)と大きいパネル(太い継ぎ目)。ずらしを半分にしない(レンガに見えないように)
    ws_small = g.math("MULTIPLY", PSCALE, 2.3)
    hs_small = g.math("MULTIPLY", PSCALE, 1.35)
    ws_big = g.math("MULTIPLY", PSCALE, 5.3)
    hs_big = g.math("MULTIPLY", PSCALE, 3.2)
    seam_t = None
    rand_t = None
    bolt_t = None
    for k, (u, v) in enumerate(((y, z), (x, z), (x, y))):
        s1, r1, b1 = panels(u, v, ws_small, hs_small, 0.03, 0.31, float(k))
        s2, _, _ = panels(g.math("ADD", u, 0.37), g.math("ADD", v, 0.21), ws_big, hs_big, 0.055, 0.57, float(k + 10))
        seam = g.math("MAXIMUM", g.math("MULTIPLY", s1, 0.6), s2)
        bolt = g.math("MULTIPLY", b1, g.math("SUBTRACT", 1.0, s2))
        w = ws[k]
        seam_w = g.math("MULTIPLY", seam, w)
        bolt_w = g.math("MULTIPLY", bolt, w)
        rand_w = g.vm("SCALE", r1, scale=w)
        seam_t = seam_w if seam_t is None else g.math("ADD", seam_t, seam_w)
        bolt_t = bolt_w if bolt_t is None else g.math("ADD", bolt_t, bolt_w)
        rand_t = rand_w if rand_t is None else g.vm("ADD", rand_t, rand_w)
    rr, rg, rb = g.sep(rand_t)

    # すき間の汚れ(AO)
    ao = g.node("ShaderNodeAmbientOcclusion", samples=16, only_local=False)
    ao.inputs["Distance"].default_value = 0.7
    cav = g.smooth(g.math("SUBTRACT", 1.0, ao.outputs["AO"]), 0.05, 0.6)
    # 角(Bevelの法線と面の法線の差)
    bev = g.node("ShaderNodeBevel", samples=8)
    bev.inputs["Radius"].default_value = 0.045
    edge = g.math("SUBTRACT", 1.0, g.vm("DOT_PRODUCT", bev.outputs["Normal"], geo.outputs["Normal"]))
    edge = g.smooth(edge, 0.002, 0.05)
    n_small = g.node("ShaderNodeTexNoise")
    n_small.inputs["Scale"].default_value = 7.0
    n_small.inputs["Detail"].default_value = 4.0
    g.set(n_small.inputs["Vector"], obj)
    chip = g.smooth(n_small.outputs["Fac"], 0.5, 0.62)
    wear = g.math("MULTIPLY", g.math("MULTIPLY_ADD", chip, 0.75, 0.25), edge)
    wear = g.clamp01(g.math("MULTIPLY", wear, WEAR))
    n_big = g.node("ShaderNodeTexNoise")
    n_big.inputs["Scale"].default_value = 0.35
    n_big.inputs["Detail"].default_value = 6.0
    g.set(n_big.inputs["Vector"], obj)
    # 雨だれ: 縦に引き伸ばしたノイズ
    n_st = g.node("ShaderNodeTexNoise")
    n_st.inputs["Scale"].default_value = 2.2
    n_st.inputs["Detail"].default_value = 3.0
    g.set(n_st.inputs["Vector"], g.xyz(g.math("MULTIPLY", x, 1.6), g.math("MULTIPLY", y, 1.6),
                                         g.math("MULTIPLY", z, 0.12)))
    streak = g.smooth(n_st.outputs["Fac"], 0.5, 0.78)
    dirt = g.math("MULTIPLY_ADD", cav, 0.75, g.math("MULTIPLY", streak, 0.35))
    dirt = g.math("ADD", dirt, g.math("MULTIPLY", g.math("SUBTRACT", n_big.outputs["Fac"], 0.42), 0.6))
    dirt = g.clamp01(g.math("MULTIPLY", g.clamp01(dirt), DIRT))

    # 色: パネルごとに明るさと色相を少しずらす(寒色〜暖色)
    tint = g.lerp((0.955, 0.97, 1.0), (1.035, 1.01, 0.965), rr)
    tint = g.vm("SCALE", tint, scale=g.math("MULTIPLY_ADD", rg, 0.09, 0.955))
    tint = g.lerp((1.0, 1.0, 1.0), tint, PANEL)
    c = g.vm("MULTIPLY", BASE, tint)
    c = g.vm("SCALE", c, scale=g.math("SUBTRACT", 1.0, g.math("MULTIPLY", dirt, 0.45)))
    c = g.lerp(c, g.vm("MULTIPLY", c, (0.9, 0.8, 0.66)), g.math("MULTIPLY", dirt, 0.6))
    seam_p = g.math("MULTIPLY", seam_t, PANEL)
    bolt_p = g.math("MULTIPLY", bolt_t, PANEL)
    c = g.vm("SCALE", c, scale=g.math("SUBTRACT", 1.0, g.math("MULTIPLY", seam_p, 0.5)))
    c = g.lerp(c, g.vm("ADD", g.vm("SCALE", c, scale=1.45), (0.02, 0.02, 0.02)), bolt_p)
    c = g.lerp(c, (0.5, 0.5, 0.52), wear)
    # ツヤ(ベース色のアルファに入る): 汚れと継ぎ目で下げ、角と ボルトで上げる
    s = g.math("MULTIPLY", SPEC, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", dirt, 0.5)))
    s = g.math("MULTIPLY", s, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", seam_p, 0.7)))
    s = g.math("MULTIPLY_ADD", wear, 0.35, s)
    s = g.clamp01(g.math("MULTIPLY_ADD", bolt_p, 0.12, s))
    # 凹凸: 継ぎ目は溝、ボルトは出っ張り、パネルごとに高さを少しずらす
    h = g.math("MULTIPLY", seam_p, -0.012)
    h = g.math("MULTIPLY_ADD", bolt_p, 0.006, h)
    h = g.math("ADD", h, g.math("MULTIPLY", g.math("SUBTRACT", rb, 0.5), g.math("MULTIPLY", PANEL, 0.004)))
    bump = g.node("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 1.0
    bump.inputs["Distance"].default_value = 1.0
    g.set(bump.inputs["Height"], h)
    g.set(bump.inputs["Normal"], bev.outputs["Normal"])
    ng.links.new(c, go.inputs["Color"])
    ng.links.new(s, go.inputs["Spec"])
    ng.links.new(bump.outputs["Normal"], go.inputs["Normal"])
    return ng


# ------------------------------------------------------------
# マテリアル(焼き付けの出力を5つ持つ)
# ------------------------------------------------------------
SURF = {}


def make_mat(name, base=None, spec=0.3, panel=1.0, wear=1.0, dirt=1.0, pscale=1.0,
             emit=None, emit_vent=None, base_fn=None, emit_fn=None, surface=True, smooth=0.4):
    """base: PNGに入るベース色。spec: ツヤ(アルファ。SPEC_IN_ALPHA のときだけ使う)。emit: 通常の発光。emit_vent: 放熱中の発光(省略時は emit)
    smooth: 滑らかさ(Boss2_Metallic.png の R)。ゲームのシェーダーで ツヤ = dot^(R*34+1)。
            このシェーダーは滑らかさが低いほどツヤが広く明るく出るので、白く飛ばないよう高めにする
    base_fn(g) / emit_fn(g): ノードで色を作る関数(縞模様・レンズのグラデーション)"""
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    g = G(nt)
    base_sock = base_fn(g) if base_fn is not None else None
    if surface:
        sn = g.node("ShaderNodeGroup")
        sn.node_tree = SURF["surface"]
        g.set(sn.inputs["BaseColor"], base_sock if base_sock is not None else (*base, 1.0))
        sn.inputs["Spec"].default_value = spec
        sn.inputs["Panel"].default_value = panel
        sn.inputs["Wear"].default_value = wear
        sn.inputs["Dirt"].default_value = dirt
        sn.inputs["PanelScale"].default_value = pscale
        col, spc, nrm = sn.outputs["Color"], sn.outputs["Spec"], sn.outputs["Normal"]
    else:
        col = base_sock if base_sock is not None else None
        spc, nrm = None, None

    oc = g.node("ShaderNodeEmission", name="OUT_COLOR")
    g.set(oc.inputs["Color"], col if col is not None else (*base, 1.0))
    os_ = g.node("ShaderNodeEmission", name="OUT_SPEC")
    if spc is not None:
        g.set(os_.inputs["Color"], g.xyz(spc, spc, spc))
    else:
        os_.inputs["Color"].default_value = (spec, spec, spec, 1.0)
    osm = g.node("ShaderNodeEmission", name="OUT_SMOOTH")
    osm.inputs["Color"].default_value = (smooth, smooth, smooth, 1.0)
    emit_sock = emit_fn(g) if emit_fn is not None else None
    oe = g.node("ShaderNodeEmission", name="OUT_EMIT")
    ov = g.node("ShaderNodeEmission", name="OUT_EMIT_VENT")
    black = (0.0, 0.0, 0.0, 1.0)
    if emit_sock is not None:
        g.set(oe.inputs["Color"], emit_sock)
        g.set(ov.inputs["Color"], emit_sock)
    else:
        oe.inputs["Color"].default_value = (*emit, 1.0) if emit is not None else black
        ev = emit_vent if emit_vent is not None else emit
        ov.inputs["Color"].default_value = (*ev, 1.0) if ev is not None else black
    on = g.node("ShaderNodeBsdfPrincipled", name="OUT_NORMAL")
    if nrm is not None:
        g.set(on.inputs["Normal"], nrm)
    g.set(on.inputs["Base Color"], col if col is not None else (*base, 1.0))
    on.inputs["Roughness"].default_value = 0.5
    out = g.node("ShaderNodeOutputMaterial", name="MatOut")
    nt.links.new(on.outputs[0], out.inputs["Surface"])
    m.diffuse_color = (*(base if base is not None else (0.2, 0.2, 0.2)), 1.0)
    return m


def hazard_fn(g):
    """黄と黒の斜めの縞(ハッチの前の縁)"""
    tc = g.node("ShaderNodeTexCoord")
    x, y, z = g.sep(tc.outputs["Object"])
    t = g.math("FRACT", g.math("DIVIDE", g.math("ADD", x, z), 0.42))
    f = g.smooth(t, 0.47, 0.53)
    return g.lerp((0.78, 0.6, 0.1), (0.06, 0.055, 0.05), f)


def lens_fn(center, rim_col, core_col):
    """発射口のレンズ: 中心が白く、外が色。細い同心円の筋"""
    def fn(g):
        tc = g.node("ShaderNodeTexCoord")
        d = g.vm("DISTANCE", tc.outputs["Object"], center)
        f = g.smooth(d, 0.12, 0.62, 1.0, 0.0)
        rings = g.math("MULTIPLY_ADD", g.math("SINE", g.math("MULTIPLY", d, 46.0)), 0.06, 0.94)
        c = g.lerp(rim_col, core_col, f)
        return g.vm("SCALE", c, scale=rings)
    return fn


def build_materials():
    SURF["panels"] = build_panel_group()
    SURF["surface"] = build_surface_group(SURF["panels"])
    lens_l = Vector((-POD_X, -3.93, POD_Z))
    lens_r = Vector((POD_X, -3.93, POD_Z))
    glow_l = (0.62, 0.3, 0.95)
    glow_r = (0.2, 0.72, 0.95)
    return {
        # 主装甲: 青みのある暗い灰色(曇り空 168,174,183 より十分暗い)
        "hull": make_mat("B2_Hull", (0.2, 0.215, 0.235), spec=0.3, smooth=0.42),
        # 骨組み・関節・へこみ: 暖かみのある黒
        "dark": make_mat("B2_Dark", (0.1, 0.095, 0.092), spec=0.2, panel=0.6, smooth=0.35),
        # 肩当て・ポッドの装甲・眉: 1段明るい灰色
        "accent": make_mat("B2_Accent", (0.34, 0.35, 0.365), spec=0.32, smooth=0.45),
        # 縁取り: 鈍い真鍮色
        "brass": make_mat("B2_Brass", (0.42, 0.33, 0.17), spec=0.55, panel=0.0, wear=0.6, smooth=0.55),
        "hazard": make_mat("B2_Hazard", spec=0.25, panel=0.0, base_fn=hazard_fn, smooth=0.4),
        "scorch": make_mat("B2_Scorch", (0.05, 0.045, 0.042), spec=0.1, panel=0.0, wear=0.3, smooth=0.3),
        # ポッドの装甲の下(放熱口の周り)。焼けた金属の色
        "core": make_mat("B2_Core", (0.13, 0.105, 0.115), spec=0.25, panel=0.4, pscale=0.5, smooth=0.35),
        # 放熱口: 通常の発光では光らず、放熱中だけ赤く光る
        # 光る所は滑らかさを最大にして、ツヤが色にかぶって白くならないようにする
        "vent": make_mat("B2_Vent", (0.2, 0.05, 0.04), spec=0.1, emit=(0.0, 0.0, 0.0),
                         emit_vent=(1.0, 0.13, 0.05), surface=False, smooth=1.0),
        "vent_hot": make_mat("B2_VentHot", (0.25, 0.07, 0.04), spec=0.1, emit=(0.0, 0.0, 0.0),
                             emit_vent=(1.0, 0.42, 0.16), surface=False, smooth=1.0),
        # 光る所。左上ポッド=紫、右上ポッド=水色。白く飛ばない強さ
        "glow_l": make_mat("B2_GlowL", (0.22, 0.1, 0.32), spec=0.05, emit=glow_l, surface=False, smooth=1.0),
        "glow_r": make_mat("B2_GlowR", (0.08, 0.25, 0.32), spec=0.05, emit=glow_r, surface=False, smooth=1.0),
        "lens_l": make_mat("B2_LensL", (0.22, 0.1, 0.32), spec=0.05, surface=False, smooth=1.0,
                           emit_fn=lens_fn(lens_l, (0.58, 0.22, 0.95), (1.0, 0.9, 1.0))),
        "lens_r": make_mat("B2_LensR", (0.08, 0.25, 0.32), spec=0.05, surface=False, smooth=1.0,
                           emit_fn=lens_fn(lens_r, (0.12, 0.65, 0.95), (0.88, 1.0, 1.0))),
        # 目: 青白く(赤は使わない)。白く飛ばない強さ
        "eye": make_mat("B2_Eye", (0.12, 0.13, 0.15), spec=0.0, emit=(0.62, 0.76, 0.86), surface=False, smooth=1.0),
        "engine": make_mat("B2_Engine", (0.15, 0.25, 0.32), spec=0.05, emit=(0.4, 0.68, 0.9), surface=False,
                           smooth=1.0),
        # 壊れたポッドの中: 赤を使わない(放熱口=弱点の合図と混ぜない)。弱い水色の火花
        "spark": make_mat("B2_Spark", (0.08, 0.12, 0.14), spec=0.05, emit=(0.22, 0.5, 0.6), surface=False,
                          smooth=1.0),
    }


# ------------------------------------------------------------
# 形を作る道具
# ------------------------------------------------------------
def octagon(w, h, c):
    """幅±w・高さ±h・角の面取りcの八角形"""
    return [(-w, -h + c), (-w + c, -h), (w - c, -h), (w, -h + c),
            (w, h - c), (w - c, h), (-w + c, h), (-w, h - c)]


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


def rings_y(profile, sections):
    """sections: (y, sx, sz, cx, cz)。点 = (cx + x*sx, y, cz + z*sz)"""
    return [[(cx + x * sx, y, cz + z * sz) for (x, z) in profile] for (y, sx, sz, cx, cz) in sections]


def oct_rings_y(cx, cz, sections):
    """sections: (y, w, h, c)"""
    return [[(cx + u, y, cz + v) for (u, v) in octagon(w, h, c)] for (y, w, h, c) in sections]


def plate(bm, pts, t, origin, u, v):
    """平面の多角形 pts(u,v) を厚さ t で押し出す(origin を中心に両側へ t/2)"""
    origin, u, v = Vector(origin), Vector(u).normalized(), Vector(v).normalized()
    n = u.cross(v).normalized()
    r0 = [origin + u * a + v * b - n * (t / 2) for a, b in pts]
    r1 = [origin + u * a + v * b + n * (t / 2) for a, b in pts]
    ring_loft(bm, [r0, r1])


def frame_of(d, up_hint=(0.0, 0.0, 1.0)):
    d = Vector(d).normalized()
    up = Vector(up_hint)
    if abs(d.dot(up)) > 0.95:
        up = Vector((0.0, 1.0, 0.0))
    s = up.cross(d).normalized()
    u = d.cross(s).normalized()
    return s, u


def taper(bm, pts, up_hint=(0.0, 0.0, 1.0), chamfer=0.3):
    """pts: [(中心, 幅の半分, 高さの半分), ...] をつないだ先細りの柱(角は面取り)"""
    rings = []
    for i, (p, w, h) in enumerate(pts):
        p = Vector(p)
        if i < len(pts) - 1:
            d = Vector(pts[i + 1][0]) - p
        else:
            d = p - Vector(pts[i - 1][0])
        s, u = frame_of(d, up_hint)
        c = min(w, h) * chamfer
        rings.append([p + s * a + u * b for (a, b) in octagon(w, h, c)])
    ring_loft(bm, rings)


def cylinder(bm, center, radius, depth, axis="Y", segments=16, radius2=None):
    if radius2 is None:
        radius2 = radius
    rot = Matrix.Identity(4)
    if axis == "Y":
        rot = Matrix.Rotation(math.radians(90), 4, "X")
    elif axis == "X":
        rot = Matrix.Rotation(math.radians(90), 4, "Y")
    mat = Matrix.Translation(center) @ rot
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=radius, radius2=radius2, depth=depth, matrix=mat)


def cyl_between(bm, p0, p1, r, segments=12, r1=None):
    p0, p1 = Vector(p0), Vector(p1)
    d = p1 - p0
    q = d.normalized().to_track_quat("Z", "Y")
    mat = Matrix.Translation((p0 + p1) / 2) @ q.to_matrix().to_4x4()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=r, radius2=(r1 if r1 is not None else r), depth=d.length, matrix=mat)


def box(bm, center, size, rot=None):
    mat = Matrix.Translation(center)
    if rot is not None:
        mat = mat @ rot
    mat = mat @ Matrix.Diagonal((size[0], size[1], size[2], 1.0))
    bmesh.ops.create_cube(bm, size=1.0, matrix=mat)


MIRROR_X = Matrix.Scale(-1.0, 4, (1.0, 0.0, 0.0))


def finish_object(coll, name, bm, mat, bone, bevel=0.0, segs=1, transform=None):
    if transform is not None:
        bmesh.ops.transform(bm, matrix=transform, verts=bm.verts)
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
        mod.angle_limit = math.radians(32)
        mod.harden_normals = False
        dg = bpy.context.evaluated_depsgraph_get()
        ev = ob.evaluated_get(dg)
        new_me = bpy.data.meshes.new_from_object(ev)
        ob.modifiers.clear()
        old = ob.data
        ob.data = new_me
        bpy.data.meshes.remove(old)
        new_me.name = name
    # 5角形以上の面は三角形に分ける(FBXで接線を書き出すため。へこんだ多角形もあるので耳切り法)
    bm3 = bmesh.new()
    bm3.from_mesh(ob.data)
    ngons = [f for f in bm3.faces if len(f.verts) > 4]
    if ngons:
        bmesh.ops.triangulate(bm3, faces=ngons, quad_method="BEAUTY", ngon_method="EAR_CLIP")
    bm3.to_mesh(ob.data)
    bm3.free()
    for poly in ob.data.polygons:
        poly.use_smooth = False
    vg = ob.vertex_groups.new(name=bone)
    vg.add(list(range(len(ob.data.vertices))), 1.0, "REPLACE")
    return ob


class Parts:
    """部品を作ってためておく。right=True の部品は左右に作る(左は X を反転)"""
    def __init__(self, coll):
        self.coll = coll
        self.list = []
        self.count = 0

    def add(self, fn, mat, bone, bevel=0.0, segs=1, mirror_bone=None):
        """fn(bm) で形を作る。mirror_bone があれば、X を反転したものを mirror_bone に作る"""
        for side_bone, tr in ((bone, None), (mirror_bone, MIRROR_X)):
            if side_bone is None:
                continue
            bm = bmesh.new()
            fn(bm)
            self.count += 1
            self.list.append(finish_object(self.coll, "B2p_%03d" % self.count, bm, mat, side_bone,
                                           bevel=bevel, segs=segs, transform=tr))


# ------------------------------------------------------------
# 部品の位置(Blenderの座標、m)
# ------------------------------------------------------------
POD_X = 12.6       # ポッドの中心の左右(ゲームで ±1260)
POD_Z = 4.4        # ポッドの中心の高さ(ゲームで +440)
HINGE_Y = 2.3      # 放熱の装甲の付け根(後ろ側)
ARMOR_W = 1.92     # 放熱の装甲の外側の半径
ARMOR_T = 0.16     # 放熱の装甲の厚さ
SHOULDER = Vector((4.2, 0.5, 1.7))
ELBOW = Vector((8.4, 1.4, 2.5))
WRIST = Vector((11.4, 2.9, 4.2))
HATCH_HINGE = Vector((0.0, -1.9, -3.3))
ROCKET_X = (-1.08, -0.36, 0.36, 1.08)
ROCKET_Z = -2.75

# 胴の断面(正面から見た形。下へとがる盾形)
TORSO_PROFILE = [(0.0, 2.75), (1.1, 2.55), (3.3, 2.05), (4.3, 1.1), (4.15, -0.4), (2.9, -1.9),
                 (1.2, -2.6), (0.0, -2.75), (-1.2, -2.6), (-2.9, -1.9), (-4.15, -0.4), (-4.3, 1.1),
                 (-3.3, 2.05), (-1.1, 2.55)]
HEAD_PROFILE = [(0.0, 1.55), (1.45, 1.38), (2.45, 0.72), (2.6, -0.3), (1.75, -1.45), (0.65, -2.1),
                (0.0, -2.2), (-0.65, -2.1), (-1.75, -1.45), (-2.6, -0.3), (-2.45, 0.72), (-1.45, 1.38)]


def build_body(P, M):
    B = "Body"
    # 胴
    P.add(lambda bm: ring_loft(bm, rings_y(TORSO_PROFILE, [
        (-4.6, 0.8, 0.8, 0.0, -0.1), (-3.4, 0.97, 0.95, 0.0, 0.0), (2.8, 1.0, 1.0, 0.0, 0.0),
        (5.6, 0.86, 0.86, 0.0, 0.1), (7.3, 0.62, 0.66, 0.0, 0.15)])), M["hull"], B, bevel=0.07)
    # 頭(前の面は平らな顔の板)
    P.add(lambda bm: ring_loft(bm, rings_y(HEAD_PROFILE, [
        (-6.9, 0.8, 0.78, 0.0, -0.2), (-6.3, 0.95, 0.93, 0.0, -0.08), (-4.6, 1.0, 1.0, 0.0, 0.0),
        (-3.4, 0.98, 0.98, 0.0, 0.02)])), M["dark"], B, bevel=0.06)
    # 目: 内側が下がった、にらむ2本(顔の板から少し出す)
    def eyes(bm):
        for sx in (-1, 1):
            pts = [(0.2, 0.0), (1.62, 0.26), (1.56, 0.7), (0.56, 0.44)]
            pts = [(sx * a, b) for a, b in pts]
            if sx < 0:
                pts.reverse()
            plate(bm, pts, 0.1, (0.0, -6.93, 0.0), (1, 0, 0), (0, 0, 1))
    P.add(eyes, M["eye"], B)
    # 目の周りの暗いくぼみ(光る線を引き立てる)
    def sockets(bm):
        for sx in (-1, 1):
            pts = [(0.06, -0.12), (1.76, 0.14), (1.7, 0.84), (0.46, 0.56)]
            pts = [(sx * a, b) for a, b in pts]
            if sx < 0:
                pts.reverse()
            plate(bm, pts, 0.06, (0.0, -6.9, 0.0), (1, 0, 0), (0, 0, 1))
    P.add(sockets, M["scorch"], B)
    # 眉: 中央が下がったV字の板。顔より前へ張り出して、目に影を落とす
    def brow(bm):
        pts = [(-2.2, 1.12), (-2.0, 0.95), (0.0, 0.5), (2.0, 0.95), (2.2, 1.12), (1.95, 1.4),
               (0.0, 0.92), (-1.95, 1.4)]
        plate(bm, pts, 0.55, (0.0, -7.1, 0.0), (1, 0, 0), (0, 0, 1))
    P.add(brow, M["accent"], B, bevel=0.05)
    # 鼻すじ(眉の中心から口の上まで)
    P.add(lambda bm: plate(bm, [(-0.14, 0.55), (0.14, 0.55), (0.11, -0.4), (0.0, -0.55), (-0.11, -0.4)],
                           0.18, (0.0, -6.95, 0.0), (1, 0, 0), (0, 0, 1)), M["accent"], B, bevel=0.03)
    # 口のグリル(暗い台形と縦の羽板)
    P.add(lambda bm: plate(bm, [(-1.15, -0.62), (1.15, -0.62), (0.85, -1.3), (-0.85, -1.3)],
                           0.05, (0.0, -6.91, 0.0), (1, 0, 0), (0, 0, 1)), M["scorch"], B)
    def mouth_slats(bm):
        for k in range(7):
            x = -0.75 + k * 0.25
            top = -0.66
            bot = -1.26 + abs(x) * 0.25
            box(bm, (x, -6.97, (top + bot) / 2), (0.09, 0.12, top - bot))
    P.add(mouth_slats, M["hull"], B)
    # ほおの板(顔の下の斜めの面に重ねる)
    P.add(lambda bm: plate(bm, [(1.25, -0.55), (1.92, -0.3), (1.48, -1.15), (1.0, -1.45)], 0.12,
                           (0.0, -6.93, 0.0), (1, 0, 0), (0, 0, 1)), M["accent"], B, bevel=0.03, mirror_bone=B)
    # 牙(あごの左右から前へ突き出す)
    def fangs(bm):
        for sx in (-1, 1):
            taper(bm, [((sx * 1.05, -6.2, -1.5), 0.3, 0.36), ((sx * 0.95, -7.2, -1.72), 0.2, 0.22),
                       ((sx * 0.72, -8.0, -2.05), 0.035, 0.035)], up_hint=(0, 0, 1))
    P.add(fangs, M["dark"], B, bevel=0.03)
    # 顔の横の吸気口(暗い四角と横の羽板)
    def intake_frame(bm):
        for sx in (-1, 1):
            box(bm, (sx * 3.15, -4.62, 0.15), (1.0, 0.3, 1.7))
    P.add(intake_frame, M["dark"], B, bevel=0.05)
    def intake_slats(bm):
        for sx in (-1, 1):
            for k in range(5):
                rot = Matrix.Rotation(math.radians(-28), 4, "X")
                box(bm, (sx * 3.15, -4.86, -0.5 + k * 0.32), (0.82, 0.06, 0.2), rot)
    P.add(intake_slats, M["hull"], B)
    P.add(lambda bm: [box(bm, (sx * 3.15, -4.785, 0.15), (0.84, 0.015, 1.52)) for sx in (-1, 1)],
          M["scorch"], B)

    # 背中: 盛り上がった背骨の板と、後ろへ流れるトサカ
    P.add(lambda bm: plate(bm, [(-1.0, -3.6), (1.0, -3.6), (1.5, 0.0), (1.2, 5.6), (-1.2, 5.6), (-1.5, 0.0)],
                           0.4, (0.0, 0.0, 2.72), (1, 0, 0), (0, 1, 0)), M["accent"], B, bevel=0.07)
    P.add(lambda bm: plate(bm, [(-2.2, 2.85), (5.4, 2.85), (6.6, 3.3), (5.2, 4.75), (2.9, 4.4), (0.2, 3.45)],
                           0.38, (0.0, 0.0, 0.0), (0, 1, 0), (0, 0, 1)), M["dark"], B, bevel=0.06)
    # 後ろの小さなひれ(外へ傾ける)
    def tail_fins(bm):
        plate(bm, [(3.6, -0.2), (6.6, -0.2), (7.6, 1.7), (6.0, 1.7)], 0.22, (0.0, 0.0, 0.0), (0, 1, 0), (0, 0, 1))
    for sx in (-1, 1):
        tr = Matrix.Translation((sx * 2.3, 0.0, 1.6)) @ Matrix.Rotation(math.radians(24 * sx), 4, "Y")
        P.add(lambda bm, tr=tr: (tail_fins(bm), bmesh.ops.transform(bm, matrix=tr, verts=bm.verts)),
              M["accent"], B, bevel=0.03)

    # 横の装甲(すそ)。下が外へ開く
    def skirt(bm):
        plate(bm, [(-3.8, 0.9), (5.0, 0.9), (4.2, -1.9), (-2.6, -2.4)], 0.36, (0.0, 0.0, 0.0), (0, 1, 0), (0, 0, 1))
        bmesh.ops.transform(bm, matrix=Matrix.Translation((4.5, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-12), 4, "Y"),
                            verts=bm.verts)
    P.add(skirt, M["dark"], B, bevel=0.06, mirror_bone=B)
    def skirt_stripe(bm):
        plate(bm, [(-3.2, 0.5), (4.4, 0.5), (4.25, 0.1), (-3.1, 0.1)], 0.12, (0.0, 0.0, 0.0), (0, 1, 0), (0, 0, 1))
        plate(bm, [(-2.4, -1.6), (3.6, -1.35), (3.5, -1.7), (-2.3, -1.95)], 0.12, (0.0, 0.0, 0.0), (0, 1, 0), (0, 0, 1))
        bmesh.ops.transform(bm, matrix=Matrix.Translation((4.74, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-12), 4, "Y"),
                            verts=bm.verts)
    P.add(skirt_stripe, M["accent"], B, bevel=0.02, mirror_bone=B)
    # 横の配管
    P.add(lambda bm: cyl_between(bm, (3.95, -3.2, 1.25), (3.7, 5.2, 1.4), 0.17, 10), M["dark"], B,
          mirror_bone=B)

    # 下の浮遊用の噴射口(4つ)
    def hover(bm):
        for y in (-0.8, 4.2):
            cylinder(bm, (2.7, y, -2.45), 0.72, 0.62, axis="Z", segments=14, radius2=0.62)
    P.add(hover, M["dark"], B, bevel=0.03, mirror_bone=B)
    P.add(lambda bm: [cylinder(bm, (2.7, y, -2.78), 0.5, 0.05, axis="Z", segments=14) for y in (-0.8, 4.2)],
          M["engine"], B, mirror_bone=B)
    P.add(lambda bm: [cylinder(bm, (2.7, y, -2.62), 0.78, 0.1, axis="Z", segments=14) for y in (-0.8, 4.2)],
          M["brass"], B, mirror_bone=B)

    # 後ろのエンジン
    def engine(bm):
        ring_loft(bm, oct_rings_y(1.9, -0.15, [(6.6, 0.95, 0.95, 0.36), (7.6, 1.08, 1.08, 0.4),
                                               (8.4, 1.25, 1.25, 0.46), (8.4, 1.05, 1.05, 0.4),
                                               (8.05, 0.98, 0.98, 0.37)]))
    P.add(engine, M["dark"], B, bevel=0.04, mirror_bone=B)
    P.add(lambda bm: cylinder(bm, (1.9, 8.08, -0.15), 0.92, 0.04, axis="Y", segments=16), M["engine"], B,
          mirror_bone=B)
    P.add(lambda bm: cyl_between(bm, (0.9, 5.4, 2.3), (1.35, 8.2, 3.7), 0.05, 6), M["dark"], B, mirror_bone=B)

    # ロケットの台と4本の筒(胴の下の前)。正面から見て下がすぼまる台形
    rack = [(-1.75, -2.0), (1.75, -2.0), (1.38, -3.25), (-1.38, -3.25)]
    P.add(lambda bm: ring_loft(bm, rings_y(rack, [(-5.3, 1.0, 1.0, 0.0, 0.0), (-2.0, 1.0, 1.0, 0.0, 0.0)])),
          M["dark"], B, bevel=0.06)
    P.add(lambda bm: [cylinder(bm, (x, -4.98, ROCKET_Z), 0.3, 0.76, axis="Y", segments=12) for x in ROCKET_X],
          M["brass"], B, bevel=0.02)
    P.add(lambda bm: [cylinder(bm, (x, -5.37, ROCKET_Z), 0.22, 0.02, axis="Y", segments=12) for x in ROCKET_X],
          M["scorch"], B)
    # ロケットのハッチ(下あご): 下の板と、筒の口をふさぐ前の縁。付け根(後ろ)を軸に前が下へ開く
    H = "RocketHatch"
    P.add(lambda bm: plate(bm, [(-1.45, -5.6), (1.45, -5.6), (1.85, -4.6), (1.85, -1.9), (-1.85, -1.9),
                                (-1.85, -4.6)], 0.22, (0.0, 0.0, -3.4), (1, 0, 0), (0, 1, 0)),
          M["accent"], H, bevel=0.05)
    P.add(lambda bm: plate(bm, [(-1.85, -2.13), (1.85, -2.13), (1.45, -3.51), (-1.45, -3.51)], 0.2,
                           (0.0, -5.5, 0.0), (1, 0, 0), (0, 0, 1)), M["accent"], H, bevel=0.05)
    P.add(lambda bm: plate(bm, [(-1.5, -2.42), (1.5, -2.42), (1.24, -3.32), (-1.24, -3.32)], 0.03,
                           (0.0, -5.61, 0.0), (1, 0, 0), (0, 0, 1)), M["hazard"], H)
    P.add(lambda bm: [box(bm, (x, -3.6, -3.55), (0.22, 3.0, 0.12)) for x in (-1.1, 0.0, 1.1)], M["dark"], H,
          bevel=0.02)
    P.add(lambda bm: cylinder(bm, HATCH_HINGE, 0.2, 3.3, axis="X", segments=10), M["dark"], H)


def build_arm(P, M):
    """右の腕を作り、X を反転して左も作る(Arm_R / Arm_L)"""
    R, L = "Arm_R", "Arm_L"

    def arm_frame(d):
        # s: 腕の向きに垂直で後ろ(+Y)寄り、u: 上寄り
        dn = Vector(d).normalized()
        y = Vector((0.0, 1.0, 0.0))
        s = (y - dn * dn.dot(y)).normalized()
        u = dn.cross(s).normalized()
        if u.z < 0:
            u = -u
        return s, u

    def arm_ring(c, d, w, h):
        s, u = arm_frame(d)
        prof = [(-w, 0.15 * h), (-0.72 * w, h), (0.55 * w, h), (w, 0.35 * h), (w, -0.5 * h), (0.45 * w, -h),
                (-0.6 * w, -h), (-w, -0.45 * h)]
        return [c + s * a + u * b for a, b in prof]

    def arm_loft(p0, p1, sizes):
        def fn(bm):
            d = p1 - p0
            rings = []
            for t, w, h in sizes:
                rings.append(arm_ring(p0 + d * t, d, w, h))
            ring_loft(bm, rings)
        return fn

    sh = SHOULDER + Vector((0.15, 0.0, 0.1))
    P.add(arm_loft(sh, ELBOW, [(0.0, 1.25, 0.95), (0.55, 1.1, 0.86), (1.0, 0.95, 0.78)]), M["hull"], R,
          bevel=0.06, mirror_bone=L)
    P.add(arm_loft(ELBOW, WRIST, [(0.0, 0.88, 0.72), (0.5, 0.8, 0.66), (1.0, 0.75, 0.62)]), M["hull"], R,
          bevel=0.05, mirror_bone=L)

    # 腕の上の装甲板
    def arm_plate(p0, p1, w, h, t0, t1):
        def fn(bm):
            d = (p1 - p0)
            ln = d.length
            dn = d.normalized()
            s, u = arm_frame(d)
            org = p0 + u * (h + 0.06)
            pts = [(ln * t0, -0.95 * w), (ln * t1 - 0.3, -0.85 * w), (ln * t1, -0.3 * w), (ln * t1, 0.7 * w),
                   (ln * t0 + 0.4, 0.75 * w)]
            plate(bm, pts, 0.16, org, dn, s)
        return fn
    P.add(arm_plate(sh, ELBOW, 1.1, 0.86, 0.12, 0.88), M["accent"], R, bevel=0.04, mirror_bone=L)
    P.add(arm_plate(ELBOW, WRIST, 0.8, 0.66, 0.15, 0.85), M["accent"], R, bevel=0.04, mirror_bone=L)

    # 肩とひじの関節
    P.add(lambda bm: cylinder(bm, SHOULDER, 1.15, 2.8, axis="Y", segments=16), M["dark"], R, bevel=0.05,
          mirror_bone=L)
    P.add(lambda bm: cylinder(bm, ELBOW, 0.74, 2.1, axis="Y", segments=16), M["dark"], R, bevel=0.05,
          mirror_bone=L)
    # ひじの蓋: 細い真鍮の輪と、その内側の暗い蓋(正面から黄色い丸に見えないように)
    P.add(lambda bm: [cylinder(bm, ELBOW + Vector((0.0, dy, 0.0)), 0.5, 0.08, axis="Y", segments=16)
                      for dy in (-1.07, 1.07)], M["brass"], R, mirror_bone=L)
    P.add(lambda bm: [cylinder(bm, ELBOW + Vector((0.0, dy, 0.0)), 0.4, 0.12, axis="Y", segments=8)
                      for dy in (-1.1, 1.1)], M["dark"], R, bevel=0.03, mirror_bone=L)
    P.add(lambda bm: cylinder(bm, SHOULDER + Vector((0.0, -1.45, 0.0)), 0.72, 0.12, axis="Y", segments=16),
          M["brass"], R, mirror_bone=L)
    # 腕の下の油圧の筒(肩→ひじ)
    def piston(bm):
        a = SHOULDER + Vector((0.6, -0.2, -0.95))
        b = ELBOW + Vector((-0.6, -0.2, -0.75))
        m = a.lerp(b, 0.55)
        cyl_between(bm, a, m, 0.2, 10)
        cyl_between(bm, m - (b - a).normalized() * 0.2, b, 0.12, 10)
    P.add(piston, M["brass"], R, mirror_bone=L)
    # 手首の台(ポッドの後ろをつかむ)
    P.add(lambda bm: box(bm, WRIST + Vector((0.55, 0.0, -0.55)), (1.3, 1.3, 0.7)), M["dark"], R, bevel=0.05,
          mirror_bone=L)

    # 腕の下に垂れる3枚の刃(正面から見ると、爪が並ぶ)
    def blade(root, ln, yaw, tilt):
        def fn(bm):
            pts = [(-0.42, 0.25), (0.42, 0.25), (0.66, -0.55 * ln), (0.4, -1.0 * ln), (-0.12, -0.5 * ln)]
            rot = Matrix.Rotation(math.radians(yaw), 3, "Z") @ Matrix.Rotation(math.radians(tilt), 3, "Y")
            u = rot @ Vector((1.0, 0.0, 0.0))
            v = rot @ Vector((0.0, 0.0, 1.0))
            plate(bm, pts, 0.2, root, u, v)
        return fn

    def arm_bottom(x):
        if x <= ELBOW.x:
            t = (x - SHOULDER.x) / (ELBOW.x - SHOULDER.x)
            c = SHOULDER.lerp(ELBOW, t)
            return Vector((x, c.y, c.z - 0.75))
        t = (x - ELBOW.x) / (WRIST.x - ELBOW.x)
        c = ELBOW.lerp(WRIST, t)
        return Vector((x, c.y, c.z - 0.55))
    for x, ln, yaw, tilt in ((5.8, 1.5, 18, -14), (7.4, 2.3, 22, -24), (9.4, 2.0, 26, -32)):
        P.add(blade(arm_bottom(x), ln, yaw, tilt), M["dark"], R, bevel=0.04, mirror_bone=L)

    # 肩当て(いからせた肩)と角
    # 正面から見て、外へ斜めに上がる板(腕の向きにそろえる)。前はすぼめて、後ろは上がる
    pauldron = [(3.0, 1.55), (4.3, 1.2), (6.8, 2.45), (6.95, 3.1), (6.2, 3.45), (3.3, 2.8)]
    cx = sum(p[0] for p in pauldron) / len(pauldron)
    cz = sum(p[1] for p in pauldron) / len(pauldron)
    prof = [(x - cx, z - cz) for x, z in pauldron]
    P.add(lambda bm: ring_loft(bm, rings_y(prof, [
        (-3.0, 0.62, 0.6, cx - 0.2, cz - 0.3), (-1.9, 1.0, 1.0, cx, cz), (1.6, 1.0, 1.0, cx, cz),
        (3.3, 0.72, 0.7, cx, cz + 0.45)])), M["accent"], R, bevel=0.07, mirror_bone=L)
    # 上に重ねたもう1枚の装甲
    top = [(3.6, 2.75), (6.1, 3.35), (6.55, 3.3), (6.7, 3.0), (4.4, 2.35)]
    tcx = sum(p[0] for p in top) / len(top)
    tcz = sum(p[1] for p in top) / len(top)
    tprof = [(x - tcx, z - tcz) for x, z in top]
    P.add(lambda bm: ring_loft(bm, rings_y(tprof, [
        (-2.5, 0.8, 0.8, tcx, tcz + 0.08), (1.4, 1.0, 1.0, tcx, tcz + 0.22),
        (2.9, 0.7, 0.7, tcx, tcz + 0.55)])), M["hull"], R, bevel=0.05, mirror_bone=L)
    # 角(肩の後ろから上へ反る)
    P.add(lambda bm: taper(bm, [((5.6, 0.6, 3.3), 0.46, 0.52), ((5.95, 1.9, 4.5), 0.28, 0.3),
                                ((6.35, 3.3, 5.6), 0.03, 0.03)], up_hint=(1, 0, 0)), M["dark"], R,
          bevel=0.03, mirror_bone=L)


def armor_band(w, t, c):
    """装甲の半分の断面(U字): 外側の折れ線 + 内側の折れ線(逆順)"""
    outer = [(-w, 0.0), (-w, w - c), (-w + c, w), (w - c, w), (w, w - c), (w, 0.0)]
    wi, ci = w - t, c - t * 0.4
    inner = [(-wi, 0.0), (-wi, wi - ci), (-wi + ci, wi), (wi - ci, wi), (wi, wi - ci), (wi, 0.0)]
    return outer + list(reversed(inner))


def build_pod(P, M, side):
    """網の発射口のポッド。side: -1=左上(紫), +1=右上(水色)"""
    tag = "L" if side < 0 else "R"
    cx, cz = side * POD_X, POD_Z
    glow = M["glow_l"] if side < 0 else M["glow_r"]
    lens = M["lens_l"] if side < 0 else M["lens_r"]
    pod = "Pod_" + tag

    # 本体: 前のレンズのくぼみ → 前の覆い → 首 → 前のふち → 放熱口のへこみ → 後ろのふち → 後ろ → 噴射口
    P.add(lambda bm: ring_loft(bm, oct_rings_y(cx, cz, [
        (-3.9, 0.95, 0.95, 0.36), (-4.2, 1.02, 1.02, 0.38), (-4.2, 1.42, 1.42, 0.52),
        (-3.75, 1.58, 1.58, 0.58), (-3.15, 1.52, 1.52, 0.56), (-3.05, 1.38, 1.38, 0.5),
        (-1.75, 1.46, 1.46, 0.54), (-1.35, 1.78, 1.78, 0.6), (-1.3, 1.2, 1.2, 0.45),
        (2.0, 1.2, 1.2, 0.45), (2.05, 1.78, 1.78, 0.6), (2.45, 1.62, 1.62, 0.58),
        (3.45, 1.5, 1.5, 0.55), (3.6, 1.2, 1.2, 0.45), (4.3, 0.95, 0.95, 0.38)])), M["hull"], pod, bevel=0.05)
    P.add(lambda bm: cylinder(bm, (cx, -3.93, cz), 0.86, 0.05, axis="Y", segments=20), lens, pod)
    # 前の3本の爪(左上・右上・下)と光る先。内へ寄せすぎない(レンズに重ならないように)
    def prongs(bm, tip):
        for ang in (30, 150, 270):
            a = math.radians(ang)
            dvec = Vector((math.cos(a), 0.0, math.sin(a)))
            c0 = Vector((cx, -4.1, cz)) + dvec * 1.24
            c1 = Vector((cx, -4.7, cz)) + dvec * 1.2
            c2 = Vector((cx, -5.05, cz)) + dvec * 1.14
            up = dvec.cross(Vector((0.0, 1.0, 0.0)))
            if tip:
                taper(bm, [(c1 + (c2 - c1) * 0.15, 0.12, 0.1), (c2, 0.02, 0.02)], up_hint=up)
            else:
                taper(bm, [(c0, 0.22, 0.17), (c1, 0.14, 0.11), (c1 + (c2 - c1) * 0.2, 0.12, 0.095)], up_hint=up)
    P.add(lambda bm: prongs(bm, False), M["dark"], pod, bevel=0.02)
    P.add(lambda bm: prongs(bm, True), glow, pod)
    # 前の面の光る差し込み(爪の間)
    def inserts(bm):
        for ang in (90, 210, 330):
            a = math.radians(ang)
            dvec = Vector((math.cos(a), 0.0, math.sin(a)))
            c = Vector((cx, -4.22, cz)) + dvec * 1.22
            s = Vector((-dvec.z, 0.0, dvec.x))
            plate(bm, [(-0.22, -0.07), (0.22, -0.07), (0.16, 0.07), (-0.16, 0.07)], 0.05, c, s, dvec)
    P.add(inserts, glow, pod)
    # 首の光る輪(斜めから見えるポッドの色)
    P.add(lambda bm: ring_loft(bm, oct_rings_y(cx, cz, [(-2.75, 1.45, 1.45, 0.53), (-2.45, 1.45, 1.45, 0.53)])),
          glow, pod)
    # 後ろの真鍮の輪と噴射口の光
    P.add(lambda bm: ring_loft(bm, oct_rings_y(cx, cz, [(3.0, 1.59, 1.59, 0.58), (3.2, 1.59, 1.59, 0.58)])),
          M["brass"], pod)
    P.add(lambda bm: cylinder(bm, (cx, 4.32, cz), 0.75, 0.05, axis="Y", segments=16), glow, pod)
    # 放熱口の中: へこみの表面(焼けた色)と、赤く光る羽板(装甲の内側 1.76 には届かない)
    P.add(lambda bm: ring_loft(bm, oct_rings_y(cx, cz, [(-1.31, 1.22, 1.22, 0.46), (2.01, 1.22, 1.22, 0.46)])),
          M["core"], pod)
    def slats(bm):
        for k in range(7):
            y = -1.12 + k * 0.47
            ring_loft(bm, oct_rings_y(cx, cz, [(y, 1.6, 1.6, 0.62), (y + 0.17, 1.6, 1.6, 0.62)]))
    P.add(slats, M["vent"], pod)
    def slat_hot(bm):
        for k in range(7):
            y = -1.12 + k * 0.47 + 0.235
            ring_loft(bm, oct_rings_y(cx, cz, [(y - 0.04, 1.3, 1.3, 0.5), (y + 0.04, 1.3, 1.3, 0.5)]))
    P.add(slat_hot, M["vent_hot"], pod)

    # 放熱の装甲(上下2枚)。付け根(後ろ HINGE_Y)を軸に、前が上下へ開く(あご)
    for half, suf in ((1, "Top"), (-1, "Bottom")):
        bone = "PodArmor_%s_%s" % (tag, suf)
        band = armor_band(ARMOR_W, ARMOR_T, 0.72)

        def shell(bm, band=band, half=half):
            rings = []
            for y in (-1.6, HINGE_Y):
                rings.append([(cx + u, y, cz + half * v) for (u, v) in band])
            ring_loft(bm, rings)
        P.add(shell, M["accent"], bone, bevel=0.035)
        # 前の縁の光る線(閉じていると、レンズの周りに色の輪が見える)
        glow_band = armor_band(ARMOR_W - 0.03, 0.09, 0.7)

        def front_glow(bm, gb=glow_band, half=half):
            rings = [[(cx + u, y, cz + half * v) for (u, v) in gb] for y in (-1.64, -1.58)]
            ring_loft(bm, rings)
        P.add(front_glow, glow, bone)
        # 装甲の上の盛り上がった板
        def ridge(bm, half=half):
            pts = [(-1.35, -0.3), (-1.1, -0.52), (2.0, -0.52), (2.0, 0.52), (-1.1, 0.52), (-1.35, 0.3)]
            plate(bm, pts, 0.12, (cx, 0.0, cz + half * (ARMOR_W + 0.05)), (0, 1, 0), (1, 0, 0))
        P.add(ridge, M["hull"], bone, bevel=0.03)
        # 付け根の蝶番
        P.add(lambda bm, half=half: cylinder(bm, (cx, HINGE_Y, cz + half * (ARMOR_W - 0.05)), 0.2, 1.4,
                                             axis="X", segments=10), M["dark"], bone)
        if half > 0:
            # 上の装甲: 外へ開くV字の角(後ろへ流れる)
            def horns(bm):
                for sx in (-1, 1):
                    pts = [(-1.0, -0.1), (1.9, -0.1), (2.75, 1.35), (1.6, 1.2), (0.2, 0.45)]
                    rot = Matrix.Rotation(math.radians(24 * sx), 3, "Y")
                    u = rot @ Vector((0.0, 1.0, 0.0))
                    v = rot @ Vector((0.0, 0.0, 1.0))
                    plate(bm, pts, 0.16, (cx + sx * 0.72, 0.0, cz + ARMOR_W - 0.02), u, v)
            P.add(horns, M["dark"], bone, bevel=0.03)
        else:
            # 下の装甲: 前へ突き出る爪
            def talon(bm):
                pts = [(2.0, 0.1), (-0.8, 0.1), (-2.25, -1.35), (-0.6, -0.78), (0.9, -0.62)]
                plate(bm, pts, 0.24, (cx, 0.0, cz - ARMOR_W + 0.02), (0, 1, 0), (0, 0, 1))
            P.add(talon, M["dark"], bone, bevel=0.03)


def build_broken_pod(P, M):
    """壊れた右ポッド: 後ろ半分だけが残り、前はぎざぎざに割れている(最初は非表示)"""
    bone = "Pod_R_Broken"
    cx, cz = POD_X, POD_Z

    def shell(bm):
        n = 16
        front = []
        for i in range(n):
            a = 2 * math.pi * i / n
            r = 1.65 if i % 2 == 0 else 1.45
            y = 0.9 + (0.55 if i % 3 == 0 else (-0.35 if i % 3 == 1 else 0.15))
            front.append((cx + r * math.cos(a), y, cz + r * math.sin(a)))
        mid = [(cx + 1.7 * math.cos(2 * math.pi * i / n), 2.1, cz + 1.7 * math.sin(2 * math.pi * i / n)) for i in range(n)]
        back = [(cx + 1.5 * math.cos(2 * math.pi * i / n), 3.45, cz + 1.5 * math.sin(2 * math.pi * i / n)) for i in range(n)]
        end = [(cx + 0.95 * math.cos(2 * math.pi * i / n), 4.3, cz + 0.95 * math.sin(2 * math.pi * i / n)) for i in range(n)]
        ring_loft(bm, [front, mid, back, end], cap0=False, cap1=True)
        # 内側の壁(厚みに見せる)
        inner = [(cx + (p[0] - cx) * 0.82, p[1] + 0.05, cz + (p[2] - cz) * 0.82) for p in front]
        inner_b = [(cx + 1.25 * math.cos(2 * math.pi * i / n), 2.85, cz + 1.25 * math.sin(2 * math.pi * i / n)) for i in range(n)]
        ring_loft(bm, [inner, inner_b], cap0=False, cap1=True)
        vf = [bm.verts.new(Vector(p)) for p in front]
        vi = [bm.verts.new(Vector(p)) for p in inner]
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((vf[i], vf[j], vi[j], vi[i]))
    P.add(shell, M["scorch"], bone, bevel=0.03)
    P.add(lambda bm: ring_loft(bm, oct_rings_y(cx, cz, [(3.0, 1.56, 1.56, 0.57), (3.2, 1.56, 1.56, 0.57)])),
          M["brass"], bone)
    # めくれた装甲の破片
    def shards(bm):
        for dx, dz, ln, rz, rx in [(-1.0, 1.0, 1.8, 25, 30), (0.8, 1.2, 1.2, -18, 40), (1.3, -0.8, 1.5, 38, -28),
                                   (-0.6, -1.2, 1.0, -30, -35), (1.55, 0.3, 1.1, 70, 10)]:
            rot = Matrix.Rotation(math.radians(rz), 4, "Z") @ Matrix.Rotation(math.radians(rx), 4, "X")
            box(bm, (cx + dx, 1.1 - ln * 0.35, cz + dz), (0.55, ln, 0.16), rot)
    P.add(shards, M["accent"], bone, bevel=0.03)
    # 中: 焦げた奥の壁と、小さな火花(弱い水色。丸いレンズに見えないよう、ばらばらの小さな破片にする)と垂れた配線
    P.add(lambda bm: cylinder(bm, (cx, 2.1, cz), 1.0, 0.08, axis="Y", segments=12), M["scorch"], bone)
    def sparks(bm):
        for dx, dz, sz, rz in [(0.35, 0.25, 0.22, 20), (-0.45, -0.1, 0.16, -35), (0.05, -0.5, 0.12, 50),
                               (-0.2, 0.55, 0.1, 10), (0.6, -0.35, 0.09, -15)]:
            rot = Matrix.Rotation(math.radians(rz), 4, "Y")
            box(bm, (cx + dx, 2.02, cz + dz), (sz, 0.05, sz * 0.45), rot)
    P.add(sparks, M["spark"], bone)
    P.add(lambda bm: [cyl_between(bm, (cx + a, 1.9, cz + b), (cx + a * 1.4, 0.4, cz + b - 1.1), 0.07, 6)
                      for a, b in ((0.4, 0.2), (-0.5, -0.1), (0.1, -0.5))], M["dark"], bone)


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


BONES = [
    # (名前, 頭の位置, 親)。どのボーンも +Y 向き・ロール0(ローカル軸=ワールド軸)
    ("Body", (0.0, 0.0, 0.0), None),
    ("Arm_L", (-SHOULDER.x, SHOULDER.y, SHOULDER.z), "Body"),
    ("Arm_R", (SHOULDER.x, SHOULDER.y, SHOULDER.z), "Body"),
    ("Pod_L", (-POD_X, 0.0, POD_Z), "Arm_L"),
    ("Pod_R", (POD_X, 0.0, POD_Z), "Arm_R"),
    ("Pod_R_Broken", (POD_X, 0.0, POD_Z), "Arm_R"),
    ("PodArmor_L_Top", (-POD_X, HINGE_Y, POD_Z + ARMOR_W - 0.05), "Pod_L"),
    ("PodArmor_L_Bottom", (-POD_X, HINGE_Y, POD_Z - ARMOR_W + 0.05), "Pod_L"),
    ("PodArmor_R_Top", (POD_X, HINGE_Y, POD_Z + ARMOR_W - 0.05), "Pod_R"),
    ("PodArmor_R_Bottom", (POD_X, HINGE_Y, POD_Z - ARMOR_W + 0.05), "Pod_R"),
    ("RocketHatch", tuple(HATCH_HINGE), "Body"),
    # 目印(形なし)
    ("Muzzle_Net_L", (-POD_X, -4.25, POD_Z), "Pod_L"),
    ("Muzzle_Net_R", (POD_X, -4.25, POD_Z), "Pod_R"),
    ("Muzzle_Rocket_0", (ROCKET_X[0], -5.5, ROCKET_Z), "Body"),
    ("Muzzle_Rocket_1", (ROCKET_X[1], -5.5, ROCKET_Z), "Body"),
    ("Muzzle_Rocket_2", (ROCKET_X[2], -5.5, ROCKET_Z), "Body"),
    ("Muzzle_Rocket_3", (ROCKET_X[3], -5.5, ROCKET_Z), "Body"),
    ("Weak_L", (-POD_X, 0.3, POD_Z), "Pod_L"),
    ("Weak_R", (POD_X, 0.3, POD_Z), "Pod_R"),
]


def build_armature(coll):
    arm = bpy.data.armatures.new(ARM_NAME)
    ob = bpy.data.objects.new(ARM_NAME, arm)
    coll.objects.link(ob)
    arm.display_type = "STICK"
    ob.show_in_front = True
    bpy.context.view_layer.objects.active = ob
    ov = view3d_override()
    with bpy.context.temp_override(**ov, active_object=ob, object=ob):
        bpy.ops.object.mode_set(mode="EDIT")
        for name, head, parent in BONES:
            eb = arm.edit_bones.new(name)
            eb.head = Vector(head)
            ln = 1.0 if not name.startswith(("Muzzle", "Weak")) else 0.5
            eb.tail = Vector(head) + Vector((0.0, ln, 0.0))
            eb.roll = 0.0
            if parent:
                eb.parent = arm.edit_bones[parent]
            if name.startswith(("Muzzle", "Weak")):
                eb.use_deform = False
        bpy.ops.object.mode_set(mode="OBJECT")
    return ob


def make_uvs(objs):
    """3つのメッシュをまとめて、重ならないUVを1組作る(テクスチャ1枚に焼き付けるため)"""
    ov = view3d_override()
    for ob in bpy.context.view_layer.objects:
        ob.select_set(False)
    for ob in objs:
        ob.hide_set(False)
        ob.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    with bpy.context.temp_override(**ov, active_object=objs[0], object=objs[0],
                                   selected_objects=objs, selected_editable_objects=objs):
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        # 面取りの細い島がたくさんできるので、余白は「固定の幅を足す」(ADD)にする。
        # 既定の SCALED だと島が小さく詰められ、画像の3割しか使わなかった(第2版で6割に)
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.0,
                                 area_weight=0.0, correct_aspect=True, scale_to_bounds=False)
        # 余白を詰めすぎると(0.0015=3画素)、隣の島の発光が遠くで縮小されたときににじむので 0.003(6画素)
        bpy.ops.uv.pack_islands(margin=0.003, margin_method="ADD", rotate=True, shape_method="CONCAVE")
        bpy.ops.object.mode_set(mode="OBJECT")
    for ob in objs:
        ob.select_set(False)


def bind(mesh_ob, arm_ob):
    mesh_ob.parent = arm_ob
    mod = mesh_ob.modifiers.new("Armature", "ARMATURE")
    mod.object = arm_ob


# ------------------------------------------------------------
# 待機アニメーション(Idle): 120Fでループ
# ------------------------------------------------------------
def build_idle(arm_ob):
    scn = bpy.context.scene
    scn.render.fps = FPS
    scn.frame_start = 0
    scn.frame_end = 120
    if arm_ob.animation_data is None:
        arm_ob.animation_data_create()
    act = bpy.data.actions.new("Idle")
    arm_ob.animation_data.action = act
    pb = arm_ob.pose.bones
    for b in pb:
        b.rotation_mode = "XYZ"
    r = math.radians
    for f in range(0, 121, 5):
        t = f / 120.0 * 2.0 * math.pi
        # 本体: 上下に揺れる(±20)、少し傾く(2つの軸をずらす)
        pb["Body"].location = (0.0, 0.0, 0.20 * math.sin(t))
        pb["Body"].rotation_euler = (r(0.8) * math.sin(t + 1.2), r(1.2) * math.sin(t + 0.5), 0.0)
        # 腕: 本体より少し遅れて、左右対称にしなる
        a = r(2.2) * math.sin(t - 0.9)
        pb["Arm_L"].rotation_euler = (0.0, a, 0.0)
        pb["Arm_R"].rotation_euler = (0.0, -a, 0.0)
        # ポッド: さらに遅れて、2つの軸で少しずらして揺れる
        p1 = r(2.5) * math.sin(t - 1.6)
        p2 = r(1.5) * math.sin(2.0 * t - 0.7)
        pb["Pod_L"].rotation_euler = (p1, 0.0, p2)
        pb["Pod_R"].rotation_euler = (p1, 0.0, -p2)
        for name in ("Body", "Arm_L", "Arm_R", "Pod_L", "Pod_R"):
            pb[name].keyframe_insert("rotation_euler", frame=f, group=name)
        pb["Body"].keyframe_insert("location", frame=f, group="Body")
    return act


# ------------------------------------------------------------
# 攻撃などのアニメーション(仕様書 5-5)
# keys: [(フレーム, {ボーン: {"rot": (x,y,z)度, "loc": (x,y,z)m}}), ...]
# 書いていないボーン・値は 0(休みのポーズ)
# ------------------------------------------------------------
def build_action(arm_ob, name, length, keys):
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    arm_ob.animation_data.action = act
    pb = arm_ob.pose.bones
    bones = sorted({b for _, d in keys for b in d})
    for f, d in keys:
        for b in bones:
            v = d.get(b, {})
            rot = v.get("rot", (0.0, 0.0, 0.0))
            loc = v.get("loc", (0.0, 0.0, 0.0))
            pb[b].rotation_euler = tuple(math.radians(a) for a in rot)
            pb[b].location = loc
            pb[b].keyframe_insert("rotation_euler", frame=f, group=b)
            pb[b].keyframe_insert("location", frame=f, group=b)
    for b in bones:
        pb[b].rotation_euler = (0.0, 0.0, 0.0)
        pb[b].location = (0.0, 0.0, 0.0)
    act.frame_range = (0, length)
    return act


VENT = 40.0  # 放熱で装甲が開く角度


def vent(a):
    return {"PodArmor_L_Top": {"rot": (-a, 0, 0)}, "PodArmor_L_Bottom": {"rot": (a, 0, 0)},
            "PodArmor_R_Top": {"rot": (-a, 0, 0)}, "PodArmor_R_Bottom": {"rot": (a, 0, 0)}}


def ready_pose(k):
    """網の構え(k=1で最大)。腕を上げてポッドを前に向け、本体は少しのけぞる"""
    return {"Body": {"rot": (4 * k, 0, 0)},
            "Arm_L": {"rot": (0, 9 * k, 0)}, "Arm_R": {"rot": (0, -9 * k, 0)},
            "Pod_L": {"rot": (-7 * k, 0, 0)}, "Pod_R": {"rot": (-7 * k, 0, 0)}}


def merge(*ds):
    out = {}
    for d in ds:
        for b, v in d.items():
            out.setdefault(b, {}).update(v)
    return out


def build_attack_actions(arm_ob):
    acts = []
    # 構え 20F: 1フレーム目で大きく構える(構えは瞬時に極端に)、あとは少し震えながら保つ
    acts.append(build_action(arm_ob, "NetReady", 20, [
        (0, ready_pose(0.0)),
        (2, ready_pose(1.1)),
        (5, ready_pose(0.95)),
        (8, ready_pose(1.02)),
        (12, ready_pose(0.97)),
        (16, ready_pose(1.01)),
        (20, ready_pose(1.0)),
    ]))
    # 発射 30F: 1Fで反動(ポッドがはね上がり、本体が後ろへ)、残りでゆっくり戻る
    acts.append(build_action(arm_ob, "NetFire", 30, [
        (0, ready_pose(1.0)),
        (1, merge(ready_pose(0.6), {"Body": {"rot": (-3, 0, 0), "loc": (0, 0.7, 0)},
                                    "Pod_L": {"rot": (12, 0, 0)}, "Pod_R": {"rot": (12, 0, 0)}})),
        (6, merge(ready_pose(0.3), {"Body": {"rot": (-1.5, 0, 0), "loc": (0, 0.45, 0)},
                                    "Pod_L": {"rot": (6, 0, 0)}, "Pod_R": {"rot": (6, 0, 0)}})),
        (18, merge(ready_pose(0.05), {"Body": {"loc": (0, 0.1, 0)},
                                      "Pod_L": {"rot": (1, 0, 0)}, "Pod_R": {"rot": (1, 0, 0)}})),
        (30, ready_pose(0.0)),
    ]))
    # 放熱: 開く 10F(素早く)/開いたまま 60F(小さく震える、ループ)/閉じる 15F(最後に少し行き過ぎて止まる)
    acts.append(build_action(arm_ob, "VentOpen", 10, [
        (0, vent(0)), (3, vent(VENT * 0.75)), (7, vent(VENT * 1.06)), (10, vent(VENT)),
    ]))
    loop_keys = []
    for f in range(0, 61, 5):
        t = f / 60.0 * 2.0 * math.pi
        loop_keys.append((f, vent(VENT + 2.5 * math.sin(2.0 * t))))
    acts.append(build_action(arm_ob, "VentLoop", 60, loop_keys))
    acts.append(build_action(arm_ob, "VentClose", 15, [
        (0, vent(VENT)), (8, vent(VENT * 0.3)), (12, vent(-2.0)), (15, vent(0)),
    ]))
    # ロケットのハッチ(下あご): 開く 8F / 閉じる 12F(前が下へ開く)
    acts.append(build_action(arm_ob, "RocketOpen", 8, [
        (0, {"RocketHatch": {"rot": (0, 0, 0)}}), (5, {"RocketHatch": {"rot": (58, 0, 0)}}),
        (8, {"RocketHatch": {"rot": (55, 0, 0)}}),
    ]))
    acts.append(build_action(arm_ob, "RocketClose", 12, [
        (0, {"RocketHatch": {"rot": (55, 0, 0)}}), (9, {"RocketHatch": {"rot": (-3, 0, 0)}}),
        (12, {"RocketHatch": {"rot": (0, 0, 0)}}),
    ]))
    # 形態の切り替え 45F: 右上のポッドが壊れた衝撃で、大きく右に傾いてのけぞり、立て直す
    acts.append(build_action(arm_ob, "Stagger", 45, [
        (0, {"Body": {}}),
        (3, {"Body": {"rot": (-10, 18, 0), "loc": (0.4, 0.8, -0.8)},
             "Arm_R": {"rot": (0, -16, 0)}, "Arm_L": {"rot": (0, -6, 0)}}),
        (12, {"Body": {"rot": (-6, 12, 0), "loc": (0.3, 0.5, -0.6)},
              "Arm_R": {"rot": (0, -10, 0)}, "Arm_L": {"rot": (0, 4, 0)}}),
        (28, {"Body": {"rot": (2, -3, 0), "loc": (0, 0.1, 0.1)},
              "Arm_R": {"rot": (0, 2, 0)}, "Arm_L": {"rot": (0, -2, 0)}}),
        (45, {"Body": {}, "Arm_R": {}, "Arm_L": {}}),
    ]))
    return acts


def main():
    cleanup()
    coll = get_collection()
    M = build_materials()
    P_main = Parts(coll)
    build_body(P_main, M)
    build_arm(P_main, M)
    build_pod(P_main, M, -1)
    P_podr = Parts(coll)
    P_podr.count = 500
    build_pod(P_podr, M, 1)
    P_broken = Parts(coll)
    P_broken.count = 800
    build_broken_pod(P_broken, M)

    main_ob = join(P_main.list, "Boss2_Main")
    pod_r_ob = join(P_podr.list, "Boss2_PodR")
    broken_ob = join(P_broken.list, "Boss2_PodR_Broken")

    make_uvs([main_ob, pod_r_ob, broken_ob])

    arm_ob = build_armature(coll)
    for ob in (main_ob, pod_r_ob, broken_ob):
        bind(ob, arm_ob)
    # 壊れたポッドは最初は見えない(ゲームでも最初は非表示にする)
    broken_ob.hide_viewport = False
    broken_ob.hide_render = True
    broken_ob.hide_set(True)

    idle = build_idle(arm_ob)
    idle.use_fake_user = True
    acts = build_attack_actions(arm_ob)
    arm_ob.animation_data.action = idle

    tris = 0
    for ob in (main_ob, pod_r_ob, broken_ob):
        ob.data.calc_loop_triangles()
        tris += len(ob.data.loop_triangles)
    return {"triangles": tris,
            "actions": [a.name for a in [idle] + acts],
            "objects": [main_ob.name, pod_r_ob.name, broken_ob.name, arm_ob.name],
            "bones": [b.name for b in arm_ob.data.bones]}


result = main()

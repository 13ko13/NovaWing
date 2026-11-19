# 2面ボスの確認用の画像を撮るスクリプト(Blender 5.x)
# build_boss2.py → bake_boss2.py の後に実行する。OUT_DIR に PNG を書き出す。
#
# 見た目は「ゲームのシェーダー(LightingCommon.hlsli / LightingPS.hlsl)と同じ式」で描く:
#   色 = ベース色 × saturate(拡散 + 環境光0.5) + ツヤ × ベース色のアルファ + 発光
#   拡散 = saturate(dot(法線, -光))、ツヤ = saturate(dot(視線, reflect(光, 法線)))^(メタリックのR*34+1) × 拡散
#   (メタリックの画像を -1 で渡すと R=0 で滑らかさは最小の1。USE_METALLIC で切り替える)
#   法線マップはDirectX式(緑=テクスチャの下向き)、xyを1.5倍してから正規化
#   ゲームはテクスチャをガンマ空間のまま計算するので、画像は Non-Color で読み、表示は Raw にする
# ゲームのカメラ: 縦の画角90°、1280x720。背景は2面の曇り空の地平線の色(168,174,183)。
# 光の向き: ゲームの Game::light_direction (-0.332, -0.473, 0.816) をBlenderの座標(x, z, y)にしたもの。
#
# 変数(実行するときに globals で渡す):
#   OUT_DIR  書き出し先(リポジトリの外。スクラッチパッドなど)
#   TEX_DIR  テクスチャのフォルダ(Boss2_Base.png など)。既定は Data/Model/Boss2.fbm
#   SHOTS    撮る名前のリスト(None なら全部)

import bpy
import math
import os
from mathutils import Vector

OUT_DIR = globals().get("OUT_DIR", r"C:\temp")
TEX_DIR = globals().get("TEX_DIR", None)
SHOTS = globals().get("SHOTS", None)
# True: Boss2_Metallic.png があれば滑らかさに使う(ゲームでメタリックの画像として渡したとき)
# False: 滑らかさ1(メタリックの画像を -1 で渡したとき = 1面のボスと同じ渡し方)
USE_METALLIC = globals().get("USE_METALLIC", True)

LIGHT_DXLIB = (-0.332, -0.473, 0.816)
LIGHT = Vector((LIGHT_DXLIB[0], LIGHT_DXLIB[2], LIGHT_DXLIB[1])).normalized()
AMBIENT = 0.5
NORMAL_STRENGTH = 1.5
SKY = (168 / 255.0, 174 / 255.0, 183 / 255.0)


def default_tex_dir():
    blend_dir = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else ""
    if blend_dir:
        root = os.path.dirname(os.path.dirname(blend_dir))
        p = os.path.join(root, "Data", "Model", "Boss2.fbm")
        if os.path.isdir(p):
            return p
    return r"C:\Users\SakamotoKou\Documents\GitHub\NovaWing\Data\Model\Boss2.fbm"


_LOADED = {}


def load_image(path):
    """プレビュー用に画像を読む(1回の実行の中では同じものを使い回す)"""
    key = os.path.normcase(path)
    if key in _LOADED:
        return _LOADED[key]
    name = "Preview_" + os.path.basename(path)
    old = bpy.data.images.get(name)
    if old is not None:
        bpy.data.images.remove(old)
    img = bpy.data.images.load(path, check_existing=False)
    img.name = name
    img.colorspace_settings.name = "Non-Color"
    img.alpha_mode = "CHANNEL_PACKED"
    _LOADED[key] = img
    return img


def build_game_material(tex_dir, emission_name):
    """ゲームのシェーダーと同じ式のマテリアル(全部の面に上書きして使う)"""
    name = "PreviewGame_" + emission_name
    m = bpy.data.materials.get(name)
    if m is not None:
        bpy.data.materials.remove(m)
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    N = nt.nodes.new
    L = nt.links.new

    def vmath(op, a=None, b=None, va=None, vb=None):
        n = N("ShaderNodeVectorMath")
        n.operation = op
        if a is not None:
            L(a, n.inputs[0])
        elif va is not None:
            n.inputs[0].default_value = va
        if b is not None:
            L(b, n.inputs[1])
        elif vb is not None:
            n.inputs[1].default_value = vb
        return n

    def math_(op, a=None, b=None, va=None, vb=None, clamp=False):
        n = N("ShaderNodeMath")
        n.operation = op
        n.use_clamp = clamp
        if a is not None:
            L(a, n.inputs[0])
        elif va is not None:
            n.inputs[0].default_value = va
        if b is not None:
            L(b, n.inputs[1])
        elif vb is not None:
            n.inputs[1].default_value = vb
        return n

    uv = N("ShaderNodeUVMap")
    uv.uv_map = "UVMap"
    base = N("ShaderNodeTexImage")
    base.image = load_image(os.path.join(tex_dir, "Boss2_Base.png"))
    nrm = N("ShaderNodeTexImage")
    nrm.image = load_image(os.path.join(tex_dir, "Boss2_Normal.png"))
    emi = N("ShaderNodeTexImage")
    emi.image = load_image(os.path.join(tex_dir, emission_name + ".png"))
    for t in (base, nrm, emi):
        L(uv.outputs["UV"], t.inputs["Vector"])

    # タンジェント空間の法線(-1〜1)。xyを1.5倍
    sep = N("ShaderNodeSeparateXYZ")
    L(nrm.outputs["Color"], sep.inputs[0])
    nx = math_("MULTIPLY_ADD", sep.outputs[0], None, vb=2.0)
    nx.inputs[2].default_value = -1.0
    ny = math_("MULTIPLY_ADD", sep.outputs[1], None, vb=2.0)
    ny.inputs[2].default_value = -1.0
    nz = math_("MULTIPLY_ADD", sep.outputs[2], None, vb=2.0)
    nz.inputs[2].default_value = -1.0
    nx2 = math_("MULTIPLY", nx.outputs[0], None, vb=NORMAL_STRENGTH)
    # DirectX式: ゲームの従法線 cross(N,T) はテクスチャの下向き。Blenderの cross(N,T) は上向きなので符号を反転
    ny2 = math_("MULTIPLY", ny.outputs[0], None, vb=-NORMAL_STRENGTH)

    geo = N("ShaderNodeNewGeometry")
    tan = N("ShaderNodeTangent")
    tan.direction_type = "UV_MAP"
    tan.uv_map = "UVMap"
    nN = vmath("NORMALIZE", geo.outputs["Normal"])
    nT = vmath("NORMALIZE", tan.outputs["Tangent"])
    nB = vmath("NORMALIZE", vmath("CROSS_PRODUCT", nN.outputs[0], nT.outputs[0]).outputs[0])
    tx = vmath("SCALE", nT.outputs[0])
    L(nx2.outputs[0], tx.inputs["Scale"])
    by = vmath("SCALE", nB.outputs[0])
    L(ny2.outputs[0], by.inputs["Scale"])
    nzv = vmath("SCALE", nN.outputs[0])
    L(nz.outputs[0], nzv.inputs["Scale"])
    nsum = vmath("ADD", vmath("ADD", tx.outputs[0], by.outputs[0]).outputs[0], nzv.outputs[0])
    nf = vmath("NORMALIZE", nsum.outputs[0])

    # 拡散と環境光
    neg_l = tuple(-c for c in LIGHT)
    dif = vmath("DOT_PRODUCT", nf.outputs[0], None, vb=neg_l)
    dif_c = math_("MAXIMUM", dif.outputs["Value"], None, vb=0.0)
    dif_c = math_("MINIMUM", dif_c.outputs[0], None, vb=1.0)
    light = math_("ADD", dif_c.outputs[0], None, vb=AMBIENT, clamp=True)

    # ツヤ
    refl = vmath("REFLECT", None, nf.outputs[0], va=tuple(LIGHT))
    vd = vmath("DOT_PRODUCT", geo.outputs["Incoming"], refl.outputs[0])
    spec = math_("MAXIMUM", vd.outputs["Value"], None, vb=0.0)
    spec = math_("MINIMUM", spec.outputs[0], None, vb=1.0)
    met_path = os.path.join(tex_dir, "Boss2_Metallic.png")
    if USE_METALLIC and os.path.exists(met_path):
        # 滑らかさ = メタリックの画像のR * 34 + 1
        met = N("ShaderNodeTexImage")
        met.image = load_image(met_path)
        L(uv.outputs["UV"], met.inputs["Vector"])
        msep = N("ShaderNodeSeparateColor")
        L(met.outputs["Color"], msep.inputs[0])
        smooth = math_("MULTIPLY_ADD", msep.outputs[0], None, vb=34.0)
        smooth.inputs[2].default_value = 1.0
        spec = math_("POWER", spec.outputs[0], smooth.outputs[0])
    spec = math_("MULTIPLY", spec.outputs[0], dif_c.outputs[0])
    spec = math_("MULTIPLY", spec.outputs[0], base.outputs["Alpha"])

    lit = vmath("SCALE", base.outputs["Color"])
    L(light.outputs[0], lit.inputs["Scale"])
    spec_v = N("ShaderNodeCombineXYZ")
    for i in range(3):
        L(spec.outputs[0], spec_v.inputs[i])
    col = vmath("ADD", vmath("ADD", lit.outputs[0], spec_v.outputs[0]).outputs[0], emi.outputs["Color"])

    em = N("ShaderNodeEmission")
    em.inputs["Strength"].default_value = 1.0
    L(col.outputs[0], em.inputs["Color"])
    out = N("ShaderNodeOutputMaterial")
    L(em.outputs[0], out.inputs["Surface"])
    return m


def build_black_material():
    m = bpy.data.materials.get("PreviewBlack") or bpy.data.materials.new("PreviewBlack")
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Color"].default_value = (0, 0, 0, 1)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    nt.links.new(em.outputs[0], out.inputs["Surface"])
    return m


def setup_scene():
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE", "BLENDER_EEVEE_NEXT"):
        try:
            scn.render.engine = eng
            break
        except TypeError:
            pass
    scn.render.resolution_x = 1280
    scn.render.resolution_y = 720
    scn.render.resolution_percentage = 100
    scn.render.film_transparent = False
    scn.view_settings.view_transform = "Raw"
    scn.view_settings.look = "None"
    scn.view_settings.exposure = 0.0
    scn.view_settings.gamma = 1.0
    world = scn.world or bpy.data.worlds.new("World")
    scn.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    bg.inputs["Color"].default_value = (*SKY, 1.0)
    bg.inputs["Strength"].default_value = 1.0
    # 前のバージョンの太陽が残っていたら消す(光はマテリアルの式で計算する)
    sun = bpy.data.objects.get("PreviewSun")
    if sun is not None:
        bpy.data.objects.remove(sun, do_unlink=True)
    cam = bpy.data.objects.get("PreviewCam")
    if cam is None:
        cd = bpy.data.cameras.new("PreviewCam")
        cam = bpy.data.objects.new("PreviewCam", cd)
        scn.collection.objects.link(cam)
    cam.data.sensor_fit = "VERTICAL"
    cam.data.angle = math.radians(90)
    cam.data.clip_start = 0.1
    cam.data.clip_end = 1000
    scn.camera = cam
    return cam


def look_at(cam, pos, target, angle=90):
    cam.location = Vector(pos)
    d = Vector(target) - Vector(pos)
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    cam.data.angle = math.radians(angle)


def render(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def set_pose(arm, pose):
    """pose: {ボーン名: (rx, ry, rz)度}。書いていないボーンは休みのポーズ"""
    for pb in arm.pose.bones:
        pb.rotation_mode = "XYZ"
        r = pose.get(pb.name, (0.0, 0.0, 0.0))
        pb.rotation_euler = tuple(math.radians(a) for a in r)
        pb.location = (0.0, 0.0, 0.0)


VENT = 40.0
POSES = {
    "rest": {},
    "vent": {"PodArmor_L_Top": (-VENT, 0, 0), "PodArmor_L_Bottom": (VENT, 0, 0),
             "PodArmor_R_Top": (-VENT, 0, 0), "PodArmor_R_Bottom": (VENT, 0, 0)},
    "hatch": {"RocketHatch": (55, 0, 0)},
    "broken": {},
    "broken_vent": {"PodArmor_L_Top": (-VENT, 0, 0), "PodArmor_L_Bottom": (VENT, 0, 0)},
}

# (名前, カメラ位置, 注視点, 画角, ポーズ, 発光の画像, 黒塗り)
# ボスは原点。ゲームでは「カメラから約3000先、カメラより約150上」を想定(Blenderで 30m, 1.5m)
SHOT_LIST = [
    ("game_view", (0.0, -30.0, -1.5), (0.0, 0.0, 0.0), 90, "rest", "Boss2_Emission", False),
    ("game_view_silhouette", (0.0, -30.0, -1.5), (0.0, 0.0, 0.0), 90, "rest", "Boss2_Emission", True),
    ("game_view_vent", (0.0, -30.0, -1.5), (0.0, 0.0, 0.0), 90, "vent", "Boss2_Emission_Vent", False),
    ("game_zoom", (0.0, -30.0, -1.5), (0.0, 0.0, 1.5), 32, "rest", "Boss2_Emission", False),
    ("game_zoom_vent", (0.0, -30.0, -1.5), (0.0, 0.0, 1.5), 32, "vent", "Boss2_Emission_Vent", False),
    ("near_3q", (-14.0, -20.0, 5.0), (0.0, 0.0, 1.0), 90, "rest", "Boss2_Emission", False),
    ("near_3q_silhouette", (-14.0, -20.0, 5.0), (0.0, 0.0, 1.0), 90, "rest", "Boss2_Emission", True),
    ("near_front_low", (3.0, -15.0, -2.5), (0.0, 0.0, 1.2), 70, "rest", "Boss2_Emission", False),
    ("vent_open_near", (-20.0, -14.0, 7.0), (-12.6, 0.0, 4.4), 60, "vent", "Boss2_Emission_Vent", False),
    ("pod_close", (-17.5, -9.0, 5.0), (-12.6, -0.5, 4.4), 45, "rest", "Boss2_Emission", False),
    ("hatch_open", (4.0, -14.0, -6.0), (0.0, -3.0, -2.5), 50, "hatch", "Boss2_Emission", False),
    ("side", (-30.0, 0.0, 2.0), (0.0, 0.0, 1.0), 60, "rest", "Boss2_Emission", False),
    ("top", (0.0, -6.0, 32.0), (0.0, 0.0, 0.0), 70, "rest", "Boss2_Emission", False),
    ("back_3q", (12.0, 22.0, 9.0), (0.0, 0.0, 1.0), 70, "rest", "Boss2_Emission", False),
    # 第二形態: 右上のポッドが壊れた状態(Boss2_PodR を隠し、Boss2_PodR_Broken を出す)
    ("broken_game", (0.0, -30.0, -1.5), (0.0, 0.0, 1.5), 32, "broken", "Boss2_Emission", False),
    ("broken_near", (20.0, -12.0, 7.0), (12.6, 1.5, 4.4), 50, "broken", "Boss2_Emission", False),
    ("broken_vent", (0.0, -30.0, -1.5), (0.0, 0.0, 1.5), 32, "broken_vent", "Boss2_Emission_Vent", False),
]


def main():
    tex_dir = TEX_DIR or default_tex_dir()
    cam = setup_scene()
    mats = {}
    black = build_black_material()
    arm = bpy.data.objects.get("Boss2_Armature")
    vl = bpy.context.view_layer
    broken = bpy.data.objects.get("Boss2_PodR_Broken")
    pod_r = bpy.data.objects.get("Boss2_PodR")
    os.makedirs(OUT_DIR, exist_ok=True)
    done = []
    try:
        for name, pos, tgt, ang, pose, emi, is_black in SHOT_LIST:
            if SHOTS is not None and name not in SHOTS:
                continue
            if emi not in mats:
                mats[emi] = build_game_material(tex_dir, emi)
            vl.material_override = black if is_black else mats[emi]
            is_broken = pose.startswith("broken")
            if broken is not None:
                broken.hide_render = not is_broken
            if pod_r is not None:
                pod_r.hide_render = is_broken
            if arm is not None:
                if arm.animation_data is not None:
                    arm.animation_data.action = None
                set_pose(arm, POSES[pose])
            look_at(cam, pos, tgt, ang)
            p = os.path.join(OUT_DIR, name + ".png")
            render(p)
            done.append(p)
    finally:
        vl.material_override = None
        if broken is not None:
            broken.hide_render = True
        if pod_r is not None:
            pod_r.hide_render = False
        if arm is not None:
            set_pose(arm, {})
            idle = bpy.data.actions.get("Idle")
            if idle is not None and arm.animation_data is not None:
                arm.animation_data.action = idle
    return {"rendered": done, "tex_dir": tex_dir}


result = main()

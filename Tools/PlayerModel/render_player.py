# 自機の確認用の画像を撮るスクリプト(Blender 5.x)
# build_player.py → bake_player.py の後に実行する。OUT_DIR に PNG を書き出す(リポジトリの外に置く)。
#
# 見た目は「ゲームのシェーダー(LightingCommon.hlsli / LightingPS.hlsl)と同じ式」で描く(2面のボスの render_preview.py と同じ):
#   色 = ベース色 × saturate(拡散 + 環境光0.5) + ツヤ + 発光
#   ツヤ = saturate(dot(視線, reflect(光, 法線)))^(メタリックのR*34+1) × 拡散(ベース色のアルファはないので1)
#   法線マップはDirectX式、xyを1.5倍してから正規化。画像は Non-Color で読み、表示は Raw
# ゲームのカメラ: 縦の画角90°、1280x720。背景は曇り空の地平線の色(168,174,183)。
# 光の向き: Game::light_direction (-0.332, -0.473, 0.816)。自機はゲームで Y軸まわりに180度回して置くので、
#   モデルの座標では (0.332, -0.473, -0.816)。Blenderの座標 (x, z, y) にして使う。
#
# 変数(実行するときに globals で渡す):
#   OUT_DIR  書き出し先
#   TEX_DIR  テクスチャのフォルダ(既定は export/Player.fbm)
#   SHOTS    撮る名前のリスト(None なら全部)

import bpy
import math
import os
from mathutils import Vector

_HERE = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else ""
OUT_DIR = globals().get("OUT_DIR", r"C:\temp")
TEX_DIR = globals().get("TEX_DIR", os.path.join(_HERE, "export", "Player.fbm"))
SHOTS = globals().get("SHOTS", None)

LIGHT_MODEL = (0.332, -0.473, -0.816)
LIGHT = Vector((LIGHT_MODEL[0], LIGHT_MODEL[2], LIGHT_MODEL[1])).normalized()
AMBIENT = 0.5
NORMAL_STRENGTH = 1.5
SKY = (168 / 255.0, 174 / 255.0, 183 / 255.0)
BODY = "Player_Body"


def load_image(path):
    name = "Preview_" + os.path.basename(path)
    old = bpy.data.images.get(name)
    if old is not None:
        bpy.data.images.remove(old)
    img = bpy.data.images.load(path, check_existing=False)
    img.name = name
    img.colorspace_settings.name = "Non-Color"
    return img


def build_game_material(tex_dir):
    name = "PreviewGame"
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
    tex = {}
    # 画像の名前は前の自機(StarSparrow)と同じ(bake_player.py)
    files = {"Base": "StarSparrow_White.png", "Normal": "StarSparrow_Normal.png",
             "Emission": "StarSparrow_Emission.png", "Metallic": "StarSparrow_MetallicSmoothness.png"}
    for k, fname in files.items():
        t = N("ShaderNodeTexImage")
        t.image = load_image(os.path.join(tex_dir, fname))
        L(uv.outputs["UV"], t.inputs["Vector"])
        tex[k] = t

    sep = N("ShaderNodeSeparateXYZ")
    L(tex["Normal"].outputs["Color"], sep.inputs[0])
    comps = []
    for i in range(3):
        c = math_("MULTIPLY_ADD", sep.outputs[i], None, vb=2.0)
        c.inputs[2].default_value = -1.0
        comps.append(c)
    nx2 = math_("MULTIPLY", comps[0].outputs[0], None, vb=NORMAL_STRENGTH)
    # DirectX式: ゲームの従法線 cross(N,T) はテクスチャの下向き。Blenderの cross(N,T) は上向きなので符号を反転
    ny2 = math_("MULTIPLY", comps[1].outputs[0], None, vb=-NORMAL_STRENGTH)
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
    L(comps[2].outputs[0], nzv.inputs["Scale"])
    nf = vmath("NORMALIZE", vmath("ADD", vmath("ADD", tx.outputs[0], by.outputs[0]).outputs[0],
                                  nzv.outputs[0]).outputs[0])

    neg_l = tuple(-c for c in LIGHT)
    dif = vmath("DOT_PRODUCT", nf.outputs[0], None, vb=neg_l)
    dif_c = math_("MINIMUM", math_("MAXIMUM", dif.outputs["Value"], None, vb=0.0).outputs[0], None, vb=1.0)
    light = math_("ADD", dif_c.outputs[0], None, vb=AMBIENT, clamp=True)

    refl = vmath("REFLECT", None, nf.outputs[0], va=tuple(LIGHT))
    vd = vmath("DOT_PRODUCT", geo.outputs["Incoming"], refl.outputs[0])
    spec = math_("MINIMUM", math_("MAXIMUM", vd.outputs["Value"], None, vb=0.0).outputs[0], None, vb=1.0)
    msep = N("ShaderNodeSeparateColor")
    L(tex["Metallic"].outputs["Color"], msep.inputs[0])
    smooth = math_("MULTIPLY_ADD", msep.outputs[0], None, vb=34.0)
    smooth.inputs[2].default_value = 1.0
    spec = math_("POWER", spec.outputs[0], smooth.outputs[0])
    spec = math_("MULTIPLY", spec.outputs[0], dif_c.outputs[0])

    lit = vmath("SCALE", tex["Base"].outputs["Color"])
    L(light.outputs[0], lit.inputs["Scale"])
    spec_v = N("ShaderNodeCombineXYZ")
    for i in range(3):
        L(spec.outputs[0], spec_v.inputs[i])
    col = vmath("ADD", vmath("ADD", lit.outputs[0], spec_v.outputs[0]).outputs[0], tex["Emission"].outputs["Color"])
    em = N("ShaderNodeEmission")
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
    world = scn.world or bpy.data.worlds.new("World")
    scn.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    bg.inputs["Color"].default_value = (*SKY, 1.0)
    bg.inputs["Strength"].default_value = 1.0
    cam = bpy.data.objects.get("PreviewCam")
    if cam is None:
        cam = bpy.data.objects.new("PreviewCam", bpy.data.cameras.new("PreviewCam"))
        scn.collection.objects.link(cam)
    cam.data.sensor_fit = "VERTICAL"
    cam.data.clip_start = 0.05
    cam.data.clip_end = 1000
    scn.camera = cam
    return cam


# (名前, カメラ位置, 注視点, 縦の画角, 機体のロール(度、+で右翼が下がる), 黒塗り)
# ゲームのカメラは機体のほぼ真後ろ(モデルの単位で 9〜16m 後ろ)、同じ高さ〜少し上
SHOT_LIST = [
    ("game_view", (0.0, 16.0, 2.0), (0.0, -100.0, 2.0), 90, 0, False),
    # 機体より上にカメラがあるとき(海面すれすれを飛んだとき)。機体を見下ろす
    ("game_above", (0.0, 13.0, 6.5), (0.0, -3.0, 0.0), 45, 0, False),
    ("game_zoom", (0.0, 16.0, 2.0), (0.0, 0.0, 0.4), 28, 0, False),
    ("game_zoom_silhouette", (0.0, 16.0, 2.0), (0.0, 0.0, 0.4), 28, 0, True),
    ("game_zoom_roll", (0.0, 16.0, 2.0), (0.0, 0.0, 0.4), 28, 50, False),
    ("back_3q", (10.0, 17.0, 7.0), (0.0, 0.0, 0.3), 34, 0, False),
    ("top", (0.0, 0.01, 24.0), (0.0, 0.0, 0.0), 34, 0, False),
    ("top_silhouette", (0.0, 0.01, 24.0), (0.0, 0.0, 0.0), 34, 0, True),
    ("front_3q", (-9.0, -15.0, 4.5), (0.0, 0.0, 0.3), 34, 0, False),
    ("side", (-26.0, 0.0, 0.8), (0.0, 0.0, 0.8), 22, 0, False),
    ("nozzle_close", (3.0, 12.0, 1.8), (0.0, 5.5, 0.3), 22, 0, False),
]


def main():
    cam = setup_scene()
    game = build_game_material(TEX_DIR)
    black = build_black_material()
    body = bpy.data.objects[BODY]
    vl = bpy.context.view_layer
    os.makedirs(OUT_DIR, exist_ok=True)
    done = []
    rot0 = tuple(body.rotation_euler)
    try:
        for name, pos, tgt, ang, roll, is_black in SHOT_LIST:
            if SHOTS is not None and name not in SHOTS:
                continue
            vl.material_override = black if is_black else game
            # ロール: 機首の軸(Y)まわり。+で右翼(-X)が下がる
            body.rotation_euler = (0.0, math.radians(-roll), 0.0)
            cam.location = Vector(pos)
            d = Vector(tgt) - Vector(pos)
            cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
            if name.startswith("top"):
                # 真上から: 機首(-Y)が画像の上
                cam.rotation_euler = (0.0, 0.0, math.pi)
            cam.data.angle = math.radians(ang)
            p = os.path.join(OUT_DIR, name + ".png")
            bpy.context.scene.render.filepath = p
            bpy.ops.render.render(write_still=True)
            done.append(p)
    finally:
        vl.material_override = None
        body.rotation_euler = rot0
        bpy.data.objects.remove(cam, do_unlink=True)
    return {"rendered": done, "tex_dir": TEX_DIR}


result = main()

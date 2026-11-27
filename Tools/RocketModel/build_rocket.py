# 2面のボスのロケット弾のモデルを作って、FBXと画像を書き出すスクリプト(Blender 5.x)
# 実行: blender.exe -b --factory-startup --python build_rocket.py  (make_all.ps1 から呼ぶ)
# 単位: Blenderの1m = ゲームの100。正面(機首)はBlenderの -Y(ゲームの -Z)、上は +Z。原点はロケットの真ん中。
# 全長1.5m(ゲームの150)、胴の太さ0.22m(22)。
#
# 形:
# - とがった赤い機首(先だけ光る)と、黄色い帯
# - 灰色の胴と、後ろで細くなる所
# - 後ろにX字の大きな羽根4枚、機首の後ろにX字の小さな羽根4枚
# - 後ろの噴射口(中が光る)
# ロケットはプレイヤーに向かって飛ぶので、プレイヤーからはほぼ機首側しか見えない。だから機首の先も光らせる。
#
# 色は1枚の小さな画像(4x4のマス)に塗り分け、面ごとにそのマスの中にUVを置く(焼き付けはしない)。
# 作る画像(export/。ゲームの他のモデルと同じ約束):
#   Rocket_Albedo.png    ベース色(RGB、アルファなし)。モデルのマテリアルに貼る
#   Rocket_Metallic.png  滑らかさ(R)。ゲームではメタリックの画像として渡す
#   Rocket_Emission.png  発光(光らない所は黒)
#   Rocket_Normal.png    法線(平ら。FBXに法線マップがないと接線なしの.mv1になるので必要)

import bpy
import bmesh
import math
import os
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
EXPORT_DIR = os.path.join(HERE, "export")
FBX_PATH = os.path.join(EXPORT_DIR, "Rocket.fbx")
BLEND_PATH = os.path.join(HERE, "Rocket.blend")
OBJ_NAME = "Rocket"

SEGMENTS = 16
TEX_SIZE = 64
CELL = TEX_SIZE // 4

# マスの番号: (ベース色, 滑らかさ, 発光)
# 2面は曇り空と灰色の街なので、灰色や暗い色は背景にまぎれる。胴は濃い赤オレンジにして少し光らせ、
# 機首は全体を光らせる(正面から見ても光る点に見えるように)
CELLS = {
    "body":   ((0.90, 0.22, 0.06), 0.55, (0.35, 0.06, 0.00)),
    "nose":   ((1.00, 0.55, 0.15), 0.65, (1.00, 0.45, 0.08)),
    "band":   ((1.00, 0.85, 0.20), 0.50, (0.60, 0.45, 0.05)),
    "fin":    ((0.15, 0.15, 0.17), 0.40, (0.0, 0.0, 0.0)),
    "nozzle": ((0.10, 0.10, 0.11), 0.30, (0.0, 0.0, 0.0)),
    "glow":   ((1.00, 0.70, 0.30), 0.20, (1.00, 0.60, 0.20)),
    "tip":    ((1.00, 0.95, 0.70), 0.20, (1.00, 0.95, 0.60)),
}
CELL_INDEX = {name: i for i, name in enumerate(CELLS)}


def cleanup():
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    for me in list(bpy.data.meshes):
        bpy.data.meshes.remove(me)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)
    for img in list(bpy.data.images):
        bpy.data.images.remove(img)


def ring(bm, y, r):
    """yの位置に半径rの輪の頂点を作る(r=0なら1点)"""
    if r <= 0.0:
        return [bm.verts.new((0.0, y, 0.0))]
    verts = []
    for i in range(SEGMENTS):
        a = 2.0 * math.pi * i / SEGMENTS
        verts.append(bm.verts.new((r * math.cos(a), y, r * math.sin(a))))
    return verts


def bridge(bm, a, b, part, faces):
    """2つの輪(または点と輪)をつなぐ面を作る。外から見て表になる向き(-Y側がa)"""
    if len(a) == 1:
        for i in range(SEGMENTS):
            f = bm.faces.new((a[0], b[(i + 1) % SEGMENTS], b[i]))
            faces.append((f, part))
        return
    if len(b) == 1:
        for i in range(SEGMENTS):
            f = bm.faces.new((a[i], a[(i + 1) % SEGMENTS], b[0]))
            faces.append((f, part))
        return
    for i in range(SEGMENTS):
        j = (i + 1) % SEGMENTS
        f = bm.faces.new((a[i], a[j], b[j], b[i]))
        faces.append((f, part))


def build_body(bm, faces):
    # (y, 半径, その輪から次の輪までの部分)
    profile = [
        (-0.75, 0.0, "tip"),
        (-0.70, 0.026, "nose"),
        (-0.60, 0.068, "nose"),
        (-0.50, 0.095, "nose"),
        (-0.42, 0.110, "band"),
        (-0.34, 0.110, "body"),
        (0.60, 0.110, "nozzle"),
        (0.68, 0.085, "nozzle"),
        (0.75, 0.095, "nozzle"),
        (0.75, 0.075, "nozzle"),
        (0.70, 0.075, "glow"),
        (0.70, 0.0, None),
    ]
    rings = [ring(bm, y, r) for (y, r, _) in profile]
    for k in range(len(profile) - 1):
        bridge(bm, rings[k], rings[k + 1], profile[k][2], faces)


def build_fin(bm, faces, angle, root_y0, root_y1, tip_y0, tip_y1, r_root, r_tip, thick):
    """胴から外へ出る板の羽根を1枚作る(angleは羽根の向き)"""
    d = Vector((math.cos(angle), 0.0, math.sin(angle)))
    n = Vector((-math.sin(angle), 0.0, math.cos(angle))) * (thick * 0.5)
    pts = [
        d * r_root + Vector((0.0, root_y0, 0.0)),
        d * r_tip + Vector((0.0, tip_y0, 0.0)),
        d * r_tip + Vector((0.0, tip_y1, 0.0)),
        d * r_root + Vector((0.0, root_y1, 0.0)),
    ]
    top = [bm.verts.new(p + n) for p in pts]
    bot = [bm.verts.new(p - n) for p in pts]
    faces.append((bm.faces.new(top), "fin"))
    faces.append((bm.faces.new(list(reversed(bot))), "fin"))
    for i in range(4):
        j = (i + 1) % 4
        faces.append((bm.faces.new((bot[i], bot[j], top[j], top[i])), "fin"))


def make_mesh():
    bm = bmesh.new()
    faces = []
    build_body(bm, faces)
    for k in range(4):
        a = math.radians(45.0 + 90.0 * k)
        # 後ろの大きな羽根
        build_fin(bm, faces, a, 0.28, 0.62, 0.50, 0.66, 0.10, 0.27, 0.014)
        # 機首の後ろの小さな羽根
        build_fin(bm, faces, a, -0.30, -0.16, -0.22, -0.16, 0.10, 0.17, 0.010)
    bmesh.ops.recalc_face_normals(bm, faces=[f for f, _ in faces])

    # UV: 面ごとに、その部分のマスの真ん中の小さな円の上に置く
    uv = bm.loops.layers.uv.new("UVMap")
    for f, part in faces:
        idx = CELL_INDEX[part]
        cx = (idx % 4 + 0.5) / 4.0
        cy = 1.0 - (idx // 4 + 0.5) / 4.0
        n = len(f.loops)
        for li, loop in enumerate(f.loops):
            a = 2.0 * math.pi * li / n
            loop[uv].uv = (cx + 0.04 * math.cos(a), cy + 0.04 * math.sin(a))

    me = bpy.data.meshes.new(OBJ_NAME)
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    me.set_sharp_from_angle(angle=math.radians(40.0))
    ob = bpy.data.objects.new(OBJ_NAME, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def cell_image(values):
    """マスごとの値(RGB)を、TEX_SIZEの画像の配列(上の行が先)にする"""
    px = np.zeros((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    px[:, :, 3] = 1.0
    for name, rgb in values.items():
        idx = CELL_INDEX[name]
        x0 = (idx % 4) * CELL
        y0 = (idx // 4) * CELL
        px[y0:y0 + CELL, x0:x0 + CELL, 0:3] = rgb
    return px


def save_png(name, px_top_first):
    """8bitのRGBのPNGに保存する(Non-Color なので値はそのまま入る)"""
    path = os.path.join(EXPORT_DIR, name)
    img = bpy.data.images.new(name, TEX_SIZE, TEX_SIZE, alpha=True, float_buffer=False)
    img.colorspace_settings.name = "Non-Color"
    # Blenderの画像は下の行が先
    img.pixels.foreach_set(np.flipud(px_top_first).astype(np.float32).ravel())
    scn = bpy.context.scene
    st = scn.render.image_settings
    st.file_format = "PNG"
    st.color_mode = "RGB"
    st.color_depth = "8"
    scn.view_settings.view_transform = "Raw"
    img.save_render(path, scene=scn)
    bpy.data.images.remove(img)
    return path


def make_textures():
    os.makedirs(EXPORT_DIR, exist_ok=True)
    save_png("Rocket_Albedo.png", cell_image({k: v[0] for k, v in CELLS.items()}))
    save_png("Rocket_Metallic.png", cell_image({k: (v[1], v[1], v[1]) for k, v in CELLS.items()}))
    save_png("Rocket_Emission.png", cell_image({k: v[2] for k, v in CELLS.items()}))
    save_png("Rocket_Normal.png", cell_image({k: (0.5, 0.5, 1.0) for k in CELLS}))


def make_material(ob):
    mat = bpy.data.materials.new("Rocket_Mat")
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(os.path.join(EXPORT_DIR, "Rocket_Albedo.png"), check_existing=False)
    tex.image.colorspace_settings.name = "Non-Color"
    tex.interpolation = "Closest"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    ntex = nt.nodes.new("ShaderNodeTexImage")
    ntex.image = bpy.data.images.load(os.path.join(EXPORT_DIR, "Rocket_Normal.png"), check_existing=False)
    ntex.image.colorspace_settings.name = "Non-Color"
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(ntex.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    # DxLibのマテリアルの値は自機・2面のボスにそろえる: 拡散色1.0・自己発光0.5
    bsdf.inputs["Base Color"].default_value = (1.0, 1.0, 1.0, 1.0)
    bsdf.inputs["Emission Color"].default_value = (0.5, 0.5, 0.5, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 1.0
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.45
    ob.data.materials.append(mat)


def export(ob):
    bpy.context.view_layer.update()
    for o in bpy.context.scene.objects:
        if o is not None and o != ob:
            o.select_set(False)
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(
        filepath=FBX_PATH,
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        global_scale=1.0,
        apply_scale_options="FBX_SCALE_NONE",
        axis_forward="-Z",
        axis_up="Y",
        bake_space_transform=True,
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_tspace=True,
        add_leaf_bones=False,
        bake_anim=False,
        path_mode="STRIP",
        embed_textures=False,
    )


cleanup()
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = 1.0
make_textures()
rocket = make_mesh()
make_material(rocket)
export(rocket)
bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
print("ROCKET_OK", FBX_PATH, len(rocket.data.polygons))

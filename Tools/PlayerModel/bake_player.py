# 自機の色・滑らかさ・発光・法線を、テクスチャ1枚ずつに焼き付けるスクリプト(Blender 5.x、Cycles)
# build_player.py の後、export_player.py の前に実行する。
# 作るもの(TEX_DIR。既定は Tools/PlayerModel/export/Player.fbm/)。
# 画像の名前は前の自機(StarSparrow)と同じにして、ゲームのコード(ResourceConstants.h)を変えずに置き換えられるようにする:
#   StarSparrow_White.png      ベース色(RGB)。モデルのマテリアルに貼る。アルファは入れない
#                        (アルファがあるとDxLibが半透明のメッシュと判定するため。2面のボスと同じ)
#   StarSparrow_MetallicSmoothness.png  滑らかさ(R)。ゲームではメタリックの画像として渡し、1 + R*34 がツヤの鋭さになる
#   StarSparrow_Emission.png  発光(影の中でも足し算で光る。光らない所は黒)
#   StarSparrow_Normal.png    法線(タンジェント空間、DirectX式=緑がテクスチャの下向き)
# 値は「PNGに入る値」をそのまま焼く(画像は Non-Color。ゲームはガンマ空間のまま計算する)。
# 最後に、メッシュの全部の面を「StarSparrow_White.png を使うマテリアル1つ」に差し替える。

import bpy
import os
import numpy as np

_HERE = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else ""
TEX_DIR = globals().get("TEX_DIR", os.path.join(_HERE, "export", "Player.fbm") if _HERE else r"C:\temp\Player.fbm")
SIZE = globals().get("SIZE", 2048)
SAMPLES = globals().get("SAMPLES", 32)
OBJ_NAMES = ("Player_Body",)
ATLAS = "P_Atlas"


def new_image(name):
    img = bpy.data.images.get(name)
    if img is not None:
        bpy.data.images.remove(img)
    img = bpy.data.images.new(name, SIZE, SIZE, alpha=False, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"
    return img


def bake_mats():
    return [m for m in bpy.data.materials if m.name.startswith("P_") and m.name != ATLAS]


def set_output(kind):
    """全部のマテリアルで、出力(OUT_COLOR など)を Material Output につなぎ替える"""
    for m in bake_mats():
        nt = m.node_tree
        out = nt.nodes["MatOut"]
        src = nt.nodes[kind]
        for l in list(out.inputs["Surface"].links):
            nt.links.remove(l)
        nt.links.new(src.outputs[0], out.inputs["Surface"])


def set_bake_target(img):
    for m in bake_mats():
        nt = m.node_tree
        node = nt.nodes.get("BakeTarget")
        if node is None:
            node = nt.nodes.new("ShaderNodeTexImage")
            node.name = "BakeTarget"
        node.image = img
        for n in nt.nodes:
            n.select = False
        node.select = True
        nt.nodes.active = node


def bake(bake_type, img, samples, **kw):
    scn = bpy.context.scene
    scn.cycles.samples = samples
    set_bake_target(img)
    objs = [bpy.data.objects[n] for n in OBJ_NAMES]
    for ob in bpy.context.view_layer.objects:
        ob.select_set(False)
    for ob in objs:
        ob.hide_set(False)
        ob.hide_render = False
        ob.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    with bpy.context.temp_override(active_object=objs[0], object=objs[0],
                                   selected_objects=objs, selected_editable_objects=objs):
        bpy.ops.object.bake(type=bake_type, margin=8, margin_type="EXTEND", use_clear=True, **kw)
    for ob in objs:
        ob.select_set(False)


def pixels(img):
    a = np.empty(SIZE * SIZE * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(SIZE, SIZE, 4)


def save_png(name, rgba, path):
    """8bitのRGBのPNGに保存する(Non-Color なので値はそのまま入る)"""
    img = bpy.data.images.get(name + "_out")
    if img is not None:
        bpy.data.images.remove(img)
    img = bpy.data.images.new(name + "_out", SIZE, SIZE, alpha=True, float_buffer=False)
    img.colorspace_settings.name = "Non-Color"
    img.alpha_mode = "STRAIGHT"
    img.pixels.foreach_set(np.clip(rgba, 0.0, 1.0).astype(np.float32).ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    scn = bpy.context.scene
    settings = scn.render.image_settings
    prev = (settings.file_format, settings.color_mode, settings.color_depth)
    settings.file_format = "PNG"
    settings.color_mode = "RGB"
    settings.color_depth = "8"
    img.save_render(path, scene=scn)
    settings.file_format, settings.color_mode, settings.color_depth = prev
    bpy.data.images.remove(img)


def use_gpu(scn):
    """GPU(OptiX → CUDA)があれば使う。なければCPU"""
    try:
        prefs = bpy.context.preferences.addons["cycles"].preferences
        for t in ("OPTIX", "CUDA"):
            prefs.compute_device_type = t
            prefs.get_devices()
            if any(d.type == t for d in prefs.devices):
                for d in prefs.devices:
                    d.use = (d.type == t)
                scn.cycles.device = "GPU"
                return t
    except Exception:
        pass
    scn.cycles.device = "CPU"
    return "CPU"


def main():
    os.makedirs(TEX_DIR, exist_ok=True)
    scn = bpy.context.scene
    prev_engine = scn.render.engine
    scn.render.engine = "CYCLES"
    device = use_gpu(scn)
    scn.render.bake.use_selected_to_active = False
    vt = scn.view_settings.view_transform
    scn.view_settings.view_transform = "Raw"
    # 前の焼き付けで差し替えたマテリアルが残っていたら、作り直してから焼く(build_player.py を先に実行する)
    me = bpy.data.objects[OBJ_NAMES[0]].data
    if any(m is not None and m.name == ATLAS for m in me.materials):
        raise RuntimeError("先に build_player.py を実行してください(マテリアルが差し替え済み)")

    out = {"device": device}
    try:
        for kind, fname, kw, smp in (
                ("OUT_COLOR", "StarSparrow_White.png", {}, SAMPLES),
                # 滑らかさは汚れ(AO)・角(Bevel)を使うので、色と同じサンプル数で焼く
                ("OUT_SMOOTH", "StarSparrow_MetallicSmoothness.png", {}, SAMPLES),
                ("OUT_EMIT", "StarSparrow_Emission.png", {}, 1)):
            img = new_image("PBake_" + kind)
            set_output(kind)
            bake("EMIT", img, smp, **kw)
            px = pixels(img).copy()
            px[:, :, 3] = 1.0
            p = os.path.join(TEX_DIR, fname)
            save_png(fname[:-4], px, p)
            out[fname] = p
        # 法線(DirectX式)
        img = new_image("PBake_Normal")
        set_output("OUT_NORMAL")
        bake("NORMAL", img, SAMPLES, normal_space="TANGENT", normal_r="POS_X", normal_g="NEG_Y", normal_b="POS_Z")
        px = pixels(img).copy()
        px[:, :, 3] = 1.0
        p = os.path.join(TEX_DIR, "StarSparrow_Normal.png")
        save_png("StarSparrow_Normal", px, p)
        out["StarSparrow_Normal.png"] = p
    finally:
        set_output("OUT_NORMAL")
        scn.render.engine = prev_engine
        scn.view_settings.view_transform = vt

    # 全部の面を、ベース色の画像を使うマテリアル1つに差し替える
    atlas = bpy.data.materials.get(ATLAS)
    if atlas is not None:
        bpy.data.materials.remove(atlas)
    atlas = bpy.data.materials.new(ATLAS)
    atlas.use_nodes = True
    nt = atlas.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(os.path.join(TEX_DIR, "StarSparrow_White.png"), check_existing=False)
    tex.image.colorspace_settings.name = "Non-Color"
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    # 法線マップもつなぐ(FBXに法線マップがないと、DxLibは接線なしの頂点形式の.mv1を作り、
    # 接線を読むゲームのシェーダーと形式が合わない。2面のボスと同じ)
    ntex = nt.nodes.new("ShaderNodeTexImage")
    ntex.image = bpy.data.images.load(os.path.join(TEX_DIR, "StarSparrow_Normal.png"), check_existing=False)
    ntex.image.colorspace_settings.name = "Non-Color"
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(ntex.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    # DxLibのマテリアルの値は2面のボスにそろえる: 拡散色1.0・自己発光0.5
    # 発光の画像はゲーム側で別に渡すので、ここにはつながない(つなぐと真っ白に描かれる)
    bsdf.inputs["Base Color"].default_value = (1.0, 1.0, 1.0, 1.0)
    bsdf.inputs["Emission Color"].default_value = (0.5, 0.5, 0.5, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 1.0
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.45
    for name in OBJ_NAMES:
        me = bpy.data.objects[name].data
        me.materials.clear()
        me.materials.append(atlas)
        for poly in me.polygons:
            poly.material_index = 0
    return out


result = main()

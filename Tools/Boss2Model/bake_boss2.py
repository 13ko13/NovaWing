# 2面のボスの色・ツヤ・発光・法線を、テクスチャ1枚ずつに焼き付けるスクリプト(Blender 5.x、Cycles) 第2版
# build_boss2.py の後、export_boss2.py の前に実行する。
# 作るもの(TEX_DIR。ゲームでは Data/Model/Boss2.fbm/。1面のボスの Boss.fbm/ と同じ置き方):
#   Boss2_Base.png          ベース色(RGB)。モデルのマテリアルに貼る。SPEC_IN_ALPHA=True のときだけツヤの強さ(A)も入れる
#                           ゲームのシェーダーは「拡散色のアルファ = スペキュラの強さ」(LightingPS.hlsl)。
#                           メタリックの画像は渡さない(-1)ので、アルファがないとツヤは広く強く出る
#   Boss2_Metallic.png      滑らかさ(R)。ゲームではメタリックの画像(tex_metalic)として渡す。
#                           -1 のままだとツヤが dot^1 で広く出て、正面が白く飛ぶ。これを渡すと角やボルトに細く光るだけになる
#   Boss2_Emission.png      発光(通常。ポッドの光る所と蓋は黄色、放熱口は光らない)
#   Boss2_Emission_Vent.png 発光(放熱中。放熱口も黄色く光る)
#   Boss2_Emission_Damage.png 発光(弱点にダメージ。放熱中の画像のポッドの黄色を、蓋・放熱口・光る所すべて赤にしたもの)
#   Boss2_Normal.png        法線(タンジェント空間、DirectX式=緑がテクスチャの下向き)
#                           ゲームは従法線を cross(N, T) で作るので、DxLibの座標では下向きになる
# 値は「PNGに入る値」をそのまま焼く(画像は Non-Color。ゲームはガンマ空間のまま計算する)。
# 最後に、3つのメッシュの全部の面を「Boss2_Base.png を使うマテリアル1つ」に差し替える。

import bpy
import os
import numpy as np

TEX_DIR = globals().get("TEX_DIR", r"C:\temp")
SIZE = globals().get("SIZE", 2048)
SAMPLES = globals().get("SAMPLES", 32)
# False にすると最後のマテリアルの差し替えをしない(色を調整しながら焼き直すとき)
APPLY_ATLAS = globals().get("APPLY_ATLAS", True)
# True にするとツヤの強さを Boss2_Base.png のアルファに入れる(ゲームのシェーダーはアルファ=ツヤの強さ)。
# (11/19・ユーザー決定)既定は False。アルファがあるとDxLibの標準の描き方(モデルビューア)で
# 透けて見え、DxLibが半透明のメッシュと判定するため。代わりにゲームではツヤが広く強く出る。
SPEC_IN_ALPHA = globals().get("SPEC_IN_ALPHA", False)
OBJ_NAMES = ("Boss2_Main", "Boss2_PodR", "Boss2_PodR_Broken")


def new_image(name, alpha=False):
    img = bpy.data.images.get(name)
    if img is not None:
        bpy.data.images.remove(img)
    img = bpy.data.images.new(name, SIZE, SIZE, alpha=alpha, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"
    return img


def bake_mats():
    return [m for m in bpy.data.materials if m.name.startswith("B2_") and m.name != "B2_Atlas"]


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
    hidden = [ob for ob in objs if ob.hide_get()]
    for ob in objs:
        ob.hide_set(False)
        ob.hide_render = False
        ob.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    with bpy.context.temp_override(active_object=objs[0], object=objs[0],
                                   selected_objects=objs, selected_editable_objects=objs):
        bpy.ops.object.bake(type=bake_type, margin=8, margin_type="EXTEND", use_clear=True, **kw)
    for ob in hidden:
        ob.hide_set(True)
        ob.hide_render = True
    for ob in objs:
        ob.select_set(False)


def pixels(img):
    a = np.empty(SIZE * SIZE * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(SIZE, SIZE, 4)


def save_png(name, rgba, path, alpha=False):
    """8bitのPNGに保存する(Non-Color なので値はそのまま入る)。alpha=False ならRGBだけ"""
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
    settings.color_mode = "RGBA" if alpha else "RGB"
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
    # 休みのポーズで焼く(装甲を閉じた形)
    arm = bpy.data.objects["Boss2_Armature"]
    arm.data.pose_position = "REST"
    # 色の管理: 焼いた値をそのまま書き出す
    vt = scn.view_settings.view_transform
    scn.view_settings.view_transform = "Raw"

    out = {"device": device}
    try:
        # ベース色とツヤ
        img_c = new_image("B2Bake_Color")
        set_output("OUT_COLOR")
        bake("EMIT", img_c, SAMPLES)
        rgba = pixels(img_c).copy()
        rgba[:, :, 3] = 1.0
        if SPEC_IN_ALPHA:
            img_s = new_image("B2Bake_Spec")
            set_output("OUT_SPEC")
            bake("EMIT", img_s, SAMPLES)
            rgba[:, :, 3] = pixels(img_s)[:, :, 0]
        p = os.path.join(TEX_DIR, "Boss2_Base.png")
        save_png("Boss2_Base", rgba, p, alpha=SPEC_IN_ALPHA)
        out["base"] = p
        out["base_mean"] = [float(v) for v in rgba.reshape(-1, 4).mean(axis=0)]

        # 滑らかさ(ゲームではメタリックの画像として渡す。R*34+1 がツヤの鋭さ)
        img_m = new_image("B2Bake_Smooth")
        set_output("OUT_SMOOTH")
        bake("EMIT", img_m, 1)
        m = pixels(img_m)
        m[:, :, 3] = 1.0
        p = os.path.join(TEX_DIR, "Boss2_Metallic.png")
        save_png("Boss2_Metallic", m, p)
        out["metallic"] = p

        # 発光(通常 / 放熱中 / 弱点にダメージ)
        for kind, fname in (("OUT_EMIT", "Boss2_Emission.png"), ("OUT_EMIT_VENT", "Boss2_Emission_Vent.png"),
                            ("OUT_EMIT_DAMAGE", "Boss2_Emission_Damage.png")):
            img_e = new_image("B2Bake_" + kind)
            set_output(kind)
            bake("EMIT", img_e, 1)
            e = pixels(img_e)
            e[:, :, 3] = 1.0
            p = os.path.join(TEX_DIR, fname)
            save_png(fname[:-4], e, p)
            out[kind] = p

        # 法線(DirectX式)
        img_n = new_image("B2Bake_Normal")
        set_output("OUT_NORMAL")
        bake("NORMAL", img_n, SAMPLES, normal_space="TANGENT",
             normal_r="POS_X", normal_g="NEG_Y", normal_b="POS_Z")
        n = pixels(img_n)
        n[:, :, 3] = 1.0
        p = os.path.join(TEX_DIR, "Boss2_Normal.png")
        save_png("Boss2_Normal", n, p)
        out["normal"] = p
    finally:
        set_output("OUT_NORMAL")
        arm.data.pose_position = "POSE"
        scn.render.engine = prev_engine
        scn.view_settings.view_transform = vt
    if not APPLY_ATLAS:
        return out

    # 全部の面を、ベース色の画像を使うマテリアル1つに差し替える
    atlas = bpy.data.materials.get("B2_Atlas")
    if atlas is not None:
        bpy.data.materials.remove(atlas)
    atlas = bpy.data.materials.new("B2_Atlas")
    atlas.use_nodes = True
    nt = atlas.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(os.path.join(TEX_DIR, "Boss2_Base.png"), check_existing=False)
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    # 法線マップもマテリアルにつなぐ。FBXに法線マップが入っていないと、DxLibは接線(タンジェント)なしの
    # 頂点形式(4FRAME)の.mv1を作る。ゲームの頂点シェーダー(SkinnedLightingVS)は接線を読むので、
    # 形式が合わずにモデルが描かれない。つなぐと接線つき(NMAP_4FRAME)になる。
    ntex = nt.nodes.new("ShaderNodeTexImage")
    ntex.image = bpy.data.images.load(os.path.join(TEX_DIR, "Boss2_Normal.png"), check_existing=False)
    ntex.image.colorspace_settings.name = "Non-Color"
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(ntex.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    # DxLibのマテリアルの値は1面のボス(Boss.mv1)にそろえる: 拡散色1.0・自己発光0.5
    # (FBXでは拡散色=Base Colorの値、自己発光=Emission Color×Strength になる)
    # 発光の画像はゲーム側で別に渡すので、ここにはつながない(つなぐと真っ白に描かれる)
    # アルファもつながない(ツヤの強さとしてシェーダーが読む。透明にはしない)
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

# 2面のボスの色と発光を、テクスチャ1枚ずつに焼き付けるスクリプト(Blender 5.x)
# build_boss2.py の後、export_boss2.py の前に実行する。
# 作るもの(TEX_DIR。ゲームでは Data/Model/Boss2.fbm/。1面のボスの Boss.fbm/ と同じ置き方):
#   Boss2_Base.png          ベース色(モデルのマテリアルに貼る)
#   Boss2_Emission.png      発光(通常。放熱口は光らない)
#   Boss2_Emission_Vent.png 発光(放熱中。放熱口が赤く光る)
#   Boss2_Normal.png        法線(平ら)
# 最後に、3つのメッシュの全部の面を「Boss2_Base.png を使うマテリアル1つ」に差し替える。

import bpy
import os

TEX_DIR = globals().get("TEX_DIR", r"C:\temp")
SIZE = globals().get("SIZE", 2048)
OBJ_NAMES = ("Boss2_Main", "Boss2_PodR", "Boss2_PodR_Broken")


def new_image(name, color, non_color=False):
    img = bpy.data.images.get(name)
    if img is not None:
        bpy.data.images.remove(img)
    img = bpy.data.images.new(name, SIZE, SIZE, alpha=False)
    img.generated_color = color
    if non_color:
        img.colorspace_settings.name = "Non-Color"
    return img


def set_bake_target(img):
    """全部のマテリアルに、焼き付け先の画像ノードを置いて選んでおく"""
    for m in bpy.data.materials:
        if not m.name.startswith("B2_") or m.name == "B2_Atlas":
            continue
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


def set_emission(scale_by_name):
    """焼き付け用に発光の強さを入れ替える(1を超えると白く飛ぶので、色がそのまま出る強さにする)"""
    backup = {}
    for m in bpy.data.materials:
        if not m.name.startswith("B2_") or m.name == "B2_Atlas":
            continue
        b = m.node_tree.nodes.get("Principled BSDF")
        backup[m.name] = b.inputs["Emission Strength"].default_value
        b.inputs["Emission Strength"].default_value = scale_by_name.get(m.name, 0.0)
    return backup


def restore_emission(backup):
    for name, v in backup.items():
        b = bpy.data.materials[name].node_tree.nodes.get("Principled BSDF")
        b.inputs["Emission Strength"].default_value = v


def bake(bake_type, img, path, **kw):
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
        bpy.ops.object.bake(type=bake_type, margin=8, use_clear=True, **kw)
    for ob in hidden:
        ob.hide_set(True)
        ob.hide_render = True
    for ob in objs:
        ob.select_set(False)
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()


def main():
    os.makedirs(TEX_DIR, exist_ok=True)
    scn = bpy.context.scene
    prev_engine = scn.render.engine
    scn.render.engine = "CYCLES"
    scn.cycles.samples = 1
    scn.cycles.device = "CPU"
    # 0フレーム(待機の最初)で焼く。形は休みのポーズと同じ所に焼けるよう、アーマチュアを休みのポーズにする
    arm = bpy.data.objects["Boss2_Armature"]
    arm.data.pose_position = "REST"

    out = {}
    # ベース色
    img = new_image("Boss2_Base", (0.05, 0.05, 0.05, 1.0))
    bake("DIFFUSE", img, os.path.join(TEX_DIR, "Boss2_Base.png"), pass_filter={"COLOR"})
    out["base"] = img.filepath_raw

    glow = {"B2_GlowL": 1.0, "B2_GlowR": 1.0, "B2_Eye": 1.0, "B2_Engine": 1.0}
    # 発光(通常)
    backup = set_emission(dict(glow, B2_Vent=0.0))
    img = new_image("Boss2_Emission", (0.0, 0.0, 0.0, 1.0))
    bake("EMIT", img, os.path.join(TEX_DIR, "Boss2_Emission.png"))
    restore_emission(backup)
    out["emission"] = img.filepath_raw
    # 発光(放熱中)
    backup = set_emission(dict(glow, B2_Vent=1.0))
    img = new_image("Boss2_Emission_Vent", (0.0, 0.0, 0.0, 1.0))
    bake("EMIT", img, os.path.join(TEX_DIR, "Boss2_Emission_Vent.png"))
    restore_emission(backup)
    out["emission_vent"] = img.filepath_raw

    # 法線(平ら)。凹凸はまだ入れていない
    nimg = bpy.data.images.get("Boss2_Normal")
    if nimg is not None:
        bpy.data.images.remove(nimg)
    nimg = bpy.data.images.new("Boss2_Normal", 64, 64, alpha=False)
    nimg.generated_color = (0.5, 0.5, 1.0, 1.0)
    nimg.colorspace_settings.name = "Non-Color"
    nimg.filepath_raw = os.path.join(TEX_DIR, "Boss2_Normal.png")
    nimg.file_format = "PNG"
    nimg.save()
    out["normal"] = nimg.filepath_raw

    arm.data.pose_position = "POSE"
    scn.render.engine = prev_engine

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
    # DxLibのマテリアルの値は1面のボス(Boss.mv1)にそろえる: 拡散色1.0・自己発光0.5
    # (FBXでは拡散色=Base Colorの値、自己発光=Emission Color×Strength になる)
    # 発光の画像はゲーム側で別に渡すので、ここにはつながない(つなぐと真っ白に描かれる)
    bsdf.inputs["Base Color"].default_value = (1.0, 1.0, 1.0, 1.0)
    bsdf.inputs["Emission Color"].default_value = (0.5, 0.5, 0.5, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 1.0
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.45
    for n in OBJ_NAMES:
        me = bpy.data.objects[n].data
        me.materials.clear()
        me.materials.append(atlas)
        for p in me.polygons:
            p.material_index = 0
    return out


result = main()

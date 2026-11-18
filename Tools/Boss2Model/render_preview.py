# 2面ボスの確認用の画像を撮るスクリプト(Blender 5.x)
# build_boss2.py の後に実行する。OUT_DIR に PNG を書き出す。
# ゲームのカメラ: 縦の画角90°、1280x720。背景は2面の曇り空の地平線の色(168,174,183)。

import bpy
import math
from mathutils import Vector, Euler

OUT_DIR = globals().get("OUT_DIR", r"C:\temp")
SHOTS = globals().get("SHOTS", None)


def srgb_to_linear(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


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
    scn.view_settings.view_transform = "Standard"
    scn.view_settings.look = "None"
    # 背景(曇り空の地平線の色)
    world = scn.world or bpy.data.worlds.new("World")
    scn.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    col = tuple(srgb_to_linear(v) for v in (168, 174, 183))
    bg.inputs["Color"].default_value = (*col, 1.0)
    bg.inputs["Strength"].default_value = 1.0
    # 曇りの日の光: 上から弱い太陽
    sun = bpy.data.objects.get("PreviewSun")
    if sun is None:
        ld = bpy.data.lights.new("PreviewSun", "SUN")
        sun = bpy.data.objects.new("PreviewSun", ld)
        scn.collection.objects.link(sun)
    sun.data.energy = 2.5
    sun.data.color = (0.95, 0.97, 1.0)
    sun.rotation_euler = Euler((math.radians(35), math.radians(-20), math.radians(15)), "XYZ")
    cam = bpy.data.objects.get("PreviewCam")
    if cam is None:
        cd = bpy.data.cameras.new("PreviewCam")
        cam = bpy.data.objects.new("PreviewCam", cd)
        scn.collection.objects.link(cam)
    cam.data.sensor_fit = "VERTICAL"
    cam.data.angle = math.radians(90)
    cam.data.clip_end = 1000
    scn.camera = cam
    return cam


def look_at(cam, pos, target):
    cam.location = Vector(pos)
    d = Vector(target) - Vector(pos)
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()


def render(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def set_black(on):
    vl = bpy.context.view_layer
    if on:
        m = bpy.data.materials.get("PreviewBlack") or bpy.data.materials.new("PreviewBlack")
        m.use_nodes = True
        b = m.node_tree.nodes.get("Principled BSDF")
        b.inputs["Base Color"].default_value = (0, 0, 0, 1)
        b.inputs["Roughness"].default_value = 1.0
        b.inputs["Specular IOR Level"].default_value = 0.0
        b.inputs["Emission Strength"].default_value = 0.0
        vl.material_override = m
    else:
        vl.material_override = None


cam = setup_scene()
# ボスは原点。ゲームでは「カメラから約3000先、カメラより約150上」を想定(Blenderで 30m, 1.5m)
shots = {
    "game_view": ((0.0, -30.0, -1.5), (0.0, 0.0, 0.0), False),
    "game_view_silhouette": ((0.0, -30.0, -1.5), (0.0, 0.0, 0.0), True),
    "near_3q": ((-14.0, -20.0, 5.0), (0.0, 0.0, 1.0), False),
    "side": ((-30.0, 0.0, 2.0), (0.0, 0.0, 1.0), False),
    "top": ((0.0, -6.0, 32.0), (0.0, 0.0, 0.0), False),
}
done = []
for name, (pos, tgt, black) in shots.items():
    if SHOTS is not None and name not in SHOTS:
        continue
    look_at(cam, pos, tgt)
    set_black(black)
    p = OUT_DIR + "\\" + name + ".png"
    render(p)
    done.append(p)
set_black(False)


def set_vent_open(angle_deg):
    arm = bpy.data.objects.get("Boss2_Armature")
    pb = arm.pose.bones
    a = math.radians(angle_deg)
    for tag in ("L", "R"):
        pb["PodArmor_%s_Top" % tag].rotation_euler = (-a, 0.0, 0.0)
        pb["PodArmor_%s_Bottom" % tag].rotation_euler = (a, 0.0, 0.0)


# 放熱(隙)の見え方: 装甲を開いた状態
if SHOTS is None or "vent_open" in SHOTS:
    set_vent_open(40)
    for name, pos, tgt in (("vent_open_game", (0.0, -30.0, -1.5), (0.0, 0.0, 0.0)),
                           ("vent_open_near", (-20.0, -14.0, 7.0), (-12.6, 0.0, 4.4))):
        look_at(cam, pos, tgt)
        p = OUT_DIR + "\\" + name + ".png"
        render(p)
        done.append(p)
    set_vent_open(0)
result = {"rendered": done}

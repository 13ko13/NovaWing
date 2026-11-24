# 2面ボス登場ムービー(BossAppear2)を連番PNGに書き出す。
# 実行: C:\bl\blender.exe -b --python build_movie.py -- [オプション]
#   --out <フォルダ>      PNGの書き出し先(既定: リポジトリ外の一時フォルダ)
#   --frames 0,60,179     撮るフレーム番号(省略すると全180フレーム)
#   --scale 50            解像度の%(確認用。既定100 = 1920x1080)
#   --samples 16          EEVEEのサンプル数
#   --end-dist 2700       最後のカメラの、ボスからの距離(ゲームの bossAppearDist)
#   --end-height 1020     最後のカメラの、ボス位置からの高さ(ゲームの bossAppearCameraHeight。OnZoomUp が boss.y に足す)
#
# ゲームの GameCamera::OnZoomUp(speed=1.0)は、カメラを  ボス位置 + (0, offsetY, -zoomLimit) に1フレームで置く。
# ボスは (0, 1000, 0)(StageConfigの bossStandY)。縦FOV90°・水平。
# 見た目はゲームのシェーダーの式、色変換なし(Raw)。詳しくは Tools/Boss2Model/movie/README.md

import math
import os
import sys
import tempfile

import bpy
import numpy as np
from mathutils import Euler, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import movie_lib as ml  # noqa: E402

# ---------------------------------------------------------------- 引数
argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def opt(name, default):
    return argv[argv.index(name) + 1] if name in argv else default


OUT = opt("--out", os.path.join(tempfile.gettempdir(), "BossAppear2_frames"))
SCALE = int(opt("--scale", "100"))
SAMPLES = int(opt("--samples", "24"))
END_DIST = float(opt("--end-dist", "2700"))
END_H = float(opt("--end-height", "300"))
TOTAL = 180
FPS = 30
FRAMES = [int(v) for v in opt("--frames", "").split(",") if v] or list(range(TOTAL))

BOSS_Y = 1000.0
END_POS = Vector((0.0, BOSS_Y + END_H, -END_DIST))  # 最後のカメラ(ゲーム座標)
SHAKE_END_FRAME = 165  # 最後の0.5秒は揺らさない


# ---------------------------------------------------------------- ボスの動き(ゲーム座標)
# 速さ(ゲームの長さ/秒)を折れ線で決めて積分する → z(t)
SPEED_KEYS = [(0.0, 3500), (1.3, 5000), (2.3, 15000), (3.0, 16000), (3.5, 9000), (4.2, 2500), (4.8, 0), (6.1, 0)]
_ts = np.linspace(0, 6.1, 6101)
_v = np.interp(_ts, [k[0] for k in SPEED_KEYS], [k[1] for k in SPEED_KEYS])
_dist = np.concatenate([[0], np.cumsum((_v[1:] + _v[:-1]) / 2 * (_ts[1] - _ts[0]))])
Z0 = _dist[-1]


def boss_state(t):
    """t秒のボスの位置(ゲーム座標)、速度(ゲーム/秒)、向き(pitch, roll, yaw ラジアン、Blender)"""
    def pos(tt):
        d = np.interp(tt, _ts, _dist)
        r = (Z0 - d) / Z0  # 1→0
        z = Z0 - d
        x = -2200.0 * (r ** 1.5)
        y = BOSS_Y + 2300.0 * (r ** 1.3)
        return np.array([x, y, z])
    p = pos(t)
    e = 1 / 60
    vel = (pos(t + e) - pos(t - e)) / (2 * e)
    sp = max(np.linalg.norm(vel), 1e-6)
    w = min(sp / 3000.0, 1.0)
    # Blender座標の速度 (vx, vy=vz_game, vz=vy_game)
    yaw = math.atan2(vel[0], -vel[2]) * w
    pitch = math.atan2(-vel[1], max(-vel[2], 1.0)) * 0.8 * w  # 下へ進むとき機首が下がる(+X回りの回転)
    roll = max(-0.35, min(0.35, vel[0] * 0.00008)) * w
    return p, vel, (pitch, roll, yaw)


# ---------------------------------------------------------------- カメラ
def smooth(a):
    a = max(0.0, min(1.0, a))
    return a * a * (3 - 2 * a)


def lerp(a, b, k):
    return a + (b - a) * k


def lerp3(a, b, k):
    return tuple(lerp(a[i], b[i], k) for i in range(3))


# ボス基準の見た目(ボスの向きに依らず、最終位置 (0,1000,0) を基準にした相対位置)
def pod_l_world():
    return (-1271.0, BOSS_Y + 440.0, -430.0)


def cam_shot(f):
    """フレームfのカメラ: (位置, 注視点, 縦FOV, 揺れの大きさ(度))。ゲーム座標"""
    t = f / FPS
    bp, _, _ = boss_state(t)
    bpos = tuple(bp)
    if f < 39:  # ① 曇り空の遠くに小さな影
        k = f / 39
        pos = lerp3((-1800, 500, -3200), (-1500, 520, -2600), k)
        tgt = (bpos[0], bpos[1] - 2200, bpos[2])
        return pos, tgt, 52.0, 0.12
    if f < 69:  # ② 雨の中をこちらへ飛んでくる(望遠)
        k = (f - 39) / 30
        pos = lerp3((1800, 650, -1800), (1500, 700, -1000), k)
        return pos, bpos, 26.0, 0.18
    if f < 105:  # ③ 頭上を通り過ぎる(あおり)
        k = (f - 69) / 36
        pos = lerp3((1000, 250, 10500), (1000, 300, 9800), k)
        # 追い方を少し遅らせる(急に振り向かない)
        bp2, _, _ = boss_state(max(0.0, t - 0.12))
        tgt = tuple(bp2)
        return pos, tgt, lerp(70.0, 82.0, k), 0.45
    if f < 137:  # ④ 前へ回り込む
        k = smooth((f - 105) / 32)
        ang = math.radians(lerp(115.0, 18.0, k))
        rad = lerp(4300.0, 3300.0, k)
        pos = (bpos[0] + rad * math.sin(ang), bpos[1] + lerp(250.0, 350.0, k), bpos[2] - rad * math.cos(ang))
        return pos, (bpos[0], bpos[1] + 100, bpos[2]), lerp(62.0, 55.0, k), 0.3
    if f < 158:
        if f < 148:  # ⑤a 発射口(左のポッド)のアップ
            k = (f - 137) / 11
            pl = pod_l_world()
            pos = lerp3((-300, BOSS_Y + 500, -2100), (-500, BOSS_Y + 520, -1750), k)
            return pos, (pl[0], pl[1], pl[2]), 40.0, 0.25
        k = (f - 148) / 10  # ⑤b 光る部分(正面の中心)のアップ
        pos = lerp3((1100, BOSS_Y + 100, -1900), (800, BOSS_Y + 160, -1700), k)
        return pos, (0, BOSS_Y + 40, -300), 42.0, 0.25
    # ⑥ ゲームのカメラ位置まで引いて画角を広げる
    k = smooth((f - 158) / (TOTAL - 1 - 158))
    p0 = Vector((800, BOSS_Y + 160, -1700))
    pos = p0.lerp(END_POS, k)
    # 注視点も、最後は水平(カメラと同じ高さ、正面)にそろえる
    t0 = Vector((0, BOSS_Y + 40, -300))
    t1 = Vector((END_POS.x, END_POS.y, 0.0))
    tgt = t0.lerp(t1, k)
    fov = lerp(42.0, 90.0, k)
    return tuple(pos), tuple(tgt), fov, 0.2


def noise(t, seed):
    """滑らかなノイズ(-1〜1)"""
    r = np.random.default_rng(seed)
    ph = r.uniform(0, 6.28, 4)
    fr = np.array([0.9, 1.7, 2.9, 4.3])
    am = np.array([1.0, 0.6, 0.35, 0.2])
    return float(np.sum(am * np.sin(fr * 6.283 * t + ph)) / am.sum())


# ---------------------------------------------------------------- シーン
def setup_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scn = bpy.context.scene
    for eng in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scn.render.engine = eng
            break
        except TypeError:
            pass
    scn.render.resolution_x = 1920
    scn.render.resolution_y = 1080
    scn.render.resolution_percentage = SCALE
    scn.render.fps = FPS
    scn.render.image_settings.file_format = "PNG"
    scn.render.image_settings.color_mode = "RGB"
    scn.render.film_transparent = False
    scn.render.filter_size = 1.0
    try:
        scn.eevee.taa_render_samples = SAMPLES
    except Exception:
        pass
    scn.view_settings.view_transform = "Raw"
    scn.view_settings.look = "None"
    scn.view_settings.exposure = 0.0
    scn.view_settings.gamma = 1.0
    scn.display_settings.display_device = "sRGB"
    cd = bpy.data.cameras.new("Cam")
    cd.sensor_fit = "VERTICAL"
    cd.clip_start = 0.5
    cd.clip_end = 2000
    cam = bpy.data.objects.new("Cam", cd)
    scn.collection.objects.link(cam)
    scn.camera = cam
    return scn, cam


def setup_boss():
    bpy.ops.import_scene.fbx(filepath=os.path.join(ml.REPO, "Tools", "Boss2Model", "export", "Boss2.fbx"))
    tex = os.path.join(ml.MODEL_DIR, "Boss2.fbm")
    mat = ml.game_material(
        "G_Boss2",
        os.path.join(tex, "Boss2_Base.png"),
        os.path.join(tex, "Boss2_Normal.png"),
        os.path.join(tex, "Boss2_Emission.png"),
        os.path.join(tex, "Boss2_Metallic.png"),
    )
    arm = bpy.data.objects["Boss2_Armature"]
    for o in bpy.data.objects:
        if o.type == "MESH" and o.name.startswith("Boss2"):
            o.data.materials.clear()
            o.data.materials.append(mat)
    bpy.data.objects["Boss2_PodR_Broken"].hide_render = True
    root = bpy.data.objects.new("BossRoot", None)
    bpy.context.scene.collection.objects.link(root)
    arm.parent = root
    act = next(a for a in bpy.data.actions if a.name.endswith("|Idle"))
    arm.animation_data_create()
    arm.animation_data.action = act
    return root, arm


def main():
    scn, cam = setup_scene()
    equi = ml.make_sky_equirect(os.path.join(tempfile.gettempdir(), "BossAppear2_sky_equirect.png"))
    ml.setup_world(equi)
    root, arm = setup_boss()
    n = ml.build_city(-6000.0, 10400.0)
    print("city pieces:", n)
    rain_ob = ml.make_rain_object()
    rain = ml.Rain()
    os.makedirs(OUT, exist_ok=True)

    # 雨の下準備(ゲームでは雨はWARNINGの前から降っている)
    cam0 = cam_shot(0)[0]
    for _ in range(120):
        rain.step(cam0)

    for f in range(TOTAL):
        pos, tgt, fov, amp = cam_shot(f)
        # 揺れ(最後の0.5秒は0に向かって消す)
        t = f / FPS
        w = max(0.0, min(1.0, (SHAKE_END_FRAME - f) / 6.0))
        a = amp * w
        pos_b = ml.g2b(pos)
        tgt_b = ml.g2b(tgt)
        d = (tgt_b - pos_b).normalized()
        cam.location = pos_b
        cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
        if a > 0:
            e = cam.rotation_euler
            cam.rotation_euler = Euler((e.x + math.radians(a * noise(t, 1)), e.y + math.radians(a * 0.5 * noise(t, 2)),
                                        e.z + math.radians(a * noise(t, 3))), e.order)
            cam.location = pos_b + Vector((noise(t, 4), noise(t, 5), noise(t, 6))) * (a * 0.05)
        cam.data.angle = math.radians(fov)

        # 雨: ゲーム2フレーム(60fps)ぶん進める。エフェクトはカメラの位置から出る
        cam_game = (cam.location.x * ml.U, cam.location.z * ml.U, cam.location.y * ml.U)
        for _ in range(2):
            rain.step(cam_game)
        if f not in FRAMES:
            continue
        v, rgba = rain.build_mesh(tuple(cam.location), None, None)
        ml.update_rain_mesh(rain_ob, v, rgba)

        # ボス
        bp, _, (pitch, roll, yaw) = boss_state(t)
        root.location = ml.g2b(bp)
        root.rotation_euler = Euler((pitch, roll, yaw), "XYZ")
        # Idleは、最後のフレームでアニメーションの0フレーム目(ゲームのボスの最初のポーズ)になるようにそろえる
        af = 1.0 + ((f - (TOTAL - 1)) * 2.0) % 120.0
        scn.frame_set(int(af), subframe=af - int(af))
        scn.render.filepath = os.path.join(OUT, "f%04d.png" % f)
        bpy.ops.render.render(write_still=True)
        print("rendered", f)


main()

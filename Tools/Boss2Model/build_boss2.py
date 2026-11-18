# 2面のボスのモデルを作るスクリプト(Blender 5.x)
# 仕様は BOSS2_SPEC.md の「5. モデル」。
# 単位: Blenderの1m = ゲームの100。正面(プレイヤーを向く側)はBlenderの -Y、上は +Z。
# 何度実行しても同じものができる(前に作ったBoss2のものは消して作り直す)。

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
    # 起動直後の Cube / Camera / Light は使わないので消す
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
        if ob.name.startswith("Boss2") or ob.name.startswith("Preview"):
            bpy.data.objects.remove(ob, do_unlink=True)
    names = ("Idle", "NetReady", "NetFire", "VentOpen", "VentLoop", "VentClose",
             "RocketOpen", "RocketClose", "Stagger", "ArmatureAction", "Boss2_ArmatureAction")
    for a in list(bpy.data.actions):
        if a.name.split(".")[0] in names:
            bpy.data.actions.remove(a)
    for coll_data in (bpy.data.meshes, bpy.data.armatures, bpy.data.actions,
                      bpy.data.cameras, bpy.data.lights):
        for d in list(coll_data):
            if d.users == 0:
                coll_data.remove(d)
    for m in list(bpy.data.materials):
        if m.name.startswith("B2_") and m.users == 0:
            bpy.data.materials.remove(m)


def get_collection():
    coll = bpy.data.collections.new(COLL_NAME)
    bpy.context.scene.collection.children.link(coll)
    return coll


# ------------------------------------------------------------
# マテリアル
# ------------------------------------------------------------
def add_panel_detail(m, base):
    """ベース色にパネルの継ぎ目(暗い線)と薄い汚れのむらを入れる(テクスチャに焼き付ける)"""
    nt = m.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tc = nt.nodes.new("ShaderNodeTexCoord")
    vor = nt.nodes.new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = 0.32
    nt.links.new(tc.outputs["Object"], vor.inputs["Vector"])
    edge = nt.nodes.new("ShaderNodeMapRange")
    edge.inputs["From Min"].default_value = 0.0
    edge.inputs["From Max"].default_value = 0.03
    edge.inputs["To Min"].default_value = 0.45
    edge.inputs["To Max"].default_value = 1.0
    edge.clamp = True
    nt.links.new(vor.outputs["Distance"], edge.inputs["Value"])
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 0.6
    noise.inputs["Detail"].default_value = 6.0
    nt.links.new(tc.outputs["Object"], noise.inputs["Vector"])
    grime = nt.nodes.new("ShaderNodeMapRange")
    grime.inputs["From Min"].default_value = 0.35
    grime.inputs["From Max"].default_value = 0.65
    grime.inputs["To Min"].default_value = 0.8
    grime.inputs["To Max"].default_value = 1.18
    grime.clamp = True
    nt.links.new(noise.outputs["Fac"], grime.inputs["Value"])
    mul = nt.nodes.new("ShaderNodeMath")
    mul.operation = "MULTIPLY"
    nt.links.new(edge.outputs["Result"], mul.inputs[0])
    nt.links.new(grime.outputs["Result"], mul.inputs[1])
    sc = nt.nodes.new("ShaderNodeVectorMath")
    sc.operation = "SCALE"
    sc.inputs[0].default_value = base
    nt.links.new(mul.outputs["Value"], sc.inputs["Scale"])
    nt.links.new(sc.outputs["Vector"], bsdf.inputs["Base Color"])


def make_mat(name, base, metal=0.0, rough=0.5, emit=None, emit_strength=0.0, detail=False):
    m = bpy.data.materials.get(name)
    if m is not None:
        bpy.data.materials.remove(m)
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*base, 1.0)
    if detail:
        add_panel_detail(m, base)
    bsdf.inputs["Metallic"].default_value = metal
    bsdf.inputs["Roughness"].default_value = rough
    if emit is not None:
        bsdf.inputs["Emission Color"].default_value = (*emit, 1.0)
        bsdf.inputs["Emission Strength"].default_value = emit_strength
    else:
        bsdf.inputs["Emission Strength"].default_value = 0.0
    m.diffuse_color = (*base, 1.0)
    return m


def build_materials():
    return {
        # 本体: 曇り空(地平線 168,174,183)より十分暗い金属
        # ベース色の明るさは1面のボスのテクスチャ(平均74/255)に近づけた。曇り空(168)よりは暗い
        "hull": make_mat("B2_Hull", (0.12, 0.13, 0.145), 0.75, 0.42, detail=True),
        "hull_dark": make_mat("B2_HullDark", (0.055, 0.06, 0.068), 0.7, 0.5, detail=True),
        "accent": make_mat("B2_Accent", (0.28, 0.29, 0.31), 0.6, 0.35, detail=True),
        "hazard": make_mat("B2_Hazard", (0.55, 0.30, 0.04), 0.2, 0.5),
        # 光る所。左上ポッド=紫寄り、右上ポッド=青寄り(どの線を出すかの対応)
        # 光の強さは色が白く飛ばない程度(左右の色の違いが分かるように)
        "glow_l": make_mat("B2_GlowL", (0.5, 0.25, 0.9), 0.0, 0.3, (0.62, 0.22, 1.0), 2.2),
        "glow_r": make_mat("B2_GlowR", (0.2, 0.5, 0.9), 0.0, 0.3, (0.15, 0.55, 1.0), 2.2),
        "eye": make_mat("B2_Eye", (0.8, 0.95, 1.0), 0.0, 0.3, (0.6, 0.95, 1.0), 3.0),
        "engine": make_mat("B2_Engine", (0.6, 0.9, 1.0), 0.0, 0.3, (0.45, 0.8, 1.0), 2.5),
        # 放熱口の中(閉じている間は装甲の下に隠れる)
        "vent": make_mat("B2_Vent", (0.6, 0.03, 0.02), 0.0, 0.4, (1.0, 0.025, 0.015), 2.6),
        "scorch": make_mat("B2_Scorch", (0.018, 0.016, 0.015), 0.3, 0.8),
    }


# ------------------------------------------------------------
# 形を作る道具
# ------------------------------------------------------------
def octagon(w, h, c):
    """幅±w・高さ±h・角の面取りcの八角形(反時計回り)"""
    return [(-w, -h + c), (-w + c, -h), (w - c, -h), (w, -h + c),
            (w, h - c), (w - c, h), (-w + c, h), (-w, h - c)]


def loft(bm, sections, axis="Y", cap=True):
    """断面をつないだ筒。sections = [(t, w, h, c, u0, v0), ...]
    axis="Y": 点=(u0+u, t, v0+v)、axis="X": 点=(t, u0+u, v0+v)"""
    rings = []
    for (t, w, h, c, u0, v0) in sections:
        ring = []
        for (u, v) in octagon(w, h, c):
            if axis == "Y":
                p = (u0 + u, t, v0 + v)
            else:
                p = (t, u0 + u, v0 + v)
            ring.append(bm.verts.new(p))
        rings.append(ring)
    n = len(rings[0])
    for a, b in zip(rings[:-1], rings[1:]):
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((a[i], a[j], b[j], b[i]))
    if cap:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])


def box(bm, center, size, rot=None):
    mat = Matrix.Translation(center)
    if rot is not None:
        mat = mat @ rot
    mat = mat @ Matrix.Diagonal((size[0], size[1], size[2], 1.0))
    bmesh.ops.create_cube(bm, size=1.0, matrix=mat)


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


def finish_object(coll, name, bm, mat, bone, bevel=0.0, transform=None):
    if transform is not None:
        bmesh.ops.transform(bm, matrix=transform, verts=bm.verts)
    # 反転を含む変形でも外向きになるよう、変形の後に向きをそろえる
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
        mod.segments = 2
        mod.limit_method = "ANGLE"
        mod.angle_limit = math.radians(35)
        mod.harden_normals = False
        dg = bpy.context.evaluated_depsgraph_get()
        ev = ob.evaluated_get(dg)
        new_me = bpy.data.meshes.new_from_object(ev)
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
        bmesh.ops.triangulate(bm3, faces=ngons)
    bm3.to_mesh(ob.data)
    bm3.free()
    for poly in ob.data.polygons:
        poly.use_smooth = False
    vg = ob.vertex_groups.new(name=bone)
    vg.add(list(range(len(ob.data.vertices))), 1.0, "REPLACE")
    return ob


# ------------------------------------------------------------
# 部品の位置(Blenderの座標、m)
# ------------------------------------------------------------
POD_X = 12.6       # ポッドの中心の左右(ゲームで ±1260)
POD_Z = 4.4        # ポッドの中心の高さ(ゲームで +440)
SHOULDER_X = 4.0
SHOULDER_Z = 1.5
HINGE_Y = 2.0      # 放熱の装甲の付け根(後ろ側)
POD_R_ = 1.7       # ポッドの太さ(半径)


def build_body(coll, M):
    parts = []
    # 胴体: 前が細く、中ほどが太い八角形の筒
    bm = bmesh.new()
    loft(bm, [
        (-7.4, 1.0, 0.7, 0.35, 0.0, -0.3),
        (-6.0, 2.6, 1.5, 0.7, 0.0, -0.1),
        (-3.0, 4.0, 2.3, 1.0, 0.0, 0.0),
        (2.5, 4.4, 2.5, 1.1, 0.0, 0.1),
        (6.0, 3.6, 2.1, 0.9, 0.0, 0.0),
        (7.4, 2.5, 1.5, 0.6, 0.0, -0.1),
    ], axis="Y")
    parts.append(finish_object(coll, "B2p_Body", bm, M["hull"], "Body", bevel=0.08))

    # 背中の稜線と装甲板
    bm = bmesh.new()
    box(bm, (0.0, 0.5, 2.75), (1.6, 9.0, 0.7))
    box(bm, (0.0, -3.6, 2.35), (2.6, 2.2, 0.5))
    parts.append(finish_object(coll, "B2p_Spine", bm, M["accent"], "Body", bevel=0.06))

    # 横の装甲(左右)
    for sx in (-1, 1):
        bm = bmesh.new()
        rot = Matrix.Rotation(math.radians(-12 * sx), 4, "Y")
        box(bm, (sx * 4.45, 0.0, -0.4), (0.5, 8.5, 2.6), rot)
        parts.append(finish_object(coll, "B2p_SideArmor%d" % sx, bm, M["hull_dark"], "Body", bevel=0.06))

    # 顔: 前面の横長の目(光る線)と、その上のひさし
    # 目: 内側が下がった斜めの2本(にらんでいる顔に見えるように)
    bm = bmesh.new()
    for sx in (-1, 1):
        rot = Matrix.Rotation(math.radians(14 * sx), 4, "Y")
        box(bm, (sx * 1.05, -6.75, 0.32), (1.5, 0.5, 0.24), rot)
    parts.append(finish_object(coll, "B2p_Eye", bm, M["eye"], "Body"))
    bm = bmesh.new()
    rot = Matrix.Rotation(math.radians(-18), 4, "X")
    box(bm, (0.0, -5.9, 0.95), (5.2, 1.4, 0.35), rot)
    parts.append(finish_object(coll, "B2p_Brow", bm, M["hull_dark"], "Body", bevel=0.05))

    # 後ろのエンジン(左右)と噴射口の光
    for sx in (-1, 1):
        bm = bmesh.new()
        cylinder(bm, (sx * 1.9, 7.6, -0.2), 1.05, 1.6, axis="Y", segments=16, radius2=1.25)
        parts.append(finish_object(coll, "B2p_Engine%d" % sx, bm, M["hull_dark"], "Body", bevel=0.04))
        bm = bmesh.new()
        cylinder(bm, (sx * 1.9, 8.42, -0.2), 0.85, 0.08, axis="Y", segments=16)
        parts.append(finish_object(coll, "B2p_EngineGlow%d" % sx, bm, M["engine"], "Body"))

    # 後ろの縦のひれ(少し外に傾ける)
    for sx in (-1, 1):
        bm = bmesh.new()
        v = [bm.verts.new(p) for p in [
            (0.0, 3.0, 0.0), (0.0, 7.0, 0.0), (0.0, 8.0, 2.2), (0.0, 6.4, 2.2)]]
        v2 = [bm.verts.new((0.25, p.co.y, p.co.z)) for p in v]
        bm.faces.new(v)
        bm.faces.new(list(reversed(v2)))
        for i in range(4):
            j = (i + 1) % 4
            bm.faces.new((v[i], v2[i], v2[j], v[j]))
        tr = (Matrix.Translation((sx * 2.6 - 0.125, 0.0, 2.3)) @
              Matrix.Rotation(math.radians(18 * sx), 4, "Y"))
        parts.append(finish_object(coll, "B2p_Fin%d" % sx, bm, M["accent"], "Body",
                                   bevel=0.03, transform=tr))

    # 下の浮遊用の噴射口(4つ)
    for sx in (-1, 1):
        for y in (-1.0, 4.5):
            bm = bmesh.new()
            cylinder(bm, (sx * 3.0, y, -2.45), 0.75, 0.5, axis="Z", segments=12)
            parts.append(finish_object(coll, "B2p_Hover%d_%d" % (sx, int(y)), bm, M["hull_dark"], "Body", bevel=0.03))
            bm = bmesh.new()
            cylinder(bm, (sx * 3.0, y, -2.72), 0.55, 0.06, axis="Z", segments=12)
            parts.append(finish_object(coll, "B2p_HoverGlow%d_%d" % (sx, int(y)), bm, M["engine"], "Body"))

    # ロケットの発射口(胴体の下の前): 4本の筒と、その前を覆うハッチ
    bm = bmesh.new()
    box(bm, (0.0, -3.0, -2.55), (3.6, 3.4, 0.7))
    parts.append(finish_object(coll, "B2p_RocketRack", bm, M["hull_dark"], "Body", bevel=0.04))
    for i, x in enumerate((-1.2, -0.4, 0.4, 1.2)):
        bm = bmesh.new()
        cylinder(bm, (x, -4.55, -2.6), 0.3, 0.4, axis="Y", segments=10)
        parts.append(finish_object(coll, "B2p_RocketTube%d" % i, bm, M["scorch"], "Body"))
    bm = bmesh.new()
    box(bm, (0.0, -3.0, -2.98), (3.8, 3.6, 0.18))
    parts.append(finish_object(coll, "B2p_RocketHatch", bm, M["accent"], "RocketHatch", bevel=0.03))
    for i, x in enumerate((-1.3, 1.3)):
        bm = bmesh.new()
        box(bm, (x, -4.55, -2.98), (0.6, 0.35, 0.2))
        parts.append(finish_object(coll, "B2p_HatchStripe%d" % i, bm, M["hazard"], "RocketHatch"))
    return parts


def build_arm(coll, M, side):
    """side: -1=左, +1=右。肩からポッドの付け根まで斜め上へ伸びる腕"""
    name = "Arm_L" if side < 0 else "Arm_R"
    sx = side
    start = Vector((sx * SHOULDER_X, 0.4, SHOULDER_Z))
    end = Vector((sx * (POD_X - 1.4), 0.4, POD_Z - 0.2))
    d = end - start
    length = d.length
    angle = math.atan2(d.z, abs(d.x))
    bm = bmesh.new()
    # X方向に作ってから傾ける(根元が太く、先が細い)
    loft(bm, [
        (0.0, 2.6, 1.1, 0.45, 0.0, 0.0),
        (length * 0.45, 2.0, 0.85, 0.35, 0.0, 0.0),
        (length, 1.5, 0.7, 0.3, 0.0, 0.0),
    ], axis="X")
    tr = (Matrix.Translation(start) @ Matrix.Rotation(-angle * sx, 4, "Y") @
          Matrix.Scale(sx if sx < 0 else 1, 4, (1, 0, 0)))
    parts = [finish_object(coll, "B2p_%s" % name, bm, M["hull"], name, bevel=0.06, transform=tr)]
    # 腕の上の補強板
    bm = bmesh.new()
    box(bm, (length * 0.5, 0.0, 0.85), (length * 0.6, 1.4, 0.3))
    parts.append(finish_object(coll, "B2p_%sPlate" % name, bm, M["accent"], name, bevel=0.04, transform=tr))
    return parts


def build_pod(coll, M, side, broken=False):
    """網の発射口のポッド。side: -1=左上, +1=右上"""
    tag = "L" if side < 0 else "R"
    cx = side * POD_X
    glow = M["glow_l"] if side < 0 else M["glow_r"]
    pod_bone = "Pod_" + tag
    parts = []
    if broken:
        # 壊れた右ポッド: 後ろ半分だけが残り、前はぎざぎざに割れている
        bone = "Pod_R_Broken"
        bm = bmesh.new()
        loft(bm, [
            (0.2, POD_R_ * 0.95, POD_R_ * 0.9, 0.55, cx, POD_Z),
            (2.0, POD_R_, POD_R_ * 0.95, 0.6, cx, POD_Z),
            (3.4, 1.1, 1.0, 0.4, cx, POD_Z),
        ], axis="Y")
        parts.append(finish_object(coll, "B2p_PodR_BrokenShell", bm, M["scorch"], bone, bevel=0.04))
        for i, (dx, dz, ln, rz) in enumerate([(-0.9, 0.9, 1.6, 20), (0.6, 1.1, 1.1, -15),
                                              (1.1, -0.7, 1.4, 35), (-0.5, -1.0, 0.9, -30)]):
            bm = bmesh.new()
            rot = Matrix.Rotation(math.radians(rz), 4, "Z") @ Matrix.Rotation(math.radians(25), 4, "X")
            box(bm, (cx + dx, 0.2 - ln * 0.4, POD_Z + dz), (0.5, ln, 0.18), rot)
            parts.append(finish_object(coll, "B2p_PodR_Shard%d" % i, bm, M["hull_dark"], bone))
        bm = bmesh.new()
        cylinder(bm, (cx, 0.15, POD_Z), 1.0, 0.1, axis="Y", segments=10)
        parts.append(finish_object(coll, "B2p_PodR_BrokenCore", bm, M["vent"], bone))
        return parts

    # ポッド本体。装甲の下(放熱口)は細くへこませて、閉じたときに中身が見えないようにする
    bm = bmesh.new()
    loft(bm, [
        (-3.5, 0.95, 0.95, 0.35, cx, POD_Z),
        (-2.9, POD_R_ * 0.9, POD_R_ * 0.9, 0.55, cx, POD_Z),
        # ふちは装甲の内側(1.72)より少し太く(1.78)して、装甲の厚みの中に食い込ませる
        (-1.35, POD_R_ + 0.08, POD_R_ + 0.08, 0.6, cx, POD_Z),
        (-1.3, 1.2, 1.2, 0.45, cx, POD_Z),
        (2.0, 1.2, 1.2, 0.45, cx, POD_Z),
        (2.05, POD_R_ + 0.08, POD_R_ + 0.08, 0.6, cx, POD_Z),
        (2.2, POD_R_, POD_R_ * 0.95, 0.6, cx, POD_Z),
        (3.5, 1.1, 1.0, 0.4, cx, POD_Z),
    ], axis="Y")
    parts.append(finish_object(coll, "B2p_Pod" + tag, bm, M["hull"], pod_bone, bevel=0.06))
    # 前の発射口(輪と光るレンズ)
    bm = bmesh.new()
    cylinder(bm, (cx, -3.75, POD_Z), 0.85, 0.5, axis="Y", segments=16)
    parts.append(finish_object(coll, "B2p_PodMuzzle" + tag, bm, M["hull_dark"], pod_bone, bevel=0.03))
    bm = bmesh.new()
    cylinder(bm, (cx, -4.03, POD_Z), 0.62, 0.08, axis="Y", segments=16)
    parts.append(finish_object(coll, "B2p_PodLens" + tag, bm, glow, pod_bone))
    # 側面の光る線(どちらのポッドか遠くからでも分かるように)
    for sz in (-1, 1):
        bm = bmesh.new()
        box(bm, (cx + side * (POD_R_ + 0.02), -1.9, POD_Z + sz * 0.45), (0.08, 1.4, 0.18))
        parts.append(finish_object(coll, "B2p_PodStripe%s%d" % (tag, sz), bm, glow, pod_bone))
    # 放熱口の中身(装甲の下。開いたときに赤く光る格子)
    bm = bmesh.new()
    for k in range(5):
        y = -0.95 + k * 0.7
        # 八角形の薄い板(へこみ 1.2 から少し出て、装甲の内側 1.72 には届かない)
        loft(bm, [(y, 1.55, 1.55, 0.6, cx, POD_Z), (y + 0.24, 1.55, 1.55, 0.6, cx, POD_Z)], axis="Y")
    parts.append(finish_object(coll, "B2p_PodVent" + tag, bm, M["vent"], pod_bone))
    # 放熱の装甲(上下2枚)。付け根(後ろ HINGE_Y)を軸に開く
    for half, bone_suffix in ((1, "Top"), (-1, "Bottom")):
        bm = bmesh.new()
        w = POD_R_ + 0.16
        h0 = 0.0
        h1 = POD_R_ + 0.16
        prof = []
        # 半分の八角形の殻(厚さ0.14)
        outer = [(-w, h0), (-w, h1 - 0.62), (-w + 0.62, h1), (w - 0.62, h1), (w, h1 - 0.62), (w, h0)]
        inner = [(-w + 0.14, h0), (-w + 0.14, h1 - 0.68), (-w + 0.68, h1 - 0.14),
                 (w - 0.68, h1 - 0.14), (w - 0.14, h1 - 0.68), (w - 0.14, h0)]
        prof = outer + list(reversed(inner))
        rings = []
        # 前後のふち(-1.35〜-1.3、2.0〜2.2)より外まで覆って、すき間から中が見えないようにする
        for y in (-1.5, HINGE_Y + 0.25):
            rings.append([bm.verts.new((cx + u, y, POD_Z + half * v)) for (u, v) in prof])
        n = len(prof)
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((rings[0][i], rings[0][j], rings[1][j], rings[1][i]))
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[1])
        bone = "PodArmor_%s_%s" % (tag, bone_suffix)
        parts.append(finish_object(coll, "B2p_PodArmor%s%s" % (tag, bone_suffix), bm, M["accent"], bone, bevel=0.03))
    return parts


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
    ("Arm_L", (-SHOULDER_X, 0.4, SHOULDER_Z), "Body"),
    ("Arm_R", (SHOULDER_X, 0.4, SHOULDER_Z), "Body"),
    ("Pod_L", (-POD_X, 0.0, POD_Z), "Arm_L"),
    ("Pod_R", (POD_X, 0.0, POD_Z), "Arm_R"),
    ("Pod_R_Broken", (POD_X, 0.0, POD_Z), "Arm_R"),
    ("PodArmor_L_Top", (-POD_X, HINGE_Y + 0.25, POD_Z + POD_R_), "Pod_L"),
    ("PodArmor_L_Bottom", (-POD_X, HINGE_Y + 0.25, POD_Z - POD_R_), "Pod_L"),
    ("PodArmor_R_Top", (POD_X, HINGE_Y + 0.25, POD_Z + POD_R_), "Pod_R"),
    ("PodArmor_R_Bottom", (POD_X, HINGE_Y + 0.25, POD_Z - POD_R_), "Pod_R"),
    ("RocketHatch", (0.0, -1.2, -2.98), "Body"),
    # 目印(形なし)
    ("Muzzle_Net_L", (-POD_X, -4.1, POD_Z), "Pod_L"),
    ("Muzzle_Net_R", (POD_X, -4.1, POD_Z), "Pod_R"),
    ("Muzzle_Rocket_0", (-1.2, -4.8, -2.6), "Body"),
    ("Muzzle_Rocket_1", (-0.4, -4.8, -2.6), "Body"),
    ("Muzzle_Rocket_2", (0.4, -4.8, -2.6), "Body"),
    ("Muzzle_Rocket_3", (1.2, -4.8, -2.6), "Body"),
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
        bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.004,
                                 area_weight=0.0, correct_aspect=True, scale_to_bounds=False)
        bpy.ops.uv.pack_islands(margin=0.004, rotate=True, shape_method="CONCAVE")
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
        # 本体: 上下に揺れる(±20)、少し傾く
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
    # ロケットのハッチ: 開く 8F / 閉じる 12F(前が下へ開く)
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
    main_parts = build_body(coll, M)
    main_parts += build_arm(coll, M, -1)
    main_parts += build_arm(coll, M, 1)
    main_parts += build_pod(coll, M, -1)
    pod_r_parts = build_pod(coll, M, 1)
    broken_parts = build_pod(coll, M, 1, broken=True)

    main_ob = join(main_parts, "Boss2_Main")
    pod_r_ob = join(pod_r_parts, "Boss2_PodR")
    broken_ob = join(broken_parts, "Boss2_PodR_Broken")

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
    # 置いておく(画面と書き出しの既定)のは待機
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

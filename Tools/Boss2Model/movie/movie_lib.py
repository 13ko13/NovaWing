# 2面ボス登場ムービー用の部品(Blender 4.5 / 5.x、バックグラウンド実行)
#
# 座標: ゲーム(x, y, z) → Blender (x, z, y) / 100 。1 Blenderユニット = ゲームの100。
#   ボスの正面はゲームの -Z = Blenderの -Y。ゲームのFBX読み込みはZを反転するので、
#   FBXをBlenderの標準設定で読んだ見た目がそのままゲームの向きになる。
# 見た目はゲームのシェーダー(LightingPS.hlsl / LightingCommon.hlsli)と同じ式。色変換なし(Raw)。

import math
import os

import bpy
import numpy as np
from mathutils import Vector

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
MODEL_DIR = os.path.join(REPO, "Data", "Model")
SKY_DIR = os.path.join(REPO, "Data", "Image", "SkyBoxCloudy")
RAIN_TEX = os.path.join(REPO, "Data", "Effect", "Rain", "Texture", "Rain_Streak.png")
CSV_DIR = os.path.join(REPO, "Data", "CSV", "Stage2")

U = 100.0  # ゲームの長さ → Blenderユニット
LIGHT_DXLIB = (-0.332, -0.473, 0.816)  # Game::light_direction
AMBIENT = 0.5
NORMAL_STRENGTH = 1.5
SMOOTH_MIN, SMOOTH_RANGE = 1.0, 34.0


def g2b(p):
    """ゲーム座標 → Blender座標"""
    return Vector((p[0] / U, p[2] / U, p[1] / U))


LIGHT = Vector((LIGHT_DXLIB[0], LIGHT_DXLIB[2], LIGHT_DXLIB[1])).normalized()


# ---------------------------------------------------------------- 画像
_IMG = {}


def load_image(path, colorspace="Non-Color"):
    key = os.path.normcase(path)
    if key in _IMG:
        return _IMG[key]
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = colorspace
    img.alpha_mode = "CHANNEL_PACKED"
    _IMG[key] = img
    return img


# ---------------------------------------------------------------- ゲームのシェーダー
def game_material(name, albedo, normal=None, emission=None, metallic=None):
    """LightingPS.hlsl と同じ式のマテリアル。albedo/normal/emission/metallic は画像パス(Noneなら既定値)"""
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    N, L = nt.nodes.new, nt.links.new

    def vmath(op, a=None, b=None, vb=None):
        n = N("ShaderNodeVectorMath")
        n.operation = op
        if a is not None:
            L(a, n.inputs[0])
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

    def tex(path):
        t = N("ShaderNodeTexImage")
        t.image = load_image(path)
        t.interpolation = "Linear"
        L(uv.outputs["UV"], t.inputs["Vector"])
        return t

    base = tex(albedo)
    geo = N("ShaderNodeNewGeometry")
    nN = vmath("NORMALIZE", geo.outputs["Normal"])
    if normal:
        nrm = tex(normal)
        sep = N("ShaderNodeSeparateXYZ")
        L(nrm.outputs["Color"], sep.inputs[0])
        c = [math_("MULTIPLY_ADD", sep.outputs[i], None, vb=2.0) for i in range(3)]
        for x in c:
            x.inputs[2].default_value = -1.0
        nx2 = math_("MULTIPLY", c[0].outputs[0], None, vb=NORMAL_STRENGTH)
        ny2 = math_("MULTIPLY", c[1].outputs[0], None, vb=-NORMAL_STRENGTH)  # DirectX式(緑=下向き)
        tan = N("ShaderNodeTangent")
        tan.direction_type = "UV_MAP"
        tan.uv_map = "UVMap"
        nT = vmath("NORMALIZE", tan.outputs["Tangent"])
        nB = vmath("NORMALIZE", vmath("CROSS_PRODUCT", nN.outputs[0], nT.outputs[0]).outputs[0])
        tx = vmath("SCALE", nT.outputs[0])
        L(nx2.outputs[0], tx.inputs["Scale"])
        by = vmath("SCALE", nB.outputs[0])
        L(ny2.outputs[0], by.inputs["Scale"])
        nzv = vmath("SCALE", nN.outputs[0])
        L(c[2].outputs[0], nzv.inputs["Scale"])
        nsum = vmath("ADD", vmath("ADD", tx.outputs[0], by.outputs[0]).outputs[0], nzv.outputs[0])
        nf = vmath("NORMALIZE", nsum.outputs[0])
    else:
        nf = nN

    dif = vmath("DOT_PRODUCT", nf.outputs[0], None, vb=tuple(-c for c in LIGHT))
    dif_c = math_("MAXIMUM", dif.outputs["Value"], None, vb=0.0)
    dif_c = math_("MINIMUM", dif_c.outputs[0], None, vb=1.0)
    light = math_("ADD", dif_c.outputs[0], None, vb=AMBIENT, clamp=True)

    refl = N("ShaderNodeVectorMath")
    refl.operation = "REFLECT"
    refl.inputs[0].default_value = tuple(LIGHT)
    L(nf.outputs[0], refl.inputs[1])
    vd = vmath("DOT_PRODUCT", geo.outputs["Incoming"], refl.outputs[0])
    spec = math_("MAXIMUM", vd.outputs["Value"], None, vb=0.0)
    spec = math_("MINIMUM", spec.outputs[0], None, vb=1.0)
    if metallic:
        met = tex(metallic)
        msep = N("ShaderNodeSeparateColor")
        L(met.outputs["Color"], msep.inputs[0])
        sm = math_("MULTIPLY_ADD", msep.outputs[0], None, vb=SMOOTH_RANGE)
        sm.inputs[2].default_value = SMOOTH_MIN
        smooth = sm.outputs[0]
    else:
        smooth = None
    if smooth is not None:
        spec = math_("POWER", spec.outputs[0], smooth)
    else:
        spec = math_("POWER", spec.outputs[0], None, vb=SMOOTH_MIN)
    spec = math_("MULTIPLY", spec.outputs[0], dif_c.outputs[0])
    spec = math_("MULTIPLY", spec.outputs[0], base.outputs["Alpha"])

    lit = vmath("SCALE", base.outputs["Color"])
    L(light.outputs[0], lit.inputs["Scale"])
    spec_v = N("ShaderNodeCombineXYZ")
    for i in range(3):
        L(spec.outputs[0], spec_v.inputs[i])
    col = vmath("ADD", lit.outputs[0], spec_v.outputs[0])
    if emission:
        emi = tex(emission)
        col = vmath("ADD", col.outputs[0], emi.outputs["Color"])

    em = N("ShaderNodeEmission")
    L(col.outputs[0], em.inputs["Color"])
    out = N("ShaderNodeOutputMaterial")
    L(em.outputs[0], out.inputs["Surface"])
    return m


# ---------------------------------------------------------------- 空(SkyBox::Draw と同じ面・UV)
# 各面: (ファイル, 左上, 右上, 左下) 。カメラ中心・半辺1(ゲームは1500)
SKY_FACES = [
    ("front", (-1, 1, 1), (1, 1, 1), (-1, -1, 1)),
    ("back", (1, 1, -1), (-1, 1, -1), (1, -1, -1)),
    ("left", (-1, 1, -1), (-1, 1, 1), (-1, -1, -1)),
    ("right", (1, 1, 1), (1, 1, -1), (1, -1, 1)),
    ("up", (-1, 1, -1), (1, 1, -1), (-1, 1, 1)),
    ("bottom", (-1, -1, 1), (1, -1, 1), (-1, -1, -1)),
]


def _read_png_topdown(path):
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = "Non-Color"
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    a = a.reshape(h, w, 4)[::-1]  # 上の行が先頭
    bpy.data.images.remove(img)
    return a


def make_sky_equirect(out_path, width=6144):
    """6枚の空を正距円筒画像にする(Blenderのワールドで使う)。無ければ作る"""
    if os.path.exists(out_path):
        return out_path
    h = width // 2
    # 行0 = 下(Blender画像は下から)。緯度 -90..90
    th = (np.arange(h) + 0.5) / h * math.pi - math.pi / 2
    ph = ((np.arange(width) + 0.5) / width - 0.5) * 2 * math.pi
    TH, PH = np.meshgrid(th, ph, indexing="ij")
    # Blender方向 → ゲーム方向(x, y, z) = (bx, bz, by)
    bx = -np.cos(TH) * np.cos(PH)
    by = np.cos(TH) * np.sin(PH)
    bz = np.sin(TH)
    d = np.stack([bx, bz, by], axis=-1)  # ゲーム座標
    ad = np.abs(d)
    axis = np.argmax(ad, axis=-1)
    sign = np.take_along_axis(d, axis[..., None], -1)[..., 0] >= 0
    out = np.zeros((h, width, 4), dtype=np.float32)
    out[..., 3] = 1.0
    for name, lt, rt, lb in SKY_FACES:
        lt, rt, lb = (np.array(v, dtype=np.float32) for v in (lt, rt, lb))
        n = np.cross(rt - lt, lb - lt)
        ax = int(np.argmax(np.abs(n)))
        sg = lt[ax] > 0
        mask = (axis == ax) & (sign == sg)
        dm = d[mask]
        p = dm / np.abs(dm[:, ax:ax + 1]) * (1.0 if True else 1.0)
        e1, e2 = rt - lt, lb - lt
        u = ((p - lt) @ e1) / (e1 @ e1)
        v = ((p - lt) @ e2) / (e2 @ e2)
        img = _read_png_topdown(os.path.join(SKY_DIR, "skybox_%s.png" % name))
        ih, iw = img.shape[:2]
        fx = np.clip(u * iw - 0.5, 0, iw - 1)
        fy = np.clip(v * ih - 0.5, 0, ih - 1)
        x0 = np.floor(fx).astype(int)
        y0 = np.floor(fy).astype(int)
        x1 = np.minimum(x0 + 1, iw - 1)
        y1 = np.minimum(y0 + 1, ih - 1)
        wx = (fx - x0)[:, None]
        wy = (fy - y0)[:, None]
        c = (img[y0, x0] * (1 - wx) * (1 - wy) + img[y0, x1] * wx * (1 - wy)
             + img[y1, x0] * (1 - wx) * wy + img[y1, x1] * wx * wy)
        out[mask] = c
        out[mask, 3] = 1.0
    im = bpy.data.images.new("SkyEqui", width, h, alpha=False, float_buffer=False)
    im.colorspace_settings.name = "Non-Color"
    im.pixels.foreach_set(out.ravel())
    im.filepath_raw = out_path
    im.file_format = "PNG"
    im.save()
    bpy.data.images.remove(im)
    return out_path


def setup_world(equi_path):
    scn = bpy.context.scene
    w = scn.world or bpy.data.worlds.new("World")
    scn.world = w
    w.use_nodes = True
    nt = w.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    t = nt.nodes.new("ShaderNodeTexEnvironment")
    t.image = load_image(equi_path)
    t.interpolation = "Cubic"
    bg = nt.nodes.new("ShaderNodeBackground")
    out = nt.nodes.new("ShaderNodeOutputWorld")
    nt.links.new(t.outputs["Color"], bg.inputs["Color"])
    nt.links.new(bg.outputs[0], out.inputs[0])


# ---------------------------------------------------------------- 街(CSVの配置どおり)
LOOP_START = 34000.0
LAP = 16000.0
# ボス(ワールドz=0)のループ内の位置: ボスCSVのz=60460
BOSS_LOCAL_Z = LOOP_START + (60460.0 - LOOP_START) % LAP


def _import_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == "MESH"]
    for o in new:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    for o in meshes:
        o.parent = None
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for o in new:
        if o.type != "MESH":
            bpy.data.objects.remove(o, do_unlink=True)
    return meshes[0]


def _model_material(name, src_dir, fbx_mesh):
    """FBXのマテリアルが参照している画像から、ゲームの式のマテリアルを作る"""
    imgs = []
    for m in fbx_mesh.data.materials:
        if m and m.node_tree:
            for n in m.node_tree.nodes:
                if n.type == "TEX_IMAGE" and n.image:
                    imgs.append(bpy.path.abspath(n.image.filepath))
    albedo = next((p for p in imgs if p.lower().endswith(("_albedo.png", ".jpg"))), None)
    normal = next((p for p in imgs if p.lower().endswith("_normal.png")), None)
    emission = next((p for p in imgs if p.lower().endswith("_emission.png")), None)
    return game_material("G_" + name, albedo, normal, emission)


def read_csv(name):
    rows = []
    with open(os.path.join(CSV_DIR, name), encoding="utf-8-sig") as f:
        next(f)
        for line in f:
            c = line.strip().split(",")
            if len(c) >= 10:
                rows.append((c[0], [float(v) for v in c[1:10]]))
    return rows


def build_city(z_min, z_max):
    """ボス(ワールドz=0)から見て z_min〜z_max に入るループ区間の地面とビルを置く"""
    coll = bpy.data.collections.new("City")
    bpy.context.scene.collection.children.link(coll)
    protos = {}
    count = 0
    for model, v in read_csv("Building.csv"):
        x, y, z, rx, ry, rz, sx, sy, sz = v
        if z < LOOP_START:
            continue
        # 周回を足して、ワールドzが範囲に入るものを探す
        wz = z - BOSS_LOCAL_Z
        k = math.ceil((z_min - wz) / LAP)
        wz += k * LAP
        if wz >= z_max:
            continue
        if model not in protos:
            sub = "Building" if model.startswith("Building_") else "Road"
            p = _import_fbx(os.path.join(MODEL_DIR, sub, model + ".fbx"))
            p.name = "Proto_" + model
            mat = _model_material(model, sub, p)
            p.data.materials.clear()
            p.data.materials.append(mat)
            bpy.context.scene.collection.objects.unlink(p)
            protos[model] = p
        o = bpy.data.objects.new(model, protos[model].data)
        coll.objects.link(o)
        o.location = g2b((x, y, wz))
        o.rotation_euler = (0, 0, -math.radians(ry))
        o.scale = (sx, sz, sy)  # Blenderの(x, y, z) = ゲームの(x, z, y)
        count += 1
    return count


# ---------------------------------------------------------------- 雨(Data/Effect/Rain の再現)
# (名前, 毎フレームの生成数, 寿命(F), x範囲, y範囲, z範囲, 幅, 長さ, 色, アルファ)  単位はEffekseer値×50=ゲームの長さ
RAIN_LAYERS = [
    ("NearL", 1.0, 26, (-11, -2.5), (-3, 12), (-12, -3), 0.08, 2.6, (230, 235, 240), 170),
    ("NearR", 1.0, 26, (2.5, 11), (-3, 12), (-12, -3), 0.08, 2.6, (230, 235, 240), 170),
    ("Mid", 1 / 0.15, 34, (-28, 28), (-6, 22), (-36, -8), 0.07, 1.7, (225, 230, 236), 110),
    ("Far", 1 / 0.2, 50, (-70, 70), (-6, 40), (-110, -36), 0.14, 2.4, (185, 195, 210), 55),
]
RAIN_VEL = -0.825 * 50.0  # ゲームの長さ/フレーム(60fps)


class Rain:
    """雨粒をワールド空間で発生させ、60fpsで進める。カメラの位置から出る(エフェクトがカメラに追従するため)"""

    def __init__(self, seed=7):
        self.rng = np.random.default_rng(seed)
        self.pos = np.zeros((0, 3))  # ゲーム座標
        self.age = np.zeros(0)
        self.life = np.zeros(0)
        self.layer = np.zeros(0, dtype=int)
        self.acc = [0.0] * len(RAIN_LAYERS)

    def step(self, cam_game):
        """ゲーム1フレーム進める。cam_game: カメラのゲーム座標"""
        self.pos[:, 1] += RAIN_VEL
        self.age += 1
        keep = self.age < self.life
        self.pos, self.age, self.life, self.layer = self.pos[keep], self.age[keep], self.life[keep], self.layer[keep]
        for i, L in enumerate(RAIN_LAYERS):
            self.acc[i] += L[1]
            n = int(self.acc[i])
            self.acc[i] -= n
            if n <= 0:
                continue
            lo = np.array([L[3][0], L[4][0], L[5][0]]) * 50.0
            hi = np.array([L[3][1], L[4][1], L[5][1]]) * 50.0
            # Effekseer(右手系)のzは、ゲームでは符号が逆(奥が正)
            off = self.rng.uniform(lo, hi, (n, 3))
            off[:, 2] *= -1
            self.pos = np.vstack([self.pos, np.array(cam_game) + off])
            self.age = np.concatenate([self.age, np.zeros(n)])
            self.life = np.concatenate([self.life, self.rng.uniform(L[2] - 2, L[2] + 2, n)])
            self.layer = np.concatenate([self.layer, np.full(n, i)])

    def build_mesh(self, cam_pos_b, cam_right, cam_up):
        """カメラを向く細長い板(縦に長い)の頂点を返す: (verts[N*4,3], faces, uv, rgba)"""
        n = len(self.pos)
        w = np.array([RAIN_LAYERS[i][6] for i in self.layer]) * 50.0 / U
        ln = np.array([RAIN_LAYERS[i][7] for i in self.layer]) * 50.0 / U
        col = np.array([RAIN_LAYERS[i][8] for i in self.layer]) / 255.0
        alpha = np.array([RAIN_LAYERS[i][9] for i in self.layer]) / 255.0
        fade = np.minimum(np.minimum(self.age / 3.0, (self.life - self.age) / 4.0), 1.0).clip(0, 1)
        c = np.stack([self.pos[:, 0], self.pos[:, 2], self.pos[:, 1]], axis=1) / U
        # 縦(Blender Z)に伸びる。横はカメラへの向きと縦軸の外積
        to_cam = np.array(cam_pos_b) - c
        axis = np.array([0, 0, 1.0])
        side = np.cross(axis, to_cam)
        side /= np.maximum(np.linalg.norm(side, axis=1, keepdims=True), 1e-6)
        hw = side * (w[:, None] * 0.5)
        hl = axis[None, :] * (ln[:, None] * 0.5)
        v = np.stack([c - hw + hl, c + hw + hl, c + hw - hl, c - hw - hl], axis=1).reshape(-1, 3)
        rgba = np.repeat(np.concatenate([col, (alpha * fade)[:, None]], axis=1), 4, axis=0)
        return v, rgba


def make_rain_object():
    me = bpy.data.meshes.new("RainMesh")
    ob = bpy.data.objects.new("Rain", me)
    bpy.context.scene.collection.objects.link(ob)
    mat = bpy.data.materials.new("RainMat")
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    N, L = nt.nodes.new, nt.links.new
    t = N("ShaderNodeTexImage")
    t.image = load_image(RAIN_TEX)
    t.extension = "CLIP"
    uvn = N("ShaderNodeUVMap")
    uvn.uv_map = "UVMap"
    L(uvn.outputs[0], t.inputs[0])
    at = N("ShaderNodeAttribute")
    at.attribute_name = "Col"
    at.attribute_type = "GEOMETRY"
    sep = N("ShaderNodeSeparateColor")
    L(at.outputs["Color"], sep.inputs[0])
    # 色 = 頂点色RGB × テクスチャRGB(Effekseerは頂点色×テクスチャ)、アルファ = テクスチャα × 頂点α
    mul = N("ShaderNodeVectorMath")
    mul.operation = "MULTIPLY"
    L(at.outputs["Color"], mul.inputs[0])
    L(t.outputs["Color"], mul.inputs[1])
    a1 = N("ShaderNodeMath")
    a1.operation = "MULTIPLY"
    L(t.outputs["Alpha"], a1.inputs[0])
    L(at.outputs["Alpha"], a1.inputs[1])
    em = N("ShaderNodeEmission")
    L(mul.outputs[0], em.inputs["Color"])
    tr = N("ShaderNodeBsdfTransparent")
    mix = N("ShaderNodeMixShader")
    L(a1.outputs[0], mix.inputs["Fac"])
    L(tr.outputs[0], mix.inputs[1])
    L(em.outputs[0], mix.inputs[2])
    out = N("ShaderNodeOutputMaterial")
    L(mix.outputs[0], out.inputs["Surface"])
    try:
        mat.surface_render_method = "BLENDED"
    except Exception:
        mat.blend_method = "BLEND"
    mat.use_backface_culling = False
    me.materials.append(mat)
    return ob


def update_rain_mesh(ob, verts, rgba):
    n = len(verts) // 4
    me = ob.data
    me.clear_geometry()
    me.from_pydata(verts.tolist(), [], [(i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3) for i in range(n)])
    uv = me.uv_layers.new(name="UVMap")
    uvs = np.tile(np.array([[0, 1], [1, 1], [1, 0], [0, 0]], dtype=np.float32), (n, 1))
    uv.data.foreach_set("uv", uvs.ravel())
    ca = me.color_attributes.new("Col", "FLOAT_COLOR", "POINT")
    ca.data.foreach_set("color", rgba.astype(np.float32).ravel())
    me.update()

# 自機をFBXに書き出すスクリプト(Blender 5.x)
# build_player.py → bake_player.py の後に実行する。OUT_PATH に書き出す(既定は Tools/PlayerModel/export/Player.fbx)。
# 書き出すもの: メッシュ Player_Body と、その子の空のノード Collider7 / Collider6(DxLibではフレームになる)。
# 書き出しの設定は2面のボス(export_boss2.py)と同じ。ただし bake_space_transform=True を足す:
#   ゲームは自機を自前のシェーダーで描くとき、フレームの行列を使わずモデル全体の行列(MV1GetLocalWorldMatrix)
#   だけを頂点にかける(Actor::UpdateShaderMatrixData)。これがないと、軸の変換と単位(100倍)がメッシュの
#   フレームの行列に入り、ゲームでは機体が1/100の大きさになって見えなくなる。
#   これで軸と単位を頂点に焼き込み、メッシュのフレームの行列は単位行列になる(Collider の位置はDxLibの単位で入る)。
#   2面のボスはボーンで動かす(スキニングのシェーダー)ので、この問題は起きない。

import bpy
import os

_HERE = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else ""
OUT_PATH = globals().get("OUT_PATH", os.path.join(_HERE, "export", "Player.fbx") if _HERE else r"C:\temp\Player.fbx")

objs = [bpy.data.objects[n] for n in ("Player_Body", "Collider7", "Collider6")]
for ob in objs:
    if "." in ob.name:
        raise RuntimeError("名前に番号が付いている: " + ob.name)
for ob in bpy.context.view_layer.objects:
    ob.select_set(False)
for ob in objs:
    ob.hide_set(False)
    ob.select_set(True)
body = objs[0]
bpy.context.view_layer.objects.active = body
os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)

with bpy.context.temp_override(active_object=body, object=body,
                               selected_objects=objs, selected_editable_objects=objs):
    bpy.ops.export_scene.fbx(
        filepath=OUT_PATH,
        use_selection=True,
        object_types={"MESH", "EMPTY"},
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
        path_mode="COPY",
        embed_textures=False,
    )

for ob in objs:
    ob.select_set(False)
result = {"exported": OUT_PATH}

# 2面のボスをFBXに書き出すスクリプト(Blender 5.x)
# build_boss2.py の後に実行する。OUT_PATH に書き出す。

import bpy

OUT_PATH = globals().get("OUT_PATH", r"C:\temp\Boss2.fbx")

objs = [bpy.data.objects[n] for n in ("Boss2_Armature", "Boss2_Main", "Boss2_PodR", "Boss2_PodR_Broken")]
for ob in bpy.context.view_layer.objects:
    ob.select_set(False)
hidden = []
for ob in objs:
    if ob.hide_get():
        hidden.append(ob)
        ob.hide_set(False)
    ob.select_set(True)
arm = objs[0]
bpy.context.view_layer.objects.active = arm
# 書き出すのは待機ポーズ(0フレーム)
bpy.context.scene.frame_set(0)

with bpy.context.temp_override(active_object=arm, object=arm,
                               selected_objects=objs, selected_editable_objects=objs):
    bpy.ops.export_scene.fbx(
        filepath=OUT_PATH,
        use_selection=True,
        object_types={"ARMATURE", "MESH"},
        apply_unit_scale=True,
        global_scale=1.0,
        apply_scale_options="FBX_SCALE_NONE",
        axis_forward="-Z",
        axis_up="Y",
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_tspace=True,
        add_leaf_bones=False,
        primary_bone_axis="Y",
        secondary_bone_axis="X",
        use_armature_deform_only=False,
        armature_nodetype="NULL",
        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=True,
        bake_anim_force_startend_keying=True,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
        path_mode="COPY",
        embed_textures=False,
    )

for ob in hidden:
    ob.hide_set(True)
for ob in objs:
    ob.select_set(False)
result = {"exported": OUT_PATH}

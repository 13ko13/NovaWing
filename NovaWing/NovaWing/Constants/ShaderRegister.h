#pragma once

//シェーダーの定数バッファ・テクスチャスロットのレジスタ番号を一元管理する
namespace ShaderRegister
{
	//---------- Actor共通(Player/WormEnemy/FloatingEnemy/Rock) ----------
	//行列情報(Matrix)の定数バッファ(頂点シェーダ)
	constexpr int cbuffer_matrix = 5;
	//カメラ情報(Camera)の定数バッファ(頂点・ピクセル共通)
	constexpr int cbuffer_camera = 6;
	//ライト情報(Light)の定数バッファ(ピクセルシェーダ)
	constexpr int cbuffer_light = 4;

	//ディフューズマップ(テクスチャ)
	constexpr int tex_diffuse = 0;
	//法線マップ
	constexpr int tex_normal = 1;
	//メタリックマップ
	constexpr int tex_metalic = 2;
	//エミッションマップ
	constexpr int tex_emission = 3;
	//ディゾルブ用のノイズテクスチャ
	constexpr int tex_noise = 4;

	//---------- WaterManager専用 ----------
	//行列情報(Matrix)の定数バッファ(頂点シェーダ)
	constexpr int water_cbuffer_matrix = 2;
	//ライト情報(Light)の定数バッファ(ピクセルシェーダ)
	constexpr int water_cbuffer_light = 3;
	//波情報(Water)の定数バッファ(頂点シェーダ)
	constexpr int water_cbuffer_water_vs = 4;
	//カメラ情報(Camera)の定数バッファ(ピクセルシェーダ)
	constexpr int water_cbuffer_camera = 5;
	//波情報(Water)の定数バッファ(ピクセルシェーダ)
	constexpr int water_cbuffer_water_ps = 6;

	//スカイボックス(前・後・右・左・上・下)
	constexpr int water_tex_sky_front = 0;
	constexpr int water_tex_sky_back = 1;
	constexpr int water_tex_sky_right = 2;
	constexpr int water_tex_sky_left = 3;
	constexpr int water_tex_sky_up = 4;
	constexpr int water_tex_sky_bottom = 5;
	//透過水キャプチャ
	constexpr int water_tex_scene_capture = 6;
	//コースティクス
	constexpr int water_tex_caustics = 7;
	//テクスチャ解除ループの範囲(0～これ未満)
	constexpr int water_tex_slot_count = 8;

	//---------- Stage専用 ----------
	//ライト情報(Light)の定数バッファ(ピクセルシェーダ)
	constexpr int stage_cbuffer_light = 4;

	//----------UI(GlitchPS)専用-----
	//ディフューズマップ(テクスチャ)
	constexpr int ui_tex_diffuse = 0;
	//グリッチシェーダに渡す定数バッファ(ピクセルシェーダ)
	constexpr int glitch_buffer = 0;

	//----------プレイヤーのダメージ専用------
	constexpr int cbuffer_damage = 7;
}
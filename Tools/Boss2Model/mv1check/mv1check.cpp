// モデルをDxLibで読んで、フレーム・大きさ・アニメーションを書き出す確認用プログラム
// 使い方: mv1check.exe <モデル> <出力テキスト> [保存するmv1]
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc < 3) return 2;

	ChangeWindowMode(TRUE);
	SetGraphMode(320, 240, 32);
	SetOutApplicationLogValidFlag(FALSE);
	SetAlwaysRunFlag(TRUE);
	SetWindowVisibleFlag(FALSE);
	if (DxLib_Init() == -1) return 1;

	FILE* fp = nullptr;
	_wfopen_s(&fp, argv[2], L"w, ccs=UTF-8");
	if (!fp) { DxLib_End(); return 3; }

	int h = MV1LoadModel(argv[1]);
	fwprintf(fp, L"handle=%d\n", h);
	if (h >= 0)
	{
		int frameNum = MV1GetFrameNum(h);
		fwprintf(fp, L"frames=%d\n", frameNum);
		for (int i = 0; i < frameNum; i++)
		{
			VECTOR p = MV1GetFramePosition(h, i);
			int parent = MV1GetFrameParent(h, i);
			fwprintf(fp, L"  [%d] %s parent=%d meshes=%d pos=(%.1f, %.1f, %.1f)\n", i,
				MV1GetFrameName(h, i), parent, MV1GetFrameMeshNum(h, i), p.x, p.y, p.z);
		}
		MV1SetupReferenceMesh(h, -1, TRUE);
		MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(h, -1, TRUE);
		fwprintf(fp, L"polygons=%d vertices=%d\n", ref.PolygonNum, ref.VertexNum);
		fwprintf(fp, L"min=(%.1f, %.1f, %.1f) max=(%.1f, %.1f, %.1f)\n",
			ref.MinPosition.x, ref.MinPosition.y, ref.MinPosition.z,
			ref.MaxPosition.x, ref.MaxPosition.y, ref.MaxPosition.z);
		int matNum = MV1GetMaterialNum(h);
		fwprintf(fp, L"materials=%d\n", matNum);
		for (int i = 0; i < matNum; i++)
		{
			COLOR_F d = MV1GetMaterialDifColor(h, i), a = MV1GetMaterialAmbColor(h, i);
			COLOR_F s = MV1GetMaterialSpcColor(h, i), e = MV1GetMaterialEmiColor(h, i);
			fwprintf(fp, L"  mat[%d] %s tex=%d dif=(%.2f,%.2f,%.2f,%.2f) amb=(%.2f,%.2f,%.2f) spc=(%.2f,%.2f,%.2f) pow=%.1f emi=(%.2f,%.2f,%.2f)\n",
				i, MV1GetMaterialName(h, i), MV1GetMaterialDifMapTexture(h, i),
				d.r, d.g, d.b, d.a, a.r, a.g, a.b, s.r, s.g, s.b, MV1GetMaterialSpcPower(h, i), e.r, e.g, e.b);
		}
		int texNum = MV1GetTextureNum(h);
		fwprintf(fp, L"textures=%d\n", texNum);
		for (int i = 0; i < texNum; i++)
		{
			fwprintf(fp, L"  tex[%d] %s\n", i, MV1GetTextureName(h, i));
		}
		int animNum = MV1GetAnimNum(h);
		fwprintf(fp, L"anims=%d\n", animNum);
		for (int i = 0; i < animNum; i++)
		{
			fwprintf(fp, L"  anim[%d] %s total=%.2f\n", i, MV1GetAnimName(h, i), MV1GetAnimTotalTime(h, i));
		}
		// 待機アニメーションの途中で、Bodyの位置が上下するか
		int body = MV1SearchFrame(h, L"Body");
		if (animNum > 0 && body >= 0)
		{
			int att = MV1AttachAnim(h, 0, -1, FALSE);
			float total = MV1GetAttachAnimTotalTime(h, att);
			for (int k = 0; k <= 4; k++)
			{
				float t = total * k / 4.0f;
				MV1SetAttachAnimTime(h, att, t);
				VECTOR p = MV1GetFramePosition(h, body);
				fwprintf(fp, L"  anim t=%.2f Body=(%.2f, %.2f, %.2f)\n", t, p.x, p.y, p.z);
			}
			MV1DetachAnim(h, att);
		}
		if (argc >= 4 && wcslen(argv[3]) > 0 && wcscmp(argv[3], L"-") != 0)
		{
			int r = MV1SaveModelToMV1File(h, argv[3]);
			fwprintf(fp, L"save_mv1=%d\n", r);
		}
		// 画像に描く: argv[4]=出力PNG, argv[5]=カメラ(x,y,z), argv[6]=注視点(x,y,z), argv[7]=アニメ時間
		if (argc >= 5)
		{
			int scr = MakeScreen(1280, 720, FALSE);
			SetDrawScreen(scr);
			SetBackgroundColor(168, 174, 183);
			ClearDrawScreen();
			DrawBox(0, 0, 1280, 720, GetColor(168, 174, 183), TRUE);
			VECTOR camPos = VGet(0.0f, -150.0f, -3000.0f);
			VECTOR camTgt = VGet(0.0f, 0.0f, 0.0f);
			if (argc >= 6) swscanf_s(argv[5], L"%f,%f,%f", &camPos.x, &camPos.y, &camPos.z);
			if (argc >= 7) swscanf_s(argv[6], L"%f,%f,%f", &camTgt.x, &camTgt.y, &camTgt.z);
			SetCameraNearFar(10.0f, 20000.0f);
			SetupCamera_Perspective(DX_PI_F / 2.0f);
			SetCameraPositionAndTarget_UpVecY(camPos, camTgt);
			SetUseZBuffer3D(TRUE);
			SetWriteZBuffer3D(TRUE);
			SetLightDirection(VGet(0.3f, -0.8f, 0.5f));
			if (argc >= 8 && animNum > 0)
			{
				int animIdx = 0;
				if (argc >= 9) animIdx = MV1GetAnimIndex(h, argv[8]);
				fwprintf(fp, L"draw anim=%d\n", animIdx);
				int att = MV1AttachAnim(h, animIdx, -1, FALSE);
				MV1SetAttachAnimTime(h, att, (float)_wtof(argv[7]));
			}
			int broken = MV1SearchFrame(h, L"Boss2_PodR_Broken");
			if (broken >= 0) MV1SetFrameVisible(h, broken, FALSE);
			MV1DrawModel(h);
			SaveDrawScreenToPNG(0, 0, 1280, 720, argv[4]);
			fwprintf(fp, L"png=%s\n", argv[4]);
		}
	}
	fclose(fp);
	DxLib_End();
	return 0;
}

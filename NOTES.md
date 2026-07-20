# 開発メモ（Claudeとの会話ログ）

学校と家でClaude Codeの会話履歴が自動でつながらないため、このファイルに毎回の話した内容を追記しています。
ローカルの会話履歴が切れたときは、このファイルを読み込むことでこれまでの経緯を把握できます。

---

## 2026-07-09
- 学校と家でClaude Codeの会話履歴を繋げたいという相談があった。
- 結論: Claude Codeは会話履歴をPCごとにローカル保存しており、自動で他PCと同期する機能はない。
- claude.ai（Webアプリ）はアカウント単位で同期されるが、プロジェクト全体のファイルを読み込むことができないため、コード作業にはClaude Codeが必要という結論に。
- 代替策として、リポジトリ直下（.gitignoreと同じ階層）に `NOTES.md` を作成し、毎回の会話内容をここに追記していく運用を開始。Gitでpush/pullすれば学校・家のどちらからでも経緯を参照できる。

### 今後のタスク: 海と岩・陸地の境界を透明にする表現（未着手・待機中）
- やりたいこと: 海に岩柱や陸地を設置し、隣接させたとき、物体の周囲が透明になって海が透けて見え、物体がうっすら見えるような表現（境界部分のトランジション/フェード的な演出）を実装したい。
- 実装の前提: このシェーダーを作るには、先に岩のモデルが必要。現在は岩モデルをfbxでエクスポートする作業待ち。
- **指示: 岩のfbxエクスポートが完了するまで、この件の実装（シェーダー作成など）には着手せず待機すること。** まだエクスポートは完了していない（2026-07-09時点）。
- 次回、fbxエクスポートが完了したと連絡があってから着手する。

### 進捗（2026-07-09 続き・自宅にて）
- 岩モデル(Rock1.mv1)のテクスチャは、diffuseとspecularを1枚のRGBA画像にパッキングしたもので、アルファ値がスペキュラ強度マップだと判明（claude.aiで先に調査済み）。
- `LightingPS.hlsl`のスペキュラ計算に`specular *= texColor.a;`を追加して対応。プレイヤー・敵モデルのテクスチャはアルファ無しか完全不透明(255)のみなので、既存モデルへの影響がないことを確認済み。
- ビルド時にFxCompileでhlsl→pso/vsoが自動コンパイルされるので、Visual Studioでビルドすれば反映される。
- 岩のモデル・テクスチャをフォルダに追加完了。次は海に岩・陸地を設置する作業を開始する予定（境界の透明化表現はまだ未着手）。

### 重要ルール: 勉強目的のため .h/.cpp/.hlsl は直接編集しない（2026-07-09〜）
- ユーザーは勉強目的でAIを使用しているため、**.h / .cpp / .hlsl ファイルはClaudeが直接Edit/Writeしない**。変更内容を説明し、ユーザー自身がコードを書く。
- それ以外のファイル（CSV、.mdなど）は直接編集してよい。
- 岩配置の実装方針（調査済み）:
  - 静的オブジェクトは`GameObjectManager`に登録する仕組み。`GameObject`を直接継承すればActor/敵の重い仕組みを経由しなくてよい。
  - MV1モデル＋LightingManagerシェーダーの描画パターンは`FloatingEnemy::DrawEnemy()`が参考になる。
  - `Data/Model`に`Rock1.mv1`, `Rock2.mv1`, `rock3.mv1`は既にあるが、`ResourceLoader`にはまだ未登録。
  - 配置は現状`GameScene::Init()`にハードコードする形（データファイルや専用マネージャーはまだ無い）。
  - 実装ステップ案: ①ResourceLoaderにRock用モデルIDと`MV1LoadModel`呼び出しを追加 ②`Game/GameObjects/Rock.h/.cpp`（GameObject継承）を新規作成 ③`GameScene::Init()`で岩の座標を指定して`GameObjectManager`に登録。
- **今後の要望**: 岩の数が増えると位置変更が大変なので、将来的にCSVファイルから岩の位置を読み込んで、CSVの値を変えるだけで配置を変更できる仕組みにしたい（未実装）。

### 進捗（2026-07-10）
- `Rock`クラスの設計方針を決定: `Charactor`ではなく`Actor`を継承（HP等の余計な機能を持たせないため）。ただしシェーダー用定数バッファ処理（`CreateShaderBuffers`等4関数）は元々`Charactor`側にあるため、`Actor`に引き上げる設計変更を実施予定（ユーザー自身が`Actor.h/.cpp`・`Charactor.h/.cpp`を編集）。
- `Rock::Draw()`にはカメラ位置を引数で渡す形にする（`SkyBox::Draw(cameraPos)`と同じパターン）。
- VS CodeとVisual Studioを併用中、同じファイルを両方で開いていると片方が編集を検知せず古い内容のまま表示されることがある → 同時に開いて編集しないよう注意。
- VS CodeのIntelliSenseとVisual Studioでエラー表示が食い違う問題が発覚。原因は`.vscode/c_cpp_properties.json`が存在せず、`.vcxproj`のインクルードパス・プリプロセッサ定義をVS Code側が認識できていなかったため。`.vcxproj`のDebug|x64設定（インクルードパス3つ、`WIN32;_DEBUG;_WINDOWS`、Unicode、C++20）を反映した`c_cpp_properties.json`を新規作成して対応。
  - **注意**: 今後`.vcxproj`側でインクルードパスやプリプロセッサ定義を変更した場合、`.vscode/c_cpp_properties.json`にも手動で同じ変更を反映しないと、またVS CodeとVisual Studioでズレが生じる。
- ファイルの新規作成（`.h`/`.cpp`/`.hlsl`）は`.vcxproj`への自動登録のためVisual Studioの「新しい項目の追加」を使い、中身の編集はVS Code、という使い分けで運用することにした。


### 進捗（2026-07-10 続き）
- `Actor`へのシェーダー定数バッファ処理（`CreateShaderBuffers`/`UpdateShaderMatrixData`/`BindShaderBuffers`/`ReleaseShaderBuffers`/`GetCameraPos`）の引き上げ完了。`Charactor`は重複がなくなり`Actor`をそのまま継承する形になった。
- `ResourceLoader.h`に岩関連のIDを登録済み：`ModelID::Rock1`/`Rock2`/`Rock3`、`GraphicID::RockDiffAndSpc`/`RockNorm`。
- `Game/GameObjects/Actors/Rock/Rock.h`・`Rock.cpp`を新規作成済みだが、中身は空のスタブ（`class Rock : public Actor {};`のみ）。コンストラクタ・`Update()`・`Draw()`はまだ未実装。
- `GameScene.cpp`には岩関連のコードはまだ一切なし（配置は未着手）。

#### 次回やること
1. `Rock`クラスの中身を実装する：コンストラクタで`ResourceLoader::ModelID::Rock1`等からモデルをロード、`Draw()`は`FloatingEnemy::DrawEnemy()`を参考にしつつ`SkyBox::Draw(cameraPos)`と同じくカメラ位置を引数で受け取る形にする。
2. `GameScene::Init()`で岩の座標を指定して`GameObjectManager`に登録する。
3. （その後）海と岩・陸地の境界を透明にするシェーダー表現に着手（岩のfbxエクスポート自体は完了済みなので着手可能）。
4. さらにその後の要望：岩の配置をCSVから読み込めるようにしたい（未実装）。


### 進捗（2026-07-10 続き2・岩の配置完了）
- `Rock`クラスのバグ修正完了：`OnInit()`内で誤って`GameObject::Init()`を呼んでいた無限再帰を削除、`CreateShaderBuffers()`の呼び出し漏れを追加。
- `GameScene.cpp`に`Rock`の生成・配置コードを追加。ここで2つハマった点：
  - `std::shared_ptr<Rock>(Rock1, m_pCamera)`と書いてしまい、`shared_ptr`の「生ポインタ+デリータ」コンストラクタと誤マッチしてstd::内部でエラーが出た → `std::make_shared<Rock>(...)`に修正。
  - `using enum ResourceLoader::ModelID;`を`GameScene::Init()`に書いたところ、`ModelID::Player`/`ModelID::FloatingEnemy`という列挙値名が、クラス名`Player`/`FloatingEnemy`と衝突し、`std::make_shared<Player>`等が型解決できずビルドエラーになった → `using enum`をやめて`ResourceLoader::ModelID::Player`のように完全修飾に戻して解決。**教訓: `using enum`は、同名のクラス・型が同じスコープに存在する場所では使わない方が安全。**
- ビルド・実行して、岩(仮位置0,0,0)がテクスチャ・法線マップ込みで正しく表示されることを確認済み（スクリーンショットで動作確認）。
- これでNOTES.mdの「次回やること」1〜2番（Rock実装・GameScene配置）が完了。

#### 次回やること（更新）
1. 岩の座標を仮の(0,0,0)から実際に配置したい位置に変更する（複数個配置する場合は座標をどう管理するか含めて検討）。
2. ~~海と岩・陸地の境界を透明にするシェーダー表現に着手する。~~ → 着手済み、実装完了。現在デバッグ中（詳細は下記参照）。
3. その後の要望：岩の配置をCSVから読み込めるようにしたい（未実装）。

### 進捗（2026-07-10 続き3・透過水A案の実装完了、デバッグ中）

**実装したもの（A案＝深度キャプチャ方式、設計は本ファイル上部で決定済み）:**
- `Manager/WaterRevealManager.h/.cpp`（新規）: シングルトン。`MakeScreen(1280,720,true)`でオフスクリーン画面`m_captureH`を確保。`BeginCapture()`/`EndCapture()`で`SetDrawScreen`を切り替え。`CapturePS.pso`のハンドルと現在キャプチャ中かのフラグを保持。
- `CapturePS.hlsl`（新規）: `LightingVS.hlsl`と共通の`PS_INPUT`を受け取り、RGB=物体の色、A=カメラからの正規化距離(near_clip=200,far_clip=5500で正規化)を出力。頂点シェーダーは`LightingVS.hlsl`をそのまま流用。
- `LightingManager::ApplyShader()`: `WaterRevealManager::IsCaptureMode()`で分岐し、キャプチャ中は`CapturePS`、通常時は`LightingPS`を使う。これでPlayer/FloatingEnemy/WormEnemy/Rock全部が自動的にキャプチャパスに対応。
- `GameScene::Draw()`: フレーム冒頭で`BeginCapture()`→`GameObjectManager::DrawAll()`(1回目、キャプチャ用)→`EndCapture()`→`m_pCamera->SetUpCamera()`(カメラの投影/位置再設定、後述の理由で必要)、その後は通常通りSkyBox→DrawAll(2回目)→WaterManager::Draw()→UI。
- `WaterManager`: `CameraBuffer`に`screenWidth`/`screenHeight`を追加(32バイト、16バイトアライメント)。`Draw()`でキャプチャテクスチャをt6にバインド、解除ループを7スロットに拡張。
- `WaterPS.hlsl`: `sceneCapture`テクスチャ(t6)を追加。スクリーンUV(`input.pos.xy / screenSize`)でサンプリングし、水面自体の正規化距離との差(`delta`)が小さいほど`reveal`を上げて`captureCol.rgb`とブレンドする処理を追加。

**解決済みの大きなバグ（今後の参考のため記録）:**
1. `SetDrawScreen`はDxLibの仕様でカメラの投影設定(`SetupCamera_Perspective`/`SetCameraNearFar`)と位置設定(`SetCameraPositionAndTarget_UpVecY`)を両方リセットする。→ `CameraBase`に`SetUpCamera()`(旧`SetUpProjection()`、両方まとめて再設定する関数)を追加し、`EndCapture()`の後に呼ぶことで解決。
2. **重大: `Actor::BindShaderBuffers()`が使っていた定数バッファスロット(頂点b2, 頂点/ピクセルb3)がDxLibの予約スロットだった。** キャプチャパス(オフスクリーン`SetDrawScreen`中)でこれらのスロットに`SetShaderConstantBuffer`を呼ぶと、後続の固定機能`DrawPolygon3D`(SkyBoxの描画)が単色崩壊する謎バグが発生。何時間もかけてMV1DrawModel単体→BindShaderBuffers単体→スロット単位で二分探索し、スロット2/3が原因と特定。**対処: スロットをb2→b5, b3→b6にずらして解決**（`Actor.cpp`, `LightingVS.hlsl`, `LightingPS.hlsl`, `CapturePS.hlsl`の4ファイルを修正）。同様の予約スロット問題は`WaterManager`(頂点b2, ピクセルb5等)にもまだ残っている可能性があるので、今後何か描画がおかしくなったらまずスロット番号を疑うこと。
3. Player.cppに実験用のコメントアウト(`LightingManager::ApplyShader/ResetShader`を無効化していた)が残っていて、シェーダーが当たらなくなるバグが発生→解消済み。

**現在デバッグ中の問題:**
海と岩の境界の透過表現自体が全く発動しない（`reveal`が常に0）。原因調査で判明したこと:
- `captureCol.a`(岩のキャプチャ深度)は正しく取れている: デバッグ表示(`return float4(captureCol.a,captureCol.a,captureCol.a,1)`)で岩の下半分だけ白くなり、岩の形と一致した。
- `normDistNtoF`(水面自体の深度)も個別に見ると正常なグラデーション: 画面奥(水平線)が白、手前(カメラに近い)が黒という自然な分布。ただし**岩がある位置の水面はかなり黒い(=カメラにかなり近いという判定)**。
- **矛盾点**: 同じ画面位置で比較すると、`captureCol.a`(岩)はほぼ1(遠い)なのに`normDistNtoF`(水面)はほぼ0(近い)という、物理的にありえない逆の値になっている。この2つの差(`delta`)が常に1近くになるため、`smoothstep`の閾値を0.05→0.5に緩めても全く反応しない。
- `near_clip`/`far_clip`は`CapturePS.hlsl`と`WaterPS.hlsl`両方とも200.0f/5500.0fで完全一致していることは確認済み。
- ブレンドモード(`SetDrawBlendMode`)はUI/フェード演出でしか使われておらず、キャプチャパス関連には無関係と確認済み。
- **次にやるべきこと**: `CapturePS.hlsl`の`return`を一時的に`return float4(depthNearToFar, depthNearToFar, depthNearToFar, depthNearToFar);`に変更し(RGBにも同じ値を入れる)、`WaterPS.hlsl`側で`captureCol.rgb`をそのまま可視化して、キャプチャ側の`dist`計算結果を直接確認する。cameraPosの値がCapturePS.hlsl(b6)とWaterPS.hlsl(b5)で本当に同じフレームの同じ値を参照しているか、Rockのモデルスケール(3.0,5.0,3.0)がworldPos計算に悪影響していないか、あたりを疑って調査を続ける。
- デバッグ用に書き換えた`WaterPS.hlsl`/`CapturePS.hlsl`の`return`文は実験用なので、次回作業再開時は最新の状態を確認してから続きを進めること（正式なreveal計算のコードに戻っているかもしれないし、デバッグ版のままかもしれない）。

### 進捗（2026-07-11・透過水の完成！！）

**海と岩の境界透過（reveal）が完成した。** 岩の水中部分が境界からふわっと透けて見え、深くなるほど水の色に戻る狙い通りの表現になった。

**真の原因（2つの独立した問題が重なっていた）:**
1. **`.vcxproj`のFxCompile設定不足**: `CapturePS.hlsl`/`LightingPS.hlsl`/`WaterPS.hlsl`/`WaterVS.hlsl`はDebug|x64にしか`ObjectFileOutput`（`$(ProjectDir)`への.pso出力）が設定されておらず、Releaseビルドでは古い.psoが読み込まれ続けていた。デバッグ中の観測結果が実験ごとに食い違って見えた元凶の一つ。→ **Release|x64にもShaderModel 5.0とObjectFileOutputを設定して解決**（LightingVSだけは元々両方設定済みだった）。
2. **DxLibの乗算済みアルファ（premultiplied alpha）問題（本命）**: アルファ付きオフスクリーン（`MakeScreen(w,h,true)`）へのMV1描画で、マテリアルのブレンド設定が全体の`SetDrawBlendMode(NOBLEND)`を上書きし、**保存される値が「RGB=色×α、A=α×α」に化けていた**。さらに岩テクスチャのα（スペキュラマップ）のせいでモデルが半透明メッシュ判定され、Z書き込みなしで裏面ポリゴンまで重ね描き→αが蓄積して1近くまで膨らんでいた。「captureCol.aがほぼ1」の正体はこれ。

**修正内容:**
- `Rock.cpp`コンストラクタに追加（この2つで解決）:
  - `MV1GetTextureNum`でループし`MV1SetTextureGraphHandle(handle, i, 同じgrHandle, FALSE)`で全テクスチャを「半透明要素なし」に再登録（半透明メッシュ判定を解除）
  - `MV1GetMaterialNum`でループし`MV1SetMaterialDrawBlendMode(handle, i, DX_BLENDMODE_NOBLEND)`で全マテリアルをブレンド無しに（**これが決定打**。グローバルな`SetDrawBlendMode`はMV1描画では効かない）
- `WaterRevealManager::BeginCapture()`/`EndCapture()`に`SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255)`も追加済み（保険。害はない）
- `WaterPS.hlsl`のreveal式: `reveal = (1.0f - smoothstep(0.0f, 0.15f, delta)) * 0.6f;`
  - `0.15f` = 透ける範囲の広さ（深度差の閾値）、`0.6f` = 透け具合の上限（水の色を4割残す）。見た目の好みはこの2つで調整する。

**デバッグ手法のメモ（今後同種の問題が出たとき用）:**
- シェーダー内の値は「段階別カラー判定」（if文で値の範囲ごとに純色を返す）で可視化するのが確実。グラデーション表示は増幅率で振り切れて情報が消えることがある。
- 「テクスチャに書いた値」と「読んだ値」が食い違うときは、①psoの鮮度（ビルド設定）②ブレンドモード（特に乗算済みα）③半透明メッシュ判定、の順に疑う。
- 検証時はαを定数（例:0.1）にして書き込み→読み出し経路だけを単独テストすると切り分けが速い。

**残タスク:**
1. ~~岩の座標を実際の配置位置に調整（今は仮位置）。複数配置するなら岩の生成をループ化。~~ → CSV化により解決（下記参照）。
2. ~~岩の配置をCSVから読み込む仕組み（以前からの要望・未実装）。~~ → 完成（下記参照）。
3. 敵など他のモデルも水に潜る演出をするなら、Rockと同じテクスチャ/マテリアル設定が必要になる点に注意（今回の修正はRockのみに適用）。将来的には`Actor`側に共通化してもよいかも。

### 進捗（2026-07-11・続き2・岩の配置をCSV化）

**岩の配置をCSVから読み込む仕組みが完成した。** `Data/CSV/RockData.csv`に`modelID,x,y,z`の行を追加するだけで、岩の数・種類・位置を変更できるようになった。

**設計方針（学習目的のため、まず一緒に方針決めから議論した）:**
- 「文字列→数値変換」のような汎用処理と、「CSVの列が何を意味するか」という固有知識は分離する方針にした。
  - `Vector3::FromWString(x,y,z)`（`Utility/Vector3.h/.cpp`に追加）: wstring3つをfloatに変換してVector3を作る汎用部品。CSVの知識は持たない。
  - `RockDataSetter`（`Game/GameObjects/Actors/Rock/RockDataSetter.h/.cpp`、新規）: 「岩の配置データを扱う」責務のみ持つクラス。CSVを読み、各行をModelID/Vector3に変換し、Rockを生成してvectorで返す`static`関数`CreateRock(pCamera)`を持つ。
- クラス名は最初`RockSpawner`案が出たが、「このゲームでは全て最初から決め打ちで配置されるものであり、ランダムに湧く(spawn)わけではない」という理由で却下し、`RockDataSetter`に決定。将来的に座標だけでなく他のパラメータも渡す拡張を見込んだ名前。
- `GameScene`が直接CSVを読む設計は「敵・プレイヤーなど他もCSV化すると、GameSceneが同じパターンのコードで膨れ上がる」という理由で避け、岩専用の`RockDataSetter`に読み込みロジックを閉じ込めた。将来敵をCSV化する際は同様に`EnemyDataSetter`的なものを増やせば良い（今回は「まず岩専用で素朴に作る」方針で、共通化・継承などはまだしていない）。

**Rockクラスの変更:**
- コンストラクタに`const Vector3& pos`引数を追加し、コンストラクタ内で直接`SetPos(pos)`するように変更（`OnInit()`ではなくコンストラクタで良いと判断した理由: `OnInit()`に置く基準は「コンストラクタで行うと問題が起きる処理」であり、`SetPos`は単なるメンバ変数への代入なので該当しない。`CreateShaderBuffers()`が`OnInit()`にあるのとは別の話）。

**発覚した既存バグ（`CSVDataLoader.cpp`）:**
- `csv_path`定数が`L"Data/CSV"`（末尾に`/`なし）になっており、`path`と単純結合すると`"Data/CSVRockData.csv"`のような誤ったパスになって読み込みに失敗していた。`L"Data/CSV/"`に修正して解決。

**開発環境の教訓（再発）:**
- `RockDataSetter.h/.cpp`をVSCode上でフォルダ移動したところ、Visual Studioの`.vcxproj`に反映されずビルドエラー（ファイルが見つからない）になった。理由: `.vcxproj`はファイルシステムを都度スキャンするのではなく、各ファイルのパスをXML内に静的にリストで保持しているため、VSCode上での移動・リネームは追従されない。VSCodeのIntelliSenseは`c_cpp_properties.json`の設定でファイルシステムを直接見るため、この種のズレに気づきにくい。
  - **ルール拡張**: ファイルの新規作成だけでなく、**移動・リネームも必ずVisual Studioのソリューションエクスプローラー上で行う**こと。VSCode上でのフォルダ構造の変更は禁止。

**残タスク:**
1. ~~敵など他のモデルも水に潜る演出をするなら、Rockと同じテクスチャ/マテリアル設定が必要になる点に注意。~~ → `Actor`のコンストラクタに`MV1SetMaterialDrawBlendMode`のループを引き上げて共通化完了。全Actor派生クラス(敵・プレイヤー含む)で自動的に適用されるようになった。関数として切り出すと名前が付けにくいため、コンストラクタに直接書いたままにする判断をした。
2. 敵・プレイヤーのパラメータもCSV化していきたい（今回の`RockDataSetter`と同じパターンで、種類ごとに専用のDataSetterを作る方針）。
3. ~~CSVに複数行追加して、複数の岩が正しく配置されるかまだ未検証。~~ → 確認済み。複数行のCSVで複数の岩が正しく出現することを確認できた。CSV化の仕組みは実用上も完成。

### 進捗（2026-07-11・続き3・ステージ(仮モデル)の導入）

**Unity/ProBuilderで箱を並べてステージの地形モデルを作成し、FBXエクスポート→NovaWingに取り込んで表示するところまで完了した。** あくまでプロトタイプ用の仮モデルで、後日ちゃんとしたアセットに差し替える予定。

**やったこと:**
- `ResourceLoader`に`ModelID::Stage`を追加し、`Data/Model/Stage.mv1`を読み込むようにした。
- `Game/GameObjects/Actors/Stage/Stage.h/.cpp`を新規作成（`Actor`継承）。`GameScene::Init()`で1体だけ生成（岩と違い複数配置しないので`RockDataSetter`のようなCSV読み込みは無し、座標は`OnInit()`で固定）。

**詰まったポイントと解決:**
1. **スケールが合わずモデルが巨大に表示**: ProBuilder側の1ユニットとNovaWing側のスケール感が違い、最初等倍(1.0)で表示したらモデルの内部にカメラがめり込んだ状態になり「何も見えない」ように見えた。`model_scale`を`0.01`まで下げて解決。今後同じ手順でモデルを追加する際はスケール差を疑うこと。
2. **既存のLightingPS.hlslが使えなかった**: `LightingPS.hlsl`は法線マップ・メタリックマップ・エミッションマップの3枚のテクスチャが必ずある前提で書かれており、テクスチャ無しのStageでこれを使うと未バインドテクスチャのサンプリング結果(不定値)で法線計算が破綻し、面ごとに真っ白/真っ黒が入り乱れる異常な見た目になった。
3. **DxLib標準ライティング(`SetUseLighting`)も試したが解決せず**: 光の向き(`ChangeLightTypeDir`)を反転させても真っ黒のまま変化がなく、原因の特定に見合わないと判断して深追いを断念。
4. **最終的にStage専用の最小限ピクセルシェーダー(`StagePS.hlsl`)を新規作成して解決**。頂点シェーダーは`LightingVS.hlsl`をそのまま流用(出力構造体がテクスチャ非依存のため使い回せた)。中身は「法線とライトの内積+アンビエントで明るさを出し、白固定色に掛けるだけ」のテクスチャ不要な最小構成。
5. **定数バッファ(`LightingBuffer`)がヌルポインタでクラッシュ**: `Vector3 lightVec;`だけの12バイト構造体だったのが原因。DxLibの定数バッファは16バイトアライメントが必要(`CameraBuffer`のときと同じ教訓)。`float padding;`を足して解決。
6. **デバッグ手法**: `StagePS.hlsl`のreturnを一時的に`normVec`や`normLightVec`をそのまま色として出す形に変え、法線とライトの向きが面ごとに正しく異なる値を持っているか可視化して原因を切り分けた(透過水デバッグで確立した手法をユーザー自身が今回も活用できた)。最終的な原因は`dot()`の呼び出しをカンマ演算子で書いてしまっていたことと、`light`の計算にベクトルを直接使ってしまっていた単純な書き間違いだった。

**現状の制約:**
- Stageは水に沈めても透過表現(reveal)が効かない。理由: `Stage::Draw()`は独自の`StagePS.hlsl`を使っており、`LightingManager::ApplyShader()`(キャプチャモード判定を持つ)を経由していないため、深度キャプチャパスでも通常パスと同じ`StagePS`が使われてしまい、正しい深度が`WaterRevealManager`のキャプチャに書き込まれない。仮モデルなので現時点では未対応、必要になったら`LightingManager`のキャプチャ分岐の仕組みをStageにも适用する必要がある。

**残タスク:**
1. 敵・プレイヤーのパラメータもCSV化していきたい（前回からの継続）。
2. Stageは仮モデル。ちゃんとしたアセットに差し替え予定。差し替え時はスケール調整とシェーダー(差し替え後のモデルにテクスチャがあるならLightingPS、無いままならStagePS)を再検討すること。
3. Stageを水に沈める演出をしたい場合、上記の「現状の制約」に対応が必要。

### 進捗（タイトルシーンのワイプ演出完成）

**タイトル画面の選択肢に、カーソルを合わせたときの左からワイプで画像が切り替わる演出を実装した。** `Scene/TitleScene.h/.cpp`のみの変更。

**実装内容:**
- `TitleScene.h`: `std::array<float, SelectMax> m_wipeProgress = {}`(選択肢ごとのワイプ進行度、0〜1)、`m_prevSelectIdx`(前フレームの選択状態、切り替わり検出用)を追加。
- `Update()`: 現在選ばれている選択肢の`m_wipeProgress`を`1.0f / wipe_max`ずつ増やし、選ばれていないものは減らす。`std::clamp`で0〜1に収める(`clamp`は新しい値を戻り値で返すだけで引数を書き換えないので、`m_wipeProgress[i] = std::clamp(...)`と代入し直す必要がある点でハマった)。
- `Draw()`: `DrawRectRotaGraph`でカーソルオン画像を「左端から`進行度 × 画像幅`の範囲」だけ切り取って重ね描き。`DrawRectRotaGraph`は切り取った矩形の**中心**を指定座標に置く仕様なので、左端固定で右に伸ばすには`leftX + (切り取り幅 × 拡大率) / 2`という座標計算が必要だった(単純に`leftX + 切り取り幅`ではない)。

**ハマった点・試して却下した案:**
- 当初「通常画像を先に`DrawRotaGraph`で描き、その上にカーソルオン画像の左側だけ`DrawRectRotaGraph`で重ねる」方式にしたところ、両方の画像のグロー・文字縁の半透明部分が重なって色がにじむ(緑背景の画像同士で、黒文字のふちがうっすら緑がかって見える)問題が発生。
- 正攻法の解決策(常に2枚の画像を`DrawRectRotaGraph`で左右に分割して重ねずに描く)も検討したが、実装がやや複雑になるため見送り、**`wipe_max`を60→10フレームに短縮して、重なりが目立つ時間自体を短くすることで実用上気にならないレベルにする**という判断で妥協・完了とした。今後にじみが気になったら、上記の「重ねずに左右分割描画」方式への変更を検討すること。

### 進捗（プレイヤー弾のEffekseerエフェクト導入・作業中）

**Effekseerで自作したエフェクト(`Data/Effect/PlayerBullet/PlayerBullet.efk`)を、プレイヤーの弾(`PlayerBullet`)の見た目として「発射中もずっと追従表示され続ける」形で組み込む作業を開始した。**

**環境構築:**
- 素のDxLibにはEffekseer連携APIが無いことが判明。DxLib公式配布の`EffekseerForDXLib`(バージョンはDxLib本体`3.24f`に合わせて`EffekseerForDXLib_1.80.5_324f`)をダウンロードし、既存の`NovaWing/DxLib_h`フォルダを丸ごと差し替える形で導入。`.vcxproj`のインクルードパス/ライブラリパスは元々`$(SolutionDir)/DxLib_h`を指していたため、追加設定は不要だった。
- 主要API: `Effekseer_Init(パーティクル最大数)`/`Effkseer_End()`(こちらは**ライブラリ側の綴りが`e`抜けの誤字**で、こちらのコードで直してはいけない。実際に`EffekseerForDXLib.h:255`で`void Effkseer_End();`と宣言されている)、`Effekseer_SetGraphicsDeviceLostCallbackFunctions()`、`LoadEffekseerEffect(パス,スケール)`、`PlayEffekseer3DEffect(リソースハンドル)`、`SetPosPlayingEffekseer3DEffect(再生ハンドル,x,y,z)`、`StopEffekseer3DEffect(再生ハンドル)`、`Effekseer_Sync3DSetting()`、`UpdateEffekseer3D()`、`DrawEffekseer3D()`、`DeleteEffekseerEffect(リソースハンドル)`。
- `Application.cpp`: `SetUseDirect3DVersion(DX_DIRECT3D_11)`(DxLib_Initより前に必要)→`DxLib_Init()`→`Effekseer_Init(8000)`の順で初期化。終了処理は逆順で`Effkseer_End()`→`DxLib_End()`。

**ResourceLoaderへの組み込み(完了):**
- 既存の`EffectID`enum(元々空で用意されていた)に`PlayerBullet`を追加。
- `KeepEffect()`を新規実装(`KeepModel()`/`KeepGraph()`と同じパターン)。`LoadAll()`から呼び出し、`ReleaseAll()`にも解放処理(`DeleteEffekseerEffect`)を追加済み。

**次回やること:**
1. ~~`PlayerBullet.h`に再生ハンドル用のメンバ変数(`int`、初期値-1)を追加。~~ → 完了。
2. ~~`PlayerBullet.cpp`の再生/追従/停止実装。~~ → 完了(コンストラクタで`PlayEffekseer3DEffect`、`Update()`で`SetPosPlayingEffekseer3DEffect`、`OnHitEnemy()`/デストラクタで`StopEffekseer3DEffect`)。
3. ~~`UpdateEffekseer3D()`/`DrawEffekseer3D()`/`Effekseer_Sync3DSetting()`の呼び出し箇所。~~ → 完了。`GameScene::Update()`は`UpdateAll()`の直後に`Effekseer_Sync3DSetting()`→`UpdateEffekseer3D()`。`GameScene::Draw()`は**2回目の`DrawAll()`(通常描画)の後、水描画より前**に`DrawEffekseer3D()`。1回目の`DrawAll()`は透過水キャプチャパスなので、ここにエフェクトを混ぜるとオフスクリーンにしか描画されず失敗する(実際に一度誤って混入させて気づいた)。
4. 敵・プレイヤーのCSV化はまだ未着手。次はこちらを再開予定。

**発覚したバグ(2026-07-15・修正提案済み、コード反映は未確認):**
- 弾を撃つと、プレイヤーの現在位置から画面下方向(ワールド原点Y=0付近)に向かって一直線の光の軌跡が残ってしまう問題が発生。
- 原因: `PlayerBullet`のコンストラクタで`PlayEffekseer3DEffect`を呼んだ直後は`SetPosPlayingEffekseer3DEffect`をまだ一度も呼んでいないため、Effekseer側はデフォルト位置(原点付近)でエフェクトの再生・軌跡の起点を作ってしまう。次の`Update()`で初めて弾の本当の位置にジャンプするため、その「原点→本来の位置」への移動そのものが軌跡として残っていた。
- 対処: コンストラクタ内、`PlayEffekseer3DEffect`の直後に`SetPosPlayingEffekseer3DEffect(m_effectPlayHandle, pos.m_x, pos.m_y, pos.m_z)`を追加して、初回から正しい位置にエフェクトを配置する。→ **反映・動作確認済み。解決済み。**

### 進捗（2026-07-15・浮遊敵の死亡爆発エフェクト完成）

**浮遊敵(`FloatingEnemy`)を倒したとき、爆発エフェクト(`EffectID::Death`)を再生して「敵が一瞬で消える」不自然さをごまかす演出が完成した。**

**ハマった点1: ResourceLoaderのコピペミス**
- `KeepEffect()`に爆発エフェクト読み込みを追加した際、`LoadEffekseerEffect`のファイルパスが`PlayerBullet.efk`のままコピペされており、かつ格納先キーも`EffectID::PlayerBullet`のままだった(本来は`EffectID::Death`用の別ファイル・別キーにする必要があった)。結果、`Death`が一度もロードされておらず`GetEffect(EffectID::Death)`で`assert`に引っかかっていた。パスとキー両方を修正して解決。

**ハマった点2: モデルが一瞬で消える不自然さ**
- 最初は体力0の瞬間に即`OnDead()`+エフェクト再生をしていたため、「モデル消滅」と「爆発開始」が完全同時になり、消える瞬間が丸わかりだった。
- 対策として、`IEnemyState`を使ったステートマシン拡張(新規State追加)は「めんどくさい」という理由で見送り、**`FloatingEnemy`に`bool m_isDying`と`int m_dyingFrame`を追加するだけの軽量な実装**を採用。`TakeDamage()`で体力0になったら`m_isDying=true`+エフェクト再生のみ行い、`OnDead()`は呼ばない。`Update()`は`m_isDying`で分岐し、死亡待機中は`true_dead_frame`(30F)経過後に初めて`OnDead()`を呼ぶ。この間モデルはそのまま`Draw()`され続ける。
- **教訓**: ステートマシンへの正式な追加(新クラス作成)よりも、フラグ+カウンタで済む場面では、既存構造を無闇に拡張せずシンプルに済ませる方が実用上speedyに完成する。今回のケースは「死亡演出」という一時的な状態で、他のステートとの相互作用(Enter/Exit)が特に必要なかったため軽量実装で十分だった。

**ハマった点3(本命): エフェクトが敵モデルの「後ろ」に隠れて見える**
- 上記の対策後も、爆発エフェクトが敵モデルのシルエットの背後で光っているように見えて違和感が消えなかった。「モデルを描画しない(=結局即消えるのと同じ)」という誤った代替案を検討したが、これは何も解決しないと気づき却下。
- **真因はEffekseer側のノード設定**: `.efkefc`の各ノード(`Impact`等)に「深度テスト」が有効(チェック有り)になっていたため、実際の奥行きに応じて敵モデルの背後にあるパーティクル部分が隠されていた。「深度書き込み」は元々無効(正しい設定)だったが、「深度テスト」もセットで見る必要があった。
- **対処**: Effekseerエディタで該当ノードの「深度テスト」のチェックを外し、常に手前に描画されるようにして解決。
- **教訓**: 半透明エフェクトがモデルに埋もれて見える系の不具合は、コード側(描画順・座標オフセット)より先に、**Effekseerエディタ側のノードごとの「深度テスト」/「深度書き込み」設定を疑うべき**。同様の問題が今後別のエフェクトでも起きたら、まずここを確認すること。

**残タスク:**
1. 敵・プレイヤーのパラメータCSV化（継続中の積み残し）。
2. ~~`WormEnemy`など他の敵にも同様の死亡演出を追加するかは未検討。~~ → 下記の通り実装・デバッグ完了。

### 進捗（2026-07-15・続き・ワームエネミーの死亡爆発エフェクト完成、水面/エフェクト描画順の修正）

**`WormEnemy`にも死亡演出（頭→胴体の順に一定間隔で爆発エフェクトを出しつつ消えていく）を実装した。ユーザー自身が実装し、Claudeはコードを直接編集せず説明とレビューのみ行う形で進めた（[[feedback_no_direct_code_edit]]のルール通り）。**

**実装の骨子:**
- `WormEnemy`に`int m_deathPlayHandle`、`bool m_isCanPlayEffect`、`int m_deathEffectNum`（何回エフェクトを出したか）、`bool m_isWatingDeath`を追加。
- `TakeDamage()`で体力0になったら`m_isCanPlayEffect = true`、`m_isWatingDeath = true`にするだけ（`FloatingEnemy`と同じ軽量フラグ方式）。
- `Update()`は`m_isCanPlayEffect`が立っていたら、`death_effect_interval`(30F)ごとに頭→胴体の順で`EffectID::WormDeath`を1体分ずつ再生。`m_deathEffectNum`が`m_segmentCount`を超えたら`OnDead()`。

**ハマった点1: if/elseのネストが2段階とも逆だった**
- 最初の実装は「一定間隔かどうか」と「まだ出し終えていないか」の2つの条件の親子関係が逆転しており、`m_frame % interval != 0`の**ほぼ全フレーム**で`else`（`OnDead()`）に落ちて即死亡していた。「未完了かどうか」を外側のif、「一定間隔かどうか」をその内側のifにする形に修正して解決。
- 続けて`m_deathEffectNum++`が「一定間隔のif」の外にあり、間隔待ちの毎フレーム加算され続けるバグも発覚。実際にエフェクトを出したときだけ増やす位置（一定間隔のifブロックの中）に移動して解決。
- **教訓**: 「Aが起きたときにXする、Aが起きていないときはYする」という一見単純なifelseでも、「一定間隔かどうか」のような周期条件と「完了したかどうか」のような状態条件が2つ絡むと、ネストの親子関係を逆にしたまま気づきにくいバグになりやすい。図や具体的なフレーム数を書き出して確認するとよい。

**ハマった点2: `PlayEffekseer3DEffect`直後に`StopEffekseer3DEffect`を呼んでいてエフェクトが一瞬で消えていた**
- 「同じ変数(`m_deathPlayHandle`)に次の再生ハンドルを代入すると、前のエフェクトが消えてしまうのでは」という誤解から、毎回のエフェクト再生後に即座に`StopEffekseer3DEffect`を呼んでいた。
- 実際は`PlayEffekseer3DEffect`を呼ぶたびに完全に独立した新しい再生インスタンスが作られるだけで、変数への代入自体は前の再生には一切影響しない。単発エフェクトは放っておいても自然に再生完了して消えるため、`StopEffekseer3DEffect`は「途中で強制的に打ち切りたいとき」以外は不要。該当行を削除して解決。

**ハマった点3: 爆発済みの部位のモデルがそのまま描画され続ける**
- `m_deathEffectNum`（何番目まで爆発済みか）を使い、`Draw()`側で「頭は`m_deathEffectNum == 0`のときだけ」「胴体iは`m_deathEffectNum < i + 1`のときだけ」描画する条件分岐を追加して解決。爆発した部位から順にモデルが消えていく見た目になった。

**ハマった点4: 爆発エフェクトが海(水面)の背面に隠れて見えない**
- 半透明パーティクル(Effekseerエフェクト)は基本的に深度バッファに書き込まないため、後から描画される不透明な水面ポリゴンが深度テストで常に勝ってしまい、水面より奥にあるエフェクトを覆い隠していた。
- `GameScene::Draw()`で`DrawEffekseer3D()`が`m_pWaterManager->Draw()`より**前**に呼ばれていたのが原因。**`DrawEffekseer3D()`を水の描画の後に移動して解決**。今回のワームは海上で爆発する想定のため、この順序変更で十分（水中の爆発まで正しく前後関係を出したい場合は別途シェーダー側の深度テスト調整が必要になる）。

**別件で発覚・修正: `TitleScene`で「ゲーム終了」を選ぶと例外が発生する不具合**
- `TitleScene::Update()`内、決定キー押下時に`DxLib_End()`を直接呼んでいたのが原因。`DxLib_End()`はメインループの真っ只中で呼ぶべきではなく、呼んだ後もループが回り続けて次のフレームで破棄済みのDxLibリソースにアクセスし例外が発生していた。
- 正しい終了処理(`ResourceLoader::ReleaseAll()`→`Effkseer_End()`→`DxLib_End()`)は`Application::Terminate()`に既に用意されており、`main.cpp`で`Run()`の後に呼ばれる設計だった。
- **対処**: `Application`に`bool m_isExitRequested`と`RequestExit()`を追加。`TitleScene`側は`DxLib_End()`を直接呼ばず`Application::GetInstance().RequestExit()`を呼ぶだけにし、`Application::Run()`のメインループのESCキー判定と同じ`break`条件に`m_isExitRequested`を追加。これでループを正常に抜けてから`main.cpp`側で`Terminate()`が呼ばれる正しい順序になった。動作確認済み。

### 次回以降の方針（2026-07-15時点で合意済み・未着手）

今後は以下の順番で進める合意ができている。

1. ~~`FloatingEnemy`のCSV化。~~ → 完成（下記参照）。
2. ~~`WormEnemy`のCSV化。~~ → 完成、さらに移動方向のCSV対応・螺旋移動のバグ修正も実施（下記参照）。
3. **岩・ステージとプレイヤーの当たり判定（プレイヤーのみダメージを受ける）**。敵とは異なり岩・ステージ側はダメージを受けない一方通行の判定でよいと合意済み。`Rock`にはまだ`Sphere`当たり判定が無い。`Stage`は地形メッシュなので、単純な球判定で表現しきれるか（岩と同様に球で近似するか、複数球を並べるか等）は着手時に改めて検討が必要。**次回はここから着手する。**

### 進捗（2026-07-15・続き・FloatingEnemy/WormEnemyのCSV化完成）

**`FloatingEnemy`と`WormEnemy`両方のCSV化が完成した。** `RockDataSetter`と同じ設計パターン（CSV読み込み→種別ごとに生成→`std::vector`で返す）を踏襲。ユーザー自身が実装し、Claudeはコードを直接編集せず説明とレビューのみ行う形で進めた（[[feedback_no_direct_code_edit]]のルール通り）。

**共通の設計変更:**
- `FloatingEnemy`・`WormEnemy`ともにコンストラクタに`const Vector3& pos`引数を追加し、`SetPos(pos)`で位置を反映する形に変更。`OnInit()`にあった固定オフセット（`m_pos.m_z += 1900.0f`等）は削除。
- `GameScene.h`の`m_pFloatingEnemy`/`m_pWormEnemy`（単数）を`m_pFloatingEnemies`/`m_pWormEnemies`（`std::vector`）に変更し、`Rock`と同じくCSVの行数分だけ生成・登録するループ処理に統一。

**新規作成したファイル:**
- `FloatingEnemyDataSetter.h/.cpp`（`Game/GameObjects/Actors/Charactor/Enemy/FloatingEnemy/`）
- `WormEnemyDataSetter.h/.cpp`（`Game/GameObjects/Actors/Charactor/Enemy/WormEnemy/`）
- `Data/CSV/FloatingEnemyData.csv`（列: `modelID,x,y,z`）
- `Data/CSV/WormEnemyData.csv`（列: `modelID,x,y,z,segmentCount,direction`）

**「文字列→ModelID変換」を`ResourceLoader`に共通化:**
- 元々`RockDataSetter`が自前で持っていた`WStringToModelID`（`if/else if`の羅列）を、ユーザーが「モデルが増えるたびにifを増やすのが非効率では」と指摘。検討の結果、`ResourceLoader::WStringToModelID`という`static`関数に一本化。
- 中身は`if/else if`ではなく`static const std::unordered_map<std::wstring, ModelID>`の対応表＋`find()`方式に変更（新しいモデルを追加する際は対応表に1行足すだけで済む）。`RockDataSetter`・`FloatingEnemyDataSetter`・`WormEnemyDataSetter`すべてがこの共通関数を呼ぶ形に統一。
- **教訓**: 同じ「文字列→enum」変換ロジックが複数箇所で重複しそうになったら、その変換対象のenumを定義しているクラス（今回は`ResourceLoader`）に寄せるのが自然。羅列的なif/elseは`unordered_map`の対応表に置き換えるとスケールしやすい。

**ハマった点1: `WormEnemyDataSetter`のシグネチャ不一致**
- `.h`で`CreateEnemy`の`private:`を書き忘れて`public:`が抜けており、外部から呼べずビルドエラー。
- `segmentCount`をCSVから読む設計にしたにも関わらず、`.h`の関数引数にも`int segmentNum`を残してしまい、`.cpp`側のローカル変数`segmentNum`と名前が被って引数の方が完全に無視される状態になっていた。引数からは削除し、CSVから読んだ値だけを使う形に統一して解決。

**ハマった点2: `WormEnemyData.csv`がタブ区切りになっていた**
- `CSVDataLoader`はカンマ区切り固定でパースするため、タブ区切りのままだと全列が1列として読み込まれ、`dataString[1]`以降で範囲外アクセスになる。カンマ区切りに修正して解決（`FloatingEnemyData.csv`でも同様の問題が過去に一度発生し、同じ修正をした）。

**ハマった点3: Worm出現方向のZ+固定と螺旋移動の座標バグ（対応中・2026-07-16時点でまだ2箇所未修正）**
- 従来`WormEnemy`は`m_pos.m_z += move_speed;`固定でZ+方向にしか進めず、「前からもワームが来るようにしたい」という要望から`float direction`（1.0でZ+、-1.0でZ-）をコンストラクタに追加し`m_pos.m_z += move_speed * m_moveDirection;`に変更する方針で合意。CSVにも`direction`列を追加済み（`Data/CSV/WormEnemyData.csv`、3体分のデータ投入済み）。
- 合わせて、螺旋の中心が常にワールド原点基準（`cosf(...) * spiral_radius`のみ）になっており、CSVで指定した初期x,yが1フレーム目で上書きされてしまう問題も発覚。コンストラクタで受け取った`pos`のx,yを`m_spiralCenter`(`Vector2`、新規追加)として保存し、`Update()`側は`m_spiralCenter.m_x/m_y + 三角関数(...) * spiral_radius`という相対座標にする方針で対応中。
- **見つかったケアレスミス（要修正、2026-07-16時点でまだ未反映）**:
  1. `.h`のコンストラクタ宣言で`float direction//移動方向);`のように、閉じ括弧とセミコロンを行コメントの中に書いてしまい構文エラー（`//`から行末までは全部コメント扱いになるため宣言が閉じない）。`float direction);//移動方向`の順に直す必要がある。
  2. コンストラクタ引数`direction`を受け取ったのに、初期化リストや本体で`m_moveDirection`への代入が漏れており、常にデフォルト値0のままでワームが前進しなくなる。`m_moveDirection(direction)`を初期化リストに追加する必要がある。
  3. 螺旋移動のx,y座標計算で、xとyの両方に`sinf`を使ってしまっている（本来はx側は`cosf`にする必要がある）。このままだと円運動にならず斜め45度の直線を往復するだけの動きになる。
- **次回作業再開時は、上記3点が実際に修正されているか`WormEnemy.h`/`.cpp`を読んで確認してから続きを進めること。**
- **教訓**: 円・螺旋運動の実装では「x軸とy軸(またはx軸とz軸)で必ず異なる三角関数(cos/sin)を使う」という基本を書き換え作業のたびに見落としやすい。動きが「直線的」「斜め」になったら、まずcos/sinの取り違えを疑うとよい。

### 進捗（2026-07-15・続き・自機をレティクルより手前に描画）

**本家スターフォックスをプレイ中に気づいた仕様「レティクル(照準UI)より自機の方が手前に描画される」をNovaWingでも再現した。**

**背景:**
- NovaWingの`ReticleUI`は`DrawRotaGraph`(Zバッファを無視する2D描画)を使っており、`GameScene::Draw()`内でも全3Dオブジェクト・水・Effekseerエフェクトの一番最後にUI(`m_pUIManager->Draw()`)を呼んでいたため、レティクルは常に自機を含めた全てより手前に表示されていた。
- 本家はこれと逆で、自機がレティクルより優先して手前に表示される場面がある。

**対処:**
- `GameScene::Draw()`の`m_pUIManager->Draw()`の直後に、`m_pPlayer->Draw()`をもう一度手動で呼ぶ処理を追加。`Player::Draw()`は`ApplyMatrix`→`UpdateShaderMatrixData`→`MV1DrawModel`という自己完結した処理なので2回呼んでも副作用がなく安全（Zバッファに書き込む通常の3D描画のため、UIの2D描画ピクセルの上に強制的に上書きされる形になる）。
- 描画順序: 3Dオブジェクト一式(自機含む)→水→Effekseerエフェクト→UI(レティクル含む)→自機(2回目)、という並びになった。
- **教訓**: 「UIは常に最前面」という設計は、ジャンルによっては仕様として崩したい場合がある(今回は自機とレティクルの前後関係)。その場合、全体のレイヤー構造を作り直すより、「該当オブジェクトだけ最後にもう一度描画する」という力技の方が変更範囲が小さく安全なことがある。

### 進捗（2026-07-16・カメラ調整に着手：視錐台クランプ完成、動的フレーミングは次回）

**きっかけ:** Switch2版『スターフォックス』（N64版のリメイク、2026-06-25発売）のカメラ挙動を参考にしたいという相談があった。Web検索では具体的なアルゴリズムまでは分からず、動画URLも渡されたがClaudeは動画を視聴できないため、動画要約AIが書き起こした仕様プロンプトをユーザーが用意し、それを元に実装方針を固めた。

**要件（動画要約AIが書き起こしたプロンプトより）:**
1. 視錐台（Frustum）による移動制限：プレイヤーの移動範囲を画面内に制限する「見えない壁」。
2. カメラ相対の移動制御：スティック入力を常にカメラの向き基準の相対移動として処理する。
3. 動的なフレーミング：プレイヤーが画面端に寄ったらカメラがわずかにパンして視界を維持する。ただし一定角度でハードリミット。

**方針決定:** レール移動（スプライン曲線）の導入は見送り、現状の「カメラのZ位置＝プレイヤーのZ位置＋固定オフセット」という既存設計を保ったまま、まず要件1（視錐台クランプ）から着手することで合意。

**1. 視錐台クランプ（完成・動作確認済み）:**
- 考え方: `tanf(視野角/2) × カメラからの距離`で画面の縦方向の半分の高さ(ワールド座標)を求め、それにアスペクト比(`screen_width/screen_height`)を掛けて横方向の半分の幅を求める。この範囲でプレイヤーのx,yを`std::clamp`する。
- `CameraBase`に`GetFov()`と`GetFrustumHalfSize(float distZ)`(戻り値`Vector2`)を追加。計算ロジックを`CameraBase`に一本化し、`Player::ClampPosition()`側はその結果を使うだけの薄い形にした（最初`Player`側に直接計算式を書いていたが、動的フレーミングでもカメラ側で同じ計算が必要になることが分かり、共通化する設計に変更した）。
- **ハマった点**: `Size`構造体の`m_width`/`m_height`が`int`型のため、アスペクト比の計算(`wsize.m_width / wsize.m_height`)が整数同士の割り算になり小数点以下切り捨てで`1`になってしまうバグがあった。`static_cast<float>`で片方(または両方)をキャストして解決。
- **ハマった点2(本命)**: クランプ自体は機能したが「変な位置で止まる」不具合が発生。原因は`CameraBase::Update()`側で、注視点(`m_targetPos`)のx,yがプレイヤーの位置に完全追従する処理(`UpdateTargetPos()`)がまだ生きていたため、カメラ自体もLerpを介してプレイヤーに追従してしまい、「プレイヤーがどこまで動いていいか」の基準がフレームごとにズレていたこと。検証のため`UpdateTargetPos()`のx,y追従処理を一時的にコメントアウト（カメラを完全固定）したところ、クランプが正しく機能することを確認できた。

**前プロジェクト由来の「手動カメラ回転」の名残を整理:**
- `CameraBase`はユーザーが前プロジェクトから流用したクラスで、元々は右スティックで手動回転できるカメラだったが、今回のプロジェクトに合わせて改造する過程で中途半端に壊れていた（`InputManager`から入力を取得しているのに一切使っていない、`camera_rotate_speed`/`camera_rot_dead_zone`/`camera_fixed_y`/`rot_rimit_up`/`rot_rimit_down`が全部未使用など）。
- 今回のスターフォックス要件には「手動でカメラを回す」操作は含まれておらず、既存の`m_angleX`/`m_angleY`（回転角、現状は初期値0のまま固定されて死んでいた）を「動的フレーミングの自動追従角度」として再利用する方針に決定。未使用だった変数・定数・include(`InputManager`)は全て削除してコードを整理した。

**次回（学校で継続予定）: 2. 動的なフレーミング（未着手）**
- `CameraBase::Update()`内、Lerp計算(`m_targetPos = Vector3::Lerp(...)`)の後、`SetCameraPositionAndTarget_UpVecY`の前に実装する箇所を確保済み（コメント`//プレイヤーが`のみ書きかけで残っている）。
- 計算方針で合意済み: ①プレイヤーの生の位置を取得 ②カメラとのZ距離を求める ③`GetFrustumHalfSize(distZ)`で視錐台範囲を求める（既存の共通関数を再利用） ④プレイヤー位置がその範囲に対して何%の位置にいるか(0〜1)を求める ⑤しきい値(例:0.7=画面の70%)を超えた分だけ目標回転角度を計算する(超えた分を0〜1に正規化して最大パン角度を掛ける) ⑥目標角度に`m_angleX`/`m_angleY`をLerpで滑らかに近づける。
- しきい値・最大パン角度の具体的な初期値は未決定。まず仮の値（しきい値0.7、最大角度15度など）で実装し、実際に動かしながら調整する方針で合意。
- 要件2（カメラ相対の移動制御）はまだ未着手・未検討。動的フレーミングの後に取り組む想定。

### 進捗（2026-07-17・動的フレーミングを回転方式から位置追従方式に作り直し）

**しきい値ベースの回転（`m_angleX`/`m_angleY`）で動的フレーミングを実装したが、原作の見た目と違いすぎたため、方式そのものを変更した。**

**最初の実装(しきい値+回転)がボツになった経緯:**
- しきい値(0.7)を超えたらパンする方式を完成させたが、実機で試すとプレイヤーがスティックをフルに倒して円を描いたとき、意図した「四角い枠をなぞる動き」ではなく**逆台形**になり、原作の挙動とかけ離れていた。
- 「そもそもプレイヤーが動くたびに常にカメラが少し追従し、画面端に近づくほど追従が弱まる」という原作寄りの新しい仕様に方針転換。しきい値方式は全面的に不採用となった。

**新方式の実装（完成、ただし別バグが判明・修正済み。詳細は下記）:**
- **カメラの位置(`m_pos`)**: `m_pos.m_x = playerPos.m_x * camera_move_strength_x`のように、プレイヤー位置に係数を掛けるだけのシンプルな比例式に変更（画面端で弱まるゲイン方式は不採用、この単純比例で「それっぽい」という評価だった）。
- **カメラの回転(`m_angleX`/`m_angleY`)は最終的に完全に削除**。「注視点は固定、カメラの位置だけ動く」を`SetCameraPositionAndTarget_UpVecY`に渡すと、カメラは常に注視点を見ようとするため、位置が動くだけで自動的に向きも変わる（=回転行列を明示的に組む必要がなかった）と気づいたのが削除の決め手。`CameraBase.h`から`m_angleX`/`m_angleY`、`GetAngleY()`、`Matrix4x4`/`Quaternion`関連のincludeも全部削除してクラスが大幅にシンプル化された。
- **注視点(`UpdateTargetPos()`)**: `m_targetPos = pPlayer->GetPos() + (-pPlayer->GetForward() * camera_to_target)`で、プレイヤーの正面方向のかなり先（`camera_to_target = 15000.0f`）を注視点にする方式に変更。

**発覚した重大バグ(2026-07-17・学校で発覚、家で修正完了):**
- `CameraBase::Update()`内、Y座標の計算式(`playerPos.m_y * camera_move_strength_y * -1.0f + camera_offset_y;`)が**どこにも代入されておらず**、計算した値を捨てるだけの式文になっていた（`m_pos.m_y =`が抜けていた）。→ 代入を追加して解決。
- **本命: `CameraBase`のコンストラクタで`m_pos`の初期値を一度もセットしていなかった**。`GameObject`基底クラスのデフォルト値(0,0,0)のまま`SetCameraPositionAndTarget_UpVecY`が呼ばれるため、ゲーム開始直後はカメラがプレイヤーとほぼ同じ位置にいる状態になり、`distZ`(カメラ〜プレイヤーのZ距離)が異常に小さい値(14程度)になっていた。これが`GetFrustumHalfSize(distZ)`の戻り値も異常に小さくし、`Player::ClampPosition()`内の`std::clamp(pos, min, max)`で`min > max`になり`invalid bounds argument`でクラッシュしていた。
  - **対処**: コンストラクタに、`Update()`と全く同じY/X/Zの初期位置計算ロジックを追加して解決。
  - **教訓**: `Update()`で毎フレーム計算しているメンバー変数でも、**コンストラクタ(1フレーム目が来る前)の初期値が正しくないと、その1フレーム目だけで別の場所(今回は`Player::ClampPosition()`)がクラッシュすることがある**。「動いているように見える=正しく初期化されている」ではない。Releaseビルドで長期間気づかれなかった（Debugのassertと違い、Releaseは`std::clamp`の不正な引数を静かに素通りすることがあるため）。

**残タスク（動的フレーミング関連）:**
- 現状は「プレイヤー位置に単純な係数を掛けるだけ」の追従なので、まだ本格的な「画面端で追従が弱まる」表現はできていない。今後見た目を見ながら調整するか、必要になったら再度作り込む。
- 要件2（カメラ相対の移動制御）は引き続き未着手。

### 進捗（2026-07-17・続き・コースティクス効果、デバッグ中）

**`WaterPS.hlsl`に、水底の光の模様（コースティクス）を追加しようとしているが、まだ模様が表示されない状態でデバッグ中。**

**実装したもの:**
- `ResourceLoader`に`GraphicID::Caustics`を追加、`Data/Image/Caustics.png`をロード（ロード自体は`assert`を通過しており成功を確認済み）。
- `WaterManager`: `m_causticsH`メンバーを追加、`Draw()`でスロットt7にバインド（テクスチャ解除ループも8スロットに拡張済み）。
- `WaterPS.hlsl`: `causticsTex : register(t7)`を追加。`input.worldPos.xz`をUVに変換し、時間でずらした2枚のUVをサンプリングして掛け合わせる（`baseCausticsCol1 * baseCausticsCol2`）ことで揺らめく模様を作る設計。

**設計の変遷:**
- 最初は`reveal`（岩などの水中物体が透けて見える強さ）を使ってコースティクスの強度を決めていたが、「岩の周辺にしか出ない」という問題が発覚（意図は海全体にコースティクスを出すことだった）。
- `reveal`ではなく、カメラからの距離(`dist`)を使った疑似的な深浅表現（B'案: 近いほど強く、遠いほど`smoothstep`で減衰）に変更して解決。`distanceStrength = 1.0f - smoothstep(1500.0f, 3000.0f, dist);`

**現在デバッグ中の問題:**
- コースティクスの模様が全く表示されず、常に無地の色（最初は白飛び、`finalCol += causticsFinal.rgb * 0.3f`のように弱める係数を追加してからは水色一色）になっている。
- **切り分け済み**: `causticsUV1`をそのままRGBとして可視化するデバッグ(`return float4(frac(causticsUV1), 0.0f, 1.0f);`)では正しい繰り返しグラデーションパターンが出た → **UV座標の計算自体は正しい**ことを確認済み。
- **未確認**: `causticsTex.Sample(smp, frac(causticsUV1))`の**結果そのもの**（`baseCausticsCol1`）をそのまま`return`して可視化するデバッグは、まだ正しく実行できていない（一度`return baseCausticsCol1;`を指示したが、実際には別の`return`文と混同していて未実施だった）。**次回はここから再開**：`return baseCausticsCol1;`を実際に試して、サンプリング結果自体が模様を持っているか、それとも単色になっているかを確認すること。
- Caustics.png自体は正常なテクスチャ（灰色の細胞状模様+白い網目の縁取り）であることを画像ビューアーで確認済み。エクスプローラーのサムネイルが真っ白に見えたのはサムネイルキャッシュの表示不具合で、ファイル自体の問題ではない。
- `m_causticsH`のロードは`assert(handle >= 0);`を通過しているのでロード自体の失敗ではなさそう。`.vcxproj`のFxCompile設定（Release含む）も確認済みで、`.pso`の鮮度が原因の可能性も低い。
- **デバッグ中に本題と無関係な`Player::ClampPosition()`のクラッシュ（上記の動的フレーミングのバグ）に遭遇し、そちらの解決を優先した**ため、コースティクスの原因究明はまだ完了していない。

**次回やること:**
1. ~~`return baseCausticsCol1;`（サンプリング結果そのもの）を実際に可視化して、テクスチャの中身が正しく読めているか確認する。~~ → 完了、原因判明（下記参照）。
2. ~~もし模様が見えない場合、`SetUseTextureToShader(7, m_causticsH);`のタイミングや、他のテクスチャスロットとの干渉を疑う。~~ → スロット制限ではなかった（下記参照）。

### 進捗（2026-07-17・続き2・コースティクス完成！）

**コースティクス効果が完成した。海全体に網目状の光の模様が揺らめく、狙い通りの見た目になった。**

**原因の切り分け手順（今後の参考用）:**
1. `causticsUV1`をそのままRGBとして可視化 → 正しいグラデーション → **UV座標の計算は正常**と確認。
2. スロット制限を疑い、`skyBottom`(t5)のバインドを外して`causticsTex`をt5に差し替えて実験 → それでも真っ白 → **8スロット制限が原因という仮説は誤りと判明**。
3. `causticsTex`の代わりに、既に正常動作している`skyFront`を同じUVでサンプリングして`return` → 正しく空の絵が表示された → **UV・サンプリング機構自体は正常、`m_causticsH`(Caustics.png)側に原因がある**と確定。
4. `baseCausticsCol1.a`（アルファチャンネル）だけを可視化 → **ここで初めて網目模様がくっきり見えた**。

**真の原因**: `Caustics.png`は、**RGBではなくアルファチャンネルに網目模様の情報が入っている**特殊なテクスチャだった（背景が透明、模様部分が不透明という一般的なアルファマスク画像）。`causticsColor = baseCausticsCol1 * baseCausticsCol2;`のようにRGBAをまるごと掛け算していたため、RGB成分（模様の情報を持たない、ほぼ均一な値）だけが結果に反映され、模様が消えて単色に見えていた。

**修正内容:**
- `causticsAlpha = baseCausticsCol1.a * baseCausticsCol2.a;` のように**アルファ値同士を掛け算**して模様の強さを求める。
- `causticsFinal = float4(1.0f,1.0f,1.0f,1.0f) * causticsAlpha * distanceStrength;` のように、まず白色を作ってからアルファ値・距離減衰を掛けて「光の模様の明るさ」として使う。

**教訓（デバッグ手法として重要）:**
- 「模様入りのテクスチャなのに単色にしかならない」系のバグは、**RGBだけでなくアルファチャンネルに模様が入っている可能性**を疑うこと。画像ビューアーで見た目が正常でも、実際にシェーダーが読んでいる成分（`.rgb`か`.a`か）が模様を持っているとは限らない。
- 切り分けの際、「既に正常動作しているテクスチャ(`skyFront`)を同じUV・同じSample呼び出しで代わりに使ってみる」という比較実験が、「UV/サンプリング機構側の問題」か「対象テクスチャ側の問題」かを一発で切り分けるのに有効だった。
- `.a`単体を可視化する（`return float4(col.a, col.a, col.a, 1.0f);`）のは、アルファチャンネル情報を疑うときの定番デバッグ手法として今後も使えそう。

**残タスク:** 特になし、コースティクス機能は完成。見た目の微調整（`distanceStrength`の減衰距離、`* 0.3f`の強さ係数など）は今後好みで変更可。

### 進捗（2026-07-17・続き3・デバッグ用リスタート機能）

**Rキー(パッドはSTARTボタン)でGameSceneを即座に再初期化できるデバッグ機能を追加した。** シェーダー修正のたびに毎回ゲームを終了→再起動する手間を解消する目的。

- `InputManager.cpp`の`#ifdef _DEBUG`ブロック（既存の`left`/`right`と同じ場所）に`"restart"`イベント(`KEY_INPUT_R`/`PAD_INPUT_START`)を追加。
- `GameScene::Update()`に`#ifdef _DEBUG`で囲んだ判定を追加、`InputManager::IsTriggered("restart")`で`m_controller.ChangeScene(std::make_shared<GameScene>(m_controller), 0.0f)`を呼ぶだけ（`GameoverScene`遷移と同じ`ChangeScene`パターンを流用、フェード無し即切り替え）。
- Releaseビルドには含まれないので本番に影響なし。

### 進捗（2026-07-17・続き4・チャージ完了時のプレイヤー発光エフェクト完成）

**チャージショットが撃てる状態になったことが視覚的にわかるよう、プレイヤー機体に追従する光のエフェクトを追加した。**

**実装の骨子:**
- `ResourceLoader`に`EffectID::Charging`を追加し、`Data/Effect/Charging/Charging.efk`（新規作成したエフェクトアセット）をロード。
- `ChargeReadyState`（チャージ完了〜発射待ちのステート）に`int m_chargingPlayEffectH`を追加し、`Enter()`で再生+初回位置セット、`Update()`で毎フレーム位置更新、`Exit()`で`StopEffekseer3DEffect`という、既存の`PlayerBullet`/死亡エフェクトと同じパターンで実装。ユーザー自身が実装し、一発で正しく書けていた（過去の「初回位置セット漏れ」の教訓が活きた）。

**ハマった点1: チャージエフェクトがプレイヤーの後ろに隠れて見えない**
- `GameScene::Draw()`は「レティクルより自機を優先描画するため、最後にもう一度`m_pPlayer->Draw()`を呼ぶ」という既存の仕組み（2026-07-15の進捗参照）を持っていたが、これが`DrawEffekseer3D()`より**後**に実行されていたため、プレイヤーの3D描画(Zバッファ書き込みあり)がエフェクトを覆い隠していた。
- **対処**: `DrawEffekseer3D()`を、最後のプレイヤー再描画のさらに後ろに移動して解決。描画順序は「通常DrawAll→水→UI→プレイヤー再描画(レティクル対策)→Effekseerエフェクト」という並びになった。

**ハマった点2: プレイヤーが回転するとエフェクトが機体の先端から外れる**
- 当初、エフェクトの位置を「プレイヤー位置 + 固定のZオフセット」（ワールド座標系の固定ベクトル）で計算していたため、プレイヤーが旋回・宙返りするとオフセットの向きがついてこず、エフェクトが機体からズレて見えた。
- **対処**: 固定オフセットの代わりに、`GameObject::GetForward()`（回転を反映した前方向ベクトル）にオフセット距離を掛けたものをプレイヤー位置に足す形に変更。`playerPos + pPlayer->GetForward() * オフセット距離`という式で、機体の回転にエフェクト位置も追従するようになった。
- **教訓**: 「機体に追従するエフェクト」を、機体の正面や特定パーツに固定したい場合、ワールド座標の固定オフセットではなく`GetForward()`(またはRight/Up)のような回転済み方向ベクトルを使ったオフセット計算にする必要がある。今後同種のエフェクト(武器の発射口、翼端など)を実装する際も同じ考え方が使える。

**残タスク:** 特になし、この機能は完成。

### 進捗（2026-07-17・続き5・チャージ完了後、チャージ中エフェクトの育成対応）

**チャージショットのエフェクトを「チャージ開始→完了まで徐々に大きくなり、完了後もそのまま光り続ける」形に改良した。**

- Effekseer側で既にエフェクト自体が時間経過(30F)で1.0サイズまで成長するように作られているため、コード側でスケール制御は不要だった。
- `Player`に`int m_chargingEffectH`（チャージエフェクトの再生ハンドル）を追加し、`ChargeShootState`(チャージ中)と`ChargeReadyState`(チャージ完了後)の**両方のステートで同じ再生インスタンスを共有**する設計にした。`ChargeShootState::Enter()`で再生してハンドルをPlayerに保存、`ChargeReadyState::Enter()`ではPlayerから既存ハンドルを取得するだけで新規再生しない。
- 両ステートの`Update()`に、`playerPos + (-GetForward()) * effect_offset_z`という機体前方向オフセット付き追従処理を追加（過去の「固定Zオフセットだと回転時にズレる」教訓を踏襲）。

**ハマった点: 通常ショットを撃っただけでもエフェクトが出てしまう**
- `NormalShootState`でボタンを押すと一瞬`ChargeShootState`に遷移し`Enter()`でエフェクトが再生されるが、`ChargeShootState::Exit()`が空実装のままだったため、チャージ未完了でキャンセルされた場合にエフェクトが停止されず残っていた。
- **対処**: `ChargeShootState::Exit()`に`if (m_chargeFrame < charge_comp_frame) { StopEffekseer3DEffect(...); }`を追加。完了時（`ChargeReadyState`へ引き継ぐ場合）は何もしない、未完了キャンセル時のみ停止、という分岐で解決。
- **教訓**: 複数ステートでエフェクトの再生インスタンスを共有する設計にする場合、「そのステートを抜けるすべての経路」（正常完了/中断キャンセル）で、エフェクトを停止すべきかどうかが経路ごとに異なることがある。`Exit()`を空実装のままにせず、遷移先条件で分岐する必要がないか必ず確認すること。

**残タスク:** 特になし、この機能は完成。

### 保留中: マジックナンバーの定数化調査（2026-07-17・着手待ち）

**ユーザーからプロジェクト全体のマジックナンバー(定数化されていない数値リテラル)を洗い出す依頼があり、調査エージェントで探索したが、実際の修正はまだ着手していない。次回このタスクを再開する際は、まずこのメモを見てから着手すること。**

**調査で見つかった、複数ファイルで重複している値（要・方針決定）:**
- `1000.0f`（スティック入力の正規化用の割る数）が`Player.cpp`、`Movement/MovingState.cpp`、`Movement/IdleMovementState.cpp`、`Movement/GaugeActionStateBase.cpp`、`Rotation/DefaultRotationState.cpp`の**5箇所**でバラバラに直書き。
- `100.0f`（ブースト/ブレーキゲージの最大値）が`Player.cpp`内で3箇所（Boost判定・Brake判定・`std::clamp`）に重複。
- `0.1f`（スティックの倒し量のしきい値、Idle⇔Moving状態遷移の境界）が`MovingState.cpp`と`IdleMovementState.cpp`で同じ値を別々に直書き。
- `1`（ワームエネミーとの接触ダメージ）が`Manager/CollisionManager.cpp`の190行目・199行目(頭・胴体)で重複。

**方針が未決定な点**: これらの重複値を「各ファイルごとに個別の定数として残す(影響範囲が小さく安全)」か「共通の1箇所(例:InputManagerにスティック正規化定数を置く等)にまとめて一元管理する(変更が楽になるが設計変更が伴う)」か、ユーザーとまだ合意していない。次回着手時にまず相談すること。

**調査で見つかった、単一ファイル内の未定数化（重複なし、着手しやすい）:**
- `Player.cpp`: `-0.2f`（宙返り判定のスティック下入力しきい値）。
- `Rotation/DefaultRotationState.cpp`: `DX_PI_F / 8.0f`（機体の最大傾き角度、X/Y両方で使用）。
- `SpecialAction/SomersaultState.cpp`: `0.6f`（宙返り開始時にY回転をLerpで180度へ寄せる係数）。
- `Manager/WaterManager.cpp`: `0.01f`（波アニメーションの時間経過速度）、`300.0f`（カメラZ座標に足す水面オフセット）。
- `Game/GameObjects/Actors/Stage/Stage.cpp`: `Vector3(0.0f, 0.0f, 17700.0f)`（ステージの初期配置座標。他のActor(`Player`など)は`first_pos`のような名前付き定数にしているのに、ここだけ直書きのまま）。

**調査済みで問題なし（既に定数化されている）と確認できたファイル**: `Movement/BoostState.cpp`、`BrakeState.cpp`、`FloatingEnemy/ActiveState.cpp`、`HideState.cpp`、`LeaveState.cpp`、`FloatingEnemy.cpp`、`WormEnemy.cpp`、`Shoot/NormalShootState.cpp`、`ChargeReadyState.cpp`、`ChargeShootState.cpp`、`ChargeBullet.cpp`、`BulletBase.cpp`、`TargetManager.cpp`、`InputManager.cpp`、`CameraBase.cpp`。マネージャー系(`GameObjectManager`、`UIManager`、`LightingManager`、`WaterRevealManager`)はシェーダーレジスタ番号・ループ境界等の構造的な値のみで対象外と判断。

### 進捗（2026-07-18・マジックナンバー再調査完了、岩のニアクリップディゾルブ完成）

**マジックナンバー再調査（完了）:**
- 調査エージェントで再調査した結果、前回(2026-07-17)指摘した重複値・単一ファイル未定数化はすべて修正済みと確認。`stick_input_max`、`gauge_max`、`worm_contact_damage`、`somersault_stick_threshold`、`max_tilt_angle`、`enter_rot_lerp_t`、`time_speed`/`water_z_offset`等、意味のある名前の定数に置き換わっていた。
- 新たに軽微な項目2件を発見（優先度低、未対応）: `Player.h`の`m_gauge`初期値`100.0f`が`Player.cpp`の`gauge_max`定数と重複した直書きのまま、`Scene/GameoverScene.cpp`のデバッグUI座標・色が直書き。
- 結論: 「概ね解消されている」で調査完了。

**岩のニアクリップディゾルブ（完成）:**
- 課題: カメラのNear(200.0f)に近づいた岩などのオブジェクトが、ポリゴンがスパッと切れて不自然に見えていた。フェード(半透明)案とディゾルブ(discard)案を検討し、過去の透過水デバッグで苦労した「半透明・乗算済みアルファ」の問題を再燃させたくないという理由でディゾルブ案を採用。
- ノイズテクスチャ`Data/Image/Noise.png`を新規用意し`ResourceLoader`に`GraphicID::DissolveNoise`として登録。`ShaderRegister.h`に`tex_noise = 4`を追加（既存の`tex_diffuse`〜`tex_emission`と同じ並びの命名規則に統一）。
- `LightingPS.hlsl`にディゾルブロジックを追加。**ユーザー自身がシェーダー計算式を考え、Claudeはレビューとヒント出しに徹する形で進めた（学習目的の方針を踏襲）**。
  - `cameraToPixelD = distance(cameraPos, input.worldPos)`でカメラ〜ピクセル距離を算出。
  - `noise_threshold = 1.0f - smoothstep(near, start_disolve, cameraToPixelD)`で、Near(200)に近づくほど閾値が1に近づく値を計算（`smoothstep`の引数を意図的に逆順(400,200)にする案が最初出たが、HLSL仕様上`edge0>edge1`は未定義動作のため、正順で呼んで`1.0f -`で反転する形に修正）。
  - `if (noiseCol.r < noise_threshold) discard;`でノイズの暗い部分から順にピクセルを消す。
- **ハマった点(本命): 岩のUVレイアウトが模様表示に向いていなかった**。`input.uv`でノイズをサンプリングすると、岩全体がノイズ模様ではなく面ごとのベタ塗り(単色)にしかならなかった。UV可視化(`return float4(input.uv,0,1)`)で「UV展開が面ごとに極小範囲へ押し込まれている(法線マップ用途向けの展開)」と判明。
- **対処**: `input.worldPos.xy`をノイズのUVとして使う方式に変更（`float2 worldBaseUV = input.worldPos.xy / noise_uv_scale;`）。ワールド座標ベースなのでUV展開に依存せず、連続的な模様になった。
  - ハマったミス: 最初`float worldBaseUV = ...`と型を`float`のままにしてしまい(`float2`であるべき)、コンパイルエラーで気づいて修正。
- 最終確認で「岩の一部だけディゾルブじゃない消え方に見える」という懸念が出たが、`noise_threshold`を直接可視化した結果、同じモデル内でもピクセルごとにカメラからの距離が微妙に違うため閾値にムラが出るのは正常な挙動と判明（カメラに近い部分から先に消え始めるのは意図通り）。

**教訓（デバッグ手法として蓄積）:**
- 「テクスチャの模様が均一なベタ塗りにしか見えない」系のバグは、以前のコースティクス問題(アルファチャンネル)とは別に、**モデルのUV展開そのものが模様表現に向いていない**（法線マップ等ディテール用の極小UV展開）というパターンもあると判明。`input.uv`を色として可視化し、面ごとにベタ塗りの色パッチになっていないか確認するのが有効。
- UVレイアウトに依存したくない模様表現（ディゾルブ、床の模様等）は、`input.worldPos`の適切な2成分を縮小して代替UVとして使うと、UV展開に関係なく安定した模様が得られる。

**残タスク:**
- 現状Rockのみ対応。`Player`/`FloatingEnemy`/`WormEnemy`など、`LightingPS.hlsl`を使う他のActorにも同様に`SetUseTextureToShader(ShaderRegister::tex_noise, ...)`のバインド追加が必要（Rockで動作確認できたので横展開するだけ）。
- `near`/`start_disolve`/`noise_uv_scale`の値は仮の初期値のまま。見た目を見ながら微調整の余地あり。

### 進捗（2026-07-20・企業見学に向けたシェーダー理解の口頭確認＋WaterPS/LightingPS/StagePSの定数化）

**背景**: 学校に企業の方が来てコードを見てもらう予定があり、既存の海シェーダー(`WaterVS.hlsl`)の実装を「なぜこうなっているか」自分の言葉で説明できるか、クイズ形式で繰り返し確認する回。

**シェーダー理解の口頭確認（`WaterVS.hlsl`中心）:**
- `WaterPS.hlsl`の`captureCol.a`が「透明度」ではなく「距離をnear~farで正規化した値」であることを、`CapturePS.hlsl`側の実装と突き合わせて確認。
- reveal処理（水面下の物体を透かす仕組み）の`delta = abs(captureCol.a - normDistNtoF)`が「物体までの距離」と「水面までの距離」の差＝「物体が水面からどれだけ深い位置にあるか」を表すことを、具体例（手前にある場合／深い場所にある場合）で確認。最初は大小関係を逆に捉えていたが、修正後は正しく説明できた。
- `captureCol.a > 0.001f`のガード（何もない背景でdeltaがたまたま小さくなり誤ってrevealが立ってしまうのを防ぐ）の必要性も確認。
- **微分を初めて学ぶ状態だったため、`sin`の傾き＝`cos`という関係、合成関数の微分（`sin(3x)`を微分すると`3cos(3x)`になる理由）を、坂道・自転車の例え話まで掘り下げて説明**。最終的に「周波数(frequency)が高い＝波が密集＝傾きが急、だから微分結果に`wave_frequency`が追加で掛かる」という理屈まで自分の言葉で言えるようになった。
- 法線ベクトル`float3(-(傾きX), 1.0f, -(傾きZ))`について、マイナス符号の意味（坂の傾きと法線の向きは逆）、Y成分が常に`1.0f`固定の理由（傾きが0のとき法線は真上を向くべきで、Yが0だと長さ0のベクトルになり方向が定義できなくなるため）を確認。
- ノイズの傾き計算（`noiseEpsilon`を使った数値微分）についても、「近い点で測るほど正確な傾きに近づく」という直感を坂道の例えで確認。
- 複数の波の傾きを単純に足し合わせている理由（高さの合成が線形和なら、傾きの合成も線形和になる）も確認。
- 同じ範囲を一度クイズし直して定着を確認する場面もあり、「理解できた気になっているだけかもしれない」と自己申告して再確認を求める場面があった。

**マジックナンバー精査・定数化（`WaterPS.hlsl`/`LightingPS.hlsl`/`StagePS.hlsl`）:**
- 就活を見据えて「本当に全部定数化できているか」の精査を実施。調査エージェントで洗い出した結果、`WaterVS.hlsl`は既に`static const`で丁寧に定数化されているのに、同じ水シェーダーである`WaterPS.hlsl`側は未定数化のマジックナンバーが大量に残っていることが判明（一貫性の欠如）。
- `WaterPS.hlsl`: 泡・霧・コースティクス・specular・フレネル・reveal閾値・浅い色/深い色・環境光をすべて`static const`化。デバッグ用の`//return float4(...)`コメント残骸も削除。
- `LightingPS.hlsl`: 環境光(`ambient_light`)、法線マップ強調度(`normal_map_strength`)、smoothnessの範囲(`smoothness_min`/`smoothness_range`)を定数化。実装と矛盾していた「一時的に固定値で確認」というコメントも削除。
- `StagePS.hlsl`: 環境光を定数化。
- **あえて対応しなかった箇所**: `near_clip`/`far_clip`が`CapturePS.hlsl`/`WaterPS.hlsl`/`LightingPS.hlsl`の3ファイルに重複定義されている問題は、共通ヘッダ(`.hlsli`)切り出しが必要な設計変更のため今回はスコープ外として据え置き。`causticsFinal`の白色`float4(1,1,1,1)`は意味が単純すぎるため定数化不要と判断。
- ビルド＆動作確認OK。見た目の変化なし。

**残タスク:**
- `near_clip`/`far_clip`の3ファイル間重複を解消したい場合は、共通`.hlsli`インクルードファイルへの切り出しを検討。
- ディゾルブ関連の残タスク（上記参照）は未着手のまま。

### 進捗（2026-07-20続き・プロジェクト全体の重複コード精査＋Actor::DrawWithLightingへの共通化）

**背景**: マジックナンバー精査に続き、就活を見据えて「無駄なコード（重複・関数化できる箇所）」がないかプロジェクト全体を調査。調査エージェントで敵クラス群/DataSetter群/シェーダー間/State系を横断的に洗い出し、実物を目視確認したうえで優先度付け。

**発見した重複（優先度順）:**
1. **敵・岩の描画処理が丸ごとコピペ（対応済み）**: `Rock::Draw()`、`FloatingEnemy::DrawEnemy()`、`WormEnemy::DrawWormHead()`/`DrawWormBody()`の4箇所で「テクスチャSet→ApplyShader→BindShaderBuffers→MV1DrawModel→テクスチャ解除→ResetShader→ReleaseShaderBuffers」という同一パターンが繰り返されていた。
2. DataSetter群（`RockDataSetter`/`FloatingEnemyDataSetter`/`WormEnemyDataSetter`）の「CSVロード→ループ→modelID/pos変換→生成」の骨格が3クラスで重複（未対応、次回候補）。
3. `LightingPS.hlsl`/`WaterPS.hlsl`間で視線ベクトル・反射ベクトル・specular計算式が重複（未対応）。
4. `WaterPS.hlsl`の`SampleSkyReflection`内、X/Y/Z面のUV計算パターンが3回繰り返し（未対応、可読性とのトレードオフあり優先度低）。

**対応した内容（1番）:**
- `Actor`基底クラス（`Charactor`ではなく`Actor`。`Rock`が`Charactor`を継承せず直接`Actor`を継承しているため、共通化の置き場所は`Actor`が適切と判断）に`DrawWithLighting(const std::vector<std::pair<int,int>>& textures)`を追加。テクスチャの(レジスタ番号, ハンドル)ペアのリストを受け取り、セット→シェーダ適用→描画→解除までを一括で行う。
- `Rock::Draw()`（28行→24行）、`FloatingEnemy::DrawEnemy()`、`WormEnemy::DrawWormHead()`/`DrawWormBody()`の4箇所を書き換え、重複していた100行超のコードを解消。
- **副産物のバグ修正**: `WormEnemy::DrawWormBody()`は元々`tex_noise`（ディゾルブ用ノイズ）のセット/解除が漏れており、頭だけディゾルブ効果が付いて胴体には付いていなかった。今回の共通化で胴体側にも`tex_noise`を追加し、実装漏れを修正（ユーザーに確認の上、意図的な仕様差ではなく実装漏れと判明）。
- **学習方針**: `std::pair`/`std::vector<std::pair<int,int>>`/`push_back`/範囲for文（`for (const std::pair<int,int>& tex : textures)`）が初見だったため、コードを提示せず一つずつ「なぜその型が必要か」「pairとmapの違い」などを確認しながらユーザー自身に書かせる形で進めた。構造化束縛(`auto& [a,b]`)は分かりにくいとの申告があり、`tex.first`/`tex.second`の明示的な書き方を採用。
- ビルド＆動作確認OK（岩・浮遊敵・ワームの頭/胴体、全て正常描画）。

**残タスク:**
- DataSetter群（2番）の共通化は未着手。テンプレート基底クラスかフリー関数での共通化を検討。
- シェーダー間の計算重複（3番）は`.hlsli`共通ヘッダ化を検討（`near_clip`/`far_clip`の重複ともまとめて対応するとよい）。
- FloatingEnemyのState系（HideState/ActiveState/LeaveState）はまだ実物未確認、共通化の余地があるか要調査。

### 進捗（2026-07-20続き・UIに電子風スキャンラインシェーダーを追加開始、`GlitchPS.hlsl`本体は完成）

**背景**: 参考動画（スターフォックス2風UI）を見て、UI画像に電子的な質感を足したいという要望。最初「グリッチ（横ズレ）」で提案したが、ユーザーの実際のイメージをすり合わせた結果、「古い携帯の画面のような、常時薄く入る横線（スキャンライン）」であり、時々ノイズが走るような演出は不要と判明。ノイズテクスチャ(`Noise.png`)は雲状で滑らかすぎるため今回は不使用と判断し、`sin`計算のみで実現する方針に決定。

**シェーダー本体(`GlitchPS.hlsl`)は完成、DxLib側の配線は次回に持ち越し:**
- 新規ファイル`GlitchPS.hlsl`を作成。`CapturePS.hlsl`を参考に、UI(2D画像)向けのシンプルな構成（`Texture2D`1枚、`SamplerState`、`PS_Input`は`pos`/`uv`のみ、3D情報は不要）にした。
- `scanline = sin(input.uv.y * scanline_frequency) * 0.5f + 0.5f` で横縞パターンを0〜1に正規化（`WaterVS.hlsl`の波の周波数、ノイズの`*0.5+0.5`正規化の知識をそのまま応用）。
- `lerp(1.0f, scanline, scanline_strength)`で「変化なし(1.0)」を基準に、`scanline_strength`の割合だけ暗い方に引っ張る「明るさの倍率」を計算(`brightnessScanline`、0.7〜1.0の範囲になる設計)。この`lerp`の使い方（色を直接ブレンドするのではなく、明るさの倍率をブレンドしてから元の色に掛ける）の理解に苦戦し、複数回説明し直した。
- `baseCol.rgb * brightnessScanline`で最終色を計算し、`baseCol.a`（元画像の透明度）をそのまま維持して`return`（一度`return float4(finalCol, 1.0f)`とアルファを固定してしまい、UI画像の透明部分が消える不具合になるところをレビューで指摘・修正）。
- 完成版:
```hlsl
Texture2D<float4> uiTex : register(t0);
SamplerState smp : register(s0);

struct PS_Input
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

static const float scanline_frequency = 300.0f;//スキャンラインを入れる間隔
static const float scanline_strength = 0.3f;//最大でどれぐらい明るさが落ちるか

float4 main(PS_Input input) : SV_TARGET
{
    float4 baseCol = uiTex.Sample(smp,input.uv);

    float scanline = sin(input.uv.y * scanline_frequency);//-1~1
    scanline = scanline * 0.5f + 0.5f;//0~1に正規化

    float brightnessScanline = lerp(1.0f,scanline,scanline_strength);//0.7～1.0になる
    float3 finalCol = baseCol * brightnessScanline;
    return float4(finalCol,baseCol.a);
}
```
- **仕様確定事項**: スキャンラインは完全に静止（時間経過で動かさない）。`time`変数は不要。

**残タスク（次回・学校で継続予定）:**
- DxLib側の配線が未着手: ①`.pso`へのシェーダーコンパイル設定を追加、②`LoadPixelShader(L"GlitchPS.pso")`でロードする場所を決める（既存の`LightingManager`のような専用管理クラスを新設するか検討）、③UI描画（`UIBase`派生クラスの`Draw()`）の直前後で`SetUsePixelShader(glitchPSH)` / `SetUsePixelShader(-1)`を呼ぶ配線、④どのUI（全部か、特定の画面のみか）に適用するかの設計。
- 現状`GlitchPS.hlsl`は既存の`UIManager`/`UIBase`/`ReticleUI`のどの描画呼び出しにも接続されていない、シェーダーファイル単体の状態。

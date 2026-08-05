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

### 進捗（2026-07-21・HPゲージUI新規実装＋GlitchPS配線完了、DrawGraphToShaderの共通化に着手中）

**HPゲージUIが完成した。** `Data/Image/HPGauge/`に用意された`HP_Frame_fix.png`/`HP_Gauge_fix.png`（Frame=枠、Gauge=緑帯、どちらも1280×140、透明部分あり）を使い、`Game/UI/HPGaugeUI.h/.cpp`（新規、`UIBase`継承）で実装。`Charactor`に`GetMaxHealth()`を追加（`Charactor.cpp`の匿名namespaceの`max_health`を返すだけ）。

**このセッションで踏襲した学習方針**: 座標計算（`DrawRectRotaGraph`の中心座標指定、左端/幅/中心の関係）を、具体的な数字の実験→式の一般化という順で段階的に導く形でユーザー自身に組み立てさせた。「めんどくさい」「わからない」という反応が出たときは、実際の計算や画像編集ソフトでの確認など負担の大きい手順を提案する前に、そもそも作業自体が不要になる設計変更（画像トリミング、UV座標方式への転換）を優先して提案するとうまくいった。

**ハマった点1: HP変化時にゲージ左端が1〜2pxガタつく**
- 原因調査で`int`への切り捨て（`gaugeWidth`、`DrawRectRotaGraph`のint版x,y引数）を複数箇所直したが、最終的にHP100/96での`gaugePosX`を実際に数値で比較すると理論値上は左端が動いていないことが判明。1〜2px程度はDxLibの拡大縮小描画（縮小サンプリングの丸め）に起因する残差と判断し、ユーザーの合意で許容範囲として受け入れて終了。

**ハマった点2(本命・大きな学び): `DrawRotaGraph`/`DrawRectRotaGraphF`にSetUsePixelShaderが一切効かない**
- HPGaugeUIにGlitchPS(スキャンライン)を適用しようとしたが、`SetUsePixelShader(m_glitchPSH)`を呼んでも見た目が一切変化しない（真っ赤に強制してみても赤くならない）ことが発覚。
- Web調査の結果、**DxLibの固定機能2D描画関数(`DrawRotaGraph`系)は独自ピクセルシェーダーを反映できない仕様**と判明（[Qiita記事](https://qiita.com/YYSS_101/items/b72234e09ba8a6b43e07)で確認）。3D側で`MV1DrawModel`(固定機能)と`LightingManager`のシェーダー付き描画が別物だったのと同じ構造。
- シェーダーを効かせるには`DrawPolygonIndexed2DToShader`（`VERTEX2DSHADER`で頂点を自前で組み立てて描画する関数）に作り直す必要があると判明し、方針転換。

**ハマった点3: UV可視化デバッグで黄色一色になる(構造体セマンティクス不一致)**
- `DrawPolygonIndexed2DToShader`で描画し直したところ、`GlitchPS.hlsl`の`main`を一時的に`return float4(input.uv,0,1);`にしてUV可視化しても黄色一色（変化なし）になる不具合が発生。
- 原因: `PS_Input`構造体が`SV_Position`の次に`TEXCOORD0`だけを書いていたが、DxLibが固定で渡す頂点データの並び順は`SV_POSITION→COLOR0(dif)→COLOR1(spc)→TEXCOORD0(uv)→TEXCOORD1(suv)`（[DxLib公式掲示板](https://dxlib.xsrv.jp/cgi/patiobbs/patio.cgi?mode=view&no=5490)で確認）。`COLOR0`/`COLOR1`を省略したことで、`uv`が実際には`dif`(白=1,1,1,1)の位置にズレて読み込まれ、`(u,v)=(1,1)`固定になっていた。
- 対処: `PS_Input`に`float4 dif : COLOR0;`と`float4 spc : COLOR1;`を(使わなくても)追加してセマンティクス順を合わせて解決。**教訓**: DxLibの`***2DToShader`系関数を使うときは、たとえ使わない値でも`PS_Input`構造体はDxLib側の頂点出力順と完全に一致させる必要がある。

**ハマった点4: 透過部分が塗りつぶされる**
- 構造体修正後は正しく表示されたが、Frame画像の透明な余白部分まで不透明なグレーで塗りつぶされて表示された。
- 原因: `DrawRotaGraph`系は透過フラグ引数(`true`)を持っていたが、自作の`DrawGraphToShader`にはブレンドモード設定がなかった。`SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255)`を描画前に、`SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0)`を描画後に追加して解決。

**ハマった点5: 拡大率(0.3倍)を掛けたサイズがintで丸められる**
- `Size`構造体(`m_width`,`m_height`が`int`)に拡大率を掛けようとして、比較的単純な問題だが「情報の欠落」を避けるため、`float`版の`Utility/SizeF.h`(新規)を作成して解決。
- 副次的なハマりどころ: `HPGaugeUI.h`で`class SizeF;`と前方宣言してしまい、`SizeF`が`struct`定義なのに`class`前方宣言と食い違ったことで「不完全な型」エラーが発生。`struct SizeF;`に修正して解決。**教訓**: 前方宣言は対象が`class`/`struct`どちらで定義されているか一致させる必要がある(一部コンパイラでは警告のみだが、DxLib::Sizeの構成では実害のあるエラーになった)。

**設計上の議論・決定事項**:
- `ShaderRegister.h`にUI用のテクスチャスロット定数(`ui_tex_diffuse = 0`)を新規追加（Actor用の`tex_diffuse`とは意味が異なるグループとして別セクション化）。
- 学校の先生の資料（Singletonは1つのみ推奨、増やすと悪いロールモデル変数になるという方針）をユーザーが提示。これを受けて、GlitchPS用のシェーダー管理を`GlitchManager`のような新規シングルトンにする案を撤回し、**`DrawGraphToShader`を`Utility/GraphShaderDraw.h/.cpp`としてクラス化しないフリー関数に切り出す**方針に転換。GlitchPS.psoのロード自体は、既存の「各機能のオーナー(LightingManager/WaterManager/Stage/WaterRevealManagerと同様、今回はHPGaugeUI・TitleSceneそれぞれ)が自分でLoadPixelShaderして自分のメンバーとして持つ」パターンを踏襲する。

**現在の状態（中断時点）**:
- `Utility/GraphShaderDraw.h`/`.cpp`をVisual Studioで新規作成済みだが、**まだ中身は空のクラステンプレートのまま**（`class GraphShaderDraw { public: GraphShaderDraw(); ~GraphShaderDraw(); };`のみ）。今回の方針(フリー関数化)に合わせて、クラスをやめて`void DrawGraphToShader(float left, float top, const SizeF& size, float uvMaxU, int texH);`という素の関数宣言に書き換える必要がある。
- `HPGaugeUI.cpp`内にはまだ動作確認済みの`DrawGraphToShader`実装がそのままメンバー関数として残っている（105〜162行目）。この中身を`GraphShaderDraw.cpp`に移植し、`HPGaugeUI`側は`#include "Utility/GraphShaderDraw.h"`して呼び出すだけの形に書き換える必要がある。
- `TitleScene.cpp`はまだ未着手。`DrawRotaGraph`/`DrawRectRotaGraph`(固定機能)のまま。GlitchPS適用対象はユーザーとの合意で「選択肢背景(SelectBackGround)とボタン画像(GameStart/GameEnd/OnCursor版含む)」、ロゴ(TitleLogo)は対象外。

**次回やること（旧・完了済み、下記の続き参照）:**
1. ~~`Utility/GraphShaderDraw.h`をクラスからフリー関数に書き換える~~ 完了
2. ~~`HPGaugeUI.cpp`の`DrawGraphToShader`の中身を`GraphShaderDraw.cpp`に移植~~ 完了
3. ~~`HPGaugeUI`をビルド・動作確認~~ 完了
4. ~~`TitleScene`に`m_glitchPSH`メンバーを追加~~ 完了
5. ~~`TitleScene::Draw()`のSelectBackGround/GameStart/GameEndをDrawGraphToShader化~~ 完了
6. ロゴ(TitleLogo)は対象外、`DrawRotaGraph`のまま据え置き（変更なし）

### 進捗（2026-07-21続き・GraphShaderDrawのフリー関数化完了、TitleSceneへのGlitchPS適用完了）

**`GraphShaderDraw`のフリー関数化とHPGaugeUIへの適用（完了）:**
- `Utility/GraphShaderDraw.h`/`.cpp`を空のクラステンプレートから、`void DrawGraphToShader(float left, float top, const SizeF& size, float uvMaxU, int texH)`というフリー関数に書き換え。`SizeF`は`struct SizeF;`で前方宣言（`class`と`struct`の不一致エラーを過去に踏んでいたための注意点）。
- `HPGaugeUI.cpp`にあった実装本体（頂点データ組み立て・UV計算・`DrawPolygonIndexed2DToShader`呼び出し）を`GraphShaderDraw.cpp`に移植。`HPGaugeUI.h`からメンバー関数宣言と使わなくなった`struct SizeF;`前方宣言を削除、`HPGaugeUI.cpp`は`#include "Utility/GraphShaderDraw.h"`するだけで既存の呼び出しがそのままフリー関数に向くようになった。
- ビルド・動作確認OK（HPゲージの見た目・スキャンライン共に変化なし）。

**`TitleScene`への適用（完了、途中でハマった点あり）:**
- `TitleScene.h`に`int m_glitchPSH = -1;`を追加、`Init()`で`LoadPixelShader(L"GlitchPS.pso")`。
- 「選択肢の背景(SelectBackGround)」から着手。中心座標(`ratio_x`/`ratio_y`)→左上座標への変換式(`centerX - width/2`)を、既存コードの`leftX`計算(173行目)から類推させる形で導出。
- **ハマった点(本命): `SetUsePixelShader`を呼ばずに`DrawGraphToShader`を使うと画像が全く見えなくなる**。座標・サイズをデバッグ表示で検証し理論値は正しいことを確認した後、原因を切り分け。`DrawPolygonIndexed2DToShader`はシェーダー前提の描画関数であり、ピクセルシェーダーが未セット(-1のまま)だと正しく描画できないと判明。`TitleScene`はこの時点でまだ`m_glitchPSH`をロードしていなかったため、シェーダーなしで`DrawPolygonIndexed2DToShader`を呼んでいたのが原因。`m_glitchPSH`のロードと`SetUsePixelShader`呼び出しを追加して解決。
- **リファクタ: 面倒な定型処理をまとめる`DrawGraphToShaderByCenter`を新設**。「`GetGraphSize`→`SizeF`計算→中心から左上への変換→`DrawGraphToShader`呼び出し」という4手順を毎回書くのが面倒という指摘を受け、`GraphShaderDraw.h/.cpp`に`void DrawGraphToShaderByCenter(float centerX, float centerY, double scale, int texH, float uvMaxU = 1.0f)`を追加（`DrawRotaGraph`感覚で使えるシェーダー版ヘルパー）。実装時、一度`uvMaxU`引数を受け取ったのに使わず`1.0f`固定で渡してしまうミスがあったが、レビューで指摘し修正。
- ワイプ演出（カーソルが乗った選択肢を`uvMaxU`で左から徐々に表示）も含めて`GameStart`/`GameEnd`両方の描画を`DrawGraphToShaderByCenter`に統一。
- **`switch`文の`case TitleSelect::StartGame`に`break;`が抜けており、フォールスルーで`ExitGame`のケースまで実行されてしまうバグを作り込んだが、レビューで指摘し即座に修正**（元のコードには`break;`があったのに、書き換え時に消えてしまっていた）。
- `SetUsePixelShader(m_glitchPSH)`/`SetUsePixelShader(-1)`の範囲を、背景だけでなく選択肢ボタンの描画も含む`switch`文全体を囲む位置に配置し直し、背景・ボタン両方にスキャンラインがかかるようにした。
- ビルド・動作確認OK（背景・ボタン両方にスキャンライン適用、選択肢切り替え、ワイプ演出、全て正常動作）。

**現状のまとめ**: UIへの電子風スキャンラインシェーダー(`GlitchPS.hlsl`)は、`HPGaugeUI`と`TitleScene`の両方で完全に配線・動作確認済み。共通処理は`Utility/GraphShaderDraw.h/.cpp`（フリー関数、シングルトン不使用）に一本化されている。

**残タスク:**
- 他にもUI要素があれば同様に`DrawGraphToShaderByCenter`+GlitchPS適用を検討可能（現状はHPGaugeUIとTitleSceneのみ対応）。
- `DrawGraphToShaderByCenter`は中心座標基準で左上を計算するため、`uvMaxU`で幅が変わると中心位置がズレる可能性がある設計上の注意点が残っている（今回のワイプでは大きな違和感はなかったが、将来「中心を固定してほしい」ケースが出たら再検討）。

### 進捗（2026-07-22・進め方の方針転換＋コスト表の把握と更新）

**重要な方針転換: ユーザーから「Claudeに頼りすぎて自分の実装力がついていない気がする」という自己申告があった。**
- これまでのUIシェーダー配線セッションで、Claudeが「これを書いてみて」と細かく手順分解し、詰まったらすぐヒントを出す進め方が続いていたことが原因と分析。CSV化やカメラ視錐台クランプなど自力で実装できていた時期と比べて、詰まる→自分で仮説を立てる→試す、という力がつく過程をClaudeが肩代わりしてしまっていた。
- **今後の方針（記憶にも保存済み）**: 「わからない」が出た瞬間に次のヒントを出さず、まず「どこまで自分で考えたか」「何を試したか」を聞き返してから最小限の一歩だけ渡す。新しい実装は設計をユーザーに先に考えさせ、Claudeは質問のみ。バグ調査も原因をすぐ言わず、怪しいと思う理由をユーザーに挙げさせる。シェーダーの数式など前提知識が必要な部分は教えてよいが、そこでも「なぜ+ではなく×を使うか」等の判断はユーザーにやらせる。
- 合わせて、シェーダーの演算子選択（足し算=合成、掛け算=減衰/マスク、lerp=按分）の直感がまだ掴めていないとの申告もあり、今後は式が出るたびに「これは合成？減衰？按分？」から自分で分類させる進め方に変更する。
- **運用変更**: 「今日はここまでにしますか」のような区切りの確認は不要、実装が一段落したら聞かずに自動でNOTES.mdに追記する運用に変更（既存の[[feedback_notes_md_sync]]を更新）。

**コスト表（Excel）の読み込みに対応:**
- ユーザーが進捗管理に使っているコスト表(Teamsチャット上のExcelをクラウド代わりに使用)を、リポジトリ直下(`NovaWing_詳細.xlsx`、NOTES.mdと同階層)に置いてもらう運用に変更。
- Claudeは`.xlsx`を直接読めないため、PowerShellのExcel COMオブジェクト経由でセル内容を取得する方法で対応（Excel側で開いたままだとファイルロックで読めない点に注意、閉じてもらってから再実行が必要だった）。
- シート構成: 「クラス図」「コスト表」「カレンダー」の3シート。「コスト表」は概要/見積コスト/実コスト/優先度/バージョン/ステータス/備考の列構成、右側に別ブロックでバージョン別集計(プロト/アルファ/ベータ/マスター)とカレンダー的な進捗率(開始日2026/6/11、終了日2026/9/3、全体84日)がある。
- 現状把握（2026-07-22時点）: 全体コスト147、消化76.5→今回の更新で81.5。プロトはほぼ完了（横回転のみ残）、アルファは当たり判定の残り・ボス関連・ステージ本配置が未着手、ベータはシーン/BGM/SE/UIが丸ごと未着手。
- **今回の更新内容**: 「シェーダ」カテゴリの「電子風シェーダ」（コスト表の77行目）が、今回完成させたGlitchPS.hlsl(スキャンラインシェーダー)に該当すると判断。実コスト`5`（見積と同額）、ステータス`未完了`→`完了`にCOM経由で書き換え、保存済み。
- **訂正: リポジトリ側のファイルとTeams側は別ファイルで、更新はTeamsに自動反映されないと判明**（当初「OneDrive同期で反映されるはず」と誤って記載していたが誤り。リポジトリ直下に置いたのはTeams添付ファイルのコピーであり、同期関係は無い）。
- **運用方針を確定**: 今後はTeamsを使わず、**このリポジトリの`NovaWing_詳細.xlsx`を唯一の本体**として扱う。Git管理下に置き(`.gitignore`に除外ルールなし、追跡対象にできることを確認済み)、学校・家で`git pull`すれば同じ最新版を共有できる、NOTES.mdと同じ運用にする。コミットはユーザー自身が行う（Claudeは指示なしに`git add`/`commit`しない）。

**残タスク:**
- コスト表の他の項目（当たり判定、ボス、シーン、BGM/SE、UIなど）はまだ現状を突き合わせていない。今後実装が進むたびに、該当する行を探して更新する運用にする。
- Teams側の古いファイルは今後参照しない（放置でよいとの合意、削除はしていない）。

### 進捗（2026-07-22続き・岩の当たり判定の方針決定＋「尖った波」実装は試行錯誤の末ロールバック）

**岩の当たり判定の方式を相談、球判定に決定:**
- 友人から「ポリゴンとの当たり判定(DxLibの`MV1CollCheck_*`系関数)でもいいのでは」という提案があり、球判定とどちらが良いか相談された。
- 岩は現在6〜7個、今後増える可能性がある。形状は普通サイズ2種+縦長1種という情報を踏まえ、**球判定を推奨**という結論に至った。理由: ①個数が増える前提がある中でポリゴン判定は1個あたりのコストが重く不利、②縦長の岩は`WormEnemy`の胴体と同じく複数の球を並べれば十分近似できる、③既存の`Sphere`/`CollisionManager`の設計と一貫性を保てる。
- **未着手**: 実装はまだこれから。「1個の岩に球1個か、縦長のものだけ複数球にするか」の設計をユーザー自身が考える段階で、今回のセッションでは着手できず終了（後述の「尖った波」の話に脱線したため）。次回はこの設計から再開。

**「尖った波」表現の実装を試行錯誤したが、最終的に方針そのものが誤りと判明しロールバック（`WaterVS.hlsl`は作業前の状態にgit checkoutで復元済み）:**
- 発端: 「今の海の波が丸っこいので、とんがった波も出したい」という要望。
- 数学的な理解の流れ: sin波の山と谷が対称であることを確認 → 「符号を保ったまま累乗する」変形(`SharpenWave(value, power)`関数)を考案 → 2乗の実験(0.2→0.04, 0.5→0.25, 0.8→0.64、小さい値ほど強く削られる性質)は理解できた。
- **ハマった点1**: `sin(...) * wave_height`という「既に高さを掛けた後の大きな値(-18~18)」を直接2乗してしまい、`pow(30,2)=900`のような桁違いの値になって「大きな波がざわざわする」不具合になった。→ `sin`の生の値(-1~1)の段階でSharpenWaveを適用し、その後にwave_heightを掛ける順序に修正して解決。
- **ハマった点2**: `power=10.0f`のような強すぎる指数を使うと、ほとんどの値が0近くまで潰れてしまい(0.5の10乗は約0.001)、「その波の寄与がほぼ消えるだけ」になり見た目が変わらなかった。→ 2〜3程度の穏やかな指数にすべきと判明。
- **ハマった点3(本命の勘違い)**: 「値の大きさを累乗で変形する」というアプローチ自体が、「山の高さを上げる」効果ではなく「山の幅を狭める(頂上付近は保ちつつ裾野を急速に低くする)」効果しか持たないと判明。しかも5つの波を合成した最終的な`wave`は、他の4波の起伏に埋もれて変化が視認しにくい。
- **デバッグ中に見つかった実装漏れ**: 「横の波だけ」に絞って検証していた際、if/elseの旧ロジックを削除する過程で`worldPos.y += wave;`という**代入行そのものを消してしまっており**、波が全く動いていない(常に平坦)状態でデバッグを続けていたことが発覚。`if(input.worldPos.y > 5.0f)`で赤/青に塗り分ける可視化で発見(真っ青一色→波が動いていない証拠)。値をそのまま色に出すと`-18~18`の範囲が0~1からはみ出して真っ黒/真っ白にクランプされる、という基礎的な可視化の失敗も経由した(`/18.0f`してから`*0.5f+0.5f`で0~1に正規化する必要があった)。
- **最終結論(ユーザー自身の気づき)**: 「そもそも常時流れているうねり波を一律に尖らせる」という前提が物理的におかしかった。現実の「波が尖る/砕ける」現象は、複数の波が特定の場所・タイミングで干渉する、または障害物(岩)に反射した波が元の波とぶつかる、という**局所的・条件付きの現象**であり、sin波を単純に足し合わせるだけの今の実装ではそもそも表現できない領域。全面的な尖り変形ではなく、岩付近など特定条件下でのみ局所的に高さを足す等の別アプローチが必要という結論になった。
- **対応**: `WaterVS.hlsl`を`git checkout`で作業前の状態(`SharpenWave`関数なし、`wave_height=10.0f`)に完全ロールバック。ビルド・見た目が元に戻ったことを確認済み。同じタイミングで別件(`WaterManager.cpp`のグリッド分割数140→300)の変更は意図的に保持。
- **教訓**: 実装がうまくいかない時、数式の間違いを疑うだけでなく、そもそも「狙っている物理現象の理解が正しいか」を疑う視点も重要。海シェーダはコスト表上プロトで完了扱いなので、「尖った波」は追加の作り込みとして保留・見送りとする。

**残タスク:**
- 岩の当たり判定（球判定、複数球にするかの設計）は次回着手。
- 「尖った波」は保留。再挑戦する場合は「岩など障害物周辺だけ局所的に高さを足す」のような条件付きアプローチを検討すること。

### 進捗（2026-07-22続き2・押し戻し方針の議論、Stageクラスの扱いを保留に）

**壁の押し戻し・ポリゴン当たり判定について議論、「壁自体を作らない」方針に確定:**
- 発端: 岩の当たり判定を球にする方針の後、「ステージに凹凸の壁を作って押し戻しをするなら、結局ポリゴンとの当たり判定が要るのでは」という懸念が出た。
- 現状把握: 今のプレイヤーの移動制限は`Player::ClampPosition()`による視錐台クランプ（`CameraBase::GetFrustumHalfSize()`を使った、見た目の壁とは無関係な数式ベースの制限）のみで、壁モデルとの物理的な当たり判定・押し戻しは元々何も実装されていないと判明。
- 議論の整理: 「岩」は押し戻しなし（当たったらダメージのみ）で確定。「壁」は当初「押し戻しあり」の想定だったが、本家スターフォックスが「障害物にぶつかると自機が透明になり突き抜ける」仕様であることを踏まえ、**壁自体を作らず「ずっと海」にする**方針に転換。壁が無ければ押し戻しの実装自体が不要になり、ポリゴン判定を検討する動機も消える。
- **結論**: 当たり判定が必要なのは「岩」のみで確定。球判定（複数球含む）だけで完結する設計になった。

**Stage（陸地の仮モデル）クラスの扱いは保留:**
- 「壁を作らない」流れで「もうステージ(陸地)自体不要では」という話になったが、「当たり判定を持つオブジェクトとしては不要だが、遠景の見た目としてなら使い道があるかもしれない」という留保が入り、判断を保留することにした。
- 参考: 現状の`Stage`クラスは元々当たり判定を一切持たず(`Update()`は空、`Draw()`のみ)、既に実質「見た目だけの遠景」として動いている。今すぐ削除しなくても実害はなく、後で必要になれば`ResourceLoader`のモデル登録・`StagePS.hlsl`はGit履歴から容易に復元できるため、保留のコストは低いと判断。

**残タスク:**
- 岩の当たり判定（球判定、複数球にするかの設計）を次回着手。この日の議論により、壁・Stageまわりの設計判断は一旦考えなくてよい状態になった。
- Stageクラスの要否（遠景として残すか、完全に削除するか）は保留のまま。判断が必要になったタイミングで再検討する。

**追記: Stageのモデル描画を一時的にコメントアウト、海の奥行きを拡張**
- 「保留」と決めたばかりだが、見た目の邪魔になるという理由で、`Stage::Draw()`内の`MV1DrawModel(m_modelHandle)`の1行だけコメントアウト（行列更新・シェーダ設定は残したまま、モデル描画だけ止める形）。クラス自体やモデル登録は削除せず、いつでも1行戻せば復元できる状態。
- `WaterManager.cpp`の`grid_size`（Z方向=`m_height`）をユーザー自身が調整し、海の奥行き（縦方向の広さ）を拡張済み。

### 進捗（2026-07-22続き3・岩の当たり判定、CSV設計まで完了・実装は途中）

**岩の当たり判定（球判定）の設計をCSVレベルまで完了。コード実装（RockDataSetter/Rock/CollisionManager）は次回持ち越し。**

**CSV設計の議論の流れ:**
- 岩は「モデルによって球の個数・半径・位置が違う」ことを踏まえ、既存のCSV化パターン（岩の配置と同じ思想、「パラメータをいじりたいときに楽だから」という理由）を踏襲してCSVで管理する方針にユーザー自身が到達。
- 「球の数が岩ごとに違う」問題は、CSVの列数が行によって変わることの懸念から始まり、「最大3個までの固定枠を用意し、使わない分は空欄にする」という設計にユーザー自身が到達。
- 空欄セルが`CSVDataLoader`の要素数チェック(`values.size() < header.size()`)に引っかからないか一緒に検証: `std::getline(..., L',')`は空欄でも1つの要素(空文字列)として数えるため、列数は変わらず既存の仕組みがそのまま使えると確認できた。
- オフセットは「y方向のみ」で十分と判断（岩は上下方向にしか複数球を並べない想定のため、x/zオフセットは不要）。
- 最終的なCSV列構成: `modelID,modelX,modelY,modelZ,sphereRadius1,sphereOffsetY1,sphereRadius2,sphereOffsetY2,sphereRadius3,sphereOffsetY3`（列名に途中で意味のない`sphere1`等の見出し列を作ってしまい、レビューで指摘・削除。番号は`radius`/`offsetY`側に付ける形に修正）。

**岩3種の形状特性とCSVへの実際の値入力（ユーザー自身が実施・完了）:**
- Rock1: 普通サイズ、球1個のみ（半径64、オフセット0）。
- Rock2: 縦長・太さ均一、球3個を均等間隔で積む（半径32固定、オフセットY = 64, 0, -64）。
- Rock3: 縦長だが根元に向かって太くなる、球3個だが半径を根元ほど大きく（半径16→32→45、オフセットY = 64, 0, -64）。
- `Data/CSV/RockData.csv`に全12行(Rock1×4, Rock2×4, Rock3×4)分の値を入力済み。

**次回の実装ステップ（未着手、ここから再開）:**
1. `RockDataSetter.h`に、`WormEnemyData`と同様の`RockData`構造体を新設する設計から着手（メンバー: modelID, pos, 球の情報。球を`std::vector<Sphere>`のような可変長で持つか、固定3枠で持つか、ユーザーに検討させている途中で中断）。
2. `RockDataSetter.cpp`のCSV読み込み処理に、新しい列(sphereRadius1〜3, sphereOffsetY1〜3)のパースを追加。空欄セルの扱い（`std::stof`は空文字列を渡すと例外になるはずなので、空欄なら「その球は使わない」とスキップする分岐が必要になる点に注意）。
3. `Rock.h`/`Rock.cpp`に球（`std::vector<Sphere>`想定）を持たせ、コンストラクタで`RockData`から受け取る。`WormEnemy`の`GetSegmentSpheres()`のようなgetterを追加。
4. `CollisionManager`に岩の配列を登録する仕組み（`RegisterFloatingEnemy`/`RegisterWormEnemy`と同じパターンで`RegisterRock`のようなものを追加）を作り、`Update()`内でプレイヤーと岩の全球との当たり判定を追加（岩は押し戻しなし、当たったらダメージのみの想定で、`WormEnemy`とプレイヤーの接触ダメージ処理が一番近い参考実装）。
5. ビルド・動作確認。

**残タスク:**
- 上記の1〜5番、次回セッションで続きから着手。

### 進捗（2026-07-23・岩の当たり判定 実装完了）

**岩の当たり判定(球判定)が完成した。CSV設計(前回完了)に続き、`Rock`/`RockDataSetter`/`CollisionManager`の実装まで完了、動作確認済み。**

**`Rock`/`RockDataSetter`の実装レビューで見つけた設計問題と修正:**
- 当初、`RockData`の`sphereRadii`/`sphereYOffsets`を`std::vector<float>`で持ちつつ、`resize(sphere_num)`で常に3個確保し、Rock1(球1個のみ)でも残り2個を「半径0のダミー球」として作ってしまっていた。実害は起きにくいが「実際に使う球の数」という情報が構造体の外(if分岐)にしか存在しない隠れた前提になっており、設計上の問題として指摘。
- 修正: `RockDataSetter.cpp`側でRock1なら`resize(1)`、他は`resize(3)`と実際の個数だけ確保するように変更。`Rock.cpp`のコンストラクタも`for (int i = 0; i < sphere_num; i++)`という固定回数ループから`for (int i = 0; i < data.sphereRadii.size(); i++)`に変更し、`.size()`を使った可変長ループに修正（固定ループのままだと`std::vector`の範囲外アクセスで未定義動作になる箇所だった）。
- 副次的な重複解消: `Rock`のコンストラクタが`pos`引数と`RockData.pos`の両方を受け取っていて情報が重複していたのを、`RockData`にまとめる形に統一（`Rock(pCamera, data)`のみに変更）。

**`CollisionManager`の岩判定・ワーム判定に「連続ダメージ防止」を追加、デバッグで複数回ハマった:**
- 岩は`hit_rock_damage=30`と重いため、「一度当たったら離れるまで再ダメージしない」仕組みをユーザー自身が発案。`Player`に`m_isTakingDamage`/`IsTakingDamage()`/`OnLeaveDamaging()`を追加（ダメージ要因を問わない汎用フラグとして設計、今後敵弾なども同様に扱う想定）。
- ワーム側の接触ダメージ処理にも同じ仕組みを追加する過程で、複数のバグを作り込み→レビューで発見→修正、を繰り返した:
  1. `if(!pPlayer->IsTakingDamage()) return;`と書いてしまい、ダメージ中に接触すると`CollisionManager::Update()`関数全体を`return`で抜けてしまい、以降の処理(岩の判定含む)が丸ごとスキップされるバグ。`continue`への修正を経て、最終的にループ外側での一括ガードに変更。
  2. 一括ガード(`if(!pPlayer->IsTakingDamage()) { for(...){...} }`)にする過程で、胴体判定の`if(playerCol.HitCollision(segmentSphere))`という当たり判定の条件式そのものを消してしまい、無条件で1回だけダメージを与えて`break`する状態になっていたミスをレビューで発見・修正。
  3. **本命のバグ(「一瞬で死ぬ」)**: `isHitWorm`(当たったかどうかの記録用フラグ)を`if(!pPlayer->IsTakingDamage()){ ... isHitWorm = true; ... }`という**IsTakingDamage()のチェックの内側**に置いてしまっていたため、「既にダメージ中で判定処理自体をスキップした」状況と「本当に当たっていない」状況を区別できなくなり、`if(!isHit && !isHitWorm)`の判定で誤って`OnLeaveDamaging()`が呼ばれ続け、ダメージがリセット→再度TakeDamageの無限ループのような状態になり、岩・ワームどちらも「一瞬で死ぬ」不具合になった。
  - **解決の型（岩側のコードと同じパターンに統一）**: 「当たったかどうかの記録(`isHitWorm = true` / `isHit = true`)」は`IsTakingDamage()`のチェックより**外側**（当たっていれば無条件で実行）、「実際にダメージを与えるかどうか」だけを`IsTakingDamage()`のチェックの**内側**に置く、という設計に統一して解決。
- **教訓**: 複数の判定処理（岩・ワームなど）が同じ状態フラグ(`m_isTakingDamage`)を共有する設計にする場合、「当たったかどうかの記録」と「ダメージを実際に与えるかどうかの制御」を必ず分離すること。同じ`if`の中に両方を混ぜると、片方の判定処理が(ガードによって)スキップされた時に、もう片方の判定結果に基づいて誤ったリセットが起きる。

**現状**: 岩の当たり判定（球判定、Rock1=1球/Rock2・Rock3=3球）、CollisionManagerでのプレイヤー-岩ダメージ判定、ワーム接触ダメージの連続ヒット防止、すべて動作確認済みで完成。

**残タスク:**
- `CollisionManager::Update()`関数のネストが深くなった点は、後で関数分割などのリファクタリング候補として残っている（今回は動作の正しさを優先し先送りにした、ユーザーと合意済み）。
- 他のダメージ要因（敵弾など）にも`IsTakingDamage()`の連続ダメージ防止を広げるかどうかは未定（`m_isTakingDamage`はダメージ要因を問わない汎用フラグとして設計されている）。

### 進捗（2026-07-23続き・エディタ設定の統一（Visual Studio ⇔ VS Code））

**インデント・波括弧位置がVisual StudioとVS Codeで食い違い、切り貼りするたびに崩れる問題に対応。**
- Visual Studio側の実設定（`ツール>オプション>テキストエディター>C/C++>タブ`）を確認: タブサイズ4、インデントサイズ4、「タブの保持」（スペース変換なし）。既存コードはタブ4幅とスペース4幅が混在していたため、これがズレの一因と判明。
- `NovaWing/.editorconfig`に`indent_style = tab` / `tab_width = 4` / `indent_size = 4`を追加。
- `.vscode/settings.json`（新規）: `editor.insertSpaces: false`、`tabSize: 4`、`detectIndentation: false`に加え、`formatOnPaste`/`formatOnSave`/`formatOnType`を全て有効化（「貼り付けた瞬間から自動整形してほしい」という要望のため）。C++の既定フォーマッタを`ms-vscode.cpptools`（clang-format内蔵）に指定。
- `.vscode/extensions.json`（新規）: `editorconfig.editorconfig`と`ms-vscode.cpptools`を推奨拡張として登録。
- `.clang-format`（新規、リポジトリ直下）: 既存の`.editorconfig`のVisual C++書式設定（`cpp_new_line_before_open_brace_*`等）を参照し、プロジェクトの実コードが一貫して採用している「波括弧を次の行に改行するAllmanスタイル」を`BreakBeforeBraces: Allman`として指定。タブ4幅、`ColumnLimit: 0`（自動改行なし）などをVisual Studio側の挙動に合わせて設定。

**現状**: VS Code側で新しく書く・貼り付けるコードは、タブ4幅・Allmanスタイルの波括弧に自動整形されるようになった。既存ファイル（スペース4幅で書かれてしまったもの、例:`Rock.cpp`等）は自動では直っておらず、今後編集する際に自然と統一されていく想定。

### 進捗（2026-07-23続き2・チャージショットが撃てなくなるバグを修正）

**背景**: チャージショットに「ボタンを離してから1秒以内に再度押さないと通常弾になり、エフェクトが徐々に小さくなる」機能(本家スターフォックス準拠)を追加しようとした際、チャージショット自体が一切撃てなくなる不具合が発生。`ChargeShootState.cpp`のデバッグセッション。

**発生した実装ミスと修正の流れ（複数回の書き直しあり）:**
1. `if (input.IsReleased(...) && m_chargeFrame < charge_comp_frame) {...} else if (m_chargeFrame >= charge_comp_frame) {...}`という元のコードを書き換える過程で、一時`if(!pPlayer->IsTakingDamage()) return;`のような`return`を使ってしまい関数全体を抜けるミス（これは実際には別セッションのCollisionManagerのデバッグと混同していた可能性があるが、同種の「returnで関数全体を抜けてしまう」パターンのバグが本ファイルでも複数回発生）。
2. 「ボタンを離した瞬間、未完了なら通常弾」の分岐(`if (input.IsReleased(...)) { if (m_chargeFrame < charge_comp_frame) {...} }`)に、`ChangeState(NormalShootState)`への遷移が抜けており、通常弾を撃った後も`ChargeShootState`に留まり続けるバグがあった（このため一度チャージを中断して弾を撃つと、そのまま`ChargeShootState`に居座り、次にボタンを押すと`m_chargeFrame`が0からではなく中断前の値から再開する状態になっていた）。
3. **本命のバグ**: 「ボタンを離した瞬間に完了/未完了を判定する」ロジック(`if (input.IsReleased(...)) { if(未完了){通常弾} else{ChargeReadyStateへ} }`)を追加した後も、**それとは別に**「押しっぱなしのまま完了した場合」の`else if (m_chargeFrame >= charge_comp_frame) { ChangeState(ChargeReadyState); }`という古い分岐がユーザー自身の手で残されており、**ボタンをまだ離していない最中に`ChargeReadyState`へ遷移してしまう**バグになっていた。
   - ユーザーの仕様定義: `ChargeReadyState`は「ボタンが離されていて、再度押されるのを待っている状態」であり、押しっぱなしの最中に入るべきステートではない。この定義に基づき、押しっぱなし中に完了判定する`else if`ブロックを削除して解決。
- **最終形**: `ChargeShootState::Update()`は「ボタンを押している間はチャージ時間を計測するだけ」「ボタンを離した瞬間にのみ、完了/未完了を判定してNormalShootState/ChargeReadyStateへ分岐する」というシンプルな形に収束。動作確認済み（1秒以上長押し→離す→ChargeReadyStateへ正しく遷移→1秒以内の再押下でチャージショット発射、を確認）。

**このセッションでの進め方の振り返り**: デバッグ中、Claude側の説明・質問が長く冗長になり、ユーザーから「質問は簡潔に」「事象は箇条書きで」「自分で確認できることは聞かずに自分で確認して」という具体的なフィードバックがあった。以降このセッション内では、事象整理は箇条書き・質問は最小限・ファイル確認は自分で行う、という形に切り替えて対応した。

**残タスク（旧・下記で全て解決済み）:**
- ~~「1秒以内に再度押さないと通常弾になり、エフェクトが小さくなっていく」という`ChargeReadyState`側の実装~~ → 下記で発見・修正完了。

### 進捗（2026-07-24・チャージショット関連バグの完全解決、UI表示タイミングの調整）

**背景**: 前回セッションで`ChargeShootState`側は解決したが、実際に動かすとまだチャージショットが撃てなかった。原因は`ChargeReadyState.cpp`側にも複数のバグが残っていたため。芋づる式に発見・修正し、最終的に一連の不具合を全て解決した。

**発見・修正したバグ（発生順）:**
1. **`ChargeReadyState.cpp`: `IsTriggered`のチェックが論理的に発火しない場所にあった**。`if (!input.IsPressed(shoot)) { m_notPressdFrame++; if (input.IsTriggered(shoot)) {...} }`という構造になっており、「ボタンが押されていない」条件の内側で「今まさに押された瞬間」をチェックしていて矛盾していた（`IsTriggered`が`true`になる瞬間は必ず`IsPressed`も`true`のため、この`if`ブロックには絶対に入れない）。`if (m_notPressdFrame < can_shoot_frame) { if (IsTriggered) {...} }`という形に外側のガードを分離して解決。
2. **`std::clamp`の引数ミス（`ChargeReadyState.cpp`・`ChargeShootState.cpp`の両方）**: `std::clamp(m_effectScale.m_x, 0.0f, m_effectScale.m_x)`のように第3引数(max)にクランプ対象の変数自身を渡してしまっていた。`m_effectScale.m_x`が毎フレーム`--`されマイナスになると`min(0.0) > max(負の値)`という不正な範囲になり、`Debug Assertion Failed! invalid bounds arguments passed to std::clamp`でクラッシュ。第3引数を`1.0f`（`first_effect_scale`の最大値）に修正して解決。
3. **`Player::IsChargeReady()`の判定漏れ**: ターゲットUI(チャージレティクル)の表示条件`IsChargeReady()`が「`ChargeReadyState`かどうか」しか見ておらず、チャージ中(`ChargeShootState`)にはUIが出ない状態だった。ユーザーの仕様「チャージ中〜ChargeReadyState終了まで、ずっとUIを出したい」を踏まえ、`ChargeShootState`か`ChargeReadyState`のどちらかであれば`true`を返すよう修正（実装時、2つ目の`dynamic_pointer_cast`が誤って`ChargeShootState`のままコピペされていたミスも発見・修正）。
4. **`NormalShootState.cpp`: 通常ショットを撃っただけでもUIが出てしまう新たな副作用**: 3番の修正でUIが`ChargeShootState`もカバーするようになった結果、`NormalShootState`の`else if(input.IsPressed(shoot))`（「押された瞬間ではないが、まだ押されている」だけで即`ChargeShootState`に遷移する設計）が原因で、単発クリックでもボタンが数フレーム押され続けているだけで一瞬`ChargeShootState`に入ってしまい、UIが見えてしまっていた。「一定フレーム(10F)以上の長押しを検知してから`ChargeShootState`に遷移する」`m_pressingShootButton`カウンタを新設して解決（実装時、「ボタンが離されたらリセット」のつもりで`if (input.IsPressed(shoot)) { m_pressingShootButton = 0; }`と書いてしまい、押している間ずっとリセットされ続けて長押し判定に絶対に到達できないミスがあったが、`IsReleased`への修正で解決）。
5. **UIアニメーションと実際のチャージ時間のズレ**: チャージレティクルの拡大→縮小＋回転アニメーション(`anim_speed`)が約12.5フレームで完了するのに対し、実際のチャージ完了(`charge_comp_frame`)は60フレームかかるため、「アニメーションは完了しているのに、まだチャージ中」という期間が長く続き違和感があった。3番の修正でUI表示開始が早まった(チャージ中から見えるようになった)ことで、このズレが目立つようになったと判明。`anim_speed`を`1.0f / 40.0f`に変更し、アニメーション完了とチャージ完了のタイミングを近づけて解決（60フレーム丁度ではなく、ユーザーが実際に動かして調整した結果40フレームに設定）。

**このセッションでの進め方**: 前回に続き、事象整理は箇条書き・質問は1問ずつ・ファイル確認は聞かずに自分で行う、という進め方を徹底した。ユーザーから「質問は1つの会話で1つにしてほしい」という追加フィードバックがあり、以降1メッセージにつき質問は1つのみに絞る形に統一した。

**現状**: チャージショット関連の一連の機能（長押しでチャージ→離す→1秒以内の再入力でチャージショット、時間切れなら通常状態に戻る、UI表示タイミング、エフェクトの拡大・縮小演出）は全て動作確認済みで完成。

**残タスク:** 特になし。

### 進捗（2026-07-24・カメラのLerp追従化 完成／2026-07-25・BulletManagerの重さ調査、修正は途中で中断）

**カメラの移動をLerpによる遅延追従に変更（完成）:**
- `Player`が動くと`CameraBase`が完全追従(毎フレーム位置を直接代入)していたのを、`m_targetPos`(注視点)と同じ「前フレームの値を保存→Lerpで補間」パターンに統一。
- `CameraBase.h`に`Position3 m_prevPos;`を追加、`CameraBase::Update()`で`m_prevPos = m_pos`(揺れ加算後の値を保存)→目標位置を計算→`m_pos = Vector3::Lerp(m_prevPos, m_pos, lerp_t)`という順序で実装。ユーザー自身が一度で正しく実装できた。動作確認済み、完成。

**ゲームが重くなる問題を調査、原因は特定済み・修正は未完了で中断:**
- 症状: プレイヤーが弾を連打し、同時に敵も弾を撃っている時にフレームレートが低下する。
- **原因判明**: `BulletManager`の弾配列(`m_pPlayerBullets`/`m_pEnemyBullets`/`m_pChargeBullets`、いずれも`std::weak_ptr`)が`push_back`で増え続けるだけで、**死んだ弾を配列から取り除く処理が一切存在しなかった**。弾の実体自体は`GameObjectManager::RemoveGameObject()`(`std::remove_if`+`erase`、`IsDead()`判定)で正しく解放されているが、`BulletManager`側の`weak_ptr`は死後も配列に残り続け、`CollisionManager::Update()`が毎フレームこの肥大化した配列を総当りでループし続けることが重さの原因と特定。
- **対応方針**: `GameObjectManager::RemoveGameObject()`と同じ`std::remove_if`+`erase`パターンを`BulletManager`にも実装する方針で合意。`BulletManager`に`Update()`関数自体が存在しなかったため新設が必要（`GameScene::Update()`から毎フレーム呼ぶ必要があるが、この配線はまだ未着手）。
- **設計変更**: 個別の3配列に加えて、共通基底クラス`BulletBase`の`std::weak_ptr<BulletBase>`を持つ`m_pAllBullets`という統合配列をユーザー自身が新設。`CreateBullet()`内の3つの`case`全てで、既存の個別配列への`push_back`に加えて`m_pAllBullets`にも`push_back`する形にした（3配列それぞれに削除処理を書く重複を避ける設計判断）。
- **`BulletManager::Update()`実装中に発生したミスと修正**:
  1. `std::remove_if(...).begin()/.end()`に渡す`erase`の第2引数に`m_pAllBullets.back()`(最後の要素への参照、イテレータではない)を渡してしまい型不一致。`.end()`に修正して解決。
  2. ラムダ式`[](const std::weak_ptr<BulletBase>& pBullet) { if(pBullet.lock() != nullptr) { return pBullet.lock()->IsDead(); } }`で、`if`が`false`(=`lock()`が`nullptr`、実体は既に破棄済み)の場合に`return`が無く、全経路でreturnしていないコンパイルエラーになっていた。「実体が破棄済みなら死んでいる扱いにして削除する」という意図で`return true;`を`if`ブロックの外に追加する対応を提示、ユーザーが眠気のため今回はコード適用前に中断。

**次回やること（旧、下記で全て完了・解決）:**
1. ~~`BulletManager::Update()`のラムダに`return true;`を追加~~ → 完了。
2. ~~`GameScene.cpp`に`BulletManager::Update()`の呼び出しを配線~~ → 完了(`GameScene::Update()`内、`m_pCollisionManager->Update()`の直後に追加)。
3. ~~ビルド・動作確認~~ → 実施したが、下記の通り真の原因は別にあった。

### 進捗（2026-07-25続き・重さ問題の真因はEffekseerエフェクト側と判明、解決）

**`BulletManager`の削除処理・配線は完了させたが、それでもゲームが重いままだった。ユーザーの観察により真因を特定、解決した。**

- `return true;`の追加、`GameScene::Update()`への`m_pBulletManager->Update()`配線、両方完了させてビルド・確認したが、弾を連打すると相変わらず重くなる症状は解消しなかった。
- **ユーザーの鋭い指摘**: 「もし配列にゴミが溜まり続けているのが原因なら、弾を全部消した後もしばらく重いままのはず。しかし実際は弾がたくさん存在する瞬間だけ重くなる」→ これは配列の肥大化(蓄積型の問題)ではなく、「今存在する弾の数に比例して単純に処理量が増えている」ことを示す観察で、`BulletManager`側の修正だけでは説明がつかないと見抜いた。
- ユーザーが「Effekseerエディタ上で`PlayerBullet.efkefc`を単体再生していても重かった」と気づき、ゲーム側のロジックではなくエフェクトアセット自体を疑う方向に方針転換。
- Effekseerエディタで各ノード(`Trail`/`Particle`/`Core`×3)を1つずつ表示・非表示にして切り分けた結果、`Particle`ノードが原因と特定。
- **真の原因**: `Particle`ノードの設定で「生成数: 無限」にチェックが入っている一方、「削除」セクションの「寿命により削除」のチェックが**外れていた**。「生存時間: 中心23」という見た目上のフェードアウト設定はあったが、これは表示用のパラメータであり、「寿命により削除」のチェックが無いとパーティクルの内部データそのものは削除されず、エフェクトが再生され続ける限りパーティクルが無限に蓄積し、負荷が増大し続けていた。
- **対処**: 「寿命により削除」にチェックを入れて保存 → 動作確認したところ即座に軽くなり、解決。

**教訓（今後の参考用）:**
- 「弾を撃つ量に比例して重くなるが、弾が無くなれば軽さが戻る」ような症状は、ゲーム側のロジック(コードの配列管理)よりも先に、**その瞬間再生されているエフェクトアセット自体の負荷**を疑う価値がある。
- Effekseerでは「生存時間」(見た目のフェード)と「寿命により削除」(内部データの破棄)は別の設定であり、両方セットで正しく設定されていないと、パーティクルが際限なく蓄積するバグになり得る。新しいエフェクトを作る際は「生成数」と「寿命により削除」の組み合わせを必ず確認すること。
- コード側の原因調査(`BulletManager`の削除漏れ)自体は無駄ではなく、実際に本来必要だった正当な修正(死んだ弾のweak_ptrを配列から除去する処理)であり、これはこれで完成・妥当な改善として残った。

**残タスク:** 特になし。ゲームの重さ問題は解決。

### 進捗（2026-07-25続き2・LightingPS/DamagePS共通化(途中)、弾連打時の重さ第2弾の調査・FPS表示機能追加）

**ダメージリアクション(機体が赤くなる演出)の実装に着手。共通ライティング計算の切り出しが完成、DamagePS本体の実装は次回。**
- ユーザーの要望: プレイヤーがダメージを受けた際、本家スターフォックスのように機体が強い赤(マゼンタ系)に染まるリアクションを追加したい。参考画像を確認し、質感(法線・陰影)は保ちつつ強めに色を混ぜる方向で合意。
- 実装方針の検討: 骨格のみ用意されていた`DamagePS.hlsl`に、`LightingPS.hlsl`と同じライティング計算をコピーする案が出たが、ユーザー自身が「コピーは効率が悪い、保守性が下がる」と気づき、共通化する方針に転換。
- **`LightingCommon.hlsli`を新規作成し、共通のライティング計算を切り出し完了**:
  - `LightingResult`構造体(`light`, `specular`の2メンバー)を新設。HLSLの関数は1つの値しか返せないため、複数の戻り値をまとめる目的（C++の`std::pair`に相当する発想をユーザー自身が導出）。
  - `CalcLighting(normMapCol, metCol, normalWS, tangentWS, lightVec, cameraPos, worldPos)`関数に、法線マップ変換〜specular計算までを移植。テクスチャのサンプリング自体とディゾルブ判定(`discard`)は各シェーダ固有の処理として関数の外に残す設計判断も、ユーザー自身が「サンプリングは各シェーダで違う可能性があるので引数で渡す方が柔軟」と正しく導出。
  - `ambient_light`/`normal_map_strength`/`smoothness_min`/`smoothness_range`の4定数も`LightingCommon.hlsli`に移動（`near`/`start_disolve`/`noise_uv_scale`はRock固有のディゾルブ用定数のため`LightingPS.hlsl`側に残置）。
  - `LightingPS.hlsl`を`#include "LightingCommon.hlsli"`し、`CalcLighting`呼び出しに置き換え。ビルド・動作確認済み(Rock/FloatingEnemy/WormEnemy等、既存の見た目に変化なし)。
  - 実装中の細かいミス(すぐ自己修正): `light`という変数名が`LightingResult.light`と衝突しそうと気づき`lightStrength`に改名、`result.specular`のタイプミス(`resspecular`)、`light`/`specular`の生変数参照忘れ、全てユーザー自身が発見・修正。
- **残タスク**: `DamagePS.hlsl`は骨格(テクスチャ1枚、`main`が白を返すだけ)のまま。`CalcLighting`を呼び出しつつ、最後に赤みを`lerp`等で混ぜる本体の実装がまだ。`Player::DrawPlayer()`側で、`LightingManager::ApplyShader()`の代わりに(または後に上書きする形で)`DamagePS`を使う配線もまだ未着手（`m_isTakingDamage`が既にPlayerにあるのでこれをトリガーに使う想定）。

**弾連打時の重さ問題、第2弾の調査（前回のParticle無限蓄積とは別原因、解決）:**
- 前回の`PlayerBullet.efkefc`修正後も、「敵が複数体+敵弾多数な状態でプレイヤーが弾を連打すると重い」症状が残っていた（プレイヤー弾単体では重くならない）。
- `CollisionManager::Update()`の当たり判定ループ(「敵の数×弾の数」の総当り)を疑い、ユーザー自身が中身を空にして検証 → それでも重い → 当たり判定ロジックは無罪と判明。
- **重要な発見: Debugビルドでは重いが、Releaseビルドでは全く重くない**。Debugは最適化なし+STLの境界チェック等で元々何倍も遅く、実配布版(Release)には実害がないと判明。ただしユーザーは「開発中の制作速度に直結する」との理由でDebug側の軽量化も追求する方針を選択。
- Effekseerエディタでの切り分け実験(`Trail`ノードの表示/非表示)で、`Trail`(軌跡)ノードがDebug時の重さの主犯と特定。パーティクル数(Player:28個, Enemy:40個)自体はEnemy側が多いにも関わらずPlayer側だけ重かった理由は、「プレイヤーは連射できるため同時に存在する発数がEnemyより多くなりやすい」ため(1発あたりの負荷×同時発射数、で考える必要があった)。
- スプラインの分割数(4→1)を下げる対策は効果が薄いと判明。最終的に**敵弾のTrailノードの「生存時間」を半分に短縮**したところ、40FPS→80FPSまで改善。見た目への影響も軽微で許容範囲と判断し、これで解決とした（プレイヤー弾側のTrail短縮は「80あれば十分」として見送り）。

**FPS表示機能を新規実装（完成）:**
- `Application::Run()`のメインループに、`_DEBUG`限定でFPS表示を追加。`GetNowHiPerformanceCount()`で計測した「1フレームにかかった時間(待機処理込み)」から`1000000.0f / elapsedTime`でFPSを算出。
- ハマった点1: 除算の演算子優先順位を誤り`1000000.0f / GetNowHiPerformanceCount() - startTime`と書いてしまい、意図と異なる計算になっていた。カッコで囲み`1000000.0f / (GetNowHiPerformanceCount() - startTime)`に修正して解決。
- ハマった点2: `DrawFormatString`を`ScreenFlip()`の**後**に書いてしまい、文字が画面に表示されなかった。DxLibの描画は裏画面に描くだけで`ScreenFlip()`で表画面に反映する仕組みのため、`ScreenFlip()`より前に描画命令を置く必要があると理解し、位置を修正して解決。
- この機能により、上記のTrail負荷検証を感覚ではなく数値で比較できるようになった。

**教訓（今後の参考用）:**
- 「Debugでは重いがReleaseでは軽い」場合、実配布に実害はないが、開発効率(頻繁なDebugビルドでの動作確認)に影響するなら軽量化する価値がある。判断はプロジェクトの開発フェーズ次第。
- Effekseerの`Trail`(軌跡)ノードは、パーティクル数以上に負荷が高くなりやすい機能。「1回の発射あたりの負荷」だけでなく「連射可能な弾かどうか(同時に何個存在しうるか)」を掛け合わせて負荷を見積もる必要がある。
- HLSLの共通コード切り出しは`.hlsli`ファイル+`#include`で行い、複数戻り値が必要な関数は`struct`にまとめて返す。

**残タスク:**
1. `DamagePS.hlsl`本体の実装（`CalcLighting`呼び出し+赤み合成）。
2. `Player::DrawPlayer()`から`DamagePS`を使う配線（`m_isTakingDamage`をトリガーに使用）。
3. プレイヤー弾側のTrail短縮は見送り中、必要になれば再検討。

### 進捗（2026-07-25続き3・ダメージ時のカメラ揺れが効かないバグを修正）

**背景**: ダメージを受けた時にカメラを揺らす処理(`CollisionManager`側で既に`sharedCamera->OnShake(...)`を呼ぶ実装は完了済み)を追加したが、実際には揺れている感じがしなかった。

**原因**: `CameraBase::Update()`内、`m_pos += UpdateShake();`（揺れの加算）が関数の**一番最初**に実行されていたが、その直後にプレイヤー位置から`m_pos.m_x`/`m_pos.m_y`/`m_pos.m_z`を**直接代入**する処理があり、揺れの加算分が毎フレーム即座に上書きされて消えていた。これは前回追加した「カメラのLerp追従」の計算順序とも関係しており、揺れは全ての位置計算(目標位置の算出→前フレームとのLerp補間)が終わった**最後**に加算する必要があった。

**修正**: `m_pos += UpdateShake();`を`Update()`の先頭から削除し、`m_pos = Vector3::Lerp(m_prevPos, m_pos, lerp_t);`（Lerp補間）の直後、`SetCameraPositionAndTarget_UpVecY(...)`を呼ぶ直前に移動。動作確認済み、ダメージ時にカメラが正しく揺れるようになった。

**教訓**: 複数の要素(基本位置の計算、Lerp補間、揺れなどのオフセット)が同じ変数(`m_pos`)に対して順番に処理される設計では、「最終的な位置に対する加算・オフセット」は必ず一連の計算の一番最後に置く必要がある。基本位置の計算処理を直接代入(`=`)で書いていると、途中の加算(`+=`)は上書きされて消えてしまう。

### 進捗（2026-07-25続き4・VS CodeのIntelliSenseエラー調査、未解決のまま保留）

**背景**: `GameScene.cpp`や`InputManager.cpp`で、`DxLib::VECTOR`/`DxLib::XINPUT_STATE`等、DxLib由来の型を使う行全てが「不完全な型」と表示される。実際にはVisual Studioでのビルドは正常に通っており(実害なし)、VS Code上でのIntelliSense表示だけの問題と判明。ユーザーによれば、以前Visual Studio 2026を使っていた時にも同様のエラーがVisual Studio自体で出ており、2022に切り替えたら直った経緯があるとのこと(コンパイラのバージョン相性が絡んでいる可能性を示唆)。

**調査したが原因特定に至らなかった項目一覧:**
1. `c_cpp_properties.json`の配置ミス — ワークスペースルート直下(`c:\Users\Admin\Documents\GitHub\NovaWing\.vscode\`)に同ファイルが存在せず、1階層下のサブフォルダ(`NovaWing\.vscode\`)にしかなかった。ワークスペースルート直下に正しいインクルードパス(`${workspaceFolder}/NovaWing/DxLib_h`等)で新規作成したが、**エラーは解消しなかった**（当初「解消した」と記録したが、後のユーザー確認で実際には直っていないと判明、この記録を訂正）。
2. `windowsSdkVersion`の不一致 — 設定値`10.0.19041.0`が実機に存在せず(`Test-Path`で確認)、実際にインストールされているのは`10.0.26100.0`のみと判明。正しい値に修正したが改善なし。
3. `defines`への`_WIN64`/`_WIN32`追加、`compilerArgs`の明示 — 改善なし。
4. `DxLib.h`内の`namespace DxLib { ... }`ブロック、`#ifndef DX_NON_NAMESPACE`等の条件分岐、`DxCompileConfig.h`/`DxDataTypeWin.h`の中身を読み込み、構文的な問題がないか確認 — 明確な異常は見つからず。`.vcxproj`の`PreprocessorDefinitions`にも`DX_NON_NAMESPACE`等の問題になりうるマクロは無いことを確認。
5. 拡張機能の競合(`clangd`等)を疑ったが、インストール済み拡張機能は「EditorConfig」「HLSL Tools」「Japanese Language Pack」「Rainbow CSV」「Shader languages support」＋`ms-vscode.cpptools`のみで、競合の心当たりなし。
6. Visual Studioの「MSBuildプロジェクトビルド出力の詳細」を「診断」にして実際の`cl.exe`呼び出しコマンドラインを取得しようとしたが、このプロジェクトのビルド出力形式では詳細なコンパイラ引数が表示されず、確認できなかった。

**確定した事実**: `GameScene.cpp`固有の問題ではなく、**DxLib由来の型を使う箇所全般**（`VECTOR`、`XINPUT_STATE`等）でIntelliSenseだけがエラーを出す。「不完全な型」というエラーメッセージ自体は、型の宣言はIntelliSenseに認識されているが定義(中身)が見えていない状態を示す。

**現状の判断**: ビルド自体は正常に通り実害が無いこと、Visual Studio側でも過去に同種の現象があったこと(コンパイラバージョン相性の可能性)を踏まえ、**これ以上のIntelliSense設定の深追いは費用対効果が低いと判断し、一旦保留**とした。今後気が向いたら`ms-vscode.cpptools`の再インストール、または`compile_commands.json`方式（実際のビルドコマンドをそのまま使う、CMake等がないと生成が難しい）を試す余地はある。

**残タスク:**
- IntelliSenseエラーは実害なしのまま放置中。再発・悪化した場合や、新しい手がかりが見つかった場合に再調査する。

### 進捗（2026-08-03・DamagePS.hlsl本体の実装完了、シェーダー設計の進め方を大きく方針転換）

**重要な方針転換: シェーダー設計は「ユーザーが先に考えて宣言し、Claudeはレビューのみ」に変更（記憶にも保存済み）。**
- `DamagePS.hlsl`(ダメージ時に機体が赤くなる演出)の実装再開にあたり、ユーザーから「シェーダーを作る際はPS_INPUTの中身・テクスチャ・計算方法など全て自分が先に考えて宣言する。Claudeはそれが正しいか精査するだけにしてほしい」という明確な要望があった。
- さらに「ヒントはほぼ答えなので、聞かれてもいないのに出さないでほしい」「AとBどちらだと思いますか、のような二択の提示も、選択肢を自分で生み出したいのでやめてほしい」という追加要望もあり、両方とも記憶に保存し、以降のセッションで徹底した。

**DamagePS.hlslの設計・実装（ユーザー主体で完成、Claudeはレビューのみ）:**
- ユーザー自身が必要な要素を順に洗い出した: `worldPos`(視線ベクトル計算用)、法線マップ+`normalWS`+`tangentWS`(法線計算のセット)、「赤みの強さ」を`cbuffer`で受け取る方針(時間経過のカーブ計算はC++側`Player.cpp`で行い、シェーダーには結果の数値だけ渡す設計）。
- 実装途中、`TEXCOORD`セマンティクスが「テクスチャ座標専用ではなく、頂点→ピクセル間でfloatデータを運ぶ汎用の箱」であるという理解を確認。
- 一度`sin(time)`をシェーダー側で計算する案や、視線ベクトルと法線の内積で「輪郭だけ赤くする」案を試したが、ユーザー自身が「最初に決めた方針(C++側で強さを計算する)とズレている」「輪郭ではなく機体全体を赤くしたいだけ」と気づき、`viewVec`/`edge`の計算とtimeベースの計算を削除して原点回帰。
- 赤みの混ぜ方は`finalCol.a *= redAmount`(アルファ操作、誤り)→`finalCol.r *= redAmount`(通常時0で赤が消えるミス)→`finalCol.r += redAmount`(GB成分が残り続け赤一色にならない)→**`lerp(元の色, float3(1,0,0), redAmount)`**という順で、ユーザー自身が試行錯誤しながら正しい形に到達。Claudeは各段階で「この演算だとredAmountが最大の時どうなるか」を問うレビューに徹し、答えは出さなかった。
- ビルド確認済み、シェーダー本体は正常にコンパイルが通ることを確認。

**残タスク（旧、下記で全て完了・解決）:**
1. ~~C++側（`Player.h`/`Player.cpp`）に、ダメージを受けてからの経過時間を管理し、`redAmount`(弱→強→元に戻るカーブ)を計算する仕組みを追加。~~ → 完了。
2. ~~`DamagePS`用の`cbuffer`(`DamageBuffer`, register(b6))へ`redAmount`を渡す配線。~~ → 完了（最終的にb7、下記参照）。
3. ~~`Player::DrawPlayer()`から、`m_isTakingDamage`(既存)をトリガーに`DamagePS`を使うか`LightingPS`を使うか切り替える配線。~~ → 完了。
4. ~~ビルド・実機での見た目確認。~~ → 完了。

### 進捗（2026-08-03続き・DamagePS.hlslのC++側配線完了、ライティングの暗さ問題を調査中・未解決）

**`redAmount`計算〜cbuffer配線〜赤み上限調整まで、DamagePS関連は一通り完成した。**

**`redAmount`計算の実装（ユーザー主体、Claudeはヒント形式のみで直接答えは出さない進め方を継続）:**
- `m_damageTime`(経過フレーム)を`0°〜180°`の角度に変換し、ラジアンに直して`sinf`に渡すことで「0→山(1.0)→0」の左右対称カーブを実装。`180.0f * (m_damageTime / 60)`のように整数同士の割り算になっていたバグ(0になる)を`static_cast<float>`で修正、`60`は`damage_eff_frame`という定数に切り出し。
- `m_isTakingDamage`とは別に`m_isDamageEffect`という専用フラグを新設し、`TakeDamage()`で`true`に、`Update()`で`damage_eff_frame`経過後に`false`+`m_damageTime`リセットする設計にユーザー自身が到達。
- `ShaderRegister::cbuffer_damage`を新規追加する際、最初`b6`にしてしまい既存の`cbuffer_camera`(b6)と衝突しかけたが、レビューで指摘し`b7`に修正(過去のDxLib予約スロット問題の教訓を踏まえ、レジスタ番号の重複は都度確認する必要がある)。
- `DrawPlayer()`内、`BindShaderBuffers()`の後に`m_isDamageEffect`のときだけ`SetShaderConstantBuffer(m_cbufferDamage, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_damage)`をセットする形で配線完了。
- 「真っ赤に染まりすぎる」問題は、`redAmount`(0〜1)に上限係数を掛けて弱める方針で解決（ユーザー自身が実装、詳細な式は未記録だが動作確認済み）。

**新たに発覚: プレイヤー機体・岩と比べて、`FloatingEnemy`（浮遊敵）だけが不自然に暗い問題。デバッグ中、原因未特定のまま次回に持ち越し。**
- ユーザーの気づきがきっかけ。「ライティングを適用させてからずっと暗い」とのことで、今回のDamagePS作業とは無関係の既存の問題と判明。
- 段階的なデバッグ手法（このプロジェクトで何度も使ってきた「値を色として可視化する」）で切り分けを実施:
  1. `diffuse`(法線とライトの内積)を可視化 → プレイヤー機体・岩は手前(カメラ側の面)が正しく明るいのに、`FloatingEnemy`だけ手前を含め全体的に暗いことを確認。
  2. `normalWSFinal`(法線)を`*0.5+0.5`で正規化して可視化 → 機体上面はきちんと緑(Y+方向)になっており、法線計算自体は正常と判明。
  3. `lightVec`(ライト方向)を絶対値化して可視化 → 全オブジェクトで一定の値になっており、ライト方向の値自体も正常と判明。
  4. `Actor::BindShaderBuffers()`/`LightingManager`のコードを確認 → `FloatingEnemy`が特別な値をセットしている箇所はなく、`Player`/`Rock`と同じ共通処理(`LightingBuffer`はシングルトンの`LightingManager`が保持)を使っていることを確認。
- **ユーザーの重要な気づき**: `FloatingEnemy`は`HideState`/`ActiveState`/`LeaveState`のどこにも回転処理が無く、`m_rotation`は常にデフォルト(単位クォータニオン)のはずなのに、見た目はプレイヤー側(カメラ側)を向いている。「回転していないのに正面を向いているのはおかしいのでは」という疑問から、`Player`と同様にモデル自体が逆向きに作られている可能性を疑い、`FloatingEnemy::OnInit()`に`m_rotation = Quaternion(Vector3(0,1,0), DX_PI_F)`(Y軸180度回転)を試験的に追加。
- **この仮説は否定された**: `GetForward()`の値は正しく反転することを確認できた(回転自体は効いている)が、180度回転を加えると`diffuse`は逆にさらに暗くなった。モデルの逆向き問題ではないと判明したため、この回転追加はロールバックする必要がある(**次回作業再開時、`FloatingEnemy::OnInit()`にこの180度回転のコードが残っていないか確認すること**)。
- **次に確認すべき方向性**: `Rock::Draw()`と`FloatingEnemy::DrawEnemy()`はどちらも`Actor::DrawWithLighting()`(共通化済み)を経由しているはずだが、コードの書き方・呼び出し方に何か違いがないか比較する必要がある。`Player`/`Rock`は明るく`FloatingEnemy`だけ暗いという条件の違いを、コードレベルで洗い出すところから次回再開する。

**残タスク（旧）:**
1. ~~`FloatingEnemy::OnInit()`に残っている検証用の180度回転コードを削除(ロールバック)。~~ → 別セッションで対応済み(ユーザー確認済み)。
2. `FloatingEnemy`だけが暗くなる原因調査は一旦保留（下記の通りボス実装を優先する流れになった）。
3. ~~ボスに着手~~ → 下記の通り着手・骨格〜描画まで完成。

### 進捗（2026-08-03続き2・Visual Studioのインデント設定問題を解決、BossEnemyの骨格〜描画成功）

**背景**: 前回の暗さ問題調査から一旦離れ、コスト表で未着手だった「ボス」の実装に着手。既に`BossEnemy`/`BossEnemyDataSetter`の骨格ファイルが存在していたことが判明し、そこから再開する形になった。

**Visual Studioのコード自動フォーマットが気持ち悪い問題を調査・解決:**
- コンストラクタの初期化子リストで`m_pPlayer(`のように括弧を打つと、余計なインデントが自動で入る現象が発生。
- 原因調査の過程で、`.editorconfig`(`NovaWing/.editorconfig`)が2026-07-02(学校のアカウントでのコミット、おそらく先生と作業した際にVisual Studioの「設定から.editorconfigファイルを生成」で作られたもの)から存在していたことが判明。ユーザー自身がこのファイルを一旦削除し、新しく生成し直した。
- 新しい`.editorconfig`でも同じ問題が発生したため、`cpp_indent_within_parentheses`を`indent`→`none`に変更したが効果なし。
- **真因**: Visual Studioの「ツール>オプション>テキストエディター>C/C++>コードスタイル>書式設定」に**「ClangFormat サポートを有効にします」**というチェックがあり、これが有効だと`.editorconfig`のC++書式ルールより、リポジトリ直下の`.clang-format`(2026-07-23にVS Code統一のため作成したもの、`BasedOnStyle: Microsoft`ベース)が優先される。Microsoftスタイルの初期化子リストインデントが原因だった。
- 対策として`.clang-format`に`ConstructorInitializerIndentWidth: 0`/`BreakConstructorInitializers: BeforeColon`を追加してみたが逆効果(左にずれすぎる)で、元に戻した。最終的に**Visual Studio側の「ClangFormat サポート」のチェックを外す**ことで解決（`.editorconfig`側の設定がそのまま効くようになった）。
- **教訓**: Visual StudioでClangFormatサポートが有効だと、`.editorconfig`のC++固有ルール(`cpp_*`)は無視され`.clang-format`が優先される。両方のファイルを併用する場合はどちらが実際に効いているかをこのチェックボックスで確認すること。

**BossEnemyの骨格〜描画に成功:**
- `BossEnemy.h/.cpp`、`BossEnemyDataSetter.h/.cpp`は既に骨格が用意されていた(コンストラクタ、`OnInit`/`Update`/`Draw`/`TakeDamage`、CSV読み込みによる生成処理)。`GameScene`側にも`#include`・メンバー変数・`BossEnemyDataSetter::CreateEnemy`の呼び出しは既に組み込み済みだった。
- **発見したバグ**: `BossEnemyDataSetter.cpp`の`CreateEnemy`関数定義が、`BossEnemyDataSetter::CreateEnemy(...)`ではなく単なる`CreateEnemy(...)`(クラス名の`::`が抜けた、クラスに属さないフリー関数)になっていた。ヘッダー側は`static`メンバー関数として宣言されているため、これでは`GameScene.cpp`から`BossEnemyDataSetter::CreateEnemy(...)`を呼び出した際に未解決の外部シンボル(リンクエラー)になる。ユーザー自身が`::`を補って修正し解決。
- 修正後、ビルド・実行してボスモデルの描画に成功。

**残タスク:**
1. `FloatingEnemy`の暗さ問題調査は保留中、再開時期未定。
2. ボスの本格実装（移動・回転・雑魚敵出現・死亡・ビーム）はこれから。現状は「CSVから生成されて棒立ちで表示される」段階。
3. `BossEnemy::DrawEnemy()`の中身が全部コメントアウトされたまま(`Draw()`は`MV1DrawModel`を直接呼ぶ最小構成)。シェーダー適用(`DrawWithLighting`)を使うかどうかは今後の設計次第。

### 進捗（BossEnemyの脚の海判定・実装途中）

**BossEnemyの脚が「海に入った瞬間」を検知する処理（`Update()`内、`m_prevLegPositions`/`m_currentLegPositions`を使ったY座標比較）をユーザーが実装中。**

- 1回目のレビュー時点のバグ2点（`m_currentLegPositions`が更新されずOnInit時点の値のまま固定されていた点、前後フレームの判定ループが脚の添字を無視して全組み合わせを比較していた点）はユーザー自身で修正済みと確認。
- **代入先バグも修正済み確認**: `Update()`内の脚位置取得ループの代入先が`m_currentLegPositions[...]`に修正されていることを確認した。これで退避タイミング・添字対応・代入先の3点すべて解消され、脚が水面を上→下にまたいだ瞬間だけ判定がtrueになる正しいロジックになった。
- **次回**: `Update()`内のTODOコメント（`//TODO:エフェクトを再生する`、水しぶき等）の実装に進む。

### 運用ルール: コードを見せてと聞かずに直接読むこと（追加）

- ユーザーからコードの提示（「こんな感じでしょうか」等）があった場合、**「コードを見せてください」と聞き返さず、該当ファイルを自分で直接読みに行くこと**。ユーザーはコードを貼らずに「見て」と言うだけの運用を想定しており、聞き返すのは時間の無駄という指摘を受けた。
- [[feedback_no_direct_code_edit]]により.cpp/.h/.hlslの直接編集はしない方針だが、これは「編集しない」だけであり「読む」ことは制限されていない。レビュー・確認のために読みに行くのは問題ない。

### 進捗（BossEnemyアニメーション着手・Visual Studio初期化子リストのインデント問題が再発→解決）

**BossEnemyのアニメーション実装に着手する流れになった（脚の海判定の続き＝エフェクト再生実装より先にアニメーションを行う方針）。**

- 着手の過程で、以前(2026-08-03)解決済みだったはずの「コンストラクタ初期化子リストで`m_animator(`のように`(`を打つとタブ1回分の余計なインデントが入る」問題が再発。
- 原因・対処は前回と同じく**Visual Studioの「ツール>オプション>テキストエディター>C/C++>コードスタイル>書式設定」の「ClangFormat サポートを有効にします」のチェックを外す**ことで解決（チェックが再度有効に戻っていたと推測される）。
- **教訓**: この設定はVisual Studioのアップデートや別PC（学校/家）環境で再度有効に戻ることがありうる。同じインデント症状が出たら、まずこのチェックボックスを確認すること。

**次回**: アニメーション実装の続きから。脚の海判定のTODO（エフェクト再生処理）はその後に着手。

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

**ハマった点3(本命): Worm出現方向のZ+固定と螺旋移動の座標バグ**
- 従来`WormEnemy`は`m_pos.m_z += move_speed;`固定でZ+方向にしか進めず、「前からもワームが来るようにしたい」という要望から`float direction`（1.0でZ+、-1.0でZ-）をコンストラクタに追加し`m_pos.m_z += move_speed * m_moveDirection;`に変更。CSVにも`direction`列を追加。
- 合わせて、螺旋の中心が常にワールド原点基準（`cosf(...) * spiral_radius`のみ）になっており、CSVで指定した初期x,yが1フレーム目で上書きされてしまう問題も発覚。コンストラクタで受け取った`pos`のx,yを`m_spiralCenter`(`Vector2`)として保存し、`Update()`側は`m_spiralCenter.m_x/m_y + 三角関数(...) * spiral_radius`という相対座標に変更して解決。
- **ケアレスミス**: `.h`のコンストラクタ宣言で`float direction//移動方向);`のように、閉じ括弧とセミコロンを行コメントの中に書いてしまいビルドエラー（`//`から行末までは全部コメント扱いになるため）。`float direction);//移動方向`の順に直して解決。
- **ケアレスミス2**: コンストラクタ引数`direction`を受け取ったのに初期化リストで`m_moveDirection`に代入し忘れており、常にデフォルト値0のままでワームが全く前進しなくなっていた。初期化リストに`m_moveDirection(direction)`を追加して解決。
- **ケアレスミス3(本命)**: 螺旋移動のx,y座標計算で、xとyの両方に`sinf`を使ってしまい（本来はcosfとsinfを別々に使うべき）、円運動にならず斜め45度の直線を往復するだけの動きになっていた。xの方を`cosf`に直して解決。
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

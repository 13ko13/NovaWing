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

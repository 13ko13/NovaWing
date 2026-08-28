# 開発メモ（Claudeとの会話ログ）

学校と家でClaude Codeの会話履歴が自動でつながらないため、このファイルに毎回の話した内容を追記しています。
ローカルの会話履歴が切れたときは、このファイルを読み込むことでこれまでの経緯を把握できます。

**2026-08-08、大幅に整理・削減しました。** 単純な完了報告や、現在のコードから読み取れる内容は削除し、今後のデバッグ・学習に使える「教訓」「未解決タスク」「運用ルール」を中心に残しています。

---

## 運用ルール

- **.h / .cpp / .hlsl ファイルはClaudeが直接Edit/Writeしない。** 変更内容を説明し、ユーザー自身がコードを書く（学習目的）。それ以外（CSV、.md等）は直接編集してよい。
- コードの提示（「こんな感じでしょうか」等）があったら、**「見せてください」と聞き返さず、該当ファイルを自分で直接読みに行く**こと。「読む」ことは制限されていない、制限されているのは「編集」のみ。
- ファイルの新規作成・移動・リネームは必ずVisual Studioのソリューションエクスプローラー上で行う。VSCode上でのフォルダ構造変更は`.vcxproj`に反映されないため禁止。
- シェーダー設計は**ユーザーが先にPS_INPUT/テクスチャ/計算方法を考えて宣言し、Claudeはレビューのみ**。ヒントはほぼ答えになるので、聞かれていないのに出さない。二択の提示（AとBどちらだと思いますか等）もユーザーが選択肢を自分で生み出したいためNG。
- デバッグも同様に、原因をすぐ言わず「どこまで自分で考えたか」「何を試したか」を先に聞き返してから最小限の一歩だけ渡す（2026-07-22の方針転換以降、継続中）。
- 質問は1メッセージにつき1つ、事象整理は箇条書きで簡潔に。自分で確認できることは聞かずに確認してから話す。

## 蓄積された教訓（デバッグ手法・DxLib/HLSL仕様）

### DxLibのシェーダーレジスタ・独自シェーダー仕様
- **b2/b3はDxLibの予約スロット**（頂点/ピクセル問わず）。キャプチャパス等でここに`SetShaderConstantBuffer`を呼ぶと、後続の固定機能描画(`DrawPolygon3D`等)が単色崩壊する。自前の定数バッファはb4以降を使うこと。
- **独自シェーダー(`MV1SetUseOrigShader(true)`)を使っていても、ボーン行列(スキニング用パレット)はDxLibが`MV1DrawModel`のたびに自動的に`register(b3)`へ供給してくれる。** C++側で`MV1GetFrameLocalWorldMatrix`等を使って手動送信する必要はない（以前「自動供給されない」と誤って記録していたが誤り）。
- DxLibのスキンメッシュ頂点フォーマット(`VS_INPUT`)は、位置・法線・UV(float4×2)・接線+**従法線(BINORMAL0)**・ボーン番号(int4)まで、型とセマンティクスを1バイトも違わず一致させる必要がある。ズレると「棒立ちですらなく完全に消える/壊れる」壊れ方をする。動作実績のある公式シェーダー(DxLib純正 or 動いているサンプル)の`VS_INPUT`と全項目突き合わせるのが近道。
- `MV1GetAnimIndex`等の名前検索APIは大文字小文字を区別する。`-1`が返ってきたら`MV1GetAnimNum`+`MV1GetAnimName`で実際の名前を列挙して確認する。
- DxLibの固定機能2D描画(`DrawRotaGraph`系)は独自ピクセルシェーダーを反映できない。シェーダーを効かせるには`DrawPolygonIndexed2DToShader`で頂点を自前で組み立てる必要がある。その際`PS_Input`はDxLib側の頂点出力順（`SV_POSITION→COLOR0→COLOR1→TEXCOORD0→TEXCOORD1`）と完全一致させること。
- `SetDrawScreen`はカメラの投影設定・位置設定を両方リセットする。オフスクリーン処理の後は再設定が必要。

### DxLibの乗算済みアルファ・ブレンド
- アルファ付きオフスクリーンへのMV1描画で、マテリアル単位のブレンド設定がグローバルな`SetDrawBlendMode`を上書きし、値が化けることがある。`MV1SetMaterialDrawBlendMode(handle, i, DX_BLENDMODE_NOBLEND)`を全マテリアルに適用するのが確実（グローバル設定はMV1描画では効かない）。
- 半透明メッシュ判定はテクスチャのアルファ要素の有無で決まる。`MV1SetTextureGraphHandle(handle, i, 同じhandle, FALSE)`で再登録すると半透明メッシュ判定を解除できる。

### シェーダーデバッグの型
- シェーダー内の値は「段階別カラー判定」（if文で値の範囲ごとに純色を返す）で可視化するのが確実。グラデーション表示は増幅率で振り切れて情報が消えることがある。
- 「テクスチャに書いた値」と「読んだ値」が食い違うときは、①psoの鮮度（Debug/Release両方のFxCompile設定を確認）②ブレンドモード（乗算済みα）③半透明メッシュ判定、の順に疑う。
- 「模様入りテクスチャなのに単色にしかならない」バグは、**RGBだけでなくアルファチャンネルに模様が入っている可能性**を疑う（`.a`単体を可視化）。また、モデルのUV展開自体が模様表現に向いていない（法線マップ用の極小UV展開）場合もある。UV展開に依存したくない模様は`input.worldPos`の2成分を代替UVとして使うと安定する。
- 「既に正常動作しているテクスチャ(例:skyFront)を同じUV・同じSample呼び出しで代わりに使ってみる」比較実験は、UV/サンプリング機構側かテクスチャ側かを一発で切り分けられる。
- 半透明パーティクル(Effekseer)がモデルの後ろに隠れる場合、コード側の描画順より先に**Effekseerエディタ側のノードごとの「深度テスト」設定**を疑うこと（深度書き込みだけでなく深度テストも見る）。
- 「何も描画されない」ように見えても、モデルスケールを一時的に縮小すると「頂点が壊れた位置に飛んでいるだけ(計算ロジックの問題)」か「本当に何も描かれていない(入力レイアウト等もっと手前の問題)」かを切り分けられることがある。
- 「ロジックの判定タイミングがズレて見える」ときは、判定ロジック自体だけでなく**再生アセット(エフェクト・アニメーション)の素材側の built-in 遅延**も疑う。

### Effekseer
- 「生存時間」(見た目のフェード)と「削除」セクションの「寿命により削除」は別設定。後者のチェックが無いとパーティクルが無限に蓄積し負荷が増大し続ける。生成数を無限にする場合は特に確認must。
- `Trail`(軌跡)ノードはパーティクル数以上に負荷が高くなりやすい。「1回あたりの負荷」×「同時に存在しうる数(連射可能かどうか)」で見積もる。
- 機体に追従するエフェクトを特定パーツに固定したい場合、ワールド座標の固定オフセットではなく`GetForward()`等の回転済み方向ベクトルでオフセット計算する。

### C++/HLSL共通の教訓
- `std::clamp(x, min, max)`は新しい値を戻り値で返すだけで引数を書き換えない。`x = std::clamp(x, min, max)`のように代入し直す必要がある。第3引数(max)にクランプ対象の変数自身を渡すと不正な範囲になりクラッシュすることがあるので注意。
- 複数の位置計算要素（基本位置、Lerp補間、揺れなどのオフセット）が同じ変数に対して順番に処理される設計では、「最終的な位置への加算・オフセット」は必ず一連の計算の一番最後に置く。基本位置を直接代入(`=`)していると途中の加算(`+=`)は上書きされて消える。
- 円・螺旋運動の実装では、x軸とy軸(またはx/z軸)で必ず異なる三角関数(cos/sin)を使う。同じ関数を使うと直線的な動きになる。
- HLSLの`*`演算子は`float4x4 * float`のようにスカラーが絡むと、スカラー側が`float1x1`とみなされ次元不一致で結果が壊れる。スカラー側を`(float4x4)`に明示キャストしてから掛け算する。
- 複数の判定処理が同じ状態フラグを共有する設計では、「当たったかどうかの記録」と「実際に効果を適用するかどうかの制御」を分離すること。同じifの中に混ぜると、片方がガードでスキップされた時にもう片方の結果で誤ったリセットが起きる。
- 前方宣言は対象が`class`/`struct`どちらで定義されているか一致させる必要がある。
- 「Debugでは重いがReleaseでは軽い」場合、実配布に実害はないが開発効率に影響するなら軽量化する価値がある。

## 未解決・保留中のタスク

**2026-08-08、コードを確認して完了済みのタスクを削除しました**（BossEnemyのボーン行列コード整理・model_scale復元、Splashエフェクトの空白対応、CSV化はすべて確認済み・完了）。

1. **`FloatingEnemy`だけライティングが不自然に暗い問題（未解決、再開時期未定）**
   - `Rock`/`Player`は正常なのに`FloatingEnemy`だけ暗い。diffuse/法線/lightVecは個別に確認して正常、`Actor::BindShaderBuffers()`も共通処理で特別な差異なし。180度回転を疑った仮説は否定済み（さらに暗くなった）。
   - 次に確認すべき方向性: `Rock::Draw()`と`FloatingEnemy::DrawEnemy()`はどちらも`Actor::DrawWithLighting()`経由のはずだが、書き方・呼び出し方の違いをコードレベルで洗い出す必要がある。

2. **`near_clip`/`far_clip`が`CapturePS.hlsl`/`WaterPS.hlsl`の2ファイルに重複定義**されている（`LightingPS.hlsl`は既に対象外と確認）。共通`.hlsli`への切り出しが未着手。

3. **DataSetter群（Rock/FloatingEnemy/WormEnemy/BossEnemy）のCSV読み込み〜生成の骨格が重複**。テンプレート基底クラスかフリー関数での共通化が候補、未着手。
   - **`CSVData`基底クラス（`CSVData/CSVData.h`、`virtual void Conversion()`を持つ）が、どのDataSetterからも継承されず活用されていないことが判明（2026-08-18）。** `CSVDataLoader::LoadCSV`は常に`CSVData`そのもの（生の`std::wstring`配列の入れ物）を作って返すだけで、各`XxxDataSetter`（`RockDataSetter`等）はそこから`GetData()`で生配列を取り出し、列番号を直接指定して自前でパースしている。
   - **参考プロジェクト`C:\Users\SakamotoKou\Documents\GitHub\ProjectNeaR`で本来の使い方を確認済み。** `CSVDataLoader::LoadCSV`自体は同じ実装（常に`CSVData`を生成）だが、呼び出し側で`ActorData(std::shared_ptr<CSVData> data)`のように**`CSVData`を受け取るサブクラスのコンストラクタ**を用意し、そこで`Conversion()`相当の変換処理（列番号パース→意味のあるメンバへの変換）を行うパターンだった。`CSVDataLoader`自体の変更は不要で、サブクラス（例:`RockCSVData : public CSVData`）を新設し、今`RockDataSetter::CreateRock`に直書きされている列番号パースをそちらに移す形が本家に忠実な直し方。
   - **保留中のタスクとして記録のみ。着手はまだ先。**

4. **`LightingPS.hlsl`(共通化済み)と`WaterPS.hlsl`間で視線ベクトル・反射ベクトル・specular計算式が重複**。`WaterPS.hlsl`側だけまだ独自実装のまま、`.hlsli`共通化が候補、未着手。

5. **ボスの本格実装（2026-08-21完了）。** 雑魚召喚・ビーム攻撃・登場演出・死亡エフェクトまで実装・動作確認済み。多段ヒット防止は見送り、ダメージ値を1に固定。ビームの当たり判定球配列は判定終端を通り過ぎたものを削除する対応済み。

6. **VS CodeのIntelliSenseで`DxLib::VECTOR`等の型が「不完全な型」表示になる問題、原因未特定のまま保留。** ビルド自体は正常なため実害なし。深追いは費用対効果が低いと判断済み。

7. **`CameraBase`のGameCamera/TitleCamera分割、GameCamera側は完了（2026-08-21）。** 詳細は下記「進捗（2026-08-21・タイトルシーン着手）」参照。`TitleCamera`はまだ空の骨格のみ、これから中身を実装する。

8. **リプレイ等で2回目のGameSceneに入ったとき、前回のEffekseerエフェクトが残っていることがある。** シーン終了時にエフェクトを明示的に停止・クリアする処理が必要（未着手）。

## 進捗（2026-08-21・タイトルシーン着手：CameraBase分割、TitlePlayer/TitleCamera新設）

**タイトルシーンの演出（プレイヤー前進→宙返り→ブースト消失→ロゴ/選択肢フェードイン、カメラはプレイヤー追従→途中で固定に切り替え）に向けて設計・実装を開始。**

**合意した設計方針:**
- 演出はタイトル専用の軽量クラス`TitlePlayer`(`Actor`継承)・`TitleCamera`で実装する。既存`Player`のステートマシンは流用せず、宙返り等の動きも一から書き直す方針（`Player`には演出用の余計な穴を開けない）。
- 宙返りの**動き自体**はゲームシーンの宙返り(`SomersaultState`)と同じでよい。
- カメラ揺れ・ズームは`TitleCamera`には不要。

**`CameraBase`のGameCamera/TitleCamera分割、完了（GameCamera側）:**
- `TitlePlayer`が`Actor`を継承する設計にしたところ、`Actor`のコンストラクタが`std::weak_ptr<CameraBase>`(具体クラス)を要求しており、`CameraBase`とは無関係な新規`TitleCamera`を渡せない問題が発覚。これをきっかけに、以前から気になっていた「`CameraBase`を継承前提の名前にしたのに全部直書きしてしまっている」問題に着手することに。
- **新しい構成**: `CameraBase`(共通基底、抽象クラス) → `GameCamera`(揺れ・ズーム・プレイヤー追従などゲームプレイ専用機能) / `TitleCamera`(未実装、タイトル演出専用)。
  - `CameraBase`に残したもの: `m_targetPos`/`m_prevTargetPos`/`m_prevPos`、`GetForward()`/`GetFov()`/`GetFrustumHalfSize()`/`SetUpCamera()`、`Update()`の骨組み（前フレーム位置を保存→仮想関数`UpdatePosition()`を呼ぶ→`SetCameraPositionAndTarget_UpVecY`でセット、の3段階）。
  - `GameCamera`に移したもの: 揺れ(`OnShake`/`IsShake`/`UpdateShake`)、ズーム(`OnZoomUp`)、プレイヤー追従の具体的な計算(`UpdateTargetPos`/`UpdatePosition`本体)、`m_pPlayer`。
  - `UpdatePosition()`を純粋仮想関数にし、`if(ズーム中){...} else {プレイヤー追従...}`という分岐ごと`GameCamera::UpdatePosition()`に丸ごと移した（`TitleCamera`にはズーム自体が無いので、この分岐構造も不要と判断）。
- **ハマった点（複数、いずれも解決済み）**:
  1. `GameScene.cpp`が旧`std::make_shared<CameraBase>(m_pPlayer)`のままだった（新`CameraBase()`は引数無しコンストラクタ、`Player`を受け取るのは`GameCamera`側）。`std::make_shared<GameCamera>(m_pPlayer)`に修正、`#include`も`CameraBase.h`→`GameCamera.h`に変更。
  2. `GameScene.h`の`m_pCamera`を`std::shared_ptr<GameCamera>`型に変更（`OnShake`/`OnZoomUp`等GameCamera固有の関数を呼ぶ必要があるため）。
  3. `GameScene.cpp`内、プレイヤー生成時に渡す「まだ生成されていない空カメラ」の型を一時`std::weak_ptr<GameCamera>()`に書き換えてしまったが、`Player`(`Actor`)のコンストラクタは共通の`std::weak_ptr<CameraBase>`を要求するため誤りと判明、`std::weak_ptr<CameraBase>()`に戻して解決。**設計判断: `Player`のコンストラクタ引数は`GameCamera`専用にせず`CameraBase`型のまま維持**（`Actor`との一貫性を優先、`Player`が将来`GameCamera`以外と組み合わさる可能性も残す）。
  4. `GameCamera`のデストラクタ実装(`.cpp`側)が抜けており`LNK2019`(未解決の外部シンボル)でリンクエラー。宣言(`.h`)はあったが実装が無い状態だった。
  5. `CameraBase.cpp`に不要な`#include "GameCamera.h"`(何も使っていない)が残っていたため削除。同様に`#include "Player.h"`も未使用の可能性があり要確認。
- **教訓**: 派生クラスへの分割作業では、①コンストラクタのシグネチャ変更が全呼び出し元に波及する ②`.h`の宣言と`.cpp`の実装が両方揃っているか(特にデストラクタは書き忘れやすい) ③型の派生関係(`shared_ptr<派生>`と`shared_ptr<基底>`は別の型)を意識して、どの型を要求するインターフェースなのか確認する、の3点を都度チェックする必要がある。

**現状（2026-08-21時点）**: `TitlePlayer.h`(`Actor`継承の空クラス)、`TitleCamera.h`(`CameraBase`未継承のTODO付き空クラス)は新規作成済みだが中身は未実装。`GameCamera`側は完成、既存のゲームプレイの動作確認済み。

**次回やること（旧、下記の続き参照）:**
1. ~~`TitleCamera`を`CameraBase`から継承させ、`UpdatePosition()`等を実装する。~~ → 完了。
2. ~~`TitlePlayer`の中身を実装する。~~ → 前進・宙返りフェーズ、描画まで完了。ブーストはまだ（下記参照）。
3. ~~`TitleScene`にこれらを組み込む。~~ → `TitleScene::Init()`で`TitlePlayer`/`TitleCamera`を生成・`GameObjectManager`登録済み、タイトル画面にプレイヤー機体が表示・前進する状態まで動作確認済み（下記参照）。演出全体のステート管理（前進→宙返り→ブーストの切り替えタイミング）はまだ未実装。
4. タイトルロゴのスタンプ演出（拡大→通常サイズ）、選択肢のフェードインを実装する。

## 進捗（2026-08-21続き2・TitleCamera/TitlePlayerの基礎実装）

**`TitleCamera`と`TitlePlayer`の骨組み〜前進フェーズまで実装、ビルド確認済み。**

**設計の合意事項:**
- 演出のフェーズ切り替え（前進→宙返り→ブースト、カメラの追従→固定）は、`TitlePlayer`/`TitleCamera`が自律的に判断するのではなく、**`TitleScene`側から明示的に指示する**方式に統一（`TitleCamera::StopFollowing()`、`TitlePlayer::StartSomersault()`/`StartBoost()`のような外部公開メソッドで切り替える）。
- 宙返りの**動き自体**はゲームシーンの宙返り(`SomersaultState`)と同じ内容でよいが、実装はコピーせず一から書く（`Player`の複雑なステートマシンには依存しない）。

**`TitleCamera`（完了）:**
- `CameraBase`を継承。`m_pPlayer`は`std::shared_ptr<TitlePlayer>`で保持（`GameCamera`が`weak_ptr`だったのとは意図的に方針を変えた、`TitleCamera`が`TitlePlayer`を強参照で持つ設計）。
- `m_isFollowing`(初期値`true`)で追従状態を管理。`UpdatePosition()`は追従中のみ`m_targetPos`をプレイヤー位置に更新、`StopFollowing()`が呼ばれた後は`m_pos`/`m_targetPos`とも一切更新しない（＝最後に追従していた向きで固定される）方針。カメラ位置(`m_pos`)自体は追従中も固定のまま動かさない設計（合意済み）。

**`TitlePlayer`（前進・宙返り・描画まで完了、ブーストは未実装）:**
- `Actor`を直接継承（`Charactor`は継承しない）。フェーズ管理は`enum class Phase { Forward, Somersault, Boost }`。フェーズ切り替えは`TitleScene`から`StartSomersault()`/`StartBoost()`を呼ぶ（合意方針通り）。
- **ハマった点: `SetVel`で速度をセットしても、`Actor`には速度を位置に反映する処理が無く、何も動かなかった。** `m_pos += m_velocity`は`Charactor::Update()`に実装されている処理だが、`TitlePlayer`は`Charactor`を継承していないため自動的には効かない。`TitlePlayer::Update()`内に同じ処理を自前で追加して解決。
- **ハマった点2: 位置反映(`m_pos += m_velocity`)と速度計算(`SetVel`)の順序を最初逆にしてしまい、1フレーム遅れの状態になっていた。** `BossEnemy::Update()`を参考に「先に`SetVel`で今フレームの速度を決めてから、後で`Charactor::Update()`相当の位置反映をする」正しい順序に修正。
- 描画(`Draw()`)は`Rock::Draw()`のパターン（`ApplyMatrix`→`UpdateShaderMatrixData`→テクスチャ取得→`DrawWithLighting`）をそのまま踏襲、プレイヤーの法線マップ等（`GraphicID::PlayerNormalMap`等）とスケール(`{0.3f,0.3f,0.3f}`、`Player`と同じ値)を使用。
- **宙返り実装**: 既存`SomersaultState::Update()`のロジック（進行度計算→X軸回転角→`sin`/`cos`で縦一回転する速度ベクトルを作る）を、`Player`固有機能(ゲージ消費・`LerpToAngleX`)を除いて移植。回転は`Quaternion(Vector3(1,0,0), -angle)`で`m_rotation`に直接代入する方式（合意通り、角度変数は持たない）。終了判定は`IsSomersaultEnd()`（`TitleScene`側が監視して次フェーズへ進める設計、合意方針通り）。
- **重大なハマりどころ: `TitlePlayer::OnInit()`の実装漏れでクラッシュ（`m_pCbufferMatrixData`がnullptrのまま`Draw()`が呼ばれアクセス違反）。** `Rock`等が`OnInit()`で`CreateShaderBuffers()`を呼んでいたのに、`TitlePlayer`にはそもそも`OnInit()`のオーバーライドが存在しなかった。追加して解決。
- **もう1点: プレイヤーモデルが逆向きに作られている問題(過去から既知)への対応漏れ。** `Player::OnInit()`が`m_rotationY = DX_PI_F`(角度変数経由)で補正していたのに対し、`TitlePlayer`は角度変数を持たない設計のため、`OnInit()`内で直接`m_rotation = Quaternion(Vector3(0,1,0), DX_PI_F)`を代入する形で補正。これも`OnInit()`実装時に合わせて追加。
- **解決済み: 宙返り中にモデルのY軸180度補正が失われる問題。** X軸回転(`Quaternion(Vector3(1,0,0), -targetAngleX)`)を`m_rotation`に直接代入すると、`OnInit()`で設定したY軸補正が上書きで消えてしまっていた。**「Y軸補正を先に適用し、その上にX軸回転を掛ける」順序で2つのクォータニオンを合成**(`m_rotation = somersaultRotation * initRotation`)して解決。クォータニオンの掛け算`A * B`は「先にBの回転を適用し、その上にAをかける」非可換な演算であるため、合成順序が重要という理解に到達。

**`TitleScene`への組み込み（完了、動作確認済み）:**
- `TitleScene::Init()`冒頭で`GameObjectManager::GetInstance().ClearAll()`を呼び、`TitlePlayer`/`TitleCamera`を生成して`Init()`→`GameObjectManager`に自動登録。`Update()`/`Draw()`は`GameObjectManager::GetInstance().UpdateAll()`/`DrawAll()`で一括処理する設計に統一（`GameScene`と同じ発想だが、水の透過キャプチャのような特殊な2回描画は不要なためシンプルな1回呼び出しのみ）。
- **ハマった点: `TitlePlayer`のコンストラクタに、`ResourceLoader::ModelID`ではなく`ResourceLoader::GetModel(...)`で取得した生のモデルハンドル(int)を渡してしまっていた。** `Actor`のコンストラクタは`ModelID`(enum)を受け取り内部で`MV1DuplicateModel`する設計のため、型は合っていてもint値の意味が全く違い、`GameScene`の`Player`生成コードと同じ形(`ResourceLoader::ModelID::Player`をそのまま渡す)に修正して解決。
- **ハマった点: `TitleScene.cpp`で`std::weak_ptr<CameraBase>()`を書く際、`CameraBase.h`の`#include`が抜けており`C7568`(想定される関数テンプレートの後に引数リストがない)エラー。** `TitlePlayer.h`が`Actor.h`経由で`CameraBase`を前方宣言でしか知らないため、完全な型が必要な箇所には別途`#include`が要ることを再確認。

**現状（2026-08-21時点）**: `TitlePlayer`は前進・宙返り・ブーストの3フェーズとも実装済み・動作確認済み（`OnInit()`で初期位置`Vector3(0,0,-900)`もセット済み）。`TitleScene`側からのテストトリガーで宙返りへの遷移も確認済み。

## 進捗（2026-08-21続き3・タイトル演出ステート管理、ロゴ/選択肢演出まで完成）

**タイトルシーンの演出一式（前進→宙返り→ブースト→ロゴのスタンプ演出→選択肢のフェードイン）が完成した。**

**`TitleScene`の演出ステート管理:**
- `TitleScene`に`enum class Phase { Forward, Somersault, Boost, LogoAndSelect }`を追加、`GameScene`の`BossApearState`と同じ`switch`文パターンで管理。
- `Forward`→`Somersault`: `player_forward_max_frame`(120)経過で`TitlePlayer::StartSomersault()`。
- `Somersault`→`Boost`: `TitlePlayer::IsSomersaultEnd()`で判定し`StartBoost()`。
- `Boost`→`LogoAndSelect`: `player_boost_max_frame`(120)経過で遷移、このタイミングで`TitleCamera::StopFollowing()`も呼ぶ（合意通り、ブースト終了時に追従をやめる仕様で確定）。
- `LogoAndSelect`: ロゴ用フレーム(`m_titleLogoFrame`)と選択肢フェード用フレーム(`m_selectFadeFrame`)をそれぞれ独立してカウント。

**ロゴのスタンプ演出（拡大→通常サイズ）:**
- `progress = m_titleLogoFrame / logo_max_frame`(30)を`std::lerp(logo_max_scale(3.0), 1.0, progress)`に通し、`DrawRotaGraph`の拡大率に反映。`std::lerp`はC++20の標準関数（このプロジェクトはC++20を使用しているため自作のLerpユーティリティは不要と判断）。

**選択肢のフェードイン、実装の紆余曲折:**
- 既存の`DrawGraphToShaderByCenter`(独自シェーダー経由の2D描画)にはアルファ値を指定する引数が無かったため、`alpha`引数(デフォルト値`1.0f`)を新設。
- **最初の実装ミス**: `SetDrawBlendMode`の第2引数(ブレンド強度)にalphaを適用してしまい、方針(頂点カラー`dif`のアルファ成分で制御)とズレていた。`dif`側(`GetColorU8(255,255,255,alpha*255)`)に統一し、`SetDrawBlendMode`は元の固定値`255`に戻して解決。
- **本命のバグ: `dif`にアルファを正しく設定しても、見た目が全く透明にならなかった。** 原因は`GlitchPS.hlsl`側にあった。シェーダーの最終出力(`return float4(finalCol, baseCol.a)`)が、頂点カラー(`input.dif.a`)を一切使わず、**テクスチャ自体のアルファ値(`baseCol.a`)だけ**を出力アルファにしていた。`return float4(finalCol, baseCol.a * input.dif.a)`のように両方を掛け合わせる形に修正して解決。**教訓**: 「頂点カラーのアルファを設定したのに反映されない」場合、C++側の頂点データだけでなく、ピクセルシェーダーの出力側が実際にそのアルファ成分を使っているかを確認する必要がある。
- `m_selectFadeFrame`のインクリメントを、最初`m_titleLogoFrame`と同じ`if`条件(ロゴの上限フレームでガード)に紐づけてしまい、`select_max_frame`(50)より短いロゴの上限(30)でカウントが止まってしまうバグがあった。選択肢用の`if`条件を独立させて解決。

**現状**: タイトルシーンの一連の演出はほぼ完成。

## 進捗（2026-08-21続き4・タイトルシーンに海とスカイボックスを追加）

**`WaterManager`/`SkyBox`を`TitleScene`にも追加、動作確認済み。**

- `WaterManager`のコンストラクタは`std::shared_ptr<CameraBase>`（共通基底型）を要求する設計だったため、`TitleCamera`をそのまま渡せた（`GameCamera`分割時に共通の型で統一しておいた設計が活きた）。`SkyBox`もカメラの型に依存しない(`Draw(cameraPos)`のみ)ため同様に組み込み容易だった。
- **ハマった点1: 海のメッシュの端(切れ目)が視界に入ってしまう。** `GameScene`側にある「プレイヤーが端に近づいたらメッシュをワープさせる」仕組み自体は流用されるが、タイトル演出はプレイヤー・カメラの移動距離がメッシュサイズ(30000程度)よりかなり小さいため、本来ワープ機構に頼らずとも収まるはずだった。**対処**: `TitlePlayer`/`TitleCamera`の初期位置を調整（メッシュの中心寄りにする）ことで解決。
- **ハマった点2: 手前の海が不自然に暗く見える。** ライティング関連の設定漏れが原因（詳細な特定方法は未記録、ユーザーが解決）。**次回、同種の問題が起きた場合は`LightingManager::SetLightDirection`が`TitleScene::Init()`で呼ばれているか確認すること**（`GameScene`ではプレイヤー生成時などに設定されている可能性がある）。

**次回やること（旧、下記の続き参照）:**
1. 細部の見た目調整（速度・フレーム数・座標などのバランス調整）があれば随時対応。
2. リプレイ等でEffekseerエフェクトが残留する問題（上記タスク8）はまだ未着手のまま。

## 進捗（2026-08-21続き5・ボスHPゲージの表示タイミング修正、リザルトシーンに着手）

**ボスHPゲージが常時表示されていた問題を修正。** `BossEnemy`に`IsBossAppear() const`ゲッターを追加（既存の`SetIsBossAppear`はセッターのみでゲッターが無かった）。`BossHPGaugeUI`側でこれを見て、ボス出現前は描画しないように対応済み。

**次のタスク: リザルトシーンの新規作成に着手。** 詳細設計はこれから。

## 進捗（2026-08-21続き6・リザルトシーンのデータ受け渡し実装）

**リザルトシーン（`ClearScene`）に「倒した敵の数・クリアタイム・被弾回数」を渡す仕組みを実装完了。** データの受け渡しは終了、表示処理はこれから。

- **データ構造**: `ClearScene`に`ClearResultData`構造体を追加（`defeatedEnemyCount`, `clearTime`(フレーム数のまま), `hitCount`）。`ClearScene`のコンストラクタを`ClearScene(SceneController&, const ClearResultData&)`に変更し、メンバー`m_resultData`として保持。
  - 受け渡し方式は「シーン遷移（`ChangeScene`）時に`make_shared<ClearScene>`へ構造体を直接渡す」というシンプルな設計をユーザーが提案・採用（シングルトン等の複雑な仕組みは避けた）。
  - クリアタイムは一旦フレーム数のまま保持する方針（秒への変換は表示側で今後検討）。
- **Player**: `m_hitCount`/`GetHitCount()`（`TakeDamage()`内でカウントアップ）、`m_defeatedEnemyCount`/`GetDefeatedEnemyCount()`/`AddDefeatedEnemyCount()`を追加。
- **EnemyBase**: 「倒した敵の数」カウントの実装方式について、各敵クラス（FloatingEnemy/WormEnemy/BossEnemy）に個別実装する案と、`EnemyBase`に共通処理をまとめる案を比較検討し、ユーザーが共通化案を選択。`EnemyBase::OnEnemyDead()`を新設し、`m_pPlayer.lock()->AddDefeatedEnemyCount()`を呼んでから`OnDead()`を呼ぶ設計に統一。
  - `FloatingEnemy.cpp`/`WormEnemy.cpp`/`BossEnemy.cpp`の3クラスそれぞれの完全死亡分岐で`OnDead()`→`OnEnemyDead()`に置き換え（ユーザー依頼によりClaudeが直接編集）。
- **GameScene**: ボス撃破判定（`m_pBoss->IsDead()`）のタイミングで`ClearResultData`を組み立て（`clearTime`は`m_frame`、`defeatedEnemyCount`/`hitCount`はPlayerから取得）、`ClearScene`へ`ChangeScene`する処理を実装済み。

**次回やること:**
1. `ClearScene::Draw()`に`m_resultData`の実際の数値表示を実装する（クリアタイムをフレームのまま出すか秒に変換するかは未決定、次回検討）。
2. 倒した敵の数・クリアタイム・被弾回数を総合した「評価（スコア）」算出ロジックの設計・実装（まだ未着手）。
3. 細部の見た目調整（速度・フレーム数・座標などのバランス調整）があれば随時対応。
4. リプレイ等でEffekseerエフェクトが残留する問題（上記タスク8）はまだ未着手のまま。

## 進捗（2026-08-24・学校で作業、リザルトテンプレート画像とカーテン演出）

**注意**: 学校での作業時にノート記入を忘れたため、家に帰ってきてからコードを見て事後的に記録した内容。

**リザルトのテンプレート画像を、カーテンのように左右へ開く演出で表示するところまで実装完了。**

- `ClearScene`に`ResourceLoader::GraphicID::ResultTemplete`（リザルト用テンプレート画像）を追加し、`DrawGraphToShaderByCenter`（グリッチシェーダー適用の中心基準描画、`Utility/GraphShaderDraw`）で描画。
- カーテン演出のロジック: `m_templeteOpenFrame`をフェード完了後（`!m_controller.GetFade().IsFading()`）から`templete_opne_max_frame`(30F)までカウントアップし、`openProgress`(0〜1)を計算。中心(0.5)を基準に`uvMinU = 0.5 - openProgress*0.5`、`uvMaxU = 0.5 + openProgress*0.5`とすることで、UVの表示範囲を中心から左右に広げ「カーテンが開く」ように見せている。
- `m_resultData`（倒した敵の数・クリアタイム・被弾回数）の描画は`DrawFormatString`で仮実装済みだが、現在はコメントアウトされたまま（[ClearScene.cpp:157-159](NovaWing/NovaWing/Scene/ClearScene.cpp#L157-L159)）。

**次のタスク: フォントを適用してスコア等の情報を描画する。**

## 進捗（2026-08-25・独自ttfフォント読み込みとリザルト数値の光彩演出）

**リザルトの数値（倒した敵の数・クリアタイム・被弾回数）に独自フォント(`Orbitron Black`)を適用し、光彩(グロー)演出まで実装。**

- **独自フォント読み込み**: `ResourceLoader`に`ModelID`/`GraphicID`/`EffectID`/`SoundID`と同じ並びで`FontID`(現状`Result`のみ)を追加。読み込みはDxLib専用関数が無いため、Windows API `AddFontResourceEx`（ttfをOSに一時登録、`<Windows.h>`が必要）→DxLib `CreateFontToHandle`（登録済みフォント名からハンドル作成、名前は内部フォント名`"Orbitron Black"`を使う）の2段階。
  - 解放時は`AddFontResourceEx`で登録した際の**パス**も`RemoveFontResourceEx`に必要になるため、単純な`unordered_map<FontID, int>`では情報が足りず、`FontData{ int handle; LPCWSTR path; }`という構造体をマップの値にする設計に変更（ユーザーが3案から選択）。`ReleaseAll()`のforループで`DeleteFontToHandle(handle)`と`RemoveFontResourceEx(path,...)`を両方行う。
  - 文字が数字によって連結して見える問題（例:"17"の7の横棒が隣の1にくっつく）は`SetFontSpaceToHandle`で文字間隔を広げて解決。
- **文字へのシェーダー適用**: `DrawFormatStringToHandle`はDxLibの固定機能描画のため独自ピクセルシェーダー(グリッチ)を反映できない。そこで`Init()`でオフスクリーン(`MakeScreen`)に文字を一度だけ描画し(`m_textRenderTargetH`)、そのオフスクリーン画像を`DrawGraphToShaderByCenter`（シェーダー経由の専用関数、テンプレート画像と同じ仕組み）で毎フレーム描画する方式にした。`m_resultData`はコンストラクタ後不変のため、文字の描き込みは`Init()`で1回のみで足りる。
  - ハマった点: `MakeScreen`はデフォルトで不透明な黒背景になるため、第3引数`true`（アルファチャンネルあり）を指定しないと、文字の周り(背景)が黒い矩形として他の描画(テンプレート画像等)を覆い隠してしまう。
- **光彩(グロー)演出**: 「同じ文字をもう一枚のオフスクリーン(`m_textGlowH`)にも描き、そちらだけ`GraphFilter(handle, DX_GRAPH_FILTER_GAUSS, PixelWidth, Param)`でぼかしてから、`SetDrawBlendMode(DX_BLENDMODE_ADD,...)`で先に加算合成描画→くっきり文字を通常合成で重ねる」という2枚構成で実装。
  - `GraphFilter`は**引数を4つ受け取り、渡したハンドル自体を直接書き換える**関数（別ハンドルへコピー出力はできない）。`PixelWidth`は8/16/32のいずれかのみ有効、`Param`は「100で約1ピクセル分」の目安（公式リファレンスで確認）。
  - ダウンロード内の`SampleTPSGame`の`GameScene.cpp`（`GraphFilter(RTBloom_, DX_GRAPH_FILTER_GAUSS, 16, 1400)`、ぼかし前後に`DrawGraph`で加算合成描画するパターン、別箇所では2回連続ぼかしがけの例）を参考に、`blur_range=16`, `blur_strength=1400`、2回連続`GraphFilter`呼び出しに調整。
  - **既知の限界**: `ResultTemplete`画像側のラベル文字(COMPLETE等)は画像編集ソフトで事前に焼き込まれた光彩のため綺麗だが、リアルタイムでガウスフィルターをかけている数字側は同じクオリティには届かない。ユーザー判断で「現状のクオリティで十分」として一旦区切りをつけた。

**次回やること:**
1. 倒した敵の数・クリアタイム・被弾回数を総合した「評価（スコア）」算出ロジックの設計・実装（まだ未着手）。
2. 細部の見た目調整（光彩の強さ、文字の座標・色・サイズなど）があれば随時対応。
3. リプレイ等でEffekseerエフェクトが残留する問題（上記タスク8）はまだ未着手のまま。

## 進捗（未記録期間・ClearSceneに評価/選択肢一式が実装済み、選択肢ワイプが未完成のまま残っていることが判明）

**注意**: 学校での作業時にノート記入を忘れたため、後から`ClearScene.cpp`のコードを読んで事後的に記録した内容。この間に以下が完成していた。

- スコア算出ロジック実装済み: `m_score = defeatedEnemyCount * kill_score_multiplier - (clearTime/60 + hitCount) * score_multiplier`（`Init()`で計算）。
- リザルト数値（キル数/クリアタイム/被弾数/スコア）を、開く演出完了後に`std::lerp`でカウントアップ風に近づける演出、`lerp_guard_threshould`で目標値にぴったり丸める処理、全項目のLerp完了フラグ(`m_isLerpFinished`)まで実装済み。
- 「次へ」ボタン(`InputEvent::next`)を押すと、テンプレート画像が閉じる演出(`m_templeteOpenFrame`を減算)→閉じきったら選択肢背景が開く演出(`m_backGroundOpenFrame`)、という2段階の演出フローが実装済み。
- 選択肢は`ReTry`(リトライ)/`BackTitle`(タイトルに戻る)の2択（`ExitGame`ではなく`ReTry`だった。`GraphicID`にも`ReTry`/`ReTryOnCursor`/`BackTitle`/`BackTitleOnCursor`が用意されている）。
- 選択肢切り替え・ワイプ進行度の増減ロジック(`m_wipeProgress`、`m_selectIndex != m_prevSelectIdx`でリセット)自体は`TitleScene.cpp`と同じパターンで`Update()`に実装済み。

**未完成: 選択肢の「カーソルが乗ったら左から右にワイプ」演出が、`Draw()`側の描画呼び出しの誤りにより実際には機能していない。**

`TitleScene.cpp`の正しいパターン(367〜425行目)は「通常画像を常に描画→カーソルが乗っている方だけカーソルオン画像を`uvMaxU`(ワイプ進行度)で重ね描き」だが、`ClearScene.cpp`の`Draw()`(436〜537行目)は以下の点で異なる・壊れている:
1. `ClearSelect::ReTry`/`BackTitle`どちらのケースも、**カーソルオン画像(`ReTryOnCursor`/`BackTitleOnCursor`)を一度も使っていない**。通常画像(`retryHandle`/`backTitleHandle`)だけを描画している。
2. `ReTry`ケース内の`DrawGraphToShaderByCenter`呼び出し(476〜484行目)で、`m_wipeProgress[...]`を`alpha`の次の引数(`uvMaxU`の位置)に渡しているが、そもそも渡している画像が通常画像(`retryHandle`)のためワイプの意味を成していない。`backTitleHandle`側は`uvMaxU`引数自体を省略(デフォルト1.0=フル表示)している。
3. `switch`文より前(401〜433行目)で通常画像を無条件描画した後、`switch`文の`openProgress != 1.0f`分岐(406〜461行目、492〜511行目)で**同じ内容をもう一度描画**しており、開く演出中は二重描画になっている。

**対処方針（次回、`TitleScene.cpp`の367〜425行目のパターンに合わせて修正すること）**:
- `switch`文の「開く演出が終わっている場合」の分岐(現在の`else`ブロック、463〜486行目・513〜534行目)を、`TitleScene`と同じ構造にする: 通常画像は常に描画した上で、選択中の項目だけ`m_wipeProgress[選択肢番号]`を`uvMaxU`としてカーソルオン画像(`ReTryOnCursor`/`BackTitleOnCursor`)を重ね描きする。
- 二重描画になっている`switch`文前(401〜433行目)の無条件描画と、`switch`内の`openProgress != 1.0f`分岐の重複も整理が必要（開く演出中はどちらか一方だけでよいはず）。

## 進捗（2026-08-11〜18・ボスの雑魚召喚/ビーム実装、多段ヒットガードの設計を試行錯誤中）

**新規クラス**: `EnemyFactory`/`EnemyBase`(FloatingEnemy/WormEnemy/BossEnemyの共通基底)、`BossIdleState`/`SummonState`/`BossBeamState`(`IBossEnemyState`のステートマシン)。

**ステートマシンの自動Enter/Exit化**: `BossEnemy::Update()`に`FloatingEnemy`と同じ「Update→GetNextState()確認→あればExit/切替/Enter」のパターンを追加。各ステートは`ChangeState(...)`を呼ぶだけでよい。

**繰り返し出た「コンストラクタ引数を初期化リストへ渡し忘れる」バグ**: `EnemyFactory`の`m_pTargetManager`/`m_pCollisionManager`、`SummonState`の`m_pEnemyFactory`、`BossIdleState`の`m_pPlayer`など、複数箇所で発生。`weak_ptr`が常に空のままになり`.lock()`が毎回nullptrを返す静かな不具合になりやすいので、コンストラクタ引数を増やしたら初期化リストへの反映を必ずセットで確認すること。

**重大バグ(解決済み): `BossEnemy`生成が`EnemyFactory`生成より先に行われ、ボスの`m_pEnemyFactory`が常に空になっていた。** `GameScene::Init()`内の生成順序の問題に加え、`EnemyFactory`が`CollisionManager`/`TargetManager`(さらに`m_pBoss`が必要)に依存する循環依存だった。**対処**: `BossEnemyData`から`pEnemyFactory`を削除し、`BossEnemy::SetEnemyFactory(...)`というセッターを新設、`EnemyFactory`生成後に呼ぶ形にした。**教訓**: 循環的な依存関係はコンストラクタ一括注入ではなくセッターで一部を後から渡す設計にする。

**重大バグ(解決済み): `TargetManager`/`CollisionManager`の敵配列に死亡済みのweak_ptrが溜まり続けてクラッシュ・重さの原因に。** `BulletManager`と同じ`std::remove_if`+`erase`パターンで対処。weak_ptrの配列を持つマネージャーは「登録」だけでなく「死んだものの除去」もセットで実装する。

**ビームの見た目・当たり判定の実装が二転三転した末、現在の形に着地**:
- 先端の移動: 最初はプレイヤー位置へLerpし続ける方式(近づくほど減速する)だったが、「近づくと遅くなるのが変」「プレイヤーに追従しすぎる」という指摘から、**「プレイヤー背後(GetVisualBack()方向)の目標点に向け、一定速度(beam_speed)で直進、プレイヤーのZを越えたらそれ以降は直進を続ける」**方式に変更。
- 当たり判定: 「発射口〜現在の先端をLerpで結んでプレイヤーのZ比率で1点求める」方式は、ビームが毎フレーム目標を追って蛇行するため実際の軌跡と乖離し、当たり判定がずれる不具合があった(特に長時間追い越すと発射口側に判定が引き戻される副作用も発覚)。複数球を軌跡沿いに並べる案は「配列/forループが重そう」「隙間で判定漏れしそう」という理由で一度却下されたが、最終的に**「プレイヤーのZを超えるまでの間、`beam_col_interval`(10)フレームごとに先端位置で球を生成し配列に追加していく」**方式に着地。配列は増える一方で削除処理はまだない。
- プレイヤーのモデルが逆向きなため`GetForward()`/`GetBack()`は見た目と正反対を指す。`Player::GetVisualForward()`/`GetVisualBack()`を新設し、以後は見た目通りの前後が欲しい場面ではこちらを使う。
- `Vector3::Lerp`の「現在位置」と「移動量」を混同し`m_beamPosL = 方向 * 速度`のように位置を移動量で丸ごと上書きするミス(`+=`ではなく`=`)、左右の変数取り違えが複数回発生。`#ifdef _DEBUG`のスペルミス(`_DEBGU`)でデバッグ専用メンバーの宣言が常にコンパイル対象外になっていたことも発覚(エディタのグレーアウトはIntelliSenseの誤表示でなく実際にプリプロセッサ条件が不成立というサインのことがある)。

**多段ヒット防止(`DamageSource`)の設計、まだ未完成。次回はここから再開:**
- 発端: 既存の岩・ワームの多段ヒット防止(`m_isTakingDamage`という単一boolフラグ)は、「同じフレーム内で複数の異なる攻撃源(岩とワーム等)に同時に当たると、先に判定された方が優先され、後の攻撃が無視される」という欠陥があると気づいた。ビーム追加でこの状況(複数種の攻撃に同時接触)が現実的になったため対応することに。
- 要件を「同じ攻撃源からの多段ヒットだけ防げればよい(異なる攻撃源は独立して判定してよい)」に決定。`bool`1個ではなく`enum class DamageSource { Rock, Worm, Beam }`＋`std::set<DamageSource>`で攻撃源ごとに独立管理する方針に転換。
- **試した設計とその問題点**:
  1. 「当たったら`insert`、離れたら`OnLeaveDamaging`で`erase`」→ `OnLeaveDamaging`をどこで呼ぶか(ループごとに`isHit`相当のフラグが要る)が面倒、かつ実装時に`if`の条件を逆にする等のミスが頻発。
  2. 「`Update()`の最初で毎フレーム`ClearTakingDamage()`」→ シンプルだが、次のフレームで記録が消えてしまうため「連続で当たり続けても1回だけ」という無敵時間の要件そのものと矛盾し、結果的に多段ヒットしてしまう。実装時、`StartTakingDamage`を`if(HitCollision)`の外に置いてしまい「毎回無条件でset に追加される」バグ(岩でダメージが一切通らなくなる)も発生。
  3. 「前フレームの記録」と「今フレームの記録」を2つの`set`で持ち、判定は前フレーム分を見る案 → 理屈は正しいが「フレームの切り替え(今の記録を前の記録としてコピーし今の記録を空にする)をどこで行うか」が分かりにくく、複雑と判断。
  - **結論: 方式1(`insert`/`OnLeaveDamaging`)に戻ることに決定。** ただし各判定ループの外側に`isHitRock`/`isHitWorm`/`isHitBeam`のようなその場限りのフラグを用意し、ループの外で`if (!isHitXxx) OnLeaveDamaging(...)`を呼ぶ、という以前の岩・ワーム実装と同じパターンで統一する。
- **現状(2026-08-18時点、未実装)**: `Player.h`/`.cpp`が`ClearTakingDamage()`方式のまま(`OnLeaveDamaging`が無い状態)。`CollisionManager.cpp`側もまだ`ClearTakingDamage()`呼び出し・`StartTakingDamage`のみの状態。**次回は`Player`に`OnLeaveDamaging(DamageSource)`を復活させ、`CollisionManager::Update()`の岩・ワーム・ビーム各ループに`isHitXxx`フラグを追加してから作業を再開すること。**

**次回やること（旧、下記の続き参照）:**
1. ~~多段ヒット防止の実装~~ → **見送りに決定（2026-08-18）**。ダメージを1に固定して多段ヒットを許容する方針に変更、上記タスク5参照。
2. `BossBeamState`の当たり判定球配列(`m_beamSpheresL/R`)がビーム中増え続ける一方で削除処理がない点、必要なら対応を検討。
3. リプレイ時のEffekseerエフェクト残留の対応(上記タスク7)。
4. ボスの死亡演出はまだ未着手。

## 進捗（2026-08-19・ボスの本配置着手：登場条件・登場演出・カメラの設計）

**ボスをCSV固定配置(`Data/CSV/BossEnemyData.csv`、現状z=3000)から、実際のゲームフローに組み込む作業に着手。**

**合意した演出仕様:**
- 登場条件: プレイヤーが特定のZ座標に到達したらボス演出開始（シンプルな`if (playerPos.m_z > boss_trigger_z)`判定で十分、専用の「イベントマネージャー」的な仕組みは今回1回限りの用途のため過剰と判断し不採用）。
- 演出内容: ①ボスが上空からドスンと落下してくる（カメラを2秒ほど揺らす）②着地の瞬間にもう一度カメラを揺らす③ボスにズームする④ズーム解除して通常プレイ開始。この間プレイヤーは一切操作不能にする。

**設計方針（決定事項）:**
- 演出の管理場所は`GameScene`自体が持つ（専用の別クラスは作らず、`GameScene`にステート/フラグを持たせる方針）。
- 「プレイヤー操作不能」は、既存の`Player`の4種のステートマシン（Movement/Rotation/Shoot/SpecialAction）を全部「無効化ステート」に切り替えることで実現する方針。
  - `Movement`: `DisabledMovementState`が既に骨格のみ存在（空実装）、そのまま使える。
  - `Rotation`: `DisabledRotState`も既に骨格のみ存在、そのまま使える。
  - `SpecialAction`: 既存の`NoneState`（何もしないステート）がそのまま無効化用として使える。
  - `Shoot`: 対応する「何もしない」ステートがまだ存在しない。`IShootState`を継承する新規`DisabledShootState`(仮称)をユーザーが作成予定（`Movement`/`Rotation`の既存実装と同じパターンで実装できる見込み）。

**次回やること（旧、下記の続き参照）:**
1. ~~`DisabledShootState`の新規作成。~~ → 対応済み（下記参照）。
2. ~~`Player`に「4ステート全部を無効化ステートに切り替える」ための処理。~~ → `DisabledAllState()`として実装済み。
3. `GameScene`に登場演出のステート/フラグと、「①落下+カメラ揺れ→②着地+カメラ揺れ→③ズーム→④解除」の順序を管理する仕組み → 落下・着地・カメラズームまで実装完了（下記参照）。カメラズーム解除〜通常プレイ復帰(TODO部分)は次回。
4. ~~ボスの「上空から落下してくる」動き~~ → `SetVel`での重力実装まで完了、下記のバグ修正を経て動作確認済み。
5. 多段ヒット防止見送りに伴うダメージ値固定の反映（まだ未確認）。

## 進捗（2026-08-19続き・ボス落下バグ修正、カメラズーム機能の実装完了）

**バグ(解決済み): `GameScene`側で`SetVel`により重力(Y=-900)をセットしても、直後の`BossEnemy::Update()`冒頭にある「プレイヤーと同じ速度で移動する」処理(`myVel.m_y = 0.0f`のあと`SetVel(myVel)`で上書き)が毎フレーム上書きしてしまい、ボスが全く落下しなかった。**
- 平時の「プレイヤー追従移動」の仕様と、「登場演出中は重力で落下させたい」という要求が同じ`SetVel`を取り合う構造的な問題。`GameScene`が`m_bossApearState`を持つ一方`BossEnemy`側はそれを知らない非対称な設計だった。
- 対処済み（詳細な実装方法はユーザーが解決、記録時点でロジックの中身は未確認・次回確認するとよい）。

**カメラズーム機能(`CameraBase::OnZoomUp`)を新規実装、完成。**
- 設計方針: カメラの視野角(`fov`)は`constexpr`定数で変更不可のため、「FOVを狭める」方式ではなく「カメラの位置自体をズーム対象に近づける」方式(`m_pos`をターゲット方向へ`Lerp`で近づける)を採用。
- **詰まった点1**: `Update()`が毎フレーム`UpdateTargetPos()`を無条件で呼んでおり、`OnZoomUp()`でせっかく`m_targetPos`をズーム対象(ボス)の位置にセットしても直後に上書きされ、ズームが機能しなかった。→ `UpdateTargetPos()`の呼び出しを「ズーム中でないとき」の`else`ブロックに移動して解決。
- **詰まった点2**: 当初`m_zoomFrame`(カウントダウン式のフレーム数)でズーム終了を判定していたが、「この形だと`m_zoomFrame`は位置計算に一切使われておらず、終了タイミングを決めるだけの役割で、`m_zoomSpeed`(呼び出し側が指定したはずの値)も使われていない」と気づき設計を見直した。**最終的に`m_zoomFrame`を廃止し、「`m_pos`とズーム目標地点(`m_targetPos - zoom_limit`)の距離が閾値(`zoom_dist_thresould`=100.0f)未満になったらズーム終了」という距離ベースの判定に変更**。`m_zoomSpeed`自体を`0.0f`にすることで「ズーム中かどうか」のフラグ代わりに使う設計にした(`if (m_zoomSpeed > 0.0f)`で判定)。
- `zoom_limit`(`Vector3(0,0,3000)`、ズーム対象からZ軸方向にどれだけ手前で止まるかの固定オフセット)を導入し、対象にめり込まず一定距離を保って寄る動きにした。
- **詰まった点3(ケアレスミス)**: `.cpp`側は上記の設計変更を先に反映していたが、`.h`側の`OnZoomUp`宣言に旧引数`int zoomFrame`が残ったままになっており、定義とシグネチャが不一致(リンクエラー相当)の状態で「見てほしい」と提示される場面が続いた。原因は単に保存し忘れていただけだった。**教訓: `.h`/`.cpp`両方に跨る設計変更をした際は、保存も含めて両ファイルが実際に一致しているか確認すること。**
- 動作確認済み、意図通りボスへ寄っていくカメラワークが完成。

**2026-08-21、コードを確認して完了済みタスクを削除しました。** カメラズーム解除→プレイヤー操作復帰（`GameScene.cpp`の`BossApearState::CameraZoom`で`OnZoomUp`/`SetIsBossAppear(true)`/`ChangeAllStateToNormal()`まで実装済み）、落下バグ修正（`BossEnemy::Update()`のプレイヤー追従上書きが`if (m_isAppear)`で正しくガードされている）、ダメージ値固定（岩・ワーム・ビームとも`1`に統一済み）はすべて完了と確認。

**次回やること:**
1. ~~ボスの死亡演出~~ → 下記「進捗（2026-08-21続き）」の通り着手・作業中。

## 進捗（2026-08-21続き・ボスの死亡エフェクト完成、HPGaugeUIも分割、タイトルシーンに着手）

**ボスの死亡エフェクトが完成した。** 作業の過程でHPゲージUIも`PlayerHPGaugeUI`/`BossHPGaugeUI`/共通基底`HPGaugeUIBase`に分割されている（旧`HPGaugeUI.h/.cpp`は削除済み、`BossHPGaugeUI`は新規）。詳細な実装内容は未記録だが、ボス関連の機能（雑魚召喚・ビーム・登場演出・死亡演出・専用HPゲージ）が一通り揃った状態。

**まだ未コミットの変更が多数残っている状態**（`BossDeath`エフェクト調整、`CameraBase`、`ResourceLoader`、`GlitchPS.hlsl`、`GraphShaderDraw`、新規`ResourceConstants.h`など）。次にコミットするタイミングで内容を整理すること。

**次のタスク: タイトルシーンの完成に着手。** `TitleScene.cpp`にも既に変更が入っている状態からのスタート。詳細はこれから設計・実装。

## 進捗（2026-08-24・PlayerHPGaugeUIのデバッグ周期変更ボタン修正、BossHPGaugeUIの縦方向対応）

**バグ1(解決済み): グリッチのスキャンライン周期をQ/Eキーで上下できるようにしたデバッグ機能が、どちらのキーを押しても反応しなかった。**
- 原因は`InputManager.h`の`InputEvent`名前空間で、`upScanlineFrequency`の文字列リテラルが`"upScanlineFrequency"`ではなく**コピペミスで`"downScanlineFrequency"`のまま**になっていたこと(`downScanlineFrequency`と文字列が重複)。結果、`m_inputTable`上で同じキーに対して2回代入することになり、後勝ちで上げる方(Q)の登録自体が消えていた。
- 修正後も「どちらも反応しない」現象が続いたため、Debug/Releaseどちらの構成でビルド・実行しているか(`_DEBUG`マクロの有無でボタン処理・表示ごと丸ごとコンパイル対象外になる)等、切り分けの観点を提示。最終的にユーザー側で解決。
- **教訓**: `constexpr const char*`で複数の定数を並べて定義するとき、コピペ後の中身(文字列リテラル)を変え忘れるミスは、コンパイルエラーにならず「片方が無反応/意図しない方に反応する」という静かな不具合になる。同じパターンが複数並ぶ定義は要注意。

**バグ2(解決済み): `HPGaugeUIBase::DrawHPGauge`を横専用(`PlayerHPGaugeUI`)からボス(縦型ゲージ)にも流用しようとして、右から減る動きになってしまった問題。**
- 対応として`DrawGraphToShader.h/.cpp`に`DrawRectHorizontalGraphToShader`(既存)と対になる`DrawRectVerticalGraphToShader`(新規)を追加、`HPGaugeUIBase::DrawHPGauge`に`isBoss`引数を追加して縦/横を切り替える設計にした。
- **要件**: ボスのゲージは「下端固定、上から削れていく」動きにすることで合意。
- **最初の実装ミス**: 位置計算で`top + (1.0f - size.m_height * uvMaxV)`のように、ピクセル単位の変数(`top`/`size.m_height`)に対して`1.0f`(=1ピクセル分)を足す形になっており、NDC座標系(-1〜1)前提の発想の式がピクセル座標の実装に紛れ込んでいた。UV側も`0.0f * uvMaxV`のように常に0になる式で、実質何も動いていなかった。
- **正しい式(ユーザーが自力で導出)**: 位置は上端2頂点のyを`top + size.m_height * (1.0f - uvMaxV)`(下端`top + size.m_height`は固定)、UVは上端2頂点のvを`1.0f - uvMaxV`(下端`v=1.0f`は固定)。`uvMaxV`(HP割合、満タンで1.0)を検算すると、満タンで上端が`top`まで伸び(v=0まで含む)、HP0で上端が下端に潰れる(v=1のみ)動きになることを確認した。
- **教訓**: 横→縦のように「削れる方向」を反転させる場合、位置とUVの両方を独立に見直す必要がある。片方だけ直すと「位置は動くのに絵が動かない(またはその逆)」というズレた壊れ方をする。水平版の式(`left + size.m_width * uvMaxU`)を土台に、固定したい端を軸にして`(1.0f - 割合)`を掛けるかどうかを検算(満タン/0のときの値を代入)して確認する進め方が有効だった。

## 完成済み機能の一覧（詳細はGit履歴・コード参照）

- 透過水（岩などの水中物体が境界からふわっと透ける表現）
- 岩の配置CSV化・当たり判定（球判定）
- 岩のニアクリップディゾルブ（カメラに近づいたオブジェクトがノイズ状に消える）
- 浮遊敵・ワームエネミーのCSV化・死亡演出
- タイトル画面のワイプ演出
- プレイヤー弾・チャージショットのEffekseerエフェクト一式
- スターフォックス風カメラ（視錐台クランプ、位置追従の動的フレーミング）
- コースティクス（水底の光の模様）
- UIの電子風スキャンラインシェーダー(`GlitchPS.hlsl`、`DrawGraphToShader`経由)
- HPゲージUI
- ダメージ時のプレイヤー発光・赤み演出(`DamagePS.hlsl`)、カメラ揺れ
- BulletManagerの死亡弾クリーンアップ
- BossEnemyのスキニング描画・アニメーション・脚の海判定

## 進捗（2026-08-27・ロックオン方式の再設計、合意まで完了・実装はこれから）

**チャージショットのロックオン（`TargetManager`のフォーカス機能）を、距離ベース→前方＋画面内＋スティッキー方式に変える設計で合意した。実装はまだ着手していない（学校から帰宅のためノート記入まで）。**

### 現状の実装の把握（変更前）

- **`TargetManager`** … `GameScene::Update`から**毎フレーム常時**`Update`されている。shootステートとは無関係に、常に「奥レティクル位置（`reticle_distance = 1800`、プレイヤー前方ベクトル基準）から半径`focus_range = 700`以内で最も近い敵1体」を`m_pFocusTarget`に入れ続ける。プレイヤー入力は一切関与しない。対象は`FloatingEnemy`と`WormEnemy`のみ（`BossEnemy`は未登録）。
- **`ReticleUI::Draw`** … `IsFocus() && m_pPlayer->IsChargeReady()`のとき、フォーカス対象の位置にチャージレティクル（回転＋スケールアニメ、アルファフェード）を描画。
- **`ChargeReadyState::Update`** … チャージ弾を撃つ瞬間に`m_pPlayer->GetFocusTarget()`を読み、`BulletManager::CreateBullet`へ`pTarget`として渡す。以降`ChargeBullet::Update`がその対象へ`Lerp(t=0.8)`でホーミング。
- shootステートの流れ: `NormalShootState`（shoot長押し10F）→ `ChargeShootState`（チャージ中、`m_chargeFrame`が`charge_comp_frame = 20`で完了扱い）→ ボタン離しで完了なら`ChargeReadyState`（発射待機、`can_shoot_frame = 60`F以内に再トリガーで`ChargeBullet`発射）→ 撃つ/時間切れ/エフェクト縮小で`NormalShootState`へ戻る。`Player::IsChargeReady()`は`ChargeShootState`か`ChargeReadyState`のとき`true`。
- `Player::IsFocus()` / `GetFocusTarget()` は`TargetManager`へ委譲しているだけ。

### 変更前の問題点

- 距離判定のため、**こちらに向かってくるワームエネミー**などが半径700の球から出るとロックが外れてしまう。
- 距離だけで判定するので、**画面外（横）にいて近い敵**も構わずロックしてしまう。

### 合意した新仕様

- **ロックの開始タイミング**: `ChargeShootState`に**入った時点**（`Enter`）でロック対象の探索を開始する。
- **探索中（未確定の間）**: 毎フレーム、以下を**すべて**満たす敵の中から**レティクル位置に最も近い1体**を候補にする。
  - プレイヤーより前方（`Dot(playerForward, playerToEnemy) > 0`）。※プレイヤーモデルは逆向きのため実際に使うのは`-GetForward()`側。既存`TargetManager`のレティクル計算が`-pPlayer->GetForward()`を使っているのと合わせる。
  - **画面内に映っている**。判定は`ConvWorldPosToScreenPos`の結果が `0 <= x <= 画面幅` かつ `0 <= y <= 画面高さ` かつ `z < 1.0`（手前＝視錐台の前方）。**マージンなし・ぴったり**でよい。
  - 候補が1体でも見つかった瞬間、それを**ロック対象として確定**（最初にロックされた敵で確定。以降は選び直さない）。
- **確定後の解除条件**（いずれか）:
  1. チャージ弾を撃った（`ChargeReadyState`から`ChargeBullet`発射）。
  2. 撃たずにチャージ／チャージ待機を終えて`NormalShootState`に戻った。
  3. 対象が後方に回った（`Dot <= 0`）。
  4. 対象が**画面外に出た**（上記スクリーン座標判定を外れた、または背後）。← 今回追加した条件。
  5. 対象が死んだ。
  - 解除後、まだ`ChargeShootState` / `ChargeReadyState`にいるなら再び探索に戻る。
- **切り替えは無し**（ロック中に別の敵へ乗り換える機能は作らない。最近傍で困るのは照準を向け直す直感で対応できる、とのユーザー判断）。

### 実装方針（合意事項）

- **ロック状態のライフサイクルは`TargetManager`に持たせる（案a）。**
  - `TargetManager`に `BeginLock()` / `EndLock()` と `IsLocking`（探索中も含めた「ロック機能ON」状態）を持たせる想定。
  - `ChargeShootState::Enter` で `BeginLock()` を呼ぶ。`NormalShootState::Enter`（またはチャージ系ステートの`Exit`）で `EndLock()` を呼ぶ。
  - `TargetManager::Update` は、ロック機能ONのときだけ「探索 → 確定 → 保持 → 解除判定」ロジックを走らせる。OFFのときは`m_pFocusTarget`を空にしておく。
  - `ChargeShootState` / `ChargeReadyState` はステート遷移のたびに作り直されるため、ロック対象を跨いで保持する主体はこの2ステートには置けない。`TargetManager`に置く。
- **`ReticleUI::Draw` の描画条件から `&& m_pPlayer.lock()->IsChargeReady()` を削除**し、`if (pTargetManager->IsFocus())` だけにする。新仕様では`IsFocus()`が`true`になるのは`ChargeShootState`以降でロックが取れているときだけなので`IsChargeReady()`は冗長。見た目の挙動は変わらない。

### 実装時に注意すべき点（未着手）

- `TargetManager`は`FloatingEnemy` / `WormEnemy`の2配列を別々にループしている。新しい候補条件（前方＋画面内）も両方のループに同じように入れる必要がある。
- 「前方」判定の基準点はプレイヤー位置。既存のレティクル位置計算（`-GetForward() * reticle_distance`）と符号の向きを揃えること。
- `ConvWorldPosToScreenPos`は対象が視錐台の背後にあると座標に極端な負値を返す。`z < 1.0`のチェックで背後を弾く。
- 死亡済みweak_ptrの`erase`処理（既存の`std::remove_if`パターン）はそのまま維持。
- ロック対象が死亡・画面外・後方で外れたとき、`m_isFocus`も`false`に戻すこと（`ReticleUI`がこれを見ている）。

## 進捗（2026-08-27〜28・プレイヤー効果音の一括実装、多段ヒット防止の再設計に着手）

**プレイヤー関連の効果音（NormalShoot/ChargeShoot/ChargeComplete/Charging/Boost/Brake/Somersoult/PlayerDamage/PlayerDeath）をClaudeが直接実装した。** `ResourceLoader`（`FontID`と同じ並びの`SoundID`）・`SoundManager`（フェード機能付き、`InitData`でハンドル/音量/ループ設定をまとめる設計）は既にユーザーが完成させていたので、今回は「各ステートから鳴らす」組み込み部分のみ担当。同じパターンの繰り返し作業のため、ユーザーの依頼で例外的にClaudeが直接編集した（学習目的の「直接編集しない」ルールの例外扱い）。

- **設計の紆余曲折**: 最初は`Player`に`PlaySound`/`StopSound`という薄いラッパーを作り、各ステートは`m_pPlayer.lock()->PlaySound(...)`で呼ぶ方式で実装したが、「Playerが音を鳴らす関数を持つのは違和感がある」という指摘を受け、既存の`BulletManager`と全く同じパターン（各ステートのコンストラクタに直接`std::weak_ptr<SoundManager>`を渡す）に設計し直した。
  - `IShootState`基底に`SoundManager`を追加 → `NormalShootState`/`ChargeShootState`/`ChargeReadyState`/`DisabledShootState`に一括で伝播。
  - `GaugeActionStateBase`（Boost/Brakeの共通基底）に追加 → 両方に伝播（`IdleMovementState`等の無関係なステートは変更せず、共通の性質を持つものだけをまとめた）。
  - `SomersaultState`は単独でコンストラクタに追加（`NoneState`は無関係なので変更なし）。
  - `Player::TakeDamage`だけは`Player`自身が`m_pSoundManager.lock()->Play(...)`を直接呼ぶ（自分自身の状態変化を音にする処理なので違和感がない、という整理）。
  - `GameScene`が`SoundManager`を生成・`Init()`・毎フレーム`Update()`する主体になった（`BulletManager`と同じ立ち位置）。
- **バグ修正1**: チャージ完了音(`ChargeComplete`)がチャージショット発射後も鳴り続ける → `ChargeShootState::Exit()`で明示的に`Stop`するよう修正。
- **バグ修正2**: ブースト音(`Boost`)がブースト終了後も鳴り続ける → `BoostState::Exit()`で明示的に`Stop`するよう修正（`Brake`も同じ構造なので同様の対応が必要になる可能性がある、未確認）。
- **`SoundManager::Play`に`isOnce`引数を追加**（`Play(SoundType, bool loop = false, bool isOnce = false)`）。「既に再生中なら重ねて鳴らさない」を、ループ音専用だった既存ガードとは別に、単発音にも適用できるようにした。`Player::TakeDamage`の`PlayerDamage`再生に`isOnce = true`を指定し、1フレーム内の多段ヒットで音が重複しないようにした。
  - **判明した副作用**: `isOnce`は「時間的に重なっていたら無視する」仕組みのため、本来別々に鳴ってほしい離れたタイミングのダメージ音まで、音声ファイルの再生時間が長いと巻き添えで消えてしまう。ユーザーが「普通に複数回被弾したときに音が鳴らないのは気持ち悪い」と気づき、この場しのぎでは不十分と判断。
- **結論: 多段ヒット防止（ダメージ自体の重複防止）を本格的に実装する方針に転換。** NOTES.md記載の過去の設計案（2026-08-11〜18、`DamageSource` enum + `std::set`、攻撃源ごとに独立管理）を土台に、「攻撃源の種類」だけでなく「攻撃源の個体」まで区別する設計に発展させた。

### 新しい多段ヒット防止の設計（実装中）

- **対象とする攻撃源**: `Rock`（岩）・`WormEnemy`（頭+胴体セグメントは同一個体ならまとめて1つ扱う）・`BossBeam`（左右2本のビームは同一ボス個体なのでまとめて1つ扱う）の3種類。`EnemyBullet`（敵弾）は命中した瞬間に消滅し多段ヒットが構造上起こらないため対象外。
- **個体の識別方法**: `GameObject`に一意なID（`m_id`、`GetID()`）を追加。コンストラクタで`static int s_nextId`をインクリメントしながら払い出す方式（**この部分は既にユーザーが自力で実装済み**、`GameObject.h/.cpp`確認済み）。
- **`DamageSource`構造体**: 新規ファイル`Game/GameObjects/Actors/Charactor/DamageSource.h`をユーザーが作成（ソリューションエクスプローラー経由）。中身は`enum class DamageSourceType { Rock, Worm, Beam }`と、`type`+`id`を持つ`DamageSource`構造体、`std::set`で使うための`operator<`（まず`type`で比較し、同じ`type`なら`id`で比較する2段階比較）。**この`operator<`の実装意図をユーザーに解説済み、DamageSource.hへの実装はこれから。**
- **API配置場所**: 多段ヒット防止のAPI（`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`想定）は、`Player`単体ではなく`Charactor`基底クラスに置く方針（将来敵側の多段ヒット防止にも使い回せるように、という判断）。

**次回やること:**
1. `DamageSource.h`の中身を実装する（enum + struct + operator<）。
2. `Charactor`に`std::set<DamageSource> m_takingDamageSources`と`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`を追加する。
3. `CollisionManager::Update()`のRock/WormEnemy/BossBeamの3箇所のループに、ループ外の`isHitXxx`フラグ＋ループ内での`IsTakingDamageFrom`確認＋ループ後の`OnLeaveDamaging`呼び出しを組み込む（NOTES.md過去の教訓：`OnLeaveDamaging`をどこで呼ぶかが以前つまずいたポイントなので注意）。
4. `Brake`もBoostと同じ「Exit()での音の停止」が必要か確認する。
5. リプレイ等でEffekseerエフェクトが残留する問題（旧タスク8）はまだ未着手のまま。

## 進捗（ClearSceneのワイプ/カーテン演出バグ修正完了、GameoverScene新規実装、EnemyBaseポリモーフィズム化に着手）

**`ClearScene`の選択肢ワイプ演出（未完成のまま残っていた件）を修正・完成させた。**

- `Draw()`の`switch`文を、`TitleScene`と同じ構造（通常画像は常に描画、選ばれている方だけカーソルオン画像を重ね描き）に直した。以前は選ばれていない方が`if (m_wipeProgress[...] > 0.0f)`の中に入っていて、ワイプが0に戻った瞬間に一瞬両方消える不具合があったが、「選ばれていない方は`if`の外」に出して解決。
- `switch`文より前にあった通常画像の無条件描画（開く演出中の分）と、`switch`内の`openProgress != 1.0f`分岐が同じ内容を二重描画していた問題は、外側の無条件描画を削除して`switch`内の`if (openProgress != 1.0f) {...} else {...}`構造に統一して解決。開く演出中は通常画像2つがuvMaxU/uvMinUで描画され、開ききった後に初めて選ばれている方だけワイプ演出が始まる。
- **「次へ」ボタン（`InputEvent::next`）と「決定」（`InputEvent::ok`）が両方`KEY_INPUT_A`に割り当てられており、「次へ」を押した瞬間に`ok`も同時反応してそのままシーン遷移してしまうバグを発見・修正。** `m_backGroundOpenFrame >= background_opne_max_frame`（選択肢背景が開ききっている）という条件を`ok`判定に追加するガードで解決。
- **重大な発見: `DrawGraphToShaderByCenter`のワイプ演出が「左端固定で右に伸びる」つもりで、実際には常に「中心から左右に開く」動きになっていたバグ。** `leftTopPos`の計算式`centerX - texSizeSizeF.m_width * (uvMinU + uvMaxU) / 2`が、`uvMaxU`の値によって左端自体を動かしてしまっていたのが原因（`TitleScene`のワイプ演出も同じ問題を抱えていたと判明）。`leftTopPos.m_x = centerX - texSizeSizeF.m_width / 2`（常にフル表示時の左端で固定）に修正し、`uvMinU`を0のまま`uvMaxU`だけ動かせば左固定で右に伸びるワイプに、中心対称に動かせば中心から開く演出に、同じ関数のまま両立できるようにした（`GraphShaderDraw.cpp`）。

**`GameoverScene`を新規実装（Claudeが直接編集、ユーザー許可あり）。** `ClearScene`の選択肢背景の開く演出＋ワイプ演出のロジックをそのまま移植。数値表示・フォント・光彩・テンプレート画像（`ClearScene`固有の要素）は含めず、「選択肢背景を出す→リトライ/ゲーム終了の選択肢をワイプで切り替える」だけのシンプルな構成。選択肢画像は`ReTry`/`ReTryOnCursor`（リトライ）と`GameEnd`/`GameEndOnCursor`（`TitleScene`用に既にあったものを流用）。

- **ハマった点: 新規作成した`.cpp`/`.h`がBOM無しUTF-8で保存され、Visual Studioが日本語コメントをShift-JISと誤認識して大量の構文エラーが発生。** `ClearScene.cpp`など既存ファイルはBOM付きUTF-8（先頭バイト`EF BB BF`）だったのに対し、新規ファイルはBOM無しだった。PowerShellの`[System.IO.File]::WriteAllText`＋`New-Object System.Text.UTF8Encoding $true`でBOM付きに保存し直して解決。**教訓: 新規.cpp/.hファイルを作成する際は、既存ファイルとバイト列レベルで同じ形式（BOM付きUTF-8）になっているか確認すること。**

**`EnemyBase`ポリモーフィズム化に着手（`TargetManager`から）。**

- 現状の把握: `GameScene`/`TargetManager`/`CollisionManager`の3箇所すべてが、`FloatingEnemy`/`WormEnemy`を型別の`vector`＋型別`RegisterXxx`関数で個別管理しており、`EnemyBase`という共通基底を作った意味が活きていないと判明。
- `TargetManager`は`GetPos()`（`Actor`由来の共通メソッド）しか使っておらず、型固有ロジックが一切ないため、`std::vector<std::weak_ptr<EnemyBase>>`一本＋`RegisterEnemy(std::shared_ptr<EnemyBase>)`一つにまとめるだけでほぼ完全にポリモーフィズム化できると判断（weak→shared変換ループ、「レティクルに一番近い敵を探す」ループ、`erase`+`remove_if`のクリーンアップが軒並み半分になる）。
- `CollisionManager`は`FloatingEnemy::GetSphere()`（単一球）と`WormEnemy::GetHeadSphere()`+`GetSegmentSpheres()`（頭+胴体複数）で当たり判定の形が非対称なため、単純な一本化はできない。`EnemyBase`に`virtual std::vector<Sphere> GetCollisionSpheres() const`のような仮想関数を用意すれば揃えられる見込みだが、まだ設計段階（未着手）。プレイヤー本体との接触ダメージ処理は`WormEnemy`にしかない機能なので、無理に共通化すべきでない可能性もある。
- `EnemyFactory::Create()`の戻り値は既に`std::shared_ptr<EnemyBase>`になっているため、`TargetManager`/`CollisionManager`側を直せば`EnemyFactory.cpp`の`RegisterFloatingEnemy(pFloating)`/`RegisterWormEnemy(pWorm)`も`RegisterEnemy(pFloating)`/`RegisterEnemy(pWorm)`に統一するだけで済む（暗黙の派生→基底変換）。

**次回やること（旧、下記の続き参照）:**
1. ~~`TargetManager.h/.cpp`を`std::vector<std::weak_ptr<EnemyBase>>`一本化。~~ → 完了（下記参照）。
2. ~~`EnemyFactory.cpp`の`RegisterFloatingEnemy`/`RegisterWormEnemy`呼び出しを統一。~~ → 完了（`Register`という名前に統一）。
3. ~~`CollisionManager`のポリモーフィズム化。~~ → 完了（下記参照）。
4. ~~`GameScene.h`の型別`vector`も統一。~~ → 完了（下記参照）。
5. 上記の多段ヒット防止の実装（`DamageSource.h`〜）も引き続き未着手。

## 進捗（EnemyBaseポリモーフィズム化 完了、ロックオン仕様変更に着手・実装途中）

**`EnemyBase`ポリモーフィズム化が全箇所完了した。**

- **`TargetManager`**: `m_pFloatingEnemies`/`m_pWormEnemies`の2配列・`RegisterFloatingEnemy`/`RegisterWormEnemy`の2関数を、`std::vector<std::weak_ptr<EnemyBase>> m_pEnemies`＋`Register(std::shared_ptr<EnemyBase>)`の1本に統合。書き換え時、`m_reticlePos`/`m_frontReticlePos`の更新、`focus_range`判定と`m_isFocus`/`m_pFocusTarget`の反映処理が**丸ごと抜け落ちるミスがあった**（ユーザーが自力で発見・修正）。**教訓**: 型別の重複コードを1本化する際は、削除・統合の過程で元の処理が漏れていないか、書き換え後に元コードと突き合わせて確認する必要がある。
- **`CollisionManager`**: `EnemyBase`に`virtual std::vector<Sphere> GetCollisionSpheres() const { return {}; }`（純粋仮想ではなくデフォルト空配列、`BossEnemy`は未対応のままでよい方針）を追加。`FloatingEnemy`は`{ m_colSphere }`、`WormEnemy`は頭+胴体をまとめた配列を返すoverrideを実装。`CollisionManager`側は「敵とプレイヤー弾の当たり判定」を`FloatingEnemy`用・`WormEnemy`用の2ループから1本の共通ループに統合。「プレイヤー本体とワームの接触ダメージ」（`WormEnemy`固有機能）は、共通の`pSharedEnemies`から`std::dynamic_pointer_cast<WormEnemy>`で絞り込む形で存置。
  - ハマった点: `Register`関数の定義に`CollisionManager::`を付け忘れ、`CollisionManager`と無関係なフリー関数になっていた（`m_pEnemies`がスコープ外でコンパイルエラーになるはずのミス）。ユーザーが自力で発見・修正。
- **`FloatingEnemyDataSetter`/`WormEnemyDataSetter`**: `CreateEnemy`の戻り値型を`std::vector<std::shared_ptr<EnemyBase>>`に統一。
- **`GameScene`**: `m_pFloatingEnemies`/`m_pWormEnemies`を`std::vector<std::shared_ptr<EnemyBase>> m_pEnemies`に統合（`GameScene::Init()`で`Register`呼び出し後、生存保持のため`m_pEnemies.push_back(pEnemy)`する形。この最後の一連はユーザーの依頼でClaudeが直接編集）。
- ビルド確認済み、エラーなし。

## 進捗（ロックオン仕様変更・実装中、TargetManagerの新ロジックほぼ完成）

**NOTES.md記載の合意済み設計（2026-08-27、前方＋画面内＋スティッキー方式）の実装に着手。**

**`TargetManager`の新ロジック（実装済み、動作未確認）:**
- `BeginLock()`/`EndLock()`/`m_isLocking`を追加。`m_isFocus`（今まさに1体ロックできているか、表示用）とは別に、`m_isLocking`（探索処理を回すべきタイミングかどうかのスイッチ）を分けて持つ設計にした理由をユーザーに説明・納得済み（「ロック中だが対象未確定」を表現するために両方必要）。
- `Update()`: ロックOFF時はフォーカスなしで即終了。ロックON時、既にロック対象が確定していれば「前方判定(内積>0)・画面内判定・生存」の3条件を毎フレームチェックし、どれか外れたら解除。未確定なら、同じ3条件（前方・画面内・生存、ただし内積は`> 0`ではなく`<= 0`でcontinueする形）を満たす敵の中からレティクルに最も近い1体を候補として確定する。
- `IsOnScreen`（新規private関数）: `ConvWorldPosToScreenPos`でスクリーン座標に変換し、x/y範囲チェック(ウィンドウサイズ内)とz<1.0チェック(視錐台の手前)で画面内判定。
- `Vector3::Dot`（静的関数）を新規追加（既存の`Vector3`クラスに内積計算が無かったため）。
- **実装中に見つかった3つのバグ（すべてユーザーが自力で修正済み）**:
  1. `IsOnScreen`のY判定が`wsize.m_width`と比較していた（コピペミス、`wsize.m_height`が正しい）。
  2. 候補探索ループが、宣言しただけで一度も要素を追加していない空の`vector`をforeachしており、**ループが1回も実行されず永遠に候補が見つからないバグ**。本来回すべきは`m_pEnemies`。
  3. 「一番近い候補を`m_pFocusTarget`に確定する」処理がforループの内側にあり、毎回そのときまでの最近傍で上書きしていた（結果は収束するが意図とズレる）。ループの外に移動して解決。
- 未使用の空`vector`宣言の残骸1行がまだ残っている（軽微、実害なし）。

**実装完了（下記参照）:**
1. ~~`IShootState`に`std::weak_ptr<TargetManager>`を持たせる。~~ → 完了。`BulletManager`/`SoundManager`と同じパターンでコンストラクタ引数・メンバ追加。
2. ~~4派生クラス全てのコンストラクタ引数追加、`ChangeState`呼び出し箇所への引き渡し。~~ → 完了。
3. ~~`ChargeShootState::Enter()`に`BeginLock()`、`NormalShootState::Enter()`に`EndLock()`。~~ → 完了。
4. ~~`ReticleUI::Draw()`の描画条件修正。~~ → 完了。`&& m_pPlayer.lock()->IsChargeReady()`を削除し`if (pTargetManager->IsFocus())`のみに変更。
5. `TargetManager.cpp`の未使用`vector`宣言の削除（軽微、未対応のまま）。
6. 多段ヒット防止の実装（`DamageSource.h`〜）は引き続き未着手。

## 進捗（ロックオン仕様変更・実装完了、ただし実機確認でロックオンが機能しない不具合を発見）

**`IShootState`系への`TargetManager`引き渡しを完了させ、ロックオン仕様変更の実装が一通り完了した。**

- `IShootState.h/.cpp`にコンストラクタ引数・メンバ`m_pTargetManager`を追加。
- `ChargeReadyState`/`ChargeShootState`/`DisabledShootState`/`NormalShootState`の4派生クラス全てのコンストラクタに`pTargetManager`引数を追加。
- **ハマった点（ユーザーが自力で発見・修正）**: `ChangeState(std::make_shared<Xxx>(...))`で次のステートを生成している5箇所（`NormalShootState.cpp`→`ChargeShootState`、`ChargeShootState.cpp`→`NormalShootState`×2/`ChargeReadyState`、`ChargeReadyState.cpp`→`NormalShootState`）全てで`m_pTargetManager`の引き渡しが最初漏れていた。さらに`Player.cpp`側で`NormalShootState`を直接生成している2箇所（コンストラクタ内の初期化、`ChangeAllStateToNormal()`相当の処理）でも同様の渡し忘れがあり、合計7箇所を1つずつ確認して直した。
- `ChargeShootState::Enter()`に`BeginLock()`、`NormalShootState::Enter()`に`EndLock()`の呼び出しを追加済み。
- `ReticleUI::Draw()`の描画条件を`pTargetManager->IsFocus() && m_pPlayer.lock()->IsChargeReady()`から`pTargetManager->IsFocus()`のみに変更（Claudeが直接編集、単純な1箇所削除のため）。
- ビルドは通った。

**未解決の不具合（次回、学校から帰宅後に調査再開）:**
- **実機で確認したところ、敵がロックオンされず、ロックオンレティクルUIも一切表示されなかった。** 原因はまだ未調査。疑うべき箇所の候補（次回の手がかり）:
  - `TargetManager::Update()`内、ロック確定後に`return`しているせいで`m_pFocusTarget`更新後の処理が正しく流れているか（`BeginLock()`が呼ばれてから実際に`Update()`が呼ばれるタイミングの前後関係）。
  - `IsOnScreen()`の判定がそもそも常にfalseになっていないか（`ConvWorldPosToScreenPos`の使い方、Z値の閾値`1.0`が実際のプロジェクション設定と合っているか）。
  - 前方判定の内積の符号（`-GetForward()`の向き、プレイヤーモデルが逆向きである既知の仕様との整合）。
  - `m_pEnemies`に敵が正しく`Register`されているか（`EnemyBase`ポリモーフィズム化後の登録経路に問題がないか）。
  - `BeginLock()`/`EndLock()`が実際に呼ばれているか（`IShootState`系のコンストラクタ引数渡しに、まだ見落としている箇所がないか）。

## 進捗（2026-08-28・ロックオン不具合の原因判明・解決）

**「ロックオンが機能しない」不具合の原因が判明し、解決した。**

- **原因**: `GameScene::Init()`で`TargetManager`の生成＆`Player::SetTargetManager()`が、`m_pPlayer->Init()`（内部で`OnInit()`が走り最初の`NormalShootState`が生成される）より**後**に呼ばれていた。`Player`の各ステート（`NormalShootState`等）はコンストラクタで`TargetManager`の`weak_ptr`を値として受け取る設計（`BulletManager`と同じパターン）のため、生成時点でまだセットされていない`m_pTargetManager`（空のweak_ptr）をそのままコピーして持ってしまっていた。以後`ChangeState`で新しいステートに遷移するたびに、この「空のまま」の`m_pTargetManager`がずっと引き継がれ続け、`ChargeShootState::Enter()`の`BeginLock()`が`m_pTargetManager.lock() == nullptr`で何もしないまま終わっていた。
- **切り分け方法**: `ChargeShootState`の`BeginLock()`呼び出し箇所にブレークポイントを置き、`m_pTargetManager.lock()`が`nullptr`になっていることをユーザーが自力で発見。そこから「`NormalShootState`が生成される`Player::OnInit()`の時点で、`TargetManager`はまだ存在するか」を`GameScene::Init()`の順序で確認する形で遡って特定した。
- **対処**: `GameScene::Init()`内で、`TargetManager`の生成＋`SetTargetManager`呼び出しを、`m_pPlayer = std::make_shared<Player>(...)`の直後・`m_pPlayer->Init()`の直前に移動。`TargetManager`のコンストラクタは`std::weak_ptr<Player>`しか要求しないため、`Player`の`shared_ptr`さえ確定していれば`Init()`より前に安全に生成できる。
- **教訓**: `Player`のように「コンストラクタで受け取った依存先をそのままステートへ値渡しする」設計では、依存先の`shared_ptr`/`SetXxx`が`Init()`（＝内部で最初のステートが生成されるタイミング）より前に揃っている必要がある。`SetXxx`のような「後から追加でセットする」形の依存は、初期化順序次第でこの種の「最初の1回だけ空を掴む」バグを生みやすい。

**ロックオン仕様変更（前方＋画面内＋スティッキー方式）、これで一通り完成・動作確認済み。**

**次回やること:**
1. `TargetManager.cpp`の未使用`vector`宣言の削除（軽微、未対応のまま）。
2. 多段ヒット防止の実装（`DamageSource.h`〜、`Charactor`基底へのAPI追加、`CollisionManager`への組み込み）は引き続き未着手。
3. `Brake`もBoostと同じ「Exit()での音の停止」が必要か確認する（未確認のまま）。
4. リプレイ等でEffekseerエフェクトが残留する問題（旧タスク8）はまだ未着手のまま。

## 進捗（2026-08-28続き・多段ヒット防止の完成、効果音・BGM全般の実装）

**多段ヒット防止(`DamageSource`)を完成させた。** 前回の設計（`DamageSourceType` enum + `id`のペア、`std::set`で管理）通りに実装。

- `DamageSource.h`（新規、ユーザー作成）: `enum class DamageSourceType { Rock, Worm, Beam }`と、`type`+`id`を持つ`DamageSource`構造体、`std::set`用の`operator<`（`type`→`id`の2段階比較）。`operator<`の実装意図（`std::set`が要素の重複判定・整列に使う裏方の仕組みであり、ゲームロジック上の優先順位とは無関係であること）をユーザーに詳しく解説し、正しく理解した上で自力で実装。
- `Charactor`基底に`std::set<DamageSource> m_damageSources`と`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`を追加（Player単体ではなく基底に置き、将来敵側でも使えるようにする方針）。
- `CollisionManager::Update()`のRock/WormEnemy/BossBeam(左右共通の1つのDamageSourceとして扱う)の3箇所に、「ループ外でヒットフラグとDamageSourceを用意→ループ内で`IsTakingDamageFrom`確認しつつダメージ処理→ループ後`isHitXxx`を見て`OnLeaveDamaging`」というパターンを適用。全てユーザーが自力で実装し、一発で正しく動作。

**プレイヤー効果音の仕上げ**: `SoundManager::Play`に`isOnce`引数を追加した際の副作用（時間的に重複した別々の被弾を巻き添えで消してしまう）を、多段ヒット防止の完成によって根本的に解消（`isOnce`はもう使わなくてよくなった）。ブースト音がフェードアウト中に鳴らし直すと音量が下がったままになる問題は、`Play()`内で`fadeState`リセット＋`ChangeVolumeSoundMem`で音量を明示的に戻す処理を追加して解決。

**ワームエネミーが画面外(far)に置き去りになったら消える処理を追加。** `CameraBase`に`GetFarClip()`ゲッターを新設（既存のprivate定数`camera_far`を公開）。`WormEnemy::Update()`の`!m_isDying`ブロック先頭で、カメラとの距離が`GetFarClip()`を超えたら`OnEnemyDead()`。`FloatingEnemy`側は既存の時間切れ方式（`LeaveState`突入から4秒）のままで良いとユーザー判断、変更せず。

**ボス・浮遊敵・ワームエネミーの効果音を一通り実装（Claudeが直接編集）:**
- ボス: 着地音(`BossMove`、着地時+一定間隔の足音)、ビーム発射音(`BossBeam`)、雑魚召喚音(`BossSummon`)、無敵シールド被弾音(`BossRecovery`)、被弾音(`BossDamage`)、死亡音(`BossDeath`、`m_isDying`になってから90F遅延)、出現前の地震音(`BossQuake`、揺れステートを抜けたらフェードアウト)。`BossEnemy`に`SoundManager`を追加し`IBossEnemyState`系は`m_pBoss`経由で`GetSoundManager()`アクセス。
- 浮遊敵: activeになった瞬間の音(`EnemyBoot`)、弾発射音(`EnemyShoot`、ワームと共通)。
- ワームエネミー: 弾発射音(`EnemyShoot`共通)、死亡時の爆発音(`EnemyDeath`、浮遊敵と共通)。**「爆発エフェクトと同時に、頭→胴体と段階的にダンダンダンと鳴ってほしい」**という要望を受け、`TakeDamage()`での即時1回再生ではなく、`Update()`内の段階的死亡エフェクト再生ループ(`death_effect_interval`ごと)に音を移動。移動音(`WormMove`ループ)は一度実装したが「思った以上に合わなかった」とのことで撤去（`OnInit()`のループ開始・`TakeDamage()`の停止呼び出しを削除、`SoundType`自体はリソースとして残存）。
- `EnemyFactory`（ボスの雑魚召喚経由の生成）が`SoundManager`受け渡し修正から漏れており、C2661/C2672のビルドエラーが発生→`EnemyFactory.h/.cpp`にも`SoundManager`を追加して解決。

**BGM実装一式:**
- タイトルBGMをロゴ演出終了時ではなく`Init()`から最初に鳴らすよう変更。
- ゲームBGM(`GameBGM`)・ボスBGM(`BossBGM`)を追加。`GameScene::Init()`でゲームBGM開始。`GameCamera`に`IsZoom()`ゲッターを新設（`m_zoomSpeed > 0.0f`を公開）、ボス出現時のカメラズームが完了した瞬間(`m_isApearBoss && !m_isChangedToBossBGM && !IsZoom()`)にゲームBGM→ボスBGMへ切り替え。
- ボスが死亡待機状態になった瞬間、一度だけボスBGMをフェードアウトする処理を実装。**ハマった点**: 同じ処理を実装したつもりが、フラグガード付き(371-376行目)とガードなし(379-385行目、後から重複して追加)の2箇所が併存し、ガードなしの方が`m_pBoss->IsDying()`の間毎フレーム`FadeOut`を呼び直し`fadeTimer`をリセットし続けるため、音量が途中までしか下がらない不具合が発生。ガードなしの重複ブロックを削除して解決。**教訓**: 「毎フレーム条件を満たし続ける処理を1回だけ実行したい」という要件は、必ず「もう実行したか」を覚えるフラグ（または状態の立ち上がり検出）で防御する必要がある。同じ処理を複数箇所に書いてしまうと、片方にガードがあっても意味がない。
- リザルトBGM(`ResultBGM`)、カーテン演出音(`DataAppear`、テンプレートが開き始める瞬間)、スコア加算音(`ScoreCount`、Lerp更新中に5F間隔のクールタイムを設けて連打を防止)、次へボタン音を`ClearScene`に実装。

**決定音・選択音の整理と統合:**
- `ClearScene`/`GameoverScene`に決定音(`Decision`)・選択音(`OnCursor`)が未実装だったため追加（`TitleScene`は実装済みだった）。
- 「リザルトで決定したときの音が2つある」という指摘を受け調査した結果、`TitleScene`/`ClearScene`/`GameoverScene`共通の`Decision`という音と、`ClearScene`の「次へボタン」用`NextButton`という**別々の音**が両方存在しており紛らわしかったため、**`NextButton`の音（`Data/Sounds/Result/NextButton.mp3`）を正式な決定音として採用し、`Decision`という古い音を削除して統合する**方針に転換。
  - ユーザーがVSのソリューションエクスプローラーで`NextButton.mp3`を`Decision.mp3`にリネーム・移動、旧`Decision.mp3`を削除。
  - コード側は`ResourceConstants.h`/`ResourceLoader.h/.cpp`/`SoundManager.h/.cpp`から`SoundID::NextButton`/`SoundType::NextButton`関連を全て削除し、`ClearScene.cpp`の次へボタン音の呼び出しを`Decision`に変更。
  - ついでに既存コードの音量定数の取り違えバグ（`OnCursor`の登録に`decision_volume`、`Decision`の登録に`on_cursor_volume`という名前と用途が逆転していた実害のないミス）も発見・整理。

**次回やること:**
1. `TargetManager.cpp`の未使用`vector`宣言の削除（軽微、まだ未対応）。
2. `Brake`もBoostと同じ「Exit()での音の停止」が必要か確認する（未確認のまま）。
3. リプレイ等でEffekseerエフェクトが残留する問題（旧タスク8）はまだ未着手のまま。
4. 効果音・BGM実装は一区切り。全体を通しでプレイして音量バランス・タイミングを最終確認するとよい。

## 参考: コスト表について

進捗管理はリポジトリ直下の`NovaWing_詳細.xlsx`が本体（Teams側は参照しない）。Claudeは`.xlsx`を直接読めないため、PowerShellのExcel COMオブジェクト経由で読み書きする（Excelで開いたままだとロックされるため閉じてもらう）。コミットはユーザー自身が行う。

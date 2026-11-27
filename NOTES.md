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
- `.hlsl`を新しく作ったら、vcxprojの`FxCompile`に登録しないと`.pso`/`.vso`ができず、`LoadPixelShader`が-1を返す。**UTF-8のBOM付きだとfxcが`X3000: Illegal character`(1行1文字目)で止まる**ので、BOM無しで保存する。

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
- **メンバを前方宣言だけで持つには`std::unique_ptr`**（値で持つとサイズ計算に完全な定義が必要で.hに`#include`が要る）。ただし生成(`make_unique`)・`->`での呼び出し・破棄（デストラクタ）には完全な定義が要るので、`#include`は.cppに移す。デストラクタは必ず.cppで定義する（.hで`= default`にすると不完全型エラー）。
- `unique_ptr`はコピーできず、戻り値にすると所有権ごと渡すことになる。外に使わせるだけなら`*ptr`で中身の参照を返して「貸す」。`weak_ptr`だけで持つと所有者がいなくて即破棄される。`shared_ptr`は共有する相手がいないなら不要で、オーナーへの参照を持つオブジェクトが持ち主より長生きする危険もある。
- 前方宣言しかしていないクラスは、基底クラスへの変換（`PlayerCollider&`→`ICollider&`、ポインタも同じ）ができない。継承関係はそのクラスのヘッダを読むまでわからないため。
- 値を返す関数・代入のつもりで`x - y;`や`obj->Func();`と書いて`return`や`=`を忘れるミスが今日2回あった（`m_rotationZ - X;`、`IsRolling()`の`return`漏れ）。コンパイルが通ってしまうので、警告C4715（値を返さないパス）やC4552（式の結果が使われていない）に注意する。

## 未解決・保留中のタスク

**2026-08-08、コードを確認して完了済みのタスクを削除しました**（BossEnemyのボーン行列コード整理・model_scale復元、Splashエフェクトの空白対応、CSV化はすべて確認済み・完了）。

**2026-10-15、再度コードと照らして完了済みを削除しました**（1: FloatingEnemyの暗さは素材側で対応済み、5: ボス本実装、7: TitleCamera実装、8: Effekseer残留、11: EffectManager新設(10/04)。番号は他の記述から参照されているため、残りの番号は振り直していない）。あわせて、下の進捗ログ内の「次回やること」「次」「残り」のうち、完了済みのものも削除した（未完了だったものは12番に集約）。

2. **`near_clip`/`far_clip`が`CapturePS.hlsl`/`WaterPS.hlsl`の2ファイルに重複定義**されている（`LightingPS.hlsl`は既に対象外と確認）。共通`.hlsli`への切り出しが未着手。

3. **DataSetter群（Rock/FloatingEnemy/WormEnemy/BossEnemy）のCSV読み込み〜生成の骨格が重複**。テンプレート基底クラスかフリー関数での共通化が候補、未着手。
   - **`CSVData`基底クラス（`CSVData/CSVData.h`、`virtual void Conversion()`を持つ）が、どのDataSetterからも継承されず活用されていないことが判明（2026-08-18）。** `CSVDataLoader::LoadCSV`は常に`CSVData`そのもの（生の`std::wstring`配列の入れ物）を作って返すだけで、各`XxxDataSetter`（`RockDataSetter`等）はそこから`GetData()`で生配列を取り出し、列番号を直接指定して自前でパースしている。
   - **参考プロジェクト`C:\Users\SakamotoKou\Documents\GitHub\ProjectNeaR`で本来の使い方を確認済み。** `CSVDataLoader::LoadCSV`自体は同じ実装（常に`CSVData`を生成）だが、呼び出し側で`ActorData(std::shared_ptr<CSVData> data)`のように**`CSVData`を受け取るサブクラスのコンストラクタ**を用意し、そこで`Conversion()`相当の変換処理（列番号パース→意味のあるメンバへの変換）を行うパターンだった。`CSVDataLoader`自体の変更は不要で、サブクラス（例:`RockCSVData : public CSVData`）を新設し、今`RockDataSetter::CreateRock`に直書きされている列番号パースをそちらに移す形が本家に忠実な直し方。
   - **保留中のタスクとして記録のみ。着手はまだ先。**

4. **`LightingPS.hlsl`(共通化済み)と`WaterPS.hlsl`間で視線ベクトル・反射ベクトル・specular計算式が重複**。`WaterPS.hlsl`側だけまだ独自実装のまま、`.hlsli`共通化が候補、未着手。

6. **VS CodeのIntelliSenseで`DxLib::VECTOR`等の型が「不完全な型」表示になる問題、原因未特定のまま保留。** ビルド自体は正常なため実害なし。深追いは費用対効果が低いと判断済み。

9. **シングルトンクラスが増えすぎている件、いずれ集約用クラス(例: GameServices)への移行を検討したい（2026-09-28・ユーザー提起、未着手）。** 現状`Application`/`InputManager`/`ResourceLoader`/`DebugManager`等が個別に`GetInstance()`を持つ素朴なMeyer'sシングルトン。今回`DebugManager`を追加する際、影響範囲を絞るためいったん同じパターンで作ったが、根本対応としては全マネージャーを1箇所で保持・提供する集約クラスへの置き換えが候補。既存コードへの影響が大きいため別タスクとして着手予定。

10. **影の追加（2026-10-01・方針のみ決定、未着手）。** DxLibのシャドウマップ(`MakeShadowMap`/`ShadowMap_DrawSetup`等)を使う方針。
   - 影を落とす側: オブジェクト全部（プレイヤー・敵・ボス・岩）。受ける側: 理想は海面・ステージ・オブジェクト同士の全部。
   - 前提: 3D描画は全て独自シェーダー（LightingPS/VS・StagePS・WaterPS）なので、`SetUseShadowMap`だけでは影は出ない。受ける処理（シャドウマップを読んで暗くする）は自前のシェーダーに書く必要がある（参考: [Shader/MV1Default_PixelLighting_PS.hlsl](Shader/MV1Default_PixelLighting_PS.hlsl)の242〜307行目、`t8`〜`t10`・`g_ShadowMap`）。シャドウマップ作成中に独自シェーダーが有効のままだと、深度ではなく色が書き込まれる点にも注意。
   - 負荷: 主な増加はシャドウマップ作成で`DrawAll()`相当がもう1回増えること（今は水の透過キャプチャで2回→3回に）。受ける面の数はほぼ影響しない。ただし影の縁を何回も読んでぼかす場合は、画面を広く占める海面が一番重くなる。
   - 手間: ステージ(小) < オブジェクト同士(中、通常/スキニングのVS2本、シャドウアクネの調整) < 海面(大、MV1でないためシャドウマップ・行列が自動で渡らない可能性が高い、要確認)。
   - ステージはZ方向に長い(1200〜27000)ので、影を描く範囲はプレイヤー周辺に限定する必要がある。
   - 進める順の案: ステージかオブジェクト同士で動く形を作る → 海面は最後。

12. **進捗ログの「次回やること」から集約した、未完了の小さなタスク（2026-10-15時点でコード確認）**
   - `BrakeState::Exit()`でもブースト同様に音を止める必要があるか確認する(今は`EndUseGauge()`のみ。10/01に低優先として放置中)。
   - `TargetManager.cpp`の98行目付近、使っていない`std::vector<std::shared_ptr<EnemyBase>> pEnemies;`を削除する(軽微)。
   - 効果音・BGMの音量バランス・タイミングを、通しプレイで最終確認する。
   - プレイヤーの定数(23個)をCSV化するかは未決定。ProjectNeaR方式なら「種類ごとに変えたい値」だけが対象(参考: `C:\Users\Admin\Documents\GitHub\ProjectNeaR\source\Project\General\CSV\CharaStatusData.h/.cpp`)。

13. **見た目の改善タスク2件（2026-11-06・コスト表に追加済み、未着手）**
   - 敵・プレイヤーが見づらいのでアウトラインを付ける(コスト表「シェーダ」の下、3時間・S・アルファ)。
   - 1面ボスの頭(黄色い部分)を攻撃した時に赤く点滅させる。人から言われた要望(コスト表に新設した「ステージ1ボス」の下、1時間・S・アルファ)。

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


## 進捗（2026-08-21続き2・TitleCamera/TitlePlayerの基礎実装）

**`TitleCamera`と`TitlePlayer`の骨組み〜前進フェーズまで実装、ビルド確認済み。**

**設計の合意事項:**
- 演出のフェーズ切り替え（前進→宙返り→ブースト、カメラの追従→固定）は、`TitlePlayer`/`TitleCamera`が自律的に判断するのではなく、**`TitleScene`側から明示的に指示する**方式に統一（`TitleCamera::StopFollowing()`、`TitlePlayer::StartSomersault()`/`StartBoost()`のような外部公開メソッドで切り替える）。
- 宙返りの**動き自体**はゲームシーンの宙返り(`SomersaultState`)と同じ内容でよいが、実装はコピーせず一から書く（`Player`の複雑なステートマシンには依存しない）。

**`TitleCamera`（完了）:**
- `CameraBase`を継承。`m_pPlayer`は`std::shared_ptr<TitlePlayer>`で保持（`GameCamera`が`weak_ptr`だったのとは意図的に方針を変えた、`TitleCamera`が`TitlePlayer`を強参照で持つ設計）。
- `m_isFollowing`(初期値`true`)で追従状態を管理。`UpdatePosition()`は追従中のみ`m_targetPos`をプレイヤー位置に更新、`StopFollowing()`が呼ばれた後は`m_pos`/`m_targetPos`とも一切更新しない（＝最後に追従していた向きで固定される）方針。カメラ位置(`m_pos`)自体は追従中も固定のまま動かさない設計（合意済み）。

**`TitlePlayer`（前進・宙返り・描画まで完了、ブーストは未実装）:**
- `Actor`を直接継承（`Character`は継承しない）。フェーズ管理は`enum class Phase { Forward, Somersault, Boost }`。フェーズ切り替えは`TitleScene`から`StartSomersault()`/`StartBoost()`を呼ぶ（合意方針通り）。
- **ハマった点: `SetVel`で速度をセットしても、`Actor`には速度を位置に反映する処理が無く、何も動かなかった。** `m_pos += m_velocity`は`Character::Update()`に実装されている処理だが、`TitlePlayer`は`Character`を継承していないため自動的には効かない。`TitlePlayer::Update()`内に同じ処理を自前で追加して解決。
- **ハマった点2: 位置反映(`m_pos += m_velocity`)と速度計算(`SetVel`)の順序を最初逆にしてしまい、1フレーム遅れの状態になっていた。** `BossEnemy::Update()`を参考に「先に`SetVel`で今フレームの速度を決めてから、後で`Character::Update()`相当の位置反映をする」正しい順序に修正。
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

**多段ヒット防止(`DamageSource`)の設計の経緯（→2026-08-28に完成）:**
- 発端: 既存の岩・ワームの多段ヒット防止(`m_isTakingDamage`という単一boolフラグ)は、「同じフレーム内で複数の異なる攻撃源(岩とワーム等)に同時に当たると、先に判定された方が優先され、後の攻撃が無視される」という欠陥があると気づいた。ビーム追加でこの状況(複数種の攻撃に同時接触)が現実的になったため対応することに。
- 要件を「同じ攻撃源からの多段ヒットだけ防げればよい(異なる攻撃源は独立して判定してよい)」に決定。`bool`1個ではなく`enum class DamageSource { Rock, Worm, Beam }`＋`std::set<DamageSource>`で攻撃源ごとに独立管理する方針に転換。
- **試した設計とその問題点**:
  1. 「当たったら`insert`、離れたら`OnLeaveDamaging`で`erase`」→ `OnLeaveDamaging`をどこで呼ぶか(ループごとに`isHit`相当のフラグが要る)が面倒、かつ実装時に`if`の条件を逆にする等のミスが頻発。
  2. 「`Update()`の最初で毎フレーム`ClearTakingDamage()`」→ シンプルだが、次のフレームで記録が消えてしまうため「連続で当たり続けても1回だけ」という無敵時間の要件そのものと矛盾し、結果的に多段ヒットしてしまう。実装時、`StartTakingDamage`を`if(HitCollision)`の外に置いてしまい「毎回無条件でset に追加される」バグ(岩でダメージが一切通らなくなる)も発生。
  3. 「前フレームの記録」と「今フレームの記録」を2つの`set`で持ち、判定は前フレーム分を見る案 → 理屈は正しいが「フレームの切り替え(今の記録を前の記録としてコピーし今の記録を空にする)をどこで行うか」が分かりにくく、複雑と判断。
  - **結論: 方式1(`insert`/`OnLeaveDamaging`)に戻ることに決定。** ただし各判定ループの外側に`isHitRock`/`isHitWorm`/`isHitBeam`のようなその場限りのフラグを用意し、ループの外で`if (!isHitXxx) OnLeaveDamaging(...)`を呼ぶ、という以前の岩・ワーム実装と同じパターンで統一する。


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
- **`DamageSource`構造体**: 新規ファイル`Game/GameObjects/Actors/Character/DamageSource.h`をユーザーが作成（ソリューションエクスプローラー経由）。中身は`enum class DamageSourceType { Rock, Worm, Beam }`と、`type`+`id`を持つ`DamageSource`構造体、`std::set`で使うための`operator<`（まず`type`で比較し、同じ`type`なら`id`で比較する2段階比較）。**この`operator<`の実装意図をユーザーに解説済み、DamageSource.hへの実装はこれから。**
- **API配置場所**: 多段ヒット防止のAPI（`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`想定）は、`Player`単体ではなく`Character`基底クラスに置く方針（将来敵側の多段ヒット防止にも使い回せるように、という判断）。


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


## 進捗（2026-08-28・ロックオン不具合の原因判明・解決）

**「ロックオンが機能しない」不具合の原因が判明し、解決した。**

- **原因**: `GameScene::Init()`で`TargetManager`の生成＆`Player::SetTargetManager()`が、`m_pPlayer->Init()`（内部で`OnInit()`が走り最初の`NormalShootState`が生成される）より**後**に呼ばれていた。`Player`の各ステート（`NormalShootState`等）はコンストラクタで`TargetManager`の`weak_ptr`を値として受け取る設計（`BulletManager`と同じパターン）のため、生成時点でまだセットされていない`m_pTargetManager`（空のweak_ptr）をそのままコピーして持ってしまっていた。以後`ChangeState`で新しいステートに遷移するたびに、この「空のまま」の`m_pTargetManager`がずっと引き継がれ続け、`ChargeShootState::Enter()`の`BeginLock()`が`m_pTargetManager.lock() == nullptr`で何もしないまま終わっていた。
- **切り分け方法**: `ChargeShootState`の`BeginLock()`呼び出し箇所にブレークポイントを置き、`m_pTargetManager.lock()`が`nullptr`になっていることをユーザーが自力で発見。そこから「`NormalShootState`が生成される`Player::OnInit()`の時点で、`TargetManager`はまだ存在するか」を`GameScene::Init()`の順序で確認する形で遡って特定した。
- **対処**: `GameScene::Init()`内で、`TargetManager`の生成＋`SetTargetManager`呼び出しを、`m_pPlayer = std::make_shared<Player>(...)`の直後・`m_pPlayer->Init()`の直前に移動。`TargetManager`のコンストラクタは`std::weak_ptr<Player>`しか要求しないため、`Player`の`shared_ptr`さえ確定していれば`Init()`より前に安全に生成できる。
- **教訓**: `Player`のように「コンストラクタで受け取った依存先をそのままステートへ値渡しする」設計では、依存先の`shared_ptr`/`SetXxx`が`Init()`（＝内部で最初のステートが生成されるタイミング）より前に揃っている必要がある。`SetXxx`のような「後から追加でセットする」形の依存は、初期化順序次第でこの種の「最初の1回だけ空を掴む」バグを生みやすい。

**ロックオン仕様変更（前方＋画面内＋スティッキー方式）、これで一通り完成・動作確認済み。**


## 進捗（2026-08-28続き・多段ヒット防止の完成、効果音・BGM全般の実装）

**多段ヒット防止(`DamageSource`)を完成させた。** 前回の設計（`DamageSourceType` enum + `id`のペア、`std::set`で管理）通りに実装。

- `DamageSource.h`（新規、ユーザー作成）: `enum class DamageSourceType { Rock, Worm, Beam }`と、`type`+`id`を持つ`DamageSource`構造体、`std::set`用の`operator<`（`type`→`id`の2段階比較）。`operator<`の実装意図（`std::set`が要素の重複判定・整列に使う裏方の仕組みであり、ゲームロジック上の優先順位とは無関係であること）をユーザーに詳しく解説し、正しく理解した上で自力で実装。
- `Character`基底に`std::set<DamageSource> m_damageSources`と`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`を追加（Player単体ではなく基底に置き、将来敵側でも使えるようにする方針）。
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


## 参考: コスト表について

進捗管理はリポジトリ直下の`NovaWing_詳細.xlsx`が本体（Teams側は参照しない）。Claudeは`.xlsx`を直接読めないため、PowerShellのExcel COMオブジェクト経由で読み書きする（Excelで開いたままだとロックされるため閉じてもらう）。コミットはユーザー自身が行う。

### 進捗（フルスクリーン化に伴うUI座標修正、流れ弾誤射バグ修正、ポーズシーン実装、操作説明UI着手）

**背景: 解像度を1280×720からフルスクリーン(1920×1080)に変更したところ、UIの位置・サイズが画面左上に小さく寄って見える不具合が発生。**

**原因:** UI要素の座標・サイズが「画面解像度に対する比率」ではなく「1280×720基準の固定ピクセル値」で書かれていたため。`TitleScene`は既に`wsize.m_width * ratio_x`という比率方式だったが、`PlayerHPGaugeUI`/`BossHPGaugeUI`（位置）、`HPGaugeUIBase`（サイズ）、`ClearScene`のリザルトテキスト4項目・ボタン類は固定値のままだった。

**対応方針:** 位置は「1280×720時点でどこに見えていたか」を比率(0〜1)に逆算して`_ratio`変数に置き換え、`wsize.m_width`/`wsize.m_height`を掛けて計算する形に統一。サイズ(スケール値)は理論値(×1.5)をベースに、実機で見た目を確認しながら微調整する方針で対応（`HPGaugeUIBase`の`hp_frame_size`/`hp_gauge_size`は理論値0.45では小さすぎたため、最終的に0.55に調整して確定）。

**対応済み:** `PlayerHPGaugeUI`/`BossHPGaugeUI`の位置、`HPGaugeUIBase`のサイズ、`ClearScene`のリザルトテキスト位置（通常描画・ぼかし描画の両方）、`TitleScene`/`GameoverScene`/`ClearScene`の選択肢・ボタン位置は全て比率化済みで確認済み。
**残タスク:** `TitleScene`/`GameoverScene`/`ClearScene`の各種`_scale`定数（`select_graph_scale`、`background_graph_scale`、`a_button_scale`等）はまだ画面解像度に応じた調整が済んでいないものが残っている可能性があるため、実機で見比べながら要確認。

### 進捗（FloatingEnemyの配置が実行のたびに消える問題→流れ弾による誤射と判明、Far距離での弾消去を実装）

**発端:** `FloatingEnemyData.csv`のZ=14000に配置した4体の敵が、実行するたびに何体か消えている現象が発覚。原因調査で`GameObjectManager`/`TargetManager`/`CollisionManager`等を広く調査したが、最終的にユーザー自身が「弾が奥まで飛んで敵に当たって死んでいるだけ」と気づいて解決。

**対応: 弾がカメラのFarクリップ距離を超えたら自動的に消える仕組みを実装。**
- `BulletBase`/`BulletManager`/`PlayerBullet`/`ChargeBullet`/`EnemyBullet`のコンストラクタ・`CreateBullet()`に`weak_ptr<CameraBase>`を追加で配線（当初`weak_ptr<GameCamera>`型で設計していたが、`BulletBase`が実際に使うのは`CameraBase`の`GetFarClip()`/`GetPos()`だけなので、途中で`CameraBase`型に統一する設計変更を行い、キャストの手間を削減）。
- `BulletBase::Update()`に「カメラ位置と弾位置の距離が`GetFarClip()`を超えたら`OnDead()`」という判定を追加。
- 呼び出し元5箇所（`NormalShootState`/`ChargeShootState`/`ChargeReadyState`/`WormEnemy`/`FloatingEnemy`の`ActiveState`）それぞれにカメラを渡す配線が必要になり、`Player`/`FloatingEnemy`に`GetCamera()`ゲッターを新設して対応。`WormEnemy`は`Actor`のprotectedな`m_pCamera`を直接使えるため配線不要だった。
- ユーザー自身が実装、Claudeはファイル確認とレビューに徹する形で進めた（[[feedback_no_direct_code_edit]]のルール通り）。

**残タスク:** 特になし、この機能は完成・動作確認済み。

### 進捗（Boostエフェクト追加、シーン切り替え時のEffekseerエフェクト残留バグ修正）

**Boostエフェクト（ブースト中に噴射エフェクトを出す）を新規実装、完成。**
- `ResourceLoader`に`EffectID::Boost`を追加、`Data/Effect/Boost/Boost.efk`をロード。
- `BoostState`に`Update()`をオーバーライドして追加する際、基底クラス`GaugeActionStateBase::Update()`の呼び出しを書き忘れ、ブースト中の移動・ゲージ消費処理が完全に失われてプレイヤーが動かなくなるバグが発生→`GaugeActionStateBase::Update()`を明示的に呼ぶ形で解決。
- エフェクトの位置をワールド座標の固定オフセット(`Vector3(0,0,-200)`加算)で計算していたため、プレイヤーが回転すると位置がズレる不具合が発生→過去の`ChargeReadyState`と同じ教訓（`GetVisualForward()`等、回転を反映した方向ベクトルを使う）で解決。
- エフェクトの**向き**をプレイヤーの回転に追従させたい要望があり、`SetRotationPlayingEffekseer3DEffect`（オイラー角X/Y/Zのみ、Quaternion非対応）を使用。`Player`が既に持つ`m_rotationX`/`m_rotationY`（オイラー角）をそのまま使えたため、Quaternion→オイラー角の変換は不要だった。`Player.h`に`GetRotationX()`/`GetRotationY()`ゲッターを新設。Y軸には`DX_PI_F`のオフセットが必要だった（プレイヤーモデルが逆向きに作られているため）。

**シーン切り替え（リトライ等）時にEffekseerエフェクトが再生されたまま残るバグを修正。**
- EffekseerForDXLibには「全エフェクトを一括停止する」関数が無いため、個別ハンドルのStopではなく、**根本原因（古いシーンのオブジェクトが破棄されるタイミングの問題）を直す方針**で対応。
- 原因: `GameObjectManager`はシングルトンで`ClearAll()`は新シーンの`Init()`内で呼ばれていたが、`SceneController::ResetScene()`で古いシーンの`shared_ptr`を`clear()`する際、`GameObjectManager`側がまだ古いオブジェクトへの参照を握ったままのため、古いオブジェクトのデストラクタ（`StopEffekseer3DEffect`を呼ぶ処理を含む）が正しいタイミングで呼ばれていなかった。
- 対処: `GameObjectManager::ClearAll()`の呼び出しを、新シーンの`GameScene::Init()`からではなく、`SceneController::ResetScene()`の`m_scenes.clear()`の**直前**に移動。順序を「古いオブジェクト全解放→新シーン生成」に修正して解決。

**残タスク:** 特になし、両機能とも完成。

### 進捗（ポーズシーン実装、操作説明UI着手・途中）

**`PauseScene`（`SceneController::PushScene`でゲームシーンに重ねる方式）を新規実装、「ゲームに戻る」「タイトルに戻る」の2択が完成。**
- `GameoverScene`の実装パターン（カーテンで開く背景演出＋ワイプでカーソルオン画像に切り替える演出）をそのまま踏襲する方針で、変数名の対応表を使って移植する形で進めた。
- 画像は`GraphicID::BackGame`/`BackGameOnCursor`（新規登録）、`BackTitle`/`BackTitleOnCursor`（既存流用）を使用。
- **ハマった点1**: `SceneController::PushScene()`が`Init()`を呼んでいなかったため、`PauseScene`のシェーダバッファ等が未初期化のままアクセスされ`nullptr`例外が発生。既存の`ResetScene`/`ChangeScene`が`SceneController`側で`Init()`を呼ぶ設計と一貫性を持たせるため、`PushScene()`にも`scene->Init()`を追加して解決。
- **ハマった点2**: `Select` enumに`HowToControll`という3番目の選択肢を用意していたが、`Draw()`の`switch(m_select)`に対応する`case`を書き忘れており、`HowToControll`を経由した際に選択肢画像が両方消える不具合が発生。最終的に`HowToControll`は使わない方針となり`Select` enumから削除、`BackGame`/`BackTitle`の2択に整理して解決。
- **ハマった点3**: `Bボタンでポーズを閉じる`機能(`InputEvent::close`)を追加した際、`if (input.IsTriggered(InputEvent::close))`のブロックを誤って`if (... && input.IsTriggered(InputEvent::ok))`の**内側**に書いてしまい、「OKボタンとBボタンを同時押ししたときだけ閉じる」という意図しない動作になっていた。外側の独立したif文に出して解決。

**→ 2026-10-02、操作説明UIは不要になり全削除（ユーザー依頼でClaudeが削除）:** `GraphicID::HowToPlay`、`how_to_play_path`、`KeepGraph`の読み込み、`GameScene.h`の`m_howToControllOpenProgress`、`GameScene.cpp`の`how_to_*`定数、画像`Data/Image/Button/HowTo.png`・`Data/Image/SelectFrame/How_To_Controll(_OnCursor).png`（ごみ箱へ）。

### 進捗（2026-09-19・当たり判定のICollider化、設計〜Player着手途中）

**目的: `CollisionManager::Update()`（300行超、Player/敵/岩/弾/ボスの組み合わせを個別にベタ書き）を、`ICollider`インターフェース経由の統一ループに書き換える。** 最初は「Actorに多重継承させる」案から出発したが、議論の末に方針転換。

**確定した設計（試行錯誤の結論、後で見返す用に理由も残す）:**
- **多重継承はしない。** ActorはGameObjectのみ継承したまま。当たり判定は各Actor派生クラスが専用の小さな内部クラス（例: `PlayerCollider`）を**コンポジションで保有**する形にした。理由: ProjectNeaR（参考プロジェクト、`C:\Users\Admin\Documents\GitHub\ProjectNeaR`）の`Collidable`/`ColliderBase`分離パターンを参考にしつつ、Actor本体への多重継承は避けたいというユーザー要望を優先した。
- **`ICollider`（`Game/Collision/ICollider.h`、完成済み）**: `GetCollision()`(shared_ptr<ColliderShape>を返す)、`GetTag()`、`OnCollision(const ICollider&)`、`IsCollisionActive()`の4つを持つ純粋仮想interface。`ColliderTag` enumもここに定義。
  - 試行錯誤の跡: 一度`Shape`を`enum`のまま返す案、`SphereData`構造体を直接持たせる案、`ShapeKind`+データの複合構造体案を経て、最終的に「Sphere固有のデータをICollider.hに持たせるのは違和感がある」というユーザーの指摘から、形状データは完全に別ファイル（`ColliderShape`/`SphereShape`）に分離する形に着地。
- **`ColliderShape`（`Game/Collision/ColliderShape.h`、完成済み）**: `Shape`(enum: 今はSphereのみ)と、デバッグ描画用の純粋仮想`Draw()`のみを持つ抽象基底。
- **`SphereShape`（`Game/Collision/SphereShape.h/.cpp`、完成済み）**: 既存の`Utility/Sphere`クラス（位置+半径+`HitCollision`+デバッグ`Draw`)と役割が完全に重複していたため、**`Sphere`を置き換える形でSphereShapeに位置・半径・`HitCollision`・`Draw`全てを統合**する方針にした。今後`Sphere`型を使っている全箇所（Player/Rock/EnemyBase系/Bullet系/CollisionManager、10ファイル以上）を`SphereShape`に順次置き換えていく必要がある。
- **OnCollisionの責務範囲**: 「自分のreaction（ダメージを受ける等）」のみを担当させ、カメラシェイクや多重ヒット防止(`DamageSource`関連)などゲーム進行に関わる処理は今まで通り`CollisionManager`側に残す。ダメージ量は`OnCollision`内で`other.GetTag()`を見てswitchし、弾の`GetAttackPower()`のような`ICollider`契約外の情報が要る場合は`other`を具体型へ`static_cast`することも許容する方針。
- **`DamageSource`による多重ヒット防止の仕組みは今回のスコープ外。** Player固有のメソッド（`IsTakingDamageFrom`等）としてそのまま残す。

**追加の設計判断（2026-09-19後半、Player実装〜ビルド確認で発覚）:**
- **Player.hが`m_collSphere`(旧`Sphere`型)と`m_collider`(PlayerCollider)を両方持つのは責務重複という指摘があり、`SphereShape`の所有権をPlayer本体からPlayerColliderに完全移動した。** `Player`は`PlayerCollider m_collider`のみを持ち、`Player::GetSphere()`/`Update()`内の球更新/`Draw()`内の球描画は全て`m_collider`経由（`m_collider.GetSphere()`、`m_collider.UpdateShape(pos, radius)`、`m_collider.GetSphere()->Draw(...)`）に委譲する形にした。「当たり判定に関する情報は全部PlayerColliderの中にある」という一貫性を優先。
- **`ColliderShape::Draw`を純粋仮想として追加、`SphereShape`がoverride。** 元々の`Utility/Sphere`が持っていたデバッグ描画機能(`DrawSphere3D`呼び出し)を`SphereShape`側に完全移植する方針にしたため。
- **`ICollider::GetCollision()`の戻り値を`std::shared_ptr<ColliderShape>`(単体)から`std::vector<std::shared_ptr<ColliderShape>>`(複数)に変更。** 理由: `Rock`は1つのActorが複数の当たり判定球(`std::vector<Sphere> m_spheres`)を持つ構造になっており、単体しか返せないインターフェースでは表現できないことが判明したため。Playerのような単一球のケースは`{ m_sphere }`のように1要素のvectorで返せばよい。

**ビルドで発覚したハマりどころ（今後の参考用）:**
- **`.vcxproj`/`.vcxproj.filters`に、過去に作って削除したはずの`Rigidbody`/`ColliderBase`/`Collidable`(.h/.cpp、ProjectNeaRを参考にした際の試行錯誤の残骸)への参照が残っており、ビルド時に`C1083: ソースファイルを開けません`エラーの原因になっていた。** Visual Studioのソリューションエクスプローラー上でファイルを削除したつもりでも、プロジェクトファイル側の参照だけ残ることがあるので、ファイルが実在しないのにビルドエラーで名前が出てきたら`.vcxproj`を直接grepして確認するとよい。今回は該当6項目をClaudeが`.vcxproj`/`.vcxproj.filters`から直接削除して解消（このファイルは.h/.cppではないため直接編集OKの対象）。
- **Playerだけを先にSphereShape化した段階で、`CollisionManager.cpp`内の`Sphere playerCol = pPlayer->GetSphere();`のような箇所が軒並み型不一致エラー(`C2440`)になった。** `CollisionManager`はPlayer/Rock/Bullet/Boss全部を`Sphere`型前提の直接比較で書いているため、1クラスだけ型を変えると即座に整合性が壊れる。→「Player→Rockの順で先に両方ICollider化してからCollisionManagerを直す」の元々の段取り通りに進めることで対応中。

**実装状況（2026-09-24、コードを直接確認して現況を棚卸し）:**

進んでいる部分と止まっている部分がはっきり分かれている。

- ✅ **完了**: `ICollider.h`/`ColliderShape.h`（vector化・`Draw`純粋仮想とも完成）。
- ✅ **完了**: `SphereShape.h/.cpp`（位置・半径・`Update`・`Draw`・`HitCollision`）。**`Sphere`型からの全面置き換えも完了済み**（`Utility/Sphere`はもう使われていない。Player/EnemyBase系/BulletBase系/BossEnemy/Rock全て`SphereShape`ベース）。
- ✅ **完了**: `PlayerCollider.h/.cpp`。`GetCollision()`はvector化済み（`return { m_sphere };`）、`Player`は`m_collider`経由に統一済み。
- ✅ **完了**: `RockCollider.h/.cpp`（新規作成済み）。`Rock`が`m_collider`として保持し、`AddSphere`で球を登録、`GetSpheres()`で取得する形まで実装・接続済み。
- ✅ **完了（当初スコープ外だった別件）**: 多段ヒット防止（`DamageSource`/`DamageSourceType`、`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`）が`CollisionManager.cpp`のRock/Worm/Beamループに完全に組み込まれている。
- ❌ **未着手**: `CollisionManager.cpp`本体は、`ICollider`経由の統一ループに**なっていない**。型は全部`SphereShape`に置き換わったが、`Update()`の中身は依然としてPlayer/EnemyBase/Rock/BossEnemyそれぞれに対する個別ベタ書きのまま（`GetCollisionSpheres()`/`GetSphere()`/`GetSpheres()`/`GetInvinsibleSphere()`等を直接呼んでいる）。`GetCollider()`/`ICollider::OnCollision`はどこからも呼ばれていない。
- ❌ **未着手**: `PlayerCollider::OnCollision`/`RockCollider::OnCollision`は中身が空のまま。
- ❌ **未着手**: `EnemyBase`系（`FloatingEnemy`/`WormEnemy`/`BossEnemy`）、`BulletBase`系（`PlayerBullet`/`EnemyBullet`/`ChargeBullet`）はどれも`ICollider`を実装していない（`SphereShape`型は使っているが、旧来の直接メソッド呼び出しパターンのまま）。

**つまり「Sphere→SphereShapeへの型置き換え」は全クラスで完了しているが、「CollisionManagerをICollider経由の統一ループに書き換える」という本来の目的自体はまだ手つかず。** 型だけ揃えて中身は個別ベタ書きのまま止まっている状態。

**2026-09-25、ICollider化をClaudeが実装（ユーザー依頼による直接編集の例外）。ビルド・実機確認は未実施。**
- 新規コライダー（`Game/Collision/`、BOM付きUTF-8・CRLF、`.vcxproj`/`.filters`にもClaudeが追記済み）:
  - `BulletCollider`（弾共通。タグはコンストラクタで`PlayerBullet`/`EnemyBullet`を渡す。`BulletBase`のコンストラクタに`ColliderTag`引数を追加）
  - `EnemyCollider`（浮遊敵=`Enemy`タグ、ワーム=`Worm`タグ。形状は`GetCollisionSpheres()`をそのまま使う）
  - ボスは反応ごとに分割: `BossDamageCollider` / `BossShieldCollider` / `BossBeamCollider`（ユーザー選択）。ボスの球は`OnInit`で作り直されるため、コライダーは球をコピーで持たず毎回ボスに問い合わせる。ビームは`BossBeamState`中のみ有効。
- `ColliderTag`に`Worm`/`BossDamage`/`BossShield`/`BossBeam`を追加。`EnemyBase`に`virtual std::vector<ICollider*> GetColliders()`を追加（ボスは3つ返す）。
- `PlayerCollider::OnCollision`にダメージ処理を実装（敵弾=弾の攻撃力、ビーム=ビームダメージ、ワーム/岩=固定20。定数は`CollisionManager`から移動）。`RockCollider`/`EnemyCollider`/`BossBeamCollider`に`GetOwnerID()`（多段ヒット防止の識別用）。
- `CollisionManager::Update()`: 全コライダーをタグ別に集め、`hit_pairs`表の組み合わせだけを上から順に判定→`OnHit`で双方の`OnCollision`を呼ぶ。カメラ揺れと多段ヒット防止は`CollisionManager::OnHit`に残した（方針通り）。多段ヒットの「離れた」判定は、前フレームと今フレームのヒット集合の差分で`OnLeaveDamaging`を呼ぶ方式に変更。
- **旧実装からの挙動の違い（意図したもの）**: ①弾は最初に当たった時点で消えるので、同じフレームに複数の敵・ワームの複数の球へ重複ダメージを与えなくなった。②死亡済みプレイヤーは被弾しない。③ビーム終了時にプレイヤーが触れていても「離れた」扱いになり、次のビームで正しくダメージが入る。
- `CounterCollider`は当初このタイミングでは未実装だったが、2026-09-25に実装・`.vcxproj`登録済み（下記セクション参照）。


### 進捗（RB/LB二回押しでバレルロール、左右とも実装済み）

**`DefaultRotationState`に「ローリングボタンを2回連続で押すと1回転する」動き（バレルロール）を実装中。既存の傾き操作（1回押しで機体を傾ける）と共存させる必要がある。**

**ここまでの経緯（完了済み）:** 2回押し判定のバグ3件、`LerpToAngleZ`の同フレーム二重呼び出しによる上書きバグは解決済み。さらに「`LerpToAngleZ`は最短距離補間のため360度指定では回転しない」と判明し、`Player::AddRotationZ(float delta)`（`m_rotationZ`に直接加算）を新設、`DefaultRotationState`側も`m_rollSumAngle`で累積回転量を管理し2πで終了する方式に書き換え済み（コード確認済み、`DefaultRotationState.cpp`/`Player.cpp`に反映済み）。

**現在の問題（2026-09-24発覚）:** 実機で「1周し終わる手前でゴムのように捻れてまた戻る」見た目になる。

**原因の訂正（2026-09-25）:** 以前ここに「Quaternionの二重被覆が原因」と書いていたが**誤り**。`Quaternion::ToMatrix4x4()`は全要素が成分の2次式なので`q`と`-q`は同一の行列になり、毎フレーム角度から作り直す今の方式では二重被覆による見た目の破綻は起きない（Playerの回転ではQuaternionのLerpも使っていない）。この誤診をもとに「差分クォータニオン積み重ね方式」「ロール中はX傾きを諦める案B（`m_isBarrelRolling`で`rotY*rotZ`に分岐）」を検討したが、どちらも不要。
- **本当の原因（2026-09-25、実機で解決確認済み）:** 1周終了時点で`m_rotationZ`が「開始角度+2π」のまま残り、次フレームから通常の`LerpToAngleZ(0 or ±π/2)`がその数値を戻そうとして**1周分を逆回転で巻き戻す**。Lerp(t=0.1)は最初速く後から遅いため、ゴムで戻るように見える。
- **教訓:** 角度を数値Lerpする設計では、周回運動のあと角度の数値が2π単位でずれたまま残ると、見た目は同じなのにLerpが巻き戻しを起こす。周回させたら値を範囲内（-π〜π等）に正規化する。

**解決（2026-09-25）:** `Player::AddRotationZ`に`if (m_rotationZ > DX_PI_F) m_rotationZ -= DX_TWO_PI_F;`を追加して-π〜πに収めた。案Bの分岐・フラグは削除し、`UpdateRotation()`は常に`rotX * rotY * rotZ`。右ローリングは巻き戻りなく動作確認済み。
- 途中でハマった点: 閾値を2πにすると、0から1周した値（ほぼ2π、float誤差でわずかに下回る）が条件を満たさず巻き戻りが残る。閾値は「半周（π）」にする必要がある。また`m_rotationZ - X;`と書いて代入し忘れ、値が変わらないミスもあった。

**左ローリングも実装済み（2026-09-25コード確認）:** `DefaultRotationState`に`m_rollDir`（右:1/左:-1/無:0）を追加して回転方向を記録、`AddRotationZ`にも`-π`を下回ったら2πを足す処理を追加済み。

### 進捗（2026-09-25・ローリング中に敵弾を防ぐカウンター判定）

**ローリング中だけ有効な`CounterCollider`を実装、`CollisionManager`に組み込み済み（ビルド・実機確認はまだ）。**
- `Player::IsRolling()`（public）が回転ステートを`DefaultRotationState`にキャストして`IsRolling()`を中継。`CounterCollider::IsCollisionActive()`はこれを返す。
- Playerはコライダーを**前方宣言＋`std::unique_ptr`**で持つ形に変更（`m_pHitCollider`/`m_pCounterCollider`、`#include`は`Player.cpp`側）。ユーザーの方針「.hに#includeを増やさない」による。`GetHitCollider()`/`GetCounterCollider()`は`ICollider&`を返し、本体は`Player.cpp`（`.h`では`PlayerCollider`が`ICollider`の子だとわからず変換できないため）。
- カウンター球は半径100（本体の判定は50）で、`Player::Update()`で本体と同じ位置に更新。
- `hit_pairs`に`{EnemyBullet, Counter}`を`{EnemyBullet, Player}`より**上**に追加。弾は当たると消えるので、ローリング中は本体に届かない。今は跳ね返さず、敵弾が消えるだけ（防御）。
- ハマった点: `GetCollider()`を`std::unique_ptr<ICollider>`で返そうとした（コピー不可＋所有権が移ってしまう）→参照で「貸す」形に。`.cpp`側の定義に`const`が残り宣言と不一致。`UpdateShape`の定義漏れ（LNK2019になるところ）。

**保留中のタスク（ユーザー判断で後回し）:**
2. **ビームの跳ね返し**: ビームは弾ではなくステートが作る判定球の列なので、本当に反射させると重い。現実的な案は「ローリング中はビームのダメージを無効化し、ボスに固定ダメージ（跳ね返した扱い）」。
3. `CollisionManager.cpp`の`hit_pairs`にある`{BossBeam, Counter}`は、今は双方の`OnCollision`が空でビームも消えないため何も起きない（ビームはそのまま本体に当たる）。2に着手するまでは不要。
4. `Player.h`の`GetCounterCollider()`のコメントが「カウンター用の球を取得」になっている（正しくはコライダー）。
5. 各クラスのコライダーのメンバ変数に付いているコメント「当たり判定インターフェース」が不正確（中身は`ICollider`を実装した具体クラスで、インターフェースそのものではない）。`Rock.h`/`BulletBase.h`/`FloatingEnemy.h`/`WormEnemy.h`/`BossEnemy.h`に残っている。`GetCollider()`側の「当たり判定インターフェースを取得」は戻り値が`ICollider&`なので正しい。

### 進捗（2026-09-27・敵弾の跳ね返し「ReflectedBullet」実装、完了）

**カウンターで敵弾を跳ね返すと、見た目は敵弾のまま発射元の敵に完全ホーミングする弾に変わる機能を実装・クリーンビルド確認済み（実機動作もユーザー確認済み、良好）。**

- 追従先は「撃ってきた敵」に決定。`BulletBase`ではなく`EnemyBullet`だけに`std::weak_ptr<EnemyBase> m_pShooter`を追加（`BulletBase`を汚さない設計、コンストラクタで受け取り`GetShooter()`で公開）。
- `WormEnemy`/`FloatingEnemy::ActiveState`の弾生成時に`std::static_pointer_cast<EnemyBase>(shared_from_this())`（または`pEnemy`）を発射元として渡すよう変更。`BulletManager::CreateBullet`の既存`pTarget`引数（元々ChargeBullet専用）を敵弾生成にも流用。
- `ReflectedBullet`（新規、`BulletBase`派生）: `ChargeBullet`と同じLerpホーミングだが`homingStrength`を外部から指定できる汎用クラス。タグは`PlayerBullet`（敵にダメージが通る）。コンストラクタ引数は`ReflectBulletData`という値型の構造体にまとめた（**参照メンバは危険なので値型にした教訓を検討済み**：一時オブジェクトを直接渡すと構造体生成時点でダングリング参照になるため）。
- `BulletManager`に`m_pReflectedBullets`配列と`CreateReflectedBullet()`を新設（既存`CreateBullet`とは別メソッド。理由: `ReflectedBullet`だけ`homingStrength`という他の弾にない情報を持つため、無理に共通引数に混ぜるとシグネチャが歪む）。
- `BulletCollider`に`GetOwner()`（`BulletBase&`を返す）を追加。
- `CollisionManager::OnHit`の冒頭で`{Counter, EnemyBullet}`の組み合わせを検知したら`ReflectEnemyBullet()`を呼んで`return`（二重`OnCollision`呼び出しのバグを`return`忘れで一度発生させ、指摘されて修正）。`ReflectEnemyBullet`内で`dynamic_cast<EnemyBullet*>`し、発射元・現在位置・元の弾速を使って反射弾を生成、`homingStrength = 1.0f`（完全追従）で`CreateReflectedBullet`を呼ぶ。
- **ユーザーの好み（今後のコードレビューでも意識する）**: 三項演算子や「参照変数への再代入に見える書き方」を避け、`if`/`else`とポインタで素直に書くことを好む。`bool`の条件式をワンライナーでまとめる書き方より、`if`/`else`で明示的に代入する書き方の方が読みやすいとのことで、後者を採用した。

**次回やること（ユーザー明言のタスク）:**
1. 反射弾（`ReflectedBullet`）にちょっとしたエフェクトを付けたい。現状は`ResourceLoader::EffectID::EnemyBullet`（見た目は敵弾のまま）を流用しているだけで、反射専用の見た目・エフェクトは未実装。

### 設計中（2026-09-28〜・ボスのビームの跳ね返し）

**決定した仕様:**
- 反射のきっかけは「**ビームの先端**（エフェクトの位置、`m_beamPosL/R`）がカウンター球に当たった瞬間」だけ。先端が届くタイミングに合わせてローリングする「パリィ」の遊び。先端が通り過ぎた後に残っている判定球に触れても反射しない。
- 反射の向きは、きちんと角度を反映する: `反射後 = v - 2 × Dot(v, n) × n`（`v`=ビームの進行方向、`n`=カウンター球の中心→当たった位置を正規化した法線）。式の意味はユーザー理解済み。
- その後、ボスへ少しだけ吸い付く。**1フレームに曲がれる角度に上限**をつける方式を採用（返し方が下手すぎると曲がりきれずに外れる。上限値だけで難易度調整できる）。`ReflectedBullet`/`ChargeBullet`のLerp追従は角度差に比例して強く曲がるので今回には向かない。**「直進」ではなく「毎フレーム、ボスへの方向に上限角度以内で少しずつ向きを寄せていく」緩やかなホーミング**が正しい理解（Claudeが一度「ボスへ直進」と誤った言い方をし、ユーザーに指摘されて訂正した経緯あり）。
- 反射後に作る判定球は「ボスに当たる」別の判定にする必要がある。今の球生成は「プレイヤーのZより手前まで」という条件なので、ボスへ戻る向きでは使えず、反射後用の処理が別に要る。

**未決定だった3点、決定済み(2026-09-28):**
1. 左右のビームは**別々に反射**（片方だけ反射も可）。
2. 反射後のビームがボスに当たったときのダメージは**大きめの固定値**、専用演出は無し、**既存の被弾エフェクト(`TakeDamage`時のもの)のみ流用**。
3. ボスの**ダメージ判定(`BossDamageCollider`/`BossDamage`タグ)に当てる**（無敵判定のシールドではない）。

**設計方針の検討・決定(2026-09-28):**
- 反射後のビーム弾の見た目は**ボスビーム自体(`BossBeam`)のエフェクトを流用**（新規エフェクト作成なし、方向をボス向きに変えるだけ）。
- 実装場所は「`BossBeamState`に反射状態を追加(A案)」か「反射専用の新規クラスを作る(B案)」かで比較検討。**A案に決定**。理由: ビームは先端移動・球の生成/削除・エフェクト追従という一連の複雑な状態を`BossBeamState`が既に持っており、新規クラスに複製すると二重管理になる(今後の速度/間隔/半径調整が2箇所に波及)。`ReflectedBullet`のような「速度+ホーミングだけの単純な弾」とは事情が違うと判断。
- 反射中の球を「ボスに当たる判定」として区別する方法は、「`BossBeamCollider`をL/R毎に分け、反射中は`GetTag()`が別タグ(`BossBeamReflected`等)を返す」方式に決定(新規コライダークラスは作らない)。
- **先端のカウンター判定は「10Fごとに生成される軌跡の球」とは別に、`m_beamPosL/R`に毎フレーム追従する専用の判定球(`m_tipSphereL/R`)を新設し、そちらで毎フレーム判定する**とユーザーが指定(軌跡の生成間隔に判定タイミングを合わせるのは却下)。



**追加決定(2026-09-30):** 先端用のコライダーは**クラス1つ（例: `BossBeamTipCollider`、タグ`BossBeamTip`）を左右で2個作る**。コンストラクタで左右を受け取り、自分がどちらの先端かを覚える。理由: 当たったコライダー自身が左右を知っているので`OnReflectLeft/Right`のどちらを呼ぶかがそのまま決まる（1つにまとめると当たった後に左右を判定し直す必要がある）。`IsCollisionActive()`は「ビーム中かつ自分の側がまだ反射していない」でtrue。

**コードの進み具合(2026-09-30確認):** `BossBeamState.h`に`m_isReflectedL/R`と`OnReflectLeft/Right(const Vector3& hitPos)`の宣言だけ入っている。残り: `OnReflectLeft/Right`を`public:`へ移す（`CollisionManager`から呼ぶため）、非デバッグの先端球`m_tipSphereL/R`（今ある`m_beamTipSphereL/R`は`#ifdef _DEBUG`内のデバッグ表示用）、`GetTipSphereL/R()`/`IsReflectedL/R()`、`.cpp`の定義すべて。

### 保留タスク（2026-09-27相談・未着手）: ほぼ全エフェクトが見づらい問題

**原因はユーザーが特定済み: エフェクトを加算合成(Additive Blend)で作っているため、背景が明るい/近い色だと確実に馴染んで埋もれる。** Effekseer側のブレンド設定だけではこの性質自体はどうにもならない（加算特有の「重なるほど輝く」表現を保ったまま視認性だけ上げたい）ので、シェーダー側での対処方法を相談された。

**提示した方向性（まだどれで進めるか未決定、実装着手前）:**
1. エフェクト用のレンダーターゲットを分離し、最終合成時にアウトライン/コントラスト強化を後処理として掛ける（要望の「アウトライン」に一番近い）。
2. 加算前に、エフェクトの明るさに応じて背景色を一時的に暗くする(にじみ防止のダークニング)。
3. Effekseerのノード側でフレネル的な縁光効果を追加する(シェーダー改修なしで近い効果を狙える可能性、要検証)。

**次回再開時にやること:** どの方向性で進めるかユーザーと相談してから着手する。現状の描画パイプライン（オフスクリーン構成、Effekseer3D描画のタイミングなど）を先に洗い出す必要がある。

### 進捗（2026-09-23〜24・Debug/ReleaseでUIの大きさが異なるバグを修正、完了）

**報告された症状: 「タイトルでのUIの大きさが、Debug/Releaseで違って、リリース時のサイズでデバッグ時に表示される」**

**根本原因の特定:**
- `Constants/Game.h`で`screen_width`/`screen_height`が`#ifdef _DEBUG`により**Debug=1280x720、Release=1920x1080**と、解像度自体が異なる値で定義されていた（`Application.cpp`の`ChangeWindowMode`もDebugはウィンドウモード、Releaseはフルスクリーンで対応する意図的な分岐）。
- UI画像の描画（`DrawRotaGraph`/`DrawGraphToShaderByCenter`/`GaugeUIBase::DrawGauge`等）は、**画像自体のピクセルサイズ**(`GetGraphSize`で取得)に定数の`scale`を掛けているだけで、解像度の情報を一切考慮していなかった。位置(`wsize.width * 比率`)は解像度に応じて変わるのに、大きさ自体は解像度に連動しないため、Debug(1280x720)ではUIが画面に対して相対的に大きく見えていた。

**修正方針・実装内容:**
- `Constants/Game.h`に`base_screen_width`/`base_screen_height`(常に1920x1080固定、UIスケール計算専用の基準解像度)を追加。既存の`screen_width`/`screen_height`(Debug/Releaseで異なる実解像度)はそのまま維持。
- `Application`クラスに`float GetUIScale() const`を追加(`Application.h`/`.cpp`)。`実際のウィンドウ幅 / base_screen_width`を返す。
- UIの`scale`引数を持つ描画箇所全てに`GetUIScale()`を掛けるよう修正。対象は`TitleScene.cpp`、`ClearScene.cpp`、`GameoverScene.cpp`、`PauseScene.cpp`（各シーンの選択肢・背景・Aボタン/決定テキスト画像）と、`Game/UI/GaugeUIBase.cpp`の`DrawGauge`(HP/特殊ゲージ共通処理、ここを直したことで`BossHPGaugeUI`/`PlayerHPGaugeUI`/`SpecialGaugeUI`は個別修正不要で解決)。
- 一方で`TargetManager::IsOnScreen`(画面内判定)、`Utility/Fade.cpp`(全画面塗りつぶし)、`CameraBase.cpp`(アスペクト比計算)の`wsize`使用箇所は、そもそも解像度に自動追従する性質のものでスケール概念が不要と判断し、変更していない。

**検証:** クリーンビルド(Debug/x64)でEffekseer以外の警告・エラー0件を確認済み。**実際にDebug/Release両方でタイトル・クリア・ゲームオーバー・ポーズ画面のUIの見た目の大きさが揃うかは、Visual Studio上で両構成を実際に起動して目視確認する必要がある(未実施)。**


### 進捗（2026-09-27・VS Code単体でビルド/実行/デバッグできるよう`.vscode/`を整備）

**見つかった問題と修正（`.vscode/*.json`のみ変更、ソースは未変更）:**
- `tasks.json`: vcxproj単体をビルドすると`$(SolutionDir)`がプロジェクトフォルダになり`DxLib_h`が見つからない → **`NovaWing\NovaWing.slnx`経由でビルド**するよう変更。`/p:GenerateFullPaths=true`を追加（問題パネルのエラーからファイルに飛べる）。
- `launch.json`: exeの実際の出力先は`NovaWing\x64\Debug\NovaWing.exe`（`NovaWing\NovaWing\x64\Debug`は.objの中間フォルダ）→ `program`を修正。`cwd`はVSの既定と同じ**プロジェクトフォルダ`NovaWing\NovaWing`**に変更（`Data/...`と`*.pso/*.vso`を相対パスで読むため）。Release構成も同様に修正。
- `c_cpp_properties.json`: include pathは全`#include`を確認して過不足なし（vcxprojの3パスと一致）。defineをvcxprojのDebug|x64に合わせ（`WIN32`を削除）、`/permissive-`・`/MTd`を追加。Release x64構成も追加（VS Codeの右下で切り替え）。compile_commands.jsonは全ファイル同一フラグのため不要と判断。
- `settings.json`: 一部`.hlsl`（`DamagePS.hlsl`/`SkinnedLightingVS.hlsl`等）がShift-JISのため`files.autoGuessEncoding`を有効化。新規ファイルはBOM付きUTF-8で保存。
- `extensions.json`: HLSL Tools（`timgjones.hlsltools`）を推奨に追加。
- DxLib/Effekseerは静的リンクでDLL不要。コピー処理は不要。

**検証:** タスクと同じコマンドでビルド成功（LNK4099はEffekseerのpdbが無いだけで無害）、launch.jsonと同じcwdで起動し10秒動作・`Log.txt`にエラーなしを確認。**VS CodeでF5→ブレークポイントで止まるかは未確認（手動で確認が必要）。**

### 進捗（2026-09-27・VS Codeに「クラスの追加」タスクを作成）

- `.vscode/scripts/New-GameClass.ps1` を作成。クラス名・基底クラス名(任意)・配置フォルダ・基底ヘッダー(任意、空なら`基底名.h`を自動検索)から`.h/.cpp`雛形(BOM付きUTF-8/CRLF/タブ)を生成し、`NovaWing.vcxproj`と`.vcxproj.filters`に登録する。
- 登録はXmlDocument(PreserveWhitespace)経由。読み書きだけならバイト単位で完全一致することを確認済み。VSと同じくItemGroupの末尾に追加し、`.filters`は既存の「ソース ファイル」「ヘッダー ファイル」に入れる(フィルタ定義自体は変えない)。同名ファイルがある・既に登録済み・プロジェクト外のパス・不正な識別子はエラーで中断。途中で失敗したら生成ファイルとプロジェクトファイルを元に戻す。
- `tasks.json`に「NovaWing: クラスの追加」タスクと入力用の`inputs`を追加。生成後は`code -r`で`.h`を開く。
- 検証: テストクラス2つ(基底あり・なし+新規フォルダ)を生成→ビルド成功を確認後、ファイルと登録を削除(`git diff`で`.vcxproj`/`.filters`に差分なし)。
- **注意: `.gitignore`の`.vscode/*`によりスクリプトがgit管理外。** 学校/家の両方で使うには`!.vscode/scripts/`の例外追加が必要(未対応・要判断)。

### 進捗（2026-09-27・クラスの削除/ファイル名リネームのタスクを追加）

- `.vscode/scripts/NovaWingProject.ps1`(共通処理)を新設し、`New-GameClass.ps1`もこれを使う形に整理。XMLの読み書き・登録/除去・対象ファイルの特定・`#include`参照検索(コンパイラと同じ順で解決するので、同名ファイルの取り違えなし)をまとめた。
- `Remove-GameClass.ps1`(タスク「Delete C++ Class」): 他ファイルから`#include`されていたら参照元一覧を出して中断。なければ変更内容を表示→y/N確認→vcxproj/.filtersから除去→ファイルは**ごみ箱へ**移動。
- `Rename-GameClass.ps1`(タスク「Rename C++ Class (filename only)」): `.h/.cpp`をまとめてリネームし、vcxproj/.filtersのパスを更新。クラス名・`#include`の中身は変えない。旧名を`#include`している箇所(リネームした.cpp自身も含む)を確認前と完了後に警告表示。
- 対象は「空Enterで今開いているファイル」か、パス入力(.h/.cpp/拡張子なし)。
- ハマった点: PowerShell 5.1では関数が1件だけ返すと配列でなくなり、XmlElement/pscustomobjectに`.Count`が無く`$null`になる → 参照ありを見逃してテストクラスを削除してしまった(ごみ箱行き)。呼び出し側を`@()`で包んで修正。
- 検証: 参照ありで削除中断、n で中止時は無変更、既存名へのリネームはエラー、リネーム/削除の実行、を確認。追加→リネーム→削除後に`.vcxproj`/`.filters`がテスト前とバイト一致。最終ビルド成功。
- テスト用ファイル(TestGenA/TestGenA2/TestGenB2)がごみ箱に残っている。
- `.gitignore`に`!.vscode/scripts/`を追加し、スクリプト4本をgit管理対象にした(学校/家の両方でタスクが動くように)。`c_cpp_properties.json`はPCごとにコンパイラパスが違う可能性があるため、引き続き管理外。

### 進捗（2026-09-27・VS Codeの配色をVisual Studio風に）

- 公式拡張「C/C++ Themes」(`ms-vscode.cpptools-themes`、マーケットプレイスで実在確認済み・インストール済み)を`extensions.json`の推奨に追加し、`settings.json`で`"workbench.colorTheme": "Visual Studio Dark - C++"`(=VS2019/2022 Dark相当)をワークスペース既定にした。
- テーマ定義を確認し、VSとずれていた「文字列(#CE9178→#D69D85)」「列挙型・列挙子(→#B8D7A3)」だけを、このテーマ限定で`tokenColorCustomizations`/`semanticTokenColorCustomizations`で上書き。
- 型・関数・引数・メンバー・ローカル変数などの色分けはC/C++拡張のセマンティックハイライト頼み(IntelliSenseの解析が終わるまでは大まかな色)。

### 進捗（2026-09-27・ショートカットをVisual Studio寄りに）

- 公式拡張「Visual Studio Keymap」(`ms-vscode.vs-keybindings` v0.2.1、マーケットプレイスで実在確認)を`extensions.json`の推奨に追加(未インストール)。中身を確認したところ、Ctrl+W(タブを閉じる→選択範囲の拡張)・Ctrl+B(サイドバー→関数ブレークポイント)・Ctrl+,(設定→ファイル検索)・Ctrl+Shift+S(名前を付けて保存→すべて保存)・Ctrl+L/Ctrl+Shift+L・Ctrl+Shift+G などVS Code標準を上書きする。コピー/貼り付け/Ctrl+S/Ctrl+Fは触らない。
- **VS Codeはワークスペースの`.vscode/keybindings.json`を読まない(キーバインドはユーザー単位のみ)**。`.vscode/keybindings.json`は共有用テンプレートとして作成し、`.gitignore`に例外を追加。使うにはユーザーのkeybindings.jsonへ貼り付けが必要。
- 割り当て: Ctrl+Shift+Alt+C=クラスの追加 / Ctrl+Shift+Alt+D=削除 / Ctrl+Shift+Alt+R=リネーム / Ctrl+Break=ビルド中止 / Ctrl+Shift+F10=次のステートメントの設定。VS Code 1.138の既定キー一覧とKeymap拡張の両方と突き合わせて衝突なしを確認。ビルドのCtrl+Shift+Bは既定のまま(既定ビルドタスク=MSBuild: Debug x64)。
### 進捗（2026-09-27・ステータスバーにタスクのボタンを追加）

- 拡張「Tasks」(`actboy168.tasks` v0.16.1、実在確認・インストール済み)を導入し、`extensions.json`の推奨に追加。
- `tasks.json`の各タスクに`options.statusbar`を追加: 「ビルド」「クラスの追加」は常に表示、「クラスの削除」「ファイル名の変更」は.h/.cppを開いているときだけ表示(`filePattern`)、Release/Cleanは非表示。ビルド中は「ビルド中」に変わる。
- マウスだけで実行する場合は、メニューバー「ターミナル」→「タスクの実行...」でも可。
### 進捗（2026-09-27・インデント/整形設定をVisual Studioに合わせる）

- VS 2026の実設定(`%LOCALAPPDATA%\Microsoft\VisualStudio\18.0_*\Settings\CurrentSettings.vssettings`)を確認: C/C++はタブ幅4・インデント4・タブ文字・スマートインデントで、VS Code側と一致済み。
- VSは「ClangFormatサポート」が有効(既定)→ リポジトリ直下の`.clang-format`で整形しており、`NovaWing/.editorconfig`の`cpp_*`書式設定は使われていない(Microsoftのドキュメントでも「ClangFormat有効時は個別設定を無視」)。VS Codeも同じ`.clang-format`を使うので整形ルールは同じ。
- 違いは入力中の自動整形だけ: VSは`;`や`}`の入力時に整形する → `settings.json`の`[cpp]`/`[c]`で`editor.formatOnType`を有効化(C/C++拡張は`;` `}` 改行で整形)。貼り付け時の整形(VSの設定値`AutoFormatOnPaste2=1`)は意味を確定できず、オフのまま。- **訂正(同日):** VSは入力中の整形にclang-formatを使っていなかった(`ClangFormatExecution=1`は「手動の整形コマンドのときだけclang-format」の意味だった)。VS Codeで`if(...) return`の後に`;`を打つと、clang-format(Microsoftスタイル)が`return;`を次の行に分けてしまい、VSと違う動きになった。VSは入力中は自前の整形エンジン+`NovaWing/.editorconfig`で、その行を整えるだけ。
  → `C_Cpp.formatting`を`vcFormat`(Visual C++の整形エンジン。`.editorconfig`の`cpp_*`設定を使う)に変更。代わりにVS CodeのCtrl+K Ctrl+Dも`.editorconfig`基準になり、VSの手動整形(`.clang-format`基準)とは細部が違うことがある(C/C++拡張は整形エンジンを1つしか選べないため)。- `launch.json`のRelease構成を「NovaWing (Release x64)」に改名し、`preLaunchTask`で起動前に`MSBuild: Release x64`を実行するようにした(Debugと同じくF5でビルド→起動)。Releaseビルドが.slnx経由で通ることを確認。

### 進捗（2026-09-28・学校PC(FIC-ADMIN)のVS Code環境を確認・整備）

- 学校PCにもVS 2026(`C:\Program Files\Microsoft Visual Studio\18\Community`、家と同じパス)と推奨拡張6つが入っており、git管理下の`.vscode`(settings/tasks/launch/extensions/scripts)もそのまま使えた。タスクと同じMSBuildコマンドでDebug x64ビルド成功を確認。
- git管理外の`c_cpp_properties.json`を学校PC用に作成（cl.exeは`MSVC\14.44.35207`(v143)、SDK `10.0.26100.0`、vcxprojと同じinclude 3つ、`_DEBUG/_WINDOWS/UNICODE/_UNICODE`、`/permissive-`、Debug/Release x64の2構成）。
- ユーザー単位の`%APPDATA%\Code\User\keybindings.json`に、`.vscode/keybindings.json`テンプレートの5項目(Ctrl+Shift+Alt+C/D/R、Ctrl+Break、Ctrl+Shift+F10)を既存設定の末尾に追記。
- 学校PCでは`git`コマンドにPATHが通っていない（GitHub Desktop等で操作）。
- 上の「コマンドラインMSBuildは`C1083`で失敗する」という記述(ICollider節)は古い情報。`.slnx`経由なら成功する。
- **未確認**: F5でのデバッグ実行・ブレークポイント停止（家・学校とも）。

### 進捗（2026-09-28・DxLib開発環境拡張(dxlib-devenv)を本採用、フォルダ構成を平らにした）

- 配布されたVS Code拡張「DxLib 開発環境」(mahirocreative.dxlib-devenv)を導入。拡張の「新規プロジェクト作成」はMultiByte固定・`src/`固定でvcxprojを毎回作り直すため、Unicode(`L"..."`)や独自includeを使うNovaWingでは使えない。代わりに「Visual Studio のプロジェクトを DxLib 拡張で使えるようにする」モード（`settings.json`の`dxlib.vsProject`）で、既存のvcxprojをそのまま使う。
- 拡張は「開いたフォルダ直下の.vcxproj」しか見ないので、`NovaWing\NovaWing\*`と`NovaWing\DxLib_h`をリポジトリ直下へ移動（`NovaWing\`フォルダは廃止）。`.slnx`も直下。vcxprojは無変更（`$(SolutionDir)`も直下になるため`DxLib_h`のパスはそのまま通る）。
- ビルドは拡張がMSBuildでvcxprojを直接ビルドする。exeは`x64\<Config>\NovaWing.exe`、作業フォルダはリポジトリ直下（`Data\`と`*.pso/*.vso`を相対パスで読む）。
- `.vscode`：旧MSBuildタスク(`.slnx`経由)を`type: dxlib`のタスク(`DxLib: Debug ビルド`/`DxLib: Release ビルド`)に置き換え、launch構成も拡張と同名に。IntelliSenseは拡張のconfigurationProviderに任せる（`c_cpp_properties.json`は各PCで拡張と同じ内容にする）。クラス追加/削除/改名スクリプトは新構成のパスに修正。
- **注意**: 拡張は`tasks.json`を`JSON.parse`で読むため、`tasks.json`/`launch.json`に`//`コメントを書くとDxLibプロジェクトと認識されなくなる。
- DxLibパネルでSDK(3.24f)を指定し、F5でビルド・実行できることを確認。上の節にある「`.slnx`経由でビルド」「`NovaWing\x64\...`に出力」などの記述はこの移行で古くなった。
- 学校PCでは：拡張を`install.bat`で入れる → DxLibパネルでSDKを指定 → `c_cpp_properties.json`を拡張と同じ内容に置き換える。

### 進捗（2026-09-28・ボスの螺旋ビームのエフェクトを作成中、見た目の調整が残り）

- 参考: スターフォックスのリメイクのボス。紫の電撃リングが螺旋状に連なり、先に行くほど広がるビーム。
- Claudeが`Data/Effect/BossBeam/BossBeam_Spiral.efkproj`（1.80形式のXML）を書いた（コミット3eceb0cに含まれる）。ゲームが読んでいるのはまだ旧版の`BossBeam_2.efk`（`ResourceConstants.h`の`boss_beam_eff_patgh`）。**次回は、エディタで`BossBeam_Spiral.efkproj`を開いて見た目を調整 → `.efkefc`保存・`.efk`書き出し → パスを差し替え、から再開。**
- **大きさの基準**: `.efk`書き出し時の拡大率が**50**（エディタ1単位＝ゲーム50単位）。ビーム先端は毎フレーム20進む＝エディタで0.4、当たり判定の半径70＝1.4。
- **しくみ**: ゲーム側(`BossBeamState`)はエフェクトをビーム先端に置いて毎フレーム動かし、ローカル+Zをボス側に向ける。エフェクト内の粒子は親の影響を「生成時のみ」にして、その場に残ることで軌跡になる。
  - `SpiralPivot`(null、Z軸まわりに30°/F回転) の子 `ThunderRingA/B`: 軸から0.5離れた位置に4Fごとにリング（Aと B は2Fずらし、テクスチャ違い）。寿命70〜90F、大きさ1.7→0.8で縮む（発射口側が細く先端側が太い）、X/Y±12°傾き、Z回転ランダム。
  - `CoreTrail`: 軸上に毎F紫の光を残す。`TipGlow`: 先端でちらつく光（先端に追従）。`StrayBolts`: 周囲に`Thunder01`の小さな雷を散らす。
  - 螺旋の巻き具合＝Pivotの回転速度、リングの間隔＝発生間隔、太さ＝リングの大きさと軸からの距離、軌跡の長さ＝寿命。
- テクスチャ`SpiralThunderRing01/02.png`はClaudeがSystem.Drawingで自動生成したもの（ギザギザの輪＋紫のぼかし＋白い芯）。
- `.efkefc`はバイナリだが、EDITチャンクはzlib圧縮のバイナリXML（名前表→値表→u16ルート数→ノード木: 要素ID u16／値有無 u32 [+値ID u16]／子有無 u32 [+子数 u16]）で、デコードすれば.efkprojと同じXMLになる。Claudeに調整を頼むときは、エディタで保存した`.efkefc`をデコードして現状を読んでもらえばよい。
- 1.80形式の注意: 発生間隔は`CommonValues/Generation/GenerationTime`、色は`DrawingValues/ColorAll/Fixed`（1.7以前と場所が違う）。
- 既存の`BossBeam.efkefc`などの古いエフェクトは、テクスチャをデスクトップの`Effekseer_Sample`から参照しており、他のPCでは見つからない。

### 進捗（2026-09-28・家PCにもDxLib開発環境拡張(dxlib-devenv)を導入）

- 学校で導入した拡張(`mahirocreative.dxlib-devenv`)を家PCにも導入。配布物は`Downloads\DxLib-devenv-1.0.0.zip`(`.vsix`+`install.bat`+README)。**`.vsix`を直接ダブルクリックしない**（Visual Studioが入っているPCでは.vsixがVSに関連付けられておりVSのインストーラーが起動して失敗する）、必ず`install.bat`経由でインストールする仕様。`install.bat`は中のVSIXを`code --install-extension --force`するだけの単純な処理と確認済み。
- インストール後、DxLibパネルのSDKパス指定で**`Downloads\DxLib_VC3_24f`を指定したら`DxLib.h`が見つからないエラー**になった。このフォルダは中身が空（サイズ0、対応するzipも無し）。**正しくは、フォルダ構成を平らにした際(9/28の別作業)にリポジトリ直下へ移動済みの`NovaWing\DxLib_h`（`DxLib.h`が実在）を指定して解決。**
- 一時展開フォルダ(`Downloads\DxLib-devenv-1.0.0_extracted`)は作業後に削除済み。元の`.zip`は残している。

### 進捗（2026-09-28・螺旋ビームの見た目を参考動画と比較）

- エディタでプレビューするとエフェクト全体が動いてしまう件：原因は「振る舞い」ウィンドウの位置の速度Z=-0.4（前回Claudeがプレビュー用に入れたもの）。これはエディタ専用の設定で、`.efk`の実行用データには入らない。ユーザーが振る舞いを変えて解決。
- `BossBeam_Spiral.efkproj`は、エディタで保存し直した結果、中身が`.efkefc`と同じバイナリ形式になっている（拡張子は.efkprojのまま）。
- 参考動画（スターフォックス・リメイク）と比べた結果：参考は、リングが隙間なく重なって1本の太い筒に見え、筒の内側も紫の光で満たされている。今のエフェクトは実体が先端だけで、後ろは丸が並ぶだけに見える。考えられる原因：
  1. リングのテクスチャが輪郭だけで、内側が空っぽ。
  2. 発生間隔2F（A/B合わせて）× 先端の速さ0.4 → リングの間隔が0.8あり、正面から見ると1個ずつの輪に見える。
  3. リングが1.7→0.8に縮み、最後の30Fでフェードアウトするので、後ろ側が細く薄くなる。
  4. `CoreTrail`が小さな丸(Particle01、1.4→0.3、寿命30F)を毎F置くので、「丸の軌跡」に見える。
- 対策案（ユーザーがエディタで試す）：リングを毎F発生、縮小とフェードを弱める、CoreTrailはリングと同じくらいの大きさで薄い光にする（または削除）、内側を塗ったリングのテクスチャを追加する。
- **(同日・続き) Claudeが上の対策を実施:** リングを A/B 合わせて毎F発生（GenerationTime 4→2、Bのずらしは1F）、大きさは2.0→1.4、フェードアウトは30→10F。CoreTrailは削除。`SpiralThunderRing01/02.png`の輪の内側に紫のもやを追加（縁ほど濃い）。**ゲーム内での見た目は未確認。**
- **書き出しはコマンドラインでできる:** `Effekseer.exe -cui -in <.efkproj/.efkefc> -e <出力.efk> -m 50`（`-m`が拡大率。`-scale`は無い）。書き出し先は、ゲームが読んでいる`BossBeam_2.efk`（中身は螺旋エフェクト。対応する編集用ファイルは`BossBeam_Spiral.efkproj`で、`BossBeam_2.efkefc`は古いエフェクトのまま）。今のファイルをこの方法で書き出すと、既存の`BossBeam_2.efk`とバイト単位で一致することを確認済み。
- `BossBeam_Spiral.efkproj`はXML形式で上書きした（エディタで開ける）。振る舞いの位置の速度Zは-0.4のまま（ゲームには影響しない）。
- **(同日・さらに修正) 明るすぎたので抑えた:** ユーザーの要望は「前の見た目はよかった。軌跡にも当たり判定を付けるので、軌跡側にも実体があるように見せたいだけ」。リングの発生間隔は元（4F、Bは2Fずらし）に戻し、大きさは1.7→1.4（元は→0.8）、フェードアウトは10F、CoreTrailは削除のまま。内側のもやは、上の版の約4割の濃さにした（元のテクスチャに対して再生成）。**ゲーム内での見た目は未確認。**
- 参考メモ: リングの見た目の直径は、エディタで約1.3（大きさ1.7のとき）。当たり判定の直径は2.8（半径70）なので、判定のほうがかなり大きい。
- **(同日・さらに修正) 軌跡を長く残す:** リングの寿命を70〜90F → **430〜450F**にした（判定球はビーム終了`beam_end_frame`=420Fまで残るので、それに合わせた）。振る舞いの位置の速度はユーザーが0にしたので、そのまま残した。

### 進捗（2026-09-28・デバッグ表示の一括ON/OFFトグルを実装、完了）

- 要望：デバッグ中のみ、デバッグ用の球・テキストを全部消せるオプションをポーズ画面に追加し、OKボタンでトグルできるようにしたい。
- 方針確認のやり取り：①項目の追加先は「ポーズ画面(PauseScene)に新規選択肢を追加」で合意。②ON/OFFフラグの置き場所は新規`DebugManager`シングルトンで合意。③その場でユーザーから「シングルトンが増えすぎているので集約クラスが欲しい」という提起があったが、今回のタスクは影響範囲を絞るため通常のシングルトンのまま実装し、集約クラスへの移行は上記「未解決タスク9」として別途保留にすることで合意。④今回は該当箇所が15箇所以上と多いため、いつもの「ユーザーが手を動かして書く」方針を外れ、Claudeが全部実装する形で合意（ユーザー：「デバッグに関することだけなのですべてお任せします」）。
- **実装内容（Claudeが直接編集・いつもの直接編集禁止ルールの例外扱い）:**
  - `Manager/DebugManager.h/.cpp`新規作成。中身は`bool m_isDebugDrawEnabled = true`だけを持つ素朴なシングルトン（`IsDebugDrawEnabled()`/`ToggleDebugDrawEnabled()`）。`.vcxproj`/`.vcxproj.filters`にも追加。
  - `PauseScene`の`Select` enumに`#ifdef _DEBUG`限定で`ToggleDebugDraw`を追加。OK決定時は`DebugManager::GetInstance().ToggleDebugDrawEnabled()`を呼ぶだけでシーン遷移はしない。表示は既存選択肢のような専用画像が無いため`DrawFormatString`で「デバッグ表示 : ON/OFF」をテキスト表示（カーソルが乗っているときは黄色）。
  - 全15箇所ほどの既存`#ifdef _DEBUG`描画ブロック（`Application.cpp`のFPS表示、`GameScene.cpp`/`TitleScene.cpp`のグリッド線、`WaterManager.cpp`、`PlayerHPGaugeUI.cpp`、`BulletBase.cpp`、`Rock.cpp`、`TitlePlayer.cpp`、`Player.cpp`、`MovingState.cpp`、`WormEnemy.cpp`、`FloatingEnemy.cpp`、`ActiveState.cpp`、`BossEnemy.cpp`、`BossBeamState.cpp`）の中身全てに`if (DebugManager::GetInstance().IsDebugDrawEnabled()) { ... }`を追加。
- **ハマった点：** `DebugManager.h/.cpp`をUTF-8(BOM無し)で新規作成したところ、日本語コメントを含む他ファイル(`PlayerHPGaugeUI.cpp`)のビルド時に`warning C4819`（現在のコードページで表示できない文字）が発生。既存ヘッダ(`InputManager.h`等)はUTF-8 BOM付きだったため、BOM付きに変換して解決。**このプロジェクトで日本語コメントを含む新規.h/.cppを作る際は、UTF-8 BOM付きで保存する必要がある。**
- Debug/Release両構成でビルド成功を確認済み（`MSBuild NovaWing.vcxproj /p:Configuration=Debug(Release) /p:Platform=x64`）。Releaseビルドでは`ToggleDebugDraw`選択肢自体が`#ifdef _DEBUG`で存在しなくなる。

### 進捗（2026-09-28続き・PauseボタンとSTARTリスタートのボタン衝突を解消、リスタートをポーズの選択肢に移設）

- ユーザー報告：「デバッグ版でPauseボタンを押すとゲームが最初からになる」。調査の結果、原因は既存バグではなく**`pause`と`restart`(`_DEBUG`限定)の入力イベントが両方`XINPUT_BUTTON_START`に割り当てられていたこと**。元々`restart`だけ実装していたところに後から`pause`を追加したため衝突していた、とユーザーからも確認が取れた。
- 対応方針（ユーザー合意）：STARTボタン直接リスタートは削除し、ポーズ画面を残す。代わりに`_DEBUG`限定でポーズの選択肢に「最初からやり直す」を追加する。
- **実装内容：**
  - `Scene/GameScene.cpp`の`_DEBUG`限定「STARTボタンでリスタート」ブロックを削除。
  - `Manager/InputManager.h/.cpp`から`InputEvent::restart`定義とテーブル登録を削除（他に参照箇所が無いことを確認済み）。
  - `Scene/PauseScene.h`の`Select` enumに`_DEBUG`限定で`Restart`を追加（`ToggleDebugDraw`の次）。
  - `Scene/PauseScene.cpp`のOK決定処理に`Select::Restart`の分岐を追加：`m_controller.ChangeScene(std::make_shared<GameScene>(m_controller), 0.0f)`で新しいGameSceneに切り替え。
  - 表示は`ToggleDebugDraw`と同様、画像が無いため`DrawFormatString`で「最初からやり直す」をテキスト表示（`debug_toggle_ratio`の下、`restart_ratio = (0.5, 0.85)`に配置）。
- Debug/Release両構成でビルド成功を確認済み。Releaseビルドでは`Restart`選択肢自体が存在しない。
- **事故と復旧:** ユーザーがエディタで保存して`.efkproj`がバイナリに戻っていたのに、ClaudeがXMLとして読もうとして失敗した。そのとき、書き込み用にファイルを開く処理だけが先に動いて、空のファイルで上書きしてしまった（`BossBeam_2.efk`も空に）。開いていたエディタでCtrl+Sしてもらって復旧した。**今後Claudeが編集するときは:** バイナリかXMLかを確認して展開する → 一時フォルダで保存と書き出しをする → 書き出したファイルのサイズとテクスチャを確認してから置き換える。**エディタで開いたまま私が書き換えたときは、エディタで開き直してから作業すること**（そのまま保存すると私の変更が消える）。

### 進捗（2026-09-28続き・ポーズ画面にプレイヤー位置ワープ機能(物差しUI)を追加、既存のbossWarpチートを削除）

- 要望：デバッグを円滑にするため、ポーズ画面に「プレイヤーを移動」の項目を作り、選ぶと初期位置〜ボスの物差し(目盛り付き)が出て、左スティックでカーソル移動、Aボタン決定でその位置にプレイヤーをワープさせたい。
- 事前調査で判明した情報：
  - プレイヤー初期位置は`Player.cpp`の`first_pos = {0.0f, 500.0f, 1200.0f}`（Z=1200が始点）。
  - ボス出現の実トリガーは`GameScene.cpp`の`constexpr float boss_appear_z = 27000.0f`（プレイヤーZがこれを超えるとカメラ演出とともにボスが出現）。目盛りの「ボス」はこの値を採用することでユーザーと合意。
  - 位置操作は`GameObject`基底クラスに既存の`GetPos()`/`SetPos()`がpublicであり、そのまま使える（新規アクセサ追加は不要）。
  - カメラはプレイヤーZから毎フレーム`m_pos.z = playerPos.z - camera_offset_z`で再計算される作りのため、ワープ後の追従は自動で行われる（特別なカメラ処理は不要と確認済み）。
  - ちょうど今回のやり取り中に、以前実装していた既存の**Wキーでボスへワープするデバッグチート(`InputEvent::bossWarp`、Z=19000固定)は今回の機能で不要になるためユーザーの指示で削除**。
- **実装内容：**
  - `Scene/PauseScene.h/.cpp`：コンストラクタに`std::weak_ptr<Player> pPlayer`を追加（`GameScene.cpp`のポーズ生成呼び出し側も修正）。`Select` enumに`_DEBUG`限定で`WarpPlayer`を追加。
  - ワープ操作は専用の別シーンにはせず、**PauseScene内のモードフラグ(`m_isPlayerWarpMode`)で分岐**する設計にした（シーンスタックを増やさずBGMや既存のグリッチ演出を共有できるため）。
  - `WarpPlayer`選択→`m_isPlayerWarpMode = true`。ワープモード中は`Update()`冒頭で`UpdatePlayerWarpMode()`に丸ごと処理を委譲し、通常の選択肢カーソル移動("上下"の`InputEvent`)は動かないようにした。
  - `UpdatePlayerWarpMode()`：`InputManager::GetBufX()`(左スティックX、±1000スケール、デッドゾーン処理済み)でカーソル位置の割合(`m_warpCursorRatio`、0.0〜1.0)を移動。`InputEvent::ok`(Aボタン)で決定：`warp_range_min_z`(1200)〜`warp_range_max_z`(27000)を`m_warpCursorRatio`で線形補間したZ座標を`pPlayer->SetPos()`に渡し、ワープモードとポーズ画面の両方を閉じてゲームに戻る。`InputEvent::close`(Bボタン)でワープモードだけキャンセルして選択肢一覧に戻る。
  - `DrawPlayerWarpMode()`：画面を暗くした上に、横一本の物差し線(`DrawLine`)、左右端に目盛り線と「初期位置」「ボス」のラベル、選択中の位置に黄色い三角カーソル、現在のZ座標の数値表示、操作説明テキストを描画。画像アセットは使わず全てプリミティブ描画+`DrawFormatString`。
  - 既存の`bossWarp`チートを削除：`Manager/InputManager.h/.cpp`から`InputEvent::bossWarp`定義とテーブル登録を削除、`Player.cpp`の`if (input.IsPressed(InputEvent::bossWarp)) { m_pos.z = 19000.0f; }`ブロックを削除。
- Debug/Release両構成でビルド成功を確認済み。Releaseビルドでは`WarpPlayer`選択肢自体が存在しない。

### 進捗（2026-09-30・ボスのビーム跳ね返し、実装着手）

- 反射の状態は`BossBeamState`に持たせる(A案)で確定。反射後の`Update()`は、プレイヤー追跡・Z越え判定・判定球生成をやめ、ボスへ向けて**1フレームの最大旋回角以内**で`m_beamMoveDir`を寄せる方式(「距離の上限」ではなく「角度の上限」)。
- **完了(コード確認済み):**
  - `BossBeamState.h`: `OnReflectLeft/Right`(public)・`GetTipSphereL/R`・`IsReflectedL/R`の宣言、`m_isReflectedL/R`、非デバッグの先端球`m_beamTipSphereL/R`。
  - `BossBeamState.cpp`: `OnReflectLeft/Right`が左右とも実装済み(先端位置と`hitPos`から法線を求め、`m_beamMoveDir = v - normal * (2 * Dot(v, normal))`で反射、`m_isReflected`を`true`に)。反射式の`float * Vector3`問題(`Vector3`は`Vector3 * float`のみ)は`normal * (2 * Dot(...))`の形で修正済み。`GetTipSphereL/R()`も定義済み。
- **完了(2026-10-01コード確認済み):** `IsReflectedL/R()`が`m_isReflectedL/R`を返す、`Enter()`で反射状態をリセット、先端球の更新を先端を動かした後(`Update()`内)に移して有効化、先端球のデバッグ描画を有効化、法線のコメントを「当たった位置→先端」に修正。
- **完了(2026-10-01コード確認済み):** `ColliderTag::BossBeamTip`追加。`BossBeamTipCollider`(`Game/Collision/`、vcxproj登録済み)を作成: コンストラクタで`BossEnemy&`と`isRight`を受け取る。`IsCollisionActive()`=ボス生存・ステートが`BossBeamState`・自分の側が未反射。`GetCollision()`=自分の側の`GetTipSphereL/R()`。`OnCollision()`=相手が`Counter`ならカウンター球の中心を`hitPos`として`OnReflectLeft/Right`を呼ぶ（反射のきっかけは`CollisionManager::OnHit`ではなく先端コライダー自身の`OnCollision`に置くことにした）。
  - ハマった点: `IsCollisionActive()`で、ビームのステートか調べた結果に関係なく`dynamic_pointer_cast(...)->IsReflectedR()`を呼んでおり、ビーム以外のステートで`nullptr`アクセスになるところだった → `nullptr`なら先に`return false`。
- **未完成・未着手(2026-10-01時点):**
  - ~~`BossEnemy`への登録、`hit_pairs`への追加~~ → **完了・実機確認済み(2026-10-01)**: `BossEnemy`が`BossBeamTipCollider`を右(`true`)・左(`false`)の2つ値で持つ（他のボスのコライダーに合わせて`BossEnemy.h`に`#include`）。`hit_pairs`の`{BossBeamTip, Counter}`は`{BossBeam, Player}`より上。パリィで`OnCollision`が呼ばれ反射のきっかけが動くことをユーザーが確認。
  - パリィした瞬間、プレイヤーの近くに残っている通り道の判定球で、同じフレームに本体がビームのダメージを受ける可能性がある（ローリング中も本体の判定は有効なため）。
    → **決定(2026-10-01)**: 反射した側の通り道の判定球は消さずに残し、**パリィ成功時にプレイヤーを短い間だけ無敵にする**。実装方針: Playerに`m_invincibleFrame`/`StartInvincible(int)`/`IsInvincible()`を追加し`Update()`で減らす、`PlayerCollider::IsCollisionActive()`に「無敵中でない」を追加、`CounterCollider::OnCollision`で相手が`BossBeamTip`なら`StartInvincible`（無敵フレームは`CounterCollider.cpp`の定数、まず30F）。`{BossBeamTip, Counter}`が`{BossBeam, Player}`より上なので、同じフレームのダメージも防げる。
    → **実装済み(2026-10-01コード確認)**: `Player::OnInvincibleStart(int)`/`IsInvincible()`/`OnInvincibleEnd()`、`m_invincibleFrame`を`Player::Update()`で減算、`PlayerCollider::IsCollisionActive()`に`!IsInvincible()`、`CounterCollider::OnCollision`で相手が`BossBeamTip`なら`OnInvincibleStart(counter_invincible_frame=30)`。無敵中にもう一度パリィしたら数え直す（左右続けてパリィしたときに早く切れないように）。実機確認はまだ。
  - **反射後の動き、実装済み(2026-10-01コード確認、実機確認はまだ)**: `Vector3::Cross`を追加。`BossBeamState::Update()`で、反射していない側だけプレイヤー追跡と通り道の判定球の生成を行い、反射した側はボスのダメージ判定球へ`one_frame_turn_angle`(=`DX_PI_F/90`、2度)を上限に向きを寄せる。ハマった点: 右の`else`を内側の`if(プレイヤーZ<先端Z)`に付けてしまい、反射していないビームがボスへ戻る逆の動きになっていた／右だけ角度判定が無く向き切った後に揺れる／かっこ不足。
  - 次: 反射したビームがボスのダメージ判定に当たったら大ダメージ。**方針決定(2026-10-01)**: 反射した側は通り道の球を作らないので、ボスに届くのは先端の球。`BossBeamTipCollider`の`GetTag()`を「反射済みなら`BossBeamReflected`、まだなら`BossBeamTip`」に切り替えて使い回す（`CollisionManager`は毎フレーム`GetTag()`で振り分け直すので、次のフレームから判定相手が変わる）。`hit_pairs`に`{BossBeamReflected, BossDamage}`、`BossDamageCollider::OnCollision`で相手が`BossBeamReflected`なら大きめの固定ダメージ。**当たった後は、先端をその場で止め、`SetColorPlayingEffekseer3DEffect`でアルファを徐々に下げ、0になったら`StopEffekseer3DEffect`**（止めたハンドルは-1にして以後触らない）。軌跡に残った粒子までちゃんと薄くなるかは実機で確認する。

**現状のまとめ（2026-10-01、学校で作業終了時にコードを読んで確認）**

- **完了済み**
  - 反射のきっかけ（先端×カウンター→`OnReflectLeft/Right`）、パリィ成功時の無敵、反射後にボスへ曲がる動き（上記のとおり）。
  - `ColliderTag::BossBeamReflect`を追加（ノート上の仮名`BossBeamReflected`ではなく、**実際の名前は`BossBeamReflect`**）。
  - `BossBeamTipCollider::GetTag()`: 自分の側が反射済みなら`BossBeamReflect`、まだなら`BossBeamTip`を返す。一度、右が`IsReflectedL()`・左が`IsReflectedR()`を見る左右逆のバグがあったが修正済み。
  - `BossBeamState`: `m_isHitBossL/R`、`OnHitBossL()`/`OnHitBossR()`、`IsHitBossL()`/`IsHitBossR()`（.hにインライン）、`Enter()`でリセット。
- **未完了（家でここから再開）**
  1. `BossBeamTipCollider::IsCollisionActive()`: まだ「反射したら`false`」のまま。**「ボスが生きていて、ステートが`BossBeamState`で、自分の側がまだボスに当たっていない（`IsHitBossL/R()`）」**に書き換える。反射前・反射後どちらも有効で、ボスに当たったら無効（先端がボスの中で止まるので、無効にしないと毎フレーム大ダメージになる）。
  2. `BossBeamTipCollider::OnCollision()`: 今は「相手が`Counter`以外なら`return`」しているので、相手が`BossDamage`のとき自分の側の`OnHitBossL()`/`OnHitBossR()`を呼ぶ分岐を追加する。
  3. `CollisionManager.cpp`の`hit_pairs`に`{ BossBeamReflect, BossDamage }`を追加。
  4. `BossDamageCollider::OnCollision()`: 相手が`BossBeamReflect`なら大きめの固定ダメージ（定数は`namespace`に）で`TakeDamage`。
  5. `BossBeamState::Update()`: ボスに当たった側は先端をその場で止め（向きの更新・移動をしない）、フェードの経過フレームを数えて`SetColorPlayingEffekseer3DEffect(ハンドル,255,255,255,アルファ)`でアルファを下げ、0で`StopEffekseer3DEffect`してハンドルを-1に。-1のハンドルには位置・向きを設定しない。
  6. 実機確認: パリィ→ボスへ曲がる→命中で1回だけ大ダメージ→軌跡ごと薄くなって消える。曲がり具合は`one_frame_turn_angle`で調整。
- 運用: 手順が合意済みのタスクでは、質問を重ねず箇条書きでやることを出す(ユーザー要望、2026-09-30)。**NOTES.mdに進捗を書くときは、書く前にコードを読んで実際の進み具合を確認すること。**

### 進捗（2026-10-01・海面の水しぶき`WingSpray`のリアル版をv2に改修）

- 指摘: 「霧が強すぎて、ただ生まれているように見える。切り裂いている感じがほしい」。
- **原因（描画して確認）**: ゲームのカメラ(機体の300後方+追従遅れ、海面から約200上、視野角90°、水平に前を見る)に対し、発生点は翼の250後方なので**画面の下端ぎりぎり**。後ろへ流れる粒子はすぐカメラの後ろへ消え、見えるのは下端から湧き上がる霧だけだった。さらにv1の筋テクスチャ(`SprayStreak.png`)はアルファが0/255の二値になっていて、筋が長方形に見えていた(生成スクリプトの不具合)。
- **v2の構成**: 海面の白い切り口線(`CutLine`) + 外側へ開くV字の引き波(`WakeV`、Y回転した親ノードの子) + 高速で細長く斜め上へ飛ぶ水の刃(`Blade`) + 筋・水滴(動きの向きに伸びる) + 後方に薄く残る霧(`MistTrail`、発生点より後ろから出す) + 薄い泡。新テクスチャ`SprayStreak2.png`/`SprayDrop2.png`/`CutLine.png`。
- ファイル: `WingSprayReal_L/R`＝v2a(控えめ、ゲームが読んでいる名前のまま)、`WingSprayReal_v2b_L/R`＝v2b(大胆: 霧ほぼ無し・刃が多く長い)、`WingSprayReal_v1_L/R`＝前回の版の退避。トゥーン版(`WingSprayToon_*`)は未変更。
- 未確認: ゲーム内での見え方(ユーザー確認待ち)。カメラ位置は翼ボーン位置を推定して再現したもの。現状`wing_splash_effect_path`は`_L`を両翼に使っているので、右翼も内側(左)へ吹く。
- 2026-10-01続き: 「v2aはもう少し霧の主張があっていい」→ 後方の霧(`MistTrail`)を濃く大きく・発生点寄りから立ち上がるようにし、さらに水の筋(`SprayJet`)の子ノード`JetVapor`で筋の軌跡から霧が膨らむようにした(筋1本あたり最大4個)。切り口線・V字の引き波・水の刃は維持。`WingSprayReal_L/R`をこの版に更新し、直前のv2aは`WingSprayReal_v2a_L/R`に退避。
- 2026-10-01続き2: 「霧はv1よりちょっと少ないぐらいでいい」→ 霧を発生点寄り(後方0.1〜0.5)から小さく出して膨らませ、後ろへ流れる速さを落として画面内に長く残すようにした(濃さ62、筋から出る霧85)。ゲームカメラ再現での明るさの合計がv1の約8割(79%、広がり96%)になるよう数値で合わせた。発生点では細いくさび形なので塊がいきなり出る見え方にはしていない。`WingSprayReal_L/R`を更新、直前の版は`WingSprayReal_v2c_L/R`に退避。
- 学び: ゲームカメラだと後方の霧はすぐ画面外に出るので、霧の「量」より「出す位置と後ろへ流れる速さ」の方が画面上の霧の多さに効く(位置を前に寄せただけで画面上の霧が2.5倍になった)。
- 2026-10-01続き3: **海に近いほど強くなるよう、動的パラメーター(入力0)に対応。** 入力0＝高度の比率(海面で0、`sea_splash_height_threshold`で1)。入力0が0のときは直前の版と完全に同じ見た目(画面上の明るさ一致を確認)なので、コード側で値を渡さなくても壊れない。式6本: 水の刃・筋・水滴・泡の発生間隔×(1+3h²)、霧の発生間隔×(1+5h²)、飛ぶ速さ(xyz×(1-0.4h/0.6h/0.35h))、霧の大きさ×(1-0.7h)、筋から出る霧の個数×(1-h²)、刃の長さ×(1-0.4h)。切り口線とV字の引き波は高度に関係なく常に出る。ゲームカメラ再現での明るさ: 高度0→100%、50→75%、100→37%、150→19%、200→15%。直前の版(動的パラメーターなし)は`WingSprayReal_v2d_L/R`に退避。
- 左右の読み分けはユーザーが対応済み(`EffectID::LeftWingSplash/RightWingSplash`、パスは`WingSprayReal_L.efk`/`_R.efk`、`UpdateWingSplash`の引数にEffectID)。上の「`_L`を両翼に使っている」は解消済み。
- **コード側は未対応(ユーザーが実装する)**: `Player::UpdateWingSplash`で毎フレーム`SetDynamicInput3DEffect(splashHandle, 0, 高度の比率)`を呼ぶ。式では`min/max/clamp`が使えないので、0〜1への切り詰めはコード側で行う。
- Effekseerの動的パラメーターのメモ: 式の変数は`@In0〜@In3`(外部入力)、`@P.x〜w`(適用前の値)、`@O.x〜w`(出力)、`@GTime`/`@PTime`。関数は`sin/cos/rand/step`のみ。動かせるのは発生間隔・寿命・位置/速度/加速度・大きさなどの数値で、**色(アルファ)は動かせない**。.efkprojでは値の中に`<DynamicEquationMin>番号</DynamicEquationMin><DynamicEquationMax>番号</DynamicEquationMax>`(最大発生数は`<DynamicEquation>番号</DynamicEquation>`)、式本体は`<Dynamic><Equations><DynamicEquation><Name/><Code/>`。有効フラグは保存されず、番号があれば読み込み時に有効になる。
### 進捗（2026-10-01・敵弾`EnemyBullet`の軌跡を延長）

- 依頼: 「敵の弾の軌跡が短いので長めに。ただし処理は重くしない」。
- 変更(`Data/Effect/EnemyBullet/EnemyBullet.efkefc`/`.efk`を上書き。元の版はgit履歴にある): 軌跡`Kiseki`(Track)の寿命9→30F、発生間隔1→3F、スプライン分割3→8。電気`Denki`(Track)の寿命11→20F、発生間隔1.4→2.2F。どちらも色をFixed→Easing(アルファ255→0、StartSlowly2)にして、尾が自然に消えるようにした(発生間隔を空けたことによる尾の段差を隠す)。
- 負荷: 弾1発あたりのインスタンス数は 9+約8 → 10+約9 でほぼ同じ。増えたのはスプライン分割の頂点数だけ(GPU側で軽い)。軌跡の長さは約3.3倍(弾速8で約70→約240ゲーム単位)。
- 反射弾(速さ5.5倍)は軌跡がかなり長くなる。発生間隔3Fのため、弾の球と軌跡の先頭に少し隙間が出ることがある。
- 未確認: ゲーム内での見え方(ユーザー確認待ち)。
- 2026-10-01続き: **「ゲーム内だと見づらく、遠近感がわからない」→ v3に作り直し**（参考: スターフォックスのリメイク版の敵弾＝明るい芯＋太い彗星の尾）。原因はゲームカメラ再現で確認した。敵は機体の2500前方から撃ってくるので、弾はほぼ正面から近づいてくる。このため細いTrackの尾は弾の後ろに隠れ、弾は小さな点のままだった。
  - 構成: `Tail`(太い尾、Track、新テクスチャ`EB_Trail.png`、寿命30F/発生3F) + `Streak`(白い芯の筋、寿命12F/発生2F) + `Ring`(通った道に残る薄いリング、寿命22F/発生5F、1.0→1.7倍に広がりながら消える) + `Glow`(大きい光、2.4) + `Core`(白い芯、0.9)。電気`Denki`は外した（ゲーム内ではほぼ見えなかった）。インスタンス数は約23（v2は約20）。
  - リングは、正面から来る弾では同心円に、斜めの弾では敵の方へ伸びる筒に見える。これで向きと距離が読める。
  - ファイル: `EnemyBullet.efk/.efkefc`＝v3b(リングあり、ゲームが読む)、`EnemyBullet_v3a_NoRing.*`＝リングなし版、`EnemyBullet_v2.*`＝直前の版の退避。
  - 学び: TrackはUがはば方向、Vが長さ方向（全体に伸びる）。`TrackSizeFor`は**尾の先(古い側)**、`Back`が弾の側。`SplineDivision`を2以上にすると、色が分割の中で補間されず縞模様になる。まっすぐ飛ぶ弾なら1でよい（軽くもなる）。
  - 未確認: ゲーム内での見え方。もっと効く奥行きの手がかりとして、海面に弾の影・映り込みを落とす方法がある（コード側で弾の高さを渡す必要あり、未提案の段階）。
- 2026-10-01続き2: **「奥行きがかなりわかりづらい」→ v4で海面に光を落とすようにした。** 動的パラメーター入力0＝弾の海面からの高さ（ゲーム単位そのまま。式の中で/75している）。`SeaGlow`（真下の海面に平らな光だまり）と`SeaTrail`（海面に残る通り道、Track）を追加。光だまりは弾との縦の距離で高さを、海面上の位置で距離を示す。通り道は、弾がどこを通ってくるかを示す。入力が1未満（コード未対応）のときは、式で1000下へ逃がして見えなくしているので、今のコードのままでもv3bと同じ見た目になる。
  - 確認: 動的パラメーターの位置の出力には、エフェクトの倍率（書き出し50×読み込み1.5）が掛かる（倍率1と50で静止値と一致することを描画して確認）。`step(edge, x)`はGLSLと同じ順番。
  - **コード側は未対応（ユーザーが実装する）**: `EnemyBullet::Update`（と、同じエフェクトを使う`ReflectedBullet`）で、毎フレーム`SetDynamicInput3DEffect(m_effectPlayHandle, 0, GetPos().y - 海面の高さ)`を呼ぶ。海面の高さ`sea_height = 100`は今`Player.cpp`の無名名前空間にある。
  - `enemy_bullet_effect_scale`(1.5)を変えたら、式の`/75`も（50×倍率に）合わせて変える必要がある。
  - ファイル: `EnemyBullet.*`＝v4（リング＋海面の光）。`EnemyBullet_v3a_NoRing.*`と`EnemyBullet_v2.*`は前回のまま。

### 進捗（2026-10-01・ヒットエフェクト`HitEffect`をリアル調で作り直し）

- 依頼: 「当たっているかわかりづらい」。敵味方共通のまま、リアル調で、元のエフェクトにこだわらず新規に作る。
- 元の版の問題（ゲームカメラ再現で確認）: 白い玉が出るだけで「弾が光った」のか「当たった」のか区別しにくい。火花は1フレームに1個ずつ15F間かけて出る（寿命100F）ため、当たった瞬間の勢いがない。前方の敵に当たったときは小さな白い点になる。
- 新構成（`HitEffect`親ノードの下、描画順）: `Smoke`（暗い煙、αブレンド、4個、22〜30F）→`Bloom`（オレンジの火球、2.4→5.0、13F）→`Ring`（衝撃波、0.5→5.0、10F）→`StreakH`/`StreakV`（十字の光条、横9・縦4.5、7F/5F）→`Sparks`（火花32本を一斉に出す、進行方向に伸びる、白→橙、12〜22F、重力あり）→`Embers`（残り火12個、18〜28F）→`Core`（白い閃光、3.2→1.8、9F）。インスタンスは1回あたり約55。
- 親ノードを毎フレーム-Z（前方）に0.08（＝ゲームの8/F、機体と`FloatingEnemy`の前進速度）動かしている。当たった相手に張り付いて見えるようにするため。止まっている物に当たったときは、少し前へずれていく。
- 新テクスチャ（`Texture/HE_*.png`、自作）: `HE_Flash`（芯の広い閃光）、`HE_Glow`（柔らかい光）、`HE_Spark`/`HE_SparkH`（縦/横の針）、`HE_Ring`、`HE_Smoke`。古い`Particle01.png`/`Ring.png`は使わなくなったが残してある。元の版はgit履歴にある。
- 確認: ゲームカメラ再現（1280×720、自機被弾＝4.25前方／敵に命中＝26前方）で、海の背景と明るい空の背景の両方を確認した。明るい空では加算の光が埋もれるが、煙の暗い塊が後に残るので当たった場所がわかる。
- 学び: Effekseerのスプライトで、固定回転のZ=90が描画に効かなかった（原因未調査）。横長の光条は、横向きのテクスチャで作った。縮小したサムネイルだけで遠くの見え方を判断しない。実際の解像度で1:1に切り出して確認する。
- 未確認: ゲーム内での見え方（ユーザー確認待ち）。

### 進捗（2026-10-01・バレルロールのエフェクト`BarrelRoll`をリアル調で新規作成、v1）

- 依頼: プレイヤーのバレルロール（RB/LB二回押し、20Fで1回転、ロール中はカウンター判定が有効）のエフェクト。リアル調、ほかはおまかせ。らしさの核は「翼端の飛行機雲の螺旋で回転を見せる＋空気の膜で防御中を見せる」。
- ファイル: `Data/Effect/BarrelRoll/BarrelRoll_v1_R.*`（右ロール用）/`_L.*`（左ロール用、回転方向だけ逆）。テクスチャは`Texture/BR_*.png`（自作: Trail/Puff/Cone/Ring/Dot）。ループなし、全体で約55F。
- 構成（描画順）: `AirRipple`（回り始めの空気の波紋、0.8→4.2倍、14F、加算）→`Spin`（20Fで360°回る親。R=+18°/F、L=-18°/F）の下に `TipR`/`TipL`（翼端、x=±1.6・z=+0.3、寿命20F）→ 各翼端に `Contrail`（Track、毎フレーム生成、生成時のみ親の影響＝空間に取り残されて螺旋になる、寿命34F）＋`Mist`（ふくらんで消える煙）＋`Droplet`（細かな水滴、加算）。`VaporSheath`（機体を包む円錐の筒の膜、Ring、内半径0.7→外半径1.6・後ろに1.8、ロール中だけ）。
- **仮定（要確認）**: 翼端の位置 x=±1.6（ゲーム単位で±80）は推定。`Player.mv1`はバイナリで翼幅を測れなかった。ずれていたら`TipX`を変えて作り直す。
- **回転方向**: R版は描画ツールで「後ろから見て右翼端が上に回る（反時計回り）」ことを確認した。コードの右ロール（`m_rotationZ`が増える）でワールドの右翼が上がる向き、と推定して合わせた。ゲームで逆だったら`_R`と`_L`を入れ替える。
- **コード側は未対応（ユーザーが実装する）**: ロール開始時（`DefaultRotationState`で`m_isStartRolling = true`にするところ）に、`m_rollDir`に応じて`_R`/`_L`を再生する。毎フレーム、位置を機体の位置に合わせる（回転は水しぶきと同じく`GetRotationY()+π`だけ。Zの回転はエフェクトの中でしているので渡さない）。飛行機雲は生成時のみ親の影響を受けるので、位置を追従させても空間に残る。
- 生成スクリプトは scratchpad の`make_br.ps1`（パラメーターで帯の太さ・α・膜の大きさなどを変えられる、`-Dummy`で検証用のダミー機体付き）。
- 未確認: ゲーム内での見え方、実際の機体の大きさとの釣り合い。

### 進捗（2026-10-01・バレルロールのエフェクトをゲームに組み込み、ビルド未確認）

- ユーザーの依頼でClaudeが直接編集（通常はユーザーが打つ方針だが今回は明示的に許可）。
- `ResourceConstants.h`に`left/right_barrel_roll_effect_path`と`_scale`(1.0f)、`ResourceLoader`の`EffectID`に`LeftBarrelRoll`/`RightBarrelRoll`と読み込みを追加。
- `DefaultRotationState`: ロール開始の2か所で`PlayRollEffect()`（`m_rollDir`でL/R選択、ハンドルは`m_rollEffectPlayH`）。`Update()`末尾で再生中は毎フレーム位置を機体に、回転は`(0, GetRotationY()+π, 0)`に合わせ、再生終了でハンドルを-1に戻す。
- 未確認: ビルド（VS上で）、ゲーム内の見え方、翼端位置(x=±80推定)と回転方向が合っているか（逆なら`_R`/`_L`を入れ替え）。
- 気になる点: ステートが切り替わる（`Exit`）とエフェクトは止まらず追従だけ止まる。必要なら`Exit`で`StopEffekseer3DEffect`する。

### 進捗（2026-10-01・ボスのビーム跳ね返し、ボス命中まで完成）

- **完成・実機確認済み(左右とも)**: パリィ → 反射 → ボスへ曲がる → 命中で大ダメージ(`BossDamageCollider`の`reflect_beam_damage=200`) → 命中した側の先端が止まり、30Fでアルファが下がって消える。
- 実装(コード確認済み): `BossBeamTipCollider::OnCollision`が相手`BossDamage`のとき`OnHitBossL/R`を呼ぶ、`IsCollisionActive()`は自分の側が`IsHitBossL/R`でなければ有効、`hit_pairs`に`{BossBeamReflect, BossDamage}`、`BossDamageCollider::OnCollision`で`BossBeamReflect`なら`TakeDamage`。`BossBeamState`は`FadeOutHitBeam(int& playH, int& fadeFrame)`(経過に応じて`SetColorPlayingEffekseer3DEffect`でアルファを下げ、30Fで`StopEffekseer3DEffect`＆ハンドル-1)、`m_hitBossFrameL/R`、命中した側は向きの更新・先端の移動をしない、ハンドル-1なら位置・向きを設定しない、デストラクタも-1を除いて停止。**この`BossBeamState`のフェード部分は、ユーザーの依頼でClaudeが直接編集した(直接編集禁止ルールの例外)。**
- ハマった点: `IsCollisionActive()`の左だけ`IsHitBossL()`ではなく`IsReflectedL()`を見ていて、左ビームだけボスに当たらなかった。左右が対のコードは片方だけ直し忘れ・取り違えが起きやすい(以前の`GetTag()`の左右逆と同じパターン)。
- 残り: 反射したビームの曲がり具合(`one_frame_turn_angle`)・大ダメージの値・フェード時間(`hit_boss_fade_frame`)は、遊んで調整する。
### 進捗（2026-10-01・スカイボックスを夜空に）

- 依頼: ゲーム全体が明るいので、スカイボックスを夜にしたい。
- `Data/Image/SkyBoxNight/`に夜版6枚を新規作成(昼の`Data/Image/SkyBox/`は残してある)。昼の画像の「赤さ」(雲ほど高い)をグラデーションで夜の色に変換して雲の形を保ち、水平線の薄い青緑のにじみ、星(上ほど多く、雲の明るいところでは隠れる)、月(正面の右上、半径38px、にじみ付き)を足した。昼の太陽(左面)のにじみは抑えた。生成スクリプトは scratchpad の`make_night_sky.ps1`(C#をAdd-Typeで埋め込み、UTF-8 BOM付きで保存しないと日本語コメントが文字化けしてコンパイルエラーになる)。
- **コード側は未対応(ユーザーが実装する)**: `Constants/ResourceConstants.h`の`skybox_*_path`6本を`Data/Image/SkyBox/`→`Data/Image/SkyBoxNight/`に変える。`SkyBox_`(末尾`_`)は以前の暗い試作版で青が強すぎるため使っていない。
- 未確認: ゲーム内での見え方。海(`WaterPS.hlsl`)は空を映すので夜空に変わるはず。ただ、ライティング(`LightingManager`の光の向き・色・環境光)や海の色は昼のままなので、機体や岩が明るすぎる場合は次の候補。
- 2026-10-01続き: **「画像の切れ目が感じられる」→ 夜版を作り直した。** 昼の元画像は全12辺の画素がほぼ一致していた(辺の平均差0.1〜1.0)のに、夜版は`up|left`=38.9、`up|front`=12.4、`front|right`=6.9と食い違っていた。原因は、太陽のにじみの抑制(左面だけ)・月のにじみ(正面だけ)・星(面ごとにばらばらに撒いた)を面の画素座標で処理していたこと。**対処: 面をまたぐ処理(太陽の抑制・月のにじみ・星)はすべて、画素ごとの3D方向から計算する形にした。** 星は方向として1回だけ置き、届くすべての面に投影してぼかす(境目の星が両方の面に出る)。作り直し後は全辺が昼の元画像と同じ水準(平均0.0〜1.5)。月の円盘だけは、ゲームのカメラが見る正面の面の上で丸くなるよう、面の画素で描く(角度で作ると画面の端寄りで楕円に伸びた)。面の対応づけ(どの辺がどの辺につながるか)は`SkyBox.cpp`の頂点定義から導き、昼の画像で一致を確認した。スクリプトは scratchpad の`make_night_sky2.ps1`(生成)と`seam_check.ps1`(12辺の食い違いを数値で出す)。
- それでも細い線が見える場合の次の手(コード側、ユーザーが実装): `SkyBox::Draw()`でバイリニア補間が面の端で反対側の画素を混ぜている可能性がある。描画前に`SetTextureAddressMode(DX_TEXADDRESS_CLAMP)`、描画後に元に戻す。
- 2026-10-01続き: **ライトの向きを月の方向に合わせた。** 月は正面(+Z)の面の(720,215)にあり、その方向は(0.332, 0.473, 0.816)。光は月から差すので逆向きの`(-0.332, -0.473, -0.816)`にした。`GameScene.cpp`と`TitleScene.cpp`に重複していた`light_direction`を、`Constants/Game.h`の`Game::light_direction`1つにまとめた(`Game.h`は`Utility/Vector3.h`をincludeするようにした)。月の位置を変えたらここも合わせること。この3ファイルの編集はユーザーの依頼でClaudeが直接行った(直接編集禁止ルールの例外)。Debug x64ビルド成功を確認(MSBuildで`NovaWing.vcxproj`を直接ビルド)。
- 見え方の注意: 月が前方なので、プレイヤーの方を向いた面(敵・岩の手前側)は逆光で暗くなる。暗すぎれば`LightingCommon.hlsli`の`ambient_light`(0.35)を上げる。光の色は、シェーダーが強さ(スカラー)しか使わないので、青白くはならない。
- 未確認: ゲーム内での見え方(ユーザー確認待ち)。
### 進捗（2026-10-01・バレルロールのエフェクトv2「渦の円盤」を追加、ゲームの読み込み先をv2に変更）

- 依頼: 「もう少し主張してほしい」。参考はスターフォックスのローリング（白〜淡い青の弧が何本も重なった渦の円盤が、回りながら広がって約0.3秒で消える）。参考動画寄りのスタイル、v1の飛行機雲と一緒に、一気に作る、できるだけ軽く。
- ファイル: `Data/Effect/BarrelRoll/BarrelRoll_v2_R/L.*`（v1は残してある）。`ResourceConstants.h`のパスをv2に変更済み。
- 追加ノード: `VortexDisk`（`BR_Swirl_R/L.png`、Blend、22F、拡大3→7、回転±14°/F）、`BigArc`（`BR_BigArc_R/L.png`、加算、2枚、拡大4→9.5、±22°/F）、`CoreFlash`（`BR_Glow.png`、加算、9F）。渦は機体の少し前（z=-0.5〜-0.6）に置いて、後ろからのカメラでは機体の奥に見えるようにした。
- 軽量化: v1のMistの生成間隔を2F、Dropletを3Fに（パーティクル数はおよそ半分〜1/3）。追加分はスプライト4枚だけ。
- 回転の向きはエディタで確認済み（R: 後ろから見て反時計回り＋弧の頭が先）。L版は弧のテクスチャを左右反転して逆回転。
- テクスチャ生成: `Data/Effect/BarrelRoll/tools/gen_vortex.ps1`（PS 5.1で実行するにはUTF-8 BOM付きで保存すること）。確認画像は`tools/review/`。
- **不具合と修正（同日）**: 最初は`.efk`を拡大率1で書き出していたため、ゲームでは1/50の大きさになり見えなかった。拡大率50で書き出し直した（v1の`.efk`が拡大率50の書き出しとバイト単位で一致することも確認）。
- 未確認: ゲーム内での見え方（大きさ・濃さ）、機体と渦の重なり方（深度）。

### 進捗（2026-10-01・海面付近の傾き制限、十字キーでの選択肢、ビーム通り抜けの反射バグをタスク化）

- **完了(コード確認済み):**
  - 海面付近で傾きを制限: `Player::IsNearSea()`(機体の中心と海面`sea_height`の距離が`sea_roll_limit_height`=150未満)、`DefaultRotationState`の右・左の傾きで、近いと目標角を0にする(174・182行目)。**バレルロールの開始を止める処理は、まだ入っていない**(ローリングボタン1回目の入力を無効にする案は未実装。低空でバレルロールすると翼端が海に入る)。
  - 十字キーで選択肢を選べるようにした: `InputManager.cpp`の`InputEvent::up/down`に`XINPUT_BUTTON_DPAD_UP/DOWN`を追加済み(全シーンが`up/down`を見ているので全部で効く)。
- **未解決タスク(ユーザー判断で後回し): 反射できなかったビームが「通り越した後に後ろから来る」ように見える。**
  - **原因(コード確認、実機での確認はまだ):** カウンター球(半径100、ローリング中だけ有効)は、先端が球に**入るとき**だけでなく**出るとき**にも判定が成立する。`BossBeamTipCollider::OnCollision`→`OnReflectLeft/Right`は、先端が球に近づいているか離れているかを見ていない。通り抜けた先端が球の出口側で触れると、法線`(先端−球の中心)`が後ろ向きになり、`Dot(v, normal) > 0`のまま`v − 2·Dot·n`でビームが前方(ボスの方向)へ反転して、ボスへ曲がる。
  - **直し方:** 反射は`Dot(v, normal) < 0`(球に近づいている)のときだけにする。実装場所のおすすめは`OnReflectLeft/Right`の先頭(法線を求めた直後に`Dot(v, normal) >= 0`なら`return`)。
  - **注意:** 無敵は`CounterCollider::OnCollision`で、`BossBeamTip`に触れた時点で付く。反射しなくても無敵だけが付いてしまうので、「近づいているときだけ」に条件を揃える必要がある。揃える方法は、①判定を`CounterCollider`と`BossBeamTipCollider`で共有する、②`OnReflect*`が「反射したか」を返して、その結果で無敵を付ける、のどちらか。
  - **併せて検討:** 反射されなかった通常ビームは、420Fの終了時に`StopEffekseer3DEffect`で軌跡ごと突然消える(命中したビームは30Fかけてフェード)。見た目を揃えるなら、通常ビームの終了にも同じフェードアウトを使える。- 2026-10-01続き: 渦の回る向きについてユーザーから質問があったが、ゲームで確認して「大丈夫」とのこと。`_R`/`_L`の入れ替えはしていない。

### 進捗（2026-10-01・次は雨のステージ。コスト表を確認）

- **方針転換:** 低優先のタスク(ビームの通り抜け反射バグ、バレルロール開始の海面制限、ブレーキ音のExit、コード整理など)はいったん放置。時間がないので、**次のステージ(雨が降っているステージ)を作る**。
- **コスト表(`NovaWing_詳細.xlsx`)の確認結果:** 期間は、プロト 9/18〜10/24、アルファ 10/25〜11/24、ベータ 11/25〜12/23、マスター 12/24〜12/30(色なし)、提出 12/31、ポートフォリオ 1月。合計112.5h・残100.5h(プロト57/残45、アルファ19、ベータ36.5)。必要ペース1.08h/日に対して、現在の平均は0.92h/日で遅れ気味。
- 雨のステージに関係するコスト表の項目: ステージ>配置(7h、S)、シェーダ>カメラレンズの水滴(8h、S)、シェーダ>海のリアル化(6h、S)、SE>雨の音(0.5h、B)、SE>ステージ変更(0.5h、A)、BGM>ステージセレクト(0.5h)、ゲームループ(ステージセレクト→ステージ1/2など)、バグ修正>ローディングを最初に全部ではなくその都度にする(3h、S)、バグ修正>エフェクトが見づらい(5h、S)。
- **コスト表の状態が古い項目(要更新):** 敵の種類(蝶・固定砲台・突進)は実装(浮遊敵・ワーム・ボス)と一致しない。「エフェクトが消えていない」はシーン切り替え時の残留を修正済み(ノート上は完了)。「横回転」「当たり判定」「敵の弾の反射」まわりも、実装済みの内容に合わせて更新が必要。更新はユーザーの了承を得てから行う。

### 設計中（2026-10-02・リソースをシーンごとにロードする、非同期ロードも検討）

- **順番の変更:** 雨のステージの前に、起動時の長い待ち時間を解消する（今は`Application.cpp`で`ResourceLoader::LoadAll()`が全部を読む）。
- **非同期ロードの理解（説明済み）:** `SetUseASyncLoadFlag(TRUE)`でDxLibのロード関数がすぐ戻り、裏で読む。読み込み自体は速くならず、待っている間も画面を動かせるようにする技術。完了確認は`GetASyncLoadNum()`/`CheckHandleASyncLoad()`。`LoadEffekseerEffect`は対象外（同期のまま）。完了前のハンドルを使わない設計が要る（`Actor`の`MV1DuplicateModel`など）。
- **重さの見立て:** `.mv1`自体は小さく、`Data/Model`の156MBはほぼ`.fbm`のテクスチャ。4096×4096が`Rock.fbm/rocks_diff_spec.png`(47MB)、`Boss.fbm/T_Mech_LOD2_M/B/N.png`。どれもゲームシーン専用。解像度を下げる手もある。計測(`GetNowHiPerformanceCount`)はまだ。
- **シーンごとの必要リソースは洗い出し済み**（タイトル/ゲーム/ポーズ/クリア/ゲームオーバー）。共有: Playerモデル＋マップ3枚・SkyBox×6・Caustics・Boost（タイトルとゲーム）、SelectBackGround・OnCursor・Decision（ほぼ全部）、ReTry系・BackTitle系・GameEnd系・ButtonA・DecideText。`GraphicID::HowToPlay`はどこからも使われていない。
- **決まった設計:**
  - シーンごとに必要なリソースを構造体でまとめ、一覧は**ResourceLoaderが持つ**。切り替え時に「今あって次に無い→解放」「次にあって今無い→読む」「両方にある→残す」（タイトル→ゲームのPlayerモデル、リトライ時のゲームのリソースは残る）。
  - 伝えるのは**SceneController**。位置は`Update()`の切り替え処理の`ResetScene`の後・`Init()`の前（古いオブジェクト・再生中のエフェクトが消えてから、新しいシーンが使う前）。
  - 次のシーンの種類は、`Scene`基底の**純粋仮想関数**で各シーンが答える（`ICollider::GetTag()`と同じ考え方、`dynamic_cast`の分岐は使わない）。戻り値のenumは`DamageSource.h`と同じく**専用の小さなヘッダ(`Scene/SceneID.h`の予定)**に置き、`Scene`とResourceLoaderの両方からincludeする。
  - 一覧の構造体は**シーンごとに別の型にせず、1種類だけ**（`SceneResources`、メンバは`std::vector<ModelID>`/`<GraphicID>`/`<EffectID>`/`<SoundID>`/`<FontID>`）。別々の構造体（`TitleResource`/`GameResource`）だと、共通かどうかの比較を遷移の組み合わせごと（タイトル⇔ゲーム、ゲーム⇔クリア、ゲーム⇔ゲームオーバー、クリア⇔タイトル）に手で書くことになるため。同じ型なら比較は1つで済む。「入っているか」は`std::find`で十分（数十個、切り替え時だけ）。
  - ResourceLoaderは`std::unordered_map<SceneID, SceneResources>`で一覧を持つ（中身は`WStringToModelID`の`table`のように初期化時にまとめて書く）。
  - **ポーズ:** `PushScene`で切り替えを通らないので、ポーズの画像・音は**ゲームの一覧に含める**。`PauseScene`は`SceneID::Pause`を返す（「ゲーム」と答えると関数の意味とずれるため）が、`Pause`はマップに登録しない。登録されていないIDが渡されたら`GetModel`等と同じく`assert`。
  - **SoundManager:** 既に`loaded`フラグがあり`Play`/`PlayFadeIn`は未ロードならスキップする。`InitData`で「読み込まれていなければ`GetSound`を呼ばず`loaded = false`」にするだけでよい（ResourceLoaderに`IsSoundLoaded`のような確認関数を追加）。
- **実装手順（合意済み、ユーザーが実装）:** ①`Scene/SceneID.h`（Title/Game/Pause/Clear/Gameover） ②`Scene`に`virtual SceneID GetSceneID() const = 0;`、5シーンでoverride ③ResourceLoaderの`Keep*`を「ID→パス(+エフェクト倍率・フォントパス)の対応表」と「IDを1つ読む関数」に分ける ④`SceneResources`構造体・`unordered_map<SceneID, SceneResources>`(Title/Game/Clear/Gameover、Gameにポーズ分を含む)・切り替え関数（読み込み済みマップにあって次に無い→解放してマップから消す、次にあってマップに無い→読む、未登録IDは`assert`）・`IsSoundLoaded` ⑤`SoundManager::InitData`の修正 ⑥`SceneController`の2か所（`Update()`の`ResetScene`と`Init()`の間、`ChangeScene()`の起動直後の分岐の`ResetScene`と`Init()`の間）で呼ぶ ⑦`Application.cpp`の`LoadAll()`を削除（`ReleaseAll()`は残す） ⑧9種類の遷移を確認（入れ忘れは`Get*`の`assert`で分かる）。
- **非同期ロードは同期版が動いてから**上に足す（ロード中の画面と、`Init()`の前に読み込み完了を待つ段階が必要）。
- 運用メモ: この設計中、質問を1つずつ重ねすぎて「実装に進めない」と言われた。設計の大筋が決まったら、細部はClaudeが決めて手順にまとめる。
- **未決定:** 非同期ロードの詳細（ロード中の画面、`Init()`の前に読み込み完了を待つ段階、`LoadEffekseerEffect`は同期のまま）。
### 進捗（2026-10-02・ボス登場演出を「WARNING→海から浮上ムービー」に変更する素材を作成）

- **ユーザー要望:** 地震で揺れている間にWARNINGを出し、その後「海の下からボスが盛り上がってくる」ムービーを再生。ムービー後の着地の衝撃揺れ→カメラズームは今まで通り。現在の「上から落ちてくる」部分を置き換える。ムービーは動画ファイル方式(Blender制作)をユーザーが選択(ゲーム内リアルタイム案もあったが不採用)。
- **作成した素材(Claude作成):**
  - `Data/Image/Warning/`: `Warning_Text.png`(1600x300、WARNINGの文字)、`Warning_SubText.png`(1600x90、A HUGE ENEMY IS APPROACHING)、`Warning_Stripe.png`(1920x72、警告ストライプ。縞の周期96pxで横にループ可能)、`Warning_Back.png`(1920x420、上下にフェードする赤い帯)。フォントはOrbitron Black。1920x1080基準での配置見本: 帯は中心y=540、ストライプ上y=330/下y=680、文字は左上(160,355)、サブ文字は(160,590)。
  - 動き方の見本: 帯が縦に開く(約0.25秒)→ストライプが左右から入ってきて横スクロールし続ける→WARNINGが0.6秒周期で点滅→サブ文字が遅れて出る→最後に帯が閉じる。
  - `Data/Movie/BossAppear.mp4`(H.264)と`BossAppear.ogv`(Theora)。1920x1080・30fps・180フレーム(6秒)、音なし。流れ: 海面が赤く光りながら盛り上がる(〜72F)→ボスが突き破って水柱(73F〜)→浮上・水が流れ落ちる→146F付近で目が強く光る→停止。ボスは最後に海面に立った状態。
  - Blender制作スクリプトはscratchpad(一時フォルダ)にあり、リポジトリには入れていない。Boss.fbxは親Emptyで回転させる必要あり(FBXのアクションがアーマチュア自身の回転をキーしているため)。正面はZ回転90度。
- **コード側(ユーザーが実装、未着手):** `BossApearState`の`Apear`(落下)をWARNING表示・ムービー再生に置き換え、ムービー終了時にボスを海面(y=0)へ`SetPos`して`Landing`へ進む想定。DxLibは`LoadGraph`で動画を読み込み、`PlayMovieToGraph`/`GetMovieStateToGraph`で再生・終了判定。
- **2026-10-02続き・ムービーを「ゲーム内の質感」で作り直し(v2)。** v1(Blenderの写実レンダー: AgX・Ocean・光る玉の水しぶき)はユーザーから「AI感が強い」との評価。原因はゲームのシェーダーと見た目の系統が違うこと。v2では以下をゲームと同じにした:
  - 空: `SkyBox::Draw`と同じ面・UV対応で6面を正距円筒画像に計算(numpy)。海の反射も同じ画像。
  - 海: `WaterVS.hlsl`のsin波5本をジオメトリノードで再現、色は`WaterPS.hlsl`の式(フレネル^20・霧800〜2000・コースティクス・泡の高さ50〜130)をノードで再現。
  - ボス: `LightingPS`/`LightingCommon`の式(ambient 0.35、法線マップ強度1.5、メタリック未設定なのでsmoothness=1)を再現。ライト方向は`Game::light_direction`。
  - 色変換なし(ViewTransform=Raw)、モーションブラー・ブルームなし。カメラはボス出現時のゲームカメラ位置(ボスの3800手前、高さ300、縦FOV90°)に固定、揺れもゲームと同じランダム方向×power(地震7、突き破り55)。
  - しぶきはゲームのEffekseer素材(SprayStreak2/SprayDrop2/Smoke/Ring)をスプライトで使用。EEVEEではParticle InfoのAge/Lifetimeが0になるので寿命フェードは使えない。
  - 1m=300ユニット換算。ボスは親EmptyのZ回転0でカメラ(-Y)を向く。60fps・360フレーム。
- **2026-10-02続き2・「赤く光って膜が盛り上がり、破れる」のをやめた(v3)。** ユーザー指摘「普通はそんな膜が破壊されるようにはならない」。海の赤い発光を0にし、盛り上がりは高さ0.45m・半径6mの低く広いうねりに変更。浮上前は泡・小さなしぶき、浮上時は爆発的な水柱ではなく、押しのけられた水があふれる程度にして、体から流れ落ちる水を増やした。水面のRingスプライトは縁がギザギザで不自然だったため削除。ボス位置は霧で空色になる距離なので、ムービー用の泡は霧の後に重ねている。カメラが低い(約1m)ため、浮上前の海面の変化は水平線上で細くしか見えない。
- **2026-10-02続き3・WARNING画像を作り直し(v2)。** 旧版(縞模様・金属グラデーション・Orbitron)は定番テンプレートの寄せ集めでゲームのUIと系統が違ったため廃止し、`Warning_Stripe.png`/`Warning_Back.png`は削除。新版はリザルトのテンプレート(`Result_Templete.png`)と同じ構成(中央の黒パネル+六角形の張り出し、左右の暗い側面パネル+アイコン、上下の太いアクセント、区切り線)を赤にしたもの。色はリザルトのDAMAGE COUNTの赤(224,64,64)、側面パネルは(72,20,22)。文字はリザルトの面取りした一筆書き風の書体に似せて、線で組んだ自作グリフ。光彩は線だけをぼかして重ねている。動画ではなく、ゲーム側で動かす前提のパーツ画像:
  - `Warning_Frame.png` 1720x480(余白込み、枠本体は1600x360)、`Warning_Text.png` 1100x240、`Warning_SubText.png` 1000x90(HUGE ENEMY APPROACHING)、`Warning_Icon.png` 180x180(左右に2回描く)、`Warning_Edge.png` 1920x1080(画面の縁を赤くする)。
  - 1920x1080基準の配置(左上座標): Frame(100,300)、Icon(165,450)と(1575,450)、Text(410,371)、SubText(460,626)、Edge(0,0)。
- **2026-10-02続き4・ボス登場演出のコード実装(ユーザーの依頼によりClaudeが実装)。** Debug/Releaseともビルド成功。起動してDxLibのLog.txtで読み込みエラーがないことも確認済み。ただし、ボス地点まで実際にプレイしての動作確認はまだ。
  - 新規`Game/UI/WarningUI`(`UIBase`継承、`New-GameClass.ps1`で追加・vcxproj登録)。`Start(totalFrame)`で表示を開始し、`IsPlaying()`で終了を判定する。GlitchPS(scanline 280)を通して描画する。演出: 画面の縁が赤く脈打つ／枠が中心から左右に開く(14F、閉じるのも同じ)／開ききったらWARNINGが素早く点滅してから40F周期で暗くなる／アイコンが脈打つ／サブテキストは22Fから左→右へワイプ。
  - `ResourceLoader::GraphicID`に`WarningFrame/Text/SubText/Icon/Edge`と`BossAppearMovie`を追加(動画もLoadGraphで読む)。パスは`ResourceConstants.h`。ムービーは`.ogv`を使用し、DxLibの「音声データのオープンに失敗」ログが出ないよう無音のvorbisトラックを付けた。
  - `GameScene`の`BossApearState`: None→Start(揺れ・地震音・WARNING開始)→Warning(揺れとWARNINGの終了待ち→`SeekMovieToGraph(0)`+`PlayMovieToGraph`)→Movie(`GetMovieStateToGraph==0`でボスをy=0に`SetPos`、地震音フェードアウト)→CameraZoom(従来通り)。`Apear`/`Landing`と、落下・着地の揺れの定数は削除。揺れの長さは2秒→3秒(`boss_appear_shake_frame = 60*3`)。ムービーは`Draw()`の最後に`DrawExtendGraph`で全画面描画。ムービー中はポーズ不可(ポーズ中も動画の再生が進むため)。
  - `BossEnemy`の初回着地判定(`m_isFirstLanding`、着地音)は落下がなくなったため使われなくなったが、コードは残している。
- **2026-10-02続き5・演出中にプレイヤーが被弾して死ぬ不具合を修正。** 演出が「揺れ2秒+落下約1秒」から「WARNING3秒+ムービー6秒」に延びたため、操作できないまま撃たれ続け、ムービーの裏でゲームオーバーになっていた(ユーザー確認済み: 被弾が原因)。`Player`に`m_isDisabled`/`IsDisabled()`を追加し、`ChangeAllStateToDisabled()`でtrue、`ChangeAllStateToNormal()`でfalseにする。`PlayerCollider::IsCollisionActive()`でdisabled中は判定を切る。ボス撃破後のズーム中も同じ扱いになり、被弾しない。
- **2026-10-02続き6・ムービーにカメラワークを追加(ユーザー要望「もっと近く、迫力を」「カメラワークがあるとかっこいい」)。** 6秒のまま6カット構成: ①0〜1.5s 海面近くから泡立つ海へ寄る ②1.5〜2.35s 少し上から泡立つ中心を見下ろす(ロール-3°) ③2.35〜3.7s 突き破り、正面の低い位置から頭をあおりで追う(衝撃の揺れ) ④3.7〜4.65s 斜め(-40°→-15°)から回り込んで全身 ⑤4.65〜5.3s 目のアップ(目が光る) ⑥5.3〜5.95s ゲームのカメラ位置・縦FOV90°まで引いて、そのままゲームにつなぐ。目の位置はボス中心から(0,-3.5,3.45)m。揺れはnoiseによる滑らかな揺れで、最後の0.5秒は揺らさない。
  - ハマった点: 低いカメラ(0.35m)が盛り上がりや波紋の山の下に潜っていた → 海面に近いカットは1m以上に置く。泡の煙スプライトは寄りのカットで浮いた綿や板のように見えたので廃止し、泡は海面シェーダー側だけで出す。
- **2026-10-02続き7・カメラの「2回引き」を解消。** ユーザー指摘: ムービー最後にゲームのカメラ位置まで引くと、ゲームのCameraZoomでもう一度寄り、さらにズーム終了後(GameCameraはズームが終わるとプレイヤー追従に戻る)プレイヤーへ引くので、寄る→引く→寄る→引くになっていた。対応:
  - ムービーの最後のカットは、目のアップから「ボス正面・ボス位置+(0,1020,-2700)ユニット(=9m手前、高さ3.4m)・水平・縦FOV90°」まで画角を広げて終わる。
  - `GameScene`のCameraZoomは`OnZoomUp(boss_appear_zoom_speed=1.0f, boss, boss_appear_camera_dist=2700, boss_appear_camera_height=1020)`で、ムービーの最後と同じ位置へ1フレームで移す。そこから既存の追従(lerp 0.06)でプレイヤーへ1回だけ引いて戻る。`boss_appear_zoom_limit`は削除。死亡時のズーム(`boss_zoom_speed`/`boss_target_offset_y`/`boss_death_zoom_limit`)はそのまま。
  - カメラが移り終わるまでの数フレーム、遠いゲーム画面がちらつかないよう、`m_isDrawBossMovie`でムービーの最後のコマを出し続け、`m_isApearBoss && !IsZoom()`になったら描画をやめる。
- **2026-10-02続き8・水しぶきを3Dの水滴に変更。** ユーザー指摘「水滴がしょぼい、縦長のテクスチャに見える」(SprayStreak2/SprayDrop2の板スプライトだったため)。UV球の粒(丸い粒と、進行方向に6倍伸ばした粒)に変更。マテリアルは空の映り込み(下向きの反射は暗い海を拾うのでzを折り返して常に空)+ライト方向のハイライト+縁の明るさ、中心を少し透かす(BLENDED)。サイズは半径0.025〜0.035m。煙(Smoke.png)のスプライトはそのまま。
  - ハマった点1: 粒の向きはパーティクルが生まれた瞬間の速度で決まり、その後は変わらない → 流れ落ちる水は最初から下向きの速度(object_align_factor z=-2.5)で出す。
  - ハマった点2: パーティクルの回転はX軸を速度に合わせるが、インスタンスとして描かれるときはオブジェクトの**Y軸**が速度方向になる → 粒はY方向に伸ばす。
  - ogvは約50MBになった(水滴の細かさのため)。
- **2026-10-02続き9・流れ落ちる水を「水流の帯」に作り直し。** ユーザー指摘「ボックスの中に縦長の楕円がいっぱいあるみたいでしょぼい」(箱の範囲から粒を均一に降らせていたため)。ユーザーの提案で参考作品を調査(メタルギア ライジングのMetal Gear RAY、エースコンバット7のアリコーン浮上。ゲームVFXでは、体から落ちる水は粒ではなく水の幕・水流メッシュ+流れるノイズの筋+下に泡としぶき、が定番)。
  - 浮上しきった姿勢でボスのメッシュから下向きの面(法線z<-0.6、高さ0.8m以上)を0.7m升目で拾い、各升目の一番低い点を水が落ちる点にする(134点→高い所を優先して48点)。
  - 各点から十字に組んだ2枚の板(長さ12m、下へ行くほど広がる)を垂らし、海面で下を隠す。BossRootの子なので一緒に浮上する。
  - マテリアル: UVのVを上端からの距離(m)にして、sqrt(距離)で加速して見えるよう下へ流すノイズの筋(太い筋+細かい泡の筋)。左右の端と上端はぼかす。drain値で水量を減らし(3.6sまで最大→5.6sでほぼ0)、だんだん細くなって途切れる。
  - 水滴は水が落ちる点から少しだけ(1500個)。箱のエミッター(PourEmitter/PourEmitterLegs)は削除。

### 進捗（2026-10-03・ResourceLoaderのh/cpp不整合を修正、リソースのシーンごとロードに再着手）

- ユーザー評価: ボス登場演出は良い感じ。次はリソースのシーンごとロード（起動時間短縮）に進める方針（10/1の合意通り）。
- **再開時に発覚した問題: `Manager/ResourceLoader.cpp`には`LoadModel`/`ReleaseModel`/`LoadEffect`/`ReleaseEffect`/`LoadSound`/`ReleaseSound`/`IsSoundLoaded`が実装済みだったが、`ResourceLoader.h`に対応する宣言が無く、ビルドエラー(C2039等、8箇所)になっていた。** 直前のコミット(`34e7381`、2026-10-02、「Resourceの読み込みをもう少し改善する必要がある」)の時点でこの不整合のまま残っていた。
- ユーザーは学校PCでここまで書いたはずと認識していたが、`git fetch`でリモート(`origin/main`)を確認しても同じ古いコミットのままで、学校PCでの続きの作業はpush(またはコミット自体)されていないことが判明。学校PC側の状況は未確認のまま、**ユーザーの指示で「`.cpp`の実装に`.h`を合わせる」機械的な整合性作業として、Claudeが直接Edit**(通常の直接編集禁止ルールの例外、設計判断を伴わない単純作業のため)。
- 追加した宣言(`private`、既存の`LoadGraphic`/`ReleaseGraphic`と同じ並び): `LoadModel(ModelID)`/`ReleaseModel(ModelID)`、`LoadEffect(EffectID)`/`ReleaseEffect(EffectID)`、`LoadSound(SoundID)`/`ReleaseSound(SoundID)`。`IsSoundLoaded(SoundID) const`は`public`の`Get*`系の下に追加(`SoundManager`から呼ばれる想定のため)。
- MSBuildでDebug x64ビルド成功を確認(エラー0件)。
- **一度Claudeが`ReleaseEffect`/`LoadSound`/`ReleaseSound`/`IsSoundLoaded`の中身を書いたが、ユーザーから「書いた覚えがない、自分で書く」と指摘があり、元の空実装(`ReleaseEffect`/`LoadSound`/`ReleaseSound`は空、`IsSoundLoaded`は`false`固定)に戻した。** `.h`の宣言追加分はビルドを通すために必要なので残した。
- **その後ユーザー自身が4つとも実装、ビルド成功を確認済み。** 内容は提示した方針通り(`ReleaseGraphic`/`LoadModel`と同じパターン)。`ReleaseEffect`/`ReleaseSound`は対象をfindして`DeleteEffekseerEffect`/`DeleteSoundMem`→`erase`。`LoadSound`は重複防止→`sound_paths`から探す→`LoadSoundMem`→格納。`IsSoundLoaded`は`m_soundHandles`に存在するかを返すだけ。
- **現状、Model/Graphic/Effect/Soundの個別Load/Release関数とIsSoundLoadedは一通り揃った。** 次は`SceneResources`/`SceneID`側の設計・実装。
- **再開して判明: ①②(`SceneID.h`、`Scene::GetSceneID()`と5シーンのoverride)は既に完了済みだった。** つまり学校での作業は実際には存在し、コミット`34e7381`の中に含まれていた(ResourceLoaderの個別Load/Release関数だけが書きかけで止まっていた)。
- **④前半(`m_sceneResources`の構築)が完了、ビルド成功確認済み。**
  - 進め方の相談: 「`GetInstance()`の中でリソース一覧の初期化もする」案をいったん提示したが、ユーザーから「`GetInstance`という名前なのにそれ以外のことをしているのはおかしい」と指摘があり、**コンストラクタで`InitSceneResources()`を呼ぶ形に変更**(`ResourceLoader() = default;`をやめて宣言のみにし、`.cpp`に`ResourceLoader::ResourceLoader() { InitSceneResources(); }`を定義)。責務が名前とずれる設計は違和感の指摘対象になりやすい。
  - `InitSceneResources()`(新規`private`関数)で、Title/Game(ポーズ分を含む)/Clear/Gameoverそれぞれの`SceneResources`(Models/Graphics/Effects/Sounds/Fonts)を組み立てて`m_sceneResources`に格納。内容はClaudeがExploreサブエージェントで全シーン・関連オブジェクトの`Get*`呼び出しを実地調査して洗い出した一覧をもとに提示し、ユーザーが書き写した。
  - **ついでに発覚・対応済み: `SoundID::WormMove`(ワームの移動音)がどのシーンからも未使用と判明、ユーザー指示でプロジェクト全体から削除。** `SoundManager.h/.cpp`の`SoundType::WormMove`・`InitData`呼び出し・音量定数、`ResourceLoader.h/.cpp`の`SoundID::WormMove`・対応表・`KeepSound`内の読み込み、`ResourceConstants.h`の`worm_move_se_path`を削除(機械的な未使用コード削除のためClaudeが直接Edit)。ビルド成功確認済み。音声ファイル本体(`Data/Sounds/Game/Worm/WormMove.mp3`)は未削除のまま残っている。
- **④完了（ビルド成功確認済み）。** `FontID`用の個別`LoadFont`/`ReleaseFont`が無かったため先に新設(`font_infos`対応表を`effect_infos`と同じ形で追加、`KeepFont()`は`LoadFont(FontID::Result)`を呼ぶだけに簡略化)。その上で`ResourceLoader.h`にテンプレート関数`ChangeResources<IDType>(prevList, nextList, loadFunc, releaseFunc)`をクラス内定義(メンバ関数ポインタ`void(ResourceLoader::*)(IDType)`を引数に取り、`std::find`で差分を見てprevのみ→解放・nextのみ→読み込み)、`OnSceneChange(prev, next)`からModel/Graphic/Effect/Sound/Fontの5回呼び出す形で実装。全てユーザーが自分で記述、Claudeは解説のみ(`std::find`の意味を含む)。
  - テンプレート関数は`.cpp`に実装を分離できず`.h`にクラス内定義で書く必要がある点、`<algorithm>`の`#include`が要る点も合わせて解説。
- **⑤完了（ビルド成功確認済み）。** `SoundManager::InitData`を修正: `loader.IsSoundLoaded(soundID)`が`false`のとき(＝そのシーンでまだ読み込んでいないサウンド)は`GetSound`を呼ばず`soundData.loaded=false`/`handle=-1`で即`return`。読み込み済みのときだけ従来通り`GetSound`→`loaded=isLoaded`→音量設定。これにより、シーンごとロードが入って一部サウンドが未読み込みの状態でも`GetSound`内の`assert`(見つからない場合に発火)に引っかからずに済む。ユーザーが自分で記述、ビルド成功。
- **⑥完了（ビルド成功確認済み）。** `SceneID`に`None`(シーンが何も無い状態、起動直後用)を追加し、`ResourceLoader::InitSceneResources()`に`m_sceneResources[SceneID::None] = SceneResources()`(空の一覧)を追加。これにより`OnSceneChange(None, next)`が「nextの分だけ読み込む」動作になり、起動直後の特別扱いが不要になった。
  - `SceneController::ChangeScene()`の`m_scenes.empty()`分岐(起動直後の初回遷移)で`OnSceneChange(SceneID::None, scene->GetSceneID())`を`ResetScene`の前に呼ぶ。
  - `SceneController::Update()`のフェードアウト完了時の分岐で、`ResetScene`より前に`m_scenes.back()->GetSceneID()`で前のシーンIDを控え(`ResetScene`で`m_scenes`がクリアされる前に取得する必要がある)、`OnSceneChange(prevSceneID, m_nextScene->GetSceneID())`を呼んでから`ResetScene`/`Init()`。
  - `#include "Manager/ResourceLoader.h"`を追加。ユーザーが自分で記述、ビルド成功。
- **⑦完了（ビルド成功確認済み）。** `Main/Application.cpp`の`ResourceLoader::GetInstance().LoadAll()`呼び出しとその前のコメントを削除(機械的な削除のためClaudeが直接Edit)。`ReleaseAll()`(終了時の全解放)はそのまま残した。`GetInstance()`の明示的な初期化呼び出しは不要(最初に`SceneController::ChangeScene`が呼ばれた時点でインスタンスが作られ、コンストラクタで`InitSceneResources()`が走る)。
- **残りタスク（10/1合意の実装手順⑧、未着手・実機確認が必要）:**
  8. 9種類のシーン遷移をひとつずつ確認する: Title→Game、Game→Pause(Push)、Pause→Game(Pop)、Game→Clear、Game→Gameover、Clear→Title(リトライ時はGame)、Gameover→Title(またはGame)、起動直後→Title、など。各遷移で「前のシーン専用のリソースが解放され、次のシーンに必要なリソースが揃っているか」「共有リソース(Playerモデル等)が無駄に再読み込み/解放されていないか」を確認する。入れ忘れがあれば`Get*`系の`assert`で気づける設計(10/1合意通り)。**次回、実機で起動してひとつずつ遷移を試す。**
- **ユーザー確認: シーンごとロードを入れても、Title→Gameに移る瞬間はまだ強く固まる。** 非同期ロード(10/2合意で「同期版が動いてから」と保留していた本題)に進む前に、**ユーザーの提案で「読み込みが重いテクスチャの解像度を先に落とす」対応に着手(2026-10-03)**。
  - 調査(PowerShellの`System.Drawing`で全`.fbm`の解像度・サイズを一覧化): `Rock.fbm/rocks_diff_spec.png`(47.3MB/4096²)が最大、`Boss.fbm`の`T_Mech_LOD2_M/B/N.png`(16.1/13.7/11MB、各4096²)、`Worm_fix.fbm`の`Worm_Metallic/Normal.png`等(5689²という変則的な大きさ、4096より大きい)も高解像度と判明。
  - **`ResourceConstants.h`に書かれているパス(`GetGraphic`経由で明示的に読む分)だけでは`rocks_diff_spec.png`や`T_Mech_LOD2_M/B.png`の参照が見つからなかった。** これらは`.mv1`モデル本体(`MV1LoadModel`)が内部のマテリアル情報から`.fbm`フォルダを自動参照して読み込んでいると考えられる(未検証だが、サイズ的に最有力)。
  - 対応: scratchpadにPowerShellスクリプト(`resize_textures.ps1`、`System.Drawing.Graphics`の`HighQualityBicubic`でリサイズ)を作成。4096²/5689²级の対象12枚(Rock 2枚、Boss 3枚、Worm_fix 7枚)を**最大2048pxにダウンスケール**(アスペクト比維持、縮小前に同スクリプトが自動でscratchpadへバックアップ)。元ファイルはgitコミット済みの状態からの変更なので、`git checkout`でも復元可能。
  - 結果: `Data/Model`合計 156MB→91MB(約42%削減)。`rocks_diff_spec.png`は47.3MB→11.3MBに。ビルド成功確認済み(テクスチャ差し替えなのでビルドへの影響はそもそも無い)。
  - **ユーザー確認済み(2026-10-03): 読み込みの固まりは「だいぶまし」になった。** 画質の劣化が気にならないかは引き続き確認中。見た目に問題があれば、`resize_textures.ps1`の`-MaxSize`を変えて再実行するか、scratchpadのバックアップから戻せる。
  - 非同期ロード自体は、この対応でも固まりが気になるようなら次の手として残っている(10/2の設計メモ参照)。

### 進捗（2026-10-03〜04・非同期ロード(LoadingScene)の土台を実装、ビルド成功）

- テクスチャ縮小後も「だいぶまし」止まりで、Title→Gameの切り替えはまだ固まる。NOTES10/2の設計メモ通り、**非同期ロード本体に着手**。
- **参考: スターフォックスリメイク版のロード画面(ユーザーが画面録画で提示)。** SPACE DYNAMICSロゴ、「TACTICAL ADVICE」のヒント枠、「SYSTEM INITIALIZING」の文字、右下に左スティックで動かせる機体プレビュー(R1/L1二回押しでバレルロールも)という構成。**今回は処理の土台のみ実装し、見た目(ロゴ画像・機体プレビュー・ヒント文表示)はプロトの段階では後回しにする方針。次回以降のタスクとして残す。**
- **決定した仕様:**
  - 専用の`LoadingScene`(画面全体を覆う)を経由させる。
  - ロード進捗は表示しない(`GetASyncLoadNum`の残数などは使わない)。「SYSTEM INITIALIZING」の文字を通常⇔半透明で繰り返し点滅させるだけ、動いている感だけ出す。
  - **ロード画面は最低1秒(60F)は表示し続ける**(あまりに一瞬だと遷移バグに見えるため)。非同期ロード自体が1秒未満で終わっても、最低時間を待ってから次のシーンへ切り替える。
  - 非同期化の対象は**モデル・画像・サウンド・フォント全部**(`LoadEffekseerEffect`だけ対象外、既存の合意通り)。
- **実装(`Manager/ResourceLoader.h/.cpp`、ユーザーが記述):** `BeginAsyncLoad()`(`SetUseASyncLoadFlag(TRUE)`)、`EndAsyncLoad()`(`FALSE`に戻す)、`IsAsyncLoading() const`(`GetASyncLoadNum() > 0`)を追加。各`Load*`関数自体の中身は変更不要(DxLib側がフラグを見て非同期/同期を切り替えてくれるため)。
- **`SceneID`に`Loading`を追加**(`Scene/SceneID.h`)。`m_sceneResources`には登録しない(`OnSceneChange`の引数として`Loading`自体が渡ることは無い設計のため、`unordered_map::operator[]`の自動空生成に任せても実害なしと確認済み)。
- **新規`Scene/LoadingScene.h/.cpp`を作成**(`.vscode`の「クラスの追加」タスクで生成、ユーザーが実装、Claudeは一貫して説明のみ):
  - コンストラクタで`prevSceneID`・本来の遷移先`nextScene`・`fadeFrame`を受け取って保持。
  - `Init()`: `BeginAsyncLoad()`→`OnSceneChange(prevSceneID, nextScene->GetSceneID())`(非同期なのですぐ戻る)。
  - `Update()`: 経過フレームと点滅用フレームを加算、`IsAsyncLoading()`が`false`かつ経過フレームが`min_loading_frame`(60)以上になったら`EndAsyncLoad()`→`m_controller.ChangeScene(m_nextScene, m_fadeFrame)`で本来の遷移先へ。
  - `Draw()`: `SYSTEM INITIALIZING`を60F周期でアルファ255⇔128に切り替えて`DrawString`(プレースホルダー、見た目は後回し方針のため最低限)。
  - `GetSceneID()`: `SceneID::Loading`を返す。
- **`Scene/SceneController.cpp`の変更(ユーザーが記述):**
  - `ChangeScene()`: 起動直後(`m_scenes.empty()`)は**今まで通り直接遷移**(ロード画面を挟まない、ユーザー判断)。2回目以降は、`ResetSceneでクリアされる前に前のシーンIDを控え`、本来の`scene`/`fadeFrame`を`LoadingScene`でラップして`m_nextScene`にセットするよう変更。
  - `Update()`: フェードアウト完了時に行っていた`OnSceneChange`の直接呼び出しを削除(`LoadingScene::Init()`内で行われるようになったため、二重呼び出しを避けた)。
  - **ハマった点(修正済み): 最初`LoadingScene`のコンストラクタに本来の遷移先`scene`ではなく自分自身`m_nextScene`(代入前はほぼ`nullptr`)を渡してしまうミスがあった。** Claudeが指摘し、`scene`に修正。放置するとロード完了後に`nullptr`へ遷移しようとして危険だった。
- ビルド成功(エラー0件)確認済み。**未確認: 実機でシーン遷移してロード画面が正しく出るか、最低1秒守られるか、固まりが改善したか。**
- **次回やること:**
  1. 実機でTitle→Game等の遷移を試し、`LoadingScene`が正しく機能するか確認。
  2. 見た目の作り込み(ロゴ画像、TACTICAL ADVICE風のヒント枠、右下の機体プレビュー+左スティック移動+バレルロール操作)。プロト期間中は後回しの合意。
  3. 9種類のシーン遷移の確認（手順⑧、非同期化の影響も含めてまだ未実施）。

### 余談（2026-10-03〜04・VS CodeのIntelliSense `#include`エラー、DxLib拡張のバグと判明）

- `LoadingScene.cpp`作成中、`#include "Manager/ResourceLoader.h"`等ワークスペース直下からの相対includeが、**ビルドは成功するのにVS Code上では「ソースファイルを開けません」「C/C++(1696)」のエラー表示になる**現象が発生。新規ファイルだけでなく`SceneController.cpp`等の既存ファイルでも同様に再現し、プロジェクト全体の問題と判明。
- **原因（拡張`mahirocreative.dxlib-devenv`のソース`dist/extension.js`を直接確認して特定）:** `settings.json`の`C_Cpp.default.configurationProvider`に設定しているDxLib拡張が、IntelliSense用のincludePathを`[DxLib SDKのパス, <ワークスペースルート>/src]`に固定して返す実装になっていた（6205〜6213行目付近）。拡張の「新規プロジェクト作成」機能が常に`src/`固定でソースを生成する仕様に合わせたものと思われ、NovaWingのような「既存の`.vcxproj`を使う」構成（ソースがワークスペース直下に複数フォルダで展開、`src/`は使わない）では、`.vcxproj`の`AdditionalIncludeDirectories`を読み取る処理が実装されておらず、ワークスペースルート自体がincludePathに含まれない。
- **発生タイミングの推定:** 拡張フォルダのタイムスタンプ(`mahirocreative.dxlib-devenv-1.2.0`、10/3 17:02作成)から、9/28に導入した1.0.0からこの会話中に自動更新(1.0.0→1.2.0)されたと推定。「昨日は発生しなかった」のはこの自動更新が原因と考えられる。
- **対処: `settings.json`の`C_Cpp.default.configurationProvider`をコメントアウトし、既存の自前`c_cpp_properties.json`(ワークスペースルートを含む正しいincludePath)にIntelliSenseを任せる形に変更。** ビルド・実行はDxLib拡張がMSBuld経由で引き続き担当するため無関係、影響なし。
- 開発者へのバグ報告文を作成し、`C:\Users\Admin\Desktop\dxlib_devenv_feedback.txt`に保存済み（ユーザー判断で、送付するかどうか・英訳するかは未定、一旦保留）。

### 進捗（2026-10-03・ビーム反射後の曲がり角度を調整、ほぼ必中だった不具合を修正）

- **ユーザー指摘: 反射したビームが「絶対に」ボスに当たる。当初の設計では「返し方が下手だと曲がりきれず外れる」想定だったはず。**
- **原因: `BossBeamState.cpp`の`one_frame_turn_angle`(反射後の1フレームあたり最大旋回角)が`DX_PI_F / (DX_TWO_PI_F * 2)`=約14.3度/Fという大きな値になっていた。** 60FPSで1秒におよそ858度分旋回できる計算で、どんな反射角度でもほぼ即座にボス方向へ補足してしまい、外れる余地がほとんど無かった。
- **最初5度/Fに変更したが、ユーザーから「一ミリも外れる気がしない」と再指摘。** 根本原因は角度上限そのものではなく、**反射〜ボス命中までの飛行フレーム数が多く、小さい角度/Fでも累積して結局補正しきってしまうこと**だと判明(ビーム速度20/Fに対し、ボスのz座標30500・CSV上の配置から、反射位置〜ボスの距離は数千ユニット単位で、飛行フレーム数は100F超と見積もられる。5度/F×100F以上=500度以上補正できてしまい、実質どんな反射角度でも必ず補足する)。
- **対策案を相談し、ユーザーが「角度上限をもっと小さくする」を選択。** 他の案(曲がれる回数に上限をつける、旋回角度の総量に上限をつける)より、既存の「1フレームあたりの上限」という仕組みを保ったまま数値だけ変えられる点がシンプルなため。
- **対処: `one_frame_turn_angle`を`DX_PI_F / 360.0f`(0.5度/F)に変更。** 100F前後の飛行時間でも最大50度程度しか補正できない計算になり、大きく外した反射角度は当たらなくなるはず。数値1箇所の調整のため、ユーザー了承のうえClaudeが直接Edit。ビルド成功確認済み。
- **未確認: 実際に遊んで、外れる場面が出るか・難易度感が良いか(今度こそ)。** 感覚に応じて`one_frame_turn_angle`をさらに調整する想定(`BossBeamState.cpp`の無名namespace内、1箇所のみ)。もし0.5度/Fでも必中なら、反射位置〜ボスの実際の距離とフレーム数を実測して、必要な上限値を逆算する方が確実。
- 注: このタスク着手時、`Game/GameObjects/Actors/Charactor`フォルダが`Character`にリネームされていることに気づいた(いつ・誰が行ったかは未確認、スペルミス修正と思われる)。

### 進捗（2026-10-03・設計レビュー：責務分離できていない箇所の洗い出し）

- ユーザー依頼: このセッションでは、責務分離ができていない場所・くそコード・設計改善点をどんどんまとめる。**コードは触らず**、指摘のみを新規`REFACTOR_NOTES.md`(リポジトリ直下)に蓄積する方針。
- 第1回の調査対象: GameScene/Player/ClearScene/PauseScene/SoundManager/CollisionManager/SceneController/Application/BossBeamState/BossEnemy/FloatingEnemy/WormEnemy/WaterManager/Stage/EnemyFactory/Fade。
- 特に重要な指摘: ①`ChangeAllStateToDisabled()`がGameSceneから毎フレーム呼ばれている ②ボスのビームステートを`dynamic_pointer_cast`してnullチェックなしで使う(クラッシュ可能性、コライダーが具象ステートに依存) ③グリッチ演出(シェーダ+cbuffer)が6か所にコピペ ④シェーダ/定数バッファの解放が全体で0件 ⑤GameSceneが演出ステートマシンまで抱えるGod Class(`BossAppearDirector`切り出し案) ⑥BossBeamStateのL/Rコピペ ⑦メニュー選択UIが4シーンに重複。
- 着手順の提案: A-1/A-2 → グリッチ共通化+解放 → BossAppearDirector → ビームL/R統合。詳細は`REFACTOR_NOTES.md`。

### 進捗（2026-10-03・敵の死亡爆発`EnemyDeath`を新規作成、v4）

- 依頼: 敵の爆発エフェクト(死亡)がしょぼいので作り直す。元のエフェクトは一切参考にしない。進め方はおまかせ（関門ごとの確認はせず一気に作り、最後に報告）。
- 方針: リアル寄り。らしさの核は「火の玉が膨らんで黒煙に変わる体積感」と「煙を引いて飛び散る破片」。夜の海の上で映えるよう、加算の光は閃光・熱の光・煙の中の火に絞り、本体の火球はαブレンドの自作フリップブックで描く。
- ファイル: `Data/Effect/EnemyDeath/EnemyDeath.efk`（ゲーム用、拡大率50で書き出し）、`EnemyDeath_v4.efkefc`（編集用の最新版）。`_v1.efkefc`・`_v2/v3.efkproj`は途中の版（比較用に残してある）。テクスチャは`Texture/ED_*.png`（すべて自作）。
- 構成（描画順、親`EnemyDeath`の下）: `Smoke`（黒煙8、10F後から、80F、3.6→7.5倍）→`HeatGlow`（熱の光）→`Fireball`（火球8、フリップブック16コマ×2F、3.4→6.2倍）→`Fireball2`（4F遅れの二次爆発6）→`FireBloom`（高温時だけの加算の火）→`InnerFire`（9F以降、煙の中でちらつく火6）→`Shockwave`（衝撃波、12F）→`Sparks`（火花36、速度方向に伸びる）→`Debris`（破片8、重力あり、子に`DebrisTrail`＝毎フレーム空間に残る煙、`DebrisBurn`＝燃える光）→`Embers`（残り火18）→`Flash`/`CoreFlash`（最初の閃光）。ピークのインスタンス数は約250。
- 生成スクリプト: `tools/gen_explosion_tex.ps1`（テクスチャ。火球は「球の集まりのなめらかな合成＋ノイズ」で形を作り、勾配から陰影、温度で黒体色、後半はノイズで侵食）、`tools/make_efkproj.ps1`（ノード構成を`.efkproj`として出力。数値はここで変える）。確認画像は`tools/review/`。
- 作る途中の学び: 球の最大値で形を作ると玉の境目がくっきり割れる→ソフトマックスでなめらかに合成。火球の縁を暗くすると「クッキー」に見える→縁の陰影を明るめに、αの縁を広く。最初は火球が衝撃波・破片の範囲に比べて小さすぎた（主役を大きく）。破片の煙は生成直後に火球の中心にかぶって汚れて見えた→5F遅らせて出す。PowerShellで関数名を`R`/`Rv`にすると既存エイリアス（`r`=Invoke-History、`rv`=Remove-Variable）と衝突する。
- **コード側は未対応（ユーザーが実装する）**: `ResourceConstants.h`の`worm_death_effect_path`と`floating_death_effect_path`を`L"Data/Effect/EnemyDeath/EnemyDeath.efk"`に変える。スケールの目安（火球の半径≒エディタ上3×50×スケール）: 浮遊敵（当たり半径132）は`1.5`のままで火球の半径約225。ワーム（節の半径40、13Fごとに連鎖）は今の`3.0`だと半径約450で大きすぎる見込みなので`1.0`前後から試す。
- 未確認: ゲーム内での見え方（大きさ・明るさ・ワームの連鎖時の重なり方）。エフェクトはワールドに置かれる（敵や機体に追従しない）ので、前進中は煙と破片が後ろへ流れていく見え方になるはず。
- **2026-10-03続き: ゲームに組み込んだ。** ユーザーの依頼でClaudeが直接編集（直接編集禁止ルールの例外）。`ResourceConstants.h`の`worm_death_effect_path`/`floating_death_effect_path`を両方`EnemyDeath/EnemyDeath.efk`に変更、`worm_death_effect_scale`を3.0→1.0（浮遊敵は1.5のまま）。同じ`.efk`を別スケールで2回`LoadEffekseerEffect`している（ヘッダにキャッシュの記述はなく、呼ぶたびに読み込む前提。もし両方同じ大きさに見えたら、ここを疑う）。Debug x64ビルド成功（MSBuild）。旧`Exprosion`/`Exprosion2`フォルダは参照されなくなったが残してある。未確認: ゲーム内での見え方。

### 進捗（2026-10-03・チャージ中`Charging`とチャージショット`PlayerChargeBullet`を作り直し、v4）

- 依頼: チャージ中のエフェクトを参考動画（スターフォックスのチャージ）を観察して良くする。チャージショットも作り直し（こちらは動画にこだわらず、かっこよければ良い）。2つの雰囲気を揃える。進め方は一気に作って最後に報告。
- 動画の観察: 機首の前に白っぽい黄緑の芯＋鋭い十字の光、その周りを半透明の緑のプラズマの筋（煙のように縁が明るい）が渦を巻く。小さな放電が走る。
- 方針（らしさの核）: 「渦を巻いて縮みながら集まるプラズマ」＋「チャージ完了の瞬間の一拍（リング＋閃光＋火花）」。弾は「溜めた玉がそのまま飛んでいく」＝同じ素材の玉が、空間にプラズマの筋を残して飛ぶ。色は通常弾`PlayerBullet`と同じ（芯235,255,238／緑30,255,110）。
- コードに合わせた点: `charge_comp_frame = 20`に合わせ、0〜20Fで成長、20Fで完了演出（`ChargeComplete`の音と同時）、以降は無限ループ。エフェクトに回転は渡されないので、全部ビルボード（向きに依存しない）。離したときの`SetScalePlayingEffekseer3DEffect`の縮小が効くよう、チャージ側の粒は親に追従（Always）。弾は毎フレーム位置だけ渡されるので、尾（Track・プラズマの筋・粒）は「生成時のみ」親に従い、空間に残る。
- ファイル: `Charging/Charging.efk`・`.efkefc`、`PlayerChargeBullet/PlayerChargeBullet.efk`・`.efkefc`を上書き（拡大率50で書き出し）。元の版はそれぞれ`Charging_v0/`・`PlayerChargeBullet_v0/`に退避。途中の版は`Charging_v1〜v4.efkproj`、`PlayerChargeBullet_v1〜v4(.._preview).efkproj`。`_preview`は確認用に親へ+Z速度を入れたもの（**ゲームには使わない**）。
- 生成スクリプト: `Charging/tools/gen_charge_tex.ps1`（テクスチャ`CS_*`。両エフェクト共通で`Charging/Texture`と`PlayerChargeBullet/Texture`に置く）、`Charging/tools/make_efkproj.ps1 -Version N [-PreviewMove]`（2つのエフェクトを同じ部品から出力。数値はここで変える）。確認画像は`Charging/tools/review/`。
- 構成（Charging、描画順）: Backing（暗い下地・通常合成、明るい海の上でも締まる）→Halo→Wisp（一本の三日月形のプラズマの筋、回転しながら2.8→0.9倍に縮む）→WispCharged（20F以降、逆回転）→Inflow（ランダムに向けた親の子が中心へ飛ぶ火花）→Arc/ArcCharged（玉の表面の放電）→Core/CoreShimmer→Star→ChargedRing/ChargedFlash/ChargedSparks（20Fの一拍）。成長が必要なものは「20Fの成長用ノード」と「20Fから出る保持ノード」の2つに分けた。
- 構成（PlayerChargeBullet）: 空間に残る Trail（通常弾と同じ`BulletTrail_v1b.png`のTrack）・WakeWisp・WakeSpark、玉は Backing・Halo・Wisp×2（速く回る）・Arc・Core・Star、発射時の LaunchRing/LaunchFlash（弾の生成位置＝機体の中心なので小さめ）。
- 学び: **スプライトの配置方法が既定の「ビルボード」だと、Zの回転が一切効かない。回転させるなら「Z軸回転ビルボード」（XMLでは`<Billboard>3</Billboard>`）。** 10/1の`HitEffect`で「固定回転のZ=90が効かなかった」原因もこれと思われる。渦の全部入りテクスチャを回すと、どの角度でも同じ形に見えて「ロゴ」になる→一本の筋をランダムな角度で重ねると、不規則なプラズマの玉になる。PowerShellの関数名`Gc`は`Get-Content`のエイリアスと衝突する。
- 気づいた点（コードは未変更）: ①`ChargeReadyState`は、ボタンを離した瞬間から`m_canShrink`で玉が縮み始め、約14Fで消える。発射できる猶予は60Fあるので、離してから撃つまでの間は玉が見えない。②`PlayerBullet.efkefc`の`Bullet`ノードに位置の速度Z=0.6が入っている。書き出した`.efk`にも入っているなら、ゲーム内で弾の見た目が毎フレーム位置を設定される弾本体より前へずれていく可能性がある（未確認）。
- 未確認: ゲーム内での見え方（大きさ・明るさ・海の上での見やすさ）。命中時の爆発は弾のエフェクトでは出せない（`OnHitEnemy`で停止するため）。出すなら命中位置で別エフェクトを再生するコードが要る。- **2026-10-03続き: ゲームでチャージした瞬間にEffekseerの`Easing.h`(356行目)でアサート。** 原因は`Wisp`の拡大のイージングの種類に、存在しない番号`13`を入れていたこと。XMLの番号は「10の位が曲線、1の位がIn(0)/Out(1)/InOut(2)」（20=EaseInCubic、21=EaseOutCubic、31=EaseOutQuartic。エディタの表示名で確認済み）。**エディタは不正な番号でもエラーを出さず、ゲーム側でだけ落ちる。** `20`に直して`Charging.efk`を書き出し直した(v5)。チャージショット側は`21`/`31`しか使っていないので変更なし。- **2026-10-03続き2: チャージショットに「分身」を追加(v10)。** ユーザー依頼: 前のチャージショット(本体＋分身が散って集まる)をまねしてよい、本当は敵に当たるときに集まってほしかった、重くしないで。
  - 構成: `PhantomArm`(5体、向きはランダムに固定、発射後12FでFCurveにより0→1倍に広がる)の子に、`PhantomTrail`(Track、空間に残る)・`PhantomHead`・`PhantomStar`(寿命1F、毎フレーム出し直す)。位置は動的パラメーターの式`PhantomOrbit`で計算: 半径2×(1−入力0)×(cos, sin)(@GTime×17)。弾が進むので、らせん状の光の尾になる。`GatherGlow`/`GatherCore`は大きさ×入力0²で、集まるほど芯が膨らむ。
  - **入力0＝集まり具合(0＝散っている、1＝本体に重なる)。** 0(コード未対応のとき)は散って周回し続けるだけなので、今のコードのままでも壊れない。
  - **コード側は未対応(ユーザーが実装する)**: `ChargeBullet::Update`で、ターゲットが生きている間、毎フレーム`SetDynamicInput3DEffect(m_effectPlayHandle, 0, 集まり具合)`。集まり具合＝(集まり始める距離−ターゲットまでの距離)/(集まり始める距離−集まり終わる距離)を0〜1に切り詰めたもの(式でmin/maxが使えないので、切り詰めはコード側)。目安: 弾速25/Fなので、始める距離600(約24F前)、終わる距離はターゲットの当たり半径＋弾の半径32くらい(浮遊敵なら約160)。
  - 学び: **「生成時のみ」で親に従う子は、親の回転が時間で変わっても、それに沿って出る位置が回らない**(乱数の種を固定して確認。「常に」で従う子は回る)。**円周発生の半径には動的パラメーターが効かない**(大きさ・位置には効く)。**`@GTime`は秒**(×17で約16°/F)。キャプチャは毎回乱数が変わるので、比較は`@effect`の`Global.RandomSeed`を固定してから行う。動的入力の値はMCPから変えられないので、`.efkproj`の`<DynamicInput><Input>`を書き換えた確認用ファイルで見た。

### 進捗（2026-10-04・DxLib拡張が「プロジェクトではない」と誤判定してF5が失敗する不具合、原因特定・解決）

- **症状: F5(DxLib: デバッグ実行)が`preLaunchTask 'DxLib: Debug ビルド' が終了コード1で終了しました`で失敗。** ターミナルには「NovaWing.vcxproj はこの拡張機能が作ったものではないので使えません。名前を変えるか移動してください。」という内容のバッチ(`%APPDATA%\Code\User\globalStorage\mahirocreative.dxlib-devenv\build\NovaWing_*_debug.bat`)が実行されていた。MSBuildで直接ビルドすると成功する(コード自体は正常)ため、拡張側の問題と判断。
- **原因（`dist/extension.js`を直接読んで特定）:** 拡張のプロジェクト判定関数`isDxLibProject`は「`.vscode/tasks.json`に`type:"dxlib"`のタスクがあり、**かつ** `adoptedVsProject`(＝`.vscode/settings.json`の`dxlib.vsProject`が設定されているか)が偽であること」を見ている。`adoptedVsProject`は`settings.json`を**`JSON.parse`で厳密パース**しており、**`settings.json`にコメント(`//`)が含まれているとパースに失敗し`undefined`を返す**。NovaWingの`.vscode/settings.json`は元々(gitにコミット済みの時点から)コメント入りだったため、`dxlib.vsProject`の設定が拡張から見えなくなり、プロジェクトとして認識されずビルドタスクが失敗していた。
- **発生タイミングの推定: 10/3の自動更新(1.0.0→1.2.0)で、`settings.json`の読み方(JSONC寛容パース→厳密`JSON.parse`)が変わった可能性が高い。** 9/28〜10/3朝まではコメント入りのままF5が機能していたとみられるため。
- **対処: `.vscode/settings.json`から全コメントを削除し、正しいJSON(コメントなし)に書き換え。** 内容（コメントで説明していた理由付け）はこのNOTESに移した。`C_Cpp.default.configurationProvider`の行自体も、前回判明したIntelliSenseのバグ対応のため削除済みのまま残している(settings.jsonにはDxLib拡張用の設定しか残っていない)。
- MSBuildでの直接ビルドは一貫して成功していた(エラー0件)。**未確認: この修正でF5(DxLib: デバッグ実行)が実際に成功するか**、実機でユーザーが確認予定。
- **教訓: `.vscode/`配下の設定ファイル(`tasks.json`/`launch.json`に続き`settings.json`も)にコメントを書くと、拡張がJSONとして厳密パースしている場合に壊れることがある。** VS Code自体はJSONC(コメント許容)として読むため、エディタ上は問題なく見えてしまうのが厄介。同様の不具合が今後起きたら、まず`.vscode/*.json`にコメントが混ざっていないか疑うとよい。
- **続報: `settings.json`修正後もF5が`preLaunchTask 'DxLib: Debug ビルド' が終了コード1で終了しました`で失敗し続けた。** 生成されたバッチの中身を確認すると「NovaWing.vcxprojはこの拡張機能が作ったものではない」というエラーのまま。`extension.js`をさらに読み込み、**`writeBuildScript`(ビルド用batを書く関数、4902行目〜)が`isDxLibProject(folder)`の結果だけを見て即座に失敗扱いにしており、`dxlib.vsProject`で「既存VSプロジェクトを使う」モードを選んでいるケース(`isDxLibProject`が意図的にfalseを返す設計)を一切考慮していないことが判明。** これは1.0.0→1.2.0の自動更新で`writeBuildScript`側の分岐が`isDxLibProject`の仕様変更に追従していない、**拡張自体の退行バグ**と判断。
- **対処: 拡張を1.2.0からアンインストールし、`Downloads\DxLib-devenv-1.0.0\DxLib-devenv-1.0.0\dxlib-devenv-1.0.0.vsix`から1.0.0を再インストール(`code --install-extension <path> --force`)。拡張機能パネルから「DxLib 開発環境」の「Auto Update」のチェックを外し、今後の自動更新を無効化した(「Auto Update All (From Publisher)」は外せなかったが、個別のAuto Updateオフが優先される認識)。** その後VS Codeを完全に再起動してF5(DxLib: デバッグ実行)が通ることを確認済み。
- **副作用: ダウングレード〜再起動の過程でVS Codeの表示言語が一時的に英語に戻った(`argv.json`が無い状態だった)。** コマンドパレットの「Configure Display Language」からjaを選び直して解決、再起動後は大部分が日本語表示に戻った(右クリックメニュー等、一部の項目は翻訳リソースの都合で英語のまま残るが実害なし)。
- 開発者へのバグ報告(`C:\Users\Admin\Desktop\dxlib_devenv_feedback.txt`)には、この`writeBuildScript`の退行バグも追記が必要(未追記、次回気が向いたら)。

### 進捗（2026-10-04・LoadingSceneが無限ループしてタイトル→ゲームの遷移で画面が真っ暗なまま固まるバグを修正）

- **症状: DxLib拡張のビルド不具合解消後、実機でタイトル→ゲーム遷移を試したところ、画面が真っ暗なままずっと戻らない。**
- **原因(コードレビューで特定): `SceneController::Update()`の`if(!m_fade.IsFading() && m_nextScene != nullptr)`分岐は、`LoadingScene`へ切り替わった後も当然また毎フレーム素通りする(`m_nextScene`はLoadingScene切替時に`nullptr`に戻るので2回目以降はelse節＝`LoadingScene::Update()`が正しく呼ばれる)。問題は`LoadingScene::Update()`がロード完了時に呼んでいた`m_controller.ChangeScene(m_nextScene, m_fadeFrame)`の方。** `ChangeScene`は「本来の遷移先をまた`LoadingScene`で包んで`m_nextScene`にセットする」関数なので、LoadingScene完了のたびに新しいLoadingSceneが生成され続ける**無限ループ**になっていた(本来のゲームシーンには永遠に辿り着けない)。
- **対処: `SceneController`に`ChangeSceneDirect(scene, fadeFrame)`(LoadingSceneを経由せず`ResetScene`→`Init()`→フェードイン開始するだけの関数)を新設し、`LoadingScene::Update()`の呼び出し先を`ChangeScene`から`ChangeSceneDirect`に変更。** ユーザーが自分で記述、ビルド成功確認済み。
- **未確認: 実機でタイトル→ゲームの遷移が正しく完了するか(LoadingScene表示→ゲームシーンへ到達)。**

### 進捗（2026-10-04・LoadingSceneの文字が出ない問題を調査、原因は単純な座標ミスと判明。終了時クラッシュも修正）

- **症状1: タイトル→ゲーム遷移で、LoadingSceneの「SYSTEM INITIALIZING」の文字が一切見えない。**
- **調査の過程(長くなったので記録): まず`ResourceLoader::OnSceneChange`に計測コード(`GetNowHiPerformanceCount`で各カテゴリの所要時間をデバッグコンソールへ出力、デバッグ後に削除済み)を一時追加して測定。結果、Model/Graphicはほぼ0ms(非同期化が効いている)だが、`Effect: 481ms`(想定通り、`LoadEffekseerEffect`は仕様上非同期対応外)、`Sound: 470〜1330ms`(想定外に重い)と判明。**
  - `min_loading_frame`を伸ばしても直らないか検討したが、「`LoadingScene::Init()`内の`OnSceneChange`自体がブロッキングしている間はそもそも`Update`/`Draw`のループに入れない」ため、`min_loading_frame`(Update開始後のカウント)をいくら伸ばしても無意味と判明(ユーザーからの「伸ばせば直るのでは」という指摘に対して、`Init()`と`Update()`の呼び出しタイミングの違いを説明)。
  - Sound/Effectの呼び出し順(`ChangeResources`内でEffect→Soundの順)を入れ替えて「Effect読み込みがSoundの非同期化を阻害しているのでは」という仮説を検証したが、効果なし(のちに元の順序に戻した)。
  - ユーザーの提案で、参考プロジェクト`C:\Users\Admin\Documents\GitHub\ProjectNeaR`(別PCの`SakamotoKou`名義だが、このPCにも別途クローンが存在)の`LoadingManager`/`AssetManager`を調査。**学んだこと**: ProjectNeaRは「シーンの`Init()`内で`StartLoading()`(非同期フラグON)→通常の重い初期化処理→`EndLoading()`」という、NovaWingの`OnSceneChange`一括呼び出しとほぼ同じ構造。決定的な違いは、ロード画面の`Update`/`Draw`が**`SceneController`を経由せず`Application::Run()`のメインループから独立して呼ばれる**設計だったこと(`if (!loadingManager.IsLoading()) sceneController->Update()` のように、ロード中はシーン側の更新・描画を止めて`LoadingManager::Draw()`だけ呼ぶ)。また`AssetManager::GetEffectHandle`は`LoadEffekseerEffect`の前後で明示的に非同期フラグを一時オフ/オンしており、「`LoadEffekseerEffect`は非同期非対応」という認識はNovaWingの合意と一致することが確認できた。サウンド(`GetSoundHandle`)は特別な処理なく素直に`LoadSoundMem`を呼んでおり、コード上の違いは見つけられなかった。
  - **最終的な原因判明: `LoadingScene::Draw()`の`DrawString(800, 950, ...)`が、Debugビルドの画面サイズ(`Constants/Game.h`の`screen_width/height` = 1280×720、Release/基準は1920×1080)を超える座標(Y=950)を指定しており、単純に画面外に描画されていた。** Sound/Effectが重いこと自体は事実として残っているが、「文字が見えない」問題とは直接関係なかった。
- **対処: 座標を`(50, 50)`(画面左上)に変更。** ビルド成功、ユーザー確認で文字が表示されることを確認済み。
- **症状2(ついでに発覚・修正済み): リザルト画面の数値カウントアップ演出中にESCキーでゲームを終了すると、`DxLib_End()`実行中にアクセス違反でクラッシュする。** 原因は、非同期ロードフラグが立った状態(またはロード中のハンドルが残ったまま)で`ResourceLoader::ReleaseAll()`→`DxLib_End()`に入っていたため。`ReleaseAll()`の先頭、`EndAsyncLoad()`より前に`WaitHandleASyncLoadAll()`(DxLibの「全ての非同期読み込みが完了するまで待つ」関数、ProjectNeaRの調査で存在を知った)を追加して解決。ユーザーが自分で記述、ビルド成功・実機で再発しないことを確認済み。
- **教訓: 複雑な原因(非同期ロードの仕組み、呼び出し順序など)を疑う前に、まず単純な原因(座標・解像度のズレ等)を先に確認すべきだった。** 今回はDebug/Releaseで解像度が違う(`screen_width/height`)ことを忘れ、Release基準の座標をそのままDebugで確認してしまったのが遠回りの原因。
- **Sound/Effectの重さ自体(Sound 470〜1330ms、Effect 481ms)はまだ残っている課題。** 座標ミス修正でLoadingSceneの文字は見えるようになったので、実用上の支障(画面が固まって見える)は`min_loading_frame`(今240=4秒に変更済み)の間に収まっていれば目立たなくなったはずだが、根本的な軽量化(Soundの非同期化の原因特定、またはフレーム分割ロードへの設計変更)は未着手のまま。気になるようなら次回再検討。

### 進捗（2026-10-04・タイトルから「ゲームを終了」でDxLib_End()実行中にアクセス違反クラッシュする不具合、根本原因を特定・修正）

- **症状: タイトル画面で「ゲームを終了」を選ぶと、`Application::Terminate()`の`DxLib_End()`実行中に`0x0000000000000000`番地への書き込みアクセス違反でクラッシュ。** 前回(リザルト画面でのESC終了時)に`WaitHandleASyncLoadAll()`を追加して直ったのとは別経路・別原因のクラッシュ。ユーザーから「全任せする、絶対に直してほしい」と依頼され、Claudeが主体的に調査・特定・修正まで実施(通常の直接編集禁止ルールの例外、ユーザー明示の委任による)。
- **調査の過程:**
  - まず`ReleaseAll()`周りを再確認したが既に対策済みで問題なし。
  - Exploreサブエージェントに「`PlayEffekseer3DEffect`の呼び出し元で`StopEffekseer3DEffect`を呼んでいないクラスを洗え」と依頼。`TitlePlayer`(`TitlePlayer.cpp`、ブーストエフェクト)と`DefaultRotationState`(バレルロールエフェクト)がデストラクタ・`Exit()`で停止処理をしていないと判明、修正(`.lock()`でガードしつつ`Stop`を追加)。ただし`TitleScene.h`のメンバー宣言順(`m_pPlayer`→`m_pEffectManager`)により、**デストラクタでは`m_pEffectManager`が先に破棄され`EffectManager::~EffectManager()`の`StopAll()`で全エフェクトが既に止まっているはずのため、この修正自体は理屈上「手遅れ」で根本原因ではない可能性が高いと判断**(ただし無害なので修正は残した)。
  - **本命の原因を特定: `Game/GameObjects/Actors/Actor.cpp`の`Actor::~Actor()`が「処理なし」の空実装で、コンストラクタで`MV1DuplicateModel`して複製したモデルハンドル(`m_modelHandle`)を一度も`MV1DeleteModel`で解放していなかった。** `Actor`は`Player`/`Rock`/`FloatingEnemy`/`WormEnemy`/`BossEnemy`/`TitlePlayer`/`Stage`全ての基底クラスであり、ゲーム全体で複製モデルハンドルが解放されずリークし続けていた。終了処理で元モデル(`ResourceLoader::ReleaseAll()`)を`MV1DeleteModel`した後、DxLib内部のモデルハンドルテーブルに「親が消えたのに子の複製ハンドルがまだ生きている」不整合が残り、`DxLib_End()`の内部後片付け処理でこれを踏んでクラッシュしていたと推測される。
  - **対処: `Actor::~Actor()`に`MV1DeleteModel(m_modelHandle);`を追加。** 1行の修正。ビルド成功確認済み。
  - ついでに、`TitleScene`/`GameoverScene`等のデストラクタが「シェーダーピクセルハンドル(`m_glitchPSH`)・定数バッファ(`m_cbufferGlitch`)」も解放していない(空デストラクタ)ことに気づいたが、これは単体では即クラッシュに直結しにくい軽微なリークと判断し、今回は未対応のまま(気になれば次回対応)。
- **ユーザー確認済み(2026-10-04): 「ゲームを終了」選択時のクラッシュは再発せず、良い感じとのこと。** `Actor::~Actor()`への`MV1DeleteModel(m_modelHandle)`追加が根本対応として有効だったと判断。
  - 負荷: 弾1発でピーク約154インスタンス(前の版は約95)。チャージ中は約58。Trackの点は頂点だけなので軽い。分身の頭と星を寿命1Fにし、尾を12Fにし、渦と粒の発生間隔を広げて抑えた。- **2026-10-03続き3: 分身の集まりをコードに組み込んだ。** ユーザーの依頼でClaudeが直接編集(直接編集禁止ルールの例外)。`ChargeBullet.cpp`: 無名名前空間に`gather_start_dist = 600`・`gather_end_dist = 160`・`gather_input_index = 0`、`Update()`でターゲットが生きている間はターゲットまでの距離から集まり具合(0〜1、`std::clamp`)を計算し、毎フレーム`SetDynamicInput3DEffect`で渡す。ターゲットがいない・途中で死んだときは0(分身が散った状態に戻る)。`GameObject`に当たり半径が無いので、集まり終わる距離は浮遊敵(当たり半径132＋弾32)に合わせた定数。Debug x64ビルド成功。未確認: ゲーム内での見え方、ワームやボスなど当たり半径が違う敵での集まるタイミング。

### 進捗（2026-10-04・EffectManagerを新設し、Effekseer直接呼び出し全箇所を移行）

- **ユーザーの依頼でClaudeが直接編集(直接編集禁止ルールの例外)。方針は事前に3点確認: ①再生ハンドルは`int`のまま、②渡し方は`weak_ptr`(SoundManagerと同じ)、③移行範囲は全箇所一気に。**
- **新設: `Manager/EffectManager.h/.cpp`。** `Update()`(Sync3DSetting+UpdateEffekseer3D)、`Draw()`、`Play(EffectID,pos)`→ハンドル、`PlayOneShot(EffectID,pos)`、`SetPos/SetRotation/SetRotationAxis/SetScale/SetColor/SetDynamicInput`、`IsPlaying`、`Stop`、`StopAll`。デストラクタで`StopAll()`するので、シーン破棄(リトライ・遷移)でエフェクトが残らない。`SetRotationAxis`はボスのビームがEffekseer本体の`SetRotation(軸,角度)`を直接呼んでいたぶんの置き場。
- **所有と受け渡し:** `GameScene`/`TitleScene`が`shared_ptr<EffectManager>`を持つ。GameSceneでは`BulletManager`のコンストラクタ引数、`Player`、`BossEnemyDataSetter`/`FloatingEnemyDataSetter`/`WormEnemyDataSetter`/`EnemyFactory`の引数に追加。`TitlePlayer`はコンストラクタ引数。プレイヤーの各ステート(Boost/DefaultRotation/ChargeReady/ChargeShoot)は`Player::GetEffectManager()`、ボスの各ステート(Beam/Summon)は`BossEnemy::GetEffectManager()`経由で取得(ボスのSoundManagerと同じ流儀で、ステートのコンストラクタは変えていない)。弾は`BulletBase`が`m_pEffectManager`を持つ。
- **デストラクタでの`Stop`は`lock()`して生きていれば止める形にした。** シーン破棄時にマネージャーが先に消えるので、`lock()`の結果を見ずに呼ぶとヌル参照になるため。`BossBeamState`だけは、ボスより先に自分が破棄されるときボス経由でマネージャーを取れない(`m_pBoss.lock()`が空)ので、`Enter()`で自分用に`weak_ptr`を保持している。
- **挙動が少し変わった箇所(`PlayOneShot`化。再生ハンドルを持つ意味が無かったもの):** 被弾ヒット、ボスの水しぶき/シールド/死亡、ワームの死亡(最後の1個がデストラクタで途中で切られなくなり最後まで再生される)、ボスの召喚(浮遊/ワーム)。浮遊敵の死亡エフェクトは敵に追従させているので従来通りハンドル保持のまま。
- **副作用: `EffekseerForDXLib.h`の間接include(`<vector>`など)が消えたので、`BossBeamState.h`に`<memory>`/`<vector>`を足した。** MSBuild(Debug x64)でビルド成功、エラー0件。
- **未確認: 実機での見た目の確認(弾・ブースト・チャージ・ビーム・水しぶき・バレルロール・タイトルのブースト)、リトライ・シーン遷移でエフェクトが残らないか、Releaseビルド。**

### 進捗（2026-10-04・海のリアル化の実験）
- **目的**: スターフォックス(昼・海)の質感に近づける。ユーザー方針: 空の反射は弱めで水の色主体、キラキラ(細かい法線)と泡の筋で見せる。
- **現状の原因分析**: 法線を頂点シェーダー(40×40グリッド、1マス約250×750)で計算しPSに補間して渡している→細かい凹凸が出ない。泡は高さだけで純白lerp、スペキュラはPhong1発、フレネル指数20で空反射はほぼ地平線のみ。ValueNoiseは過去に試して「なだらかなコブ」になり不採用。
- **実験(Claudeが直接編集・ユーザー許可済みの例外)**: C#で生成したタイリング法線マップ(512px)をPSで2枚スクロール合成(UDN)、空反射×0.4、specular power200/strength1.5。**結果: 中央に変な線が入っただけで不採用**。原因の見立て: 夜シーンなので月のスペキュラが power200 で細い筋(glitter path)になった。参考画像は昼で太陽光が前提。法線マップ自体も絵柄が合わない可能性。
- **実験は全て元に戻した**(ユーザー依頼「実装が終わったら必ず直す」)。WaterPS.hlslのタイムスタンプを更新したので次回VSビルドでWaterPS.psoも再生成される。実験版のコピーはスクラッチパッドのみ(リポジトリには残していない)。
- **次回**: 夜のシーンで昼の参考画像に寄せること自体の方針確認(ライト/月/空のどこを主役にするか)、法線マップの素材選び、泡の筋パターン。ユーザー主導で設計→Claudeがレビュー。

### 進捗（2026-10-04 続き・海のリアル化の実験と原状復帰）
- **結論: 実験は全て元に戻した(ソースは実験前と同一、MSBuild Debug x64でビルド成功を確認済み)。** ユーザーが実験6/7の内容を見て自分で作り直す予定。
- **比較方法**: 参考画像(スターフォックス)と自分のスクリーンショットを、水面を水平線〜下端で5分割した帯ごとに「平均色／輝度／輝度>0.55の画素割合／縞の向き(横方向と縦方向の勾配の比 gx/gy)」で数値比較し、拡大画像も目視した。
- **原作の数値**: 手前〜中景の平均色 約(0.03,0.27,0.38)・輝度0.22。水平線のすぐ下は輝度0.35・R成分0.18(明るく灰色がかる＝遠景は反射が強い)。輝度>0.55の画素は手前〜中景で約0.8%、遠景で3〜5%。縞の向きは手前0.75(等方的)→遠景0.26(横縞は遠近法による)。
- **分かったこと**: ①白い網目の正体は既存のコースティクス加算(係数を下げる) ②原作は近景ほぼ反射せず遠景ほど強く反射=Schlick(F0≈0.02) ③波の向きを絞ると手前がリボン状になる(向きは広く散らす) ④点は格子ハッシュだと整列して見える→ValueNoiseの閾値処理による不規則な破片＋低周波の塊マスク、ハッシュは整数演算(座標が大きくても精度が落ちない) ⑤法線は頂点補間だとグリッドの境目が見える→PSで計算 ⑥海メッシュの端が見えると水平線が弧になる→端を霧で空色になじませる ⑦.hlsliはMSBuildの依存に入らない(編集したらhlslも保存し直す)。
- **実験6**(ユーザーが実機確認): 平均色・遠景の明るさ・点の割合・遠景の縞の向きが原作と数値でほぼ一致。残った差: 遠景が鏡のようになめらか(雲の形が映る水たまり)、遠景のちらつき、大きな白い破片が紙片に見える、遠景の色が青すぎる。**実験7**(波ごとの位相変化率でLOD、スケール不変の遠景ざわつきノイズ、大きな破片を柔らかく、空の反射にティント)はコンパイルのみ確認で実機未確認。
- **スナップショット**: 実験7のコードはこのPCの`C:\Users\Admin\.claude\projects\c--Users-Admin-Documents-GitHub-NovaWing\memory\water_experiment_2026-10-04\`にあるが、リポジトリには入れていない(別PCには無い)。
- **教訓**: バックアップからコピーで戻すと更新日時が古いままでMSBuildが再コンパイルせず、古い実験コードのexeが動く(今回は法線マップ読み込みのassertで発覚)。戻したら更新日時を新しくし、ビルドして確認する。
- **次回**: 雨の2面ステージ作業が本題。海は上の数値と知見を元に、ユーザーが設計→Claudeがレビューの形で再実装(優先度は低め)。

### 次にやること（2026-10-04・海のリアル化を自分で再現する・学校で実施）
実験(上記)は複雑すぎて再現困難だったため、効果が大きく簡単なものから4段階に分けて**ユーザーが自分で書く**。段階ごとにF5で確認し、スクリーンショットをClaudeに見せてレビューを受ける。LOD・スケール不変ノイズ・共通.hlsli化は**やらない**。

1. **コースティクスを弱める**(WaterPS.hlsl): `finalCol += causticsFinal.rgb` に係数を掛ける。ユーザー案は`* 0.3f`。`float3(...)`のキャストは不要。係数は冒頭の定数群に`caustics_strength`として名前を付ける(他の係数と同じ流儀)。実験では`0.1`で白い網目がほぼ消えた。`0.3`で網目が残るなら下げる。
2. **水の色を原作に寄せる**: 手前〜中景の平均色が約(0.03,0.27,0.38)・輝度0.22になるように`shallow_color`/`deep_color`を調整(ライティングで約0.9倍になる分を見込んで少し明るめ)。
3. **フレネルをSchlick風に**: 「真上から見て約2%(F0=0.02)、浅い角度ほど急に強く」。`fresnel_power`は5程度。空の反射の強さも調整して、近景は暗く遠景だけ明るくなるようにする。
4. **白い点を1層だけ**(1〜3の後に見た目を見て判断): 既存の`ValueNoise`を使い、閾値処理で小さな点を出す。輝度の高い画素が手前〜中景で約1%になる量が目安。

- 確認方法: 原作と数値で比較できる(平均色・水平線側の輝度0.35・点の割合など)。詳細な数値と知見は上の「進捗（2026-10-04 続き）」を参照。
- 注意: 戻す・差し替えるときは、ビルドが再コンパイルされるよう更新日時に気を付ける(.hlslを保存し直す)。

### 設計中（2026-10-05・チャージショットの着弾爆発で範囲ワンパン）
- **仕様**: チャージ弾が当たってもダメージは入れない。着弾した瞬間に大きいコライダー(爆発)を出し、その範囲内の敵を倒す(雑魚はワンパン、ボスは低め)。
- **現状の問題**: チャージ弾は`ColliderTag::PlayerBullet`+`BulletCollider`なので通常弾と同じく接触でダメージが入る。当たると即`OnDead()`なので爆発の判定を持てない。作りかけの`ChargeExprosionCollider`は`IsCollisionActive()`が常にfalse、`GetOwner()`にreturnがない、弾の小さい球を返している、Wormがボス扱い(20ダメ)になる、OnCollisionの対象にWormがない。
- **提案した方針**: ①タグを`ChargeBullet`(着弾を検知するだけ)と`ChargeExplosion`(ダメージ)に分ける ②ChargeBulletを消さずに「飛行中→爆発中」の状態にして、爆発の球とコライダーをChargeBulletに持たせる(.hには前方宣言+unique_ptr) ③爆発の判定は着弾フレームの1フレームだけ有効(数フレーム有効だとボスに毎フレームダメージが入る。ダメージを入れるのは受ける側なので、爆発側では多段ヒットを止められない) ④hit_pairsでは爆発のペアをチャージ弾のペアより後ろに置き、同じフレームで爆発させる。CollisionManagerで爆発のコライダーもaddColliderする。
- **ボスの扱い(2026-10-05決定)**: シールドに当たっても爆発はする。ただしボスへの効果は直撃したコライダーだけ。BossDamageに直撃→ダメージのみ(回復なし)、BossShieldに直撃→回復のみ(ダメージなし)。→ボスはChargeBulletタグの直撃で処理し(BossDamage/BossShieldColliderのOnCollisionにChargeBullet分岐を追加、値はチャージ弾のGetAttackPower)、爆発のペアは`{ChargeExplosion, Enemy}`と`{ChargeExplosion, Worm}`だけにする。ChargeExprosionColliderは雑魚専用になり、ボス用の分岐は不要。
- **注意**: チャージ弾は当たっても死なないので、BossDamageとBossShieldに同時に触れるとダメージと回復が両方起きる。爆発状態に入ったらチャージ弾のコライダーを無効にする(例: BulletBaseに`virtual bool IsHitActive()`を追加してBulletCollider::IsCollisionActiveで使い、ChargeBulletでオーバーライド)。hit_pairsはBossDamageをBossShieldより先にする。
- **確認待ち**: 雑魚に当たった爆発の範囲にボスがいても、ボスには何も起きない(直撃のみ)で良いか。
- **(完了・2026-10-05)** ユーザーがゲーム内で確認し、チャージ弾の爆発(挙動・エフェクトとも)は「良い感じ」と承認。

### やりたいこと（2026-10-05・チャージ爆発が終わった後）: CollisionManagerを自分で作り直す
- CollisionManagerはClaudeが作ったもので、ユーザーは中身を理解できていない。**ユーザーが自分で作り直し、Claudeは説明役**(コードは書かない)。
- 進め方の案: 今の作りを部品ごとに説明する(コライダーをタグごとに集める→hit_pairsの組み合わせだけ判定→IsHitで形状同士を比較→OnHitで両方のOnCollisionを呼ぶ→プレイヤー被弾の多段ヒット防止(DamageSource)→カウンター時の敵弾反射)。理解した部品からユーザーが書き直す。

### 進捗（2026-10-05・チャージ爆発エフェクト`ChargeExplosion`の制作準備とEffekseer MCPの導入）
- **Effekseer MCPを導入した**（それまでこのPCには無かった）。ユーザーがダウンロードした`EFFEKSEER_MCP_SETUP_GUIDE.md`の手順で構築。
  - Effekseer **1.80.7**を公式GitHubから`Downloads\Effekseer1.80.7Win\`に展開(1.80.6は残してある)。MCP用のコピーは`%LOCALAPPDATA%\EffekseerMcp\effekseer`、サーバーは`%LOCALAPPDATA%\EffekseerMcp\server\EffekseerMcp.Server.exe`、ソースは`C:\dev\EffekseerMcp`。
  - テスト: ホスト層24/24 PASS、MCP受け入れ102/103(失敗1件は「capture uses the new background」。手順書のトラブル表にある既知の症状で、制作には影響なし)。
  - `~/.claude.json`の先頭にユーザー単位の`mcpServers.effekseer`を追加(バックアップ`.claude.json.bak-effekseer-mcp`)。**新しいセッションから有効**。
  - `setup.ps1`はDownloadsのEffekseerを自動検出するが、1.80.6と1.80.7が両方あると古い方を拾い得るので`-EffekseerTool`で1.80.7を明示した。
- **ChargeExplosionのヒアリング結果（ユーザー回答済み）**: 一気に完成まで作る(関門ごとの確認なし)。完成の基準は「ゲーム内で判定の球のデバッグ表示と外周が重なり、範囲が一目で分かる」。球は「縁が光る泡」を基本に、多少中が見えるように(最初の数フレームだけ中も光る)。オレンジの炎・煙は入れない(緑だけ)。カメラは全方位、負荷は少なめ、雰囲気はv10のチャージショットに合わせる。
- **方針案**: 球はどの方向から見ても円なので、3Dメッシュは使わずビルボードで作る(全方位でも軽い)。層: Flash(0〜6F、白緑のCore+StarB。v10の玉が弾けた印) / Shell(縁が光る泡テクスチャを新規作成。0→3Fで半径0→8へEaseOut、8〜15F保持、40Fまでに消える。**縁がちょうど8**) / Fill(薄い緑のGlowで内側) / Arc(CS_Arcの電気が球の中で短く走る) / Wisp・Spark(外周へ散る余韻)。最大40〜60インスタンス目標(v10の弾は約154)。
- **v10と参考動画の分析**: v10の色は緑(30,255,110)・白緑(235,255,238)、加算。暗い緑の`Backing`(アルファブレンド)で背景から浮かせている。参考動画(スターフォックス)の着弾は、緑の電気の球が約0.1秒で一気に広がり、中心が白飛び、表面に電気の筋、約0.3秒保持→緑の筋が散って消える。
- **大きさ**: 出力倍率50なので半径400＝エディタ8単位。`charge_explosion_effect_scale = 1.0f`が基準、判定半径が変わったら「新しい半径÷400」。

### 進捗（2026-10-05・ChargeExplosion制作）
- **成果物**: `Data/Effect/ChargeExplosion/ChargeExplosion.efkefc`(編集用) / `ChargeExplosion.efk`(出力倍率50) / `ChargeExplosion_v1.efkproj`(手書きの初版) / `Texture/`(CS_系はPlayerChargeBulletからコピー、`CE_Shell.png`は新規の「縁が光る泡」512px。縁のピークは半幅の0.90)。
- **層**(描画順): Backing(暗緑ブレンド) / Fill(内側の薄い緑) / FillFlash(0〜8Fだけ中も光る) / Shell(泡。0→3Fで判定半径まで広がり15Fまで保持、40Fで消える) / ShellFlash(縁を0〜10Fだけ強調) / Core・Star(中心の白飛び) / Arc(内部の電気、12個) / Spark(外へ散る粒、24個) / Wisp(12F以降の緑の筋、5個)。合計で約50インスタンス。
- **大きさ**: Shellのスケール17.78で縁が半径8(エディタ単位)＝ゲームで400。エディタで一辺16の四角と重ねて、縁が接することを確認した。
- **判定半径は700だった**(`ChargeBullet.cpp`の`explosion_radius`)ので、`charge_explosion_effect_scale = 1.75f`(700÷400)にした。半径を変えたらこの値も変える。
- **登録済み**: `ResourceConstants.h`(パスとscale)、`ResourceLoader.h`(`EffectID::ChargeExplosion`)、`ResourceLoader.cpp`(対応表・ゲームシーンの一覧・個別の読み込み)。
- **再生も追加済み**(ユーザーの許可を得て): `ChargeBullet.cpp`の`OnHitEnemy()`で`PlayOneShot(ResourceLoader::EffectID::ChargeExplosion, m_pos)`。Debugビルド成功。
- **未**: ゲーム内でデバッグ表示の球と外周が重なるかの確認(ユーザーがプレイして確認)。
- **v2(立体感の追加・2026-10-05)**: ユーザー「3D感がない」→ 本物の3D球を追加。`Model/CE_Sphere.efkmodel`(半径1のUV球、64×32)+`Texture/CE_Net.png`(電気の筋を正距円筒で描いたもの。左右がつながり、極付近は消える)。ノード`Net`(手前の半球、Culling=Front、明るい)と`NetBack`(奥の半球、Culling=Back、alpha70)。スケールは0→3Fで7.9(縁の泡の内側)、ゆっくり回転(Y 1.6°/F、X 0.4°/F)。平たく見えた`Wisp`は削除。v1は`ChargeExplosion_v1.efkefc/.efk`に残してある。
  - 知見: エディタではCulling=**Back**で奥側の面が描かれた(球の中心に不透明な板を置き、深度で隠して確認)。**ゲームは左手系なので表裏が逆になる可能性がある**。ゲームで奥の筋が明るく見えたら、NetとNetBackのCullingを入れ替える。
  - **ユーザーがゲーム内で確認し「いい感じ」と承認(2026-10-05)。ChargeExplosionは完成。**
  - 生成スクリプトはセッションのscratchpadにあった(`gen_shell.ps1`/`gen_net.ps1`/`gen_proj.ps1`)。消えるので、作り直すときは要再作成。
- **注意**: エフェクトは約45Fで消えるが、爆発状態は90F続く(`explosion_max_frame`)。デバッグ表示の球はそれより長く残る。
- **知見(Effekseer)**: 手書き.efkprojのSingleFCurveは、サンプリング既定値(10)だと短いキー(0→3F)がなまり、約10Fかけて広がった。`fcurve_set`で`sampling: 1`にすると、キーどおり3Fで広がる。PowerShellでは`R`がInvoke-Historyの別名なので、関数名に使わない。

### 進捗（2026-10-05・ブーストのエフェクトをリアル調で作り直し、v1）

- 依頼: ブースト時のエフェクトをかっこよく。リアル調、水色、負荷は少なめ、一気に作る。両翼で風を切っている感じと、ブーストの炎らしさの両方を出す。参考作品はなし。
- ファイル: `Data/Effect/Boost/Boost_v1/Boost.efkefc` / `.efk`（拡大率50で書き出し）/ `Texture/BS_*.png`（自作）。旧版の`Data/Effect/Boost/Boost.*`は残してある。テクスチャ生成は`Data/Effect/Boost/tools/gen_boost_textures.ps1`。確認画像は`tools/review/`（`compare_old_v1.png`が旧版との比較）。
- 座標の前提（今のコードに合わせた）: エフェクトの原点＝機体の200後ろ（`BoostState`の`boost_effect_offset_pos`）、回転は`(rotX, rotY+π, 0)`なので**エディタの+Zが機体の後ろ**。機体の中心は`Ship`ノードのz=-4。噴射口は`Ship/Nozzle`のz=+1.2（機体中心から60後ろ、推定）、翼端は`Ship/WingTipR/L`のx=±1.6・z=+0.45（バレルロールと同じ推定値）。**ずれていたらこの3ノードの位置だけ直せばよい。**
- 構成: 炎＝`FlameOuter`/`FlameCore`（Ringの円錐、流れる筋のテクスチャをUVスクロール、毎F作り直して長さを揺らす）＋`NozzleGlow`＋`ShockDiamond`（炎の中の明るい節、3個）＋`ExhaustTrail`（空間に残る排気、2個/F）。開始の一瞬だけ`IgnitionRing`/`IgnitionFlash`。風を切る＝翼端の`Vortex`（Track、空間に残る細い渦の線、軽くねじれる）＋`TipCone`（翼端の鋭い白い円錐）＋`WingSliceR/L`（翼の後縁からはがれる気流の筋）＋`AirStreak`（機体のまわりを後ろへ流れる気流の線）。同時に出ている数はおよそ100個。
- プレビューのしかた: エディタの「振る舞い」で位置の速度Zを-0.34（ブースト速度17÷50）にすると、空間に残る層の見え方がゲームに近くなる（ファイルには保存されない）。仮の機体モデル`tools/DummyJet.efkmodel`は書き出し前に外してある。
- **コード側は未対応（ユーザーが実装する）**: `Constants/ResourceConstants.h`の`boost_effect_path`を`L"Data/Effect/Boost/Boost_v1/Boost.efk"`にする。タイトル画面も同じエフェクトを使っている。
- 未確認: ゲーム内での見え方（大きさ、噴射口と翼端の位置、明るさ）。タイトルはブースト速度が70/Fなので、空間に残る層（排気・渦）がゲーム中より長く伸びるはず。

### 方針決定（2026-10-05・2面「雨の荒廃ビル街」）
- **2面は海を使わない。** 理由: 1面と同じ景色が嫌。1面=今の海ステージのまま、2面=荒廃したビル街。WaterManagerは2面では使わない。
- **GameSceneは1つのまま使い回す(案B)。** スカイボックス・CSV4種・BGM・天候・ボスの有無などを`StageConfig`にまとめて差し替える。コピーして`Game2Scene`は作らない。リトライは同じ設定、クリア後は次の設定で作り直す。
- **ステージ選択画面は一旦なし**(余裕があれば)。1面クリア→2面へ直行を先に作る。
- **見た目**: 地面は道路＋がれき。ビルはフリー素材。曇り空のスカイボックス＋霧(フォグ)＋Effekseerのカメラ追従の雨粒(ユーザーの思い描いていたイメージと一致)。
- **敵**: コスト表の新敵3種(蝶=かくかく移動・回転弾・逃げる・死亡 7h / 固定砲台=弾・死亡 2h / 突進=プレイヤーへ移動・死亡 2h、計11h、いずれもプロト・S)を入れる方向。2面専用か1面にも出すかは未確認。ボスは仕様未定。
- **後回し**: カメラレンズの水滴(シェーダ8h)。海のリアル化は放置(2面に海が無いので優先度はさらに下がる)。
- **未決定**: ビルの当たり判定(Rock流用が候補)、地面の作り方、新敵の初登場ステージ、2面のボス。

### 進捗（2026-10-05続き・ブーストのエフェクト: 全体を後ろへ）

- ユーザーがゲームで確認し「モデルより少し前に寄っている、もう少し後ろでいい」。
- `Ship`ノードのzを-4→-3.2にした（全体を0.8＝ゲームで40後ろへ。噴射口・翼端・気流すべて一緒に動く）。ほかは変更なし。
- ファイル: `Data/Effect/Boost/Boost_v1/`を上書きした。**ユーザーの希望で、エフェクトの版ごとのフォルダ（_v2など）は今後作らない。戻すときはgitで。** 位置の比較画像は`tools/review/compare_v1_v2.png`。
- コード側の変更は不要（`boost_effect_path`は`Boost_v1/Boost.efk`のまま）。
- 未確認: ゲーム内での位置。翼端の線だけ前後がずれて見える場合は、`Ship/WingTipR/L`のzを個別に直す。

### 進捗（2026-10-05続き・エフェクトの前の版と未使用素材を整理）

- ユーザーの方針: **前の版は残さない。Claudeが作ったエフェクトを使う。戻すときはgitで。** 以後、修正は同じファイルを上書きする。
- 消したもの（gitの履歴から戻せる）: 旧ブースト(`Boost.*`、`aurora01/orb_colored`)、`BarrelRoll_v1_*`、`BossBeam.*`(旧版、`BossBeam_2`は使用中なので残した)、`ChargeExplosion_v1.*`、`Charging_v0/`と`Charging_v1〜v10.efkproj`、`EnemyBullet_v2.*`/`_v3a_NoRing.*`、`EnemyDeath_v1〜v4`の途中版、`PlayerBullet_v0/`と`work/`、`PlayerChargeBullet_v0/`と`_v1〜v10(_preview).efkproj`、`WingSprayReal_v1/v2a〜d_*`と`WingSprayToon_*`、`Exprosion`/`Exprosion2`/`WingSplash`(どこからも読まれていない)。さらに、残ったエフェクトのどれからも参照されていないテクスチャ・モデル・マテリアル43個(`Splash/Parts`の未使用分など)。
- `EnemyDeath_v4.efkefc`は`EnemyDeath.efkefc`に名前を変えた(ゲームが読む`.efk`と名前をそろえた)。
- ブーストは`Data/Effect/Boost/`直下に移した(`Boost.efk`/`.efkefc`/`Texture/BS_*.png`)。`Boost_v1/`はなくなった。**コード側(ユーザーが実装する): `boost_effect_path`を元の`L"Data/Effect/Boost/Boost.efk"`に戻す。**
- 残すと決めたもの: `BossBeam_Spiral.efkproj`(Claude作、未完成でゲーム未使用)。各エフェクトの`tools/`(生成スクリプト・確認画像)。
- 既存の問題(今回の整理とは無関係): `BossBeam_2.efkefc`はテクスチャをデスクトップの`Effekseer_Sample`から参照している。`.efk`はゲームで動いている。
- 確認: 残ったすべての`.efk`/`.efkefc`が参照する素材が存在することを確かめた(上の`BossBeam_2.efkefc`の4件を除く)。

### 進捗（2026-10-05続き・ブーストのエフェクト: さらに後ろへ＋赤みを追加）

- ユーザーがゲームで確認し「機体にのめりこんでいる」「赤を混ぜてよい（不完全燃焼っぽさ）。下の海と色がかぶる」。
- `Ship`のzを-3.2→-2.4（さらにゲームで40後ろ。最初の版からは合計80後ろ）。
- 赤み: `FlameBurn`（Ringの円錐、噴射口の少し後ろから長さ4.6、橙→赤）を追加し、`ExhaustTrail`を橙→暗い赤に変えた。**この2つは合成を加算(Add)ではなく通常(Blend)にした。** 加算だと明るい青い海に重なったとき赤が白〜ピンクに飛んで見えなくなるため。噴射口近くの青白い芯(`FlameCore`/`FlameOuter`/`NozzleGlow`)は加算のまま。
- 気づいた点: ゲームのスクリーンショットでは、炎の付け根が機体の上に重なって描かれていた。エフェクトが機体の深度で隠れていない可能性がある（`EffectManager`の`DrawEffekseer3D`の呼び順・深度の扱いは未確認）。後ろへずらして対処したが、重なりが残るならコード側の描画順を調べる。

### 進捗（2026-10-05続き・ブーストのエフェクト: さらに後ろへ＋赤みを追加）

- ユーザーがゲームで確認し「機体にのめりこんでいる」「少し赤を混ぜてよい（不完全燃焼っぽく。青い海と色がかぶるので）」。
- `Ship`のzを-3.2→-2.4にした（最初の-4から合計80後ろ）。
- 赤み: `FlameBurn`（Ringの円錐、噴射口から0.6後ろ〜3.9、橙→赤）を炎の外側に追加。`ExhaustTrail`（排気）の色を水色→橙(255,120,50)から暗い赤(120,35,30)へ変わるようにした。噴射口付近は青白いまま、後ろへ行くほど橙〜赤になる。
- 描画の仕組みのメモ: エフェクトは`EffectManager::Draw()`の`DrawEffekseer3D()`で描いている。機体に重なって見えるのは位置が前すぎるため（深度で隠れていない可能性もある）。まだ重なるなら`Ship`のzをさらに大きくする。
- `Data/Effect/Boost/Boost.efk`を上書きして書き出した。未確認: ゲーム内での見え方。

### 設計（2026-10-05・Unityでステージ配置→CSV書き出し、定数もCSV化）
- **方針(ユーザー決定)**: 配置はすべてUnityで行いCSVに書き出す。定数・パラメータも「ほぼ全部」CSV化する(就職後の実務を意識。調整頻度に関係なく)。参考は`ProjectNeaR`(先輩の作品)。
- **ProjectNeaRの仕組み**: `Data/CSV/<ステージ>/`に`StageData.csv`(配置)・`CharacterData.csv`・`CharaStatusData.csv`(HP等、IDで配置と結合)。`CSVData`継承の`ActorData`/`CharaStatusData`がコンストラクタで`Conversion()`。Unity座標→DxLibは`kUnityToDXPosition = 100`を読み込み時に1か所で掛ける。回転はUnityの順序に合わせたクォータニオン。
- **NovaWingの設計**: `Data/CSV/Stage1/`・`Stage2/`に配置(Unity書き出し: Rock/FloatingEnemy/WormEnemy/Boss/…)、`Data/CSV/Params/`に手調整の数値(`EnemyStatus.csv`=modelIDキーでhp等、`RockShape.csv`=岩の球をモデルごとに、`<クラス>.csv`=`Name,Value`の定数)。共通列は`modelID,posXYZ,rotXYZ,scaleXYZ`、種類固有の列は後ろ。列はヘッダ名で引く。定数は「キー無しはassert」の型付き窓口。
- **Unity側**: リポジトリ直下`StageEditor/`(Libraryは.gitignore)。プレハブに`PlacedObject`(modelID＋ワーム用追加項目)。エディタメニュー「NovaWing/Export Stage CSV」で`Data/CSV/<ステージ名>/`に書き出す。Unity1単位=ゲーム100単位(Z=8000→Unity80)。FBXは`Data/Model`からコピー(二重管理は許容)。
- **タスク順**: ①`StageEditor/`作成 ②PlacedObject+書き出し(まず浮遊敵) ③既存配置を再現して座標・回転(Unity/DxLibで順序が違う)を実機検証 ④岩・ワーム・ボスを追加、hpと岩の球を`Params/`へ ⑤DataSetterをヘッダ名引き＋`CSVData`継承の型へ(旧保留タスク3と同時) ⑥2面のビルを載せる。
- **2026-10-05続き: 「赤の主張が強すぎる」→ 控えめにした。** `FlameBurn`の中心色を(255,130,60,α85)に(元はα120でより赤い)。`ExhaustTrail`は青白(170,190,230,α60)から始まり、消えぎわだけ橙(220,95,60,α35)になるようにした。さらに弱めるなら`FlameBurn`のα、強めるなら排気の終わりの色を調整する。
- **ユーザーがゲーム内で確認し「良い感じ」と承認(2026-10-05)。ブーストのエフェクトは完成。**
- **(変更・2026-10-05)** 座標変換(Unity1単位=ゲーム100単位)は**C++側ではなくUnityの書き出しで1か所だけ**行う(`StageCsvExporter.cs`の`UnityToGame`)。CSVはゲーム単位(ワームの`activatePlayerZ`もゲーム単位なので揃う)。ゲーム側の`unity_to_game_scale`は不要。回転(度)と大きさは変換なし。理由: ユーザー指摘「毎回scale定数を作るのが面倒」＋単位の混在を避ける。
- **(進捗)** Unity側: `StageEditor/`をNovaWing直下に移動済み(`.gitignore`追記済み)、`PlacedObject.cs`と`Editor/StageCsvExporter.cs`作成、浮遊敵1体の書き出しはユーザー確認済み。ゲーム側の読み込み(`FloatingEnemyDataSetter`を`Stage1/FloatingEnemy`の新形式に)は未実施。CSVのpos列を1セルvectorにするかは後回し(今は`posX,posY,posZ`の分割列)。
- **(完了・2026-10-05)** 浮遊敵で「Unityで置く→書き出し→ゲームで読む」の一通りがユーザー確認済み。`FloatingEnemyDataSetter`の読み込み先は`L"Stage1/FloatingEnemy"`(ファイル名に`Data`は付かない。最初`Stage1/FloatingEnemyData`と書いて敵が0体になった。`LoadCSV`は存在しないファイルでもエラーを出さず空配列を返す)。hpは仮に`5`の直書き(あとで`Params/EnemyStatus.csv`へ)。Unity書き出し時、Excelで開いたままのCSVがあると`Sharing violation`になる。
- **次**: 岩・ワーム・ボスの読み込みを新形式に(ヘッダ名引き＋`CSVData`継承の型へ、回転・大きさの扱い)、旧CSVの敵を全部Unityに置き直す、`hp`と岩の球を`Params/`へ。

### 進捗（2026-10-07・1面のレベルデザインを作り直し、Unityに同期）
- **依頼**: チャージ弾が範囲爆発になったので、それを活かす配置をClaudeが作る。ワームは正面からだけ。岩・敵・ボスの位置は全部変えてよい。
- **ワーム**: 全8体を`direction=-1`(プレイヤーへ向かう)に統一。出現位置は動き出しのZ＋約3500。後ろから来る4体は削除。
- **浮遊敵(42体)**: 爆発半径700に収まる塊で構成。Z=4000導入2体 / 7500リング6体 / 11000左右4体ずつ / 14500の3×3壁 / 17000〜18000縦列5体 / 22000密集9体 / 26000離れた3体。
- **岩(31個)**: 低い岩(Rock1)は飾り、高い岩(Rock2/3)は「門」「中央の仕切り」「スラローム」「狭まる回廊(±1100→±900→±700)」として敵の塊の前に配置。ボスはそのまま(Z=30500、登場Z=27000)。
- **Unity同期**: `StageEditor/Assets/Editor/StageCsvImporter.cs`を追加(メニュー「NovaWing/Import Stage CSV」)。Claudeがバッチモードで実行しStage1.unityを更新済み。コマンド: `Unity.exe -batchmode -quit -nographics -projectPath StageEditor -executeMethod StageCsvImporter.ImportAndSaveBatch`(Unity 2022.3.62f3)。書き出し直して元のCSVと一致を確認(負の回転角が345表記になるだけ)。
- **未確認**: ゲーム内での塊の大きさ・門の隙間・ワームの出現距離。岩の幅は推定。プレイして調整する。

### 進捗・決定（2026-10-07 続き・DataSetter整理と「パラメータのCSV化」の設計）
**今日終わったこと(ユーザーが実装、コミットはまだ)**
- 浮遊敵・ボスのDataSetterから、新形式では意味が変わった`dataString[4]`(hpの読み込み。新形式の4列目はrotX)を削除。hpは当面直書き(浮遊敵`5`、ボス`2000`)のまま。
- ワームの列番号を定数化(`segment_count_number=10`/`direction_number=11`/`active_player_number=12`)。※`active_player_number`は`activate_player_z_number`の方が分かりやすいとレビュー済み(直すかは任意)。
- 岩のY軸回転を反映(A案)。`Rock::RockData::rotYRadian`を追加し、`RockDataSetter`でCSVの度→ラジアン変換(`model_rot_y_number=5`)、`Rock`のコンストラクタで`m_rotation`に代入。**回転は度でCSVに入り、変換はDataSetterで行う。** 当たり判定の球はY軸回転では動かないので修正不要。X/Z回転・scaleは未対応(球の変換が必要になる)。この修正(名前の統一)だけは例外的にClaudeが直接編集した。**ゲーム内で`rotY=345`の岩(`Rock1,1200,0,3200`)の向きが変わるか、向きが逆でないかは未確認。** 空白がスペース→タブに変わった行が残っている(無害)。
- ビルド・実行は未確認(Claude側ではしていない)。

**パラメータのCSV化: 方針の見直し(重要)**
- 以前は「定数をほぼ全部(433個、約60ファイル)CSV化」としていたが、**ProjectNeaR方式に寄せることにした(ユーザー決定)**。ProjectNeaRを調べた結果:
  - CSVにあるのは「キャラ/敵ごとのステータス」(`CharaStatusData.csv`: 1行=1キャラ、列=体力・攻撃力・移動速度など)だけ。`CSVData`の派生クラス(`CharaStatusData`)のコンストラクタで`Conversion()`して型にする。
  - `Player.cpp`などの`kXxx`定数はCSVにせず、cppの匿名名前空間に残している。
  - 弱点: 列を番号で引き、要素数が違うと`Conversion()`が黙ってreturnする。→NovaWingでは**ヘッダ名で引き、無い名前はassert**にする。
- したがって**CSVに出すのは「種類ごとに変えたい数値」(HP・球の半径・死亡フレーム・移動速度・弾の値など)だけ**。モデル/アニメ名・骨のインデックス・アニメ再生速度・エフェクトのオフセットなど「モデルと対になった値」はコードに残す。**「描画の細かい数値も含めたい」という以前の希望は、この方針で取り下げ(要望があれば再検討)。**
- 検討したが採用しなかった案: `Name,Value`のCSVを`ParamTable`で読み、クラスごとの`XxxParams`構造体を関数内staticで共有する案(Claudeの提案)。ProjectNeaR方式を優先したので保留。

**`EnemyStatus.csv`の設計(決定)**
- `Data/CSV/Params/EnemyStatus.csv`に**全敵を1つの表に並べる**(ユーザー決定)。使わない列は0にする。1列目は`modelID`、列はヘッダ名付き(英語名)、BOMなしUTF-8。
- 最初の列案: `modelID,hp,colRadius,trueDeadFrame,deathEffectInterval`
  - FloatingEnemy: hp5(直書き)/colRadius132/trueDeadFrame10/deathEffectInterval0
  - WormHead: hp=現在値(`WormEnemy.cpp`で要確認)/colRadius40/trueDeadFrame**0**/deathEffectInterval13
  - Boss: hp2000/trueDeadFrame330(=60*5+30)/colRadiusは球が3つ(無敵用1500・ダメージ用310など)あるので**最初は対象外**
- ワームは`true_dead_frame`を使わない(胴体が全部消えたら`OnEnemyDead()`、`WormEnemy.cpp:227`付近)ので、そこは0で良い。**0は「使わない」の意味なので、`EnemyStatusData`のコメントかCSVに「使わない敵は0」と明記する。**
- 後で足す候補: ワームの`move_speed`18・`bullet_speed`8・`bullet_power`5・`shoot_interval`60、ボスの`recovery_rate`0.4など。
- 読み込み: `EnemyStatusData : CSVData`(ヘッダ名で引く、無いのはassert)を作り、`modelID`で行を引く取り出し口を用意。同じ表を敵ごとに読み直さないよう**1回だけ読んでキャッシュ**する(浮遊敵が42体いるため)。


**その他の注意(次回の確認用)**
- 1面の配置は今日作り直してコミット済み(上の「進捗（2026-10-07・1面のレベルデザイン…）」)。Unityの`Stage1.unity`も同期済み。**未確認: ゲーム内での塊の大きさ・門の隙間・ワームの出現距離**(岩のscaleはコードで`(3,5,3)`固定、岩の幅は推定)。
- Unityで「Export Stage CSV」を押すと、Unityシーンの内容でCSVが上書きされる。CSVを直接直したら`StageCsvImporter`(バッチ実行コマンドは上の項目)でUnityに取り込むこと。
- ウィンドウの縮小の相談があった(VS Codeの`Ctrl`+`-`が効かない件)。コマンドパレットの「View: Zoom Out」か`window.zoomLevel`で対応する案を伝えた。

### 方針決定（2026-10-08・ステージ1をチュートリアルに）
- **進める順(ユーザー決定)**: ①パラメータの外部化(`EnemyStatus.csv`、上の「次にやること」1〜5)をとりあえず終わらせる → ②**ステージ1をチュートリアルにする**(大まかでよい) → ③ステージ2(雨の荒廃ビル街)へ。
- チュートリアルの中身(何を教えるか・進め方)は未決定。
- 外部化のメモ: ワームのHPは直書きではなく、`EnemyBase`のコンストラクタ引数`maxHealth`の既定値`100`がそのまま使われている(`WormEnemy.cpp:45`で渡していない)。

### 進捗（2026-10-08・外部化: ヘッダをCSVDataまで届ける、完了）
- **`Data/CSV/Params/EnemyStatus.csv`作成済み**(ユーザー)。`modelID,hp,colRadius,trueDeadFrame,deathEffectInterval` / FloatingEnemy 5,132,10,0 / WormHead 100,40,0,13 / Boss 2000,0,330,0。
- **BOM**: 最初Excelの「CSV UTF-8」で保存しBOM付きになった→VS Codeで「エンコード付きで保存→UTF-8」で除去。原因の一つはワークスペース設定`files.encoding: utf8bom`。`.vscode/settings.json`に`"[csv]": {"files.encoding": "utf8"}`と`rainbow_csv.virtual_alignment_mode: always`(見た目だけ列をそろえる。ステータスバーの「Align」は実際にスペースを書き込むので押さない)を追加。**読み込み側のBOM除去はユーザー判断で見送り**(ExcelでCSV UTF-8保存すると1列目のヘッダ名が`modelID`になりassertで止まる、と覚えておく)。旧`Data/CSV/RockData.csv`だけBOM付き(旧形式、整理予定)。
- **ユーザーが実装・動作確認済み**: `GetWStringList`がヘッダ行も先頭に入れて返す / `CSVData`に`m_header`と`SetHeader` / `LoadCSV`が空チェック→`[0]`をヘッダ→`i=1`から`SetData`+`SetHeader`。既存の岩・敵・ボスは今までどおり出る。
- **(済・ユーザー実装)** `CSVData::GetColumnIndex(name) const`(protected、無ければassert→-1)、`GetHeader()`、デストラクタをvirtualに。`CSVData/EnemyStatusData`(`CSVData`継承、`shared_ptr<CSVData>`を受けるコンストラクタでデータ+ヘッダを写して`Conversion()`、modelIDは`ResourceLoader::ModelID`で保持＝.hで`ResourceLoader.h`をincludeするのはユーザー判断で許容)。
- **キャッシュの置き場所(ユーザー決定)**: 新クラス(`GameScene`が持つ`EnemyStatusTable`)は「敵のステータスごときにクラスを作りたくない」で不採用。`EnemyStatusData`の`static const EnemyStatusData& FindByModelID(ModelID)`の中の`static const auto table = CreateStatusTable()`(private static)で初回だけ読む。**CSVを変えたらゲーム再起動が必要**。
- **(済・動作確認済み)** 浮遊敵のHP: `FloatingEnemyDataSetter`と`EnemyFactory`(ボスの召喚)の`5`を`FindByModelID(...).GetHp()`に置き換え。
- **(済・ビルドOK)設計変更**: `FindByModelID`は`EnemyBase`のコンストラクタで呼ぶ(ユーザー提案)。HPは`Character`に`FindByModelID(modelID).GetHp()`を渡し、`EnemyBase`の`maxHealth`引数は削除。`EnemyBase`に`protected`の`m_colRadius`/`m_trueDeadFrame`(コンストラクタ本体で代入。初期化子リストは長くなるのでユーザーは使わない方針)。→**`EnemyBase`継承の敵は全員`EnemyStatus.csv`に行が必要(無いとassert)**。浮遊敵の`health`引数・定数2つ、`FloatingEnemyDataSetter`/`EnemyFactory`の`hp`、`BossEnemy`の`EnemyBase`への`data.health`を削除済み。`BossData::health`と`BossEnemyDataSetter`の`2000`は未使用のまま残っている(ボスの番で片付け)。
- **(済)ワーム**: `sphere_radius`→`m_colRadius`、`death_effect_interval`はワームだけなので`WormEnemy`の`m_deathEffectInterval`(コンストラクタで`FindByModelID(data.modelID)`)。動作確認済み。**注意: `m_frame % m_deathEffectInterval`なのでCSVで0にすると死亡時に0除算でクラッシュ。**
- **(済)ボス**: `true_dead_frame`→`m_trueDeadFrame`、`BossEnemyData::health`と`BossEnemyDataSetter`の`2000`を削除。ボスの判定半径(1500/310)は対象外のまま。
- **敵の外部化は完了。残り: 手順5の`RockShape.csv`(岩の球)**。→**ユーザー判断で岩は当面放置**。

### 設計中（2026-10-08・チュートリアル）
- **形式(ユーザー決定)**: ステージ型。今のステージ1をそのまま使い、進みながらヒントを出す(課題型ではない)。実装案は「プレイヤーのZがこの値を超えたらヒント」を`Stage1/Hint.csv`等で持つ(未決定)。
- **教える順(ユーザー案)**: 移動→傾き→バレルロール→ブースト→ブレーキ→ショット→チャージショット。ロックオンは順に入っていない(未確認)。
- **未実装と判明**: 「傾けた方向への横移動が速くなる」。`MovingState.cpp`の`vel.x = stick.x * move_speed_x`(19)に傾きが反映されていない。傾き=ローリングボタン1回押しの長押しでZ回転±90°(`DefaultRotationState.cpp`の180〜195行目、海面付近は0)。
- 別件: 浮遊敵(オレンジのテクスチャ)と敵弾(黄オレンジ、R255/G200/B40前後)の色が似て見づらい→別セッションで対応中。

### 進捗（2026-10-08・リザルト画面のDebug/ReleaseでUIサイズが違う問題を修正）
- 2026-09-23の`GetUIScale()`対応から漏れていた箇所を修正。`ClearScene.cpp`のテンプレート画像(`templete_size`)に`uiScale`を掛けた。リザルトの数字はフォント(`FontID::Result`, 105px固定)で描いていたため、`ResourceLoader::LoadFont`で`size`と`space`に`GetUIScale()`を掛けるようにした(全フォント共通、現状フォントはResultのみ)。
- Debug/Release両方でビルド成功。**実機での見た目確認は未実施。** ぼかし(`blur_range`=16)はGraphFilterの仕様上8/16/32しか選べないため据え置き(Debugでは相対的に少し強めのにじみになる)。

### 進捗（2026-10-08・敵の弾の色を黄〜橙からピンク寄りのマゼンタに変更）
- 理由: 黄色の浮遊敵や橙の着弾・爆発と色がかぶっていた。緑(プレイヤー弾)・青/水色(海・空・ブースト)・黄(浮遊敵)・橙(爆発)・赤(WARNING)・紫(ボスのビーム)を避けて、ピンク寄りのマゼンタにした。
- Effekseer MCPで`EnemyBullet.efkefc`の全ノードの色を変更し、`.efk`を書き出した(倍率50)。Glow(255,40,150,255)・Ring(255,70,180,220→230,20,140,0)・Tail(255,80,200,200→200,20,140,0)・SeaGlow(255,70,185,210)・SeaTrail(255,60,180,140→)・芯のCore(255,235,250)・Streak(255,225,245→255,120,215)。
- **Glow/Ring/Tail/SeaGlow/SeaTrailの合成を加算→通常(Blend)に変更。** 加算のままだと青い海の上で青が足されて紫に見え、ボスのビームと区別しにくかったため。芯(Core/Streak)は加算のまま。テクスチャは白+アルファなのでBlendでも黒い四角は出ない。
- エディタ上で黒背景・海の青(40,110,160)背景の両方で確認済み。**ゲーム内の見た目は未確認。**
### 進捗（2026-10-08・傾けた方向への横移動が速くなる、実装済み）
- 上の「設計中（2026-10-08・チュートリアル）」にあった**「傾けた方向への横移動が速くなる」は未実装ではなくなった**(ユーザーが`MovingState.cpp`に実装、コミット`b31f70c`)。
- 仕様: 機体のZ回転が`tilt_threshold`(π/4=45°)を超えて傾いていて、スティックがその傾き方向に倒されているときだけ、横速度が`tilt_move_speed_x`(25)になる。それ以外は`move_speed_x`(19)。逆方向に動かすときは通常のまま。右ローリング=Z回転+π/2、左=-π/2で、符号の対応は合っている(Claudeがコードを読んで確認)。
- `GaugeActionStateBase`(ゲージ消費中)の`vel.x`は対象外。傾いたままブースト等をしても速くならない。意図どおりかは未確認。
- 読みやすさの指摘(任意): 2つの分岐の`std::abs(stick.x) * ±tilt_move_speed_x`は`abs`不要(条件で符号が決まる)。「傾きとスティックの向きが同じなら速度=25」の1つの判断にまとめると短くなる。
- **未確認: 実機での体感**(25が速すぎないか、45°の切り替わりが唐突でないか)。チュートリアルの「傾き」の項で教える内容になる。

### 設計決定・進捗（2026-10-08・チュートリアルのメッセージ表示 `TutorialMessageScene`）
**設計(ユーザー決定)**
- 形式: ステージ型は維持しつつ、**「説明を出してゲームを止める→次へで閉じる→右上にタスクを表示→成功するまで先へ進ませない」**というタスク型の流れ。メッセージは**画面下部の黒い四角の上に文字**を出す(最初は文字だけ。画像・ボタン表示はあと)。
- 動機(ユーザー): **プレイヤーに成功体験を感じてほしい**。→Claudeの提案で、巻き戻しは「位置を戻す(A案)」ではなく**「成功するまでその場で練習できる(B案: 見えない壁・進行を止める)」**を採用する方向(ユーザーの明確な選択はまだ無く、理由の説明だけ)。成功したら右上のタスクに✓/色変更＋SE＋「OK!」などの演出を出す案。各タスクの条件は「1回できたら成功」と甘めにする。一定時間成功しなければヒント再表示(後回し)。
- 実装方針: ポーズと同じく**シーンのスタック**(`GameScene`から`PushScene(TutorialMessageScene)`、閉じるときに`PopScene`)。`SceneController::Update`は末尾のシーンだけ更新、`Draw`は全シーンなので、ゲームは止まり背景に残る。`PushScene`は`OnSceneChange`を呼ばない。
- タスクと配置の対応(たたき台、1面の新配置に合わせた): 移動・傾き=Z0〜3000(敵なし) / バレルロール〜3500 / ショット=3500〜(最初の浮遊敵Z=4000の手前) / チャージ=6500〜(門Z=7000、リング6体Z=7500の手前) / ブースト・ブレーキ=10000前後。ロックオンは順番に入っていない(実在するか未確認)。
- 将来の形: `Stage1/Tutorial.csv`(`triggerZ,message,taskText,taskType`など)。外部化で作ったヘッダ名引き(`CSVData::GetColumnIndex`)を使う。

**今日の作業(ユーザーが実装、未コミット)**
- `Scene/TutorialMessageScene.h/.cpp`を新規作成(`.vcxproj`/`.filters`にも登録済み)。`Scene`を継承し、`Init`/`Update`/`Draw`/`GetSceneID`を実装。画面比率で下部(x 0.2〜0.8、y 0.8〜0.95)に`DrawBox`し、`DrawString`で「テストメッセージ」を表示できた。
- **試作のため`Main/Application.cpp`の最初のシーンを`TitleScene`→`TutorialMessageScene`に変更している。コミット前に必ず`TitleScene`に戻すこと。**
- つまずきと原因(次回の参考):
  - `xmemory`のエラー: `Scene::Init()`が`abstract`なのに`Init`を実装していなかったため、`make_shared`で「抽象クラスをインスタンス化できない」(C2259)。
  - 黒い四角が出ない: Y座標を`wsize.width`で計算していた(`height`が正しい)。→修正済み。
  - リンカエラー: ヘッダに`Init()`を書いたが`.cpp`に定義が無かった。→修正済み。
  - フォントのアサート(`GetFont`の「フォントIDが見つかりません」): `Result`フォントはクリアシーンのリソース一覧(`clear.fonts`、`ResourceLoader.cpp`486行付近)にしか入っておらず、最初のシーンにするとIDが`None`で何も読まれないため。今は`DrawString`(既定フォント)で回避。本番では**メッセージ用の`FontID`を足し、`font_infos`(204行付近)と`game.fonts`(472行付近)に登録**すれば、そのまま`GetFont`が使える。`ReleaseFont`は`RemoveFontResourceEx`を呼ぶので、同じフォントファイルを複数IDで使う場合は解放の影響に注意。
- 現状のコードの注意: 四角の色が`0xffffff`(白)・文字が黒になっている(確認用。黒い四角＋白文字に戻す)。`m_textFontHandle`と`#include "Manager/ResourceLoader.h"`は今は未使用。`GetSceneID`は`SceneID()`(=`None`)を返している。本番は専用IDにするか、`PauseScene`のように専用IDを持たせる。


**その他**
- `abs`の簡略化(傾き加速の2分岐を1つの判断に)は任意。実機での体感(25が適切か)は未確認。
- 1面の配置(塊の大きさ・門の隙間・ワームの出現距離)と岩`rotY`の向きもゲーム内で未確認。

### 2026-10-09 `InputEvent`を文字列定数から`enum class`に変更
- 指摘(`InputEvent`はenumでよいのでは)を受けて変更。ユーザーの明示的な依頼でClaudeが直接編集した。
- `Manager/InputManager.h`: `namespace InputEvent`(`constexpr const char*`)→`enum class InputEvent`。デバッグ用5つは`#ifdef _DEBUG`のまま。`m_inputTable`/`m_inputData`/`m_lastInputData`のキーを`std::string`→`InputEvent`、`IsPressed/IsTriggered/IsReleased`の引数を`InputEvent`に変更。
- `Manager/InputManager.cpp`: 上記3関数の引数変更とコメント修正。登録側(`m_inputTable[InputEvent::ok] = ...`)は変更なし。
- `BossEnemy.cpp`: 文字列リテラルで直接呼んでいた`IsTriggered("killBoss")`を`InputEvent::killBoss`に修正(これのみ)。
- `GaugeActionStateBase`/`BoostState`/`BrakeState`: `const char* GetButtonName()`が文字列を返していたので`InputEvent GetInputEvent()`に変更(基底ヘッダに`InputManager.h`をinclude)。
- 効果: 綴りミス・コピペ重複(過去の`upScanlineFrequency`バグ)がコンパイルエラーになり、毎フレームの文字列比較が整数比較になる。
- **未確認: ビルド**(ユーザーがF5で確認する)。

### 2026-10-09 チュートリアルのメッセージフォントを M PLUS 1p Bold に変更
- Stick(試していた)が微妙だったため、日本語対応・NovaWingの雰囲気(リザルトはOrbitron)に合うフォントを探して差し替え。ユーザーの明示的な依頼でClaudeが直接編集した。
- 採用: **M PLUS 1p Bold**(Google Fonts / SIL OFL 1.1、第1・第2水準漢字入り)。幾何学的で角のある字形がOrbitronと馴染み、小さくても読みやすい。
- `Data/Fonts/Stick-Regular.ttf`と`OFL_Stick.txt`を削除し、`MPLUS1p-Bold.ttf`と`OFL_MPLUS1p.txt`を追加。`ResourceConstants.h`の`message_font_path`/`message_font_name`(`L"M PLUS 1p"`、GDI+で確認)を変更。
- 注意: `.gitignore`の`*.txt`で`OFL_MPLUS1p.txt`が無視される。OFLはライセンス文の同梱が条件なので`git add -f`で追加する。
- 候補だったが不採用: マキナス(Makinas)。SF感は一番強いが再配布禁止のため、公開リポジトリにttfを置けない。
- **未確認: ゲーム内表示**(ユーザーがF5で確認する)。
- **追記(同日): マキナス 4 Flat に再変更。** M PLUS 1p を実際に見たユーザーの評価は「ただのゴシック体は仕事感が出てダサい」。ゴシック体以外の9種(マキナス4 Square/Flat、DotGothic16、Dela Gothic One、Rampart One、RocknRoll One、Reggae One、Train One、Potta One)を実際の文で比較し、ユーザーが **Makinas 4 Flat** を選んだ。
  - `MPLUS1p-Bold.ttf`と`OFL_MPLUS1p.txt`は削除。`message_font_path = L"Data/Fonts/Makinas-4-Flat.otf"`、`message_font_name = L"Makinas-4-Flat"`(フォントファイル内のWin32ファミリー名)。
  - ライセンス(もじワク研究): 商用可、ゲーム内表示OK。ただし無断の二次配布・アップロードは禁止 → 一度`.gitignore`に入れたが、ユーザーより「リポジトリはprivateなので再配布は考えなくてよい」とのことで取り消し、otfもコミット対象にした(公開する場合は要再検討)。
  - **未確認: ゲーム内表示**(otfがDxLibで読めるか、F5で確認)。

### 設計・タスク（2026-10-09・設定画面とチャージ操作の切り替え）
**動機(ユーザー)**: スターフォックス式チャージショット(長押し→離す→もう一度押して発射)が「分かりづらい」という声が複数人から出た。よくある「長押し→離す」で撃てる方式にオプションで切り替えられるようにしたい。あわせてポーズから開ける「設定」を作る。

**設定項目(ユーザー決定)**
- 音量
- 移動の切り替え(スティックの方向にそのまま動く / 上に倒すと下に動く=Y軸反転)
- チャージショットの打ち方(現行: 長押し→離す→再度押す / 新: 長押し→離すで発射)
- フルスクリーン切り替え

**現状の把握(Claudeがコードを読んで確認)**
- チャージ: `NormalShootState`で10フレーム長押し→`ChargeShootState`→`ChargeReadyState`。`ChargeReadyState`は離した後60フレーム以内に`IsTriggered(shoot)`で発射。
- スティックYを読む場所は4か所: `MovingState`/`IdleMovementState`/`GaugeActionStateBase`/`DefaultRotationState`(`GetBufY()`直呼び)。
- `PauseScene`の選択肢は画像2枚(ゲームに戻る/タイトルに戻る)+デバッグ用の文字。`SoundManager::SetVolume`は既にある。`ChangeWindowMode`は`Application.cpp`にある。

**設計案(Claude提案、未決定)**
- `GameSettings`(シングルトン)に音量・Y反転・チャージ方式・フルスクリーンを持たせる。ファイル保存は後回し。
- `SettingsScene`を新設し、`PauseScene`から`PushScene`。上下で項目選択、左右で値変更、Bで戻る。最初は画像なしの文字表示(デバッグ項目と同じ)。
- Y反転: 4か所に`if`を書かず、`InputManager`に「設定反映済みのY値を返す関数」を1つ作って4か所を置き換える。
- チャージ方式: `ChargeReadyState`の「再度押して発射」を、方式によって「離した瞬間に発射」に切り替える。「溜まっているか」と「発射トリガー」を分けておく。
- フルスクリーン: DxLibは`ChangeWindowMode`でグラフィックハンドル/シェーダが消える場合があるので実機確認が必要。リスクが高いので最後。

**タスク(この順を提案)**
1. `GameSettings`を作る
2. `PauseScene`に「設定」を追加し、`SettingsScene`の枠を作る
3. チャージ方式の切り替え(要望が一番強いので先に)
4. Y軸反転(`InputManager`に共通関数)
5. 音量
6. フルスクリーン(実機確認が必須)

**先に決めること**
- 音量はBGM/SE別か、全体1本か
- 設定を次回起動に引き継ぐ(保存する)か。日程的には後回しでよいと思う

**注意**: 現在の優先は「チュートリアルのメッセージ表示」の続きと2面制作。この設定機能は日程(試作→10/24、アルファ→11/24)を見て差し込む。

### 2026-10-09 チュートリアルの続き(現状確認)
- ユーザーの作業(未コミット): `TutorialMessageScene`のコンストラクタで文を受け取り、`FontID::Message`(マキナス4 Flat)で`DrawStringToHandle`。OKボタン(`InputEvent::ok`)で`PopScene`。`GetSceneID`は`SceneID::Game`。`GameScene::Update`の末尾で`m_pPlayer->GetPos().z > tutorial_z`(2000)なら`PushScene`。`Application.cpp`は`TitleScene`に戻っている。
- Claudeの指摘(答えは言わず、F5で確かめてもらう): 今の条件だとZ2000を超えた後は毎フレーム条件が真になる。`PopScene`した次のフレームに何が起きるか。
- ほかの気づき: 文字が四角の左上の角にくっついている(余白なし)。ショットとOKが同じボタンなら、撃ちながら通過した瞬間に読む前に閉じる可能性(要確認)。

### 2026-10-10 `.editorconfig`にタブ指定を追加
- VS内でコピペすると1行目以外がスペースになる問題。VSの個人設定(スペースの挿入)が原因と推測。家・学校どちらでも効くよう、`.editorconfig`のC++セクションに`indent_style = tab`/`indent_size = 4`/`tab_width = 4`を追加(ユーザー了承、Claudeが編集)。VSでファイルを開き直すと反映。既存のスペース行は`Ctrl+K, Ctrl+D`で直す。

### 2026-10-10 `TutorialController`を作成、動作確認OK
- ユーザーの発案で、チュートリアルの進行を`GameScene`から分離した`Manager/TutorialController.h/.cpp`を新規作成(ユーザーが実装、Claudeはレビュー)。シングルトンにせず`GameScene`のメンバー(`Init`で`Player`生成後に作成、`Update`の末尾で呼ぶ)。
- 構成: `enum class TaskType{None,Move,Tilt,BarrelRoll,Boost,Brake,Shot,ChargeShot}`(public)、`enum class StepState{WaitTrigger,Trying,Success}`、`struct StepData{triggerZ,message,taskType}`、`std::vector<StepData> m_steps`、`m_stepIndex`、`m_successFrame`。`weak_ptr<Player>`と`SceneController&`を受け取る。
- 流れ: `WaitTrigger`でZを超えたら`TutorialMessageScene`を`PushScene`して`Trying`へ(メッセージ表示中は`GameScene::Update`が止まるので「表示中」状態は不要)→成功で`Success`→2秒(`to_next_step_frame`)で次のステップへ(index++、フレーム0、`WaitTrigger`に戻す)。全部終わったら何もしない。
- 設計の判断: 「各ステップの完了bool」は進行度(index)と重複するので持たない。`TaskType`は文字列でなくenum(CSVからは読み込み時に1回変換)。
- 仮の部分: ステップはコンストラクタに直書き(Z2000「移動しろ！」/Z3000「傾けろ！」)。成功判定は`InputEvent::Tutorial`(デバッグ専用キー)を押したら成功→**Releaseではビルドが通らないので要対応**。
- 途中で直したバグ: Success後に状態・フレームを戻さず次ステップが飛ばされる/`if (pPlayer) return;`の条件が逆。
- **F5で動作確認OK**(ユーザー)。


### 2026-10-12 チュートリアルUI(`TutorialUI`)の進捗
- `Game/UI/TutorialUI.h/.cpp`(`UIBase`派生)。`TutorialController`のコンストラクタで生成し`UIManager`に登録(`GameScene`から`m_pUIManager`を渡す)。状態は`enum class TaskDrawState{Hide,Show,Success}`、外から`OnStart(taskText)`/`OnSuccess()`/`OnEnd()`。
- 右上(画面比率0.87,0.1)に黒い枠を`DrawBox`。枠の半幅150・半高20に`GetUIScale()`を掛けた。
- `OnStart`は`Trying`中に毎フレーム呼ばれるが、`TutorialUI::OnStart`の先頭で`Hide`以外なら`return`して1回だけ効くようにした(ユーザー判断: 状態を増やしたくない。PushScene時に呼ぶとメッセージの後ろに枠が見えてしまうため)。
- 今は右上にも`message`を渡している。`taskText`は**CSV化のときに`StepData`とCSVの列に追加する**(ユーザー決定)。
- フォントは`FontID::Message`(60px)を流用。大きければ同じファイルでサイズ違いの`FontID`を追加する案。

### 後でやるタスク(2026-10-12 ユーザー報告、未調査)
1. **タイトルで早めに決定(演出スキップ)を押すと、すぐフェードせず少し固まってから非同期ロードに入る。** フリーズと勘違いされるので直したい。(推測: スキップ時に同期ロードや重い初期化が走っている、またはフェード開始前に何かを待っている。`TitleScene`のスキップ処理と`ChangeScene`→`LoadingScene`の流れを確認する)
2. **タイトルシーンで、プレイヤー(`TitlePlayer`)が回転する演出のステート中に演出スキップすると、プレイヤーが回転し続ける。** タイトルのステートとプレイヤーの動きが連動していない(スキップでタイトル側のステートが進んでも、プレイヤーの回転が止まらない)のが原因。ゲーム中のバレルロール(`DefaultRotationState`)とは無関係。(2026-10-13 ユーザーが訂正)

### 2026-10-12 タスクUIのチェックマークアニメーション(完了)
- `TutorialUI`: 右上の枠(背景0x777777→枠線 通常0x00bb00/成功0xdddd00、太さ6)＋テキスト(`FontID::Task` 30px)＋左側のチェックボックス(黒、半幅13、画面比率0.805,0.1)。描画順は 背景→枠→テキスト→チェックボックス→✓(DxLibは後に描いたものが上)。
- 成功時、`DrawCheckMark(const Vector2&)`で✓を「線が描かれていく」アニメーション(A案)。3点(start(-8,0)/corner(-3,6)/end(8,-7)、チェックボックス中心からのoffset)、20フレーム、境目0.35までに1本目、残りで2本目。`Vector2::Lerp`使用。`m_checkAnimFrame`は`OnSuccess`で0に戻し`Update`で増やす。
- 学び: `uiScale`は「ずらし量・長さ・太さ」にだけ掛ける。Lerpのt(割合)や計算済みの画面座標には掛けない。`std::min`で1本目の進み具合を1に止める(止めないとLerpが終点を越える)。
- 太い線は端が直角に切れるので角に隙間が出る。対策は角に`DrawCircleAA`(直径=太さ)だが、**太さ3なら目立たないので見送り**(ユーザー判断)。
- 見た目の調整(文字位置の`100`を文字幅から計算、余白、下のメッセージ枠も)は後回し。

### 2026-10-13 チュートリアルのタスク成功判定(全TaskType)
- `TutorialController::IsTaskCleared(const Player&)`(private)で`taskType`ごとに判定(ユーザー実装、方針A: Controllerが毎フレームPlayerに聞く)。`Trying`に入る瞬間に開始時の値を記録(条件なしで毎回)。
  - Move: 開始位置からのXYだけの距離(`Vector2(diff.x,diff.y).Length()`) > `move_task_distance`(500)。Zを入れると自動前進で成功してしまうため除外。
  - Tilt: `abs(GetRotationZ())` > 45°(左はマイナスなのでabs)。
  - BarrelRoll: `IsRolling()`(=`DefaultRotationState::m_isStartRolling`、2回押しのバレルロール)。
  - Boost/Brake: `Player::IsBoosting()/IsBraking()`を追加(`IsSomersault`と同じく`dynamic_pointer_cast`で`m_pMovementState`を判定)。
  - Shot/ChargeShot: **その弾で敵を倒したら成功**(ユーザー希望)。
- 「何で倒したか」の仕組み(方針A、ユーザーの依頼でClaudeが実装、勉強用):
  - `EnemyBase::DamageSource{None,NormalShot,ChargeShot}`と`m_lastDamageSource`/`SetLastDamageSource`。`EnemyCollider::OnCollision`で`TakeDamage`の**前に**記録(`PlayerBullet`→NormalShot(反射弾含む)、`ChargeExplosion`→ChargeShot。チャージ弾本体は雑魚にダメージなし)。
  - `EnemyBase::OnEnemyDead`で従来の`AddDefeatedEnemyCount()`に加え、種類に応じて`Player::AddNormalShotDefeatedCount()/AddChargeShotDefeatedCount()`。
  - Tutorialは`m_normalShotDefeatedStartCount`/`m_chargeShotDefeatedStartCount`から増えたら成功。
  - 一度入れた「撃った回数」カウンター(`AddNormalShoot`等、3つのShootState)は不要になったので取り消し。
- ついでに: ヘッダの重複`private:`、未使用の`InputManager.h`includeを削除。Debug/Releaseともビルド成功(Claude確認)。**ゲーム内の動作は未確認**。
- 未解決: 敵を撃ち漏らして通り過ぎるとShot系タスクが成功不能→「成功するまで先に進ませない仕組み」で対策する。

### 2026-10-13 チュートリアル通し確認用の調整(Claudeが実装、ユーザーが最終確認する)
- `TutorialController`のコンストラクタの仮ステップを7つに(初期化リストで`m_steps = {{triggerZ, message, taskType}, ...}`): 移動1300/傾き2800/バレルロール4300/ブースト5800/ブレーキ9200/ショット10500/チャージ12600。根拠: プレイヤーはZ=1200開始・8/フレーム(約480/秒)、成功後2秒で次へ。ブースト・ブレーキはゲージ満タン必須(回復約3.3秒)なのでブレーキを離した。
- ステージ1の敵配置(チュートリアル区間を安全にし、的を用意):
  - FloatingEnemy: Z4000の2体とZ7500のリング6体を削除。ショットの的としてZ11500(0)/12000(-250)/12500(250)、y500に3体追加。チャージの的は既存のZ14500の9体。
  - WormEnemy: Z5500(起動2000)を削除。Z10500の起動Zを7000→9500(ショット区間で出る)。
  - 岩は変更なし(Z1500とZ5000に中央の岩あり)。
- **(2026-10-14)ユーザーが通しで確認し「いい感じ」。** 指摘: ①敵がちょっと多い(場所は未確認) ②バレルロールがカウンター(敵弾の跳ね返し)になることを説明していない ③**カウンターもタスクにする**。
- **(2026-10-14決定)カウンター**: `TaskType`は増やさず、**バレルロールのタスクの成功条件を「敵弾をカウンターしたら」に変える**(バレルロールとカウンターを同時に説明)。`Player::GetCounterSuccessCount()/AddCounterSuccess()`を追加済み、`CounterCollider::OnCollision`で相手が`EnemyBullet`のときだけ加算(ユーザー実装)。`TutorialController`は開始時の回数を`m_counterSuccessCount`に記録(名前は`...StartCount`への変更を提案中)。`IsTaskCleared`のBarrelRollはまだ`IsRolling()`のまま。
- **(2026-10-14決定)敵とメッセージを「前のタスクの成功」基準に(A案)**: チュートリアル用の敵はステージCSVから外し、各ステップ開始時に`TutorialController`が`EnemyFactory::Create(pos,type)`でプレイヤー前方に生成する。タスク未成功のまま敵がいなくなったら出し直す(撃ち漏らし対策・その場で練習できる)。メッセージも`triggerZ`ではなく「前のステップが終わったら次」にする。浮遊敵の参考: 距離4000未満で出現→3秒間プレイヤーと並走し0.5秒ごとに弾(6発)→離脱。
- **未決定**: 自動前進で手間取るとZ14500以降の本番の敵区間に入ってしまう問題の対策。「一定Zでチュートリアルを強制終了」はユーザーが却下(タスクができないまま終わるのは不親切)。
- **(決定・ユーザー案)巻き戻し**: 移動・傾き・ブースト・ブレーキは区間を設けず成功まで続ける。敵が要るタスク(バレルロール=カウンター/ショット/チャージ)は、**成功しないまま敵がいなくなったら、フェードしてタスク開始地点(`m_taskStartPos`)に戻し、敵を出し直す**。「もう一度！」等の一言を出す案。戻すもの: プレイヤー位置、カメラ(補間追従を即座に合わせる)、飛んでいる弾(敵弾・自弾)、残っている敵、必要ならエフェクト・ゲージ。暗転中に入れ替える。(Claudeの「前進を止める」案は不採用)
- **順番**: ①ステップ開始時に`TutorialController`が`EnemyFactory`で敵を出す → ②巻き戻し。
- **(10/14 実装)** 位置を戻す巻き戻しはせず、「敵がいなくなったらその場で出し直す」形で実装した(下の「タスク用の敵の出し直し」参照)。

### 進捗（2026-10-14・チュートリアルの敵をTutorialControllerから出す、作業途中）
- 確認したこと: `EnemyFactory::Create(pos,type)`は生成→`Init()`(中で`GameObjectManager`に登録)→Target/CollisionManagerへの登録まで行うので、呼ぶだけで動く(ボスの召喚と同じ)。`GameScene`では`EnemyFactory`の生成(171行目)が`TutorialController`の生成(266行目)より前。浮遊敵は距離4000未満で出現→3秒並走して0.5秒ごとに撃つ→`LeaveState`で左右へ離脱し**7秒後に`OnDead`**(②の「敵がいなくなった」判定では、離脱中・プレイヤーの後ろに行った敵も「いない」扱いにする必要がありそう)。
- **ユーザー実装済み**: `TutorialController.h`で`EnemyFactory.h`をinclude(ユーザー判断で許容)。`struct SpawnData{Vector3 offset; EnemyFactory::EnemyType type;}`、`StepData::spawnData`(`std::vector<SpawnData>`)、コンストラクタに`std::shared_ptr<EnemyFactory>`を追加しメンバ`m_pEnemyFactory`(weak_ptr)。
- **Claude編集(ユーザー依頼、仮の`m_steps`のみ)**: 全ステップに敵リストを追加。バレルロール=浮遊敵2体(±300,0,2500)、メッセージに「回転中は敵の弾を跳ね返せる！」を追加。ショット=3体(-400/0/400,0,2500)。チャージ=5体の十字(中央と上下左右±300、前方2500。爆発半径700に収まる)。他は`{}`。`triggerZ`は据え置き。**チャージの下の1体(Y-300)は海面付近だと海に沈む可能性あり、要確認。**

### ブラッシュアップの方針（2026-10-14・ユーザー決定）
- **これからのブラッシュアップには「プレイヤーに不親切な部分をなくしていく」ことも含む。** 例: 敵の攻撃の予備動作(撃つ前に分かるようにする)。
- 設計案を出すときは、締め切り優先で不親切な解決策(できないまま先へ進める・強制終了するなど)を勧めない。親切さを保ったまま軽く実装できる案を優先して提案する。
- 気づき(未対応): 右上のタスク枠は約8文字分しか入らないので長い文ははみ出す(`taskText`追加で解決予定)。`InputManager.cpp`のLB/RBのコメントが逆(RIGHT_SHOULDERはRB)。メッセージを閉じるOK(A)とブレーキ(A)が同じボタンだが、ブレーキは`IsTriggered`なので閉じた押下では発動しない。

### 2026-10-14 重さ対策(チュートリアル中に敵が多いと重い)
- ユーザーの判断: 原因は「敵が多い＋弾が多い」。
- 実施: CSVからチュートリアル区間の敵を削除(Claude、ユーザー依頼)。FloatingEnemyのZ11000(8体)/11500/12000/12500(各1体)、WormEnemyのZ10500。これで本番の敵はZ14500の9体＋ワーム2体(起動Z11000)から。
- (済)敵弾は画面外に出たら消す(`EnemyBullet`で`CheckCameraViewClip_Box`)、離脱した浮遊敵も画面外で消す(`LeaveState`)。
- 候補(未実施): ①`BulletBase.cpp`の`life_time`(300フレーム=**5秒**)のコメントが「3秒」になっている ②`HideState`中の浮遊敵は描画・アニメーションを省く(今は状態に関係なく毎フレーム描画) ③`GameObjectManager`のfor文2か所(27・49行目)が`shared_ptr`を値コピーしている→`const auto&`。
- 注意: Z14500のワーム2体は起動Z11000なので、チャージのステップ(12600)の頃に出てくる。

### 2026-10-14 チュートリアル確認のためステージの雑魚を一時的に全削除
- ユーザー依頼(Claude実施): `Stage1/FloatingEnemy.csv`と`Stage1/WormEnemy.csv`をヘッダ行だけにした(雑魚0体)。ボス・岩はそのまま。ヘッダだけのCSVは`CSVDataLoader`で空の配列になるので落ちない。
- (済・10/15)チュートリアル後にステージを開始する仕組みを作り、雑魚CSVは10/07のレベルデザイン(`f039c0a`)から戻した。

### 2026-10-14 チュートリアルの進行をZ基準から時間基準に(ユーザー実装、動作確認OK)
- `StepState`: `WaitTrigger`→`WaitNextStep`(1秒待ってからメッセージ・開始値の記録・敵生成→`Trying`)、`Trying`、`Success`(✓を`success_show_frame`=2秒見せる→`OnEnd`・index++→`WaitNextStep`)。最初の状態も`WaitNextStep`なので開始直後に1秒の間が入る。「✓を見せる時間」と「次までの間」はユーザー判断で別々の待ちにした(個別に調整できる)。
- `StepData`から`triggerZ`を削除(`TutorialData.csv`のヘッダーからも後で消す)。
- BarrelRollの判定を`GetCounterSuccessCount() > m_counterSuccessStartCount`に変更(反射弾は撃った敵を追尾するので跳ね返せば当たる)。
- 敵が撃つのは画面内(`!CheckCameraViewClip(敵の位置)`)にいるときだけ(浮遊敵`ActiveState`・ワーム)。`EnemyBullet::Update`で弾を包む箱(位置±半径)が`CheckCameraViewClip_Box`で視野外なら`OnDead`。プレイヤーの弾(チャージの追尾)は対象外にするため`BulletBase`ではなく`EnemyBullet`に書いた。

### 2026-10-14 タスク用の敵の出し直し(リスポーン)と離脱敵の画面外削除(ユーザー実装、動作確認OK)
- `TutorialController`: `SpawnTaskEnemies(const Player&)`に敵生成を切り出し(`WaitNextStep`と`Trying`から呼ぶ)。`IsDeadAllTaskEnemies()`で`m_pTaskEnemies`が全部`expired()`か判定。`Trying`で「未成功 かつ `!spawnData.empty()` かつ 全員いない」なら出し直す。`IsTaskCleared`は`Trying`の最初で1回だけ計算して使い回す。
- 敵を持っている所はどれも`weak_ptr`(TargetManager/CollisionManager/EnemyBulletのShooter)なので、`GameObjectManager`から外れれば`expired()`になる。
- バグ(修正済み): 出し直し条件が`spawnData.empty()`(`!`抜け)で、敵ありのステップで出し直しが起きず、バレルロールがクリア不能だった。
- `LeaveState`: 4秒だと画面内で消えて不自然→7秒にしていたが、**画面外(`CheckCameraViewClip_Box`、当たり判定の半径の箱)に出たら`OnDead`**に変更(撃墜ではないので`OnEnemyDead`ではない)。7秒は保険として残す。
- 残りの任意: `IsLeaving()`で離脱開始時点で「いない」扱い(待ちが気になれば)、`IsDeadAllTaskEnemies`→`AreAllTaskEnemiesGone`、離脱の判定半径を見た目に合わせて少し大きく。
- この先: チュートリアルの`Tutorial.csv`化(`taskText`列も)、タスク枠に文字を収める、ステージの雑魚CSVを戻す(チュートリアル後に出す仕組みか配置調整)。

### 設計決定（2026-10-14・チュートリアル後にステージを開始する）
**方針(ユーザー決定)**: チュートリアルが終わってから、通常ステージとして敵などを出していく。ステージの中身は「チュートリアル終了時のプレイヤー位置」基準にする。
- 理由: 今のステージは絶対Zで作られている。敵だけずらすと岩の門と敵の組み合わせがずれる。さらにチュートリアルが長引いてZ27000(`boss_appear_z`)を超えると、チュートリアル中にボスが出てしまう(7ステップに1分かかるとZ30000前後まで進む)。
- Zで決め打ちされているもの: 浮遊敵・ワームの位置(`Stage1/*.csv`)、ワームの`activatePlayerZ`、岩(`Stage1/Rock.csv`)、ボスの位置(`Stage1/Boss.csv`、Z30500)、ボス出現判定(`GameScene.cpp`62行目`boss_appear_z`=27000)。
- **岩もずらす**(ユーザー決定)。チュートリアル中は岩のない海を飛ぶ。
- **チュートリアル終了の伝え方**: `TutorialController`に`IsFinished()`を足し、`GameScene`が毎フレーム聞く(ユーザー決定)。

**ずらし量**: `チュートリアル終了時のプレイヤーZ − 1200`(1200=プレイヤーの開始Z、`Player.cpp`の`first_pos`)。こうするとチュートリアル終了時のプレイヤーから見て、ステージの中身が今までのゲーム開始時と同じ距離に並ぶので、**CSVの値は書き直さずに使える**(消した雑魚CSVは`git`で戻すだけ)。


**(2026-10-15 変更)ずらし方を変えた(ユーザー案)**
- **Stage1のCSVを全部Zで−1200した**(Claude実施)ので、CSVの値は「ステージ開始時のプレイヤーからの距離」。**ずらし量＝チュートリアル終了時のプレイヤーZそのもの**(−1200はコードに不要)。対象: `FloatingEnemy`/`WormEnemy`(posZと`activatePlayerZ`)/`Rock`/`Boss`(30500→29300)。
- 雑魚CSVは、チュートリアル用に削る前の**10/07のレベルデザイン(コミット`f039c0a`)から戻した**(配置はユーザーがClaudeに一任「面白ければいい」。今はそのまま)。
- ユーザーが作った`Player::GetFirstPos()`はユーザー依頼でClaudeが削除。`GameScene::Update`の呼び出しは`CreateStageObjects(m_pPlayer->GetPos().z)`に。
- **ボスは`Init`で作ったまま、ステージ開始時に`SetPos`でZにずらし量を足す**(`m_pBoss`は`CollisionManager`/`BossHPGaugeUI`/`Update`の`IsDead`・`IsDying`で生成直後から使われるため、生成を遅らせるとnullで落ちる。`BossEnemy`は最初の位置を別に覚えていないので`SetPos`だけでよい)。`boss_appear_z`は27000→**25800**にしてずらし量を足し、ステージ開始前は判定しない。
- **Unityの`Stage1.unity`はCSVとずれた**(Zが1200違う)。Unityで書き出す前に`StageCsvImporter`で取り込み直すこと。
- **(10/15)ユーザー実装・動作確認OK**: `CreateStageObjects(offsetZ)`、`m_isAppearEnemy`で1回だけ、`m_stageOffsetZ`、3つの`DataSetter`に`offsetZ`(ワームは`activatePlayerZ`も)、ボスは`CreateStageObjects`の最後で`SetPos`、`boss_appear_z`=25800+オフセット、ステージ開始前は判定しない。`m_isApear*`→`m_isAppear*`に綴り修正。
- **(10/15)ユーザー報告の細かい問題**:
  1. メッセージ→ボタンで閉じるまでの猶予がなく、連打ですぐ閉じる(`TutorialMessageScene::Update`が表示直後からOK(A)を受け付ける。ブレーキもA)。案: 経過フレームを数えて約1秒は閉じない＋受付後に「Aで次へ」を表示。
  2. 岩などが急に出てくる(チュートリアル終了時に一括生成、最も近い岩は300先、`camera_far`=5500の内側なので目の前に出現)。案: `m_stageOffsetZ = プレイヤーZ + GetFarClip()`で描画範囲の外に作る。副作用で最初の岩まで約12秒の空白→「MISSION START」等の演出で埋める案。未決定。
  3. 浮遊敵の爆発エフェクトがおかしい気がする(症状は未確認)。
  3の結論: 浮遊敵は爆発を`Play`してハンドルを持ち、デストラクタで`Stop`していた→`trueDeadFrame`(10F)で消えると爆発が途中で切れていた(ワームは`PlayOneShot`なので無事。ファイルは同じ`EnemyDeath.efk`、倍率が浮遊敵1.5/ワーム1.0)。**`PlayOneShot`に変更する(ユーザー決定)**、`m_effectPlayHandle`・104行目の`SetPos`・デストラクタの`Stop`は不要に。
  5. **ブースト後にブレーキのタスクが来ると、ゲージ満タン待ちの時間ができる**→ゲージを使うタスクではゲージを満タンにしてあげる(ユーザー)。`Player::ChangeGauge(delta)`/`GetMaxSpecialGauge()`(100)で外から満タンにできる。**決定: タスク開始時に1回だけ満タン(A案)**。「タスク中は使っていなければ常に満タン(B案)」は親切だが、ゲージの仕組みをプレイヤーが誤解する(回復が速い/減らないと思う)のでユーザーが不採用。
  6. **宙返りのチュートリアルがない**。宙返り=十字キー上(キーボードZ)、ゲージ50より多く必要(`somersoult_use_gauge`)。判定は既存の`Player::IsSomersault()`が使える。
  - **(10/15終了時点の状況)** 3の爆発は`PlayOneShot`に修正済み(ユーザー、`m_effectPlayHandle`も削除済み)。残りを手間の少ない順に: ①メッセージの猶予(方針あり) ②ゲージ満タン(方針あり、A案) ③宙返りタスク(**入れる位置が未決定**。ブレーキの後を提案中。`m_steps`はClaudeが書く) ④岩の急な出現(**先回り後の約12秒の空白をどうするか未決定**) ⑤カウンターのリアクション(案のみ)。
  - **次回はまず①から(ユーザーが書く)**: `TutorialMessageScene`に経過フレームのメンバ(例`m_frame`)を足して`Update`の先頭で増やす→定数(例`close_enable_frame = 60`)未満の間はOKを押しても`PopScene`しない→`Draw`で定数以上になったらボックス右下に「Aで次へ」を描く(点滅させるなら`(m_frame / 30) % 2 == 0`のときだけ描く)。
  - ②の書き方: `TutorialController`の`WaitNextStep`→`Trying`の切り替え時(開始値の記録・敵生成をしている所)で、`taskType`がブースト/ブレーキ/宙返りなら`pPlayer->ChangeGauge(pPlayer->GetMaxSpecialGauge() - pPlayer->GetSpecialGauge())`。タスクが増えるなら`StepData`に「開始時に満タンにするか」の`bool`を持たせる案も。
  - ③で足すもの: `TaskType::Somersault`、`IsTaskCleared`に`case TaskType::Somersault: return player.IsSomersault();`、`m_steps`に1行(メッセージ案「十字キー上で宙返りしろ！」)。
  4. **カウンター成功時のリアクションがない(タスク)**。案(ユーザー): 一瞬スロー(ヒットストップ的)／画面効果(ポストエフェクト)／ヒットエフェクト(ユーザー評価は「一番合わなさそう」)。未着手。カウンター成功の検知場所は`CounterCollider::OnCollision`(相手が`EnemyBullet`のとき、`AddCounterSuccess`している所)。

### 2026-10-15 チュートリアル: メッセージを閉じるまでの猶予(①、完了)
- `TutorialMessageScene`: `m_frame`を`Update`で増やし、`CanClose()`(`m_frame >= close_enable_frame`=60)の間だけOKで`PopScene`。閉じられるようになったら箱の右下にリザルトと同じ「Aボタン＋次へ」の画像を`DrawRotaGraph`で描く(ユーザー実装、位置はClaude: 箱の右下の角からA(-110,-45)/次へ(-50,-45)、拡大率0.45、`uiScale`を掛ける)。
- `ResourceLoader.cpp`の`game.graphics`に`ButtonA`/`NextText`を追加(Claude、ユーザー依頼。今まで`clear.graphics`にしか無かった)。
- つまずき: `DrawRotaGraph`の4番目の引数は回転角(ラジアン)なので`1.0`だと約57°傾く。中心座標で置くので角の座標をそのまま渡すと半分はみ出す。
- 次: ②ゲージを使うタスクで開始時にゲージ満タン(A案) → ③宙返りタスク(位置未決定、ブレーキの後を提案)。

### 2026-10-15 チュートリアル: ゲージ満タン(②)と宙返りタスク(③)
- ②(ユーザー実装): `WaitNextStep`→`Trying`の切り替え時、`taskType`がBoost/Brake/Somersaultなら`ChangeGauge(GetMaxSpecialGauge() - GetSpecialGauge())`で満タンにする(A案)。Somersaultの条件はClaudeが追加(ユーザー依頼)。
- ③: `TaskType::Somersault`(ユーザー)、`IsTaskCleared`で`player.IsSomersault()`(ユーザー)、`m_steps`のブレーキの直後に「十字キー上で宙返りしろ！」(Claude)。Debugビルド成功。**ゲーム内は未確認。**
- 残り(チュートリアル): ④右上のタスク枠に文字を収める(`taskText`、`Tutorial.csv`化とあわせるか)、⑤岩が急に出る(先回り生成と約12秒の空白)、⑥カウンター成功のリアクション。

### 2026-10-15 チュートリアル: 移動タスクがクリアできない問題(解決、ユーザー確認OK)
- 原因: 判定は「開始位置からの距離 > 500」。上下の可動範囲はY100〜900で開始Y500からは最大400しか離れられず、上下だけでは届かない。また「ぐりぐり」回すと開始位置の周りを回るだけで離れない。
- 対応(ユーザー判断でB案): `move_task_distance`を500→100にした。A案「動いた量の合計で判定」は不採用。
- **ここまで(10/15家)**: ①メッセージの猶予 ②ゲージ満タン ③宙返りタスク 完了。
- **(10/16決定)CSV化も同時にやる**: `Data/CSV/Stage1/Tutorial.csv`(`stepID,message,taskText,taskType`、1行=1ステップ、上から順)と`TutorialEnemy.csv`(`stepID,enemyType,offsetX,offsetY,offsetZ`、1行=敵1体)。読み込みは`TutorialStepData`/`TutorialEnemyData`(`CSVData`派生、`taskType`/`enemyType`は文字列のまま持ち、enumへの変換は`TutorialController.cpp`で対応表+assert)。CSVはClaudeが今の`m_steps`から作る、クラスと読み込みはユーザー。日本語入りなのでVS Codeで編集(ExcelのCSV保存は文字化け)、メッセージに半角`,`は使えない。**順番: まず文字を枠に収める→CSV化**。
- **タスク(未着手)**: メッセージを1文字ずつ出す演出(タイピング風)。
- **運用ルール(10/16ユーザー依頼)**: Claudeがコードの追記・修正箇所を説明するときは、**必ずファイル:行の飛べるリンクを付ける**(NOTES.mdに書く手順も同じ)。

### 2026-10-16 右上のタスク枠を文字の長さに合わせる(①、作業中・ユーザーが書く)
- 考え方: 枠の**右端を画面比率で固定し、左に伸ばす**。中身を左から `|余白|☐|すき間|文字|余白|` と並べ、左端 = 右端 −(余白×2+チェックボックスの幅+すき間)×`uiScale` − 文字の幅。
- 手順(行番号は10/16時点):
  1. 文字の幅を`OnStart`([TutorialUI.cpp:172-180](Game/UI/TutorialUI.cpp#L172-L180))で1回だけ測る: `GetDrawStringWidthToHandle(m_taskText.c_str(), static_cast<int>(m_taskText.size()), m_taskFontHandle)`。メンバ(例`int m_taskTextWidth`)を[TutorialUI.h:25](Game/UI/TutorialUI.h#L25)の`m_taskText`の下に追加。**フォントは読み込み時に`GetUIScale()`済みなので、測った幅に`uiScale`を掛けない**(掛けるのは自分で決めた余白などの定数だけ)。
  2. 定数を整理([TutorialUI.cpp:12-23](Game/UI/TutorialUI.cpp#L12-L23)): `task_box_pos_ratio`のx・`center_to_horizontal_end`・`check_box_pos_ratio`を消し、枠の右端の比率(例0.95)・高さ中心の比率(今の0.1)・左右の余白(例15)・チェックボックスと文字のすき間(例10)を足す。`center_to_vertical_end`/`check_box_center_to_end`はそのまま。
  3. `Draw`で位置を計算([TutorialUI.cpp:55-60](Game/UI/TutorialUI.cpp#L55-L60)の位置計算を置き換え): 右端→左端→チェックボックス中心x=左端+(余白+チェック半分)×uiScale→文字x=チェック中心x+(チェック半分+すき間)×uiScale。背景と枠線の`DrawBox`([66-72](Game/UI/TutorialUI.cpp#L66-L72)・[83-89](Game/UI/TutorialUI.cpp#L83-L89))に左端・右端、文字([92-96](Game/UI/TutorialUI.cpp#L92-L96)の`-100`直書き)に文字x、チェックボックス([99-105](Game/UI/TutorialUI.cpp#L99-L105))と`DrawCheckMark`([109](Game/UI/TutorialUI.cpp#L109))に計算した中心を渡す。
  4. (ついでに)文字を上下中央に: y = 枠の中心y − `GetFontSizeToHandle(m_taskFontHandle)`/2([TutorialUI.cpp:94](Game/UI/TutorialUI.cpp#L94))。
- **(完了・ユーザー確認OK)** 右端固定で`boxWidth=(余白×2+チェック幅+すき間)×uiScale+m_textWidth`、`boxLeft=boxRight-boxWidth`、チェックボックス中心=`boxLeft+(余白+チェック半分)×uiScale`、文字x=チェック中心+(チェック半分+すき間)×uiScale、文字y=枠の中心−`GetFontSizeToHandle`/2。定数`task_box_margin`(15)/`check_to_text_space`(10)を追加、`check_box_pos_ratio`は削除。
- その後: ②下のメッセージ枠(CSVの`\n`で改行、[TutorialMessageScene.cpp:78](Scene/TutorialMessageScene.cpp#L78)の書き始めを余白ぶん内側に)→CSV化。
- **(完了)②下のメッセージ枠の余白**(ユーザー実装)。改行はCSVの`\n`で行う(`CSVDataLoader`が改行に変換、`DrawStringToHandle`は改行対応)。

### 2026-10-16 チュートリアルのCSV化(作業中、続きは家で)
- **CSV(Claude作成)**: [Tutorial.csv](Data/CSV/Stage1/Tutorial.csv)(`stepID,message,taskText,taskType`、8ステップ。バレルロールとチャージの`message`は`\n`で2行。`taskText`は右上の枠用の短い文)、[TutorialEnemy.csv](Data/CSV/Stage1/TutorialEnemy.csv)(`stepID,enemyType,offsetX,offsetY,offsetZ`、10体。stepID 2/6/7)。UTF-8(BOMなし)・CRLF。**VS Codeで編集すること**。
- **データクラス(完成)**: [TutorialStepData.h](CSVData/TutorialStepData.h)/[.cpp](CSVData/TutorialStepData.cpp)(ユーザー)、[TutorialEnemyData.h](CSVData/TutorialEnemyData.h)/[.cpp](CSVData/TutorialEnemyData.cpp)(大枠Claude・中身ユーザー)。`taskType`/`enemyType`は文字列のまま返す(enumへの変換は`TutorialController.cpp`。データクラスが使う側に依存しないため)。`GetMessage`はWindowsのマクロ(`GetMessageW`)とぶつかるので`GetMessageText`。
- **途中で直したミス(参考)**: Getterの`return`忘れ、`m_stepID - std::stoi(...)`(`=`のつもりが`-`、C4552)、`Vector3(a, b, c,)`の最後の`,`(関数呼び出しの`()`では不可、`{}`の初期化リストなら可)、`m_enemyType`の代入漏れ。
- **(ユーザー判断)** `Vector3.h`くらいの基本の型は.hでincludeしてよい。
- **設定(Claude、ユーザー依頼)**: 自動生成コードの`{`が関数名と同じ行になる問題→[.editorconfig:29-33](.editorconfig#L29-L33)の`cpp_new_line_before_open_brace_*`を`ignore`→`new_line`、[.vscode/settings.json](.vscode/settings.json)に`C_Cpp.vcFormat.newLine.beforeOpenBrace.*`=`newLine`を追加(VSはファイルを開き直し、VS Codeはウィンドウ再読み込みで反映)。原因はVS/VS Codeの「定義の作成」(`return {};`付きのGetterを作ったのもこれ)。「クラスの追加」タスクのスクリプトは元から次の行。

**`TutorialController`の読み込み(3段階)**
1. **(完了)** `StepData`に`std::wstring taskText;`を追加([TutorialController.h:78](Manager/TutorialController.h#L78))、右上の枠に`taskText`を渡す([TutorialController.cpp:116](Manager/TutorialController.cpp#L116))。仮の`m_steps`の中身は削除済み([TutorialController.cpp:39-42](Manager/TutorialController.cpp#L39-L42)、今は空なので起動するとチュートリアルが即終了しステージが始まる)。
2. **(次・家で)文字列→enumの対応表**: [TutorialController.cpp:12-23](Manager/TutorialController.cpp#L12-L23)の匿名名前空間に`ToTaskType(const std::wstring&)`(`TutorialController::TaskType`を返す)と`ToEnemyType`(`EnemyFactory::EnemyType`を返す)を作る。形は`ResourceLoader::WStringToModelID`と同じ(`static const std::unordered_map`→`find`→見つからなければ`assert`→`it->second`)。TaskTypeは`None`以外の8つ(Move/Tilt/BarrelRoll/Boost/Brake/Somersault/Shot/ChargeShot)、EnemyTypeは`FloatingEnemy`と`WormEnemy`。cppに`#include <unordered_map>`/`<cassert>`。
3. **読み込みに置き換え**([TutorialController.cpp:34-42](Manager/TutorialController.cpp#L34-L42)の仮のコメントと`m_steps =`を置き換え): `LoadCSV(L"Stage1/Tutorial")`の各行を`TutorialStepData`にして、`StepData`(message/taskText/`ToTaskType`/空のspawnData)を`m_steps`に`push_back`。同時に`stepID`→`m_steps`の添字の対応を`unordered_map<int, size_t>`に記録。次に`LoadCSV(L"Stage1/TutorialEnemy")`の各行を`TutorialEnemyData`にして、`stepID`から添字を引き(無ければassert)、その`spawnData`に`{GetOffset(), ToEnemyType(GetEnemyType())}`を`push_back`。
- その後: 動作確認(メッセージの2行表示、右上の`taskText`、敵の出現)→残りは⑤岩が急に出る(先回り生成と約12秒の空白)→⑥カウンター成功のリアクション→タイピング風のメッセージ演出→雨の2面。

### 2026-10-18 チュートリアルのCSV読み込み(完了・ユーザー動作確認OK)
- **CSVをExcelで編集できるように**: `CSVDataLoader::GetWStringList`でUTF-16に変換した直後、先頭が`L'\uFEFF'`(BOM)なら1文字消す([CSVDataLoader.cpp:100-105](Manager/CSVDataLoader.cpp#L100-L105)、ユーザー実装)。Excelの「CSV UTF-8 (コンマ区切り)」で保存するとBOMが付き、ヘッダー名で列を探す(`GetColumnIndex`)ときに1列目が一致しなくなるため。先輩のProjectNeaRはヘッダー行を捨てて列番号で読むので、BOMがあっても困らない(NovaWingはヘッダー名で探すので必要)。
  - **Excelでの編集ルール**: 保存形式は必ず「CSV UTF-8 (コンマ区切り)」(ただの「CSV」はShift-JISで文字化け)。メッセージに半角`,`は使えない。`\n`は日本語フォントでは`¥n`に見えるが同じ文字。`Tutorial.csv`はBOM付きにした(ダブルクリックで開ける)。`TutorialEnemy.csv`はまだBOMなし(初回は「データ→テキストまたはCSVから」でUTF-8指定して開き、CSV UTF-8で保存)。
- **文字列→enum**: `TutorialController.cpp`の匿名名前空間に`ToTaskType`/`ToEnemyType`(`static const unordered_map`→`find`→無ければ`assert`して`None`)。ヘッダーではなく匿名名前空間にした理由: この.cppでしか使わず、メンバ変数も使わないため(ヘッダーに書くとincludeしている全員が再コンパイル)。
- **`EnemyFactory::EnemyType`に`None`を先頭に追加**(Claude、ユーザー依頼)。ボスの召喚([SummonState.cpp:35-40](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossSummonState.cpp#L35-L40))の乱数を`None`の次〜`Max`の手前にした(マジックナンバーなしで`None`/`Max`から計算)。
- **読み込み**(ユーザー実装): `Stage1/Tutorial`を1行ずつ`TutorialStepData`→`StepData`→`m_steps`。`Stage1/TutorialEnemy`を1行ずつ`TutorialEnemyData`→種類が`None`なら飛ばす→`m_steps[stepID].spawnData`に追加。
  - **ルール(ユーザー決定: ステップはあまり増減しないので表を作らない)**: **stepIDは0から順番に振り、行の順番と一致させる**(敵のCSVがstepIDをそのまま添字に使う)。ずれたら起動時に`assert`で止まる。ステップを消す・並べ替えるときは両方のCSVのstepIDを振り直す。
  - 直したバグ: 範囲チェックが`size() < stepID`で`stepID == size()`を素通りしていた→`<=`。
- 残り(チュートリアル): ⑤岩が急に出る(先回り生成と約12秒の空白) → ⑥カウンター成功のリアクション → タイピング風のメッセージ演出。その後、雨の2面。
- (10/18追加・Claude、ユーザー依頼) 下のメッセージの背景に緑の枠線: [TutorialMessageScene.cpp:31-33](Scene/TutorialMessageScene.cpp#L31-L33)に`message_frame_color`(0x60e9a4、Aボタン・次へ画像と同じ緑。右上のタスク枠も同じ色の定数`task_frame_color`に変更)・`message_frame_thickness`(3)、[85-91行目](Scene/TutorialMessageScene.cpp#L85-L91)で背景の左上を`boxLeft`/`boxTop`にまとめ、背景の後に枠線を`DrawBox(..., false, 太さ)`。
- **タスク(10/18ユーザー追加、未着手)**: チュートリアルのメッセージ(下の枠)や右上のタスク枠などにも**グリッチシェーダをかけたい**。参考: `WarningUI`が`GlitchPS.pso`＋定数バッファ(`GlitchBuffer`: time/scanlineFrequency)を`Utility/GraphShaderDraw`で使っている。シェーダは2D画像に掛ける形なので、文字や`DrawBox`に掛けるには一度オフスクリーン(描画先の画像)に描いてからその画像に掛ける必要がある見込み(ClearSceneの文字の光彩と同じ流れ)。
- **タスク(10/18ユーザー追加、未着手)**: **宙返り中もプレイヤーの機体が必ず画面内に映るように、カメラを動かす**(せっかくかっこいいのに見えなくて残念、とのこと)。手がかり: 宙返り中は`Player::ClampPosition`が処理を飛ばす([Player.cpp:277](Game/GameObjects/Actors/Character/Player/Player.cpp#L277))ので、機体が視錐台の外に出られる。カメラは`GameCamera::UpdatePosition`([GameCamera.cpp:62](Game/GameObjects/Camera/GameCamera.cpp#L62)〜)でプレイヤーを追従しており、Yは`camera_move_strength_y`(0.3)倍しか追わない([GameCamera.cpp:97](Game/GameObjects/Camera/GameCamera.cpp#L97))。[GameCamera.cpp:167](Game/GameObjects/Camera/GameCamera.cpp#L167)にも宙返り中かどうかの分岐がある。宙返りの長さは`SomersaultState`の`max_frame`(160F)。案(未検討): 宙返り中だけ追従を強める/注視点を機体に向ける/少し引く。
- **タスク(10/18ユーザー報告)**: 雑魚・ワームの爆発(`EnemyDeath`)と敵の弾(`EnemyBullet`)のエフェクトが重い→Claude Desktop(Effekseer MCP)に軽量化させる。プロンプトはClaudeが作成(このチャットに記載)。重い理由の見積もり: `EnemyDeath`はピーク約250インスタンス/1回で、ワームは頭＋節(5〜7)が13Fごとに連鎖して最大8回、チャージで浮遊敵5〜9体を同時に倒すと同時に5〜9回。`EnemyBullet`はTrack2本(Tail・SeaTrail)を含む7ノードを弾1発ごとに再生(浮遊敵は0.5秒ごと、ワームは1秒ごとに撃つ)。

### 2026-10-18 敵の爆発・敵の弾のエフェクト軽量化(Claude Desktop・Effekseer MCP、ゲーム内未確認)
- 対象: [EnemyDeath.efkefc](Data/Effect/EnemyDeath/EnemyDeath.efkefc)と[EnemyBullet.efkefc](Data/Effect/EnemyBullet/EnemyBullet.efkefc)を上書きし、どちらも**拡大率50で.efkを書き出し直した**。C++は変更なし。[make_efkproj.ps1](Data/Effect/EnemyDeath/tools/make_efkproj.ps1)の数値も新しい値に合わせた(作り直しても元に戻らないように)。
- **EnemyDeath**(1回あたり同時に生きる数の理論値 250→78、エディタで測ったピーク 約215→約68): Smoke 8→4・Fireball 8→4・Fireball2 6→3・FireBloom 4→2・InnerFire 6→3(発生間隔2.5→4F)・Sparks 36→12・Debris 8→5・DebrisTrail 約144→約30(毎フレーム→3Fごと、寿命18±2)・DebrisBurn 8→5・Embers 18→6。HeatGlow/Shockwave/Flash/CoreFlashは1のまま。数を減らした板は大きくしてすき間を埋めた(Smoke 終わり7.5→8.75など)。加算の層が減って火の玉が暗く見えたので、FireBloomのアルファを130→190に上げた。
  - 見た目への影響: 「火の玉が膨らんで黒煙に変わる」は保持。火花・残り火はまばら、破片は5本で煙の尾は少しつぶつぶ(毎フレーム出していたのを3Fごとにしたため、煙の玉を大きくして補った)。
- **EnemyBullet**(弾1発あたり 34→17、測定 36→18(Mainを含む)): Tail 10→5(3Fごと・寿命30→4Fごと・寿命19)・Streak 6→3(2F・12→3F・8)・Ring 5→3(5F・22→6F・17)・SeaTrail 10→3(3F・30→8F・23)。Trackの点の補間(TrailSmoothing)をオフに(まっすぐ飛ぶので不要。スプライン分割は元から1)。SeaGlow/Glow/Coreは1のまま。**寿命を「発生間隔の倍数−1」にして、生まれる瞬間と消える瞬間が重なって数が一瞬増えるのを防いだ**。
  - 色・動的パラメーター入力0(SeaGlow/SeaTrailの位置の式)は変更なし。見た目への影響: 尾の長さが約2/3、リングが3つに。正面から近づく弾でもリングが奥へ重なる見え方は残っている。
- 比較画像(同じカメラ・同じフレーム)はClaude Desktopのチャットに添付(リポジトリには置いていない)。
- **ゲーム内で確認すること**: ①ワームの連鎖爆発(最大8回重なる)で重さと見た目、②チャージで浮遊敵をまとめて倒したとき、③弾がたくさん飛んでいるとき、正面から来る弾の遠近感(リングと海面の光)で避けやすさが落ちていないか、④破片の煙の尾がつぶつぶに見えすぎないか(気になるならDebrisTrailの発生間隔を3→2にする。約+15インスタンス)。

### 2026-10-19 デバッグのプレイヤー移動でチュートリアルをスキップ(Claude、ユーザー依頼、ビルドOK・動作未確認)
- 壊れていた理由: チュートリアル導入後、通常ステージの敵・岩の生成とボス出現の判定が「チュートリアル終了時のプレイヤーZ(`m_stageOffsetZ`)」基準になった。チュートリアル中にワープすると敵が出ず(`m_isAppearEnemy`がfalseのまま)ボスも出ない。チュートリアルが終わった瞬間にワープ先がオフセットになってしまう問題もあった。
- [TutorialController::Skip](Manager/TutorialController.cpp#L244): 残っているタスク用の敵を`OnDead()`で消し(倒した数には数えない)、タスクUIを`OnEnd()`、`m_stepIndex`を全ステップ数にして終了状態にする。
- [GameScene::StartStage](Scene/GameScene.cpp#L279): 今のプレイヤーZをオフセットにして`CreateStageObjects`(Updateにあった処理をまとめただけ。[Update側](Scene/GameScene.cpp#L315))。
- [GameScene::WarpPlayer](Scene/GameScene.cpp#L288): チュートリアル中ならSkip→**ワープ前の位置**で`StartStage`→`m_stageOffsetZ + stageZ`へ移動。物差しのZ(1200〜27000)は「ステージ開始位置からの距離」として扱う(敵・岩・ボスと同じくオフセットを足す)。
- PauseSceneはPlayerを借りるのをやめ、`std::function<void(float)>`でワープ処理を受け取る([PauseScene.cpp:256](Scene/PauseScene.cpp#L256)、渡しているのは[GameScene.cpp:493](Scene/GameScene.cpp#L493))。PauseScene.cppから`Player.h`のincludeを削除。

### 2026-10-19 ワープでボス前に飛んだときのボスとの距離を通常どおりに(Claude、ユーザー依頼、ビルドOK・動作未確認)
- 原因: 10/15にステージの座標を「開始位置0基準」に直した(`boss_appear_z` 27000→25800、ボス30500→29300)が、物差しは1200〜27000のままだった。右端に飛ぶとボスまで2300(普通に進むと3500)で、ムービー明けのカメラ(ボスの2700手前)より前にプレイヤーがいた。
- `boss_appear_z`をGameScene.cppの匿名名前空間から[GameScene.h:37](Scene/GameScene.h#L37)の`static constexpr`に移し、物差しの範囲を[PauseScene.cpp:43-44](Scene/PauseScene.cpp#L43-L44)で`0`〜`GameScene::boss_appear_z`にした(ボスの出現位置を変えても物差しが自動で合う)。プレイヤーは常に前進するので、ちょうど25800に置いても次のフレームで出現判定(`>`)に入る。

### 2026-10-19 カメラ: 端で遠近感が分からない問題(ユーザー実装、動作確認OK)
- 症状: 右端で右を向く(左端で左)と遠近感が分からない。原因は2つ: ①カメラxがプレイヤーxの0.3倍しか追わず斜め後ろから見ていた ②カメラの注視点が「機体の前方38000」なので機体のヨー(最大22.5°)で注視点が約14500横にずれ、カメラ自体が大きく首を振っていた。
- 対応(ユーザー決定): カメラxは**完全追従**(`camera_move_strength_x` 0.3→1.0)＋**上限`camera_move_limit_x`=1000でclamp**([GameCamera.cpp:102](Game/GameObjects/Camera/GameCamera.cpp#L102)、代入の**後**に置く。最初は前に置いて上書きされていた)。プレイヤーのx範囲は`ClampPosition`の計算で約±1600、カメラの実距離は補間の遅れ込みで約420・その位置の画面の片幅約750なので、1000で端の機体が画面の端近くに映る。
- カメラの向きは**振れるまま**、横成分だけ弱める: `lookDir = -GetForward(); lookDir.x *= camera_yaw_rate(0.4)`([GameCamera.cpp:176-178](Game/GameObjects/Camera/GameCamera.cpp#L176-L178))。端ほど弱める案(|x|/1600で倍率を補間)は必要になったら。
- 注意: `ClampPosition`([Player.cpp:274-300](Game/GameObjects/Actors/Character/Player/Player.cpp#L274-L300))は「カメラから900先の画面の幅」をワールド原点基準で使っている(カメラがほぼ動かない前提の作り)。今は結果的に約±1600の固定範囲として働いている。

### 残りのタスク(10/19時点)
1. ⑤岩などが急に出る(先回り生成と約12秒の空白: A「MISSION START」演出 / B 空白を詰める)
2. ⑥カウンター成功のリアクション(スロー/画面効果)
3. タイピング風のメッセージ演出
4. チュートリアルUIにグリッチシェーダ(オフスクリーン経由)
5. 宙返り中もカメラに機体が映るように
6. 軽量化したエフェクト・デバッグワープのゲーム内確認
7. 雨の2面

### 2026-10-19 AIテストプレイ(デバッグ専用ツール、Claude、ユーザー依頼、動作確認済み)
- 置き場所: [DebugTools/AutoPlay/](DebugTools/AutoPlay/)。使い方・仕組み・消し方は [README.md](DebugTools/AutoPlay/README.md)。
- **ユーザー方針: ゲームの既存コードには1行も足さない、Debug専用、すぐ消せる作り**。全C++は `#ifdef _DEBUG`。vcxproj/.filters には New-GameClass.ps1 で4クラス(AutoPlaySystem/AutoPlayWorld/AutoPlayBot/PlaytestRecorder)を登録しただけ。最初に InputManager に仮想パッドを足しかけたが、指示で取り消した(InputManager.h は元どおり)。
- 起動: `.\DebugTools\AutoPlay\Run-AutoPlay.ps1 -Runs 10 -Speed 4 [-Mode hidden] [-Skill beginner|normal|expert]`。終わると [PlaytestReport.html](DebugTools/AutoPlay/PlaytestReport.html) が開く(被弾・死亡の場所を原因別に、カウンター、HP推移、1回ごとの表)。ログは `DebugTools/AutoPlay/Logs/`(.gitignore の `[Ll]ogs/` で対象外)。
- 既存コードを変えずに割り込む方法:
  - Debugのインクリメンタルリンクの ILT(jmp中継)の飛び先を書き換えて DxLib の `GetJoypadXInputState`/`GetNowHiPerformanceCount`/`ScreenFlip`/`PlaySoundMem` を差し替え([AutoPlaySystem.cpp:49](DebugTools/AutoPlay/AutoPlaySystem.cpp#L49))。パッド読み取りが毎フレーム1回なので、そこがAIの処理のきっかけ([AutoPlaySystem.cpp:255](DebugTools/AutoPlay/AutoPlaySystem.cpp#L255))。
  - `GameObjectManager::m_pGameObjects`(private)はテンプレートの明示的インスタンス化の規則で読む([AutoPlayWorld.cpp:30](DebugTools/AutoPlay/AutoPlayWorld.cpp#L30))。
  - チュートリアルはポーズの「プレイヤーを移動」で物差し左端へワープして飛ばす。**ポーズの選択肢の順番を変えたら** [AutoPlaySystem.cpp:35](DebugTools/AutoPlay/AutoPlaySystem.cpp#L35)・[42行目](DebugTools/AutoPlay/AutoPlaySystem.cpp#L42) を合わせる。
- AIの回避は、16方向+行きたい方向に動いたときの危険物とのすき間を数十フレーム先まで比べて一番空く方向を選ぶ([AutoPlayBot.cpp:100](DebugTools/AutoPlay/AutoPlayBot.cpp#L100))。最初は「離れる向きの合計」だったが、Z9300/10100の中央の岩の柱(3段)で上下の押しが打ち消し合って毎回当たっていたので変えた。被弾の原因は、被弾した瞬間にプレイヤーの当たり判定に一番近いものから推測([PlaytestRecorder.cpp:323](DebugTools/AutoPlay/PlaytestRecorder.cpp#L323))。
- 試走の結果(各3回・最大速度・画面なし): 初心者0/3クリア(主な被弾はワーム・岩、死因はワームとボスのビーム)、普通3/3、上級者3/3(被弾は合計1)。普通以上の被弾はワーム(Z1900/6900/24000付近)とボス戦の敵弾が中心。最大速度で約6倍速。
- 未対応: AIはブースト・ブレーキ・宙返りを使わない。倍速でもボス登場ムービーは実時間。
- (10/19追加) VS Code のステータスバーに「AIテストプレイ」「テスト結果」ボタンを追加([tasks.json](.vscode/tasks.json) のタスク2つと input 4つ、`autoPlay*`)。回数・速さ・画面・腕前をメニューで選び、Debugビルドしてから起動する。消すときはこれも外す。

### 2026-10-19〜20 右上のタスク枠を右からスライドイン・アウト(ユーザー実装、コミット`69a6e7e`)
- 状態: `TaskDrawState{Hide, SlideIn, Show, Success, SlideOut}`。`OnStart`→`SlideIn`、`OnEnd`→`SlideOut`(どちらも`m_slideAnimFrame = 0`)。`Update`でSlideIn/SlideOut中だけフレームを進め、`slide_anim_max_frame`(10F)を超えたらShow/Hideへ。
- `Draw`([TutorialUI.cpp:66-95](Game/UI/TutorialUI.cpp#L66-L95)): 進行度を0〜1にclamp→SlideOutなら`1-t`→ease-out(`1-(1-t)^2`)→枠の右端を「画面外(`wsize.width + boxWidth`)」と「定位置(`wsize.width * task_box_right_ratio`)」の間で補間。チェックボックス・文字は`boxLeft`基準なので一緒に動く。SlideOut中も成功の黄色の枠と✓を描く(条件を`Success || SlideOut`に)。
- つまずき(参考): フレームを0に戻し忘れて2回目以降スライドしない/引っ込むときの逆向きの値を別変数に入れて使っていなかった(未初期化の変数も残る)→同じ変数を書き換える形に。
- 任意: ease-outの行([TutorialUI.cpp:83](Game/UI/TutorialUI.cpp#L83))にコメントが無い。
- **残りのタスク(10/20時点)**: ⑤岩などが急に出る / ⑥カウンター成功のリアクション / タイピング風のメッセージ演出 / チュートリアルUIにグリッチシェーダ / 宙返り中もカメラに機体が映るように / AIテストプレイで初心者がクリアできない(ワーム・岩の被弾が多い)件の検討 / 雨の2面。軽量化したエフェクトとデバッグワープの確認は「大体OK」(ユーザー、10/19)。

### 2026-10-20 景色用の島モデルを追加(Claudeが直接実装、実機の見た目は未確認)
- **素材**: Kenney Nature Kit 2.1(CC0、`Data/Model/Scenery/License_Kenney_NatureKit.txt`)。Unity Asset Storeは再配布規約・ログインの都合で使わず、CC0にした。使用: `cliff_block_rock`→`SceneryCliff`(草の天面つき崖ブロック)、`rock_tallA`→`ScenerySpire`、`rock_largeA`→`SceneryRock`、`tree_pineGroundA`→`SceneryTree`。
- **変換**: このPCにFBX→mv1変換ツールが無く、PythonもBlenderも無いので、OBJ→DirectX `.x`(テキスト)に変換するPowerShellスクリプトを自作して`.x`のままロード(`MV1LoadModel`は`.x`を読める)。スクリプトはリポジトリ外(scratchpad)。Kenneyは右手系なのでZ座標・法線Zを反転済み。**面の向き(裏表)が逆なら見た目がおかしくなるので実機で確認が必要。**
- **仕組み**: `Scenery`(Actor継承、当たり判定なし、`MV1DrawModel`で標準描画) + `SceneryDataSetter`(CSVから生成)。`Data/CSV/Stage1/Scenery.csv`(列はRock.csvと同じ。rotY=5列目、scale=7〜9列目を使用)。生成は[GameScene.cpp:211-216](Scene/GameScene.cpp#L211-L216)で、ステージのオフセットに関係なく固定位置(チュートリアル区間)。モデルIDは[ResourceLoader.h](Manager/ResourceLoader.h)・[ResourceConstants.h](Constants/ResourceConstants.h)に`Scenery*`を追加。
- **配置の考え方**: カメラのFarが5500([CameraBase.cpp:11](Game/GameObjects/Camera/CameraBase.cpp#L11))なので、島は進路の左右|x|=2600〜5000に置いた(プレイヤーは画角内にclampされ約±1000以内、敵・岩は|x|≦1200なので干渉しない)。z=2600〜16200に4つの島+岩。島の土台は海面下(y=-100)から立ち上げ。
- **確認してほしい点**: ①島の見た目(色・向き・水との境目)、②見えるタイミング(Far=5500のため遠くの島はポップインする可能性)、③ライティングが標準描画でNovaWingの雰囲気と合うか。気に入らなければ`Scenery.csv`の数値で調整(コード修正不要)。
- **追記(同日)**: 実機で面が裏返って見えた→変換スクリプトでZ反転に加えて三角形の頂点順(b,c)も入れ替えて`.x`を作り直した(コード・ビルドの変更なし、`.x`差し替えのみ)。再確認待ち。

### 2026-10-20 景色の島: 方針変更(Kenneyをやめてゲーム内の岩を流用)
- ユーザー評価: Kenney(低ポリ・フラット)は暗くてチープ→不採用。`Data/Model/Scenery/`(Kenney由来の.xと License)は削除、`ResourceLoader`/`ResourceConstants`のScenery*追加分は`git checkout`で元に戻した。リアル系CC0はPoly Havenがあるが1個100万ポリ超でDxLibには重く、Blenderでの軽量化が必要(未実施)。
- 現状: [Scenery.csv](Data/CSV/Stage1/Scenery.csv)をRock1/2/3(大きく拡大、y=-30〜-50で海面下から立ち上げ)に書き換え済み。`Scenery`クラスは`MV1DrawModel`+自発光の標準描画のまま(ビルドは通る)。
- **ルール違反の反省**: 「3まであなたがやってみて」を.cpp/.h編集の許可と誤解して、Scenery一式・GameScene・vcxprojを直接編集した。ユーザーから「C++/HLSLは勝手に改変しない」と指摘。以降は文章での説明のみ。**未了の理想形**(ユーザーが実装するなら): `Scenery`の描画を`Rock::Draw`と同じ`DrawWithLighting`経路(法線マップ・ノイズ、OnInitでCreateShaderBuffers)にして岩と質感を揃える。
- **追記(同日)**: ユーザーから「今回は許可してるからいい」と明確化があり、`Scenery`を岩と同じ描画経路に変更(`OnInit`で`CreateShaderBuffers`、`Draw`で`UpdateShaderMatrixData`+岩の法線マップ/ノイズ+`DrawWithLighting`)。ビルドOK。実機の見た目は未確認(確認点: 岩と質感が揃うか、島の大きさ・位置・ポップイン)。調整は[Scenery.csv](Data/CSV/Stage1/Scenery.csv)の数値で可能。

### 2026-10-20 景色の島(最終): Poly Havenの実写スキャン岩(CC0)に差し替え
- ユーザー要望: 自分のRockの流用は手抜きなのでNG、外部のリアルなモデルを取ってくる。→ **Poly Haven(CC0)**の`mountainside`/`rock_face_01`/`rock_face_02`/`rock_07`/`coastal_cliff_01`を取得。(`boulder_01`は軽量化で穴だらけになったので不採用。)
- **加工**: Blender 4.5(ポータブル版、`C:\bl`に展開、リポジトリ外)でポリゴン数を1.5〜4万に削減→DirectX`.x`(テキスト)にスクリプトで出力。UV・法線・拡散テクスチャ(1k jpg)つき。幅1=1.0に正規化済みなのでCSVのscaleが「幅(ワールド単位)」。座標系はZ-up右手→Y-up左手、三角形の頂点順を入れ替え。出力先は[Data/Model/Scenery/](Data/Model/Scenery/)(合計約10MB、ライセンスはREADME_License.txt)。
- **描画**: [Scenery.cpp](Game/GameObjects/Actors/Scenery/Scenery.cpp)はDxLib標準描画+マテリアルの自発光/拡散/環境光で明るさ調整(`emissive_color`/`diffuse_color`/`ambient_color`)。岩のシェーダ経路は法線マップがモデルごとに必要になるので使っていない(ゲーム内Rockの質感とは別物)。
- **確認方法**: ゲーム外の小さなDxLibテストプログラム(scratchpad)でモデルを読み込んで画像化し、テクスチャ・面の向き・配置を確認済み(ゲーム本体での見た目は未確認)。岩壁(rock_face)は片面の板状なので、CSVのrotYで進路側を向けている(左側300前後、右側50〜60前後)。
- **配置**: [Scenery.csv](Data/CSV/Stage1/Scenery.csv)の10個(進路の左右2300〜4300、z=3000〜16000、y=-120〜-250で海面下から)。プレイヤーの移動範囲(約±1200)と重ならない。
- **ライセンス**: Unity Asset Storeは、Unity公式でも他エンジン利用は可だが「最終製品から素材を抽出できないこと(=リポジトリに生ファイルを公開しない)」「Unity経由でのみDL」が条件で、公開予定の就活リポジトリには不向きなのでCC0にした。

### 2026-10-20 景色の島(最終・その2): 岩ではなく「島」を生成
- ユーザー評価: Poly Havenのスキャン岩は「島/陸というよりクソでかい岩」→不採用。Poly Havenに島の地形モデルは無いので、**島の地形をBlender+numpyのスクリプトで生成**(ノイズで不規則な海岸線・丘・尾根、海面下に沈む斜面)し、テクスチャは**Poly Havenの砂・草・崖(CC0)**を高さと傾斜でブレンドして2048pxにベイク(上から見た平面UV)。草地の濃い部分は森のキャノピー風。形違いの3種(`SceneryIslandA/B/C`、約3.1〜5.1万ポリゴン、合計約12MB)。スクリプトはリポジトリ外(scratchpad)。
- 海面(y=0)=モデルのy=0なので、CSVのposYはほぼ0(-10)。モデルは幅1.1に正規化済みで、scaleX/Z=幅、scaleYで高さを伸ばす(Aは約1.35倍)。
- 配置: [Scenery.csv](Data/CSV/Stage1/Scenery.csv)の6島(進路の左右3000〜3800、z=3200〜15800)。テスト画像で、島としての見た目・向き・色を確認済み。ゲーム本体での見た目(水面との境目、夜の明るさ、ポップイン)は未確認。
- 旧: Kenney/Poly Havenの岩モデル、ゲーム内Rock流用はすべて削除。
- 追記: ユーザー要望で島を海面下に沈めた(Scenery.csvのposY -10 → -200)。水面越しに水中の斜面が見えるか確認待ち。さらに沈めたい/戻したい場合はposYだけ変える。
- **追記: 水面越しの透け**: 島が透けなかった原因は、水面のrevealが「キャプチャ画面(色+アルファ=カメラからの距離)」を使うのに、Sceneryがキャプチャ時も標準描画(アルファ=1)だったため。岩は`DrawWithLighting`経由でCapturePSを通るので透けていた。→ `Scenery::Draw`でキャプチャ中(`WaterRevealManager::IsCaptureMode`)だけ`UpdateShaderMatrixData`+`DrawWithLighting({})`(LightingVS+CapturePS)、通常時は標準描画のまま。`OnInit`で`CreateShaderBuffers`。.xモデルがLightingVS(接線入力あり)+CapturePSで描けることはゲーム外テストで確認済み。ゲーム本体での透け具合は未確認。

### 2026-10-20 ⑤岩などが急に出る問題: ステージの中身をカメラのfarの外に生成(Claude、ユーザー依頼、ビルドOK・ゲーム内未確認)
- 原因: チュートリアル終了時(`GameScene::StartStage`)にプレイヤーZをオフセットにして一括生成するが、一番近い岩は300先、カメラのfarは5500なので、描画範囲の内側に急に出ていた。
- 対応(A案、ユーザー判断で「CSVの値にfarを足す」): `Data/CSV/Stage1/`の`FloatingEnemy`/`Rock`/`Boss`のposZと、`WormEnemy`のposZ・`activatePlayerZ`に**すべて+5500**(far=[CameraBase.cpp:11](Game/GameObjects/Camera/CameraBase.cpp#L11)の`camera_far`)。最初の岩は5800先(地平線の外)から見えてくる。ボスは29300→34800。
- ボス出現の判定も同じだけずらした: [GameScene.h:39](Scene/GameScene.h#L39)の`boss_appear_z` 25800→**31300**(コメント付き)。デバッグワープの物差しの右端もこれを使うので自動で合う。
- 副作用: チュートリアル終了から最初の岩まで約12秒(5500÷480/秒)何もしない時間ができる。→ 「MISSION START」等の演出で埋める案(未着手・未決定)。
- **注意**: `camera_far`を変えたらCSVのずらし量も見直す。Unityの`Stage1.unity`はCSVとさらにずれた(取り込み直しは`StageCsvImporter`で)。AIテストプレイの被弾位置(Z)の記録も、これ以降は5500ずれる。
- 追記: 島を27個に増やした(進路の左右、z=2200〜25900、島同士は外周が300以上離れて重ならない、進路(x=0)からも外周が2000以上離す)。配置はスクリプトで乱数生成(リポジトリ外)。重いと感じたらScenery.csvの行を減らすだけでよい。
- 追記: 島を27個に増やした([Scenery.csv](Data/CSV/Stage1/Scenery.csv))。進路の左右、z=2200〜25900、島同士は外周が300以上離れて重ならない(計算で確認済み)、進路(x=0)からも外周が2000以上離す。大きさ・向き・高さの伸びはランダム、沈める量は島の大きさに比例(posY=-0.059×幅)。重いと感じたらCSVの行を減らすだけでよい。配置は乱数生成のスクリプト(リポジトリ外)で作った。
- 追記(谷の配置): ユーザー要望『島の間に隙間があり、厳しい谷を進んでいく感じ』。進路の両側に島を連ねる配置に変更(31島: 谷の壁17+背景14)。壁の島は進路から内側の海岸線が約1400離れ、高さ(scaleY)を幅の1.5〜1.9倍にして切り立たせた。島同士は海岸線が少し離れる程度に詰めて並べ、その外側の背景用の島で奥行きを出している。配置は乱数生成のスクリプト(リポジトリ外)。ゲーム外のテスト画像で谷の見え方を確認済み(ゲーム本体では未確認)。進路の幅(±約1000)・岩(±1200)とは重ならない。
- **追記(島のループ配置)**: ユーザー要望『海と同じで、島が途切れると違和感があるのでループさせたい』。`Scenery`にループ処理を追加([Scenery.cpp](Game/GameObjects/Actors/Scenery/Scenery.cpp)): CSVのzは「1周分(`loop_length`=24000)の中の位置」として扱い、`Update`で「カメラzの3000後ろ(`recycle_behind_distance`)より前方で一番手前になる位置」= `baseZ + ceil((camZ - 3000 - baseZ) / 24000) * 24000` に置き直す(通り過ぎた島は1周分前方へ回る)。見える範囲(Far=5500)より周期が十分長いので継ぎ目は見えない。**CSVのzは0〜24000で、端同士がつながる(周期的に重ならない)ように生成した29島(壁14+背景15)**。ループ長を変えるときはScenery.cppの`loop_length`とCSVの両方を直す(CSVは作り直し)。継ぎ目をまたぐ見え方はゲーム外のテスト画像で確認済み(ゲーム本体では未確認)。

### 2026-10-20 【タスク】プレイヤー移動に加速度をつける(戦闘機らしく、初速は遅く)
- **背景**: 今は移動が「スティックの傾き × 最高速度」を毎フレームそのまま速度に代入([MovingState.cpp:79-97](Game/GameObjects/Actors/Character/Player/Movement/MovingState.cpp#L79-L97))、離すと[IdleMovementState.cpp:31-36](Game/GameObjects/Actors/Character/Player/Movement/IdleMovementState.cpp#L31-L36)の`Enter`で速度をz以外0にする。入力した瞬間に最高速度・離した瞬間に停止で、機体の重さが出ない。実現できるとかなりリアルなゲームになりそう(ユーザー談)。
- **考え方**: 「目標速度(今と同じ計算)」と「現在速度」を分け、現在速度を毎フレーム目標へ、**1フレームで変えられる量に上限**をつけて近づける(その上限が加速度)。
- **決めること(設計メモ)**:
  1. 加速(入力中)と減速(離したとき)の値を分ける。戦闘機らしくするなら減速をやや緩やかに(離しても少し流れて止まる)。
  2. 値は「最高速度に達するまで何フレームか」から決める(目安0.2〜0.3秒 = 12〜18フレーム)。
  3. 傾き中の横移動(`tilt_move_speed_x`)は目標速度側でそのまま使える。回避用なので傾き中だけ加速度を大きくするのがおすすめ。
  4. **ステートをまたぐ**: 今はスティックを離すとIdleに移って速度を0にするので、このままだと減速が効かない。→ Idleでも現在速度を0へ近づける(`Enter`の速度リセットをやめる)方が構造がすっきり。または速度が十分小さくなるまでMovingに留まる。
  5. 「現在値を目標値へ上限つきで近づける」処理はMovingとIdleの両方で使うので、`Player`に1つだけ関数として置いて重複を避ける。
  6. 他のステート(ブースト/ブレーキ/ショット/宙返り)は速度を直接セットしている。Movingに戻ったときは「そのときの速度から加速をやり直す」で十分。
  7. 画面端(フラスタムでのクランプ、[Player.cpp:294-298](Game/GameObjects/Actors/Character/Player/Player.cpp#L294-L298))に当たったら、その方向の速度を0に戻す(壁にこすりながら加速し続けるのを防ぐ)。
- **進め方(案)**: ①Movingに加速だけ入れる → ②Idleに減速を入れる(`Enter`のリセットをやめる) → ③加速度の値を感触で調整 → ④傾き中だけ加速度を大きくする。
- **関連(カメラ)**: カメラの`lerp_t`を0.03に下げた影響で、実際のカメラ距離が伸びる(前進速度に比例: 通常時+約134、ブースト時+約277)。`camera_offset_z`を300→200前後に下げて通常時の見え方を戻す案を検討中。ブースト時は離れる/ブレーキ時は近づく(演出として残すか要検討)。加速度を入れると操作感が変わるので、カメラ調整は加速度の後でまとめて見直すのがよい。
- **関連(移動速度)**: カメラを近づけた結果、左右上下の移動が速く見える問題あり。`MovingState.cpp`の`move_speed_x/y`(19)・`tilt_move_speed_x`(25)をカメラ距離の比率(約0.62〜0.75倍)で下げる案。加速度とセットで速度の値を決めるとよい。

### 2026-10-20 ボス戦で召喚された浮遊敵の弾が当たらない問題(ユーザー実装、確認OK)
- 症状: ボス戦の召喚された浮遊敵の弾が、プレイヤーが止まっていても当たらず、頭の上や横を抜けて後ろへ行く(通常のステージでは止まっていれば当たる)。召喚位置を近づけたり正面にしたりすると当たるが、ボスと重なる・見づらいので不採用。
- 原因: 弾は「撃った瞬間のプレイヤーの位置へ速さ7」で撃つだけで、プレイヤーの前進(z=8/フレーム)を考えていなかった。プレイヤーから見ると弾の速度は「7×方向 − (0,0,8)」になり、奥行きだけ速く詰まって、上下左右のずれを詰め切る前にプレイヤーの奥行きを通り過ぎる(ずれの約半分外れる。当たる範囲はプレイヤーの球50＋弾32＝82)。通常のステージで当たってボス戦で外れたのは、位置関係(ずれの大きさ)の差でこの範囲に収まるかどうかと推測(未測定)。
- 対応([ActiveState.cpp:88-90](Game/GameObjects/Actors/Character/Enemy/FloatingEnemy/ActiveState.cpp#L88-L90)): 弾の速度に**プレイヤーの速度のzだけ**足す(`FloatingEnemy::GetPlayerVel()`を追加)。プレイヤーから見ると弾が撃った方向そのままにまっすぐ来るので、**止まっていれば必ず当たり、上下左右に動けば外れる**。上下左右まで足すと避けても追ってくるので足さない。撃った後にブースト/ブレーキで前進の速さが変わると外れる(ブーストで避けられる、で問題なし)。
  - 例(2000前方・300上): 前はプレイヤーから見て「下1.0:手前14.9」→300のうち134しか下がらず外れる。今は「下2.2:手前14.8」で撃った方向と同じ→300下がって当たる。
- 弾が届く前に消える問題: 足したことでプレイヤーから見た近づく速さが7になり、距離2000で約285F(寿命300F=5秒、[BulletBase.cpp:10](Game/GameObjects/Bullet/BulletBase.cpp#L10))。**`bullet_speed`を7→15**([ActiveState.cpp:23](Game/GameObjects/Actors/Character/Enemy/FloatingEnemy/ActiveState.cpp#L23))にして、近づく速さを前と同じ約15に戻した。
- 未対応: ワームの弾([WormEnemy.cpp:185](Game/GameObjects/Actors/Character/Enemy/WormEnemy/WormEnemy.cpp#L185)、`bullet_speed`=8)は同じ撃ち方のまま。必要なら同じく`m_pPlayer.lock()->GetVel().z`を足して速さを見直す。召喚ワームを「召喚の瞬間のプレイヤーの横・高さ」に出す案もあり(未決定)。
- 気づき(任意): ease-outのコメントが補間の行([TutorialUI.cpp:93](Game/UI/TutorialUI.cpp#L93))に付いている。式は[83行目](Game/UI/TutorialUI.cpp#L83)。

### 設計決定(2026-10-21・カウンター成功時のリアクション: スロー＋カメラの揺れ＋白いフラッシュ)
- **方針(ユーザー)**: スローはカウンター専用にしない(他の場面でも使う)。**GameSceneは各オブジェクトの初期化・更新・描画を呼ぶだけ**にして、「回数を毎フレーム見比べて反応する」ような処理は入れない。先輩(ProjectNeaR)の作りを参考にする。
- 先輩の作り: `Application`がタイムスケール(`SetTimeScale`/`GetTimeScale`)とポストエフェクト(`ShaderPostProcess`、グリッチ等の状態をどこからでも`AddPostEffectState`)を持つ。ヒットストップは当たりを知っている`AttackManager`が自分で起こし、カメラは`GameScene::Init`で`SetPlayerCamera`として渡すだけ。物体ごとに速度・カウンターへタイムスケールを掛けている。
- NovaWingでの形(ユーザー了承):
  - **タイムスケール**: `Application`に`m_timeScale`/`m_slowFrame`、`StartSlow(scale, frame)`(重なったら残りの長い方)、`UpdateTimeScale()`(毎フレーム、0で1.0に戻す)、`GetTimeScale()`、(予定)`ResetTimeScale()`。効かせ方は先輩と違い**GameSceneで「進み具合を貯めて1以上でゲームの世界を1回更新」**(`m_timeProgress`、`UpdateGameWorld()`に`UpdateAll`〜`WaterManager`を切り出し)。物体ごとにタイムスケールを掛ける先輩方式は、移動約30か所・フレームカウンター約45か所(18ファイル)・エフェクトの書き換えが要るので見送り。呼ぶ側(`StartSlow`)は先輩と同じなので、後で移行しても呼び出し側は変わらない。**(ユーザー実装済み)**
  - **反応を起こす場所**: 当たり判定を管理する`CollisionManager`のカウンター成功の分岐([CollisionManager.cpp:259-271](Manager/CollisionManager.cpp#L259-L271))。`StartSlow`、カメラの揺れ(`CollisionManager`は元から`m_pCamera`=GameCameraを持つ→`OnShake`)、フラッシュを呼ぶ。
  - **フラッシュはUIではなく画面効果**: `ScreenEffect`(仮)を`Application`が持ち、`Flash(frame, alpha)`。メインループの`controller.Draw()`の後([Application.cpp:93](Main/Application.cpp#L93))で`Update`/`Draw`(白い四角を半透明で重ねるだけ、UIの上にも掛かる)。将来グリッチ(オフスクリーン→シェーダ)もここに足す。
  - `GameScene::Init`ではスローと画面効果のリセットを呼ぶだけ。

### 日程の変更(2026-10-22・ユーザー)
- **試作(プロト)の締め切りを10月いっぱい(10/31)に延長**(元は10/24)。カレンダーも更新済み: プロト9/18〜10/31、アルファ11/1〜11/24、ベータ11/25〜12/23、マスター12/24〜12/30、提出12/31、ポートフォリオ1月。
- **運用ルール(10/22ユーザー依頼)**: 「ノートを見て」と言われたら、NOTES.mdに加えて`NovaWing_詳細.xlsx`の**コスト表**と**カレンダー**も確認して報告する。
- **運用ルール(10/22ユーザー依頼)**: 「タスクとして残して」と言われたら、**コスト表に行を追加する**。見積コスト・優先度・バージョンはユーザーが提示するので、追加前に聞く。
- コスト表(10/22時点): プロト 全66h/残42h、アルファ 30.5/27.5、ベータ 36.5/36.5、合計133/残106。1日に必要1.28h、現在の平均0.79h/日。
- **(10/22)コスト表をNOTESに合わせて更新(Claude、ユーザー依頼。バックアップはscratchpad)**: 
  - 完了にした既存行: 横回転反射(親、G5)、シーン＞チュートリアルシーン(実20h)、ステージ＞配置(1面、実8h)、テキスト(ヒント)＞システム部分(実4h)。
  - 追加した完了行(B83〜B94、バージョン=プロト、見積=実、**コストはClaudeの目安**で備考に「目安(Claude)」): 傾き加速1/ICollider化・カウンター判定6/ボス登場演出4/エフェクト作り直し・軽量化6/敵ステータスのCSV化5/チュートリアル後のステージ開始・岩の出現対策3/景色の島2/カメラの端の遠近感2/システム整備5/その他バグ修正6/開発環境3/AIテストプレイ1。
  - 「全コスト」(K8〜K10)の式を固定範囲`C$4:C$82`から`SUMIF(テーブル1[バージョン],J,テーブル1[見積コスト])`に変更(行を足しても集計に入るように)。
  - 更新後: プロト 全110/残42、アルファ 30.5/20.5、ベータ 36.5/27.5、合計177/残90。1日に必要1.70h、平均2.56h/日(過去分を一度に計上したため)。
  - **(追加済み)未完了タスク(B95〜B103)**: カウンターのリアクション(スロー・揺れ 2h/A/プロト、白いフラッシュ 2h/B/ベータ)、タイピング風メッセージ 3h/B/ベータ、チュートリアルUIのグリッチ 1h/B/ベータ、宙返り中のカメラ 0.5h/C/アルファ、移動の加速度 2h/B/プロト、チュートリアル後の空白の演出 2h/C/ベータ、2面の配置 5h/S/アルファ、AI初心者がクリアできない件の調整 0h(不明)/B/プロト。コスト・優先度はユーザー提示、バージョンはClaudeの案(定義に沿って)。追加後: プロト 114/残46、アルファ 36/26、ベータ 44.5/35.5、合計194.5/残107.5、必要1.87h/日。
  - **(決定・反映済み)シェーダ5つ(海のリアル化・集中線・水滴・モーションブラー・被写界深度、計30h)と親の「シェーダ」をプロト→アルファに変更**(ユーザー了承、集中線もアルファ)。反映後: **プロト 84/残16**(当たり判定1・新しい敵3種11・カウンターのスロー/揺れ2・加速度2・AI初心者の件0)、アルファ 66/56、ベータ 44.5/35.5、合計194.5/残107.5。
  - **(作り直し・ユーザー指摘)** 上の追加行は、元の表の書き方(親カテゴリの下に「　ー」の子項目)に作り直した。**コスト表にはClaude/AI/目安・日付・Claudeにさせた作業の説明を書かない**、開発環境とデバッグ専用ツール(AIテストプレイ)の行は削除。「AIテストプレイで初心者が〜」→ステージ＞「難易度調整(初心者がクリアできない)」。「配置」→「1面の配置」。新しい親カテゴリ「カメラ」「システム」を末尾に追加。プレイヤー・横回転反射の親は未完了の子があるので「未完了」に戻した。追加した完了の行は優先度が空欄。最終: プロト 80/残16、アルファ 66/56、ベータ 44.5/35.5、合計190.5/残107.5。
- **(10/22ユーザー決定)新しい敵(蝶・固定砲台・突進)など、コスト表にある新しい要素は全部2面用。** 1面には出さない。
- **(10/22タスク追加・コスト表 敵全体＞「個体ごとに動きと攻撃のタイミングをずらす」1h/A/アルファ)**: 敵が同じ瞬間に撃つ・同時に動き出す・全員同じ動きなのが変。案: 生成時に個体ごとの乱数(撃つ間隔の開始のずれ、揺れの周期のずれ等)を持たせる。
- (10/22)投射物の基底クラス(`ProjectileBase`仮)を`BulletBase`の上に作る案が出たが、**ユーザー判断で取りやめ**。参考: 今のビームは`BossBeamState`が位置を持ちコライダーはボスのもの(オブジェクトではない)、動きも「伸びる」。次のボスで投射物を作るときに改めて検討。
- **(10/22完了・ユーザー確認「いい感じ」)カウンター成功時のスロー・カメラの揺れ**: `GameScene`の間引き更新をやめて毎フレーム更新に戻し、移動にタイムスケールを掛ける方式に変更。掛けている場所: `Character::Update`(プレイヤーは`IsAffectedByTimeScale()`=falseで除外)、`BulletBase::Update`、ワームの前進、ボスのビームの先端([BossBeamState.cpp:140-141・245-252](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossBeamState.cpp#L245-L252))。カメラは毎フレーム更新なので揺れ・追従は通常速度。`Application::ResetTimeScale()`を`GameScene::Init`で呼ぶ。スロー0.3倍・30F、揺れ5・12F。コスト表: 見積2h/実3hで完了、親「横回転反射」も完了に(プロト残14h)。**掛かっていないもの**(必要なら): フレームカウンター(撃つ間隔・状態の時間)、弾の寿命、アニメーション、エフェクトの再生、ワームの螺旋の回転、海の時間。
- **(10/22)2面の制作に着手(ユーザー決定)。** 当たり判定と岩(ノーマル/チャージ)はいったん放置→優先度Bに変更済み。コスト表に追加: ゲームループ＞「ステージの切り替え(1面クリア→2面)」5h/S/プロト、ステージ＞「2面の建物をUnityで配置して読み込む」3h/S/プロト(建物はUnityで置いてCSV経由で読み込む、ユーザー方針)。進め方: ①ステージ切り替えの土台(`StageConfig`、`GameScene`が設定でCSVフォルダ・海の有無・チュートリアルの有無等を切り替え、クリア後に次のステージで作り直す) ②仮の2面(Unityで建物を配置) ③新しい敵3種(突進→固定砲台→蝶) ④2面の配置 ⑤見た目(アルファ) ⑥音(ベータ)。
  - **次回(返事待ち)**: 1面のボスを倒したあと2面へどう進むか。A案(Claude推奨): クリア画面(リザルト)を見せてから選択肢に「次のステージへ」を足す([ClearScene.cpp:332-350](Scene/ClearScene.cpp#L332-L350)の`ClearSelect`に1つ追加)。B案: クリア画面を出さずにそのまま2面へ。決まったら`StageConfig`の設計(CSVフォルダ名・海の有無・チュートリアルの有無・次のステージ、`GameScene`のコンストラクタで受け取り、リトライ4か所は同じステージを渡す)に進む。
  - 今のコードで「Stage1」固定の場所: 各DataSetterのCSV名(`FloatingEnemy`/`WormEnemy`/`Rock`/`Boss`/`Scenery`)、`TutorialController`の`Tutorial`/`TutorialEnemy`。`GameScene`を作っている所: `TitleScene.cpp:287`・`PauseScene.cpp:183`・`GameoverScene.cpp:139`・`ClearScene.cpp:338`(リトライ)。`ClearScene`の選択肢は`ReTry`/`BackTitle`の2つ。
- **(10/22)ボスのビームの跳ね返しでもスロー・揺れ(敵弾と同じ強さ・長さ)**: `CollisionManager`にスローと揺れをまとめた関数(例`StartCounterReaction()`)を作り、`OnHit`で「Counter×BossBeamTip」でも呼ぶ。**ここでは`return`しない**(跳ね返しは最後の`OnCollision`→`BossBeamTipCollider::OnCollision`で起きるため)。跳ね返った先端はタグが`BossBeamReflect`になり、Counterとの組み合わせは判定対象外なので、左右で1回ずつしか起きない。
- **(10/22・カウンターのスローが処理落ちに見える)** 原因: `GameScene::Update`で進み具合を貯めて世界を間引き更新しているため(0.3なら3〜4フレームに1回、間は止まる)。プレイヤーだけ通常速度にもできない。案(返事待ち): 毎フレーム更新に戻し、移動の2か所([Character.cpp:28](Game/GameObjects/Actors/Character/Character.cpp#L28)・[BulletBase.cpp:44](Game/GameObjects/Bullet/BulletBase.cpp#L44)の`m_pos += m_velocity`)で速度にタイムスケールを掛ける。プレイヤーは`Character`の仮想関数(例`IsAffectedByTimeScale`)で除外。フレームカウンター・アニメーション・エフェクトは通常速度のまま(必要ならアニメの`Update`の速さ、`GetEffekseer3DManager()->Update(量)`)。
- **バージョンの定義(10/22ユーザー)**: プロト=ゲームの面白さが分かるようにする段階 / アルファ=見た目(モデルなど) / ベータ=エフェクト・チュートリアルなど、なくてもいい要素。進行不能バグは適宜。細かいバグは本来マスターだが、今回マスターは設けないのでベータ以降。

### デバッグ用ウィンドウ(Dear ImGui)の導入(2026-10-22・デバッグ機能は全部Claudeが実装する、とユーザーが依頼)
- **方針(ユーザー)**: デバッグ機能(ImGui、ステージ/チュートリアルのスキップ、無敵、時間操作、情報パネルなど)は**Claudeが実装し、進め方もお任せ**。数値を画面に出すと邪魔なので、ImGuiに集約する。コスト表には残さない。
- **導入済み(Debug|x64でビルド成功、起動確認済み。FPSとデバッグ表示の切り替えが日本語で表示される)**:
  - ImGui v1.91.9を[DebugTools/ImGui/](DebugTools/ImGui/)に置いた(本体+dx11/win32バックエンド)。Releaseのビルドからは除外(vcxprojの`ExcludedFromBuild`)。ウィンドウ位置の保存`imgui.ini`は.gitignore。
  - [DebugUI.h](DebugTools/DebugUI/DebugUI.h)/[DebugUI.cpp](DebugTools/DebugUI/DebugUI.cpp): シングルトン。**ImGuiの型はcppの中に閉じ込めた**。項目は`AddButton`/`AddCheckbox`/`AddText`/`AddSliderAction`で**登録式**(`owner`=thisを渡し、破棄時に`RemoveOwner(this)`で解除)。ボタンの処理はUIを組み立てたあとに実行(処理中に登録が解除されても壊れない)。文字は`std::wstring`で受けて内部でUTF-8に変換(このプロジェクトの`L""`に合わせた)。F1キーで表示/非表示。
  - 組み込み: [Application.cpp](Main/Application.cpp)で`Init`/`Terminate`、メインループは`BeginFrame()`(入力取得の前)と`Render()`(`ScreenFlip`の前)。FPS表示はImGuiへ移したので`DrawFormatString`のFPSは削除。DxLibのウィンドウメッセージは`SetHookWinProc`でImGuiにも渡す(戻り値は使わない)。描画は`RenderVertex`→バックバッファに描画→`RefreshDxLibDirect3DSetting`。
  - [InputManager.cpp](Manager/InputManager.cpp): ImGuiで文字入力中はゲーム側のキー入力を空にする。[DebugManager.h](Manager/DebugManager.h)に`SetDebugDrawEnabled`を追加。
  - **注意**: 新規ファイルは**BOM付きUTF-8**にする(BOMなしだとMSVCがCP932として読み、日本語コメントの末尾で次の行がコメント扱いになりエラーになる)。
- **次にやること(Claude、`GameScene`/`PauseScene`を触るので、ユーザーの`StageConfig`作業と衝突しないよう後回し)**: ①ポーズ画面のデバッグ項目(デバッグ表示切り替え・リスタート・プレイヤーを移動の物差し)をImGuiへ移して`PauseScene`から`#ifdef _DEBUG`部分を外す(ワープは`AddSliderAction`、`GameScene::Init`で登録/デストラクタで解除) ②ステージ選択・チュートリアルの指定段階へスキップ ③無敵・HP/ゲージ全回復 ④ボスHPを1・即撃破(今は`K`キー) ⑤時間操作(一時停止・コマ送り・スロー倍率) ⑥情報パネル(オブジェクト数、プレイヤー座標・速度、シーン状態)。
- **(10/22・ユーザー指摘)ImGuiのウィンドウをゲームのウィンドウの外へ出すと見えなくなる → 対応済み**: ImGuiを**docking版(v1.91.9-docking)に差し替え**、`ImGuiConfigFlags_ViewportsEnable`を有効化([DebugUI.cpp](DebugTools/DebugUI/DebugUI.cpp))。ゲームの外へドラッグするとOSの別ウィンドウになる(実機で確認)。`Render()`の最後に`UpdatePlatformWindows`/`RenderPlatformWindowsDefault`。別ウィンドウを操作してもゲームが止まらないよう`SetAlwaysRunFlag(TRUE)`(デバッグ時のみ。ゲームの窓が非アクティブでも動き続ける)。
- **(10/22・ユーザー依頼「どんどんこれに移して」)ポーズ画面のデバッグ項目などをImGuiへ移行済み(実機で表示確認)**:
  - [PauseScene.h](Scene/PauseScene.h)/[PauseScene.cpp](Scene/PauseScene.cpp)から`#ifdef _DEBUG`部分(デバッグ表示切替・最初からやり直す・プレイヤーを移動の物差し)を全て削除。コンストラクタの`warpPlayer`引数もなくなった(`GameScene`は2引数で作る)。
  - [GameScene.cpp](Scene/GameScene.cpp)の`RegisterDebugItems()`(`Init`の最後で呼び、デストラクタで`RemoveOwner(this)`)で登録: **Stage**=最初からやり直す/チュートリアルをスキップ/ステージ上のZ座標スライダー+「プレイヤーをその位置へ移動」(`WarpPlayer`)/プレイヤーZ表示、**Player**=HP表示・HP全回復・HPスライダー、**Boss**=HP表示・HPを1にする・ボスを倒す(`TakeDamage(最大HP)`で通常の死亡の流れ)。**System**(`DebugUI`組み込み)=FPS・デバッグ表示・**プレイヤー無敵**(`DebugManager::SetPlayerInvincible`、`Player::TakeDamage`の先頭で参照)。
  - キー操作のデバッグを廃止してImGuiへ: 上下キーのHP増減(`gaugeUp/gaugeDown`)、Kキーのボス即死(`killBoss`)。`InputEvent`と`InputManager`の対応表、`Player.cpp`/`BossEnemy.cpp`の処理を削除。`Character::DebugSetHealth`(`_DEBUG`のみ)を追加。残してあるもの: Q/Eのスキャンライン周期(`PlayerHPGaugeUI`、シェーダへの反映はコメントアウト済みで実質未使用)、Tキー(`InputEvent::Tutorial`、使っている所なし)。
  - 確認時の注意: ユーザーが`StageConfig.cpp`を編集中でビルドが通らなかったため、`StageConfig`を除いたコピーをscratchpadでビルドして確認した(本体のx64/Debugのexeには反映されていない。ユーザーのビルドで反映される)。
  - 次: ステージ選択(`StageConfig`が`GameScene`につながってから)、時間操作(一時停止・コマ送り・スロー倍率)、情報パネル(オブジェクト数など)。
- **(10/22・ユーザー「ImGuiがあるなら画面内のデバッグ描画はいらない」)画面内の文字のデバッグ表示を全て削除**(当たり判定の球とグリッドなど空間に出る描画は残した。「デバッグ表示」チェックボックスはこれらの切り替え)。削除: `GameScene`(GameScene/FRAME)、`Player`(座標・ゲージ・Health・IsFocus・InvincibleTime)、`TitlePlayer`(playerPosZ)、`MovingState`(stick)、`FloatingEnemy`(位置・Forward)、`ActiveState`(velY)、`BossEnemy`(BossHP)、`WaterManager`(frame)、`PlayerHPGaugeUI`(scanlineFrequency)。使わなくなった`DebugManager.h`/`InputManager.h`のincludeも整理。
  - **Q/Eキー(スキャンライン周期)とTキーのデバッグ入力も削除**(画面に値を出すだけで実質未使用だった)。`PlayerHPGaugeUI`の`Update`オーバーライドと`m_scanlineFrequency`もなくなった。`InputEvent`のデバッグ用の列挙子はゼロ。
  - 数値はImGuiのPlayerセクションに移した: 座標(x,y,z)・HP・ゲージ・被弾後の無敵。Stageにプレイヤーz、Bossにボスhp。リポジトリ本体のフルビルド成功、起動確認済み。

### 2026-10-22 ステージの切り替え(1面クリア→2面)の設計と進捗(ユーザー実装中)
- **決定(ユーザー)**: 1面のボスを倒したら**クリア画面(リザルト)を挟み**、選択肢に「次のステージへ」を足して2面へ。**ステージ選択画面は理想だが余裕がないので今回はなし**(後で作るときは、選んだ番号で`GameScene`を作るだけで済む形にしておく)。
- **設計**: 「何面を遊ぶか」(ステージ番号、**0始まり=1面が0**)を`GameScene`に渡す。ステージごとの設定は`StageConfig`にまとめる。
  1. `StageConfig`(**完成**): [StageConfig.h](Constants/StageConfig.h)/[StageConfig.cpp](Constants/StageConfig.cpp)(雛形はClaudeが`New-GameClass.ps1`で作成・vcxproj登録、中身はユーザー)。`StageConfig(int stageNo)`で`.cpp`の匿名名前空間の`constexpr StageData stage_datas[]`(`const wchar_t* csvFolderName`/`bool hasTutorial`/`float bossAppearZ`)から1行をメンバへコピー、範囲外は`assert`。`GetCSVFolderName()`(`const std::wstring&`)/`HasTutorial()`/`GetBossAppearZ()`/`IsLastStage()`(`m_stageNo == std::size(stage_datas) - 1`)。今の値: 1面`L"Stage1"`・チュートリアルあり・31300、2面`L"Stage2"`・なし・30000(仮)。
     - つまずき(参考): `struct`のメンバを`std::wstring`にすると`constexpr`の配列に入れられない(「式には定数値が必要です」)→`const wchar_t*`。`IsLastStage`を`== std::size(...)`(要素数)と比べていつもfalse→`- 1`。`std::size`には`<iterator>`、`assert`には`<cassert>`。
  2. **(次)`GameScene`にステージ番号を渡す**: `StageConfig`に`GetStageNo()`を追加 → `GameScene(SceneController&, int stageNo)`([GameScene.h:28](Scene/GameScene.h#L28)、[GameScene.cpp:96](Scene/GameScene.cpp#L96))、メンバ`StageConfig m_stageConfig`を初期化子リストで作る → `boss_appear_z`([GameScene.h:39](Scene/GameScene.h#L39)の`static constexpr`)を消して`m_stageConfig.GetBossAppearZ()`に([GameScene.cpp:425](Scene/GameScene.cpp#L425)のボス出現判定、[341行目](Scene/GameScene.cpp#L341)のImGuiのワープのスライダー) → AIテストプレイ([PlaytestRecorder.cpp:255](DebugTools/AutoPlay/PlaytestRecorder.cpp#L255)・[391行目](DebugTools/AutoPlay/PlaytestRecorder.cpp#L391))も`GameScene::boss_appear_z`を使っているので`StageConfig(0).GetBossAppearZ()`等に → ポーズ([GameScene.cpp:573](Scene/GameScene.cpp#L573))・ゲームオーバー([563](Scene/GameScene.cpp#L563))・クリア([540](Scene/GameScene.cpp#L540))にステージ番号を渡し、各シーンのリトライ([PauseScene.cpp](Scene/PauseScene.cpp)・[GameoverScene.cpp:139](Scene/GameoverScene.cpp#L139)・[ClearScene.cpp:338](Scene/ClearScene.cpp#L338))で同じ番号の`GameScene`を作る → タイトル([TitleScene.cpp:287](Scene/TitleScene.cpp#L287))は定数(例`first_stage_no = 0`)で1面。
  - **(10/23完了・ユーザー確認OK)2のうち**: `StageConfig::GetStageNo()`追加、`GameScene(SceneController&, int stageNo)`+`std::unique_ptr<StageConfig> m_pStageConfig`(前方宣言、コンストラクタの`{}`で`make_unique`)、`GameScene`を作る4か所にステージ番号(タイトル=`first_stage_no`(0)、ImGuiのやり直し=今のステージ、ゲームオーバー=`GameoverScene`のコンストラクタに`stageNo`→`m_stageNo`、クリア=`ClearResultData::stageNo`)。`PauseScene`はもう`GameScene`を作っていない(リスタートはImGuiへ移ったため)。**残り: `boss_appear_z`を`StageConfig`に置き換え**。→**(10/23完了)** `GameScene::boss_appear_z`を削除し、ボスの出現判定([GameScene.cpp:429](Scene/GameScene.cpp#L429))とImGuiのワープの物差し([345行目](Scene/GameScene.cpp#L345))を`m_pStageConfig->GetBossAppearZ()`に(ユーザー)。AIテストプレイ([PlaytestRecorder.cpp:27-36](DebugTools/AutoPlay/PlaytestRecorder.cpp#L27-L36))は`StageConfig(autoplay_stage_no=0).GetBossAppearZ()`を返す`GetBossAppearZ()`に置き換え、`GameScene.h`のincludeを`StageConfig.h`に(Claude)。Debug x64ビルド成功(エラー・警告なし)。
  - **(10/23完了)3. CSVのフォルダをステージで切り替え**: 浮遊敵・ワーム(DataSetter側)・岩・浮遊敵の呼び出しはユーザー、残り(ワームの読み込みが`worm_csv_name`のままだったのを`csvPath`に、ワーム/岩の呼び出し、ボス・景色の島のDataSetterと呼び出し、`TutorialController`のコンストラクタに`csvFolderName`)はユーザー依頼でClaudeが実装。どれも「定数はファイル名だけ→`csvFolderName + L"/" + ファイル名`→`LoadCSV(csvPath.c_str())`」、呼び出し側は`m_pStageConfig->GetCSVFolderName()`。Debug x64ビルド成功。`L"Stage1/..."`の決め打ちはゼロ。
  - **`Data/CSV/Stage2/`を作成(Claude)**: `FloatingEnemy`/`WormEnemy`/`Rock`/`Scenery`はヘッダだけ、`Boss.csv`は**仮に1面のをコピー**(ボスがいないと`m_pBoss`がnullで落ちるため)。2面の`bossAppearZ`(30000仮)とボスのZ(34800)の関係は1面(31300/34800、差3500)とずれているので、2面のボスを決めるときに合わせる。`Tutorial.csv`は無い(4で2面はチュートリアルを作らない)。
  - **(10/23作業中)4. チュートリアルは`HasTutorial()`の面だけ(ユーザー実装中)**: 済み=作るのを`HasTutorial()`のときだけに([GameScene.cpp:236-239](Scene/GameScene.cpp#L236-L239))、`IsTutorialFinished()`(「`m_pTutorialController`がない、または`IsFinished()`」、[GameScene.cpp:695](Scene/GameScene.cpp#L695))を追加。**残り=使っている所のnull対策**: [321-323行目](Scene/GameScene.cpp#L321-L323)の`WarpPlayer`のスキップ(`!IsTutorialFinished()`のときだけ`Skip()`)、[348行目](Scene/GameScene.cpp#L348)のImGuiの「チュートリアルをスキップ」(あるときだけ)、[420行目](Scene/GameScene.cpp#L420)のステージ開始の判定(`IsTutorialFinished()`に)、[588行目](Scene/GameScene.cpp#L588)の`Update`(あるときだけ)。**(10/24完了・ユーザー確認OK)残りの4か所のnull対策も済み(WarpPlayerとステージ開始の判定は`IsTutorialFinished()`、ImGuiのスキップボタンと`Update`はチュートリアルがあるときだけ)。2面から始めても落ちない。**Debug x64ビルドは通る。確認は一時的に[TitleScene.cpp:80](Scene/TitleScene.cpp#L80)の`first_stage_no`を1にして2面から始め、確認後0に戻す。
  - その次: 5. クリア画面に「次のステージへ」→ 2面の海をなくす(`StageConfig`に「海を使うか」)→ 2面の建物をUnityで配置して読み込む。
  3. CSVのフォルダをステージで切り替え: `L"Stage1/..."`決め打ちの7か所(各DataSetterの`FloatingEnemy`/`WormEnemy`/`Rock`/`Boss`/`Scenery`、`TutorialController`の`Tutorial`/`TutorialEnemy`)をフォルダ名を引数で受け取る形に。
  4. チュートリアルは`HasTutorial()`の面だけ。ない面は作らずに最初からステージを始める。
  5. クリア画面の選択肢([ClearScene.cpp:332-350](Scene/ClearScene.cpp#L332-L350)の`ClearSelect`)に「次のステージへ」を足し、`IsLastStage()`なら出さない。
  - 1〜4の後、デバッグでタイトルから2面を渡せば2面のCSV読み込みを先に確認できる(`Data/CSV/Stage2/`はヘッダだけのCSVでも読める)。
- 注意(10/22時点の作業ツリー): 別セッションのImGuiデバッグUIの変更(`PauseScene`のデバッグ項目の移行、`GameScene::RegisterDebugItems`など)が未コミットで混ざっている。ImGui側の「ステージ選択」は`StageConfig`が`GameScene`につながってから。
- **(10/24)5. クリア画面に「次のステージへ」(作業中)**: 選択肢は画像(768×512)。**画像はClaudeが作成**: [Next_Stage.png](Data/Image/SelectFrame/Next_Stage.png)/[Next_Stage_OnCursor.png](Data/Image/SelectFrame/Next_Stage_OnCursor.png)(番号は「03」)。作り方: `Retry`の2枚から文字と番号の部分だけを消し(文字の無い列をコピー)、元の文字に合わせて測ったフォント(**BIZ UDPGothic**、通常=標準54px＋光彩(43,190,133)、カーソル=標準54px黒＋薄い影、番号19px)で描き直した。**(10/24修正)最初は通常の方を太字53pxにしていてゲーム内で字が違って見えた→インストール済みの日本語フォント290種を「タイトルに戻る」「リトライ」の両方と比べ直し、通常もBIZ UDPGothicの標準(太字ではない)54pxが一致すると分かったので作り直した。**生成スクリプトはscratchpad(リポジトリ外)。
  - **(ユーザー決定)描画は選択肢ごとの書き分けをやめて、表(画像2枚＋位置)を`for`で回す形にする**([ClearScene.cpp:455-540](Scene/ClearScene.cpp#L455-L540)の`switch`の書き分け)。最後のステージでは「次のステージへ」を出さない。
  - 登録が必要(ユーザー): `ResourceConstants.h`に2つのパス([ResourceConstants.h:76-80](Constants/ResourceConstants.h#L76-L80)付近)、`ResourceLoader.h`の`GraphicID`([ResourceLoader.h:81-83](Manager/ResourceLoader.h#L81-L83)付近)、`ResourceLoader.cpp`の対応表([73-75行目](Manager/ResourceLoader.cpp#L73-L75)付近)と`clear.graphics`([488-489行目](Manager/ResourceLoader.cpp#L488-L489))。
- **(10/26完了・ユーザー確認OK)5. クリア画面「次のステージへ」**(コミット`647d78b`): `ClearSelect::NextStage`、選択肢を`select_datas[]`(通常/カーソルの`GraphicID`)＋`DrawSelect`の`for`で描画、位置は「中心(0.53)−(数−1)×間隔×0.5＋i×間隔」で計算、`GetSelectCount()`(最後のステージなら1つ減らす)、決定で`GameScene(stageNo + 1)`。※`select_spacing`は0.2のまま(3つだと背景の枠から下にはみ出す件は0.15に詰める案、未確認)。
- **(10/26完了・ユーザー確認OK)2面の海をなくす**: `StageConfig`に`hasSea`/`HasSea()`(1面true・2面false)。`GameScene::Init`で海があるときだけ`WaterManager`と海底の`Stage`を作り、`Update`/`Draw`はあるときだけ呼ぶ。海の中を透かすキャプチャ(`WaterRevealManager`の`BeginCapture`〜`EndCapture`)も海があるときだけ(2面は描画がほぼ半分に)。`Player`のコンストラクタに`isSeaStage`を渡し、羽の水しぶき(`UpdateWingSplash`)は海があるときだけ。高さの下限(`sea_player_margin`)・海面付近の傾き制限・ボスの立つ高さ(`water_y`=0)は地面として流用。
  - つまずき: `StageConfig.h`にだけ足して`.cpp`(`StageData`・`stage_datas`・コンストラクタのコピー)を忘れ、1面からも海が消えるところだった。
- **次**: 2面の建物をUnityで配置して読み込む(コスト表 3h/S/プロト)。建物のモデルはまだ無い(`Data/Model/`にビル無し)。
  - (10/27確認)`Data/Model/Building/`にビルのモデル5つ(`Building_BridgeWreck`/`Collapsed`/`Leaning`/`OfficeGutted`/`TallBroken`、.mv1+.fbm)が未コミットで置かれている。
- **(10/27)プロトの絞り込み(ユーザー決定)**: 締め切り10/31まで5日。**蝶みたいな敵(7h)をアルファへ**(コスト表で親と子4行を変更済み)。プロトでやる: 2面の建物3h→突進する敵2h→固定砲台の敵2h。Bの3つ(移動の加速度・岩の当たり判定・難易度調整)はプロトのまま。コスト表: プロト 81/残10、アルファ 74/64、合計199.5/残109.5。
  - 次に決めること: ビルに当たり判定を付けるか(A: 障害物、`Rock`の仕組みを流用・縦長なので球を重ねる / B: 景色のみ、`Scenery`と同じ)。Claude推奨はA(遠景はBで併用可)。
  - **(10/27決定)ビルは当たり判定あり(障害物)。当たり判定はUnityのProBuilderで簡単なメッシュを作りFBXで書き出す(ユーザー案)。メッシュは「ビル1種類ごと」(A案)**: 6種類(`AntennaTower`/`BridgeWreck`/`Collapsed`/`Leaning`/`OfficeGutted`/`TallBroken`)それぞれに当たり判定用FBX(例`Building_Leaning_Col.fbx`)。ゲームでは配置CSVの位置・回転・大きさで見た目と当たり判定の両方のモデルを置く(配置を変えても書き出し直し不要)。当たり判定はDxLibの`MV1SetupCollInfo`+`MV1CollCheck_Sphere`(描画しないモデル)。
  - **(10/27済・Claude、ユーザー依頼)①パッケージ導入**: StageEditorにProBuilder 5.2.4・FBX Exporter 5.1.6(+Autodesk FBX SDK 5.1.3)。Unity 2022.3.62f1をバッチモードで起動し、一時エディタスクリプト(`Client.AddAndRemove`)で追加→スクリプトは削除済み。`.gitignore`の`**/[Pp]ackages/*`(NuGet用)が`StageEditor/Packages/`も無視していたので、`!StageEditor/Packages/manifest.json`と`packages-lock.json`を例外に追加(家と学校で同じパッケージになる。学校でUnityを開くと自動で取ってくる)。**コミットが必要**。
  - **(10/27済・Claude、ユーザー依頼「どれが何の敵か分かりづらい」→B案: Unityでゲームの実際のモデルを表示)**:
    - **一番の原因**: `.gitignore`の`*.meta`(VS用)がUnityの`.meta`も無視していて、PCごとにスクリプトのGUIDが違い、Stage1のシーンの82個が「スクリプトが外れた」状態だった(色付きの枠・名前のギズモが出ず白い仮の形だけ)。→`PlacedObject.cs.meta`のGUIDをシーンが覚えている`a55b2dc981af5b5409471bd2e6b950f3`に合わせ、`.gitignore`に`!StageEditor/**/*.meta`と`!StageEditor/Assets/**/*.obj`(`*.obj`もVS用で、岩のOBJが無視されるため)を追加。**今後は家と学校で`.meta`が共有される(コミットが必要)**。
    - モデル: `StageEditor/Assets/Models/`に浮遊敵(`Drone_fix.fbx`+`Drone.fbm/Drone_Albedo.png`)・ワーム(`Worm_fix.fbx`+`T_Space_Drone_D_Red.png`)・ボス(`Boss.fbx`+`T_Mech_LOD2_B.png`)・ビル6種(FBX+jpg)・岩3種(`.mv1`しか無いので、DxLibで`MV1GetReferenceMesh`→OBJに書き出す変換プログラムをscratchpadで作って変換。x反転+三角形の順序反転でUnityの読み込み後にDxLibと同じ向き)。二重管理(ゲームのモデルを変えたらコピーし直す)。
    - [StageModelPostprocessor.cs](StageEditor/Assets/Editor/StageModelPostprocessor.cs): `Assets/Models`はファイルの数値のまま読む(`useFileScale=false`、`globalScale=1`、マテリアル/アニメは読まない)。
    - [StageModelCatalog.cs](StageEditor/Assets/Editor/StageModelCatalog.cs): modelID→モデル・テクスチャ・大きさ(ゲームの`model_scale`÷100: 浮遊敵2・ワーム1・ボス3・岩(3,5,3)・ビル1)。マテリアルは`Assets/Models/Materials/<modelID>.mat`に自動作成。一覧にないmodelIDは種類ごとの色の仮の形。メニュー「NovaWing/Refresh Visuals」(置いてあるものの見た目を作り直す)、バッチ用`RefreshStage1Batch`/`LogBoundsBatch`。`StageCsvImporter`も取り込み時にこれで見た目を作る。**ゲームの`model_scale`を変えたらここも合わせる**。
    - [PlacedObject.cs](StageEditor/Assets/Scripts/PlacedObject.cs): 種類に`Building`(黄色、enumの末尾)を追加→`Export Stage CSV`で`Building.csv`が出る。`KindColor`を`public static`に。
    - 確認: Unityのバッチで、Stage1のシーンの82個の見た目をゲームのモデルに作り直して保存。**FBXの向きはDxLibとUnityで同じ**(`Building_Leaning`の横の範囲がどちらも-82.41〜67.00)。ビルの大きさ(ファイルの数値): 幅100〜180・高さ47〜781程度(Leaning 高さ約340)。ゲームでの大きさはCSVのscale(Unityで親の大きさを変える)で決める想定。
  - **(10/27追記)大きさの修正**: ゲームの浮遊敵・ワーム・岩は独自シェーダ(`DrawWithLighting`、スキンなし)で描いていて、**モデル内の部品ごとの拡大(フレームの行列)を使わない**。UnityのFBX読み込みはそれを使うので、浮遊敵・ワームは2.54倍、岩は横3〜5倍・縦8倍大きく見えていた。→この3つはDxLibで`MV1SetupReferenceMesh(handle,-1,FALSE)`(部品ごとの拡大なし)で書き出したOBJに差し替え(`Drone_fix.obj`/`Worm_fix.obj`/`Rock1〜3.obj`、FBXのコピーは削除)。ボス(スキンメッシュ、`DrawWithLighting(textures,true)`)とビル(部品ごとの拡大なし)はFBXのままで合う。岩の大きさ(半径55〜160)はゲームの当たり判定の球(64〜180)とも合う。
  - 名前の文字は「選んでいる or シーンビューのカメラから40以内」だけ、短い日本語名1行(浮遊敵/ワーム/ボス/岩1/ビルは種類名)。枠は当たり判定の大きさ(浮遊敵132・ワーム40)。**ワームは置いた位置(螺旋の中心)にモデルの中心を合わせて置き(表示だけ6倍なので当たり判定の枠は描かない)、螺旋の円(半径160)だけ描く。胴体はユーザー判断で描かない**(置いた位置=螺旋の中心)。ワームの頭はゲームでももともと小さい(当たり判定40)。
  - (10/27ユーザー「Unityだけの話、ゲームはどうでもいい」)ワームの頭のモデルは**Unityの表示だけ6倍**(3倍ではまだ小さいとのこと)(`StageModelCatalog`の`WormDisplayScale`)。CSV・ゲームには影響しない。枠・螺旋・胴体の球はゲームと同じ大きさのまま。
  - 注意: Unity側の値(大きさ・ワームの動き・当たり判定の半径)はゲームの.cppの定数を写したもの。ゲーム側を変えたら[StageModelCatalog.cs](StageEditor/Assets/Editor/StageModelCatalog.cs)・[PlacedObject.cs](StageEditor/Assets/Scripts/PlacedObject.cs)も合わせる。
  - (10/27)ボスの大きさはUnityとゲームで同じ比率と確認(幅約3250・高さ約2400、ボスのCSVの高さ8000は登場ムービー用で戦闘は海面)。ユーザー了承。
  - **(10/27・Claude)ビルの当たり判定を作る道具** [BuildingCollisionTool.cs](StageEditor/Assets/Editor/BuildingCollisionTool.cs)+[BuildingCollisionMarker.cs](StageEditor/Assets/Scripts/BuildingCollisionMarker.cs): メニュー「NovaWing/Building Collision/Edit <ビル>」でビルを原点(1/100)に置いた作業用シーン(`Assets/Scenes/BuildingCollision.unity`)を開く→「Collision」の子にProBuilderで形を作る→「Export」で`Data/Model/Building/<ビル>_Col.fbx`に書き出し(Unity側`Assets/Models/`にもコピーし、次回Editで前回の形を読み込む)。**FBX Exporterは1単位を100倍(m→cm)で書くので、1/100で作るとビルのFBXと同じ単位になる**(要検証: 書き出したFBXをDxLibで測ってビルと比べる)。
  - **(10/27完了・Claude、ユーザー依頼)ビル6種の当たり判定を自動生成**: メニュー「NovaWing/Building Collision/Auto Generate All」= ビルの頂点を高さ方向に10段に輪切りし、段ごとのXZの範囲の箱を作り、上下で大きさの差が15%以内の段はまとめる(AntennaTower4・BridgeWreck9・Collapsed6・Leaning4・OfficeGutted2・TallBroken3箱)。**ゲーム用は`.x`(`Data/Model/Building/Building_<種類>_Col.x`)**: DxLibはFBX ExporterのFBX(ASCII 7.7)もBlenderのFBXも`MV1LoadModel`で読めなかった。さらにFBX Exporter経由(ユーザーがモデルビューアで変換した`Building_AntennaTower_Col.mv1`)は**ビルに対して180°回転**していた→FBX Exporterを通さず、Unityの座標×100をそのまま`.x`(左手系、三角形の順番そのまま)に書く方式にした。**DxLibで6種とも読み込めて、ビルのモデルと範囲が小数点以下まで一致**することを確認。Unityでの手直し用FBXは`Assets/Models/<種類>_Col.fbx`のみ(Editで前回の形を読み込む)。`Building_AntennaTower_Col.mv1`は向きが違うので削除済み(ユーザー了承)。
  - **(10/27・Claude)2面の配置用メニュー** [StagePlacementMenu.cs](StageEditor/Assets/Editor/StagePlacementMenu.cs): 「NovaWing/Create Stage2 Scene」で`Assets/Scenes/Stage2.unity`を作る(書き出し先はシーン名のフォルダ=`Data/CSV/Stage2`)。「NovaWing/Add/...」でシーンビューの注視点にビル6種・浮遊敵・ワーム・岩1〜3・ボスを見た目付きで置く(ビル・岩は高さ0)。**ビルは置いたときscale 10**(モデルの幅100〜180→ゲームで1000〜1800。CSVのscaleになるので、ゲーム側はビルの見た目と当たり判定の両方にCSVのscaleを掛ける必要がある)。
  - **(10/27)ユーザーがStage2シーンを作りビルを1つ置いて書き出し済み**: [Data/CSV/Stage2/Building.csv](Data/CSV/Stage2/Building.csv)(`Building_AntennaTower,0,0,0,0,0,0,10,10,10`)。「Export Stage CSV」は置いてある種類のCSVだけ書くので、2面の他のCSV(仮のボス等)は前のまま。**注意: Z=0はステージ開始時のプレイヤーの位置。他のCSVと同じくZは5500(Unityで55)より奥に置く**(今は読み込み確認用ならそのままでも可)。
  - **次回(ゲーム側、ユーザーが書く)**:
    1. ビルのモデルを読めるように(岩と同じ場所): [ResourceLoader.h:14-25](Manager/ResourceLoader.h#L14-L25)の`ModelID`にビル6種、[ResourceConstants.h:12](Constants/ResourceConstants.h#L12)付近にモデルのパス(`Data/Model/Building/Building_○○.mv1`)、[ResourceLoader.cpp:106](Manager/ResourceLoader.cpp#L106)の対応表、[442-444行目](Manager/ResourceLoader.cpp#L442-L444)のゲームシーンのモデル一覧、CSVのmodelID→ModelID([ResourceConstants.h:143](Constants/ResourceConstants.h#L143)付近のCSV名と[ResourceLoader.cpp:641](Manager/ResourceLoader.cpp#L641)の`WStringToModelID`の表)。
    2. `BuildingDataSetter`とビルのクラス(`RockDataSetter`・`Rock`をまねる): `Building.csv`を`csvFolderName`・`offsetZ`付きで読む。**CSVのscaleを見た目に掛ける**(岩は固定の大きさだった)。同じ位置・回転・大きさで当たり判定用の`Building_○○_Col.x`も読み込んで持つ。
    3. 当たり判定: 今の`ICollider`は球どうしだけ。ビルはDxLibの`MV1SetupCollInfo`+`MV1CollCheck_Sphere`で球×メッシュを判定するので、仕組みに足す(2の後に一緒に設計)。
  - **コミットが必要なもの(10/27時点で未コミット)**: `.gitignore`(Unityの`.meta`/`*.obj`/パッケージ一覧の例外)、`StageEditor/Packages/`(manifest・lock)、`StageEditor/Assets/`のスクリプト・`.meta`・`Models/`・シーン(Stage1/Stage2/BuildingCollision)、`Data/Model/Building/`(ビル6種と当たり判定の`.x`6つ)、`Data/CSV/Stage2/Building.csv`。
  - **(10/28)ゲーム側のビル(ユーザー実装)**: [Building.h](Game/GameObjects/Actors/Building/Building.h)/[.cpp](Game/GameObjects/Actors/Building/Building.cpp)(位置・Y回転(ラジアン)・大きさ、当たり判定の`.x`を`MV1DuplicateModel`→`ApplyMatrix`→`MV1SetupCollInfo`、描画は`MV1DrawModel`)、[BuildingDataSetter](Game/GameObjects/Actors/Building/BuildingDataSetter.cpp)(大きさは7〜9列目、見た目→当たり判定のID対応表`col_model_ids`)、`GameScene::CreateStageObjects`で生成・`Init`(`m_pBuildings`)。**大きさ`m_scale`を`Actor`に移した**(初期値1、`SetScale`/`GetScale`、`Scenery`も`Actor`のものを使う。プレイヤー・敵・岩は今まで通り.cppの`model_scale`)。Debugビルド成功。当たり判定(`CollisionManager`への登録)は未着手。
  - **(10/28設計・ユーザー了承)ビルの当たり判定**: ①`Shape::Mesh`+`MeshShape`(当たり判定モデルのハンドルを持つ) ②`IsHit`に球×メッシュ(`MV1CollCheck_Sphere`→`HitNum>0`→`MV1CollResultPolyDimTerminate`) ③`BuildingCollider`(`ColliderTag::Building`、`GetOwnerID`、RockColliderをまねる、ビルがメンバで持つ) ④`DamageSourceType::Building`・`TryGetDamageSource`・`PlayerCollider::OnCollision`・`hit_pairs`に`{Building, Player}` ⑤`CollisionManager::RegisterBuilding`+`Update`で集める、`GameScene`で登録。**ぶつかった時は岩と同じ(ダメージ20、離れるまで1回だけ)**。弾×ビルは後回し(優先度Bの扱い)。
  - **(10/28作業中・ユーザー)当たり判定の①**: [ColliderShape.h:9](Game/Collision/ColliderShape.h#L9)に`Shape::Mesh`を追加済み。[MeshShape.h](Game/Collision/MeshShape.h)/[.cpp](Game/Collision/MeshShape.cpp)の大枠を作成(vcxproj登録済み)。**次回(家で)の残り**:
    - コンストラクタを`MeshShape(int modelHandle)`にし、`ColliderShape(Shape::Mesh)`を呼んでメンバに入れる(**今は引数なしで`ColliderShape`の初期化もないので、ビルドするとエラーになる**。`ColliderShape`には引数なしのコンストラクタがない)。
    - `int GetModelHandle() const`を追加(②の`MV1CollCheck_Sphere`で使う)。
    - `Draw`: `MV1SetWireFrameDrawFlag(m_colModelHandle, TRUE)`→`MV1DrawModel`→`FALSE`に戻す(色は`MV1SetDifColorScale`で任意)。
    - **デストラクタで`MV1DeleteModel`しない**(ハンドルはビルの`m_colModelHandle`を借りるだけ。ビルのデストラクタが消す)。
    - [MeshShape.h:3](Game/Collision/MeshShape.h#L3)のincludeが`"../Game/Collision/ColliderShape.h"`になっている(同じフォルダなので`"ColliderShape.h"`でよい。今のパスはフォルダの位置がずれていて見つからない可能性がある)。
    - その後: ②`IsHit`に球×メッシュ([CollisionManager.cpp:62-79](Manager/CollisionManager.cpp#L62-L79)) → ③`BuildingCollider` → ④ダメージ・`hit_pairs` → ⑤登録。
  - **(10/29完了・ユーザー実装、動作確認OK)ビルの当たり判定**: `MeshShape(int modelHandle)`(ハンドルは借りるだけ、`Draw`はワイヤーフレーム)、`CollisionManager`の`IsHitSphereAndMesh`(表面=`MV1CollCheck_Sphere`、**中にいるか=球の中心から真上へ`inside_check_length`(100000)の線を`MV1CollCheck_LineDim`し、つき抜けた回数が奇数なら中**)と`IsHit`の球×球/球×メッシュ/メッシュ×球の分岐、`BuildingCollider`(`ColliderTag::Building`、`GetOwnerID`)、`Building`が`unique_ptr<BuildingCollider>`を持ち`GetCollider()`、`DamageSourceType::Building`・`TryGetDamageSource`・`PlayerCollider`のダメージ・`hit_pairs`に`{Building, Player}`、`RegisterBuilding`と`Update`での収集、`GameScene`で登録。ビルの`Draw`でデバッグ表示時に当たり判定を描く。
    - コスト表「2面の建物をUnityで配置して読み込む」: 見積3h/**実4hで完了**。プロト残7h(突進する敵2h・固定砲台の敵2h・B: 移動の加速度2h・岩の当たり判定1h・難易度調整0h)。
    - つまずき(参考): `MV1CollCheck_Sphere`は表面のポリゴンしか見ないので、ビルの中に完全に入ると「離れた」扱いになり、出るときにもう一度ダメージを受けた→中にいるかの判定を追加(線が短いとビルの上まで届かず数えられない)。`IsHit`の球どうしで結果を捨てて`return true`していなかった(全部当たらなくなるところだった)。`Building.h`で`ICollider`の前方宣言が抜けてビルドエラー。
  - (10/28)ビルが見えない→Unityで子の`Visual`を動かしていて、親(書き出される方)はZ=0のままだった。`PlacedObject`に`[SelectionBase]`を付け、シーンビューでモデルをクリックしても親が選ばれるようにした(Claude)。
  - 進め方: ①StageEditorにProBuilder(`com.unity.probuilder`)とFBX Exporter(`com.unity.formats.fbx`)を入れる(未導入) ②1種類だけ作って書き出し→ゲームで読み込み、**大きさ・向き・単位(Unity1=ゲーム100、FBXの単位)が見た目と合うか先に確認** ③残りの種類→配置CSVと読み込み→`ICollider`の仕組みにつなぐ(球×メッシュの判定を足す)。
  - **(10/27完了・Claude、ユーザー依頼)1. ビルのモデルの登録(コメントは「2面」ではなく「2ステージ目」と書く、ユーザー指定)**: `ModelID`に見た目6種(`BuildingTallBroken`/`OfficeGutted`/`Leaning`/`Collapsed`/`AntennaTower`/`BridgeWreck`)と当たり判定用6種(各`...Col`)、`ResourceConstants.h`にパス12個(見た目`.mv1`、当たり判定`_Col.x`)とCSV名6個(Unityの`StageModelCatalog.cs`と同じ`Building_○○`)、`ResourceLoader.cpp`の対応表・`game.models`・`WStringToModelID`に追加。Debugビルド成功。**ゲーム内で`_Col.x`が読めるか(assertで止まらないか)は未確認**。当たり判定のモデルも`ModelID`で登録したのは、読み込み・解放を`ResourceLoader`に任せるため。※`KeepModel()`(ResourceLoader.cpp)はどこからも呼ばれていない古い処理なので足していない。

### 2026-10-29 突進する敵の仕様(ユーザー了承)と、タスク2つ(ユーザー報告)
- **突進する敵の仕様(合意)**: 置いたままだと何もしていないうちに倒せてしまうので、**プレイヤーが一定の位置まで来たら画面外から現れる**。
  1. ゲーム開始時に作っておくが、出てくるまでは見えない・当たらない・ロックオンできない。
  2. プレイヤーのZがCSVの`activatePlayerZ`(ワームと同じ考え方)を超えたら、**プレイヤーの位置から決まった分ずれた画面外**に現れる。
  3. 予備動作(約0.5秒): 画面の端から少し入って止まり、プレイヤーを向いて震える。
  4. 突進: そのときのプレイヤーの位置へまっすぐ速く(追いかけない)。
  5. すれ違って画面外に出たら消える。ぶつかったらプレイヤーにダメージ20。
  6. 倒すと浮遊敵と同じ爆発。HPは少なめ(`EnemyStatus.csv`)。
  - モデルはユーザーが後で拾う。それまで今あるモデル(浮遊敵のドローン)を仮で使う。
  - 配置はUnity: 置いた位置のZ=`activatePlayerZ`、X・Y=プレイヤーからのずれ、前後のずれは別の数字で持つ。
- **【タスク・ユーザー報告】farの外で見えない敵をロックオンできてしまうバグ**: ロックオン候補の判定は[TargetManager.cpp:154-169](Manager/TargetManager.cpp#L154-L169)の`IsOnScreen`(画面内+`screenPos.z >= 1.0f`で奥を除外)。カメラのfarは[CameraBase.cpp:11](Game/GameObjects/Camera/CameraBase.cpp#L11)の5500。原因は未調査。
- **【タスク・ユーザー報告】浮遊敵の弾を撃つタイミングが全員同じで、弾が散らばらない**: 浮遊敵は最初から配置しているため。コスト表の「敵全体＞個体ごとに動きと攻撃のタイミングをずらす」(A・アルファ)に当たる。
  - いずれ浮遊敵も「画面外から生まれる」仕様にしたい(今は直さない、ユーザー)。
  - (10/29)コスト表に追加: 「バグ修正＞　ーfarの外の敵をロックオンできる」1h・B・ベータ、「敵全体＞　ー浮遊敵を画面外から出現させる」4h・C・アルファ。バグ修正の親を未完了に戻した。プロト 81/残7、アルファ 78/68、ベータ 45.5/36.5、合計204.5/残111.5。
- **(10/29)突進する敵の実装・進捗**: 作り方を変更。最初から作って隠すのではなく、**プレイヤーが決めたZまで来たら`EnemyFactory::Create`で作る**(`EnemySpawner`がCSVを持つ。いずれ浮遊敵にも使う)。位置は、Appear/Chargeの間は「プレイヤーの位置＋待機位置のずれ」。Rushでは`m_pos`を直接動かし、Zだけプレイヤーの前進分を足す(横に動けば避けられる)。
  - 手順1(済・ユーザー): `ModelID::RushEnemy`(モデルは仮で`Drone_fix.mv1`)、CSV名`RushEnemy`、[EnemyStatus.csv](Data/CSV/Params/EnemyStatus.csv)に`RushEnemy,30,132,10,0`。
  - 手順2(済・ユーザー): `ColliderTag::RushEnemy`・`DamageSourceType::RushEnemy`・`hit_pairs`4つ・`TryGetDamageSource`・[PlayerCollider.cpp](Game/Collision/PlayerCollider.cpp)でダメージ20。
  - 手順3: [RushEnemy.h](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.h)/[.cpp](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.cpp)を、Claudeが「クラスの追加」と同じ作ったばかりの形(`EnemyBase`継承、コンストラクタ・デストラクタだけ)で作成、vcxproj登録済み(ユーザー依頼)。中身はユーザーが書く。`EnemyBase()`の引数を書くまではビルドエラー。
  - ついでに、`NovaWing.vcxproj.filters`の閉じタグのずれ(`WarningUI`の行が閉じておらず、末尾に余分な閉じタグ)を直した。
  - 残り: 手順3の中身 → 4. `EnemyType::RushEnemy` → 5. `EnemySpawner`+`GameScene` → 6. Unityの配置・書き出し(Claude)・AIテストプレイが突進する敵を認識するようにする(Claude)。
  - モデル: ロケット型で顔付きのモデルを、ユーザーがClaude Desktopで作る(.x、機首-Z、全長約260)。届いたら`rush_enemy_model_path`を差し替える。
  - (10/29・Claude、ユーザー依頼)突進する敵のモデルが届いた(`Data/Model/RushEnemy/`、ユーザーが`.x`→`.mv1`に変換)。`rush_enemy_model_path`を`RushEnemy.mv1`に変更し、`GraphicID::RushEnemyNormalMap/MetalicMap/EmissionMap`をパス・対応表・`game.graphics`に登録。Debugビルド成功。`.x`の頂点の範囲はX±75・Y-51〜86・Z±130(全長260、細い機首が-Z側＝プレイヤーの方を向く)。**`model_scale`は1倍**(ドローンの2倍ではない)。
  - (10/29・Claude、ユーザー依頼)[EnemySpawner.h](Manager/EnemySpawner.h)/[.cpp](Manager/EnemySpawner.cpp)を作ったばかりの形(コンストラクタ・デストラクタだけ)で作成、vcxproj登録、Debugビルド成功。中身はユーザーが書く。仮の[Data/CSV/Stage2/RushEnemy.csv](Data/CSV/Stage2/RushEnemy.csv)(列: `modelID,posX,posY,posZ,rot×3,scale×3,frontOffset`。posZ=出てくるタイミング(ステージ開始からの距離)、posX/posY=プレイヤーからの左右・上下のずれ、frontOffset=どれだけ前か)。2体(Z=3000で右上、Z=6000で左上)。Unityの書き出しはこの形に合わせる(Claude、未着手)。
- **(10/29帰宅時点)突進する敵の進み具合**
  - 完了: 手順1・2(ModelID・ステータス・当たり判定のタグ)、`RushEnemy`クラス(Appear→Charge→Rush、Draw・TakeDamage・GetCollisionSpheres)、`EnemyFactory`の`EnemyType::RushEnemy`、モデルとテクスチャの登録、`EnemySpawner`の.hとコンストラクタ。Debugビルド成功。**まだゲームの中で動かしていない**。
  - **次回(ユーザーが書く)**:
    1. [EnemySpawner.cpp](Manager/EnemySpawner.cpp)の`LoadRushEnemy`: [WormEnemyDataSetter.cpp:38-89](Game/GameObjects/Actors/Character/Enemy/WormEnemy/WormEnemyDataSetter.cpp#L38-L89)と同じ読み方。include `Manager/CSVDataLoader.h`・`CSVData/CSVData.h`。定数はCSVのファイル名`L"RushEnemy"`(ResourceConstants.hの`rush_enemy_csv_name`と名前がぶつかったら`rush_enemy_file_name`に)と`front_offset_number = 10`。1行ごとに`posX`(1)・`posY`(2)・`posZ`(3)・`frontOffset`(10)を`std::stof`し、`SpawnData{ RushEnemy, posZ + offsetZ, Vector3(posX, posY, frontOffset) }`を`m_spawnDatas`に`push_back`する(敵はまだ作らない)。
    2. `Update`: `m_spawnDatas`を見て、`プレイヤーのZ >= activatePlayerZ`なら`m_pEnemyFactory.lock()->Create(プレイヤーの位置 + offset, type)`して一覧から消す(`erase`+`remove_if`、[TargetManager.cpp:38-45](Manager/TargetManager.cpp#L38-L45)と同じ書き方)。
    3. `GameScene`: .hに`std::unique_ptr<EnemySpawner> m_pEnemySpawner;`(前方宣言)。[CreateStageObjects](Scene/GameScene.cpp#L254)のビルの下で`make_unique`して`LoadRushEnemy(m_pStageConfig->GetCSVFolderName(), offsetZ)`。[UpdateGameWorld](Scene/GameScene.cpp#L616)の`UpdateAll()`の下で`if (m_pEnemySpawner) m_pEnemySpawner->Update();`。
    4. ゲームで確認: 2ステージ目で、ステージ開始から3000進むと右上、6000で左上から入ってきて、震えてから突っ込んでくるか。モデルの向き(機首=顔がプレイヤーの方)・大きさ・HP30の硬さ・`posX/posY`(画面の端から入ってくるか)を見て調整する。
  - 細かい直し: [RushEnemy.cpp:258](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.cpp#L258)のコメントが「メタリックマップはないので-1」のまま(中身は直っている)。[EnemySpawner.h](Manager/EnemySpawner.h)のクラスの説明コメント(`/// <summary>`)が消えている。
  - Claudeがやること: Unityで突進する敵を置いて、`RushEnemy.csv`(`frontOffset`列付き)を書き出せるようにする。AIのテストプレイが突進する敵を敵と分かるようにする([AutoPlayWorld.cpp:163-164](DebugTools/AutoPlay/AutoPlayWorld.cpp#L163-L164)のタグ判定に`RushEnemy`を足す)。
  - 別件(ユーザー報告待ち): ワームがfarの外に出て消えるとき([WormEnemy.cpp:114](Game/GameObjects/Actors/Character/Enemy/WormEnemy/WormEnemy.cpp#L114))が`OnEnemyDead()`なので、倒していなくても撃破数が増える。
  - **要確認**: `Data/Model/Drone.mv1`・`Drone_fix.mv1`が10/29 17:05に変わっていて、`Data/Model/Drone.FBX`が消えている(git上は変更・削除)。突進する敵の`.mv1`を作った時刻(17:06)と同じなので、モデルビューアで開いたときに上書きされた可能性がある。わざとでなければ、コミットする前に元に戻す(GitHub Desktopでその3つを「Discard changes」)。
  - 未コミット: 建物の当たり判定一式・突進する敵一式・`EnemySpawner`・`Data/Model/RushEnemy/`・`Data/CSV/Stage2/RushEnemy.csv`・コスト表など。

### 2026-10-30 突進する敵がゲームの中で動いた(ユーザー確認)
- **完了(ユーザーが書いた)**: [EnemySpawner.cpp](Manager/EnemySpawner.cpp)の`LoadRushEnemy`(CSVから`SpawnData`を作って`m_spawnDatas`に入れるだけ、敵はまだ作らない)と`Update`(イテレータで回し、出したら`it = erase(it)`)。`GameScene`は、Initで工場の後に`make_unique`、`CreateStageObjects`で`LoadRushEnemy`、`UpdateGameWorld`の`UpdateAll()`の下で`Update()`。
- **ちらつきの修正**: Spawnerが作ったフレームは、Updateを通らないまま待機位置に描かれていた(`UpdateAll`の後に作られるため)→ コンストラクタで最初から「待機位置＋外側の方向×`appear_distance`」に置く。
- **向き**: `m_rotation`を一度も設定しておらず、モデルが下を向いたままだった → `LookAtDir(dir)`を追加(`Cross(model_forward, to)`を軸、`acos(Dot)`を角度、同じ向き・真逆の時は別処理)。Appear・Chargeはプレイヤーの方、Rushは`m_rushDir`を向く。**`model_forward = (0,-1,0)`で正しく向いた**(`.x`の頂点は機首-Zだったが、mv1では-Y)。
- **予備動作中の位置**: プレイヤーの上下左右の動きについていくのが気持ち悪い(ユーザー) → 出現が終わった位置を`m_chargeCenter`に覚え、予備動作中はZだけプレイヤーの前進分を足す。位置は「中心＋震え」。上下左右についていくのは出現(Appear)の間だけ。
- **配置(Claude、ユーザー依頼)**: [Data/CSV/Stage2/RushEnemy.csv](Data/CSV/Stage2/RushEnemy.csv)を2体→12体に(3000〜25000、後半ほど同時出現・近い距離。ボスは30000)。Unityの書き出しを作ると上書きされるので、配置はUnity側にも置く。
- **残り**:
  - [TitleScene.cpp:80](Scene/TitleScene.cpp#L80)の`first_stage_no`がテスト用の`1`のまま → `0`に戻す。
  - Claude: Unityで突進する敵を置いて`RushEnemy.csv`(`frontOffset`列付き)を書き出す。AIテストプレイが突進する敵を敵と分かるようにする([AutoPlayWorld.cpp:163-164](DebugTools/AutoPlay/AutoPlayWorld.cpp#L163-L164))。
  - デバッグワープで出現Zを一気に飛び越えると、飛び越えた分の敵が同じフレームでまとめて出る(デバッグ用なので今は気にしない)。
- (10/30)出現方向: 真上から来るのがしぶい(ユーザー、上下左右はOK)→ CSVの`posX=0`の2体を変更(8500: `-900,200`=左、17000: `900,400`=右上)。`posX`を0にすると真上/真下から来る。frontOffsetはユーザーが全部+1000に変更済み。
- **(10/30)コスト表「突進する敵」を完了に(ユーザー申告で実5h)**: 死亡 実1h・プレイヤーに向かって移動 実4h(内訳はClaudeの割り振り)、親も完了。バックアップはscratchpad。**プロト 81/残5**(S: 固定砲台の敵2h、B: 移動の加速度2h・岩の当たり判定1h・難易度調整0h)、合計204.5/残109.5。
- **(10/30帰宅時点)まとめ・次回やること**
  - 突進する敵は**完了**(ゲームで確認済み・コスト表も完了)。[TitleScene.cpp:80](Scene/TitleScene.cpp#L80)の`first_stage_no`は`0`に戻っている(確認済み)。古いコメント(「メタリックマップはないので-1」)も無くなっている。
  - **締め切り10/31(明日)。プロトの残り5h**:
    1. **固めの固定砲台の敵(2h・S)← 次はこれ**。突進する敵の道具がそのまま使える: 出し方は`EnemySpawner`に`LoadTurretEnemy`(仮)を足す/`EnemyFactory`に`EnemyType`を足す、向きは`LookAtDir`と同じ作り、弾は浮遊敵の撃ち方。CSVは`Data/CSV/Stage2/`に置く。仕様(どこに置くか・動くか・HP・弾の間隔)はまだ決めていない → 最初に決める。
    2. B(終わらなければアルファへ回してよい): 移動の加速度2h、岩の当たり判定(ノーマル/チャージ)1h、難易度調整0h。
  - Claudeがやること(急ぎではない): Unityで突進する敵を置いて`RushEnemy.csv`を書き出せるようにする(作ると今の手書きの配置は上書きされるので、配置をUnity側にも置く)。AIテストプレイが突進する敵を認識するようにする([AutoPlayWorld.cpp:163-164](DebugTools/AutoPlay/AutoPlayWorld.cpp#L163-L164)、.cppなのでユーザーの依頼があってから)。
  - コミット: 突進する敵一式・`EnemySpawner`・`GameScene`の変更・`RushEnemy.csv`・コスト表・NOTES.md。(このPowerShellには`git`が無く、未コミットかどうかは確かめられなかった)

### 2026-10-31 突進する敵はプレイヤーに当たったら爆発して消える(ユーザー実装、確認OK)
- `EnemyBase`に`virtual void OnHitPlayer() {}`を追加(普通の敵は何もしない)。`EnemyCollider::OnCollision`で相手が`ColliderTag::Player`なら`m_owner.OnHitPlayer()`。`RushEnemy`が上書きして死亡処理へ。プレイヤーのダメージは多段ヒット防止で1回だけ。
- **次**: 固定砲台の敵(コスト表 2h・S)。仕様(了承済み): ビルの屋上などに固定、HP多め(30目安)、プレイヤーが近づいて画面に映っている間だけ一定間隔(1〜1.5秒)で撃つ。モデルは`Base`→`Turret`(Y軸回転)→`Barrel`(上下)のフレームの親子で、ユーザーがClaude Desktopで作る(`Data/Model/TurretEnemy/`、`.x`→`.mv1`はユーザーが変換)。届くまでは`Drone_fix.mv1`を仮で使い、砲塔の回転は後で足す。作り方は最初から置く形(`TurretEnemyDataSetter`、浮遊敵と同じ)。

### 2026-10-31 固定砲台の敵: 登録とファイル作成(Claude、Debugビルド成功)
- 仕様追加: **接触ダメージ無し**(`ColliderTag::Floating`を使う)。
- 登録: `ModelID::TurretEnemy`([ResourceLoader.h:44](Manager/ResourceLoader.h#L44))、パス表・`game.models`・`WStringToModelID`([ResourceLoader.cpp:115](Manager/ResourceLoader.cpp#L115)、[473](Manager/ResourceLoader.cpp#L473)、[674](Manager/ResourceLoader.cpp#L674))、モデルは仮で`Drone_fix.mv1`、`EnemyStatus.csv`に`TurretEnemy,30,132,10,0`。
- ファイル: [TurretEnemy.h](Game/GameObjects/Actors/Character/Enemy/TurretEnemy/TurretEnemy.h)/.cpp(コンストラクタとデストラクタだけ)、[TurretEnemyDataSetter.h](Game/GameObjects/Actors/Character/Enemy/TurretEnemy/TurretEnemyDataSetter.h)/.cpp、[Data/CSV/Stage2/TurretEnemy.csv](Data/CSV/Stage2/TurretEnemy.csv)(ヘッダのみ)。vcxproj登録済み。
- **次**: ユーザーが中身を書く(データ構造体・Init/Update/Draw/被ダメージ・当たり判定、`CreateEnemy`、[GameScene.cpp:257](Scene/GameScene.cpp#L257)の`CreateStageObjects`で生成)、CSVに配置。

### 2026-10-31 敵の共通部分をEnemyBaseへ移動(Claude、ユーザー依頼)
- `m_pSoundManager`・`m_pEffectManager`・`m_dyingFrame`を[EnemyBase.h:64](Game/GameObjects/Actors/Character/Enemy/EnemyBase.h#L64)へ。コンストラクタの引数に2つのマネージャーを追加(浮遊敵・ワーム・ボス・突進・砲台を修正)。
- `TakeDamage`の標準処理を[EnemyBase.cpp:29](Game/GameObjects/Actors/Character/Enemy/EnemyBase.cpp#L29)へ。浮遊敵・突進・砲台は親のものを使う。ワームとボスは今までどおりoverride。当たり判定は敵ごとに違うので子に残した。
- 全ファイルのコンパイルは成功。リンクは`TurretEnemy`の未実装6関数(`OnInit`・`Update`・`Draw`・`GetCollisionSpheres`・`GetColliders`・`OnHitPlayer`)だけで失敗中。砲台の中身を書くまでゲームは起動できず、動作確認は未実施。`OnHitPlayer`は宣言を消す。

### 2026-11-01 固定砲台の敵のモデルを登録(Claude、ユーザーが`.mv1`に変換)
- `turret_enemy_model_path`を`Data/Model/TurretEnemy/TurretEnemy.mv1`に変更。`GraphicID::TurretEnemyNormalMap/MetalicMap/EmissionMap`をパス・対応表・`game.graphics`に登録。
- 変換はDxLibModelViewerの「名前を付けて保存」。モデルの原点は底面の中心、正面は-Z、`Turret`はY軸(原点0,115,0)、`Barrel`はX軸(Turretから0,64,-78、±50°)。
- **次(ユーザー)**: `DrawEnemy`のテクスチャ差し替え、`model_scale`を1倍で確認、当たり判定の球と発射位置を上にずらす、ステート、回転。

### 2026-11-01 固定砲台の敵が土台に埋まって見える原因(調査のみ、未修正)
- モデルは正常。`TurretEnemy.mv1`をDxLib標準の描画で表示すると正しい形で、フレーム位置も`Turret`=(0,115,0)、`Barrel`=(0,179,-78)。
- 原因は描画側: [Actor.cpp:63](Game/GameObjects/Actors/Actor.cpp#L63)がモデル全体の行列だけを渡し、`LightingVS.hlsl`がそれで全頂点を変換するので、フレームのずれが消える。
- 直し方(案、未検証): `Actor`に、フレームごとに`MV1GetFrameLocalWorldMatrix`→`world`へ入れて`UpdateShaderConstantBuffer`→`MV1DrawFrame`する描画関数を足す。`DrawWithLighting`とのテクスチャ処理の重複は切り出す。砲塔の回転もこれで見えるようになるはずなので、ステートより先に直す。
- 補足: ボスは`DrawWithLighting(textures, true)`で`SkinnedLightingVS`(フレームごとの行列を使う)、プレイヤーは`MV1DrawModel`なので同じ現象は起きない。突進・ワーム・浮遊敵・岩はメッシュを持つフレームにずれがない。砲台の頂点はボーン情報を持たないので`true`では直らない。
- **要確認**: 砲台の頂点は`type0`で接線の情報が無い(他のモデルは`type4`、ボスは`type5`)。法線マップが正しく効かない可能性がある。原因は未調査。フレーム描画を直してから見た目で確認し、問題があれば`.fbx`から変換するなど出力形式を見直す。

### 2026-11-01 固定砲台の敵の表示を修正(ユーザー実装、確認OK)
- `Actor::DrawFrameWithLighting`を追加(フレームごとに`MV1GetFrameLocalWorldMatrix`→`world`→`MV1DrawFrame`)。テクスチャ処理は`SetTexturesToShader`/`ReleaseTexturesToShader`に切り出した。砲台の見た目は「良い感じ」とユーザー確認。
- **次**: 当たり判定の球を上にずらす → ステート → `Shoot` → 砲塔と砲身の回転。

### 2026-11-01 固定砲台の敵のステートと発射(ユーザー実装、確認OK)
- `Wait`/`Aim`のステート、`CanAttack`、`Shoot`が完成。発射位置は`Barrel`フレームから取る。今は砲身の根元(回転軸)から弾が出る。
- **次**: 砲塔と砲身の回転、銃口の位置まで発射位置を前へ。`first_stage_no`は確認後`0`に戻す。

### 2026-11-01 固定砲台の敵の回転(ユーザー実装、向きの確認OK)
- `UpdateAimAngle`(砲塔`atan2(-x,-z)`、砲身は±50°)と`ApplyFrameRotation`(`MMult(回転, MV1GetFrameBaseLocalMatrix)`→`MV1SetFrameUserLocalMatrix`)。今は一瞬で向く。
- **次**: 少しずつ回す(差を-180〜180°に直す)、発射10F前に止めて`m_aimPos`へ撃つ、発射位置を銃口(`Barrel`の`0,0,-132`)へ。

### 2026-11-01 固定砲台の敵: 回転と予備動作が完成(ユーザー実装、確認OK「めちゃめちゃいい感じ」)
- 砲塔と砲身は1フレーム3度まで、近い方向に回る。発射の`aim_lock_frame`(10F)前に回転を止め、`m_aimPos`(止まる直前のプレイヤー位置)へ撃つ。止まるのが発射の合図で、横に動けば避けられる。
- **残り**: 発射位置を銃口へ、CSVの配置をビルの上へ、`first_stage_no`を`0`に戻す。

### 2026-11-01 2ステージ目は地上のステージにする(ユーザー決定)
- 固定砲台の敵はビルの上に置かない(10/31の仕様「ビルの屋上などに固定」を変更)。地上に置く。
- 後で道路のモデルを探してきて敷き詰め、ステージにする予定。道路の高さが決まったら`Data/CSV/Stage2/TurretEnemy.csv`のYを合わせる(今は仮の400〜700)。

### 2026-11-01 固定砲台の敵が完成(ユーザー実装、確認OK)
- 発射位置を銃口へ: `MV1GetFrameLocalWorldMatrix`(`Barrel`)で`(0,0,-barrel_to_muzzle_distance)`を`VTransform`。
- 残り: `first_stage_no`を`0`に戻す、地上の道路を敷いたらCSVのYを合わせる。プロトの残りはB(移動の加速度2h、岩の当たり判定1h、難易度調整)。
- **(11/01)コスト表「固めの固定砲台の敵」を完了に(ユーザー申告で実3h)**: 死亡 実1h・弾 実2h(内訳はClaudeの割り振り)、親も完了。バックアップはscratchpad。

### 2026-11-04 2面の地面のモデル(道路・線路・空き地、ユーザーがAIで作成)をClaudeが確認
- 置き場所: [Data/Model/Road/](Data/Model/Road/)。6種(`Road_Straight`/`Road_Damaged`/`Road_Debris`/`Rail_Straight`/`Rail_Damaged`/`Ground_Rubble`)、それぞれ`.x`/`.fbx`/`.mv1`+テクスチャ4枚。仕様は[README_License.txt](Data/Model/Road/README_License.txt)(原点=手前の端の幅の中央・地面の高さ、Y-up・正面-Z、1枚の長さはZ方向に2000、幅は道路2200・線路1000・空き地1000)。
- `.x`を読んで確かめた結果: 三角形の数・幅・高さ・長さはREADMEと一致。フレームは1つで単位行列、マテリアル1つ、インデックスの壊れなし。**手前(Z=0)と奥(Z=2000)の断面の頂点(X・Y・U)は6種とも完全に一致**し、V(1→0)もくり返せる → Zを2000ずつずらせば継ぎ目なくつながる。テクスチャは全部2048(発光だけ1024・真っ黒)。
- 気になる点(未対応):
  - **地面の幅が足りない**: fov 90°(縦)・16:9・far 5500なので、地面の高さで見える横幅は奥行きdの所で±1.78d(1000先で±1780、far付近で±約9800)。道路2200＋線路1000＋空き地1000=4200では、少し先から地面の外側(Y=-40までの側面とその外)が見える。案: 外側に安い大きな地面を敷く、両側をビルで埋める、霧で隠す。
  - プレイヤーの高さの下限は`sea_player_margin`(100)で、がれき・焼けた車など(高さ最大約160)を通り抜けて見える場所がある(当たり判定用のモデルは無い)。
  - `.x`に接線が無い(砲台と同じ。砲台は見た目OKだった)。
  - テクスチャが重め(PNG合計で約110MB、展開すると1種あたり約50MB×6)。発光は全部黒なので1枚にまとめるか使わない、法線・金属度を1024に縮める、が候補。
  - [Log.txt](Data/Model/Road/Log.txt)はモデルビューアのログ(モデルとは関係ない)。
- **(11/04)外側の地面`Ground_Side_Right`/`Left`が届いた**(X 0〜±8000、高さ-9〜28、1440三角形、両端の断面一致を確認)。`.mv1`はまだ無い(ユーザーが変換)。
- **(11/04)方針(ユーザー)**: スターフォックスのコーネリアのように、**飛ぶ高さは変えず、ステージの方に高低差を付ける**(レールカメラはやらない)。飛べる範囲は高さ100〜900・左右約±1600。地面は400刻み(-800/-400/0/+400)で上げ下げする想定。坂(道路・線路・空き地のUp400/Down400、テクスチャは直線版と共有)と壁(`Wall_Side`/`Wall_Side_Slope`/`Wall_Front`)をAIに依頼中。
- **(11/04)地面はビルの仕組みで置く**: `Building`(CSVの位置・Y回転・大きさ、当たり判定モデルで球×メッシュ)をそのまま使い、当たり判定のモデルは地面自身。Unityでは種類`Building`で置くので`Building.csv`に書き出される。
- **(11/04・Claude)Unity側の準備(済、バッチで確認済み)**:
  - `StageEditor/Assets/Models/`に地面8種のFBXと、表示用に縮めたAlbedo(1024のjpg)を追加。[StageModelCatalog.cs](StageEditor/Assets/Editor/StageModelCatalog.cs)に登録。**地面のFBXはUnityで読むとY軸180°回った向きになる**ので、見た目だけ`visualRotY`で戻した(CSVには関係しない。ビルは今までどおり)。
  - [GroundPlacementMenu.cs](StageEditor/Assets/Editor/GroundPlacementMenu.cs): 「NovaWing/Ground/Lay Straight Rows」= 親`Ground`の下に、Z -2000〜42000を2000ごと、1列に外側左-2100・空き地-1600・道路0・線路+1600・外側右+2100の5枚(計110枚)。「Add/...」=1枚置く(Zは2000の区切りにそろえる)。「Change Selected To/...」=選んだ地面の種類を変える(幅が違うものには変えない)。敷く高さは`LayY`(今は0)。
  - [PlacedObject.cs](StageEditor/Assets/Scripts/PlacedObject.cs): 地面(Building種で名前が`Building_`以外)は数が多いので、名前の文字は選んでいるときだけ。
  - **注意**: ゲーム側に地面のModelIDを登録する前に`Export Stage CSV`すると、`Building.csv`に知らない名前が入り`col_model_ids.at`で止まる。
  - **プレイヤーの当たり判定(半径50)と地面**: 一番低く(高さ100)飛ぶと球の下は50で、レールの上面(55)・がれきの山(最大約160)に当たる。地面を下げる(`LayY`)か、障害物として扱うかは未決定。
- **(11/04)坂6つ・擁壁3つが届いた(確認済み)**: `Road/Rail/Ground_Rubble_Slope_Up400/Down400`(S字`3t²−2t³`、最大約16.7°、テクスチャは直線版と共有)、`Wall_Side`(16三角形)・`Wall_Side_Slope`(166)・`Wall_Front`(10、X±500・厚さ100)、壁のテクスチャは`Wall_Concrete_*`(1024)。**坂の両端の断面(位置・UV・法線)が直線版と完全に一致**(奥は±400ずらして比較)。置き方の例は[README_License.txt](Data/Model/Road/README_License.txt)の「置き方」(壁は段差の低い側、高い側の地面の端から50外に中心。道路の`Wall_Front`は1.1倍を2枚)。
  - Unity: 9つとも`StageModelCatalog`に登録(同じく見た目だけ180°)、`GroundPlacementMenu`に「Add/Slope」「Add/Wall」「Change Selected To/Slope」を追加。Addは高さを400の刻みにそろえる。バッチで向き・高さを確認済み。
  - (11/04 10:57)17種すべて`.mv1`に変換済み(ユーザー)。ゲームでの読み込みは未確認。
- **(11/04・Claude、ユーザー依頼)2面の配置(地形＋今いる敵)をUnityで作って書き出した**(Unityのバッチで組み立て→`Stage2.unity`を保存→Export。組み立てに使った一時スクリプトは削除済み、写しと元のCSV・シーンのバックアップはscratchpad)。Unityでゲームに近いカメラで撮って見た目を確認済み。**ゲームでは未確認**。
  - 地形(Z): -2000〜8000 平ら(2000に`Road_Debris`、4000に`Road_Damaged`、6000に`Rail_Damaged`) / **8000〜18000 道路が+400に上がり(8000で上り、16000で下り)、線路は-400の切り通し(10000で下り、16000で上り)** / 17500 空き地に崩れたビル(高さ約1300、障害物)、19000 道路に崩れた橋(高さ約470、障害物) / **20000〜22000 空き地だけ+400の台(手前と奥に`Wall_Front`)** / **22000〜32000 真ん中の帯(空き地・道路・線路)が-800の谷(22000・24000で下り、28000・30000で上り、両側に`Wall_Side`を2段)** / 32000〜 平ら(ボスの場所)。壁は[README](Data/Model/Road/README_License.txt)の置き方どおり(低い側、高い側の端から50外)。`Building.csv`は地面・壁147＋ビル20=167行。
  - ビル: 両側の外側の地面(X±2900〜3600、大きさ10)に14、遠く(X±6000〜6500、大きさ12〜14)に4。最初からあった`Building_AntennaTower`(0,0,8260)は道路の真ん中だったので消し、Z=24000の左に置き直した。
  - 敵: 浮遊敵31(道路が+400の区間はY650以上)、ワーム4(動き出すのは3500手前)、**固定砲台10(地面の上に置いた: 歩道Y=20/+420/-780、線路のバラスト-372/-772)**、ボスは今のBoss.csvと同じ。**突進する敵**はCSVを直接直した: Yが-200の3体(地面に埋まるおそれ)を100〜200に、22500と27500に1体ずつ追加(計14)。
  - Unity: `PlacedObject.Kind`に`TurretEnemy`を末尾に追加(Export Stage CSVで`TurretEnemy.csv`も書き出す、Unityでは仮のオレンジの箱)、「NovaWing/Add/Turret Enemy」。突進する敵はまだUnityで扱えない(CSVを直接編集)。
  - 注意: Unityの`Lay Straight Rows`をもう一度実行すると、親`Ground`の下(地面・坂・壁)が全部まっすぐに敷き直される。
- **(11/04・ユーザー指摘→作り直し)プレイヤーが通る帯は平らにする**: 道路が+400に上がる区間でプレイヤーが道路に埋まってダメージを受けるのはよくない(ユーザー)。**通る帯は平ら、高低差はその外側**。通る帯の上に「くぐれる・上を通れる高台」(陸橋など)はあってよく、その上に砲台を置きたい(→新しいモデルをAIに依頼する)。**2面の敵は2面用の2種(突進・固定砲台)だけ**(浮遊敵・ワームは2面から外した。ボスは残す)。
  - 1列9枚: 外側左(-4100)・空き地L2(-3600)・空き地L1(-2600)・**[通る帯] 線路(-1600)・道路(0)・線路(+1600)**・線路R1(+2600)・空き地R2(+3600)・外側右(+4100)。**地面の高さは全部-10が基準**(プレイヤーの当たり判定の下の端は50、線路の上面55→45・道路20→10で、一番低く飛んでも当たらない)。通る帯には出っ張りの高い空き地・壊れた地面を使わない。
  - 高低差(基準からの高さ): 左のL2は10000〜18000で+400→+800の段、24000〜30000で+400→+800。L1は12000〜18000と24000〜28000で+400。右のR1は**高架の線路**(10000〜18000で+800)、**22000〜32000はR1・R2が-800の谷**。高さの変化はすべて坂。
  - [GroundPlacementMenu.cs](StageEditor/Assets/Editor/GroundPlacementMenu.cs): `Lay Straight Rows`を上の9列・高さ-10に変更。**「NovaWing/Ground/Rebuild Walls」を追加**: 同じZの列で隣り合う地面の高さの違いから、横の段差に`Wall_Side`/`Wall_Side_Slope`(下りは180°)を置き直す(低い側、高い側の端から50外、足りなければ下に重ねる)。手前と奥の段差(`Wall_Front`)は置かない。今の配置の壁64個はこれで置いた。
  - 固定砲台10: 通る帯の歩道3(Y=10)、左の高台5(+400はY=390、+800はY=790。空き地は継ぎ目付近が平らなのでZは継ぎ目+100)、右の高架の線路2(Y=818)。突進する敵14は前のまま。ビルは外側の地面の上(X±4900〜5600、遠くは±8000〜8500)、通る帯の障害物は崩れた橋(19000)と崩れたビル(-1700, 21000)。
  - `Building.csv`は282行(地面と壁262＋ビル20)。`FloatingEnemy.csv`・`WormEnemy.csv`は見出しだけにした。前の配置のバックアップはscratchpad(`Stage2_backup2`)。
- **(11/04・ユーザー「物理的にあり得ない街の作りはしない」→作り直し、今の配置)掘割(切り通し)を通る幹線道路**: 直前の配置には、線路の坂(最大約17°、鉄道は数%しか上れない)、線路の上に建つ崩れたビル、上に橋がないのに落ちている橋の残骸があったので直した。
  - 通る帯(線路-1600・道路0・線路+1600)は**掘割の底で平ら(-10)**。**両側の街は+400**(外側の地面も+400=Y390)、帯の両側は擁壁。線路は帯の2本だけで坂にしない。高低差は空き地(土・がれき)だけ: 左L2に+800の丘が2つ(10000〜18000、24000〜30000)、右R2に+800の丘(4000〜10000)、**右R1・R2は22000〜34000で-800まで陥没した谷**。
  - 崩れたビルと崩れた橋の残骸は帯から外した(残骸は、崩れた陸橋のモデルが届いたらその真下に置く)。ビル18は両側の街の上(Y390)。
  - 固定砲台10: 掘割の底の歩道3(Y10)、掘割のふち4(Y390)、丘の上3(Y790)。突進する敵14はそのまま。
  - `Building.csv`は地面と壁280(壁82、Rebuild Wallsで自動)＋ビル18。バックアップはscratchpad(`Stage2_backup3`)、組み立てたスクリプトの写しは`TempBuildStage2_v3.cs.txt`。
  - 陸橋(上面+400)は両側の街をつなぐ形になる。**置けるのは両側が+400の所**(右の谷22000〜34000、丘の区間は避ける)。
  - **(11/04・ユーザー「ビルはもっと置いていい、見栄えが変わらないのが嫌」)ビルを18→83に**: 両側の街(Y390)に3列ずつ。掘割に近い列X±5400(大きさ10、Z2800おき)、その奥±7900(大きさ10、2800おき、半分ずらす)、遠く±10600(大きさ12、3400おき)。種類(アンテナは少なめ)・向き(90°ずつ)・Zのずれ(±200)は乱数(種2)。列どうし・掘割のふち(±4100)・外側の地面の端(±12100)と重ならない間隔。崩れた橋の残骸は使っていない。`Building.csv`は地面と壁280＋ビル83=363行。置き直す前のCSV・シーンはscratchpad。**ゲームでの重さが心配**(当たり判定は363個を毎フレーム調べる)。
- **(11/04・ユーザー「通る帯にもビルを置いていい、ただし必ず避けられるように、敵との相性も考えて」)通る帯のビル8棟と、敵の組み直し**:
  - 区間ごとに「ビルを避ける区間」と「開けた所で突進する敵が来る区間」を交互にした。突進する敵はプレイヤーの2000〜2800先から突っ込むので、出てから突っ込み終わるまでがビルのない区間に収まるよう、出る位置を組み直した。
  - 0〜5500 開けた所(突進2) / **5500〜9500 左右交互**(TallBroken 大きさ7 X-1220〜22 @6500、OfficeGutted 8 X334〜1249 @8800) / 9500〜13000 開けた所(突進3) / **13000〜17500 門→正面**(Collapsed 7 を X-1710〜-763 と 763〜1710 @14000 で真ん中1526が空く、Leaning 7 X-577〜469 @16200) / 17200〜21000 開けた所(突進3) / **21000〜25200 ずらして3棟**(AntennaTower 6 X-1336〜-636 @21800、TallBroken 7(180°) X-122〜1120 @23200、OfficeGutted 7 X-908〜-107 @24600) / 25300〜29500 開けた所(突進4＋歩道の砲台) / 29500〜 ボス戦なので何も置かない。
  - 帯のビルは回転0か180だけ(90°はDxLibとUnityで向きが逆になるかもしれず、すき間の計算が合わなくなるため)。どの所でもすき間600以上、続くビルの間は横に移る時間がある間隔。Unityでは名前が`Path_`で始まる。
  - 固定砲台10を置き直した(ビルの陰にならない所): ビルの区間はふち・丘の上から撃つ(4100左ふち・6100右丘・8100右ふち / 14100左丘・16100右ふち / 24100左ふち)、歩道は帯のビルとぶつからない12500・18500・27000、丘26100。
  - [RushEnemy.csv](Data/CSV/Stage2/RushEnemy.csv)は13体(出る位置1500〜27600、frontOffset 1900〜2800)。
  - 注意: 今は弾とビルの当たり判定がないので、砲台の弾はビルを突き抜ける(ビルの陰に隠れても防げない)。
  - `Building.csv`は地面と壁280＋ビル91=371行。置く前のバックアップはscratchpad(`Stage2_backup4`)。
- **(11/04)帯の上の構造物3種が届いた(確認済み)**: `Overpass_Cross`(816三角形、橋げたX±2100・Z0〜600・上面400・下面340、柱X±1950、くぐる=球の中心≤290、越える≥450/手すり越え486)、`Overpass_Cross_Broken`(1340、左の柱の上で折れ、残りが右の柱を支点に4.94°傾いて左端が地面に着く。くぐれるのは右側だけ(X≥1056で下面250以上)、左の柱の上に残った端の上面Y=400に砲台を置ける)、`Sign_Gantry`(1044、脚X±1000、梁の下面440・足場の床520、くぐる≤390、越える≥570/606)。どれも閉じた立体。テクスチャは`Overpass_*`(2つで共有)と`Sign_Gantry_*`、1024。`.mv1`はまだ無い。
  - Unity: 3つを`StageModelCatalog`に登録(見た目だけ180°)、「NovaWing/Ground/Add/Structure/...」。**壁の自動配置が構造物を地面と数えていたので、地面(Road_/Rail_/Ground_)だけを数えるように直した**。
  - **Unityのシーンには置いた(CSVはまだ書き出していない。ゲーム側に登録する前に書き出すと`Building.csv`の知らない名前で止まるため)**: 標識柱 Z4300、陸橋 Z12000、崩れた陸橋 Z19300(陸橋は両端が+400の所だけ。右の谷が20000から始まるのでその手前)。砲台: 標識柱の足場(0,510,4450)、陸橋の上(0,390,12300)、崩れた陸橋の左の残った端(-1975,390,19600)。陸橋の下になる歩道の砲台(12500)は消した→砲台12。撮影で向き・位置を確認済み。
  - [RushEnemy.csv](Data/CSV/Stage2/RushEnemy.csv)は12体に(構造物と重ならない時刻に: 300・1800 / 9600・10100 / 16900×2・17400 / 25400×2・26400・26900・27600)。これは書き出し済み。
  - **(11/04完了)ユーザーが`.mv1`に変換、登録はClaude(ユーザー依頼)**: `ModelID`の`OverpassCross`/`OverpassCrossBroken`/`SignGantry`、パス(`Data/Model/Road/○○.mv1`)・CSV名・ResourceLoader.cppの3つの表・`col_model_ids`(自分→自分)。Debug x64ビルド成功(エラー・警告なし)。Unityから書き出し済み(`Building.csv`に構造物3つ、`TurretEnemy.csv`は12体)。**ゲームでの確認はまだ**。
- **(11/04・ユーザー提起)プレイヤーが絶対に当たらない場所のビルには当たり判定はいらない**: 今は毎フレーム`Building.csv`の全部(371個)に`MV1CollCheck_Sphere`と`MV1CollCheck_LineDim`を呼んでいる(742回/フレーム)。プレイヤーが届く範囲は X±1600・高さ100〜900(宙返りは始めた高さから上に約1020まで、下と横には出ない)に半径50を足した所。今の配置で届くのは帯のビル8棟と構造物3つだけ(帯の地面は上面45以下で当たらない、擁壁は|X|≥2000、街のビルは|X|≥4200)。案: ビルの当たり判定モデルの世界での範囲(AABB)を作るときに1回求め、届く範囲と重ならなければ当たり判定を作らない・登録しない(＋毎フレームAABBとプレイヤーの球を先に比べる)。未実装。
- **(11/04・ユーザー報告)ビルの当たり判定が見た目より大きく、当たっていないように見えるのに当たる → 作り直し中(Claude)**:
  - 原因: [BuildingCollisionTool.cs](StageEditor/Assets/Editor/BuildingCollisionTool.cs)の自動生成が「高さ10段、段ごとに全部を囲む箱1つ、大きさが近い段は位置がずれていてもまとめる」作りで、L字・飛び出し・傾きの分だけ大きかった(傾いたビルは箱4つ)。
  - 直し方: `BuildAutoBoxes`を**ボクセル方式**に書き換えた(表面を8(ファイルの数値)のマスで拾う→段ごとに外から届かない中身を埋める→段ごとに長方形にまとめ、上下で同じ長方形をつなげて箱に)。ゲームのコードは変えない(`Data/Model/Building/Building_○○_Col.x`を作り直すだけ)。
  - 元の当たり判定(`_Col.x`と`Assets/Models/*_Col.fbx`)のバックアップはscratchpadの`col_backup`。
  - **(12:32)Claudeが①を実行済み**: 箱の数 AntennaTower 98・BridgeWreck 21・Collapsed 81・Leaning 103・OfficeGutted 24・TallBroken 191(前は2〜9個)。`_Col.x`は作り直し済み、**ゲームでの確認はまだ**(②から)。
  - **途中で止まったときの手順(ユーザー)**: ①Unityで StageEditor を開き、メニュー「NovaWing/Building Collision/Auto Generate All」を実行(Consoleに「箱○個で書き出しました」が6つ出る)。②ゲームを起動し、F1のデバッグ表示で当たり判定(赤いワイヤーフレーム)がビルの形に沿っているか、帯のビルの近くを飛んで当たり方を確かめる。③箱が多すぎて重い・すき間から抜けるなどあれば、`AutoVoxelSize`(今は8)を大きく(粗く)／小さく(細かく)して①をやり直す。④元に戻すときは`col_backup`の`_Col.x`を`Data/Model/Building/`へ戻す。
- **(11/04完了・Claude、ユーザー依頼)地面17種の登録**: `ModelID`(`RoadStraight`〜`WallFront`、[ResourceLoader.h](Manager/ResourceLoader.h)のビルの当たり判定の下)、[ResourceConstants.h](Constants/ResourceConstants.h)のパス(`Data/Model/Road/○○.mv1`)とCSV名(Unityと同じ`Road_Straight`など)、[ResourceLoader.cpp](Manager/ResourceLoader.cpp)のパスの対応表・`game.models`・`WStringToModelID`、[BuildingDataSetter.cpp](Game/GameObjects/Actors/Building/BuildingDataSetter.cpp)の`col_model_ids`に「自分→自分」。Debug x64ビルド成功(エラー・警告なし)。**ゲームでの確認はまだ**(`first_stage_no`は1のままなので、タイトルから2面が始まる)。
- (以下は登録前に書いたメモ)**次(ユーザーが書く、ビルの登録と同じ場所)**: `ModelID`に地面8種(＋坂6・壁3)([ResourceLoader.h:34](Manager/ResourceLoader.h#L34)付近)、パス([ResourceConstants.h:25](Constants/ResourceConstants.h#L25)付近、`Data/Model/Road/○○.mv1`)、CSV名([ResourceConstants.h:183](Constants/ResourceConstants.h#L183)付近、Unityと同じ`Road_Straight`など)、[ResourceLoader.cpp](Manager/ResourceLoader.cpp)の対応表(130行目付近)・`game.models`(474行目付近)・`WStringToModelID`(690行目付近)、[BuildingDataSetter.cpp:23](Game/GameObjects/Actors/Building/BuildingDataSetter.cpp#L23)の`col_model_ids`に「地面→地面自身」。`Ground_Side_*`の`.mv1`変換も。

### 2026-11-04 ブーストのポストエフェクト`BoostingEffect`(ユーザー実装中)
- [BoostingEffect.cpp](ShaderEffect/BoostingEffect.cpp)はユーザーが自分で考えながら書いている途中。**Claudeは方法だけを伝える方針**(ユーザーが聞いたことに答えるだけで、コードの指摘や答えは先回りして出さない)。
- **(11/04)参考記事: CSDN「径向模糊shader」(思依_xuni、2016-12-23、https://blog.csdn.net/mansir123/article/details/53840446)の日本語訳(Claudeが訳した。CC 4.0 BY-SA)。** 放射ブラー(ズームブラー)の考え方。**コードは読んで自分の言葉で設計する(写さない)。** 原文の「原理」の図は取れていない。
  - 要点: ①`dir = 0.5 - uv`(画面中心を向くベクトル)、`dist = length(dir)`、`dir /= dist`で正規化 ②元の色`color = tex.Sample(uv)` ③`samples[10]`(-0.08〜0.08の10個のずれ)で、`uv + dir * samples[i] * fSampleDist`を10回サンプリングして`sum`に足す ④`sum /= 11`(元の色を入れて11個) ⑤`t = saturate(dist * fSampleStrength)`(中心から遠いほど強く) ⑥`lerp(color, sum, t)`。パラメータは`fSampleDist`(サンプリング距離)と`fSampleStrength`(強さ)の2つ。動的に変えるとスピード感が出る。
  - 原文HLSL(コメントのみ訳): `float2 dir = 0.5 - texCoord; float dist = length(dir); dir /= dist; float4 color = tex2D(Texture0, texCoord); float4 sum = color; for (int i = 0; i < 10; ++i) { sum += tex2D(Texture0, texCoord + dir * samples[i] * fSampleDist); } sum /= 11.0; float t = saturate(dist * fSampleStrength); return lerp(color, sum, t);`
  - 気づき(Claudeから): `samples[i]`と`dist`はぼける長さに別々の形で関わる。符号(中心を向くか外を向くか)の扱いが「中心側のピクセルが外ににじむ」設計に関係する。
- **(11/04)ブーストエフェクトの設計(ユーザーの発想)**: 中心付近は何もかけず、画面の端ほど中心側のピクセルが外側へにじむ。= 放射ブラー。ファイルは`ShaderEffect/`に移動済み([BoostingEffect.h](ShaderEffect/BoostingEffect.h)・[BoostingEffectPS.hlsl](BoostingEffectPS.hlsl)は今はビネットのみ)。
- **(11/04・方針)「一回で全部やろうとしない」**: 「まずはこれ→ダメならこれ→できたらこれ」と1歩ずつ進める(AIがない時代の作り方をAIありで)。全体の計画・機能一覧を一度に出さない。
- **(11/04・ユーザー実装、確認OK)ブースト中だけ描き先を切り替える**: `BoostingEffect`に`ChangeBoostEffectRT`/`ChangeNormalRT`(`m_screenH`はコンストラクタで`MakeScreen`)。[GameScene::Draw](Scene/GameScene.cpp#L667)で、`SetUpCamera`の前と`m_pEffectManager->Draw()`の後(UIの前)に呼ぶ。ブースト中はUIだけが映れば成功(3Dは`m_screenH`の中)。`ChangeNormalRT`の`ClearDrawScreen`は不要(バックバッファは`Application.cpp:97`で毎フレーム先頭にクリア済み)。
- **次の1歩**: `m_screenH`を画面に出す(まずシェーダー無しで`DrawGraph(0,0,...)`)→ブースト中も普通に映る。その後、放射ブラーのシェーダー(上の参考記事)。

### 2026-11-05 2面のゲーム内確認OK、ブーストのポストエフェクトを再開
- **(11/05・ユーザー確認)2面は問題なし**: 地面17種・構造物3種・ビルの当たり判定(ボクセル方式)・2面の配置。コスト表「2面の配置」(5h・アルファ)を完了に(ユーザー申告で実5h)。親「ステージ」は未完了の子があるので未完了のまま。バックアップはscratchpad。更新後: プロト 81/残3、アルファ 78/残63、ベータ 45.5/36.5、合計204.5/残102.5。
- ブーストのポストエフェクト([BoostingEffect.cpp](ShaderEffect/BoostingEffect.cpp))を再開。方針は11/04のまま(Claudeは聞かれたことに答えるだけ、先回りの指摘・答え・全体計画は出さない、1歩ずつ)。
- **(11/05・ユーザー実装、確認OK)ブースト中に`m_screenH`を画面に出す**: `BoostingEffect::Update`→`Draw(isBoosting)`に改名(中身が描画だけなので。Updateで描くと、このフレームの3Dが`m_screenH`に描かれる前になる)。[GameScene.cpp:720](Scene/GameScene.cpp#L720)で`ChangeNormalRT`の後・UIの前に呼ぶ。`DrawGraph`の座標を`(0,0)`に直して、ブースト中に画面が映った。
- **(11/05・ユーザー実装、確認OK)ブーストしていないときも画面が映るように**: 3Dは常に`m_screenH`に描かれる(切り替えに条件なし)ので、ブーストしていないときも`DrawGraph(0,0,m_screenH)`で出す。エフェクトがつくのはシェーダーを通して描いたときだけなので、普通に描けばいつもの画面。今は両方の分岐が同じ`DrawGraph`(ブースト中の方をシェーダーに置き換える予定)。
- **次**: 放射ブラーのシェーダー。ユーザーが先に設計(PS_INPUT・テクスチャ・計算方法)を考え、Claudeはレビューだけ。
- **(11/05)ブースト中にシェーダー(今はビネット)がかからない → `m_boostingEffectPSH`が-1だった**: `BoostingEffectPS.hlsl`がvcxprojに登録されておらず、`.pso`が作られていなかった。Claudeが[NovaWing.vcxproj](NovaWing.vcxproj)(ほかのPSと同じ設定、`$(ProjectDir)%(Filename).pso`)と`.filters`に登録。さらにファイルが**UTF-8のBOM付き**で、fxcが`error X3000: Illegal character in shader file`(1行1文字目)になったので、BOMだけ外した(ほかの.hlslはBOM無しUTF-8)。Debug x64ビルド成功、`BoostingEffectPS.pso`生成を確認。ゲームでの確認はまだ。
- **(11/05)再発防止: [Directory.Build.targets](Directory.Build.targets)を追加**(MSBuildがビルドのたびに自動で読む。vcxprojは変えていない。消せば元どおり)。①直下の`*PS.hlsl`/`*VS.hlsl`でvcxprojに未登録のものも、ほかと同じ設定(5.0、`.pso`/`.vso`は直下)でコンパイル ②直下の`.hlsl`/`.hlsli`がBOM付きならビルド前にBOMだけ外す。BOMの原因: [.vscode/settings.json](.vscode/settings.json)の`files.encoding: utf8bom`と、dxlib-devenvの「新しいシェーダーを作成」がわざとBOMを付けること。未登録・BOM付きの仮のシェーダーで、BOMが外れて`.pso`ができることを確認済み(仮のファイルは削除)。
- **(11/05・ユーザー実装、確認OK)ブースト中にビネットがかかった(画面の端が暗くなる)**: `.pso`が読めても`DrawGraph`では自作PSが効かない(教訓どおり) → ブースト中は`DrawGraphToShaderByCenter`([GraphShaderDraw.cpp](Utility/GraphShaderDraw.cpp)、中で`DrawPolygonIndexed2DToShader`、テクスチャは`t0`)で描く。あわせて、ブーストしていないときの描き方と使い終わったシェーダーの解除を見直し、[BoostingEffectPS.hlsl](BoostingEffectPS.hlsl)の`PS_INPUT`をDxLibの頂点出力の並び(`COLOR1`を含む、[GlitchPS.hlsl](GlitchPS.hlsl)と同じ)に合わせた。
- **次**: 放射ブラー。ユーザーが設計を先に考え、Claudeはレビューだけ。
- **(11/05・ユーザー実装、確認OK「いい感じ」)ブースト中の放射ブラー(にじみ)**: 参考はスターフォックスのリメイクのブースト画面(端ほど、中心へ向かう筋が長くなめらかに伸びる)。参考記事の形(前後に10回サンプリング＋距離で混ぜる割合)だと「同じ幅のぼけが端ほど濃く重なる」だけで、イメージと違った。直した点: ①`samples`を全部プラスにして**中心側からだけ**色を借りる(10個とも違う値) ②ずらす量に`toCenterDist`を掛けて**端ほど長く**伸ばす(`float2`の`uv-0.5`を掛けるとx・y別々に掛かり向きが外側に戻るので、長さのスカラーを使う) ③`sample_strength`を2.0に(混ぜる割合が上下左右の端で1に届く)。[BoostingEffectPS.hlsl:41](BoostingEffectPS.hlsl#L41)。
  - 残り(必要なら): `samples`の間隔がそろっていない(大きい値の間が空く)ので、筋をもっと長くするとコピーが並んで見えるおそれ。筋の長さの倍率は今は無い(四隅で最大約0.067)。
- **(11/05・ユーザー実装)ブーストのにじみをスーっと出し入れする**: `BoostingEffect`にブースト量(0〜1)を持たせ、定数バッファでシェーダーに渡してずらす量に掛ける。シェーダーを通す条件は`isBoosting`ではなく「ブースト量が0より大きい間」。レビューで指摘した点: C++のスロット(`ShaderRegister::cbuffer_boosting`=8)とシェーダーの`register(b0)`の不一致、`boostAmount`未使用、増え方0.0001/フレームは遅すぎ。
- **(11/05・ユーザー修正、確認OK)ブースト以外のエフェクト・レティクルの位置がずれた**: 原因は[GameScene.cpp:717](Scene/GameScene.cpp#L717)の`ChangeNormalRT`(`SetDrawScreen`)でカメラ設定がリセットされ、その後のUI(レティクルの3D→画面変換)と次フレームの`Effekseer_Sync3DSetting`([EffectManager.cpp:18](Manager/EffectManager.cpp#L18))・ロックオンの画面内判定がリセット後のカメラを使っていたこと(教訓[NOTES.md:27](NOTES.md#L27)どおり)。描き先を戻した後に`SetUpCamera`し直して解決。
- **(11/05・仕様合意)蝶みたいな敵(仮名`ButterflyEnemy`、2面、コスト表 計7h・S・アルファ)**: 仕様はユーザーがClaude Desktopでまとめたもの。数値はすべて仮で、単位も気にせず後で調整(ユーザー)。
  - 小型の戦闘機、V字の翼でオレンジ〜金に発光。1〜3機で出る。出し方は`EnemySpawner`(プレイヤーが決めたZまで来たら)で、**画面外から**出てくる。1〜3機はCSVに1機ずつ。
  - かくかく移動: 登場(プレイヤー前方の所定の距離まで一気に)→ダッシュ(0.12〜0.2秒、出だし速く最後に急ブレーキ、進行方向へ最大45°傾く)→停止(0.4〜0.8秒ランダム、機首をプレイヤーへ、傾きはすぐ水平、攻撃は止まっている間だけ・毎回ではない)をくり返す→離脱(一定時間か決まった回数で画面外へ高速で抜けて消える)。プレイヤーの前方30〜50m相当を保ってついてくる。次の移動先は上下左右に3〜8m相当、同じ方向に何度も続けない、20%くらいで前後にも動いて詰め寄る。
  - 回転弾: 撃つ直前に機体の前が0.2〜0.3秒白く光る。3発を等間隔に、見えない軸のまわりを回しながら撃つ。軸は撃った瞬間のプレイヤー位置へまっすぐ(追尾なし〜ごく弱い)、1秒に1〜1.5周、半径は0.5m相当から4〜6m相当へ広がる、回る向きは撃つたびに交互。弾頭は白く光る小さな球、虹色(白→シアン→紫→ピンク)の太い尾が0.3〜0.5秒で消える。当たり判定は弾頭だけ。プレイヤーの後ろへ抜けるか3〜4秒で消える。
  - **カウンターで跳ね返せる。回転はそのまま、軸の進む向きだけ蝶の方へ変わる**(ユーザー)。
  - 仮の値(ユーザー未回答、Claudeの案): 離脱は15秒かダッシュ8回、止まったときに撃つ確率50%、HP・弾のダメージは浮遊敵と同じ、体当たりダメージ無し(固定砲台と同じ`ColliderTag::Floating`)。
  - 見た目: モデルはユーザーがClaude Desktopで作る(プロンプトはClaudeが書いた)。虹色の尾・白い光はEffekseer(エフェクト=ベータの定義、扱いは未決定)。コストは見積なので気にしない(ユーザー)。
  - **(11/05・Claude、ユーザー依頼)フォルダとクラスの初期状態だけ作成**: [ButterflyEnemy.h](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.h)/[.cpp](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp)(`EnemyBase`継承、コンストラクタ・デストラクタだけ、「クラスの追加」のスクリプトで作成しvcxproj登録済み)。中身はユーザーが書く。`EnemyBase()`の引数を書くまではビルドエラー。モデル・ステータスの登録は本物のモデルが届いてから(それまで`ModelID::FloatingEnemy`、ステータスはモデルIDから引くので浮遊敵と同じになる)。最初の目標は「2面で出てきて浮いているだけ」(クラス→`EnemyType`と工場の`case`→`EnemySpawner`の`LoadButterflyEnemy`)。
  - **(11/05・ユーザー実装中)クラスの中身を書き始めた**: `ButterflyEnemyData`(突進する敵と同じ中身)、コンストラクタ(`EnemyBase`に引数、当たり判定の生成、待機位置のずれ、画面外の初期位置)、`OnInit`まで。`Update`/`Draw`/`GetCollisionSpheres`/`GetColliders`はまだ空。
    - **レビューで指摘(ユーザーが後で変える)**: [ButterflyEnemy.cpp:22](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L22)の`ColliderTag::ButterflyEnemy`(ユーザーが[ICollider.h:28](Game/Collision/ICollider.h#L28)に追加)は[CollisionManager.cpp:44-69](Manager/CollisionManager.cpp#L44-L69)の`hit_pairs`にないので弾が素通りする。直し方は`ColliderTag::Floating`にする(固定砲台[TurretEnemy.cpp:53](Game/GameObjects/Actors/Character/Enemy/TurretEnemy/TurretEnemy.cpp#L53)と同じ、体当たりダメージなしなのでこれで足りる)か、`hit_pairs`に`PlayerBullet`/`ChargeBullet`/`ChargeExplosion`との組を足すか。
    - 今は`Update`が空なので画面外(`appear_distance`)に置かれたままでプレイヤーにもついてこない → 次は`UpdateAppear`([RushEnemy.cpp:161](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.cpp#L161)と同じ形)と`Draw`([RushEnemy.cpp:122](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.cpp#L122)と同じ形)。
- **(11/05)ブーストのエフェクトはひとまず完了(ユーザー)**。コスト表「シェーダ＞ブースト中の集中線」(見積6h)を実3hで完了に。バックアップはscratchpad。更新後: プロト 81/残3、アルファ 78/残57、ベータ 45.5/36.5、合計204.5/残96.5。
- **(11/05・Claude、ユーザー依頼)VSCodeでHLSLのエラーが出ない → 自作拡張`local.hlsl-check`を入れた**: HLSL Toolsは未宣言の変数(例: `samp`)などを検出しない。書いている途中(止まって0.4秒後)・開いた時・保存時に、Windows Kitsの`fxc.exe`で一時ファイル(BOMを外した写し)をコンパイルし、エラーを赤線と問題パネルに出す。種類はファイル名の末尾(`〇〇PS`→ps_5_0、`〇〇VS`→vs_5_0)、`.hlsli`を変えると開いているシェーダーを再チェック。ソースと`.vsix`は`C:\Users\SakamotoKou\.claude\local-extensions\hlsl-check\`(学校PCには`code --install-extension`でこの`.vsix`を入れる)。入れた後に「ウィンドウの再読み込み」が必要。
- **(11/05・ユーザー)環境まわり(vcxproj・pso・文字コード)はユーザーが確認する必要はない** → Claudeが調べて直す。聞き返し・1歩ずつの方針はコード・シェーダーの中身だけ。
- **(11/05・Claude、ユーザー依頼)VSCodeに「ソリューション エクスプローラー」拡張を自作**(Visual Studioの使い心地に寄せる): 左のバーにツリー、ドラッグ&ドロップ・切り取り貼り付け・「最上位フォルダへ移動」(VSCode標準だと最上位に置けない問題の解決)、追加(クラス・HLSL・フォルダー)、名前の変更・削除、ビルド/リビルド/クリーン(MSBuild、ステータスバーで構成切り替え、Ctrl+Shift+B)。追加・移動・改名・削除のたびに**vcxprojと.filtersへ自動反映**(BOM・CRLFは維持)、未登録のソースには「(未登録)」表示と「プロジェクトに含める」。「クラスの追加」はダイアログでクラス名と継承元を同時に入力(継承元の.hは自動検索して`#include`を書く)、「新しいHLSL」は種類(3Dモデル用PS・画面全体PS・3Dモデル用VS・スキンメッシュVS・hlsli)ごとの雛形。
  - **家と学校の両方に入れる**: ソースは[Tools/SolutionExplorer/](Tools/SolutionExplorer/)(gitで同期)。もう一方のPCでpullしたあと[install.ps1](Tools/SolutionExplorer/install.ps1)を実行(`powershell -ExecutionPolicy Bypass -File Tools/SolutionExplorer/install.ps1`)→VSCodeで「Developer: Reload Window」。ソースを直したらこのフォルダを直して再実行。
  - 画面での動作(ダイアログ・ドラッグ・ビルド)はまだ実機確認していない。
  - **(11/11・Claude)学校PC(FIC-ADMIN)にも入れた**: `install.ps1`を実行 → `code --list-extensions`に`novawing.solution-explorer`が出ることを確認。学校PCには`local.hlsl-check`・`local.vcxproj-sync`・Visual Studio Keymap・ユーザーのkeybindings(Ctrl+Shift+Alt+C/D/R、Ctrl+Break、Ctrl+Shift+F10)は前から入っていた。ソリューション エクスプローラーは`fs.renameSync`で動かすので、`vcxproj-sync`(`onDidRenameFiles`で反応)と二重にvcxprojを書き換えることはない。VSCodeで「Developer: Reload Window」をすると左のバーに出る。

### 2026-11-05〜06 蝶の敵: Draw(ユーザー実装)と、次の手順(学校で続き)
- **(ユーザー実装・Claudeレビュー済み)[ButterflyEnemy::Draw](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L58)と`DrawEnemy`を書いた**([RushEnemy::Draw](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.cpp#L122)と同じ形)。`model_scale`は仮の2倍。`DrawEnemy`のテクスチャは**突進敵のものを仮で使用中**(蝶のに差し替える)。`appear_distance`(今6000)は確認のとき一時的に0にすると、蝶が最初から登場位置に居る。
- **調べて分かったこと(Claude)**:
  - [EnemyFactory.cpp:141](Game/GameObjects/Actors/Character/Enemy/EnemyFactory.cpp#L141)の`ButterflyEnemy`の分岐は完成済み。
  - **生成の入口が無い**: [EnemySpawner::LoadRushEnemy](Manager/EnemySpawner.cpp#L29)は突進敵専用(CSV名`RushEnemy`・種類も[58行目](Manager/EnemySpawner.cpp#L58)で固定)。蝶用のCSVは`Data/CSV/Stage2/`に無い。→ 蝶を1体出す方法を決める(Spawnerを一般化するか、蝶用の読み込みを足すか)。
  - **モデルの登録が無い(11/06にモデルは届いた)**: `ModelID::ButterflyEnemy`は[ResourceLoader.h](Manager/ResourceLoader.h)にあるだけで、`ResourceLoader.cpp`(パスの対応表・`game.models`・`WStringToModelID`)と`ResourceConstants.h`に無い。モデルは[Data/Model/ButterflyEnemy/](Data/Model/ButterflyEnemy/)(`.fbx`/`.x`＋Albedo/Emission/Metallic/Normalのpng)。**`.mv1`への変換はユーザー(DxLibModelViewerの「名前を付けて保存」)**。変換後、登録とGraphIDの追加はClaudeに頼める(環境まわりなので)。
- **次の手順(1歩ずつ)**:
  1. ユーザー: 蝶の`Update`に「登場」だけを書く(`m_stateTimer`にタイムスケールを足す、`appear_frame`定数、進行度と`easeOut`から`m_pos`。ステートの`enum`はまだ作らない)。
  2. 蝶のモデルを`.mv1`にして、登録(Claude)。
  3. 蝶を1体出す(Spawner/CSV)→ 画面で登場の動きを確認。
  4. 当たり判定: `m_colSphere->Update`、`GetCollisionSpheres`/`GetColliders`、[CollisionManager.cpp:44-69](Manager/CollisionManager.cpp#L44-L69)の`hit_pairs`に`ButterflyEnemy`を足す(今は弾が素通り)。
  5. かくかく移動(ダッシュ・停止・離脱)→ 回転弾 → 死亡 → 逃げる(コスト表の子項目の順)。
- 気づき(後で): 登場の「外側への方向」の計算はコンストラクタと`UpdateAppear`にあり、突進敵とほぼ同じ。2体目でも同じ形なら`EnemyBase`へ寄せる候補。
- 未コミット: `ButterflyEnemy.cpp/.h`の変更、`Data/Model/ButterflyEnemy/`、`Tools/`(11/06時点)。
- **(11/06・Claude、ユーザー依頼)蝶のモデルを登録(手順2完了)**: `.mv1`はユーザーが変換。登録した場所: [ResourceConstants.h](Constants/ResourceConstants.h)(モデル・法線/メタリック/エミッションのパス、CSV名`ButterflyEnemy`)、[ResourceLoader.h](Manager/ResourceLoader.h)の`GraphicID::ButterflyEnemyNormalMap/MetalicMap/EmissionMap`、[ResourceLoader.cpp](Manager/ResourceLoader.cpp)(画像とモデルの対応表、ゲームシーンで読むモデル・画像の一覧、`WStringToModelID`)、[EnemyStatus.csv](Data/CSV/Params/EnemyStatus.csv)に`ButterflyEnemy,5,132,10,0`(浮遊敵と同じ仮の値。これが無いと`EnemyBase`のステータス検索で落ちる)。Debug x64ビルド成功。
  - (ユーザー依頼でClaudeが変更)[ButterflyEnemy::DrawEnemy](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L88)のテクスチャを`RushEnemy〇〇Map`から`ButterflyEnemy〇〇Map`に差し替えた。
- **(11/06・ユーザー実装、確認OK)手順1「登場」**: [ButterflyEnemy::UpdateAppear](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L161)(突進敵と同じ、進行度→`easeOut`→残りの距離)。進行度が1になってもステートは`Appear`のまま(次のステートがまだ無い)なので、毎フレーム「プレイヤー位置＋ずれ」に居てついてくる。`appear_frame`=60。ステートの`enum`は`Appear/Charge/Shoot`で先に作ってある(ユーザー)。
- **(11/06・ユーザー実装、確認OK「でました」)手順3「蝶を1体出す」**: `EnemySpawner::LoadRushEnemy`を一般化して[LoadEnemy](Manager/EnemySpawner.cpp#L27)(csvFolderName, csvName, type, offsetZ)に。[GameScene.cpp:328](Scene/GameScene.cpp#L328)で突進敵と蝶の2回呼ぶ。CSVはClaudeが[Data/CSV/Stage2/ButterflyEnemy.csv](Data/CSV/Stage2/ButterflyEnemy.csv)を作成(1体、Z=800、x=0/y=400、frontOffset=2500、仮)。`appear_distance`は6000に戻した。
- **(11/06)手順4「当たり判定」**: `GetCollisionSpheres`/`GetColliders`はユーザーが突進敵と同じ形で実装。`hit_pairs`への追加はユーザー依頼でClaudeが[CollisionManager.cpp:51-53](Manager/CollisionManager.cpp#L51-L53)に`PlayerBullet`/`ChargeBullet`/`ChargeExplosion`×`ButterflyEnemy`(体当たりダメージ無しなので`Player`との組は無し)。Debug x64ビルド成功。**(11/06・ユーザー確認OK)弾で倒せた。**
- **次**: 手順5 かくかく移動(ダッシュ・停止・離脱)。最初の1歩は「登場→停止→ダッシュ→停止…」のくり返しだけ(傾き・機首の向き・同じ方向を続けない・前後・離脱は後)。
- **(11/06・ユーザー実装、確認OK「大丈夫そう」)停止とダッシュのくり返し**: `State`に`Stop`/`Dash`を追加。[UpdateStop](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L215)は`stop_frame`(固定、ユーザー判断)経ったら出発点`m_dashStartPos`=今の`m_standbyOffset`、行き先`m_dashEndPos`=出発点＋方向×距離(300〜800)を決めてダッシュへ。方向は8方向を**角度で**出す(`DX_TWO_PI_F / dash_dir_num * dirIndex`→`cos`/`sin`、長さは最初から1。最初は`enum`＋`switch`で書いていたのを置き換え)。[UpdateDash](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L244)は`easeOut`で出発点→行き先、`dash_frame`=20。
  - 用語: offset(`m_standbyOffset`)は「ずれ」ではなく「プレイヤーから見た位置」と言う(ユーザー)。
  - 残り(かくかく移動): 画面外へ出ていかない制限、同じ方向を続けない、20%で前後、進行方向へ傾く、止まったら機首をプレイヤーへ、離脱(15秒かダッシュ8回、仮)。
  - **(11/06・ユーザー)位置の計算はプレイヤーのZだけを使う**(`m_pos = (0,0,playerPos.z) + m_standbyOffset`、x・yはプレイヤーについていかない)。
- **(11/06・ユーザー実装、確認OK「いい感じ」)蝶をプレイヤーへ向ける[LookAtDir](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L274)**: 見せたい向きは「翼の模様が多い面(モデルの+Y、V字の内側)をプレイヤーへ、触覚(モデルの-Z側に生えている)を上」(ユーザー)。突進敵の「正面から一番近い回り方で回す」方式だと上の向きを気にしないので、正面を真後ろに向けるときX軸180°で裏返る → 3つの回転に分けた: `standUp`(X軸+90°でモデルを起こす: +Y→+Z、-Z→+Y)、`pitch`(X軸、`-atan2(dir.y, 水平の長さ)`、プラスだと前が下を向くのでマイナス)、`yaw`(Y軸、`atan2(dir.x, dir.z)`)。`m_rotation = yaw * pitch * standUp`(右から先にかかる)。プロジェクトの`Quaternion`は`q*v*q⁻¹`、`ToMatrix4x4`は行ベクトル用で、回転の向きはDxLibの`MGetRotX`等と同じ。
- **(11/06・ユーザー、仕様の変更)攻撃のリズム**: 「1回移動して止まったら弾を2回撃つ → また移動」(本家を見て)。前の仕様の「止まったときに撃つ確率50%」は置き換え。
- **(11/06・ユーザー実装中、続きは次回)離脱**: きっかけは**ダッシュの回数だけ**(時間の条件は必要になったら)。消すときは`OnEnemyDead()`ではなく`OnDead()`(撃破数を増やさない、突進敵が後ろへ抜けたときと同じ)。
  - できているところ: `State::Leave`、`m_dashNum`(ダッシュに入るとき`++`)、`leave_dash_num`=6(仮)、[UpdateStop:227](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L227)で回数を超えたら`Leave`へ。Debug x64ビルドは通る。
  - **残り**: ①`UpdateLeave`を作る(登場と同じ「プレイヤーから見た位置のx・yの向き」へ、毎フレーム プレイヤーから見た位置 += 方向×速さ(仮80)×タイムスケール、位置は`プレイヤーのZ＋プレイヤーから見た位置`、`m_stateTimer`が仮60を超えたら`OnDead()`) ②`switch`に`case State::Leave:` ③条件が`m_dashNum > leave_dash_num`でコメントは「以上」(`>`だと7回ダッシュしてから離脱。どちらにするか決める)。
- **その後**: 回転弾(停止中に2回撃つ)→ 死亡 → かくかく移動の残り(画面外へ出ない制限、同じ方向を続けない、前後、傾き)。
- 11/06のコミットに、ほかのセッションで作ったもの(Effekseerのエフェクトの見直し、`NormalShootState.cpp`、`CLAUDE.md`、`sakurai-knowledge/`、`.claude/skills/`)も入れた。`.claude/settings.local.json`は個人のローカル設定なので入れていない。

### 進捗（2026-11-06・弾を明るい背景でも見えるようにした：加算の芯＋暗い縁/裏打ちの2層）
- 依頼: 敵・プレイヤーの弾が明るい空・海で埋もれる。加算の「重なるほど輝く」は残す。データ(.efkefc/.efk/テクスチャ)だけで対応、C++/HLSLは未変更。3つとも上書きし倍率50で.efkを書き出した(元はgit履歴)。
- テクスチャ生成: [gen_outline_textures.ps1](Data/Effect/EnemyBullet/tools/gen_outline_textures.ps1)(リポジトリ直下で実行、3エフェクトのTextureへ出力)。
- **敵の弾** [EnemyBullet.efkefc](Data/Effect/EnemyBullet/EnemyBullet.efkefc): `Body`追加(Glowの後・Coreの前、通常ブレンド、`EB_Body2.png`=中は白・縁だけ暗いはっきりした円盤、色235,30,140、1.2倍→縁は約55,7,33の暗いマゼンタ)。Coreを0.9→1.1倍。Tailのテクスチャを`EB_TrailRim.png`(縁が暗い帯)に、Tail色を225,30,140,220→150,0,90,0、Ring色を215,20,125,230→150,0,85,0(少し深いマゼンタ)。インスタンス18→19。動的パラメーター(SeaGlow/SeaTrailの式)は未変更。
- **通常弾** [PlayerBullet.efkefc](Data/Effect/PlayerBullet/PlayerBullet.efkefc): 中身はBullet(None、速度Z0.6)→Trail(Track加算)・Head(Sprite加算)の2つだけだった。`TrailBack`(Track、通常ブレンド、`PB_TrailBack.png`、0,55,25,230、最後の9Fでフェード)を先頭に、`Backing`(Sprite、通常ブレンド、`PB_Backing.png`、0,50,22,210、0.9倍)をHeadの前に追加。TrailとTrailBackは2Fごと・寿命17(元のTrailは1Fごと・18)。インスタンス21→21。明るい海で加算の緑が水色に寄っていたのが緑に戻った。
- **チャージ弾** [PlayerChargeBullet.efkefc](Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efkefc): 既存`Backing`をはっきりした縁の`CB_Backing.png`に、0,48,26でアルファ90→200、3.2→2.6倍。`TrailBack`(Track、通常ブレンド、`CB_TrailBack.png`、0,55,25,220)を先頭に追加。Trailを1F・16→2F・15に。インスタンス(40F時点)153→151。
- 確認方法: Effekseer MCPのエディタで、ゲームと同じ画面上の大きさになる距離で撮影(エディタは距離1で440px/単位、ゲームは1280×720・縦90°で360px)。背景は単色(明るい空215,222,232＝スカイボックスの地平線のもや、海40,110,160、夜6,14,24)。エディタの背景画像はキャプチャに写らなかった。比較画像と白黒版で、3背景とも弾が分かれて見えることを確認。
- 気づき: 暗い背景では通常弾の白い芯が少し小さく見える(Trailの間引きで太い根元が弾頭に重ならなくなったため)。
- **未確認(ゲーム内)**: 実際の空・海での見え方、弾が多いときにプレイヤーの弾の暗い層が敵の弾に重なって隠さないか(別エフェクト同士の描画順はデータでは決められない)、反射弾(敵の弾と同じ見た目)。
- **(11/06続き・ユーザー報告「通常弾が変なとこ行く」)原因と修正**: `PlayerBullet.efkefc`の`Bullet`ノードに位置の速度Z=0.6(PVA)が入っていた(10/03に気づいて未確認のままだったもの)。[PlayerBullet::Update](Game/GameObjects/Bullet/PlayerBullet.cpp#L36)が毎フレーム`SetPos`するのに加えて、エフェクト自身もローカル+Zへ毎フレーム60(0.6×倍率100)動くため、弾頭が本当の弾から離れていった。今回の暗い層で目立つようになった。→`Bullet`の位置をFixedに戻し、PVAの速度も0にして、倍率50で.efkを書き出し直した。エディタで外から0.6/F動かして、弾頭が軌跡の先頭に付いてくることを確認。チャージ弾・敵の弾のルートには速度なし(確認済み)。ゲーム内は未確認。

### 進捗（2026-11-06続き・当たったときの火花`HitEffect`とボスのビーム`BossBeam`にも暗い層を追加）
- 依頼: 弾と同じ方針(加算の明るい芯＋通常ブレンドの暗い層)で、火花とビームを明るい背景でも見えるようにする。データだけで対応、C++/HLSLは未変更。
- テクスチャ: [gen_outline_textures.ps1](Data/Effect/EnemyBullet/tools/gen_outline_textures.ps1)に`Ring`(輪)と`Needle`(針)を追加し、`HE_SparkDark.png`・`HE_RingDark.png`・`BB_RimDisc2.png`を出力。
- **火花** [HitEffect.efkefc](Data/Effect/HitEffect/HitEffect.efkefc)(倍率50で[.efk](Data/Effect/HitEffect/HitEffect.efk)も書き出し): `SparksDark`追加(Sparksの前、通常ブレンド、`HE_SparkDark.png`=はっきりした縁の針、12本、寿命10〜16F、0.18×2.1→0.12×0.7、75,26,4,255→55,18,4,0、動きはSparksと同じ)。`Sparks`は32→24本。`RingDark`追加(Ringの前、通常ブレンド、`HE_RingDark.png`、8F、0.5→3.6、80,30,5,230→60,20,5,0)。親`HitEffect`の-Z 0.08はそのまま。インスタンス54→59(3F時点)。
- **ビーム** [BossBeam_Spiral.efkproj](Data/Effect/BossBeam/BossBeam_Spiral.efkproj)(XMLのまま手で編集)→1.80.6のCLIで[BossBeam_2.efk](Data/Effect/BossBeam/BossBeam_2.efk)に書き出し(倍率50)。`BossBeam_2.efkefc`は触っていない。
  - `RimDisc`追加(先頭＝いちばん先に描く、通常ブレンド、配置は固定(Billboard 2)、`BB_RimDisc2.png`=太めの輪＋中を薄く塗る、60,15,95,230、1.6倍＝輪の外側が半径0.75≒当たり判定の半径70、10Fごと＝当たり判定の球と同じ間隔、寿命430〜450、位置・回転は生成時のみ)。`TipBack`追加(TipGlowの前、通常ブレンド、同じテクスチャ、60,15,95,220、2.2倍、先端に1個)。
  - インスタンス(239F時点)127→152。7秒続くと約+43。
  - 安全手順: 一時フォルダにコピーして編集・書き出し→元の版をCLIで書き出すと今の`BossBeam_2.efk`とバイト一致することを確認→書き出したファイルのサイズ(4574→6090)と参照テクスチャ5つを確認してから本物と置き換えた。エディタではリポジトリのファイルを開いていない。
  - 跳ね返したビームのフェード(`SetColor`のアルファ)は全体の色に掛かるので、新しい通常ブレンドの層も一緒に消えるはず(未確認)。
- 確認: 前回と同じ(エディタ440px/単位、背景3色、1:1、白黒版)。火花は敵に命中(26前方→エディタ距離31.8)と自機被弾(4.25前方→5.19)、親の-Z移動はゲームのカメラも同じ速さで進むので、エディタでは打ち消して撮った。ビームは正面(ボスの砲口から約3000、先端が0.2/Fで近づく)と斜め(40°)。
- 気づき: ビームの暗い輪は10Fごとなので、斜めから見ると「輪の列」に見える(連続した筒ではない)。5Fごとにすると筒に近づくが、インスタンスが約+40%。自機被弾の近距離では、最初の数Fの白い閃光(Core 3.2倍)が機体を大きく覆う(今回の変更前から)。
- **未確認(ゲーム内)**: 実際の空・海での見え方、連続ヒットで画面が埋まらないか、自機被弾で機体が隠れすぎないか、跳ね返したビームのフェードで暗い層も消えるか。

### 2026-11-07 バグ報告(ユーザー): ボスを倒してリザルトからタイトルに戻ると、タイトルのプレイヤーが一定時間見えない
- 再現: ボスを倒す → リザルト(ClearScene) → タイトルへ。タイトルのプレイヤー(TitlePlayer)が見えない時間がある(ずっとではなく、消えている時間がある)。未調査。
- コスト表「バグ修正」に追加済み(見積1h・A・ベータ)。

### 2026-11-08 蝶の敵: 離脱で画面外に出たら消える・停止中に弾を2発撃つ(ユーザー実装、確認OK)
- **画面外で消える**: [ButterflyEnemy::Update](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp)の`switch`の後に、「前フレーム画面内 かつ 今フレーム画面外」で`OnDead()`(倒したことにしない)。`IsOnScreen()`は`m_colRadius`ぶんの箱の`CheckCameraViewClip_Box`(戻り値を反転)。登場時は最初が画面外なので消えない。**判定は状態を問わず全状態**(ユーザー: 蝶の移動は画面内に収める予定のため)。ユーザーの案(RushEnemyに書いた版)は、判定が`Update`の先頭でフラグ保存が末尾だと同じ位置を比べてしまうため、動かした後に判定する形に直して蝶へ移した。RushEnemyは元に戻した。
- **弾を撃つ**: `Shoot(playerPos)`(普通の`EnemyBullet`を発射、プレイヤーのZ速度を足す、`SoundType::EnemyShoot`、`CreateBullet`の最後に撃った敵`shared_from_this()`を渡すので**カウンターで跳ね返せる(確認OK)**)。撃つ条件は`m_stateTimer >= first_shoot_frame + shoot_interval * m_shootNum && m_shootNum < max_shoot_num`(60・90フレーム。回数が増えるごとに撃つ時間がずれる = 連続フレームで撃たれない)。`m_shootNum`は登場の終わりとダッシュの終わり(停止に入る所)で0に戻し、`Shoot`の中で`++`。
- **残り**: 回転弾(今は普通の弾2発)→ 死亡 → かくかく移動の残り(同じ方向を続けない、前後20%、進行方向へ傾く、止まったら機首をプレイヤーへ)。
- 未コミット(11/08時点): `ButterflyEnemy.cpp/.h`、`.pso`(ビルドで再生成されたもの)、`Log.txt`。
- **(11/08・ユーザー決定)蝶の残りの方針**: 死亡は「殺されたら爆発」だけ(特に追加なし)。かくかく移動の残りのうち、傾き・前後・同じ方向を続けない、は**しなくてもいい**。それより**画面から出ないように移動する**、**地面のモデルに埋まらないようにする**、の方が大切。次は回転弾から。

### 2026-11-08 蝶の回転弾`SpiralBullet`(ユーザー実装中)
- **設計(C案)**: [SpiralBullet](Game/GameObjects/Bullet/SpiralBullet.h)は`EnemyBullet`を継承して`Update`だけ上書き。`m_pos`=**軸の位置**(親の移動・寿命・画面外で消す処理がそのまま動く)、`m_spiralPos`=軸から`spiral_distance`離れて回る弾本体の位置。当たり判定の球とエフェクト(`m_effectPlayHandle`を`EnemyBullet`でprotectedに変更、`EffectManager::SetPos`)を`m_spiralPos`に合わせ直す。`BulletManager::BulletType::SpiralBullet`を追加(`case`は`EnemyBullet`と同じ形、`m_pEnemyBullets`に入る)。蝶の`Shoot`はこれを撃つ。
- **回転**: `m_bulletAngle += 1フレームの角度 * timeScale`、`m_spiralPos = 軸 + (cos, sin) * spiral_distance`(x,y平面)。単位はラジアン(最初は`++`で1rad/フレーム=約9.5周/秒で速すぎた)。60フレームで1周(2π/60)にした。
- **(ユーザー決定)回る向きは一方向でよい(撃つたびに交互にしない)。半径も広げない。**
- 残り: 名前と中身のずれ(`rotation_per_second`は実際は1フレームの角度。`rotation_per_frame`にするか、定数を「1秒に何周」にして計算する)、複数発の位相(角度を等間隔にずらすか)、カウンターで跳ね返したときの動き(軸の向きだけ蝶の方へ。`ReflectedBullet`との関係)、見た目の確認。
- **(11/08・ユーザー実装、確認OK)回転弾の最初の角度**: `SpiralBullet`のコンストラクタ末尾に`startAngle`。`BulletManager::CreateSpiralBullet(pos, vel, attackPower, pCamera, startAngle, pTarget)`を専用の作成関数として追加(`CreateReflectedBullet`と同じ流儀。`BulletType::SpiralBullet`と`CreateBullet`のcaseは削除)。蝶の`Shoot`は`DX_PI_F * m_shootNum`(1発目0・2発目π=反対側から回り始める)を渡す。`rotation_per_frame`(2π/60)、`spiral_distance`=300。
- **気づき(未対応)**: カウンターで跳ね返す[CollisionManager::ReflectEnemyBullet](Manager/CollisionManager.cpp#L423)は`pEnemyBullet->GetPos()`(=軸の位置)から反射弾を出すので、回転弾だと見えている弾の位置(軸から最大300ずれ)から出ず、跳ね返した瞬間に位置がとぶはず。

### 2026-11-08 ここまでのまとめと次(学校で続き)
- **蝶の敵(`ButterflyEnemy`)の今**: 登場→停止→ダッシュのくり返し、停止中に回転弾を2発(1発目0・2発目π=反対側から回り始める)、ダッシュ`leave_dash_num`(今2、仮)回で離脱、離脱して画面外へ出たら`OnDead()`で消える。カウンターで跳ね返せる(通常の`ReflectedBullet`)。ビルドは通っている。
- **次の1歩: カウンターで回転弾を跳ね返したときの位置を確認する**(まだ試していない。カウンターが当てづらいので、確認の間だけ蝶の`bullet_speed`(今30)を10くらいに下げる。終わったら30に戻す)。見るのは、跳ね返った弾が、見えている弾の位置から出るか、軸の位置(最大300ずれ)から出て位置がとぶか。とぶなら直し方を決める([ReflectEnemyBullet](Manager/CollisionManager.cpp#L423)が`GetPos()`=軸の位置を使っているため。案: 弾本体の位置を返す関数を`EnemyBullet`に作って反射が使う、など)。
- **蝶の残り(ユーザー決定)**: ①**画面から出ないように移動する**(ダッシュの行き先が画面外に出ない制限。今は端から出ると消える) ②**地面のモデルに埋まらないようにする**(2面の地面。Yの下限) ③死亡は「殺されたら爆発」だけ(`OnEnemyDead`、他の敵と同じでよいはず) ④回転弾の仕様のうち、**回る向きの交互・半径を広げる、はやらない**(ユーザー決定)。
- **コスト表**: 蝶みたいな敵(かくかく移動3h・回転弾2h・死亡1h・逃げる1h、アルファ)はまだ全部「未完了」。完了にするのは、画面内に収める移動・回転弾の確認・爆発まで済んでから。
- **バグ(コスト表に追加済み、ベータ・A・1h)**: ボスを倒してリザルトからタイトルへ戻ると、タイトルのプレイヤーが見えない時間がある(未調査)。
- 注意(今日のコミットに入れるかどうか): `.pso`(ビルドで再生成)、`Log.txt`(実行ログ、コミットしない方針)。
- **(11/08)`AGENTS.md`と`.agents/skills/sakurai-game-design/`**: 11/07に別のセッションで作られたもの(`CLAUDE.md`とまったく同じ内容、スキルも`.claude/skills/`の同名のものと同じ)。Claude Code以外のAIツールが読む用。ノートには11/06の`CLAUDE.md`・`sakurai-knowledge/`の記述だけがあり、これは記録されていなかったので、ここに残す。`Log.txt`は実行ログなのでコミットせず、変更は破棄した。
  - **→ 11/09訂正: これはわざと作ったものではなく、Codexの自動コピーだった(下の11/09)。リポジトリから外した。**

### 2026-11-09 `AGENTS.md`・`.agents/`はCodexの自動コピー → 同期を切ってリポジトリから外した(Claude、ユーザー依頼)
- ユーザー: 「pullしてくると`SKILL.md`と`AGENTS.md`が絶対いる」「家でも学校でも起きていた」「起きないようになればそれでいい」。
- 原因: **Codexデスクトップアプリの「外部エージェントの取り込み同期」**(`~/.codex/config.toml`の`[desktop]`に`external-agent-import-sync-enabled = true`、`external-agent-import-sync-item-types = "all"`)。Codexの起動時に`CLAUDE.md`→`AGENTS.md`、`.claude/skills/`→`.agents/skills/`へ同じ中身でコピーする。11/08に家のセッションがそれを「ほかのAIツール用」と思ってコミットしていた(af26665)。
- 対処: **学校のPC**で`external-agent-import-sync-enabled = false`(Codexの再起動後も`false`のままを確認)。`git rm`で`AGENTS.md`と`.agents/`をリポジトリから外し、[.gitignore](.gitignore)に`/AGENTS.md`と`/.agents/`を追加。
- **(11/09・ユーザー確認)回転弾をカウンターで跳ね返したときの位置**: 跳ね返せて、見た目の問題は気づかない程度 → 今は直さない([ReflectEnemyBullet](Manager/CollisionManager.cpp#L423)は軸の位置から出すまま)。確認のため蝶の`bullet_speed`を15に下げている(元は30)。
- **(11/09)回転弾が跳ね返しづらい理由(Claudeが数字で確認)**: 回る半径`spiral_distance`=300 > カウンターが届く距離132(カウンターの球100＋弾32) > 当たる距離82(プレイヤーの球50＋弾32)。じっとしていると当たりもせずカウンターも届かない。跳ね返すには回る弾の通り道へ自分から入る必要がある。案: 半径を82未満(例60)にすると「放っておくと当たる・カウンターなら返せる」になる(輪が小さくなり回転弾らしさは減る)。ユーザーはまだ決めていない。
- **(11/09・ユーザー実装、たぶん大丈夫)蝶が画面から出ないように移動する**: [UpdateStop](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L271)で行き先を決めるとき、ランダムな`dirIndex`から`(dirIndex + i) % dash_dir_num`で8方向を順に試し、行き先の**実際の位置**(`playerZ + dashPos`、プレイヤーから見た位置のままだとZがずれて判定が狂う)を`ConvWorldPosToScreenPos`で画面座標にして、端から`margin`(100px)より内側なら採用して`break`。`for`の前に`m_dashEndPos = m_dashStartPos`(全部ダメならその場から動かない)。今の`IsOnScreen`(`CheckCameraViewClip_Box`、少しでも見えていれば画面内)は離脱の「外に出たら消す」用で、行き先には使わない。ビルドOK。**建物に埋まるのでまだ確認しきれていない**(ユーザー)。
- **次**: 蝶が地面・建物に埋まらないようにする。
- **(11/09・ユーザー決定)蝶がダッシュでビルに飛び込む → ビルに当たったら爆発して死ぬ**(行き先をビルの当たり判定で避ける案より簡単で、止まっている間に前から来たビルに入る場合もまとめて片付く。前から来るのは「なるべくビルを小さくするしかない」)。**プレイヤーの撃破数には数えない**(ユーザー)。
  - ユーザー依頼でClaudeが書いた: [EnemyBase.h](Game/GameObjects/Actors/Character/Enemy/EnemyBase.h#L55)に`virtual void OnHitBuilding() {}`、[ButterflyEnemy::OnHitBuilding](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp)(死亡待機中なら何もしない＝プレイヤーが倒した扱いを守る → `m_isCrashed = true` → `TakeDamage(m_health)`)、`Update`の死亡待機の最後で`m_isCrashed`なら`OnDead()`、それ以外は`OnEnemyDead()`。Debug x64ビルドOK。
  - ユーザーが書いた: `hit_pairs`に`{ ButterflyEnemy, Building }`、[EnemyCollider::OnCollision](Game/Collision/EnemyCollider.cpp#L43)に`Building`なら`m_owner.OnHitBuilding()`。
- **(11/09・ユーザー決定、ユーザー依頼でClaudeが実装)突進する敵も、体当たり・ビル衝突で爆発したときは撃破数に数えない。ビルに当たったら爆発する**: 2種類で使うので「数えない死に方」を`EnemyBase`へ移した。[EnemyBase::Crash](Game/GameObjects/Actors/Character/Enemy/EnemyBase.cpp)(死亡待機中なら何もしない → `m_isCrashed = true` → `TakeDamage(m_health)`)、[OnEnemyDead](Game/GameObjects/Actors/Character/Enemy/EnemyBase.cpp#L51)の最初で`m_isCrashed`なら`OnDead()`して`return`(撃破数を数える場所はここだけ)。蝶の`m_isCrashed`と`Update`の分岐は消して、`OnHitBuilding`は`Crash()`だけに。突進する敵は[OnHitPlayer](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.cpp#L155)を`Crash()`に、`OnHitBuilding`を追加(`Crash()`)、`hit_pairs`に`{ RushEnemy, Building }`。Debug x64ビルドOK。**(11/09・ユーザー確認)蝶がビル・地面に当たると死ぬ、ぶつかった死は撃破数に数えない、突進する敵も同じ、弾で倒せば数える、すべてOK**。
- **(11/09・ユーザー決定)蝶は今のままでOK**: 新しいビルの当たり判定、弾の速さ15([ButterflyEnemy.cpp:41](Game/GameObjects/Actors/Character/Enemy/ButterflyEnemy/ButterflyEnemy.cpp#L41))、回転弾の半径90([SpiralBullet.cpp:10](Game/GameObjects/Bullet/SpiralBullet.cpp#L10))の3つとも変えない。コスト表の蝶の行を完了にした(実コスト: かくかく移動3・回転弾2・死亡0.5・逃げる0.5、計6h/見積7h)。アルファの残りは61→54。
- **エフェクトは別のAIに作らせる**(ユーザー)。こちらではエフェクトのファイルに触らない。
- **(11/09・Claude、ユーザー依頼「友達にプレイさせるので、レベルデザインを感じられる配置に」)敵の配置を見直した**。根拠は桜井さんの『単調な坂は山登りにならない』(困難と平地でデコボコ)、『パラメーターでキャラ立ちを』(敵は配置の組み合わせで印象的に)、『悪魔の釜』(新パルテナの反省「敵配置が満遍なさすぎ」)、『ハンデをつけてバランス調整』(開発者は慣れている)。元のCSV・Stage1.unityはscratchpadの`enemy_layout_backup_1109`。
  - **2面**(区間は11/04の「開けた所」と「ビルを避ける所」の交互のまま、砲台12はそのまま): 新しい敵を**1種ずつ初登場**させ、その後に組み合わせ、最後に山場→静けさ→ボス。
    - 0〜5500 突進の初登場(300右・1200左。1800→1200にして砲台の初登場と重ならないように)→標識の門の砲台
    - 5500〜9500 ビルを避ける＋ふち・丘の砲台だけ
    - 9500〜13000 **蝶の初登場**(7500、真ん中Y1000、ひとりで)。ここにいた突進2体(9600・10100)は消した
    - 13000〜17500 門と正面のビルを避ける＋砲台
    - 17200〜21000 **組み合わせ**: 突進の左右ペア16900→蝶17500(右上)→突進18400(左)
    - 21000〜25200 3棟のビルを避ける＋砲台
    - 25300〜29500 **山場**: 蝶2匹(23800右・24800左、ビルの区間を抜けた所で出そろう)＋突進5体(25400〜27600)。27600の後はボス(30000)まで何も出さない
    - 蝶は9→4、突進は12→10。蝶はどれも出て止まる場所に物がないことを確認(その後のダッシュ先は見ていない)。
  - **1面**: もとから導入(2体→リング)→ワーム単独→左右の塊→壁＋ワーム→縦列＋ワーム2→**山場(密集9＋ワーム3)**→余韻(3体)→ボス、の山の形だったので、**ボス直前のワーム(動き出し28300)だけ消した**(ボス前に静かな間を作り、ボスの登場を引き立てる)。Unityの`Stage1.unity`もImportAndSaveBatchで同期済み(81個)。
  - **ゲームでの確認はまだ**。友達のプレイで、どこで死んだか・どこで退屈したかを見て直す。
- **(11/09・ユーザー提起)2面のボス戦で地面とビルが途切れる → ボス区間をループさせる**: 2面の地面はCSVで42000まで(ボス出現30000)、ビルは[Building.cpp:34](Game/GameObjects/Actors/Building/Building.cpp#L34)の`Update`が空で動かない。1面は海と[Scenery](Game/GameObjects/Actors/Scenery/Scenery.cpp#L48-L55)(1周24000)がループするので困らない。
  - **1周は12000以上必要**: 置き直したものはカメラ＋(1周−3000)に出るので、描画の奥の限界(far 9000)より遠くないと目の前に急に出てくる。同じ形の地面が続くのは34000〜42000だけだった(32000までは右の谷の坂)。
  - **(済・Claude、ユーザー依頼)データ**: Unityの`Stage2.unity`で、40000の地面の列(11枚)を42000・44000に複製し、ビル10棟を足して**34000〜46000(12000)をつなぎ目の自然な1周**にして書き出した(`Building.csv`に32行追加、ほかは変化なし)。足したビルは、1周してつながった所も含めて同じ列の前後と2600〜3600おき、Unityの実寸の範囲で隣とのすき間600以上を確認。一時スクリプトは削除済み、元のシーン・CSVはscratchpadの`stage2_loop_backup_1109`。
  - **コード(ユーザーが書く)**: ①[StageConfig.cpp](Constants/StageConfig.cpp#L22-L26)の表にループの始まり(2面34000)と1周(12000)を足す(1面は0=ループなし) ②[BuildingDataSetter.cpp](Game/GameObjects/Actors/Building/BuildingDataSetter.cpp)で、CSVのZがその区間にあるビルにだけループの情報を渡す(区間には`offsetZ`も足す) ③`Building::Update`でScenery.cppと同じ計算。ただし**1周目は動かさない**(Sceneryの式のままだと周回数が負になり、ステージの最初に前へ出てきてしまう)、**Zが変わったフレームだけ当たり判定モデルに`ApplyMatrix`→`MV1RefreshCollInfo`**(今は作るときの1回だけ、[Building.cpp:18-20](Game/GameObjects/Actors/Building/Building.cpp#L18-L20))。`Update`はGameObjectManagerが毎フレーム呼ぶのでGameScene側の変更はいらない。
  - **(済・ユーザー実装)コード①〜③**: [StageConfig.cpp](Constants/StageConfig.cpp)に`buildingLoopStartZ`/`oneLapDistance`、[GameScene.cpp:351](Scene/GameScene.cpp#L351)から[BuildingDataSetter.cpp:106-107](Game/GameObjects/Actors/Building/BuildingDataSetter.cpp#L106-L107)へ渡して`isLoop`を決める(`offsetZ`を足す前のCSVのZで判定)。[Building.cpp:49-77](Game/GameObjects/Actors/Building/Building.cpp#L49-L77)の`Update`: 境目`minZ = カメラZ − 3000`、`m_baseZ > minZ`ならreturn(1周目は動かさない)、`周回数 = ceil((minZ − m_baseZ) / 1周)`、新しいZが今と違うときだけ`m_pos.z`を変えて当たり判定モデルに`ApplyMatrix`→`MV1RefreshCollInfo`(見た目は`Draw`で毎回`ApplyMatrix`しているので付いてくる。当たり判定の`MeshShape`はハンドルを持つだけなので作り直し不要)。
  - 途中で直したまちがい(本人の理解のため): `a <= x < b`はC++では常に真(`&&`で2つに分ける)、`if (m_isLoop) return;`の条件が逆、`ceil(-1.52)`は-1(0ではない。だから1周目のreturnが必要)、**`ceil(static_cast<int>(x))`はintで先に切り捨てるので切り上げにならない → `static_cast<int>(ceil(x))`**([Building.cpp:63](Game/GameObjects/Actors/Building/Building.cpp#L63))。
  - コメントの直し(動作に関係なし、まだ): [Building.cpp:33](Game/GameObjects/Actors/Building/Building.cpp#L33)「ボスの登場時からの」→「ループ区間の1周の長さ」、[Building.cpp:35](Game/GameObjects/Actors/Building/Building.cpp#L35)「CSVに書かれていたZ」→「CSVのZ＋チュートリアル終了位置」。
  - **ゲームでの確認はまだ(次にやること)**: 2面のボス戦で地面とビルが途切れずに続くか、置き直した瞬間が見えないか、F1の赤いワイヤーフレームが動いたビルに付いてくるか、ステージの最初でループ区間のビルが前に出てこないか。
  - **(11/11・ユーザーが確認して修正、1291ea5)「ボス登場後、ビルが突然現れたように見える」**: ①[Building.cpp:11](Game/GameObjects/Actors/Building/Building.cpp#L11)の`recycle_behind_distance`を3000→1000 ②`Building.csv`の46000・48000に地面2列とビル11棟(46081〜49939、ループ区間の外なので動かない)、45419.8にビル1棟(ループ区間の中)を追加。`Stage2.unity`は変わっていない(次にUnityから書き出すと②が消える)。
    - 【Claudeの見立て】原因は、置き直したものが「カメラ＋(1周−後ろの距離)」＝3000なら**カメラ＋9000＝far(9000、[CameraBase.cpp:11](Game/GameObjects/Camera/CameraBase.cpp#L11))のちょうど端**に出ていたこと。①だけでカメラ＋11000(farの外)になるので、これだけで直るはず。
    - **②の心配**: 1周目に34000〜38000のビルが+12000されて46000〜50000に来るので、②の動かないビルと**同じ列で重なる**(中心の距離: x=-10600で46137と46610(差474)、x=10600で46842と47326(差484)、x=-5400で47570と48309(差739)、x=-7900で46082と47045(差963)、x=5400で47940と48929(差989)、x=7900で46752と47856(差1104)。ビルの奥行きは約1300〜1800)。地面の46000・48000も、34000・36000の列が来て同じ場所に2枚重なる。45419.8のビルは、2周目以降に同じ列(x=5400)の34459が46459に来て差1040。→ ユーザーと相談中(②を外して①だけで確かめる案)。
- **(11/11・ユーザー変更)元の作品と見比べて、カメラと機体の傾きを調整**(まだコミットしていない):
  - カメラ: [GameCamera.cpp:185-186](Game/GameObjects/Camera/GameCamera.cpp#L185-L186)で注視点を「プレイヤーの向きの先」→**「カメラの真正面ずっと先」**に(回転が気持ち悪いので一旦。元の3行はコメントで残してある)。`lerp_t` 0.03→0.06。
  - 機体の傾き([DefaultRotationState.cpp](Game/GameObjects/Actors/Character/Player/Rotation/DefaultRotationState.cpp)、ユーザー実装、Claudeは説明のみ): **原因はZ軸の`LerpToAngleZ`が1フレームに2回(スティック用とボタン用で別のif文)呼ばれ、2つの目標の真ん中で止まっていたこと**(ボタン90°→実際約46°、スティック36°→約18°。落ち着く角度≒(0.97a+b)/1.97)。Z軸は後ろのif文1か所に: ロール中は`AddRotationZ`、右/左ボタン中は`±max_roll_tilt_angle`(90°、スティックに関係なくボタンが勝つ)、海の近くでボタンなら0(水平でいい、ユーザー判断)、それ以外は`stick.x * max_normal_tilt_angle`。Y軸(機首の左右)は**ボタンを押している間は最大90°、押していないときは45°**(`max_normal_tilt_angle`を36°→45°に。X軸の上下もこれを使うので45°になった)。
  - ゲームで見ること: ボタンで90°まで傾くのが遅くないか(t=0.03で9割まで約1.3秒。遅ければボタンのときだけtを大きく、『かまえは瞬時に極端に』)。
  - **(11/11・ユーザー)`recycle_behind_distance`は1000→3000に戻した**。【Claudeの調べ】46000〜50000に足した地面とビルは、`oneLapDistance`を12000のままにすると1周目に来るビルと重なる(中心の差が約480の所もある)が、**16000(34000〜50000を1周)にすると重なりはゼロ**(Unityで測ったビルの大きさで全部の組を確認。つなぎ目の45419.8も問題なし)。16000だと置き直したビルはカメラ＋13000に出るので、farの外になり突然現れない。→ [StageConfig.cpp:29](Constants/StageConfig.cpp#L29)の`12000.0f`を`16000.0f`にするのはユーザー(まだ)。`Stage2.unity`は今のCSV(ユーザーが足した分も入り)に合わせて取り込み直した。
  - **同時に変わっていた数値(ユーザー)**: 前に進む速さ8→**15**([IdleMovementState.cpp:11](Game/GameObjects/Actors/Character/Player/Movement/IdleMovementState.cpp#L11)、[MovingState.cpp:10](Game/GameObjects/Actors/Character/Player/Movement/MovingState.cpp#L10))、チャージ弾の速さ25→50([ChargeReadyState.cpp:17](Game/GameObjects/Actors/Character/Player/Shoot/ChargeReadyState.cpp#L17))。`BoostingEffectPS.pso`が20004→1724バイトになっているのはReleaseでビルドしたため(デバッグ情報なし)で壊れてはいない。ボスの置き場所のY(1面50000、2面30000、`BossEnemyData.csv`も)もユーザーが変えている。
- **(11/11・ユーザー依頼、Claude実装)速さ15に合わせてボスまでの時間を前(約65秒)に戻し、敵を少し増やした**。ユーザー: 「ボスまでの道が短くなった」「ボスに入ったら普通の敵はもう出なくていい、ボスまでの間だけ雑魚を出して」「難しすぎない程度に」。元のCSV・StageConfig.cpp・2つのシーンはscratchpadの`enemy_layout_backup_1111`。
  - **ボスの出現**: [StageConfig.cpp:28-29](Constants/StageConfig.cpp#L28-L29)の1面31300→**58700**、2面30000→**56300**(15/8倍。この2つの数字だけユーザーの許可でClaudeが直した)。ボスの置き場所(Boss.csvのZ)も同じだけ後ろへ: 1面34800→62200、2面34800→61100。
  - **1面**: 前の配置は時間で見て山の形(導入→…→山場→余韻→ボス)になっていたので、**浮遊敵・岩・ワームのZを全部15/8倍**にして前と同じ時間の並びにした。ワームの動き出しは「出会うまでの時間」が前と同じになるよう、Zの4450手前(前は3500。近づく速さが8+18→15+18になったため)。足したもの: 9500に2体(最初の敵が出るまで17秒あったのを早める、『遅さは罪』)、25500に2体、45500に3体(山場の前の小波)、31500にワーム1。浮遊敵42→49、ワーム7→8、岩31(数は同じ)。
  - **2面**: 地形(ビル)は伸ばせないので前半0〜29500はそのまま(突進を20300に1体だけ足した)。前の山場(25300〜29500)を「1つ目の山」にして、**後半(ループ区間の大通り、ビルは両脇|x|≥5400だけ)を新しく作った**: 29500〜33000 何もなし → 33000〜39500 **砲台の並木**(±2600の瓦礫の上に左右交互に4台、34000〜38800)＋蝶1(36000)＋突進ペア(39000) → 39500〜44500 突進1体ずつ2回(41500・43000) → 44000〜50500 **最後の山場**(蝶2: 44000・45200、突進5: 47500〜50500、砲台3: 47000〜50000) → 50500〜56300 静けさ → ボス。突進10→20、蝶4→7、砲台12→19。
  - ボス戦に普通の敵を残さないため: 蝶は約584フレーム生きるので最後の蝶を45200に(ブーストなしならボスの約11000手前で消える。ずっとブーストしていると残るかもしれない)。突進は約150フレームで終わる。1面の最後の浮遊敵(57750)はプレイヤーへ向かってくるので、ボスの前に後ろへ抜ける。
  - Unityの`Stage1.unity`(89個)・`Stage2.unity`(460個)をCSVから取り込み直して保存(Stage2は一時スクリプトで取り込み、削除済み)。
  - 根拠: 『単調な坂は山登りにならない』(困難と平地でデコボコ)、『悪魔の釜』(満遍なさすぎない)、『ハンデをつけてバランス調整』(開発者は慣れている → 難しすぎない量に)。**ゲームでの確認はまだ**。
  - (11/11・ユーザー)ボスが召喚する敵はそのままでいい。
- **(11/11・ユーザー)タスク追加(コスト表の「バグ修正」に入れる、どちらもベータ)**: ①ロックオンしたとき、画面上でカーソルに一番近い敵ではない敵にロックオンされる(優先度A・1h) ②ビルと重なって見えない敵にロックオンされる(優先度S・2.5h)。~~Excelでxlsxが開いていて書き込めなかったので、まだコスト表に入っていない~~ → **(11/12確認)コスト表の「バグ修正」の100・101行目に入っている**。
- **(11/11・ユーザー着手)画面に水滴がつくシェーダー**(コスト表「シェーダ＞カメラレンズの水滴」8h・S・アルファ)。雨のエフェクト(Effekseer)とボスのビームの暗い層の作り直しは別のAIに頼む(プロンプトはClaudeが書いた)。
  - 作り方(Claudeの説明、コードはユーザー): BoostingEffectと同じ「画面全体に後からかけるシェーダー」。[GameScene.cpp:733-742](Scene/GameScene.cpp#L733-L742)で「ブーストを水滴用の画面に描く → 水滴シェーダーを通してバックバッファへ → UI」。水滴は後ろの絵を中心からの向きと逆へずらして読む(屈折)。順番は①決まった場所に1つ ②マス目＋ハッシュでたくさん ③時間で動かす ④雨のステージだけ(`StageConfig`)。画面の中心は薄く、端ほど強く(『キャラクターを際立たせる』)。
  - **(11/11・ユーザー決定)水滴は風で後ろ(画面の外側)へ流れる**。【Claudeの案】極座標(中心からの距離と角度)でマス目を作ると、「下へ流れる」窓の雨の作り方の「下」を「外側」に置きかえられる。
  - **(11/11・ユーザー実装)①の水滴1つがゲームに出た**。シェーダー[WaterDropEffectPS.hlsl:37](WaterDropEffectPS.hlsl#L37)で、水滴の中だけ `中心 - (中心→ピクセルのベクトル) * shrink_ratio` を読む(ユーザーが自分で導いた式。マイナスで逆さ、0〜1の割合で縮む)。中心(0.7,0.4)・半径0.05・割合0.4は`static const`の仮の値。C++は[WaterDropEffect.cpp](ShaderEffect/WaterDropEffect.cpp)、描画の順番は[GameScene.cpp:733-748](Scene/GameScene.cpp#L733-L748)(ブーストを水滴用の画面へ→バックバッファに戻す→水滴シェーダー→`SetUpCamera`→UI)。**次**: 画面が横長なので丸が横に伸びるはず(縦横比の補正)→縁をなめらかに・暗く→②たくさん。
    - (11/11)縦横比の補正済み: [WaterDropEffectPS.hlsl:33-35](WaterDropEffectPS.hlsl#L33-L35)で「中心→ピクセルのベクトル」のyを`screen_aspect_ratio`(横幅÷高さ、仮で1280/720)で割った`correctedWaterDropToUV`を作り、`length()`(中か外か)にだけ使う。読む場所の式はUVのままの`waterDropToUV`(`tex.Sample`はUVを受け取るため)。つまずいた所: yに掛けると逆(yの数字はもともと大きいので小さくする)、位置ではなく引き算の結果を割る(`/`は`-`より先)。
  - (11/11・ユーザーの希望)説明は小さな手順で1つずつ。式は答えを出さず、数字の計算ではなくベクトルの意味から自分で導く。参考サイトは日本語(日本語の「画面の水滴」の解説は見つからなかった。近いのは[Siv3Dの歪みシェーダ](https://scrapbox.io/voidproc-siv3d-examples/歪みシェーダ))。
  - **(11/11・ユーザー依頼、Claude)1周を16000に**: [StageConfig.cpp:29](Constants/StageConfig.cpp#L29)の`oneLapDistance`を12000→16000(34000〜50000で1周)。
  - **(11/11・ユーザー「蝶はプレイヤーから離れすぎ」)蝶を近くに**: [ButterflyEnemy.csv](Data/CSV/Stage2/ButterflyEnemy.csv)の`frontOffset`(プレイヤーの何前で止まるか)2500→**1500**、Y(プレイヤーから見た高さ)を300下げた(1000→700など)。止まる場所のZが前と同じになるよう、出現のZを1000後ろへ(7500→8500など)。プレイヤーがX±1600・Y100〜900のどこにいても、止まる場所にビル・地面・門がないことをUnityの範囲で確認。プレイヤーからの距離は約2800→約1800。最後の蝶は46200(ブーストなしならボスの前に消える)。
- **(11/09・ユーザー変更)ライトの向き**[Game.h](Constants/Game.h#L30)のZを`-0.816`→`0.816`(光がカメラ側から奥へ進む向き)。理由: 夜空のスカイボックスではなくなったので月に合わせる必要がない、カメラから見てプレイヤーや敵の手前側に光が当たらず見にくかった。月の方向の説明コメントも削除。ビルとの判定はモデルの形で調べるので、突進する敵が多い所で重くならないか見る。
- **(11/09・ユーザー依頼、Claude実装)ビルの当たり判定を「箱の山」から「外側の面だけの1つのメッシュ」に**: ユーザー「いっぱい四角を用意するんじゃなく、最初みたいに1つのメッシュで、形はちゃんとビルに寄せられないか(ProBuilderなら)」。手作業のProBuilderの代わりに、[BuildingCollisionTool.cs](StageEditor/Assets/Editor/BuildingCollisionTool.cs)に**「Auto Generate All (1つのメッシュで作る)」**を追加: 今までと同じボクセル(`Voxelize`に切り出して共通化、`AutoVoxelSize`=8)から、埋まったマスと空いたマスの境目にだけ面を張り、同じ平面に並んだ面は長方形にまとめる(閉じたメッシュなので`IsHitSphereAndMesh`の「真上に線を伸ばして中か調べる」もそのまま効く)。ゲームのコードは変更なし(`_Col.x`の差し替えだけ)。Unity 2022.3.62f1のバッチ(`-executeMethod BuildingCollisionTool.AutoGenerateAllSingleMeshBatch`)で6種類を書き出し済み。
  - 三角形の数(箱のとき→1つのメッシュ): AntennaTower 1176→702、BridgeWreck 252→186、Collapsed 972→684、Leaning 1236→912、OfficeGutted 288→268、TallBroken 2292→1464(合計約3割減)。
  - **ついでに見つけたずれ**: 箱を作る計算(`addBox`の`bmax`)が外側の空き1マス分を引き忘れていて、**+X・+Z側が1マス(ファイルの数値で8、ゲームで60〜100)大きかった**(11/04の「見た目より大きい」の残り)。新しいメッシュは正しい大きさ。箱の方の計算も直した(今は使っていない)。
  - 元の`_Col.x`・`_Col.fbx`はscratchpadの`col_backup_1109`。**ゲームでの確認はまだ**(F1の赤いワイヤーフレームがビルに沿っているか、すき間から抜けないか)。形をもっと寄せたいときは`AutoVoxelSize`を小さく(例: 4で三角形は約4倍)。
- **蝶をステージ2全体に配置**([Data/CSV/Stage2/ButterflyEnemy.csv](Data/CSV/Stage2/ButterflyEnemy.csv)、9匹、出現Z 3500〜24000)。1匹目(0,400,800)は道路をまたぐ標識の門(Sign_Gantry)にぶつかっていた。
  - 決め方: **出てきて止まる場所(出現が終わってから最初のダッシュまで)にビル・地面・高架などがないこと**だけを確かめた(ユーザーの指定)。ダッシュ後の場所までは見ていないので、ダッシュ先でぶつかることはまだある。
  - 確かめ方: Unityの`Stage2.unity`から全部の物の範囲(箱)を一時スクリプトで書き出して比べた(スクリプトは削除済み、結果はscratchpadの`stage2_bounds.csv`)。
- **家のPCはまだ**: 家で最初にClaudeが`~/.codex/config.toml`を同じように`false`にし、残っている`AGENTS.md`/`.agents/`を消す(環境まわりなのでユーザーに確認させない)。`.gitignore`があるので、それまで作られても変更一覧には出ない。

### 進捗（2026-11-11・ボスのビームの暗い層を作り直した：輪の列 → 暗い螺旋の筒＋先端の渦）
- 依頼(ユーザー): 11/06の暗い層(`RimDisc`=暗い輪を10Fごと、`TipBack`)が「黒いわっかを並べただけ」で、斜めから見ると輪の列になりつながった筒に見えない。明るい背景で形が読める役目はそのまま、暗い部分そのものを凝った見た目に。データだけで対応(C++/HLSLは未変更)、`BossBeam_2.efk`の名前はそのまま。
- 素材: [gen_beam_dark.ps1](Data/Effect/BossBeam/tools/gen_beam_dark.ps1)(UTF-8 BOM付きで保存すること。BOMなしだとPowerShell 5.1が日本語コメントで行を読み違えてC#が壊れる)で`Texture/BB_TubeSpiral.png`(筒に巻く螺旋の膜、上下左右とも継ぎ目なし、3本の帯・帯の片側だけ明るい縁)、`Texture/BB_Vortex.png`(中心を抜いた3本腕の渦)、`Model/BB_Tube.efkmodel`(中空の筒、半径0.66＝当たり判定70より少し内側、長さ±1.1、両端0.2を頂点alphaで薄く)を出力。確認用の並べ道具は[compose_bg.ps1](Data/Effect/BossBeam/tools/compose_bg.ps1)。
- [BossBeam_Spiral.efkproj](Data/Effect/BossBeam/BossBeam_Spiral.efkproj)(XMLのまま)。ノード順(＝描画順): `SpiralPivot`(ThunderRingA/B、変更なし)→**`DarkTube`(新)**→`TipGlow`(変更なし)→**`TipVortex`(新、旧TipBack)**→`StrayBolts`(変更なし)。`RimDisc`は削除。
  - `DarkTube`: モデル、通常ブレンド、両面描画、10Fごと(＝当たり判定の球と同じ間隔、先端0.2/F×10＝2.0おき、筒の長さ2.2で0.2重なる)、寿命430〜450、位置・回転・拡大は生成時のみ、フェードイン4F・アウト10F。UVスクロールX 8.5333px/F(10Fで帯1本ぶん＝隣の筒と模様がずれない)。拡大はFCurveで生まれた時XY 1.25→24Fで1.0(先端が少しふくらむ)。**明るい螺旋の輪の後に描く**(前に描くと加算の輪の中の塗りで筒が白っぽく消えた。後に描くと、筒の内側の輪は膜のすき間から透け、外へはみ出た輪だけが明るく見え、2層でひとつのビームになる)。
  - `TipVortex`: スプライト(カメラ向き)、`BB_Vortex.png`、2.3倍、Z回転-9°/F、色255,255,255,255。**`TipGlow`の後に描く**(前だと加算の光で渦が消え、正面から見て明るい空で読めなかった)。
- インスタンス(239F時点): 152→**152**(RimDisc→DarkTubeで同数)。`.efk` 6090→6412バイト、参照7つ(上の新素材3つ＋Particle01/SpiralThunderRing01/02/Thunder01)。`BB_RimDisc2.png`はもうどこからも参照されていない(消していない)。
- 安全手順: 一時フォルダで、元の版を1.80.6のCLIで書き出すと今の`BossBeam_2.efk`とバイト一致することを確認→一時フォルダで作って書き出し→エディタ(1.80.7)でXMLを開き直して見た目とインスタンス数を確認→置き換え→リポジトリのファイルからもう一度書き出してバイト一致を確認。
- 確認(エディタ、ボスの砲口が原点、先端が-Zへ0.2/F): 斜め40°(150F)、正面(砲口から30＝ゲーム3000、100F)、至近距離(ゲームのカメラと同じくプレイヤーの166後ろ・同じ高さ、146F、機体の目印つき)、背景4色(明るい空・海・曇り・夜)と白黒版。比較画像: [compare_obl.png](Data/Effect/BossBeam/tools/review/compare_obl.png)、[compare_front.png](Data/Effect/BossBeam/tools/review/compare_front.png)、[compare_close.png](Data/Effect/BossBeam/tools/review/compare_close.png)。跳ね返しのフェードはエディタの全体の色(アルファ64)で代わりに確かめ、筒も一緒に薄くなった([new_fade_alpha64_sky.png](Data/Effect/BossBeam/tools/review/new_fade_alpha64_sky.png))。
- 気づき: 至近距離で機体を一番隠しているのは前からある加算の`TipGlow`(白、2.4〜3.6倍)で、今回の暗い層ではない(渦の腕は翼の端にかかる程度)。気になるなら`TipGlow`を小さく・薄くするのが効く(今回は明るい層なので触っていない)。
- **未確認(ゲーム内)**: 実際の空・海・2面の曇り空での見え方、ビームがプレイヤーを追って曲がるときの筒のつなぎ目(外側にすき間が出るか)、スロー(カウンター)中は先端が遅くなり筒が重なる(濃くなる)はず、跳ね返したビームのフェード、DxLib(1.80.6)で`.efkmodel`のUVスクロールがエディタと同じに動くか。

### 進捗（2026-11-11・2面の雨のエフェクト`Rain`を作った：カメラ追従・3層・切れ目なし）
- 依頼(ユーザー): 10/05の方針「Effekseerのカメラ追従の雨粒」。空間に降る雨だけ(画面の水滴は別にシェーダーで作る)。データだけ、`Data/Effect/Rain/`に新規。**コード側はまだ(ユーザーが書く)**。
- ファイル: [Rain.efkproj](Data/Effect/Rain/Rain.efkproj)(XML)→1.80.6のCLIで[Rain.efk](Data/Effect/Rain/Rain.efk)(倍率50、3744バイト、参照は`Texture/Rain_Streak.png`だけ)。作り直しは[build_rain_efkproj.ps1](Data/Effect/Rain/tools/build_rain_efkproj.ps1)の層の表を書き換えて実行(XMLを丸ごと書き直す)、テクスチャは[gen_rain.ps1](Data/Effect/Rain/tools/gen_rain.ps1)(白い筋、下ほど濃い、端でalpha=0。色はノードで付ける)。
- **軸**: 回転を渡さないと、エディタ-Z＝ゲームの前、+X＝右、+Y＝上(DxLibの左手系でZが反転。プレイヤー追従のエフェクトも`GetRotationY()+π`＝実質回転0なので同じ扱い)。
- ノード(すべて「位置・回転・拡大は生成時のみ」、通常ブレンド、`Billboard=Fixed`、X回転-20°＝前へ0.3/F進むときの見かけの流れの向き、落下0.75〜0.9/F＝ゲームで37〜45/F、無限に出し続ける):
  - `RainNearL/R`(近い層): X 2.5〜11(左右だけ。**画面の中央の列 |x|<125 には出さない**)、Y -3〜12、Z -12〜-3(ゲームで150〜600前＝自機より手前)、1F に1本ずつ、寿命24〜28、0.05×2.0、色195,205,218,110。
  - `RainMid`: X ±28、Y -6〜22、Z -36〜-8(自機より奥なので、自機・敵・ビルの後ろは深度で隠れる)、0.15Fに1本、寿命32〜36、0.07×1.7、190,200,214,80。
  - `RainFar`: X ±70、Y -6〜40、Z -110〜-36(ゲームで1800〜5500前)、0.2Fに1本、寿命48〜52、0.14×2.4、185,195,210,55(霞む)。
- **ループ**: どのノードも無限に出し続け、エフェクト自体に終わりも切れ目もない(エディタの240Fループは確認用の表示だけ)。ステージ開始から約0.5秒で雨がそろう(最初の一瞬は少ない)。
- **インスタンス**: 定常で約530(エディタで532〜537)、理論上の上限は約560。`Effekseer_Init(8000)`(ゲーム全体の上限、[Application.cpp:63](Main/Application.cpp#L63))の約7%。
- **強さの調整**: 量＝動的パラメーター入力0(`RainAmount`: 発生間隔×(1+入力0))。0で今の量、+1で半分、-0.5で2倍(インスタンス約1100)。**-1に近づけると発生間隔が0になるので-0.75より下げないこと**。濃さ＝`SetColor`のアルファ(全体に掛かる)。
- **コード側で渡す値**(ユーザーが書く): ステージ開始時に1回`Play`、毎フレーム`SetPos(カメラの位置)`、回転・拡大は渡さない(読み込み倍率は1.0)。量を変えるなら`SetDynamicInput(ハンドル, 0, 値)`、濃さは`SetColor(ハンドル,255,255,255,アルファ)`。
- 確認(エディタ、エフェクトを-Zへ0.3/F動かしカメラを毎回その位置に置く。エディタは440px/単位なので1564×880で撮るとゲームの縦90°と同じ範囲→1280×720に縮小): [曇り](Data/Effect/Rain/tools/review/rain_gamecam_cloudy_1280x720.png)、[暗いグレー](Data/Effect/Rain/tools/review/rain_gamecam_darkgray_1280x720.png)、[連続6F](Data/Effect/Rain/tools/review/rain_frames_60-65_darkgray.png)。自機の位置(カメラの400前＝166＋Lerpの遅れ約235)に敵の弾(マゼンタ)、右奥に自機の弾(緑)の目印を置き、どちらも雨に埋もれないこと、中央が薄いことを確認。
- **地面のしぶきは作っていない**: エフェクトはカメラの位置しか知らず、カメラの高さ(`0.5×プレイヤーY＋250`で300〜700)と地面(道路-10、がれき390)の差が毎フレーム変わるため、データだけでは地面に置けない。やるなら「カメラの高さ」を動的パラメーターで渡す作りになる(コードが要る)。
- **未確認(ゲーム内)**: ビル・地面・フォグと重ねたときの濃さ、ブースト中(速い)の流れ方、深度で本当に自機・敵の後ろの雨が隠れるか、他のエフェクト(弾)との描画順、雨の下の端が地面の下に見えないか(地面より下は深度で隠れるはず)。連続フレームはエディタのカメラを止めて撮ったので、6Fの間に雨の範囲が1.8だけ奥へずれている。
- **(11/11・ユーザー実装、確認OK「だいぶそれっぽく見えます」)水滴は「縮小」のほうが水滴らしい**: ユーザーが「縮められた部分の解像度が落ちている」と気づいた。原因は、`shrink_ratio`=0.3が「中心から0.3倍の近い所を読む」=**拡大**になっていたこと(1280×720の絵を引き伸ばすのでぼやける)。本物の水滴は小さな凸レンズ(魚眼に近い)で、周りの景色を小さく上下左右反転して映す。`shrink_ratio`を**2.0**(広い範囲を読んで小さく詰める=縮小)にしたら、ぼやけず水滴っぽくなった。覚え方: 読む範囲が元の何倍か。<1で拡大・>1で縮小。名前の`shrink_ratio`は2.0で意味が合う。
- **水滴の次**: 輪郭を強調する(縁のあたりだけ大きい値を作る→暗い縁や明るいふち)。その後にマス目でたくさん出す。ユーザーは「数を増やすより先に1つをリアルに」と判断した(桜井さん『やり過ぎてちょうどよい』の考え方とも合う)。

### 2026-11-11 ボス登場: WARNING中はプレイヤーを止めず、ムービーから止める(コミット`940a91d`)
- **(ユーザー実装)** [GameScene.cpp:510](Scene/GameScene.cpp#L510)の`ChangeAllStateToDisabled()`(ボス登場Zに着いた瞬間に止めていた)をコメントアウトし、[GameScene.cpp:548](Scene/GameScene.cpp#L548)の`case BossApearState::Movie:`で止める。毎フレーム呼んでも[Player.cpp:673](Game/GameObjects/Actors/Character/Player/Player.cpp#L673)で2回目以降は何もしない。
- **(ユーザー依頼「CSVのボスの位置をその分ずらせばいい」、Claude)** ボスは出現前は動かない([BossEnemy.cpp:158](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L158)の`m_isAppear`まで)ので、WARNING中に進む分だけボスを奥へ: [Stage1/Boss.csv](Data/CSV/Stage1/Boss.csv#L2) 62200→**64960**、[Stage2/Boss.csv](Data/CSV/Stage2/Boss.csv#L2) 61100→**63860**(+2760)。
  - 2760＝15×184フレーム: 着いたフレーム(None→Start)の次から、Start 1F・揺れ(`m_shakeFrame`180から減らして`<0`で終わる)が終わるまで・Movieに切り替わった次のフレーム(そこで速度0)まで。
  - **ずれる場合**: WARNING中にブースト・ブレーキを使うと進む量が変わり、そのぶん距離がずれる(ユーザーはCSVで固定にする方を選んだ。きっちり合わせるなら、Startで「プレイヤーから見たボスのZ」を覚えて、ムービー開始時に置き直す方法がある)。
  - WARNING中は`m_isDisabled`が立たないので、被弾・岩との衝突は起きる。WARNING中に死んだときの流れは未確認。
  - **ゲームでの確認はまだ**: ムービー明けのボスとの距離が前と同じに見えるか。

### 2026-11-12 未確認だったもののゲーム内確認(ユーザー「全部大体OK」)
- ボス登場のボスとの距離(上の+2760)、2面のボス区間のループ(1周16000、途切れ・突然出るビル)、1面・2面のボスまでの敵の配置、2面の蝶を近くに寄せた配置、機体の傾き(ボタンで90°)。どれも大きな問題なし。
- **次**: カメラレンズの水滴の続き(輪郭の強調 → マス目でたくさん → 時間で流す → 雨のステージだけ)。

### 2026-11-12 水滴の縁を暗くした(ユーザー実装、動いた)
- ユーザーの考え: 水滴は平面ではないので「ボコっと」出っ張って見えてほしい。本物の水滴は縁のすぐ内側だけが細く暗い(中心からなだらかに暗くすると穴や影に見える)。
- [WaterDropEffectPS.hlsl:61](WaterDropEffectPS.hlsl#L61): `waterDropRim = saturate(1 - waterToFrameRatio / water_drop_rim_width)`(縁で1、`water_drop_rim_width`=0.2より内側で0。0.2はUVではなく半径に対する割合)。[64-67行目](WaterDropEffectPS.hlsl#L64-L67)で水滴の外は0に(外も`waterToFrameRatio`が0なので、しないと画面全体が真っ暗)。[75行目](WaterDropEffectPS.hlsl#L75)で`col.rgb * (1 - waterDropRim)`。
- つまずいた所: 条件を`< 0.0f`にしていた → `saturate`で0に切りそろえているのでマイナスにならず、いつも偽。`<= 0.0f`に。色を暗くする＝rgbに0〜1を掛ける(アルファは掛けない)。
- 縁が黒すぎた → [WaterDropEffectPS.hlsl:65](WaterDropEffectPS.hlsl#L65)で`waterDropRim`に`water_drop_rim_darkness`(0.5、0で暗くしない・1で真っ黒、[28行目](WaterDropEffectPS.hlsl#L28))を掛けて、縁でも半分の明るさに。ユーザー「めっちゃよさそう」。つまずいた所: `+0.5`だと内側が1.5になり明るくなる(足し算は全体を持ち上げる。引く量を縮める＝掛け算)。
- **次**: 水滴に光(ハイライト)を持たせる。

### 2026-11-12 水滴に光る点(ユーザー実装、「まぁ悪くなかった」)
- 場所: ライト([Game.h:30](Constants/Game.h#L30))は光源がカメラの後ろの右上なので、水滴の中の右上。本物の水滴の光る点は小さく(直径が水滴の1/5〜1/4)、白に近くくっきり。
- [WaterDropEffectPS.hlsl:83-85](WaterDropEffectPS.hlsl#L83-L85): 光る点の位置は**水滴の中心から見た位置・補正済みのものさし**で`(r*offset, -r*offset)`(`water_drop_highlight_offset`=0.4)。`correctedWaterDropToUV`がすでに`uv - center`なので`water_drop_center`は足さない(足すと住所と相対位置を引くことになる)。UVのyは下がプラスなので上はマイナス。
- [88行目](WaterDropEffectPS.hlsl#L88)で光る点からピクセルまでの距離、[91行目](WaterDropEffectPS.hlsl#L91)で`highlightRatio`(52行目と同じ形、半径`water_drop_brightness_radius`=水滴の半径×0.4)、[96行目](WaterDropEffectPS.hlsl#L96)で縁を暗くした色を白へ`lerp`。
- 残り(気になったら): 一定の割合で薄くなるのでくっきりした点ではない、中心が真っ白で強いかも。名前の`brightnessToWaterCenterLength`(実際は光る点→ピクセル)と68行目のコメントは前のまま。

### 2026-11-12 水滴をユーザーが自分だけでもう一度書いた(前の版はコメントで残してある)
- [WaterDropEffectPS.hlsl:98-142](WaterDropEffectPS.hlsl#L98-L142)。丸のまま(`water_drop_stretch`なし)、縮小なし(`water_drop_pos - waterDropToUV`で上下左右反転のみ)、光る点はまだ。
- 暗さは[124行目](WaterDropEffectPS.hlsl#L124)で`saturate(1 - 補正後の距離/半径 * water_drop_rim_darkness)`(中心1・縁0.5)。前の版と違い中心からなだらかに暗くなる(穴に見えたら`water_drop_rim_width`の書き方に戻す)。
- つまずいた所: ①`saturate(1 - 距離/半径) * 0.5`だと0〜0.5になる(掛け算は範囲を縮めるだけで持ち上げない) → 0.5を距離/半径の側に掛けて解決 ②暗さだけ補正なしの距離を使っていて、上下の縁が0.11まで暗くなっていた。
- 残り(動きに関係なし): 104行目の`waterDropToUVLength`は未使用、122行目のコメントが逆。
- **次(ユーザー)**: 水滴の数を増やす、後ろ(画面の外側)へなびいていくようにする。増やし方は個数しだい(数十個まで→C++から配列で渡して全部調べる、何百個も→マス目＋ハッシュ。外側へ流すなら極座標のマス目、11/11の案)。個数はまだ決めていない。

### 2026-11-12 環境: HLSL Tools → Shader validator に入れ替え(家PC、Claude、ユーザー依頼)
- HLSL Tools(`timgjones.hlsltools` 1.1.303)の`ShaderTools.LanguageServer`が、起動から数時間で**約45GB**を確保してPCが重くなった(シェーダーは16ファイル・0.2MBなので量ではなく増え続ける不具合と見ている。既知の報告は見つからず)。
- HLSL Toolsをアンインストールし、**Shader validator(`antaalt.shader-validator` 1.6.0)**を入れた。補完・ホバー・定義へジャンプはこちらが担当。[.vscode/settings.json](.vscode/settings.json)で`shader-validator.validate: false`(DXC/SM6で判定するのでfxc向けのコードに間違ったエラーが出うる。エラーは今まで通りhlsl-check)、`symbols: true`。
- `[hlsl]`の`quickSuggestions`オフなどは前からの設定のまま(補完は`Ctrl+Space`で出す)。
- **学校PCはまだ**: 同じようにHLSL Toolsを外してShader validatorを入れる(設定はリポジトリの`.vscode/settings.json`で付いてくる)。

### 2026-11-12 水滴の形は丸のまま(ユーザー決定)
- 昨夜(11/11)試した「縮小(`shrink_ratio`=2.0)」「縦長(0.7)」は入れず、**書き直した今の版のまま(丸・上下左右反転のみ)で行く**(ユーザー「丸で行きます。そのまま」)。ノートの11/11の縮小・縦長の記録は、この版には入っていない。
- **次**: 水滴の数を増やす。個数がまだ決まっていない(数十個まで→C++から配列で渡して全部調べる、何百個→マス目＋ハッシュ)。

### 2026-11-12 水滴を数十個に(設計、ユーザー実装中)
- **方針(ユーザー)**: 個数は数十個、プレイの邪魔にならない程度(画面の中央は薄く端に寄せる)。C++から**定数バッファ(`b9`、`ShaderRegister::cbuffer_water_drop`)で`WaterDropData`の配列**を渡し、ピクセルシェーダーで全部をループして評価する。
- **HLSL**([WaterDropEffectPS.hlsl:12-22](WaterDropEffectPS.hlsl#L12-L22)): `struct WaterDropData { float radius; float2 waterDropPos; float padding; }` と `cbuffer WaterDropBuffer : register(b9) { WaterDropData data[15]; }`(16バイト、配列の要素は16バイト境界)。個数15は定数にして、C++の`max_water_drop_count`と合わせる。変数名`data`は一般的すぎるので`waterDrops`など。
- **C++**([WaterDropEffect.h](ShaderEffect/WaterDropEffect.h))に構造体を同じ並びで定義(ユーザー)。`WaterDropEffect.cpp`で定数バッファの作成・ポインタ取得・解放(コンストラクタ/デストラクタ、`BoostingEffect`と同じ形)をClaudeが書いた(ユーザー依頼)。`Vector2`は`float x,y`だけの8バイト。
- **cbufferのパッキング**(ClaudeのHLSL説明の訂正): 変数は16バイトの「行」に前から詰める。**16バイト境界をまたぐ場合だけ**次の行へ。`float2`が8バイトに揃う規則はない(最初「隙間が自動で入る」と説明したが誤り)。配列の要素は16バイト境界から始まる(ストライド16)。
- **残り**: ①C++で配列に値を書く(最初は確認用に2〜3個を決め打ち) ②`Draw`で`UpdateShaderConstantBuffer`と`SetShaderConstantBuffer(…, cbuffer_water_drop)` ③HLSLの`main`を配列のループに ④重なり: 配置で被らないようにする(ユーザー案)。シェーダーは「あとから評価した水滴が上書き」の単純な形で。
- **案(後で)**: 水滴どうしがつながって見えるようにする(メタボール)。各水滴の`waterToFrameRatio`を足し合わせ、合計が一定以上を内側とみなす。近い水滴の境目がくびれてつながる。

### 2026-11-12 水滴を数十個に: C++側ができ、HLSLのループは途中(ユーザー実装、続きは次回)
- **C++**([WaterDropEffect.cpp](ShaderEffect/WaterDropEffect.cpp)): コンストラクタで`waterData[0〜2]`に確認用の3個(位置はUV、半径0.03/0.05/0.07)を決め打ちし、残りは`radius=0`・位置0に初期化(`GetBufferShaderConstantBuffer`が返すメモリは0とは限らない)。`Draw`で`UpdateShaderConstantBuffer`→`SetShaderConstantBuffer(…, cbuffer_water_drop)`→描画→`-1`で外す(`BoostingEffect`と同じ形)。個数は`max_water_drop_count`(15)。
- **HLSL**([WaterDropEffectPS.hlsl](WaterDropEffectPS.hlsl)): `struct WaterDropData { float radius; float2 waterDropPos; float padding; }`、`cbuffer WaterDropBuffer : register(b9) { WaterDropData waterDrops[max_water_drop_count]; }`。`main`は**途中**: `for`ループが`waterDropToUV`を出すだけで、ループの外の計算(定数`water_drop_pos`/`water_drop_radius`を使ったまま)とつながっていない。**ビルドが通らない状態**(`waterDrops[i].pos`は構造体のメンバ名が`waterDropPos`なので存在しない。C++側は`pos`、HLSL側は`waterDropPos`と名前が違う。どちらかに揃える)。
- **次(ユーザー)**: ①メンバ名を揃える ②`WaterDropData drop = waterDrops[i];`のようにローカルに取って(補完が`.`で出ないため。`Ctrl+Space`で出る)、ループの中に今の計算(定数をdropの値に置き換えて)を移す ③色は、ループに入る前に元の色を持ち、水滴ごとに更新(重なったら後が上書き) ④半径が0の水滴は飛ばす(0割り防止) ⑤`static const`は実行時の半径で計算できないので、半径に比例するもの(`water_drop_light_radius`=半径×0.5、光の位置のずらし)は、割合だけ定数にして、ループの中で`drop.radius`に掛ける(`water_drop_highlight_offset`は元から「半径の何割」なのでそのまま)。
- 環境メモ: Shader validatorの補完は`waterDrops[i].`の添え字のあとだと一覧が出ない。ローカル変数に取ってから`.`+`Ctrl+Space`なら出る。

### 2026-11-13 水滴のループが完成(ユーザー実装、ゲームでの確認はこれから)
- [WaterDropEffectPS.hlsl:112](WaterDropEffectPS.hlsl#L112)でループの前に`col = tex.Sample(smp, uv)`(元の景色)、ループの中は`col = lerp(col, 水滴の中の色, 割合)` → `col.rgb *= 暗さ` → `col = lerp(col, 白, lightRatio)`で水滴ごとに重ねる(前の水滴の結果を残す)。最後に`return col`。
- [118-119行目](WaterDropEffectPS.hlsl#L118-L119)で半径0は`continue`(使っていない12個、0割り防止)。暗さ([140行目](WaterDropEffectPS.hlsl#L140)付近)は`/ drop.radius`に。`colOutWaterDrop`はループの中で15回むだにSampleするので削除。
- つまずいた所: ループの中で宣言した変数はループの外から見えない／毎回`colOutWaterDrop`から作ると最後の水滴しか残らない／`return`は`main`ごと終わる(飛ばすのは`continue`)。
- `shrink_ratio`(2.0)はユーザーが入れ直した(11/12の「丸・反転のみ」から、縮小ありに)。
- **次**: ゲームで3個(半径0.03/0.05/0.07)が出るか確認 → 配置(画面の中央は薄く端に寄せる、被らない) → 外側へなびく動き。

### 2026-11-13 水滴15個のランダム出現と外側への流れ(ユーザー実装、ゲームで確認OK)
- [WaterDropEffect.cpp:111](ShaderEffect/WaterDropEffect.cpp#L111)の`RespawnWaterDrop(i)`: 半径=`(max_radius - min_radius) * GetRandRatio() + min_radius`(「幅×ランダム+下の端」)、位置=UVで0〜1のランダム。`GetRandRatio()`は`GetRand(rand_resolution)`(int 1000)をfloatにして割る。
- [WaterDropEffect.cpp:56-80](ShaderEffect/WaterDropEffect.cpp#L56-L80)の`Update()`: 中央(`screen_center_uv`)→水滴のベクトル(B−A)を`Normalized()`して`screen_out_speed`(0.001)を掛けて足す。x/yのどれかが0〜1の外に出たら`RespawnWaterDrop(i)`。[GameScene.cpp:502](Scene/GameScene.cpp#L502)で毎フレーム呼ぶ。
- つまずいた所: 半径の式(`min + rand / max`→`min + max*rand`→`+min`忘れ)、向きを`1.0f - 中央`で作ろうとした(全部同じ向きになる、ピクセルとUVの混在)。
- 残り: [WaterDropEffect.cpp:19](ShaderEffect/WaterDropEffect.cpp#L19)の`max_water_drop_count = 10`は.hの15と二重(クラス内は15が使われる)、消す。
- **次**: 端で水滴がパッと消える/パッと出る対策(判定に半径を入れる、出現時にじわっと出す) → 中央を避けた出現(角度+下限つき距離) → 横長画面の補正。
- 追記: 流れを見たら「2Dの水滴ではなく3Dの水泡に見える」。原因候補は①中央から放射状・一定速度の動き(カメラが空間の物を通り過ぎる見え方)②5倍縮小+反転+強いハイライトの球っぽさ。切り分け(速度0で見る)と「レンズに付くかキャノピーに付くか」の決定は未。
- 光が真っ白すぎたので[WaterDropEffectPS.hlsl:103](WaterDropEffectPS.hlsl#L103)で`lightRatio`に光の強さを掛けて上限を下げた → 「めちゃいい感じ」。ユーザーの依頼でClaudeが定数`water_drop_light_strength`(0.3、[45行目](WaterDropEffectPS.hlsl#L45))に出した。
- **(ユーザー判断)3D感はなくなったので、水滴はこれで完了。** 残りの小さい直し(.cppの`max_water_drop_count`の二重、端でパッと消える等)も触らなくてよい。

### 2026-11-13 2面の曇り空スカイボックスを作った(画像だけ。コードはユーザーが書く)
- ユーザー決定: 暗さは「中間の青灰色」。雨の再生コードとスカイボックスの切り替えコードは**ユーザーが書く**(Claudeは手順と名前の候補)。
- 画像: `Data/Image/SkyBoxCloudy/`に6枚(1024×1024、ファイル名は昼と同じ`skybox_front.png`など)。生成は[make_cloudy_sky.ps1](Data/Image/SkyBoxCloudy/tools/make_cloudy_sky.ps1)(C#埋め込み、**UTF-8 BOM付きで保存**、1秒程度)。雲は方向から計算する(高い平面へ投影したノイズ)ので面のつなぎ目で切れない。面の向きは[SkyBox.cpp](Game/BackGround/SkyBox.cpp#L60-L107)の頂点の並びから逆算。
- 色: 地平線のかすみ(168,174,183)、真上(120,128,140)、雲の厚い所(100,107,120)〜薄い所(168,174,183)、地平線より下(112,118,127)。**フォグを入れるならフォグの色を地平線の色(168,174,183)に合わせると、遠くのビルが空に溶ける。**
- 確認(ゲームのカメラ相当、縦90°・1280×720、6枚をSkyBox.cppと同じ貼り方で読む): [正面](Data/Image/SkyBoxCloudy/tools/review/gamecam_front.png)、[面のつなぎ目をまたぐ向き](Data/Image/SkyBoxCloudy/tools/review/gamecam_seam_yaw45_pitch35.png)。貼り方の逆算は、ゲームで正しく出ている昼の空を同じ読み方で見て、つなぎ目が切れていないことで検算([昼](Data/Image/SkyBoxCloudy/tools/review/check_day_seam_yaw45_pitch35.png))。1回目は雲が大理石のようにうねりすぎたので、ゆがみを弱めて大きくぼんやりした固まりにした。
- コード側の手順(ユーザーが書く): ①ResourceConstantsに曇りの6枚のパス ②ResourceLoaderにGraphicIDと表、`game.graphics`に追加 ③StageConfigに「雨のステージか」 ④GameSceneでSkyBoxを作るときに切り替え。雨: ①EffectIDとパス・倍率1.0、`game.effects`に追加 ②雨のステージだけ開始時に`Play`してハンドルを持つ ③毎フレーム[GameScene.cpp:676](Scene/GameScene.cpp#L676)の`m_pEffectManager->Update()`の前で`SetPos(ハンドル, カメラの位置)`(カメラはGameObjectなので`UpdateAll`の後なら最新)。
- 未確認(ゲーム内): ビル・雨と重ねたときの明るさ、敵や弾が空に埋もれないか。
- **(ユーザー依頼でClaudeが実装)パスとIDの追加**: 曇りの6枚のパス`skybox_cloudy_*_path`([ResourceConstants.h:96-101](Constants/ResourceConstants.h#L96-L101))、`rain_effect_path`([199行目](Constants/ResourceConstants.h#L199))・`rain_effect_scale`(1.0、[266行目](Constants/ResourceConstants.h#L266))。GraphicID`SkyBoxCloudyFront`〜`SkyBoxCloudyBottom`([ResourceLoader.h:105-110](Manager/ResourceLoader.h#L105-L110))、EffectID`Rain`([181行目](Manager/ResourceLoader.h#L181)))。表は[ResourceLoader.cpp:58-63](Manager/ResourceLoader.cpp#L58-L63)と[244行目](Manager/ResourceLoader.cpp#L244)、`game.graphics`([531-532行目](Manager/ResourceLoader.cpp#L531-L532))と`game.effects`([559行目](Manager/ResourceLoader.cpp#L559))に追加。Debug x64のビルドOK。`KeepEffect()`(古い一括読み込み)はどこからも呼ばれていないので足していない。
- **残り(ユーザーが書く)**: StageConfigに「雨のステージか」→GameSceneでSkyBoxに曇りのIDを渡す、雨の`Play`とハンドル保持、毎フレーム`SetPos(カメラの位置)`。
- **(11/17・ユーザー実装)スカイボックスの切り替えが完成**: 最初のコミット(c9c7fd6)はif/elseが逆+`m_isRainStage`未設定で、両方重なって1面も2面も曇りだった。[GameScene.cpp:229-250](Scene/GameScene.cpp#L229-L250)を入れ替え、[StageConfig.cpp:24](Constants/StageConfig.cpp#L24)に`isRainStage`、表([30-31行目](Constants/StageConfig.cpp#L30-L31))の1面false・2面true、[52行目](Constants/StageConfig.cpp#L52)で`m_isRainStage`に入れた。残りは雨の再生。
- **(11/17・ユーザー依頼でClaudeが変更)雨の主張を強めた**: 雨の色が曇り空の地平線(168,174,183)に近く、近い層も細くて溶けていた。方針は「端と手前は強く、中央と奥は抑える」(『やり過ぎてちょうどよい』と『判定のあるものを強調する』の両立)。[build_rain_efkproj.ps1:12-17](Data/Effect/Rain/tools/build_rain_efkproj.ps1#L12-L17)で、近い層 幅0.05→0.08・長さ2.0→2.6・色(195,205,218,110)→(230,235,240,170)、中くらいの層 色(190,200,214,80)→(225,230,236,110)。遠い層はそのまま。量(発生間隔)は変えていないのでインスタンス数も同じ。`Rain.efkproj`を作り直し、1.80.6の`EffekseerCore.dll`で倍率50の`Rain.efk`(3744バイト、数値の所だけ差分)を書き出した。比較画像(エディタ静止カメラ・曇り空の色に合成、上=前/下=後): [rain_compare_before_after.png](Data/Effect/Rain/tools/review/rain_compare_before_after.png)。**未確認(ゲーム内)**: ビル・フォグと重ねたときの見え方、弾や敵が埋もれないか。
- **(11/17)コスト表を更新(ユーザー依頼)**: 「移動の加速度」(2h)と「カメラレンズの水滴」(8h)を完了、実コストは見積と同じ(ユーザー「大体同じ」)。残コストはプロト1・アルファ47.5・ベータ41(計89.5)。アルファの締め切りは12/3(カレンダー)。
- **(11/17・ユーザー実装)雨の再生が完成**: ハンドル`m_rainEffectHandle`(-1で初期化、[GameScene.h:105](Scene/GameScene.h#L105))。雨のステージのときだけ`Play(EffectID::Rain, カメラの位置)`([GameScene.cpp:254-258](Scene/GameScene.cpp#L254-L258))。毎フレーム、エフェクトの更新の前に`SetPos(カメラの位置)`([699-703行目](Scene/GameScene.cpp#L699-L703)、`>= 0`で判定)。止める処理はEffectManagerのデストラクタの`StopAll`に任せている。最初はIDが`PlayerBullet`のままだったのを`Rain`に直した。ゲームで確認済み(ユーザー「いい感じだった」)。**雨のステージ(曇り空+雨)はこれで完成。**
- **(11/17)コスト表に2行追加(ユーザー依頼)**: 「ステージ ー2面の雨と曇り空」(79行目、見積1・実1・S・アルファ・完了)、「エフェクト ー機体後部の十字の光(スターフレア)の組み込み」(93行目、見積1・S・アルファ・未完了)。残コストはアルファ48.5(計90.5)、1日に必要なペース2.08に対して今の平均2.10。表のフィルター(S・アルファ・未完了)はそのままかけ直した。

### 2026-11-16 機体後部の十字の光芒(スターフレア)エフェクトを作った(エフェクトのみ。コードはユーザーが書く)
- 依頼: 噴射口に付く十字の光芒。芯(白)+縦横の光条(青白→水色、先端は透明)+ごく薄いグロー。加算。ビルボードにせず機体に固定、直交する板で全方向から見える。90F周期の明滅(±12%)と光条の伸縮(±8%)、乱数なし。ブースト版は光条1.5倍・明るさ約1.25倍。一気に作る。
- ファイル: [StarFlare.efkefc](Data/Effect/StarFlare/StarFlare.efkefc) / `StarFlare.efk`、[StarFlareBoost.efkefc](Data/Effect/StarFlare/StarFlareBoost.efkefc) / `StarFlareBoost.efk`(どちらも拡大率50で書き出し)、`Texture/SF_Ray.png`・`SF_Core.png`・`SF_Glow.png`(自作、生成は[gen_starflare_textures.ps1](Data/Effect/StarFlare/tools/gen_starflare_textures.ps1))。確認画像は[compare_angles.png](Data/Effect/StarFlare/tools/review/compare_angles.png)(通常/ブースト×5方向)と[compare_bg_roll.png](Data/Effect/StarFlare/tools/review/compare_bg_roll.png)(暗い背景・ロール40°)。
- 座標の前提: **原点=噴射口**。エディタの+Zが機体の後ろ(ブーストと同じ、回転は`(rotX, rotY+π, …)`想定)。仮の機体`tools/DummyJet.efkmodel`(`Ship`ノード、z=-1.25)は書き出し前に外してある。寸法はブースト制作時の推定の機体幅3.2(翼端±1.6=ゲームで±80)が基準: 横の光条5.6(1.75倍)、縦3.64(横の65%)、芯の直径約0.55(横の約1/10。仮の機体の胴が細く、1/8だと胴を覆ったため小さくした)。
- 構成: `Flare`(None)の下に`Glow_XY/XZ/YZ`、`Rays`(None、伸縮のFCurve)の下に`RayH_XY`・`RayH_XZ`(横の光条)・`RayV_XY`・`RayV_YZ`(縦の光条)、`Core_XY/XZ/YZ`。すべて`Billboard=Fixed`、`MaxGeneration=1`・`Life=100000`。後ろ向きの板(XY)が主、ほかの面はalphaを約6割にして、斜めで重なっても明るくなりすぎないようにした。ブースト版だけ`BoostGrow`(None)があり、開始8Fで光条が0.67倍→1倍に伸びる(差し替えたときのパッと感を減らす)。
- 確認済み(エディタ): 背面・斜め・真横・正面・真上・ロール40°で十字が消えない(真横は縦の線+芯、真上は横の線+芯が残る)。明るい灰色(曇り空相当)と暗い紺の背景の両方で見える。0Fと90Fが同じ見た目でループする。
- 気づいた点: カメラは機体の後ろなので、光は機体より手前にあり、**深度テストでは機体を守れない**(芯の大きさと明るさで抑えている)。ブースト版の横の全長は8.4(機体幅の約2.6倍)で、通常版の上限(2倍)を超える(依頼の1.5倍を優先)。
- 未確認(ゲーム内): 実機の大きさと噴射口の位置、明るさ、ロールの向き(`SetRotation`にZを渡したとき逆回りなら符号を反転)。
- **(2026-11-16 続き)ユーザー指摘「板感がある」「主張が強すぎる、もう少し薄めでいい」→ 直した。** 板感の原因は、①芯の画像の縁がくっきりしていて、斜めから見るとコインのような円盤に見えた ②光条の幅が広く、縁の形(紡錘形)がはっきりしていて、斜めから見ると平たい刃に見えた、の2つ。芯を縁のないなだらかな山(ガウス)に、光条の断面をなだらかなベル形にして、にじみを0.12→0.04、太さを0.4→0.32にした([gen_starflare_textures.ps1](Data/Effect/StarFlare/tools/gen_starflare_textures.ps1))。明るさは全体を約0.75倍に、背面以外の面(XZ/YZ)は主の面の約5割(光条)・約4割(芯)に下げた。長さは変えていない。ブースト版も同じ割合で直した(通常版の約1.25倍の明るさのまま)。両方とも拡大率50で上書きして書き出した。
- 比較: [compare_before_after.png](Data/Effect/StarFlare/tools/review/compare_before_after.png)(前/今回、同じカメラ・45F)。古い確認画像(compare_angles/compare_bg_roll と撮った画像)は消した。
- まだ強いと感じたら、次は芯のalphaを下げる(光条はすでに細い)。ゲーム内での見え方は未確認。
- **(2026-11-16 続き2) ユーザー指摘「原作だと画面にかかっているエフェクトに見える。シェーダーを書く暇はないので、それっぽく作ってほしい」→ 画面に貼り付いて見える作りに変えた。** 上の「ビルボードにしない・直交する板」の方針は、ここで取りやめた。
  - 全スプライトを`Billboard=Billboard`(常にカメラ正面)、`ZTest=false`(深度テストなし、機体や地形に隠されない)にした。XZ/YZの副の板は消して、`Glow`・`Rays/RayH`・`Rays/RayV`・`Core`の1枚ずつにした。どの角度から見ても同じ形になり、板の傾きがなくなったので板感も出ない。
  - **十字は画面に固定(ロールしても傾かない)。** `Billboard`はノードの回転を無視するため。縦の光条をZ回転90°で作ると回転を無視して横になってしまう(`RotatedBillboard`にすると親の回転についてくるので、ロールで横と縦がずれる)。そこで縦向きの画像`Texture/SF_RayV.png`(SF_Rayを90°回したもの、生成スクリプトで一緒に作る)を足し、縦の光条は回転0・スケール(0.32, 3.64)にした。
  - レンズっぽさとして、細く長い横の線`Rays/Streak`(SF_Ray、スケール7.0×0.12、alpha 55→70)を足した。ブースト版は10.5×0.12、alpha 69→88。
  - ブースト版も同じ作り(`BoostGrow`の伸びはビルボードでも効く)。どちらも拡大率50で上書きした。
  - 確認画像: [screen_overlay_angles.png](Data/Effect/StarFlare/tools/review/screen_overlay_angles.png)(後ろ/斜め/真横/真上/ロール40°/明るい背景)、[screen_overlay_boost.png](Data/Effect/StarFlare/tools/review/screen_overlay_boost.png)。[compare_before_after.png](Data/Effect/StarFlare/tools/review/compare_before_after.png)は板の版どうしの比較として残している。
  - **コード側への影響**: エフェクトの向きは見た目に効かないので、`SetRotation`は不要(位置だけ`SetPos`で噴射口に合わせる。噴射口の位置は機体の回転を使って計算する)。
  - 未確認(ゲーム内): 深度テストを切ったので、エフェクトを描く順番より前に描いたものにはすべて上から重なる。UIより後に描かれていたらUIにもかかる。画面に出したときの大きさ。

### 2026-11-16 移動の加速・減速を入れた(ユーザー実装、ゲームで確認OK「良い感じだった」)
- 考え方: 1フレームに変えられる速度に上限(step)を付けて、目標速度へ近づける。`cur = cur + clamp(target - cur, -step, +step)`。増える・減る・負の向き、どれでも同じ式で、行き過ぎない。`step = 最高速度 / 最高速度までのフレーム数`。最高速度は**定数**の`move_speed_xy`を使う(`targetVel`はスティックの傾きで変わるので使わない)。
- 加速: [MovingState.cpp:31](Game/GameObjects/Actors/Character/Player/Movement/MovingState.cpp#L31)の`accel_frame`(20F)、式は[MovingState.cpp:105-112](Game/GameObjects/Actors/Character/Player/Movement/MovingState.cpp#L105-L112)。`targetVel`(目標)と`vel`(近づけた結果)を分けて、`SetVel(vel)`に渡す。最初は`SetVel(targetVel)`のままで効いていなかった。
- 減速: [IdleMovementState.cpp:22](Game/GameObjects/Actors/Character/Player/Movement/IdleMovementState.cpp#L22)の`stop_frame`(24F)。目標を0にして同じ式。**`Enter`は切り替えの瞬間に1回しか呼ばれないので、毎フレームの減速は`Update`に置く**(`Enter`は空。離した瞬間の速度を引き継ぐ)。`ChangeState`は次のステートを保存するだけで自分を破棄しないので、その後も計算を続けて問題ない。
- `move_speed_xy`(18)はIdle側にも同じ値を置いた(Moving側と二重。直すときは両方)。zは`move_speed`を18にそろえて、Idleに入るたびに落ちる問題をなくした。
- 未実装(依頼にあった「傾いているときだけ加速度を大きくする」): 傾き中は`tilt_move_speed_x`(25)が最高速度なので、`step`の基準も25にそろえる必要がある。仮の値は6フレーム。**今回は見送り**。
- ほかの状態(ブースト・ブレーキ・宙返りなど)は速度を直接入れているので、加速の対象外。画面端のクランプで速度を0にするかも未確認。
- 追加したタスク: 「HPゲージの表示ズレ修正(画像の余白を計算に含めている疑い)」1.5h、優先度A、アルファ。敵とプレイヤーのHPが、最初と最後で減っていないように見える。

### 2026-11-17 チャージ中のエフェクト`Charging`を作り直した(v6D「黒縁なしの濃淡」)
- 依頼: 主張が弱く「溜めている」「溜まった」が一目でわからない。明るい背景(明るい海・空)で見えにくい。暗い場所でも明るい場所でも同じように見せたい。コードは変更しない。
- 原因(ゲームのカメラを再現して確認): ほぼ全部が加算合成で、明るい空の上では玉がほぼ消えていた。暗い下地`Backing`はぼかしたテクスチャ×アルファ90で、どの背景でも効いていなかった。さらに、溜め中(5〜15F)と保持(35〜40F)の見た目がほぼ同じで、「溜まった」が読めなかった。
- ゲームのカメラの再現方法: カメラは機体の166後方([GameCamera.cpp:19](Game/GameObjects/Camera/GameCamera.cpp#L19))、玉は機体の200前方([ChargeShootState.cpp:22](Game/GameObjects/Actors/Character/Player/Shoot/ChargeShootState.cpp#L22))、前進中は追従の遅れ(lerp 0.09、8/F)で約81離れる→玉まで約447(エディタ8.9単位)。自機の高さ500ではカメラと同じ高さで画面の中央。**MCPのビューアーは距離10で1単位=44px(キャプチャはビューアーの等倍の切り出し)。ゲーム(1280×720、縦90°)と同じ大きさで見るには、エディタの距離=ゲームの距離×1.22**(今回は10.9、正面)。背景は白っぽい空(225,235,240)・明るいティールの海(80,190,185)・中間(70,100,120)・ほぼ黒(8,10,12)、乱数の種は`@effect`の`Global.RandomSeed`=7で固定して比べた。
- 経緯: A(深緑の円盤の下地)/B(白黒の破片)/C(セル調)の3案からAを選んで一度仕上げたが、「雑な下地はやめて」「ダサい。黒い縁がついただけに見える」。下地を球の陰影(A2)・プラズマの影(A3)にしても黒い縁の路線のままだったので、**黒い縁を一切やめて、形の中を深い緑→明るい緑→白い芯の濃淡で描くD**に作り直した。
- **学び: 「暗い色を混ぜる」は形に黒い線を付けることではない。全部の形に同じ黒っぽい縁を付けると、シールを縁取っただけに見えてダサい。暗い色は形の本体の色(深い緑)として入れ、芯へ向かって明るくする。** 一番暗い色も緑(0,82,42)にとどめ、黒にはしない。縁は線で切らずにぼかす。
- 構成(v6D、描画順): `BackingGrow`/`Backing`(CS6T_Orb＝渦を巻くエメラルドの玉、中心は透けて縁ほど深い緑、ゆっくり回転、0.6→2.5倍)→`Halo`→`Wisp`/`WispCharged`→`Inflow`→`Arc`→`Core`→`Star`→**`GaugeRing`(CS6T_Ring、6.0→2.7倍、EaseInCubicで0〜20Fに外から閉じてくる＝溜めの進み具合)**→**`HoldRim`(20F以降、2.7倍で固定)**→**`HoldPulse`(28Fから15Fごとに2.7→4.3倍へ広がって消える)**→20Fの一拍: `ChargedRing`(2.7→9.0倍)＋`ChargedRingGlow`(同じ輪の加算版)、`ChargedBurst`(CS6T_Burst、白い芯から深い緑へ細くなる八芒星、7.0→2.4倍・9F)、`ChargedFlash`、`ChargedSparks`。
- 新しいテクスチャ`CS6T_Ring`(深い緑の帯の中を明るい線が通る輪)・`CS6T_Burst`・`CS6T_Orb`。既存の`CS_*`はチャージショットと共用なので触っていない。通常合成で描くので明るい背景でも形が残り、暗い背景では芯と明るい線が光る。暗い背景で輪を半透明にすると明るい線が灰色にくすむので、`GaugeRing`は最初からアルファ210で出し、広がって消える`ChargedRing`には加算の`ChargedRingGlow`を重ねた。
- ファイル: `Charging.efk`・`.efkefc`を上書き(拡大率50、乱数の種は-1に戻して保存)。前の版(v5)は`Charging_v5/`に退避(CS_*テクスチャも複製したので、そのフォルダだけで開ける)。イージング番号は20/21/31だけ。生成に使ったスクリプトと確認画像は、別の作業で`Data/Effect/Charging/tools/`ごと消した(ユーザーの意図どおり。Data/にはゲームで使うものだけを置く)ので残っていない。最後の2か所の直しはMCPでエディタ上で直接行った。
- **見つけた不具合(v5から): 旧`make_efkproj.ps1`で`Spin -9 2`のように負の数を関数に渡すと、文字列"-9"として渡り、`"-9" + 2`が"-92"に連結される。** v5の`WispCharged`の回転速度は-11〜-7°/Fのつもりが-92〜-11°/Fになっていた。v6では-11〜-7°/F。**チャージショット`PlayerChargeBullet`の`Wisp2`(`Spin -14 3`)にも同じ不具合がある見込み(-143〜-17°/F)。今回は触っていない。**
- 根拠(桜井さんの動画): 『モヤっとさせるな【エフェクト】』(加算に頼りすぎない、暗い色を混ぜる)、『ヒットマーク【エフェクト】』(明度差、複数要素)、『キャラクターを際立たせる【エフェクト】』(中央を遮らない)。
- 未確認(ゲーム内): 手前の機体との重なり、チャージ中に始まるロックオンのレティクルと閉じてくる輪が紛らわしくないか、離したときの縮小(`SetScale`)での消え方。前の版の「気になった点」(離すと玉がすぐ消える、など)はそのまま。

### 2026-11-17 ボスの残り作業の仕様(ユーザー決定。コードはユーザーが書く)
- **頭の赤い点滅 = 顔(黄色く光る部分)だけを赤くする。方法はテクスチャの切り替え。** 発光マップ`Data/Model/Boss.fbm/T_Mech_LOD2_E.png`(4096×4096)の左下寄り(UVで横0.24〜0.38・縦0.68〜0.79)の大きな形が顔(ベース色`T_Mech_LOD2_B.png`の顔のプレートと同じ位置)。[BossEnemy::DrawEnemy](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L352-L359)で渡す`emissionGraphH`を、点滅中だけ赤い版にする。開始は[TakeDamage](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L293)(頭のダメージ判定のときだけ呼ばれる。シールドは`OnHitInvincibleCol`)。ベース色の顔は黄〜オレンジなので、発光だけ赤くするとオレンジに見える可能性あり→そのときはベース色も`MV1SetTextureGraphHandle`で替える。赤い版の画像はClaudeが作れる(顔だけか、ほかの黄色も含むかは未決定)。
  - **(ユーザー依頼でClaudeが実装)赤い版の画像とパス・登録**: 画像は`Data/Model/Boss.fbm/T_Mech_LOD2_E_Damage.png`(2048×2048。顔の範囲=元の4096でx950〜1620・y2740〜3260だけ、明るさを保って赤(255,25,20)に置き換え。右隣の黄色い丸などはそのまま)。生成は[make_damage_emission.ps1](Data/Model/Boss.fbm/tools/make_damage_emission.ps1)(`-Size 4096`で元と同じ大きさにもできる)。パス`boss_emission_damage_path`([ResourceConstants.h:122](Constants/ResourceConstants.h#L122))、GraphicID`BossEmissionDamage`([ResourceLoader.h:128](Manager/ResourceLoader.h#L128))、表([ResourceLoader.cpp:86](Manager/ResourceLoader.cpp#L86))、`game.graphics`([ResourceLoader.cpp:539](Manager/ResourceLoader.cpp#L539))。Debug x64のビルドOK。
  - 見え方の予想([damage_preview.png](Data/Model/Boss.fbm/tools/review/damage_preview.png)、ベース色×明るさ0.3/0.7＋発光、左から 元/赤 ×2、下段は発光だけ): 光が弱い所では赤寄りのオレンジ、強い所ではオレンジ。ベース色の顔が黄色いので、発光だけでは真っ赤にはならない。黄色との違いははっきりわかる。
  - **ベース色も赤くする案はやめた(ユーザー決定)。** 試しにベース色の顔も赤くした版を作ると、光の強さに関係なくはっきり赤になった。ただしベース色はモデル自身のテクスチャなので、`MV1SetTextureGraphHandle`で差し替えるコードが要る(元に戻すときは画像に`-1`)。画像とスクリプトは削除し、発光だけの版に戻した。
  - **(ユーザー実装)点滅ができた(ビルドOK、ゲーム内は未確認)**: `m_damageBlinkingFrame`([BossEnemy.h](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.h))を`TakeDamage`で1にして開始、`Update`で`blinking_frame`(30F)を超えたら0に戻す。`DrawEnemy`で、通常の発光と`BossEmissionDamage`を交互に渡す。
  - 気づいた点(未変更): 切り替えの条件`m_damageBlinkingFrame % blinking_interval * 2 < blinking_interval`は、`%`と`*`が同じ優先順位で左から計算されるので`(frame % 10) * 2 < 10`、つまり5Fごとの切り替えになる。10Fごとのつもりなら`frame % (blinking_interval * 2) < blinking_interval`。点滅のカウントは`m_isAppear`の中で進むので、死亡中は進まない(最後の一撃で1になる→通常の発光のまま止まる)。
  - **生成スクリプトと比較画像(`Data/Model/Boss.fbm/tools/`)はコミットしていない。** 「Data/にはゲームで使うものだけを置く」方針(下の「Data/Effect の未使用素材と tools/ を削除した」)に合わせた。ローカルにだけある。
- **死亡時のまわりのエフェクトは、ゆっくりフェードして消す。** 原因: 死亡待機中([BossEnemy.cpp:205-221](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L205-L221))はステートの`Update`/`Exit`が呼ばれず、ビームは[BossBeamStateのデストラクタ](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossBeamState.cpp#L67-L83)でしか止まらない→ボスが消えるまでビームが残る。前例: ボスに当たったビームを消す[FadeOutHitBeam](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossBeamState.cpp#L433-L452)(長さは`hit_boss_fade_frame`=30F固定)。
- **死亡中はUIを消す。** UIは全部[GameScene.cpp:786](Scene/GameScene.cpp#L786)の`m_pUIManager->Draw()`で描いている(照準・HP・ボスHP・特殊ゲージ・WARNING)。
- **見つけた問題: ボスの死亡待機中もビームの当たり判定が生きている。** [BossBeamCollider.cpp:29-33](Game/Collision/SpiderBossBeamCollider.cpp#L29-L33)と[BossBeamTipCollider.cpp:96](Game/Collision/SpiderBossBeamTipCollider.cpp#L96)は`IsDead`だけを見ていて`IsDying`を見ない。ほかの敵は`GetColliders`で死亡中なら空を返している(例: [RushEnemy.cpp:145](Game/GameObjects/Actors/Character/Enemy/RushEnemy/RushEnemy.cpp#L145))。ボスを倒した後にビームでプレイヤーが死ぬと、ゲームオーバーになるおそれ(未確認)。
- **2026-11-17続き: 輪を全部外した。** ユーザー「わっか見たいなやついらない。ダサい」。`GaugeRing`・`HoldRim`・`HoldPulse`・`ChargedRing`・`ChargedRingGlow`を削除し、使わなくなった`CS6T_Ring.png`も消した(ごみ箱)。残りは、深い緑の玉(`Backing`)が0〜20Fで大きくなる＋プラズマ・吸い込まれる火花・芯、20Fの一拍は`ChargedBurst`(八芒星)・`ChargedFlash`・`ChargedSparks`。4つの背景で玉と20Fの星は見える。**輪が受け持っていた「溜めの進み具合」と「溜まった後」の表示がなくなったので、15F(溜め中)と25F以降(保持)の見た目が近い。** →次の項目で、輪以外の方法を足した。
- **2026-11-17続き2: 保持中に玉が脈打つようにした(輪の代わり)。** 大きさの変化は明るい背景でも暗い背景でも見えるので、通常合成の玉`Backing`と加算の芯`Core`の大きさを、FCurve(フレーム指定、最後をLoop)で18Fごとに膨らませて戻す。玉は2.5→3.0→2.5倍、芯は1.5→2.0→1.5倍(9Fで最大)。どちらも20Fに出るので同じ位相で動く。溜め中は玉が大きくなり続け、保持中はその場で脈打つ、という動きの違いで見分ける。

### 2026-11-17 Data/Effect の未使用素材と tools/ を削除した
- ゲームが読む`.efk`は20個([ResourceConstants.h:168-201](Constants/ResourceConstants.h#L168-L201)、[ResourceLoader.cpp:230-262](Manager/ResourceLoader.cpp#L230-L262)の表)。その`.efk`から参照されているテクスチャ・モデル・マテリアルは104件で、全部残っていることを確認した。
- 削除した素材(どの`.efk`からも参照されていない): `StarFlare/StarFlareBoost.efk`、`BossBeam/Texture/BB_RimDisc2.png`、`Charging/Tex/`(Particle01.png)、`ChargeExplosion/Texture/CS_Spark.png`・`CS_Wisp2.png`、`EnemyBullet/Texture/Particle01.png`、`HitEffect/Texture/Particle01.png`・`Ring.png`、`PlayerBullet/Texture/Particle01.png`・`Thunder01.png`、`PlayerChargeBullet/Texture/CS_Spark.png`・`Particle01.png`、`Splash/Parts/Ring.png`。
- 削除したソース: 全エフェクトの`tools/`(生成スクリプトとreview画像、約12.9MB)。ただし`Boost/tools/DummyJet.efkmodel`と`StarFlare/tools/DummyJet.efkmodel`は`.efkefc`がプレビュー用に参照しているので残した。`.efkefc`/`.efkproj`はすべて残した(`StarFlareBoost.efkefc`も)。
- 戻し方: git管理下の分は`git checkout -- Data/Effect`で戻せる。**gitに入っていなかったCharging v6のtools(`gen_charge_tex_v6.ps1`・`make_charging_v6.ps1`・`compose_v6.ps1`とv6のreview画像)と、Rainのreview画像3枚は戻せない。** 削除の時点でCharging v6が別のセッションで作業中だったことに後から気づいた(そのセッションには連絡済み)。`Charging/Texture/CS6_*`は残している。
- 確認: Debug x64でビルドが通った。起動してタイトル画面で15秒止まらないことを確認した。エフェクトはシーンごとに読み込まれるので、ゲームシーンのエフェクトはゲーム内では未確認(参照先がすべて存在することはファイルで確認済み)。

### 2026-11-17 ボス死亡中はUIを消す・水滴を止める(ユーザー実装、ゲームで確認OK)
- UI: [GameScene.cpp](Scene/GameScene.cpp)の`Draw`で、`m_pBoss->IsDying()`のときは`m_pUIManager->Draw()`を呼ばない。
- 水滴: 死亡エフェクト(画面が真っ白になる)の上に水滴が乗るのが嫌 → **死亡中は水滴の工程を飛ばす**(方法A)。原因は描画順で、死亡エフェクトも`m_pEffectManager->Draw()`でブースト用の画面に描かれ、その絵ごと水滴のシェーダーを通っていた。飛ばすときは`m_pBoostingEffect->ChangeNormalRT()`で描画先をバックバッファに戻してから`m_pBoostingEffect->Draw`(ブーストしていなくてもDrawGraphで描く)。水滴は死亡待機に入る瞬間に消えるが、問題なし。方法B(死亡エフェクトだけ水滴の後に描く)は、EffectManagerにエフェクトごとの描き分けが要るので見送り。
- **つまずき: `m_pCamera->SetUpCamera()`をUIの描画より後ろに置いたら、レティクルが機体の回転についてこなくなった。** レティクルは`ConvWorldPosToScreenPos`(そのとき設定されているカメラを使う)で位置を出す。水滴・ブーストで画面を切り替えるとカメラの設定が崩れるので、**UIを描く前に`SetUpCamera()`が必要**。UIの前に戻して直った。
- ボスの点滅の条件式を`frame % (blinking_interval * 2) < blinking_interval`に直し、間隔を10→5にした(ユーザー)。点滅は問題なし。
- コスト表: 「ボスの頭を攻撃したら赤く点滅」「スターフレアの組み込み」を完了(実コスト=見積の1h)。「ステージ1ボス」の下に「死亡中はUIを消す・水滴を止める」(1h・S・アルファ・完了)を追加。コスト表の「ボス(仕様未定)」は2面のボスの行なので、1面の作業は紐づけない。
- **次**: 「ボス死亡時のまわりのエフェクトを消す」(1.5h・S)。ビームは死亡待機中に止まらない([BossBeamStateのデストラクタ](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossBeamState.cpp#L67-L83)でしか止まらない)。ゆっくりフェードして消す(前例: `FadeOutHitBeam`)。ビームの当たり判定が死亡中も生きている問題は別。

### 2026-11-17 ボス死亡時のまわりのエフェクト(ビーム)を消す: 設計の途中(コードはまだ。ユーザーは集中が切れたので今日はここまで)
- 目的: 死亡待機に入ったらビームを**ゆっくりフェードして消す**(コスト表「バグ修正＞ボス死亡時のまわりのエフェクトを消す」1.5h・S・アルファ。未着手)。
- 原因: [BossEnemy.cpp:161](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L161)の`if (!m_isDying)`の中に`m_pState->Update()`があるので、死亡待機中はステートの`Update`が呼ばれず、ビームのエフェクトは[BossBeamStateのデストラクタ](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossBeamState.cpp#L67-L83)でしか止まらない(ボスが消えるまで残る)。
- 前例: [FadeOutHitBeam](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossBeamState.cpp#L433-L453)(`playH`と`fadeFrame`を参照で受け、アルファを下げ、`hit_boss_fade_frame`=30Fで`Stop`して`playH=-1`)。ボスに当たったビームだけ、`Update`内で呼んでいる。
- **決まったこと(ユーザー)**: ステートに知らせる形(前者)。`BossEnemy`が死んだ瞬間に`m_pState`へ伝え、`BossBeamState`だけが自分のビームを消す。ボス側へビームのハンドルを移す案は、変更が大きいので採らない。
- **ユーザーの提案**: 「`BossBeamState`の中でボスが死んだかどうかを確かめればいいのでは」。→ 概念は正しいが、**死亡待機中は`Update`が呼ばれない**ので、そのままでは動かない。
- **分かれ道(未決定。Claudeは A を勧めた)**:
  - **A**: `m_pState->Update()`を死亡待機中も呼ぶ。`BossBeamState::Update`の先頭で、死亡中ならフェードだけして`return`。代償: `IdleState`と`SummonState`の`Update`も死亡中に走るので、**特にSummonは死亡中に敵を呼ぶおそれ** → その2つにも「死亡中なら何もしない」が要る。
  - **B**: `IBossEnemyState`に中身が空の仮想関数`OnBossDying()`を足し(名前候補: `OnBossDying`/`OnDeath`/`NotifyDying`)、死んだ瞬間に1回だけ呼ぶ。ただし毎フレームのフェードを進めるには`Update`相当の呼び出しが要るので、結局どこかで回す必要がある。
- ビームの当たり判定が死亡中も生きている問題(未確認)は別。[BossBeamCollider.cpp:29-33](Game/Collision/SpiderBossBeamCollider.cpp#L29-L33)・[BossBeamTipCollider.cpp:96](Game/Collision/SpiderBossBeamTipCollider.cpp#L96)が`IsDead`だけ見て`IsDying`を見ない。
- **次**: AかBを決める → 選んだ形で実装 → フェードの長さを決める(`hit_boss_fade_frame`の30Fより長くする?仕様は「ゆっくり」) → ゲームで確認(死亡中にビームが残らないか、倒した後にビームでプレイヤーが死なないか)。
- (11/18)↑はA/B未決定のまま。ユーザーは先に2面のボスの仕様決めに移った。

### 2026-11-18 2面のボスの仕様決め(途中。コードはまだ)
- **狙い(ユーザー)**: ちゃんと動き回る。第二形態がある。少し苦戦する。「避けて弾を当てる」をくり返す、しっかりした戦闘のループ。
  - 1面ボスとの違い: 1面はプレイヤーの前進の速さだけを写してついてくる(x・yは0、[BossEnemy.cpp:166-170](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L166-L170))。攻撃は3秒ごとに召喚かビームをランダムで選ぶ([BossIdleState.cpp:41-55](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossIdleState.cpp#L41-L55))。
  - 根拠: ループは「予兆→攻撃→隙」(『攻撃モーションの内訳』『かまえは瞬時に極端に』)。第二形態はHPを増やすのではなくリズムか動き方を変える(『単調な坂は山登りにならない』『ギュッとしてパッと解消』)。苦戦させるならやり直しは速く(『リトライは迅速に』)→今はゲームオーバーからのリトライでステージの最初に戻る。ボスの前からやり直せる仕組みは別の作業として検討。
- **第一形態の主役の攻撃(ユーザー決定)**: ボスの前の空間に、右上→左下と左上→右下の斜めのレーザーを何本も並べて**バツ型の網目**を作る。**網はその場に置いたまま**(プレイヤーは前進し続けるので、網が壁のように近づいてくる→網目のすき間をくぐり抜ける。※Claudeがこう受け取った、ユーザーの否定はなし)。
  - 案: 「細い線で予告→太くなって当たり判定あり」の2段階(【予告の長さ】【判定がある長さ】【消えるまでの長さ】)。曇り空・雨で埋もれないよう、芯は明るく外側に暗い層。網が自分の所に来るタイミングの手がかり(地面に光の線など)。
  - 公平さ: プレイヤーの当たり判定は半径50([Player.cpp:50](Game/GameObjects/Actors/Character/Player/Player.cpp#L50))。網目のひし形に内接する円の半径=(平行な線の間隔−レーザーの太さ)÷2 を50より十分大きく。飛べる範囲(左右±1600・高さ100〜900)のどこを通っても網目が1つ以上あるように。
  - 当たり判定: 今の形は球とメッシュだけ([ColliderShape.h:6-9](Game/Collision/ColliderShape.h#L6-L9))→カプセル(線分×球、DxLibの`HitCheck_Sphere_Capsule`)を足すのが自然。1本ずつのオブジェクトか網1枚で1つかは実装時に決める。
- **隙の形(ユーザー決定)= A「撃った後に放熱する」**: 網を撃ち終わると発射口が開いて熱を逃がし(数秒)、開いている間だけそこが弱点。案: 隙の長さはチャージショットを1発撃てる長さ以上。(候補だったB=網を出す子機を撃つと網が消える、C=くぐった後にボスが背中・コアを見せる、は採らなかった)
- **モデルの方針(ユーザー)**: 仕様をある程度決めてから、それに合わせたモデルをClaudeが作る。**人型にはしない**(AIは人型のアニメーションが下手なため)。アニメーションは1面ボスと同じくモデルに入れ、コードは名前で探して`ModelAnimator`で再生するだけ([BossEnemy.cpp:116](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L116))。描画は1面と同じスキンありの描き方(`DrawWithLighting(textures, true)`)。Blender 4.2・5.2がこのPC(家)にある→Pythonで形・ボーン・キーフレームまで作ってFBXに書き出せる。
  - アニメーションごとに、待機ポーズ・攻撃ポーズ・攻撃が出るフレーム・全体のフレーム数を決めておく(『攻撃モーションの内訳』)。
- 規模の注意: アルファ残り46.5h、締め切り12/3。コスト表の「ボス(仕様未定)」は今「死亡」1hだけなので、仕様が決まったら見積を足す。攻撃は形態ごとに1〜2種類、第二形態は第一形態の攻撃を作り変えて使い回す程度が現実的。
- **動き方(ユーザー決定)= 飛ぶ**(1面のボスは海面に立っていたので、違いを出す)。曇り空に溶けないよう、暗めの本体と光る発射口などで明度差を付ける(『シルエットが命』『判定のあるものを強調する』)。
- **発射口(ユーザー決定)= 2つ。場所はClaudeに任せる** → Claudeの案(ユーザーの反対なし、モデルを作るときの前提):
  - 横に長い飛行体。中央に暗い色の装甲の胴体、左右の上に張り出した腕の先に発射口のポッドが1つずつ。
  - 右上のポッド=右上→左下の線、左上のポッド=左上→右下の線(線がポッドの方向から伸びる)。2つの光の色を少し変えると対応がはっきりする(例: 青寄りと紫寄り)。
  - 隙: 閉じている間は暗い装甲、開くと中が赤く光る(発光マップの切り替え=1面ボスの赤い点滅と同じ方法)。閉じた装甲に当たったら火花と硬い音(ダメージなし、1面ボスのシールドの`OnHitInvincibleCol`と同じ考え方)。
  - ロックオンは敵1体につき`GetPos()`の1か所だけ([TargetManager.cpp:58](Manager/TargetManager.cpp#L58))→ボスだけ「ロックオンで狙う位置」を返す関数が要る(名前の候補: `GetLockOnPos`/`GetAimPos`)。開いている間は照準に近いほうのポッド、など(実装時に決める)。
  - 第二形態へのつながり(案): 片方のポッドが壊れて左右非対称のシルエットになる。
- **第二形態(ユーザー決定)**: 攻撃の種類を3種類ほどにする・網のリズムを変える・動きを少し速く・形も変える(全部)。※Claudeは「攻撃は網＋新しい2種類で計3種類」と受け取った(未確認)。
- **規模(Claudeの目安、ユーザー未確認)**: ボスの共通化3h・飛ぶ動き2h・網のレーザー5h・放熱の隙/弱点/ロックオン位置2h・形態変化3h・新しい攻撃2種6h・リズムと速さ1h・2面の登場演出2h・モデルの確認と変換2h=約26h。アルファ残りはボス抜きで46.5h(12/3まで16日、2.9h/日が必要、今の平均2.15h/日)→足すと約72h(4.5h/日)。
  - Claudeの案: アルファの優先度B・C(計18.5h)と海のリアル化(6h)をベータへ回す → **ユーザー判断で回さない**。優先度の低いものは時間があれば後で作る扱いでよく、ベータに移すとベータのコストが増えたように見えるので、コスト表のバージョンはそのままにする。
- **2つ目の攻撃(ユーザー案)= 扉**: プレイヤーの前方に扉のようなものを出し、ブーストかブレーキを強いる。Claudeの受け取り: 扉が開閉をくり返し、普通の速さだとちょうど閉じている時に着く→ブーストで開いているうちに抜けるか、ブレーキで次に開くのを待つ(未確認)。網=位置、扉=タイミングで役割が分かれる(『対戦は複雑になるもの?』)。
  - **見つけた点(未変更)**: ブースト中の前進は17([BoostState.cpp:8](Game/GameObjects/Actors/Character/Player/Movement/BoostState.cpp#L8))で、普通の前進18([MovingState.cpp:11](Game/GameObjects/Actors/Character/Player/Movement/MovingState.cpp#L11)・[IdleMovementState.cpp:12](Game/GameObjects/Actors/Character/Player/Movement/IdleMovementState.cpp#L12))より遅い。11/16に普通の前進を18にそろえたためと思われる。ブレーキは2([BrakeState.cpp:7](Game/GameObjects/Actors/Character/Player/Movement/BrakeState.cpp#L7))。扉の攻撃にはブーストの見直しが要る(値はユーザーが決める)。
  - 公平さ: ゲージは押している間0.5/F([GaugeActionStateBase.cpp:15](Game/GameObjects/Actors/Character/Player/Movement/GaugeActionStateBase.cpp#L15))、満タン100で約3.3秒。扉を抜けるのに要る長さを満タンの一部(半分以下など)に収める。開閉のリズムを着く前に読めるように(扉の縁のランプの点滅など)。
  - リスクとリターン(案): ブースト=反応時間が短い代わりに早く抜けられる、ブレーキ=安全だが遅い。ブーストで抜けた人に得(ボスの隙に早く着くなど)があると選ぶ意味が出る。
  - **(11/18・ユーザー依頼でClaudeが変更)ブースト17→30、ブレーキ2→5**([BoostState.cpp:8](Game/GameObjects/Actors/Character/Player/Movement/BoostState.cpp#L8)、[BrakeState.cpp:7](Game/GameObjects/Actors/Character/Player/Movement/BrakeState.cpp#L7))。経緯: 最初(10/01)は普通8・ブースト17・ブレーキ2で、普通だけ8→15(11/11)→18(11/17)と上がっていた。カメラは毎フレーム残りの9%ずつ追う(`lerp_t`=0.09、[GameCamera.cpp:25](Game/GameObjects/Camera/GameCamera.cpp#L25))ので、一定の速さvではカメラが約v×10だけ余分に遅れる(機体との距離≒166＋v×10)。普通18で約350、ブレーキ2で約190(=近すぎ)、5で約220、ブースト30で約470。Debug x64のビルドOK、ゲームでは未確認。速さを使っているのは「ブースト中か」の判定だけ(チュートリアル・AIテストプレイ・ブーストの画面効果)で、距離には依存していない。
- **3つ目の攻撃(ユーザー決定)= ロケットを飛ばしてくる(単純な攻撃)**。
  - `EnemyBullet`の仲間として作れば、バレルロールのカウンターでの跳ね返しがそのまま使える([CollisionManager.cpp:425-452](Manager/CollisionManager.cpp#L425-L452)の`ReflectEnemyBullet`)。ただし跳ね返った弾は撃った敵の`GetPos()`(中心)を狙う([438行目](Manager/CollisionManager.cpp#L438))→放熱口に当てるなら「ロックオンで狙う位置」の関数をここでも使う。
  - 3つの役割: 網=位置(移動)、扉=タイミング(ブースト/ブレーキ)、ロケット=追ってくるものをかわす/跳ね返す(移動/バレルロール)。
  - モデル: ロケットの発射口はポッドとは別に胴体の下側(Claudeの案、見分けやすさのため)。
- **攻撃の割り振り(ユーザー決定)**: 第一形態=網・ロケット。第二形態=数が増えたロケット＋扉を追加。網は第二形態でも残りリズムが変わる(前の「網のリズムも変えたい」から。Claudeの受け取り)。動きも少し速くなる。
- **第二形態の形(ユーザー決定、「一旦」)= ポッドが片方壊れる**。壊れたポッドの線は消え、網は片方向の斜めの線だけになる(すき間が増える分リズムを速くする)。放熱して弱点になるのは残ったポッド1つ。
- **細かい所(Claudeの仮の値、ユーザー了承「いったんやってみましょう」)**: 第二形態はHP50%から、壊れるのは右上のポッドで固定(→第二形態の網は左上のポッドの線=左上→右下だけ、弱点も左上だけ)、壊れ方は部品の非表示＋壊れた部品＋爆発エフェクト、2面の登場は遠くの空から飛んできて前に回り込む(コードで動かす)、死亡は1面と同じくエフェクト。
- **仕様書を作った: [BOSS2_SPEC.md](BOSS2_SPEC.md)**(流れ・動き方・攻撃4つ・モデルの形/大きさ/部品の名前/テクスチャ/アニメーション9本・作る順番・書き出しの注意)。攻撃の順番はランダムにせず決まった順(第一=網→ロケット、第二=網→扉→ロケット)、放熱は網の後だけ、とした。
- **次**: ユーザーが仕様書を確認 → Claudeがボスのモデル(形・部品・ボーン・Idle)をBlenderで作り、DxLibで読めるか確かめる。
  - (11/18)ユーザー「一旦やってみて違ければまた作り直しましょう」→仕様書はこのまま。

### 2026-11-18 Claude Code(FIC-ADMIN、ユーザーSakamotoKou)にBlender MCPを登録した(Claude、ユーザー依頼)
- **注意: このPCのSteamは別のWindowsユーザー「admin」で動いている**ので、Steamから開いたBlenderは`C:\Users\Admin\AppData\Roaming\Blender Foundation\Blender\`の設定を読む(SakamotoKouからは権限がなく見えない)。SakamotoKou側の5.3には5.2の設定(MCP拡張・Blender Labのリポジトリ込み)をコピー済みで、`blender.exe`を直接SakamotoKouとして起動すればMCPが有効(画面なしの起動で`bl_ext.lab_blender_org.mcp`が有効と確認)。
- もともとClaude Desktopの拡張(Blender Labの`blender-mcp`、`%LOCALAPPDATA%\Claude-Data\claude_pzs8sxrjxfjjc\Claude Extensions\ant.dir.gh.blender.blender-mcp`)で連携していた。Blender側は5.1・5.2に拡張`mcp`(Blender Lab、ポート9876で待ち受け)。
- **Desktopの拡張の環境は壊れていた**: `.venv`の土台のPython(`%APPDATA%\uv\python\cpython-3.14.0...`)が消えていて起動できない(Desktopでも今は動かない可能性)。Desktopのフォルダは触っていない。
- Claude Code用に別の環境を作った: `%LOCALAPPDATA%\BlenderMcpClaudeCode`(拡張のソースをコピー、Desktop同梱のuv 0.9.7で`uv sync --frozen`、**土台はBlender 5.2同梱のPython 3.13**なので消えにくい)。`claude mcp add --scope user blender -- %LOCALAPPDATA%\BlenderMcpClaudeCode\.venv\Scripts\blender-mcp.exe`で登録(`~/.claude.json`)、`claude mcp list`で Connected。
- 使うとき: Blender 5.2を開き、MCPの拡張のサーバーを起動しておく。Claude Codeは登録後の再起動で道具が使えるようになる。DesktopとCodeで同時に同じBlenderにつながない。
- ほかのPCには入っていない(同じ手順で作れる)。
- 別件: Effekseer MCPはこのセッションで「Failed to connect」(未調査)。
- (11/18)Blender MCPの確認のたびに作業が止まるので、`.cpp/.h/.hlsl`以外は確認なしにした(`.claude/settings.local.json`のallowにRead/Edit/Write/Glob/Grep/mcp__blender/mcp__effekseerなど、スクラッチパッド等をadditionalDirectoriesに)。

### 2026-11-18 2面のボスのモデル第1版を作った(Claude、Blender MCP。コードはまだ)
- 詳細は [BOSS2_SPEC.md](BOSS2_SPEC.md) の「8. 作ったモデル」。ゲームが読むのは`Data/Model/Boss2.mv1`と`Data/Model/Boss2.fbm/`(ベース色・発光・放熱中の発光・平らな法線)。
- 形: 横に長い飛行体。暗い金属の胴体、斜め上に伸びる2本の腕の先にポッド(左上=紫、右上=水色のレンズ)、胴体の下にロケットのハッチと4本の筒、前面に斜めの2本の目。ポッドの胴は装甲の下がへこんでいて、閉じていると放熱口(赤)は見えず、装甲が上下に開くと見える。パネルの継ぎ目と汚れのむらはベース色に焼き付けた。
- DxLibで確認済み: 大きさ(幅約2920)・向き(正面-Z、左-X)・フレームの名前・アニメーション9本の名前と長さ・各ポーズ(放熱/ハッチ/のけぞり)の向き。**ゲームの中ではまだ表示していない**(ゲームのシェーダーでの見え方は未確認)。
- 作り方: `Tools/Boss2Model/`のスクリプト(Blenderで build→bake→export、`make_all.ps1`でモデルビューアを使って`.mv1`に変換)。BlenderのFBXは`MV1LoadModel`で直接読めない(handle=-1)が、モデルビューアに渡すと`LoadAndSave=1`の設定で自動で`.mv1`になる。変換のときだけ`NormalRemake`を0にして、終わったら1に戻す。
- つまずき: マテリアルに発光をつないだまま書き出すとDxLibで真っ白、拡散色0.8・自己発光0だとDxLibの標準の描き方で真っ黒に近い→1面のボスの値(拡散色1.0・自己発光0.5)にそろえた。テクスチャのパスは`Boss2.fbm\Boss2_Base.png`で入るので、1面と同じ「`.mv1`の隣に`.fbm`フォルダ」に置く。Blenderの画面のスクリーンショットは古いままで更新されなかったので、確認はカメラを置いたレンダリングで行った。
- **(11/18・ユーザー依頼でClaudeが実施)1面のボスを`SpiderBoss`に名前を変えた**(2面のボスと区別するため。上位に`Boss`クラスを作る予定=ユーザー案)。フォルダ`Enemy/BossEnemy/`→`Enemy/Boss/SpiderBoss/`。`BossEnemy`→`SpiderBoss`(構造体`SpiderBossData`)、`BossEnemyDataSetter`→`SpiderBossDataSetter`、`IBossEnemyState`→`ISpiderBossState`、`BossIdleState`/`BossBeamState`/`SummonState`→`SpiderBossIdleState`/`SpiderBossBeamState`/`SpiderBossSummonState`、`Game/Collision/`の`BossDamageCollider`/`BossShieldCollider`/`BossBeamCollider`/`BossBeamTipCollider`→頭に`Spider`。`git mv`で移動(履歴は残る)、中身は単語単位で置き換え(32ファイル、`.vcxproj`/`.filters`も)、Debug x64ビルドOK。ボス全般のもの(`BossHPGaugeUI`、`ModelID::Boss`などの素材ID、`ColliderTag`、CSV名`Boss`)は変えていない。このノートの中の古いファイルへのリンク(20か所)も新しいパスに直した(本文の名前は当時のまま)。
- **(11/18)コードの設計(ユーザーと相談中)**:
  - 2つのボスの上位クラスとして`Boss`を作る(ユーザー案)。`EnemyBase`/`Character`にあるもの(HP・`IsDying`・`GetColliders`など)は足さない。**`Boss`に持たせるのは`IsBossAppear`/`SetIsBossAppear`(と`SpiderBoss`の`m_isAppear`を移す)**。`SetEnemyFactory`は1面の召喚用なので`SpiderBoss`に残す。
  - **2面も1面と同じくムービーで登場する(ユーザー決定「そっちの方が楽そう」)**。WARNING→ムービー→カメラのズームの流れを2つのボスで共通にする。ボスごとに違うのは「どのムービーか」「ムービー後にボスを置く位置(1面は海面`boss_stand_y`)」「ムービーの最後のカメラの位置(1面は`boss_appear_camera_dist`2700・`height`1020)」。2面のムービーはClaudeがBlenderで作る(1面のムービーの作り方は10/02の記録: ゲームのシェーダーと同じ見た目、最後のコマをゲームのカメラ位置に合わせる)。
- **次(ゲーム側、ユーザーが書く)**: モデルと画像の登録(ResourceLoaderの`ModelID`/`GraphicID`、パス、`game.models`/`game.graphics`) → 2面のボスのクラス(`GameScene`・HPゲージ・当たり判定がボス全般を扱える形) → 動き → 網と放熱 → ロケット → 形態の切り替え → 扉。扉とロケットのモデルはまだ作っていない。

### 2026-11-18 2面のボスの準備: コスト表・素材の登録・上位クラス`BossBase`(途中。**ビルドは通らない状態**)
- **コスト表**: 「ボス(仕様未定)」の下に10行を足した(S・アルファ・未完了、見積は仮の目安): ボスの共通化3、飛ぶ動き2、網のレーザー5、放熱の隙・弱点・ロックオン位置2、形態変化3、扉4、ロケット2、第二形態のリズムと速さ1、2面の登場演出2、モデルの確認と変換2(計約26h)。「ボス(仕様未定)」は**2面のボスの行**なので、1面のボスの作業は紐づけない(「ステージ1ボス」の下に別の行)。
- **素材の登録(Claudeが実施、Debug x64ビルドOK)**: `ModelID::Boss2`、`GraphicID::Boss2Normal`/`Boss2Emission`/`Boss2EmissionVent`(放熱中の赤い版)。パスは[ResourceConstants.h](Constants/ResourceConstants.h)の`boss2_*_path`。ゲームシーンの読み込み一覧に入れたので、**1面でも2面のボスのモデルを読み込む**(重さが気になったら2面だけに分ける)。
- **上位クラス`BossBase`(ユーザーが作成)**: `GameScene`が`m_pBoss`で呼ぶもの(Init/GetPos/SetPos/IsDying/IsDead/GetHealth/GetMaxHealth/TakeDamage/DebugSetHealth)はすでに`EnemyBase`/`Character`にあるので、`BossBase : public EnemyBase`に足すのは`m_isAppear`と`IsBossAppear`/`SetIsBossAppear`だけ(`SetEnemyFactory`は1面の召喚用なので`SpiderBoss`に残す)。`BossBase`のコンストラクタは構造体`BossBaseData`(modelID・カメラ・プレイヤー・弾・音・エフェクトの6つ)を受けて`EnemyBase`へ渡す。名前は`Boss`だとわかりづらいのでユーザーが`BossBase`に決め、フォルダは`Enemy/Boss/`。`m_isAppear`と2つの関数は`SpiderBoss`から`BossBase`へ移動済み。
- **データ構造体の共通化(方針B、ユーザー決定)**: `SpiderBossData`は`BossBase::BossBaseData`を継承して`pos`だけ持つ。名前は`modelID`にそろえた(`SpiderBossData::Id`→`modelID`、Claudeが3か所直した)。
- **残り(ユーザーが自分で直す。これでビルドが通る)**: ①[SpiderBoss.h:21](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.h#L21)の`SpiderBossData`を`BossBase::BossBaseData`の継承にして`pos`だけにする ②[SpiderBoss.cpp:68](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L68)の初期化リストを`EnemyBase(...)`から`BossBase(data)`にする(継承していれば`SpiderBossData`をそのまま渡せる)。現在のエラーは`EnemyBase が基底クラスでもメンバーでもない`と`BossBase に既定のコンストラクターがない`の2つ。
- 任意: `BossBase`の`SetIsBossAppear`/`IsBossAppear`の`virtual`は上書きの予定がないので不要。
- 次: `GameScene`の`m_pBoss`、`BossHPGaugeUI`、`CollisionManager`、AutoPlay系の`SpiderBoss`直接指定を`BossBase`に変える必要があるか確認 → 2面のボスのクラス → 動き → 網 → …(仕様は[BOSS2_SPEC.md](BOSS2_SPEC.md))。
- 別: 1面のボス死亡時のビームを消す作業(コスト表1.5h、S)は、AかB(`NOTES`の11/17の項)が未決定のまま。
- **(11/18・ユーザー依頼、未着手)2面のボスのモデルを作り直す**: 「もっと丁寧に、敵だけどかっこよく作り直してほしい」。第1版(上の項)は形が箱の組み合わせに近く、のっぺりしている。根拠にする原則: 『キャラを立てるデザイン』(シルエットが最重要)、『スケール感は正しく合わせる』(巨大な機械には汚し・微妙な色相差、ツヤは強すぎない)、『大事と小事を見分ける』(よく見える正面に手をかける)。部品の名前・ボーン・アニメーション名は仕様書のまま変えない(ゲーム側の登録をそのまま使うため)。
- **(11/19・ユーザー実装、Debug x64ビルドOK)残りの①②ができた**: `SpiderBossData : public BossBaseData`で`pos`だけ([SpiderBoss.h:21-24](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.h#L21-L24))、初期化リストは`BossBase(data)`([SpiderBoss.cpp:68](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L68))。学んだこと: 構造体も継承できる(`struct`は何も書かないとpublic継承、`class`はprivate)。初期化リストで呼べるのは直接の親のコンストラクタだけ。任意: `BossBaseData::modelID`に初期値`ModelID::None`がない。
- (11/19)モデルの作り直しは別のセッションに任せた(プロンプトを渡した)。確認用プログラムを`Tools/Boss2Model/mv1check/`(`build.ps1`でビルド、exeはgitに入れない)に移した。
- **(11/19・ユーザー実装)`SpiderBoss`の名指しを`BossBase`に**: `CollisionManager`(.hの前方宣言・引数・`m_pBoss`、.cppのincludeと`lock()`)、`BossHPGaugeUI`、`GameScene`(`m_pBoss`を`std::shared_ptr<BossBase>`、`SetEnemyFactory`は`dynamic_pointer_cast<SpiderBoss>`で1面のときだけ、[GameScene.cpp:203-204](Scene/GameScene.cpp#L203-L204))、AutoPlay(`AutoPlayWorld`の前方宣言と`GetBoss`、`PlaytestRecorder.cpp:273`)。`CollisionManager`まではビルドOK、その後はビルド未確認。任意: `PlaytestRecorder.cpp:15`のincludeは`SpiderBoss.h`のまま(`BossBase.h`で足りる)。
  - つまずき: Claudeが止めたビルドの`cl.exe`が残ってファイルを開いたままにし、VS Codeで「保存できませんでした」になった→残った`cl.exe`を終了して解決。
- **(11/19・ユーザー決定)ボスごとに違う登場の値は`StageConfig`に持たせる**: 登場ムービー(`ResourceLoader::GraphicID`。`StageConfig.h`で`ResourceLoader.h`をincludeしてよい、ユーザー判断)、ムービー後のボスの高さ(1面`boss_stand_y`=0)、ムービーの最後のカメラの距離(2700)・高さ(1020)。`GameScene`で置き換える所: ムービー[563](Scene/GameScene.cpp#L563)・[580](Scene/GameScene.cpp#L580)・[808](Scene/GameScene.cpp#L808)行目、高さ[585](Scene/GameScene.cpp#L585)、カメラ[602-605](Scene/GameScene.cpp#L602-L605)。消える定数[82](Scene/GameScene.cpp#L82)・[91-92](Scene/GameScene.cpp#L91-L92)行目(`boss_appear_zoom_speed`は共通なので残す)。2面はムービーができるまで1面と同じ値を仮に入れる。
  - **(11/19・できた、Debug x64ビルドOK)**: `StageData`に`bossAppearMovieID`/`bossStandY`/`bossAppearDist`/`bossAppearCameraHeight`(表は1面・2面とも`BossAppearMovie`・0・2700・1020。2面は今も`SpiderBoss`が出るので仮に同じ値)、`StageConfig.h`にゲッター`GetBossAppearMovieID`/`GetBossStandY`/`GetBossAppearCameraDist`/`GetBossAppearCameraHeight`とメンバ(Claude)、コンストラクタのコピーと`GameScene`の6か所の置き換え・定数3つの削除(ユーザー)。`GraphicID`には`None`がないので、メンバの初期値は`BossAppearMovie`。
  - 学んだこと: 構造体の`{ }`の初期化(集成体初期化)は関数の呼び出しではないので、MicrosoftのC/C++拡張は引数のヒントを出さない。出したいなら`constexpr`のコンストラクタ、名前を見たいならC++20の指示付き初期化(`.hasTutorial = true`)、またはclangdに替える(コードはそのまま、未導入・ユーザーは見送り)。
- **(11/19)2面のボスのクラス名は`GunshipBoss`(ユーザー決定)。フォルダを`Enemy/Boss/`の下にまとめた(ユーザー依頼、Claudeが実施)**: `Enemy/Boss/BossBase`、`Enemy/Boss/SpiderBoss/`(`git mv`で移動、includeのパス5ファイルと`.vcxproj`/`.filters`を書き換え、ノートのリンク22か所も)、`Enemy/Boss/GunshipBoss/`に`GunshipBoss : public BossBase`の雛形(`New-GameClass.ps1`、vcxproj登録済み)。**雛形のコンストラクタが引数なしの`BossBase()`を呼んでいるので、今はビルドが通らない**(ユーザーが`GunshipBossData`とコンストラクタを書く)。
- **(11/19)どのボスを作るかは`StageConfig`で決める(ユーザー決定)**: 列挙型`StageConfig::BossType`(`Spider`/`Gunship`、外から使うので`public`に置く)をユーザーが作成中。`GunshipBoss`(ユーザー作成)は`GunshipBossData : public BossBaseData`で`pos`だけ、コンストラクタは`BossBase(data)`。**`GunshipBossDataSetter`はClaudeが作成**(`SpiderBossDataSetter`と同じ形、`Boss.csv`を読んで`GunshipBoss`を作る、vcxproj登録済み)。**今はビルドが通らない**: `GunshipBoss`に`Draw()`がない(`Character::Draw`が純粋仮想なので抽象クラスのまま→`make_shared`できない)。残り: `GunshipBoss`に`Draw()`の`override`、コンストラクタで`SetPos(data.pos)`、`GameScene`で`GetBossType()`による切り替え、CSVのモデル名`Boss2`を読めるように(`ResourceLoader.cpp`の`WStringToModelID`の表は今`Boss`だけ)、2面の`Boss.csv`を`Boss2`に。2面を`Gunship`に切り替えるのは`GunshipBoss`が表示できてから(未決定)。
  - **(11/19・ユーザー実装、Debug x64ビルドOK)**: `StageConfig::BossType`(表は1面`Spider`・2面`GunShip`)、`GameScene`で`GetBossType()`による呼び分け([GameScene.cpp:170-189](Scene/GameScene.cpp#L170-L189))と`GunshipBoss.h`のinclude(前方宣言だけでは基底クラスへ変換できない)、`GunshipBoss`に`Draw()`と`SetPos(data.pos)`。**今の2面は`GunshipBoss`が出るが、中身はまだなく、`Boss.csv`のモデル名も`Boss`のまま**なので、ボス戦が進まない。任意: `m_bossType`に初期値なし、値の綴り`GunShip`(クラス名は`Gunship`)。
  - **(11/19・Claude、ユーザー依頼、ビルドOK)CSVのモデル名`Boss2`**: [ResourceConstants.h:219](Constants/ResourceConstants.h#L219)に`boss2_csv_name`、[ResourceLoader.cpp:743](Manager/ResourceLoader.cpp#L743)の`WStringToModelID`の表に`Boss2`、[Data/CSV/Stage2/Boss.csv](Data/CSV/Stage2/Boss.csv)のモデル名を`Boss2`に(位置は1面のコピーのまま)。**注意: Unityの2面のシーンのボスは`Boss`のままなので、Unityから2面を書き出すと`Boss.csv`が`Boss`に戻る**(書き出す前にUnity側を`Boss2`に対応させる。`StageModelCatalog.cs`に2面のボスのモデルも未登録)。
  - 次: `GunshipBoss`の描画(法線・発光の画像、壊れたポッド`Boss2_PodR_Broken`は最初は非表示)→待機アニメーション`Boss2_Armature|Idle`→2面のボスの位置と`bossStandY`。
  - **(11/19・Claude、ユーザー依頼、ビルドOK・ゲーム内未確認)`GunshipBoss`の描画**: `OnInit()`で`CreateShaderBuffers()`、`Draw()`で`ApplyMatrix(m_scale, …)`→`UpdateShaderMatrixData()`→法線`Boss2Normal`・メタリックなし・発光`Boss2Emission`・ノイズなしで`DrawWithLighting(textures, true)`(1面の`DrawEnemy`と同じ流れ)。放熱中の発光`Boss2EmissionVent`への切り替えは放熱を作るときに。**壊れたポッドの非表示はユーザーが書く**(`OnInit`の`CreateShaderBuffers()`の後ろに`MV1SearchFrame`+`MV1SetFrameVisible`)。
- (11/18・済)ボスごとに違う登場の値(ムービー・ムービー後の位置・カメラの位置)を`BossBase`に持たせるか`StageConfig`に持たせるか。

### 2026-11-19 2面のボスのモデル第2版を作った(Claude、Blender MCP。モデルだけ。ゲームのコードは触っていない)
- 上の「作り直し」の依頼を実施。ゲームが読むファイル(`Data/Model/Boss2.mv1`、`Data/Model/Boss2.fbm/`の4枚)を、同じ名前のまま差し替えた。数値は[BOSS2_SPEC.md](BOSS2_SPEC.md)の「8. 作ったモデル」に記録した。
- **変えていないもの**: ボーンの名前と親子関係、メッシュ3つ(`Boss2_Main`/`Boss2_PodR`/`Boss2_PodR_Broken`)、アニメーション9本の名前と長さ、向きと単位、マテリアルの値(拡散色1.0・自己発光0.5)、テクスチャのファイル名、ポッドの位置(±1260・+440付近)、放熱の仕組み(閉じていると赤は見えず、開くと見える)。**変えたもの**: `RocketHatch`のボーンの位置(ハッチの付け根に合わせた。ゲームは名前で探すので影響なし)。
- **形**(『キャラを立てるデザイン』の、黒く塗っても分かるシルエット / 『大事と小事を見分ける』の、よく見える正面に手をかける): 下へとがる盾形の胴、外へ斜めに上がる肩当てと角、背中のトサカ、ひじで折れる腕と腕の下に垂れる3枚の刃、ポッドの上のV字の角と下の爪、発射口の周りの3本の爪。顔はにらむ目・V字の眉・口のグリル・牙。ロケットのハッチは台形の「下あご」(前の縁が筒の口をふさぎ、開くと筒が見える)。【解釈】ポッド・ハッチを「あご」にそろえると、遠くからでも「開く=攻撃か隙」と読める。三角形は7360→13064枚。
- **色と細部**(『スケール感は正しく合わせる』の、巨大な機械には汚しと微妙な色相差、ツヤは強すぎない): 主装甲は青みのある暗い灰色、骨組みは暖かみのある黒、肩当てとポッドの装甲は1段明るい灰色、縁に鈍い真鍮色。パネル割り(3方向から投影した長方形。第1版のひび割れ模様はやめた)、ボルト、すき間の汚れ(AO)、雨だれ、角の塗装はがれを焼き付けた。法線マップに角の丸め・継ぎ目の溝・ボルトを焼き付けた(第1版は平ら)。
- **光る所**: ポッドに集めた(レンズは中心が白いグラデーション、装甲の前の縁の輪、首の輪、爪の先、後ろの噴射口。左=紫、右=水色)。胴は青白い目と下の噴射口だけ。**赤は放熱口だけ**(弱点の合図を混ぜないため)。壊れた右ポッドの中も赤にせず、弱い水色の小さな火花にした(第二形態で左が放熱すると、放熱中の発光の画像に切り替わるため。第1版は壊れたポッドの芯も赤く光っていた)。
- **わかったこと(ゲームのシェーダー)**: メタリックの画像を-1で渡すと、D3D11では外したテクスチャを読むと0になるので滑らかさが最小の1になり、ツヤが`dot^1`で広く強く出る。正面は光がカメラの後ろから来るので、ツヤがほぼ真正面に返る。**第1版はゲームの中では正面の面が白く飛ぶはずだった**。ツヤを抑える方法は、ベース色のアルファ(ツヤの強さ、[LightingPS.hlsl:80](LightingPS.hlsl#L80))か、メタリックの画像のR(滑らかさ=R×34+1、[LightingCommon.hlsli:71](LightingCommon.hlsli#L71))の2つしかない。
  - 最初はアルファにツヤを入れたが、**モデルビューア(DxLibの標準の描き方)ではアルファが透明度になり、すごく薄く透けて見えた**(ユーザーが気づいた)。DxLibもこのメッシュを半透明と判定する。
  - **(11/19・ユーザー決定)ベース色からアルファを抜いた**。代わりに**滑らかさの画像`Boss2_Metallic.png`を足した**(装甲0.35〜0.55、光る所は1.0)。これをメタリックとして渡すと、ツヤが角やボルトに細く光るだけになる。ビューアでもゲームでも不透明。
- **ゲーム側への注意(コードのセッション向け)**:
  - **2面のボスの描画では、`tex_metalic`に-1ではなく`Boss2_Metallic.png`を渡す**(1面のボスの[SpiderBoss.cpp:390](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.cpp#L390)は-1)。`GraphicID`にも登録が要る(名前の候補: `Boss2Metallic`)。**-1のままだと正面が白く飛ぶ**(プレビューの`USE_METALLIC=False`で、その見え方を確かめられる)。
  - 法線マップは**DirectX式(緑=テクスチャの下向き)**で焼いた。ゲームは従法線を`cross(N, T)`で作る([LightingCommon.hlsli:43](LightingCommon.hlsli#L43))ので、DxLibの座標(左手系)では下向きになる、と計算した。1面のボスの法線マップ(Unreal系の素材)と同じ向き。**ゲームの中での確認はまだ**(凹凸の陰が上下逆に見えたら、`bake_boss2.py`の`normal_g="NEG_Y"`を`POS_Y`にして焼き直す)。
  - 法線の画像はもう平らではないので、`Boss2Normal`をシェーダーに渡すと凹凸が出る。
- **確認**: Blenderにゲームのシェーダーと同じ式で描くプレビューを作った([render_preview.py:67](Tools/Boss2Model/render_preview.py#L67)。拡散+環境光0.5+ツヤ(滑らかさ込み)×アルファ+発光、光はゲームの`Game::light_direction`、ガンマ空間のまま)。第1版も同じ条件で撮って並べた。DxLibでは、フレーム名・親子・アニメーション9本の名前と長さ・大きさ・向き・放熱/ハッチ/のけぞりのポーズを確認し、標準の描き方でも不透明に写ることを確認した(標準の描き方は発光と法線マップを使わないので暗く写る。第1版も同じ)。**ゲームの中ではまだ表示していない**。
- **つまずき**: ①プレビューで放熱用のマテリアルを作るときに同じ画像を消して読み直していたため、通常用の画像が外れて真っ黒になった(1回の実行で画像を使い回すように直した)。②UVの島の余白を既定(SCALED)にすると、面取りの細い島が多いせいで画像の3割しか使わなかった。`margin_method="ADD"`で詰めた(1mあたり約21画素→約28画素)。余白を3画素まで詰めると、遠くで縮小されたときに隣の島の発光がにじんで水色の点が出たので、6画素にした。③PowerShell 5.1はBOMなしUTF-8の`.ps1`の日本語を読めない。
- **作り直し方**: `Tools/Boss2Model/`の`build_boss2.py`→`bake_boss2.py`(Cycles。GPUがあれば使う。RTX 4060 Tiで約30秒)→`export_boss2.py`→`make_all.ps1`。色と滑らかさは[build_boss2.py:371](Tools/Boss2Model/build_boss2.py#L371)の`build_materials`、表面の汚れとパネル割りは[build_boss2.py:176](Tools/Boss2Model/build_boss2.py#L176)、形は[build_body](Tools/Boss2Model/build_boss2.py#L597)・[build_arm](Tools/Boss2Model/build_boss2.py#L738)・[build_pod](Tools/Boss2Model/build_boss2.py#L864)・[build_broken_pod](Tools/Boss2Model/build_boss2.py#L969)、ボーンは[BONES](Tools/Boss2Model/build_boss2.py#L1040)。アルファにツヤを入れる方式に戻すときは`bake_boss2.py`に`SPEC_IN_ALPHA=True`。
- 次(モデル): ゲームの中で見え方(ツヤ・法線の向き・発光の強さ)を確かめて調整。2面の登場ムービー、ロケットと扉のモデルはまだ。

### 2026-11-19 2面のボスのコード: ここまで(帰る前のまとめ。**ビルドは通らない状態でコミット**)
- できたこと(詳細は上の「2面のボスの準備」の項の11/19の行): `SpiderBoss`の`BossBase`化と名指しの置き換え(`CollisionManager`・`BossHPGaugeUI`・`GameScene`・AutoPlay)、登場の値を`StageConfig`へ、フォルダを`Enemy/Boss/`(`BossBase`)・`Enemy/Boss/SpiderBoss/`・`Enemy/Boss/GunshipBoss/`に整理、`StageConfig::BossType`で作るボスを切り替え、`GunshipBoss`・`GunshipBossDataSetter`、CSVのモデル名`Boss2`、`GunshipBoss`の描画(Claude)と壊れたポッドの非表示(ユーザー、`m_brokenPodFrameIdx`と`assert`)。
- **(Claude、ユーザー依頼)1面の`SpiderBoss`のアニメーターを`std::unique_ptr<ModelAnimator> m_pAnimator`にした**(`.h`は前方宣言、作るのはコンストラクタの中、`Play`/`Update`の3か所を`->`に)。`SpiderBoss.cpp`はエラーなし。
- **今ビルドが通らない理由(ユーザーが書いている途中)**: [GunshipBoss.h:24](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h#L24)の`ModelAnimator m_animator;`が値のまま(`ModelAnimator`は前方宣言なので「未定義の class」エラー)。1面と同じく`std::unique_ptr<ModelAnimator>`にする。
- **次にやること(順に)**:
  1. `GunshipBoss`の待機アニメーション: `.h`は`std::unique_ptr<ModelAnimator>`(名前の候補`m_pAnimator`)、`OnInit`で`std::make_unique<ModelAnimator>(m_modelHandle)`→`Play(L"Boss2_Armature|Idle", true, 1.0f)`(2面は速さ1.0で120F=2秒)、`Update()`の`override`で`Character::Update()`→アニメーターの`Update(20)`(1面のブレンドは20F)。定数の名前の候補`idle_anim_name`/`idle_anim_speed`/`anim_blend_time`。
  2. **メタリックの画像**: モデルの第2版で`Data/Model/Boss2.fbm/Boss2_Metallic.png`が足された。`GraphicID`に登録(名前の候補`Boss2Metallic`、`ResourceConstants`のパス・`ResourceLoader.cpp`の表・`game.graphics`)して、`GunshipBoss::Draw`の`tex_metalic`に渡す(**今は-1なので正面が白く飛ぶ**)。
  3. ゲームの中で2面のボスを表示して確かめる(`first_stage_no`を1、ImGuiのワープでボスの手前へ)。法線マップの凹凸の向き(逆なら`bake_boss2.py`の`normal_g`)、ツヤ、発光の強さ。
  4. 2面のボスの位置と高さ(`Data/CSV/Stage2/Boss.csv`は1面のコピーのy=30000・z=63860、`StageConfig`の`bossStandY`は仮に0)。
  5. その後: 動き → 網と放熱 → ロケット → 形態の切り替え → 扉(仕様は[BOSS2_SPEC.md](BOSS2_SPEC.md))。2面の登場ムービー、ロケットと扉のモデルはまだ。
- 注意: 今の2面は`GunshipBoss`が出るが、当たり判定も動きもないのでボス戦が進まない。Unityから2面を書き出すと`Boss.csv`のモデル名が`Boss`に戻る。
- つまずき(環境): Claudeが途中で止めたビルドの`cl.exe`が残ってファイルを開いたままにし、VS Codeで「保存できませんでした」になった→残った`cl.exe`を終了して解決。

### 2026-11-19 2面のボスが表示できた(待機アニメーション・追従・位置。ビルドOK)
- **ユーザー実装**: `GunshipBoss`の`Update()`(`BossBase::Update()`→アニメーター`Update(anim_blend_frame)`)、`OnInit`の待機`Play(idle_anim_name, true)`。**プレイヤーに追従**(出現済みなら、プレイヤーの速度のzだけを自分の速度にする。[GunshipBoss.cpp:54-62](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L54-L62)、`SpiderBoss`と同じ処理。**共通化はまだしない(ユーザー決定)**。名前の候補は`MoveWithPlayer`/`FollowPlayerZ`/`CopyPlayerVelZ`で`BossBase`に置く案。追従のブロックが`BossBase::Update()`の後ろにあるので、1フレーム遅れる(前に移せば直る。見た目には出にくい))。
- **「存在しないモデルID」のassert**: [EnemyStatus.csv](Data/CSV/Params/EnemyStatus.csv)に`Boss2`の行がなかった(`EnemyBase`が`FindByModelID`でHPなどを探す)。`Boss2,2000,0,330,0`を足した(1面と同じ仮の値。HPは仕様書に具体値がないので後で調整)。
- **2面のボスが見えない原因(最初のディゾルブ説は外れ)**: **`Boss2.mv1`の頂点形式が接線なし(`4FRAME`、vtype 1)だった**。ゲームの頂点シェーダー[SkinnedLightingVS.hlsl](SkinnedLightingVS.hlsl)は`tangent`/`bin`を入力に持つので、頂点データの並びとずれて何も描かれなかった。1面のボスは接線つき(`NMAP_4FRAME`、vtype 5)。DxLibは**FBXのマテリアルに法線マップが入っているモデルだけ**接線つきの`.mv1`を作る([NOTES.md:1810](NOTES.md#L1810)の「法線マップがモデルごとに必要」と同じ制約)。
  - 調べ方: 小さなDxLibプログラム(`MV1LoadModel`→`MV1GetTriangleListVertexType`)で、1面と2面のモデルの頂点タイプを比べた。大きさ(ポッドがx≈±1270)、Idleの動き、マテリアル(半透明でない)は正常だった。
  - 直し方: [bake_boss2.py](Tools/Boss2Model/bake_boss2.py)の最後のマテリアルに、`Boss2_Normal.png`をNormal Mapノード経由でPrincipled BSDFのNormalにつないだ。FBXに法線マップが入り、`.mv1`が接線つき(vtype 5)になった。Blenderは**バックグラウンドで実行できる**(`C:\bl\blender.exe -b --python`、build→bake→exportを続けて約3分)。`make_all.ps1`はモデルビューアの場所を学校(`C:\Users\SakamotoKou\...`)と家(`C:\DxLib_VC\Tool\DxLibModelViewer`)の両方から探すようにした。**テクスチャ5枚も焼き直したので少し変わっている**(形・ボーン・アニメーションは同じ)。
  - **教訓**: 「描画のコードは1面と同じ形なのに見えない」ときは、**モデルの頂点形式**(`MV1GetTriangleListVertexType`)を疑う。最初に疑った「ディゾルブ」(カメラからの距離で消える)は、ボスの位置(出現Zから7560先で、1面は5160先)を1面にそろえても見えなかったので外れ。ただし**ズーム中は「カメラ〜プレイヤー」の距離が長くなり、ディゾルブの範囲が広がる**のは本当なので、ボスの位置を変えるときは注意(ボスの描画をディゾルブ対象から外す根本対応は未)。[Stage2/Boss.csv](Data/CSV/Stage2/Boss.csv)のzを61460(出現Z+5160)にしたが、原因とは無関係。
- **メタリックの画像の登録(Claude、ユーザー依頼、Debug x64ビルドOK)**: [ResourceConstants.h:127](Constants/ResourceConstants.h#L127)の`boss2_metallic_path`、`GraphicID::Boss2Metallic`([ResourceLoader.h:134](Manager/ResourceLoader.h#L134)、`.cpp`の表と`game.graphics`)、[GunshipBoss.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp)の`tex_metalic`に`-1`の代わりに渡した。
- **ボスの高さ**: 2面の`bossStandY`を0→**1000**([StageConfig.cpp:48](Constants/StageConfig.cpp#L48))。画面のスクリーンショットで、ボスが道路に近く下寄りだったので(中心が原点で高さ約1200、足元が道路に近かった)。**1000の見え方は未確認**(ユーザー「確認しなくていい」)。気になったらこの数値だけ変える。
- **デバッグ**: [GameScene.cpp:498](Scene/GameScene.cpp#L498)のデバッグの「Boss」タブにボスの座標(x,y,z)を足した(Claude、ユーザー依頼)。
- **ゲーム内で確認できたこと**: 2面のボスが見える(ポッドの紫と水色の発光、胴の水色の目と下の噴射口、パネル割りの質感)。**まだ確認していない**: 法線マップの凹凸の向き(逆に見えたら`bake_boss2.py`の`normal_g="NEG_Y"`を`POS_Y`)、ツヤ、アニメーションの動き。
- **(11/20)ボスの動きの設計(相談中)**: 動きと攻撃の切り替えは**1面と同じくステートに分ける**(ユーザー決定)。共通の型`IGunshipBossState`(Enter/Update/Exit/Draw、`ChangeState`で次を予約)。ステート: 移動`GunshipBossMoveState`、網`GunshipBossNetState`、放熱(隙)は**`GunshipBossChanceState`**(ユーザー命名、「当てるチャンス」)、ロケット`GunshipBossRocketState`、形態の切り替え`GunshipBossFormChangeState`、扉は**`GunshipBossShutterState`**(ユーザー決定。`Split`/`Gate`/`Pinch`なども検討した)。流れ: 第一形態 移動→網→放熱→移動→ロケット→…、第二形態 移動→網→放熱→移動→扉→移動→ロケット→…。
  - **移動の行き先(ユーザー決定)**: 候補の地点を配列で持ち、乱数で選ぶ(今いる地点と同じものは選ばない)。**網と扉は必ず真ん中で撃つ**(見た目のため)→次の攻撃が網か扉なら、移動の最後に真ん中(x=0、高さ`bossStandY`)へ飛んでから構えに入る(たまたま真ん中にいるのを待つのではなく、撃つ前に必ず真ん中へ移動する、とユーザーが確認)。ロケットはどこからでも撃つ。**地点に着いたら少し止まる**(【地点で止まる長さ】)。**地点の間は加速してから減速する**(ユーザー決定。出だしもゆっくり、重量感)。**1回の移動はいつも同じフレーム数**(【移動にかけるフレーム数】、ユーザー決定。遠い地点へは速く、近い地点へはゆっくりになる。距離で変えるのは後で1か所直せばできる)。作り方: 出発地点と目標を覚え、経過フレームから進んだ割合(0〜1)を出し、出だしと終わりがゆっくりの形に変えて位置を決める。動かすのはxとyだけ(zは追従のまま)。
  - **(11/20・ユーザーが導いた式)**: 時間の割合`t = frame / moveFrame`(intどうしの割り算に注意)、位置`start + (target - start) * f(t)`。`f`はイーズインアウト(2乗): `t < 0.5`なら`2 × t × t`(加速)、`t >= 0.5`なら`1 - 2 × (1 - t) × (1 - t)`(減速、前半の逆再生=残り時間`1 - t`で残りの道のりを出して「全体 − 残り」)。つなぎ目`t = 0.5`で両方0.5。関数名の候補`EaseInOut`/`EaseInOutQuad`、置き場所は`Utility/`。導き方: `t`は時間、`f(t)`は道のり。`t × t`は加速だけ(最後が一番速く急停止)、その逆再生が減速だけ、前半と後半でつなぐ。前半は0.5で0.25しか進まないので2倍。
  - つまずいた所(次に説明するときの参考): 「関数に通す」の意味(時間と道のりを分けて表で見せたら分かった)、数字の差から速さを読むこと(100mを10フレームの表で各フレームの移動距離を見せたら分かった)、「全体 − 残り」(100mで残り2mの例で分かった)。一度に2歩進めると分からなくなる。
  - **移動のステートから攻撃へ移るきっかけは「移動の回数」(ユーザー決定)**。流れ: 移動中(式で動く、`moveFrame`で着く)→止まる(【地点で止まる長さ】)→決めた回数に達していなければ次の地点へ、達したら攻撃のステートへ(網・扉の前の最後の行き先は真ん中)。`IGunshipBossState`(1面と同じ形)と`GunshipBossMoveState`の雛形はユーザーが作成済み(コンストラクタがまだ引数なしで、親にボスを渡していない)。
  - **`GunshipBossMoveState`のメンバ(ユーザーと決めた)**: 移動した回数`m_moveCount`、出発地点`m_startPos`・目標地点`m_targetPos`(`Vector3`)、フレーム`m_frame`(**移動中と止まっている間で使い回す**。着いたとき・待ち終わったときに0に戻す。同時に数えることはないので使い回せる)、移動中か`m_isMoving`(`bool`。速度はzの追従があり、位置を式で直接決めるので見分けに使えない)。
  - **攻撃はランダムに選ぶ(ユーザー決定、仕様書も直した)**。移動のステートが自分の中で次の攻撃を乱数で選べるので、攻撃の順番をどこかに覚えておく必要はない。候補は形態で変わる(第一=網・ロケット、第二=網・扉・ロケット)。次が網か扉なら最後の行き先を真ん中に。注意: 運で偏る(ロケットが続いて隙が来ない等)。→**同じ攻撃が続くのは2回まで(ユーザー決定)**。「同じ攻撃を続けない」は第一形態(網・ロケットの2つ)だと交互=決まった順と同じになるので採らなかった。前の攻撃と続いた回数は、毎回作り直される移動のステートではなく、ボス本体に覚える。
  - (11/20)入力欄に次の返事の予想が出て答えが見えてしまうので、`~/.claude/settings.json`に`"promptSuggestionEnabled": false`を入れた(ユーザー依頼)。
  - **(11/20・ユーザーの案、ベータの予定)ボスのzの追従を変える**: 今は1面も2面もプレイヤーの速度のzをそのまま写すので、ブースト・ブレーキ・宙返りでもボスとの距離が変わらない。→ボスのzの速さは普段は一定(普通の前進18)で距離も一定、プレイヤーが離れた・近づいたときは、その行動(ブースト・ブレーキ)が終わってから一定の距離に戻る。ブーストで近づける=扉を抜けた人がボスの隙に早く着く得にもなる。目安1〜2h(ブースト・ブレーキ中でない間だけ、目標の距離との差を詰める速さを足す)。扉・網の距離の計算に関わるので、攻撃がひととおりできてから。**コスト表に追加(11/20)**: 「ボス」の下の44行目「ボスとの距離をブースト・ブレーキで変えられるように(終わったら一定の距離に戻る)」1.5h・B・**アルファ**(最初ベータで入れ、ユーザーの指示でアルファに変更)・未完了。**1面・2面共通**(ユーザー確認。`BossBase`に1つ書いて両方で使う。行の説明にも「1面・2面共通」と書き足した)。追加後: アルファ 113/残73、合計残115、1日に必要2.35h。表はExcel(COM)で`ListRows.Add`して足した(表の集計の式に自動で入る)。変更前の控えは`%TEMP%\NovaWing_詳細_backup_20261120.xlsx`。
  - **扉の仕様をユーザーが訂正**(仕様書4-4を書き直し): 開いた状態で出て置かれる→プレイヤーが近づくと震える(予備動作)→閉まる→フェードアウトして消える。ブーストで閉まる前に抜ける/何もしないと当たる/ブレーキで消えてから通る。震える時間はバランス調整。
- ~~**次**: ボスの動き~~ → 下の11/20に続く。

### 2026-11-20 2面のボスの移動(1回目)、ボスをムービーのあとに作る形へ、1面ボスの召喚の修正(Debugビルドで確認)
- **進め方の変更(ユーザー依頼)**: 一度に色々決めず、「1つの動きに要る変数を決める → ユーザーがコードに打つ → 次の1つ」の小さなループで進める。
- **イージング(ユーザー実装)**: [Easing.h](Utility/Easing.h)/[Easing.cpp](Utility/Easing.cpp)の`Easing::EaseInOutQuad(t)`(前半`2*t*t`、後半`1 - 2*(1-t)*(1-t)`)。ファイルの作成とvcxproj登録はClaude。
- **移動のステート(ユーザー実装)**: [GunshipBossMoveState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossMoveState.cpp)。`Enter()`で出発地点と目標(仮で右に1000)を覚え、`Move()`で`timeRatio = frame / move_frame`(floatにして1.0で止める)→`EaseInOutQuad`→`Vector3::Lerp`、zだけは今の位置のまま(追従)。`std::min`のために先頭で`#define NOMINMAX`(`(std::min)(...)`でも可)。
- **ボスにステートを持たせた(ユーザー実装)**: [GunshipBoss.h](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h)の`std::shared_ptr<IGunshipBossState> m_pState`(親の型で持ち、`make_shared<GunshipBossMoveState>`で作る)。作るのは`OnInit()`([GunshipBoss.cpp:49-54](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L49-L54))。**コンストラクタでは`shared_from_this()`が使えない**(まだ`shared_ptr`で管理されていないので`bad_weak_ptr`)。`Update()`は出現後だけ`m_pState->Update()`([GunshipBoss.cpp:67-68](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L67-L68))。ステートの切り替え(`GetNextState`)は2つ目のステートを作るときに足す。
- **ボスが見えなくなった原因**: ボスはCSVの位置(2面はy=30000の空の上)で作られ、`Enter()`がその位置を出発地点として覚えた。ムービーのあとにGameSceneがyを海面へ下ろしても、`Move()`のLerpがy=30000に戻していた。**ステートが位置を覚えたあとで外から位置を変えると、こうなる**。
- **ボスをムービーが終わってから作る形に変えた(ユーザー決定・ユーザー実装)**: 1面・2面とも、最初の名残でボスを空の上に置いていたのをやめた。
  - `GameScene::CreateBoss()`([GameScene.cpp](Scene/GameScene.cpp)): DataSetterで作成→`Init()`→1面なら工場を渡す→zに`m_stageOffsetZ`を足す→HPゲージをUIManagerに登録→`CollisionManager::RegisterBoss`。ムービーの終わり(前はyを下ろしていた所)で呼ぶ。
  - `CollisionManager`はコンストラクタでボスを受け取らず、[CollisionManager.cpp](Manager/CollisionManager.cpp)の`RegisterBoss`で後から受け取る(衝突判定→工場→ボスの順につながっているので、ボスを後から作るにはこれが要る)。ボスがいないフレームは判定に入れない。
  - `GameScene::IsBossDying()`: 「ボスがいて、かつ死にかけている」(`m_pBoss != nullptr && m_pBoss->IsDying()`)。描画の2か所は`!IsBossDying()`(ボスがいないときは「死にかけていない」側に入る)。`IsDead()`も先に空を確かめる。
  - デバッグ用UIのボスの項目は、`Init()`で1回だけ登録されるので、ラムダの中で空を確かめる(文字は`std::wstring(L"ボスなし")`を返す。ラムダの`return`は全部同じ型にする)。
  - CSVのyを海面の高さにした(Claude): [Stage1/Boss.csv](Data/CSV/Stage1/Boss.csv) 50000→0、[Stage2/Boss.csv](Data/CSV/Stage2/Boss.csv) 30000→1000。**Unityから書き出すと戻るので、Unity側のボスの位置も直す必要がある**。`StageConfig`の`bossStandY`は使われなくなった(消してよい、未)。
  - つまずき: `&&`の左から調べる(空の確認を左に置く)、`||`と`&&`の取り違え、`!`の付け忘れで描画の向きが逆になる所。
- **1面ボスの出現位置を遠くした(ユーザー依頼)**: ビームが避けられないほど近かったので、[Stage1/Boss.csv](Data/CSV/Stage1/Boss.csv)のzを64960→**67460**(+2500。+3000も試してユーザーが2500に決めた)。出現後はプレイヤーと同じ速さで進むので、出たときの距離が戦闘中の距離になる。Unity側も同じ値にする必要あり。
- **1面ボスが突進敵を召喚していた不具合(ユーザー実装)**: [SpiderBossSummonState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossSummonState.cpp)が`EnemyType`の`None`の次〜`Max`の手前から選んでいたので、あとから足した`RushEnemy`/`ButterflyEnemy`も選ばれていた。→召喚してよい敵の配列`summon_enemy_types[]`(浮遊・ワーム)から`std::size`と`GetRand`で選ぶ形に。C++の配列の`[]`は名前のうしろ(C#と逆)。残り(任意): `std::size`の戻り値に`static_cast<int>`、`#include <iterator>`、`switch`の不要な`static_cast`。
- `TitleScene.cpp`の`first_stage_no`が0(1面から)になっている(ユーザーの確認用の変更)。
- **次**: 2面ボスの移動の続き。候補は「地点で少し止まってから動く」(ボスを後から作るようにしたので見えなくなる不具合の対策としては不要になったが、止まる動きは作る予定。待つ時間の定数`wait_frame`/`stop_frame`、`m_isMoving`、`m_frame`の使い回し。出発地点と目標は動き出す瞬間に覚える)。ゲームで「ムービーのあとにボスが出て、右に動いて止まる」をまだ見ていない。その後: 候補の地点を乱数で選ぶ → 回数で攻撃へ → 網と放熱 → ロケット → 形態の切り替え → 扉。

### 2026-11-21 【方針】制作を終えたら、Claudeに作らせたデバッグ関係を全部消す(ユーザー)
- **ユーザーの方針(11/21)**: 制作を終えたときに、**Claudeが作ったデバッグ関係の処理は全部消す**。**テストプレイ機能(AIテストプレイ)も消す可能性がある**。そして**デバッグ用の表示(プレイヤーの位置など)は、ImGuiではなく、ゲームの中に`DrawString`などで書く昔のやり方に戻す**(ユーザー自身が戻す)。
- **作るときの注意(Claude)**: デバッグ機能は、あとでまとめて消せるように**`#ifdef _DEBUG`で囲み、`DebugTools/`か、`DebugUI`への登録(`RegisterDebugItems`など)に寄せる**。ゲームの本体の流れの中に、デバッグ専用の処理を混ぜない。足したら、この下の「消す対象の一覧」に書き足す。
- **消す対象の一覧(11/21時点。`_DEBUG`を使っているファイルから拾った)**:
  - フォルダごと: [DebugTools/](DebugTools/)(`ImGui/`、`DebugUI/`、`AutoPlay/`=AIテストプレイ。`PlaytestRecorder`、`AutoPlayBot`/`AutoPlaySystem`/`AutoPlayWorld`)。[NovaWing.vcxproj](NovaWing.vcxproj)と`.filters`の登録、ReleaseのときのImGui除外(`ExcludedFromBuild`)、`.gitignore`の`imgui.ini`も。
  - [Application.cpp](Main/Application.cpp): `DebugUI`の`Init`/`BeginFrame`/`Render`/`Terminate`、**シーン移動のデバッグボタン`RegisterSceneDebugItems`(11/21追加)**とその`#include`/定数、`SetAlwaysRunFlag`、`SetHookWinProc`(ImGui用)。
  - [GameScene.cpp](Scene/GameScene.cpp)/[GameScene.h](Scene/GameScene.h): `RegisterDebugItems()`(最初からやり直す・チュートリアルをスキップ・ワープ・プレイヤー情報・ボス情報・**敵を全員倒す(11/21追加)**・ボスのHPを1/倒す)、デストラクタの`RemoveOwner`、`WarpPlayer`、AIテストプレイ関係。
  - [TargetManager.cpp/.h](Manager/TargetManager.cpp): `DebugDefeatAllEnemies()`(11/21追加)。
  - [Character.h](Game/GameObjects/Actors/Character/Character.h): `DebugSetHealth`。[DebugManager.h/.cpp](Manager/DebugManager.h)(`SetPlayerInvincible`、デバッグ表示の切り替え)と、[Player.cpp](Game/GameObjects/Actors/Character/Player/Player.cpp)の無敵の参照。[InputManager.cpp](Manager/InputManager.cpp)の「ImGuiで文字入力中はゲームの入力を空にする」処理。
  - `_DEBUG`を使っているその他(当たり判定の球などの空間のデバッグ描画。表示の切り替えは「デバッグ表示」チェックボックス): [Building.cpp](Game/GameObjects/Actors/Building/Building.cpp)、[Rock.cpp](Game/GameObjects/Actors/Rock/Rock.cpp)、[BulletBase.cpp](Game/GameObjects/Bullet/BulletBase.cpp)、[ChargeBullet.cpp](Game/GameObjects/Bullet/ChargeBullet.cpp)、各敵(`SpiderBoss`/`SpiderBossBeamState`/`ButterflyEnemy`/`FloatingEnemy`/`RushEnemy`/`TurretEnemy`/`WormEnemy`)、[BossHPGaugeUI.cpp](Game/UI/BossHPGaugeUI.cpp)、[SpecialGaugeUI.cpp](Game/UI/SpecialGaugeUI.cpp)、[TitleScene.cpp](Scene/TitleScene.cpp)、[Game.h](Constants/Game.h)。
  - VS Codeの[tasks.json](.vscode/tasks.json)の「AIテストプレイ」「テスト結果」(タスク2つと`autoPlay*`の入力4つ)。
  - 消す前に確認: 本番の流れで使っている物が`_DEBUG`の中に混ざっていないか(消してReleaseがビルドできることを確認する)。
- **戻し方(ユーザーがやる)**: 画面に出したい値は、`DrawFormatString`/`DrawString`でゲームの中に書く。その手前で、ImGuiに移したときに消した画面内のデバッグ表示([NOTES.md:1907](NOTES.md#L1907)の削除一覧: `GameScene`のFRAME、`Player`の座標・ゲージ・HP、`MovingState`のstick、`FloatingEnemy`の位置、など)が、git履歴から参照できる。

### 2026-11-22 2面ボスの移動地点・網のステート(構え→発射→放熱→移動へ戻る)、下にいるときの上向き補正、ボスの出現距離、チュートリアルの敵の位置(ビルドOK、ゲームで確認)
- **ボスの移動(ユーザー実装)**: [GunshipBossMoveState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossMoveState.cpp)。定数`move_frame`(60)・`stop_frame`(120)・`max_move_count`(3)・地点の配列`move_point[]`(5地点: 左上・中央の上・右上・右下・中央の下。zは0で、zは追従のまま)。`Update()`を`UpdateMoving()`と`UpdateStopping()`に分け、`StartMove()`で「出発=今の位置、目標=`move_point[m_moveCount % 地点の数]`」を決める。止まっている間が終わったとき、**`m_moveCount >= max_move_count`なら`StartMove()`せず、`GunshipBossNetState`へ`ChangeState`**。回数は**固定(ユーザー決定)**。ランダムにするのは「どの攻撃か」だけにする(ランダムを2か所に分けないほうが調整しやすい)。仕様書の「攻撃はランダム」とは別に、今は網しかないので網に固定している。
- **ステートの切り替え(ユーザー実装)**: [GunshipBoss.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp)の`UpdateState()`(`SpiderBoss`と同じ形: `Update`→`GetNextState()`→あれば`Exit`→入れ替え→`Enter`)。**今までは切り替えの処理がなく、`ChangeState`を呼んでも動かなかった**。
- **アニメーターの渡し方(ユーザー決定)**: `GunshipBoss::GetAnimator()`が`ModelAnimator&`を返す([GunshipBoss.h](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h))。`FloatingEnemy::GetAnimator()`と同じ形。ボスに`PlayAnimation`のような関数を足す案は、引数をそのまま渡すだけの関数が増えるので採らなかった(ユーザーの違和感)。ステートはコンストラクタで`m_animator(pBoss.lock()->GetAnimator())`と**参照のメンバ**に持つ。参照のメンバはヘッダでは宣言だけ書き、初期化はコンストラクタの初期化リスト(`ModelAnimator`をステートの中に作るのは不可: 同じモデルに2つのアニメーターがぶつかる)。
- **網のステート(ユーザー実装)**: [GunshipBossNetState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp)/[.h](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.h)。段階`NetPhase`(Ready/Fire/VentOpen/VentLoop/VentClose)を`switch`で分け、段階ごとの`UpdateXXX()`関数。各段階は`m_animator.IsEnd()`を見て次のアニメーション(`NetReady`→`NetFire`→`VentOpen`→`VentLoop`(ループ)→`VentClose`)を再生し、`VentClose`が終わったら`GunshipBossMoveState`に戻す。**`VentLoop`はループ再生なので`IsEnd()`が`true`にならない**→経過フレームを数える`m_ventFrame`と`vent_loop_frame`(180、仮)で終わりを決める。戻ると`m_moveCount`は0から数え直し(3回動いてからまた網)。コメントには、モデルの見た目(装甲が開く、など)ではなく、コード側から見た役割を書く(ユーザー方針)。
  - **まだ作っていない**: 網のレーザー本体(予告の線→本番の線→当たり判定→消える)、放熱中の弱点(ポッドに弾が通る・`GetLockOnPos`)、放熱の赤い発光の切り替え(`Boss2EmissionVent`)、ボスのダメージ、攻撃のランダム選び(今は網のみ)。構えと発射の間にレーザーを足す予定。
- **アニメーションを派手にした(Claude、ユーザー依頼)**: [build_boss2.py](Tools/Boss2Model/build_boss2.py)の動きを約2〜3倍の大きさに(構え: 腕28°・ポッド22°・本体12°でため→行き過ぎ→震え、発射: 本体が200ほど後ろへ14°のけぞりポッド30°、待機: 上下±35、放熱: 装甲60°、ロケットのハッチ80°、のけぞり: 約2倍)。Blenderをバックグラウンドで実行(`C:\bl\blender.exe -b --python`、build→bake→export、約40秒)→`make_all.ps1`で`Boss2.mv1`を作り直した。ユーザー評価は「ちょっと微妙だけどいい。最終的に調整を考える」。**調整する場所**: `ready_pose`の数字、`NetFire`の本体の`loc`のy、`VENT`。**放熱60°が装甲をほかの部品にめり込ませていないかは未確認**(`render_preview.py`で撮れる)。
  - **つまずき(Claude)**: `Boss2.mv1`の中を文字列で検索して「`NetReady`などが入っていない」と誤って言った。DxLibのmv1は名前が文字としてそのまま入っているとは限らない。**変換前のFBXには9個すべて入っていた**。アニメーションが入っているかは、ゲームで再生して確かめる(または`MV1GetAnimNum`)。
- **ボスの出現距離**: [Stage2/Boss.csv](Data/CSV/Stage2/Boss.csv)のzを62960→**60460**(出現Z 56300 + 4160。1面は+8760)。ユーザー依頼で近づけた。`CreateBoss()`([GameScene.cpp:761-768](Scene/GameScene.cpp#L761-L768))で、**CSVのzを「出現位置にいるときのボスまでの距離」として読み、プレイヤーの今のzにその距離を足す**ように変えた(ブーストしながらボス戦に入ると、想定より近くにボスが出る不具合の対策。ユーザー実装)。**Unityから書き出すとCSVが戻るので、Unity側のボスの位置も同じ値に直す**。
- **プレイヤーが上に偏る問題(学校の先生の指摘: プレイヤーがずっと上にいる)**: 原因は、地面すれすれにいるときのレティクルの先に敵がいない(海に敵はいない)ので、プレイヤーが下に降りにくいこと。レティクルを上へ向けるより、**プレイヤーが下にいるほど少し上に傾ける**ことにした。[DefaultRotationState.cpp](Game/GameObjects/Actors/Character/Player/Rotation/DefaultRotationState.cpp)の`AddLowTilt(pPlayer, targetAngle)`(ユーザー実装): `lowRatio = (low_tilt_start_y - 高さ) / (low_tilt_start_y - Game::sea_player_margin)`を0〜1に制限し、`targetAngle -= lowRatio * low_tilt_max_angle`(**符号は`-=`**: スティックを上に倒すと`stick.y`は負で、角度も負が「上向き」)。定数`low_tilt_start_y`(500)と`low_tilt_max_angle`(17°。10°から調整)。**`LerpToAngleX`の前に補正する**(後だと補正が効かない)。`Game::sea_player_margin`は100に戻し、`GameCamera`の`camera_offset_y`は250、`camera_offset_z`は230(ユーザーが調整)。
  - 2面は海との関係でプレイヤーが低くなりにくいが、それは「デザイン上しょうがない」(ユーザー)。1面・2面で数字は同じ。
- **チュートリアルの敵の位置(ユーザー実装)**: [TutorialController.cpp:281-288](Manager/TutorialController.cpp#L281-L288)。今まで`プレイヤーの位置 + offset`で出していたので、**島や水に埋まる敵が出ていた**。→**xはCSVの`offsetX`をそのまま(ステージの中心が基準)、yは定数`tutorial_enemy_y`(500)+`offsetY`、zはプレイヤーのz+`offsetZ`**(前に出す)。CSV([TutorialEnemy.csv](Data/CSV/Stage1/TutorialEnemy.csv))は変えていない(`offsetY`は0/±300のまま「真ん中からの散らし方」として読む)。CSVの`offsetY`を絶対の高さに書き直す案は、列の名前と意味がずれるので採らなかった。
- **デバッグ(Claude)**: [GameScene.cpp](Scene/GameScene.cpp)の「Player」タブに、画面上のY(中心からの差、上が+)の機体・レティクルを表示(`ConvWorldPosToScreenPos`)。`#ifdef _DEBUG`の中で、上の「消す対象の一覧」の`RegisterDebugItems`に含まれる。
- **次**: 網のレーザー本体(構えと発射の間)→ 攻撃のランダム選び(網・ロケット) → ロケット → 形態の切り替え → 扉。2面のボスのHP(今は仮2000)、ボスをディゾルブ対象から外す根本対応、`SpiderBoss`/`GunshipBoss`の追従の共通化、GunshipBossの追従ブロックの順番(1フレーム遅れ)、Unity側のボスの位置とCSVの値の同期(1面のz 67460・y 0、2面のz 60460・y 1000)は未。
- **コミット前の注意**: `Log.txt`は`git checkout -- Log.txt`で戻す。

### 2026-11-23 2面ボスの網の線を描く(準備段階、ユーザー実装、コミット`6c3973c`「2体目のボスの網攻撃準備」)
- **網の線の描画**: [GunshipBossNetState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp)の`Draw()`→`DrawNetLines(lineCol, 左の発射口, 右の発射口)`。`DrawLine3D`で、斜めの線2本ずつ(右上→左下、左上→右下)を`-net_line_num`〜`net_line_num - 1`の12組、`net_line_interval`(400)ずつyをずらして並べ、ひし形の網にする。網の中心は`Enter()`で覚えたボスの位置`m_netPos`のx、高さ`net_center_y`(500)、zは`m_netPos.z - 1200`(仮の直書き)。発射口から網の端へ黄色い線も引く。
  - **伸びる演出**: 構え(`Ready`)の間だけ、`extendRate = アニメーションの再生時間 / 長さ`(1.0で止める)で線の終点を`Vector3::Lerp`で伸ばす。構え中は灰色(`0x444444`)、それ以降は白。構えのアニメーションは`net_ready_anim_speed`(0.3)でゆっくり再生。
  - **発射口の位置**: `GetMuzzlePos(フレーム名)`で`MV1SearchFrame`→`MV1GetFramePosition`(`Muzzle_Net_L`/`Muzzle_Net_R`)。そのために`GetModelHandle()`を`SpiderBoss.h`から[BossBase.h](Game/GameObjects/Actors/Character/Enemy/Boss/BossBase.h)へ移した(1面・2面共通)。
  - [GunshipBoss.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp)の`Draw()`の最後で`m_pState->Draw()`を呼ぶようにした。
  - 移動のステートの`Enter()`で待機アニメーション(`Boss2_Armature|Idle`)を再生するようにした(網から戻ったときに待機に戻すため)。
  - 使っていない定数・変数: `net_extend_frame`(120)と`m_netFrame`(数えているが未使用)。網のzの`1200`は定数にしていない。
- **まだ作っていない(網)**: 当たり判定、網が消える/プレイヤーへ向かって動くなどの本番の動き、予告と本番の見た目の差(今は色だけ)、放熱中の弱点、ダメージ。
- **(11/24)Claudeのつまずき**: 「ノートを見て」で、11/20の項を最新だと思い込み、11/21〜23の進捗(移動・網)を見落として「地点で止まる動きを作ろう」と言ってしまった。NOTES.mdは大きいので、見出しを検索して一番下の項と`git log`を見る。
- **次**: 網の続き(当たり判定と、網の本番の動き)→ 放熱中の弱点とダメージ → 攻撃のランダム選び(網・ロケット) → ロケット → 形態の切り替え → 扉。そのほかは11/22の「次」と同じ。

### 2026-11-24 2面ボスの網の当たり判定(カプセルの形・網のステートに判定を持つ・網のコライダー、ユーザー実装、ビルドOK、まだ登録していない)
- **カプセルの形**: [CapsuleShape.h](Game/Collision/CapsuleShape.h) / [CapsuleShape.cpp](Game/Collision/CapsuleShape.cpp)(形はClaude、中身はユーザーが整理)。線分の始点・終点・半径を持つ。`HitCollision(球)`はDxLibの`HitCheck_Sphere_Capsule`、デバッグ描画は`DrawCapsule3D`。[ColliderShape.h](Game/Collision/ColliderShape.h)の`Shape`に`Capsule`を追加。
- **判定の振り分け**: [CollisionManager.cpp](Manager/CollisionManager.cpp)の`IsHit`に「球とカプセル」「カプセルと球」の2つを追加(`if (capsule.HitCollision(sphere)) return true;`)。
- **網のステート**: [GunshipBossNetState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp)の`Enter()`で、線1本ごとにカプセル(半径`net_line_radius` 30)を作って`m_netCapsules`に入れる(偶数番=右の発射口の線A、奇数番=左の発射口の線B)。`DrawNetLines`は始点・終点をカプセルから取り、添字を2ずつ増やして回す。デバッグ時はカプセルを赤で描く(`#ifdef _DEBUG`)。
  - `GetNetCapsules() const`(const参照で返す)と`IsNetHitActive() const`(発射から`net_active_frame` 90フレームの間。構え中は`false`)を`public`に。`m_netFrame`は構えが終わったら0に戻し、発射からの経過フレームとして使う。
  - `Draw()`は「構え中でもなく、判定中でもなければ描かない」(`if (m_phase != NetPhase::Ready && !IsNetHitActive()) return;`)。Claudeが前に「IsNetHitActiveのちょうど逆」と言ったのは間違い(構え中の予告が消える)。
  - ボスの[GunshipBoss.h](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h)に`GetCurrentState() const`を追加。
- **網のコライダー**: [GunshipBossNetCollider.h](Game/Collision/GunshipBossNetCollider.h) / [GunshipBossNetCollider.cpp](Game/Collision/GunshipBossNetCollider.cpp)。1面の`SpiderBossBeamCollider`と同じ形で、持ち主`GunshipBoss&`を持つ。`IsCollisionActive`=ボスが生きている && 今のステートが網(`dynamic_pointer_cast`) && `IsNetHitActive()`。`GetCollision`はカプセルを`ColliderShape`の配列に詰め替えて返す(判定が有効なときだけ呼ばれる前提。変換を1回にまとめたのはユーザーの依頼でClaude)。
  - つまずき: `std::vector<ColliderShape>`(抽象クラスは値で持てない→`shared_ptr`の配列)、`return`忘れ、`CapsuleShape.h`のinclude忘れ(前方宣言だけだと子クラスのポインタを親のポインタに変換できない)、`bool`の初期値なし。
- **次**: `ColliderTag`に網のタグ(候補`BossNet`)を足して`GetTag()`で返す(今は`ColliderTag()`=`Player`扱いを返しているので要注意) → ボスにコライダーを持たせて`GetColliders`で返す → `CollisionManager`のペアに{網, プレイヤー} → `PlayerCollider`でダメージ → 多段ヒットのダメージ源。そのあと網の調整(`net_active_frame`、半径、zの`1200`の直書き、色)と、11/23の「次」の続き。
- **コミットについて**: シェーダーの`.pso`/`.vso`はビルドで作り直されて変更扱いになるが、コミットには入れていない。

### 進捗（2026-11-24・2面ボス網のEffekseerエフェクト）
- **成果物**: `Data/Effect/BossNet/` に `NetLaser`・`NetWarning`・`NetMuzzle` の `.efkefc`(編集用)と`.efk`(拡大率50で書き出し)、`Texture/NB_*.png`(自作4枚)。生成スクリプトは `tools/gen_net_tex.ps1`(テクスチャ)と `tools/make_efkproj.ps1`(`-Effect Laser|Warning|Muzzle`で`.efkproj`を作る。太さなどの数値はここ)。
- **約束**: 線の長手方向=ローカル+X、原点=始点。動的パラメーターは生成時にしか効かないので、線の層は寿命1Fで毎フレーム出し直している。
- **動的パラメーター**: NetLaser `@In0`=伸びた割合(0〜1)、`@In1`=線の長さ(エディタ単位=ゲーム単位÷50。読み込み倍率1.0のとき)。NetWarning `@In0`=線の長さ。NetMuzzleは無し。
- **ゲーム側は未実装・未確認(実機)**。エディタでのプレビュー(曇り空色と暗い背景、距離3500ユニット相当)までは確認済み。引き継ぎメモは会話の最終報告を参照。

### 2026-11-24 2面ボスの登場ムービー(BossAppear2)を作成(Claude、ユーザー依頼)
- 成果物: `Data/Movie/BossAppear2.ogv`(Theora+無音vorbis、36.4MB)と `.mp4`(25.5MB)。1920x1080・30fps・180フレーム(6秒)。1面の `BossAppear.*` は未変更。コードは未編集(.h/.cpp/.hlsl)。
- 作り方: [Tools/Boss2Model/movie/](Tools/Boss2Model/movie/) の `movie_lib.py`(空・ゲーム式マテリアル・街・雨)、`build_movie.py`(カット・カメラ・ボスの動き、`blender -b --python build_movie.py -- --out <dir>`、全フレーム約3分40秒)、`encode.ps1`(連番PNG→ogv/mp4)。
- **指示書との違い: 2面は海がない(`hasSea=false`)** ので、海の式は使わず、足元は `Data/CSV/Stage2/Building.csv` の地面・線路・ビルを(ループ区間34000〜50000、1周16000)ボスのいる位置に合わせて再現した。ゲームの街は、カメラの後ろ3000〜前13000の範囲にしか無いので、同じ範囲(ワールドz -6000〜10400)だけ置いた。
- 空はSkyBox::Drawと同じ面・UVで6面を正距円筒にして使用。ボスはLightingPS式(Boss2_Base/Normal/Emission/Metallic)。雨は `Rain.efkproj` の4層(NearL/NearR/Mid/Far)の数値(×50)を再現して60fpsで進め、カメラの位置から発生。色変換なし(Raw)。
- カット: ①0〜1.3s遠くの影 ②〜2.3s望遠で飛来 ③〜3.5s道路から見上げて通過 ④〜4.55s回り込み ⑤〜5.25s発射口・顔のアップ ⑥〜6.0sゲームのカメラ位置へ引いて画角90°。最後の0.5秒(165F以降)は揺れなし。最後のコマはIdleの0フレーム目。

### 2026-11-24 2面ボスの網: 筋・伸びる演出・当たり判定とダメージの登録・エフェクトの登録(ユーザー実装+一部Claude)
- **見せ方の決定(ユーザー・11/23)**: 「なぜか網が出てきた」を避けるため、**発射口(`Muzzle_Net_L/R`)から網の始点へ光の筋が走り、そこから斜めの線が伸びる**。右のポッド→右上の始点(線A)、左のポッド→左上の始点(線B)。発射口の位置は`GetMuzzlePos(フレーム名)`(`MV1SearchFrame`→`MV1GetFramePosition`)。筋は外側へ扇状に広がる(交差はしない)。
  - 構えの動きの長さ(`NetReady`は20フレーム)が短く、網が一瞬で出たので、`m_animator.Play(net_ready_anim_name, false, net_ready_anim_speed)`で**再生速度0.3**に。伸びる割合は`GetPlayTime() / GetAnimLength()`(`m_netFrame`は速度に関係なく進むのでずれた)。`DrawNetLines`にまとめ、関数に分けた(`GetMuzzlePos`、`DrawNetLines`、`InitNetCol`)。
  - **最新の設計(ユーザー・11/24)**: 伸びる動きは「予告」ではなく**本番の線**に付ける。**予告=フェードインで全体が出る(伸びない)、本番=ビームとして始点から終点へ伸びる、発射口の筋は本番の線の先端に追尾**。※コードはまだ旧版(準備中に線が伸びる)のまま。エフェクトで置き換える。
- **当たり判定とダメージ(11/24)**: `ColliderTag::BossNet`(ユーザー)、`GunshipBossNetCollider::GetTag()`が`BossNet`(ユーザー)、ボスが`m_netCol`を持つ(`GunshipBoss(.h/.cpp)`、初期化リストで`*this`)、`GunshipBoss::GetColliders() override`が`{ &m_netCol }`を返す、`hit_pairs`に`{BossNet, Player}`(CollisionManager.cpp)、`PlayerCollider`に`BossNet`のケース(ユーザー)。
  - **多段ヒット防止の登録(Claude・ユーザー依頼)**: `DamageSourceType::Net`、`GunshipBossNetCollider::GetOwnerID()`、`CollisionManager::TryGetDamageSource`に`BossNet`のケース(`static_cast<const GunshipBossNetCollider&>`)。ビルドOK。**ゲームでの確認は未**(当たると1回だけ減るか、網が消えたあとまた当たると減るか、赤いカプセルと見た目が合うか)。
- **`GetEffectManager()`を`BossBase`に引き上げ(ユーザー)**: `SpiderBoss.h`から`BossBase.h`へ(1面・2面共通)。
- **エフェクト(別セッションで作成、11/24)**: [Data/Effect/BossNet/](Data/Effect/BossNet/)の`NetWarning`(予告、`@In0`=線の長さ)・`NetLaser`(本番、`@In0`=伸びた割合0〜1・`@In1`=線の長さ)・`NetMuzzle`(発射口の光、パラメーターなし)の`.efkefc`/`.efk`(拡大率50)。**約束**: 線の長手方向=ローカル+X、原点=始点。動的パラメーターは**エディタ単位=ゲーム単位÷50**(読み込み倍率1.0のとき)。**生成時にしか効かないので線の層は寿命1Fで出し直している→毎フレーム`SetDynamicInput`で渡せば効く**。色は`SetColor`の乗算(右=青寄り、左=紫寄り)。作り方は[tools/](Data/Effect/BossNet/tools/)。
  - **登録(Claude・ユーザー依頼)**: `ResourceLoader::EffectID`に`BossNetWarning`/`BossNetLaser`/`BossNetMuzzle`、[ResourceConstants.h](Constants/ResourceConstants.h)にパス3つと倍率(1.0)3つ、`effect_infos`の対応表、`game.effects`の読み込み一覧。ビルドOK。
  - **網のステートの途中(ユーザー)**: `m_pEffectManager`を持ち、`Enter()`で予告を1本だけ`m_netCapsules[0]`の始点に再生(`m_warningEffectPlayH`)。**見えなかった**。原因の見立て: ①`m_netCapsules[0]`(`i=-6`の線A)の始点は**網の中心からx+2500・y約100で、設計上、画面の外**(プレイヤーが飛べる範囲はx±1600・y100〜900) ②`@In0`(長さ)をまだ渡していない。確認のため、`DrawNetLines`の呼び出しと赤いカプセルの描画を**コメントアウト中**(`Draw()`、[GunshipBossNetState.cpp:168-176](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp#L168-L176))。**戻すこと**。
- **次(明日、学校で)**: 全部の線にエフェクトを出す。方針:
  - メンバ: 予告のハンドル配列(`m_warningHandles`)、本番のハンドル配列(`m_laserHandles`)、発射口の光のハンドル(`m_muzzleHandles`、左右2つ)。`m_warningEffectPlayH`は消す。
  - 定数: エディタ単位への換算(50)、本番の伸びるフレーム数(20)、色(右=青寄り、左=紫寄り)。
  - 関数: `PlayWarningEffects()`(`Enter`でカプセルを回して予告を再生)、`PlayLaserEffects()`(準備→発射のとき、予告を止めて本番を再生、発射口の光も)、`UpdateLaserEffects()`(毎フレーム`@In0`=伸びた割合)、`StopNetEffects()`(`net_active_frame`に届いたとき・`Exit()`で全部止める。配列が空なら何もしない)。
  - 1本の渡し方: カプセルの始点・終点から`angle = atan2f(end.y-start.y, end.x-start.x)`(`SetRotation(h, Vector3(0,0,angle))`、**向きが逆なら符号を反転**)、`length = (end-start).Length() / 50`、色はカプセルの添字が偶数=右(線A)、奇数=左(線B)。位置は`m_netCapsules[i]->GetStartPos()`(式を書き直さない)。
  - そのあと: 発射口の筋の追従、予告のフェードイン(`SetColor`のアルファ)、判定が終わったあとのフェードアウト、`DrawNetLines`(仮の線)の削除、**網のzの`1200`の直書きを定数に**、線の本数を減らす(カバー範囲は網の中心からx±1600・y100〜900)。
- **2面ボスの登場ムービー(別セッション、11/24)**: 上の「2面ボスの登場ムービー(BossAppear2)を作成」の項を参照。**ゲーム側は未接続**(`GraphicID`に`BossAppear2Movie`などを足し、`StageConfig`の2面の行のムービーID・距離・高さを差し替える。最終値は別セッションの報告を参照)。
- **デバッグコード(消す対象に追加)**: `GunshipBossNetState::Draw()`の`DrawLine3D`による仮の網と筋(`DrawNetLines`)、赤いカプセルの描画(`#ifdef _DEBUG`)。エフェクトに置き換えたら消す。
- **コミット前の注意**: `Log.txt`は`git checkout -- Log.txt`で戻す。シェーダーの`.pso`/`.vso`は入れない。

### 2026-11-25 網のエフェクトの長さをゲーム単位で渡せるようにした(Claude、ユーザー依頼)・Effekseer MCPの入れ直し
- **きっかけ**: 予告を全部の線に出す一歩目で「エディタ単位への換算の定数(50)」を作ろうとしたら、ユーザーが「わざわざ倍率を作らなくていいようにできないの？」。
- **やったこと**: [NetWarning.efkefc](Data/Effect/BossNet/NetWarning.efkefc)・[NetLaser.efkefc](Data/Effect/BossNet/NetLaser.efkefc)の動的パラメーターの式で、長さを`* 0.02`(=÷50)するようにした(中心は`* 0.01`)。`.efk`も書き出し直し(倍率50は前と同じ)。確認用の入力値は140→7000。[make_efkproj.ps1](Data/Effect/BossNet/tools/make_efkproj.ps1)の式とコメントも合わせた。
  - **ゲーム側は長さをゲーム単位のまま`SetDynamicInput`に渡す**(予告=`@In0`に長さ、本番=`@In0`に伸びた割合0〜1・`@In1`に長さ)。換算の定数は作らない。
  - やり方: Effekseer MCPには動的パラメーターの式を変える機能がないので、`EffekseerCore.dll`(1.80.7。網のエフェクトは1.80.7で保存されていて1.80.6では開けない)を使う小さなプログラムで`Core.LoadFrom`→`Core.Dynamic.Equations`の`Code.SetValue`→`Core.SaveTo`→`Binary.Exporter().Export(Root, 50f)`。
- **Effekseer MCP**: この学校PCで`%LOCALAPPDATA%\EffekseerMcp`が丸ごと消えていて接続できなかった。`C:\dev\EffekseerMcp\tools\setup.ps1 -EffekseerTool <Downloads\Effekseer1.80.7Win\Effekseer1.80.7Win\Tool>`で入れ直し(サーバーは応答OK)。セッションへの再接続は`/mcp`。
- **次**: 予告のハンドルの配列(`m_warningHandles`など)と`PlayWarningEffects()`の宣言 → `Enter()`でカプセルを回して予告を再生(位置=始点、向き=`atan2f`、`@In0`=長さ)。

### 2026-11-25 網のエフェクトを緑+ビリビリに(Claude、ユーザー依頼「黒っぽくではなく緑っぽく、ちょっとビリビリ」)
- **色**: 黒っぽく見えた原因は外側の暗い層(紺黒 `(30,32,52)`/`(25,28,45)`)。深い緑にして濃さも下げ、本体・光・芯・先端・発射口を緑系に。値は[make_efkproj.ps1](Data/Effect/BossNet/tools/make_efkproj.ps1#L100-L145)。
- **ビリビリ**: 新テクスチャ`Texture/NB_Arc.png`(ギザギザの電気を縦8コマ、所々途切れる。[gen_net_tex.ps1](Data/Effect/BossNet/tools/gen_net_tex.ps1#L60)の`NetArc`)。本番の線に`Arc`(2Fごと)・`Arc2`(3Fごと、上下反転)を足し、出し直すたびにコマと明るさを乱数で選んでチラつかせる。光の層`Glow`と発射口の光も明るさを乱数で揺らす。予告にも弱い`Arc`(3Fごと)を足した(本番より弱くして差を残す)。
- `Node`関数に`-colMax`(色の乱数)と`-sheets`(コマを乱数で選ぶ)を追加。動的パラメーターの式・倍率50・ゲーム側の渡し方は変更なし(コード変更不要)。
- 前の版(紺黒)は会話のスクラッチパッドに退避しただけ(消えることがある)。戻すならgitの前のコミットから。
- **未確認**: 実機での見え方、発射口の光のキャプチャ(色だけの変更)。
- **v2(同日、ユーザー「全然ビリビリしてない。がっつりでいい」)**: ゲームで見えていたのは予告の線で、その電気をいちばん弱くしていたのが原因。
  - 電気の画像を`NB_Arc2.png`に(振れ幅・芯の太さを上げ、筋3本、途切れを減らす)。本番の`Arc`は太さ5.0・毎フレーム出し直し、予告の`Arc`も太さ3.5・2Fごとで強く。
  - 線から飛び出す枝分かれの稲妻`NB_Spark2.png`(4x2コマ)を追加。**線上20か所の発生点**(`Spark0`〜`Spark19`、線の長さの(k+0.5)/20の位置の式)から、2〜8F(予告は3〜10F)ごとに、向き・大きさ・コマを乱数で出す。[make_efkproj.ps1](Data/Effect/BossNet/tools/make_efkproj.ps1)の`Sparks`/`SparkNode`。
  - **分かったこと**: 動的パラメーターの式では乱数が作れない(`rand()`・`@P`の乱数範囲・最小/最大に別の式、どれも毎回同じ値)。線上のランダムな位置に出したいときは、決まった位置を複数用意して出す間隔を乱数にする。
  - エディタでは線の始点(x=0)付近にもスパークが固まって見えた(原因未確定。網の線の始点は画面外なので実害は小さい見込み)。本番の線は電気がほぼ白に見える(緑を強めたければ`Arc`とスパークの色を下げる)。
- **v3(同日、ユーザー「まぶしすぎる」)**: 電気(`Arc`/`Arc2`)とスパークの明るさ(アルファ)を約半分にし、色を白から緑寄りに。スパークも少し小さく・少なく(予告4〜12Fごと、本番3〜10Fごと)。電気の形・本数・線の本体はv2のまま。
- **(11/25続き・ユーザー実装)予告を全部の線に出す・止める**: `PlayWarningEffects()`(カプセルを回して始点に`BossNetWarning`を再生、`atan2f(向き.y, 向き.x)`を`SetRotationAxis(h, (0,0,1), 角度)`、`SetDynamicInput(h, 0, 長さ)`、ハンドルを`m_warningEffectHandles`へ)。`StopWarningEffects()`(全部`Stop`して`clear`)を構え→発射の切り替えと`Exit()`で呼ぶ。ゲームで網の形に出て、発射で消えることを確認OK。
  - ユーザー評価: **予告の主張が強くて本番に見える**→あとでエフェクト側で薄く・細く(フェードインと合わせて)。
  - ボスが倒されたときに予告を止める処理はまだない(ボスの死亡の流れを作るときにそこから止める)。
  - 別タスク: Debug時にエフェクトが多いと重い(Effekseerの`_d`ライブラリのため。コスト表113行目、2コスト・C・ベータ)。止めていなかった予告が網のたびに24本ずつ増えていたのも重さの一因だった(止める処理で解消)。
- **次**: 発射のときに本番(`BossNetLaser`、0番=伸びた割合・1番=長さ)を出す→毎フレーム伸ばす→判定が終わったら止める→発射口の光→色(右=青寄り、左=紫寄り)→予告の見た目の調整。

### 2026-11-25 網の本番の線を伸ばす・発射口から先端への光の筋(ユーザー実装+登録とハンドルはClaude)
- **本番の線**: [PlayLaserEffects()](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp#L292)でカプセルごとに`BossNetLaser`を始点に再生(向きは予告と同じ`atan2f`、0番=伸びた割合0、1番=長さ)。[UpdateLaserEffects()](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp#L354)で毎フレーム`progress = m_netFrame / laser_extend_frame`(0〜1にclamp)を0番に渡し、カプセルも`Lerp(始点, m_lineEndPositions[i], progress)`まで伸ばす(元の終点は`InitNetCol()`で`m_lineEndPositions`に保存)。判定が終わったら(`!IsNetHitActive()`)止める。
- **発射口の光(`BossNetMuzzle`)はやめた**: ユーザーが欲しかったのは「発射口から網まで伸びる光の線」で、発射口で光るだけのエフェクトは不要(コードから削除、読み込み登録とファイルは残してある。網の攻撃が完成したら片付ける)。
- **光の筋(`BossNetStreak`)を新規作成**: [NetStreak.efkefc](Data/Effect/BossNet/NetStreak.efkefc)(緑、板2枚を90度ずらし、`@In0`=長さ(ゲーム単位)、寿命1Fで毎フレーム出し直し)。読み込み登録([ResourceLoader.h:182](Manager/ResourceLoader.h#L182)、[ResourceConstants.h:201](Constants/ResourceConstants.h#L201))とハンドルの配列`m_streakEffectHandles`はClaude。
  - 線の順番はA(右の発射口)→B(左の発射口)なので、`i`が偶数なら右・奇数なら左。[GetStreakStartPos(i)](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp#L465)にまとめた。
  - `PlayLaserEffects()`で線と同じ数だけ再生、`StopLaserEffects()`で止める。
  - [UpdateLaserEffects()の373〜396行目](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.cpp#L373-L396)で毎フレーム: 位置を発射口に`SetPos` → 発射口→今の線の先端(`newEndPos`)のベクトル → 長さと向き → **3Dの回転は軸=`Cross(+X, 向き)`、角度=`acosf(clamp(Dot(向き, +X), -1, 1))`で`SetRotationAxis`** → 0番に長さ。
- ゲームで、筋が発射口から線の先端に届いて一緒に伸びることを確認OK。
- **次の候補**: 色(右=青寄り、左=紫寄り)、予告の見た目を弱く(フェードイン)、`DrawNetLines`とデバッグの線の片付け、z 1200の定数化、線の本数を減らすか、ボスが倒されたときにエフェクトを止める。
- **(11/25続き)予告を弱くした(v4、Claude、ユーザー依頼「予告の主張が強くて本番に見える」→「細く薄く+電気なし」を選択)**: [make_efkproj.ps1の'Warning'](Data/Effect/BossNet/tools/make_efkproj.ps1#L157)で、電気(`Arc`)とスパーク20か所を外し、太さを暗い層0.9→0.45・光0.3→0.15、明るさ(アルファ)を120/140/90→70/110/70に。**「ビリビリしたら当たる」で本番と見分ける**。[NetWarning.efkefc](Data/Effect/BossNet/NetWarning.efkefc)と`.efk`を作り直し(式・倍率50・ゲーム側の渡し方は変更なし)。前の版(v3)はスクラッチパッドに退避しただけ(戻すならgitの前のコミットから)。
  - 発射口の光(`BossNetMuzzle`)はコードから外した(読み込み登録とファイルは網の攻撃の完成時に片付け)。
- **(11/25続き)予告の点滅(v5)と本番を細く・暗く(v4)(Claude、ユーザー依頼)**
  - 予告: 暗い層は出しっぱなし、明るい芯(`Light`/`LightCross`)だけ寿命6Fで12Fごとに出す=6F光る→6F消える(1秒に約5回)。[make_efkproj.ps1](Data/Effect/BossNet/tools/make_efkproj.ps1)の`$blinkOn`/`$blinkCycle`で速さを変えられる。予告は長さが変わらないので、出し直しが毎フレームでなくても問題ない。
  - 本番(「まだ少しまぶしい、ちょっと太すぎる」): **本体の太さ1.0は当たり判定(半径30=エディタで直径1.2)に近いので変えない**(見た目を判定より細くすると「当たってないのに当たる」になる)。外側の暗い層1.7→1.4、光1.8→1.4、芯0.5→0.4、電気5.0→3.5、先端3.5→3.0、スパークを少し小さく。光る層(Add)のアルファを約3/4に。もっと細くしたいときは当たり判定の`net_line_radius`(30)も一緒に小さくする。
  - 前の版は会話のスクラッチパッドに退避しただけ(戻すならgitの前のコミットから)。
- **(11/25)網の攻撃はいったん完成(ユーザー判断)**。残り: `BossNetMuzzle`の読み込み登録とファイル(NetMuzzle.efkefc/.efk)の片付け、ボスが倒されたときにエフェクトを止める(ボスの死亡の流れを作るとき)、Debugでエフェクトが重い(コスト表113行目)。
- **(11/25)発射口の光(`BossNetMuzzle`)を片付けた(Claude、ユーザー依頼)**: `EffectID::BossNetMuzzle`([ResourceLoader.h](Manager/ResourceLoader.h#L180))、パスと倍率([ResourceConstants.h](Constants/ResourceConstants.h#L197))、表と読み込みリスト([ResourceLoader.cpp](Manager/ResourceLoader.cpp#L265))、`NetMuzzle.efkefc`/`.efk`、[make_efkproj.ps1](Data/Effect/BossNet/tools/make_efkproj.ps1)の`'Muzzle'`を削除。ビルドOK。

### 2026-11-25 網のレーザーのエフェクトを軽くした(v6、Claude、ユーザー依頼。C++は変更なし、未コミット)
- **ノード数**: 本番 29→4(Line/Arc/Head/Spark)、予告 3→2(Dark/Light)、筋 4→1(Streak)。24本で1フレームのノードは 約700→約96(+筋 96→24)。毎フレームの出し直しは本番1本あたり 約8.5個→3個(+スパーク約3個は同じ)、筋は4個→1個。`.efk` は本番 31KB→4.7KB。
- **やったこと**([make_efkproj.ps1の'Laser'](Data/Effect/BossNet/tools/make_efkproj.ps1#L124)、[gen_net_tex.ps1の焼き込み](Data/Effect/BossNet/tools/gen_net_tex.ps1#L185)):
  - 本番の Dark/Body/Glow/Core を1枚の Blend テクスチャ `NB_LineBake.png` に焼いた(光の明るさの揺れは明るさ違い8コマを乱数で選ぶ)。Arc+Arc2 は `NB_Arc3.png`、筋の Glow+Core は `NB_StreakBake.png` に焼いた。
  - 線と筋は**Y軸固定ビルボード**(Billboard=1、Z回転-90で板の縦を+Xへ)。線を軸にカメラへ回るので横から見ても消えず、90度ずらした板(Cross)を全部外せた。長さは Scale の Y に入れる(式 `NetLaserScaleY`/`NetStreakScaleY`)。
  - スパーク20か所→1つ([NetSparkMin/Max](Data/Effect/BossNet/tools/make_efkproj.ps1#L135))。**位置の最小/最大に別の式を入れると、その間の乱数になる**(エディタ1.80.7で確認。以前のメモ「同じ値になる」は誤り)。出す間隔0.2〜0.45F(平均3個/F、以前と同じ量)。
  - 予告は LightCross だけ外した(正面から見るので)。
- **わかったこと**: 寿命1Fで毎フレーム出し直す層は、消える前の1個と新しい1個が**重なって2回描かれる**(エディタで確認)。以前の加算の層は実際は2倍の明るさだったので、焼き込みもそれに合わせた。予告の暗い層の寿命を延ばすと薄くなるので、寿命1Fのまま。
- **見た目の差**: 正面では線の断面の明るさが数%暗い程度(外していた Cross の分)。スパークの位置が20か所に固定でなく、線上のどこにでも出るようになった。筋は斜めから見ると少しだけ白さが減る(以前は板2枚が重なっていた)。横から見たときは本番の線も消えなくなった。比べた画像は `C:\Users\SakamotoKou\Documents\NovaWing_backup\BossNet_compare\compare_sheet.png`。
- 使わなくなった `NB_Arc2.png`/`NB_Soft.png` は消した。
- **戻し方**: 前の版(エフェクト・テクスチャ・スクリプト)を `C:\Users\SakamotoKou\Documents\NovaWing_backup\BossNet_before_20261125\` に置いた。中身を `Data/Effect/BossNet/` に上書きして、`NB_LineBake.png`/`NB_Arc3.png`/`NB_StreakBake.png` を消せば戻る(gitの前のコミットからでも可)。
- **未確認**: ゲームでの見た目と、Debugでの重さの変化。

### 2026-11-25 2面ボスの放熱の隙: 弱点・胴体の当たり判定、放熱中の赤い発光(コスト表36行目の途中。ユーザー実装+コライダーはClaude)
- コスト表35行目「網のレーザー」を完了に(実コスト6、ユーザー申告)。
- **弱点の球**(ユーザー): ボスに持たせる(A案。ロックオンや跳ね返したロケットが、ステートに関係なく位置を聞けるため)。`Weak_L`/`Weak_R`フレームに半径200の球([GunshipBoss.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp)の`InitWeakSphere`、毎フレームアニメーション更新の後に位置を取り直す)。デバッグ描画は赤。
- **放熱中か**(ユーザー): `GunshipBossNetState::IsVentOpen()`=`VentLoop`の間だけ。ボス側は`GunshipBoss::IsWeakOpen()`(網のステートでなければfalse)。
- **弱点のコライダー**(Claude): [GunshipBossWeakCollider.cpp](Game/Collision/GunshipBossWeakCollider.cpp)。タグ`BossDamage`(プレイヤーの弾・チャージとの組み合わせは登録済み)、`IsVentOpen()`の間だけ有効、ダメージは**2倍**(`weak_damage_rate`、ユーザー「せっかく弱点だから」)。
- **胴体のコライダー**(Claude、ユーザー依頼): [GunshipBossBodyCollider.cpp](Game/Collision/GunshipBossBodyCollider.cpp)。`Body`フレームを中心に横向きのカプセル(±250、半径330、前(-Z)に250出す=「顔に当てると中でエフェクトが出て見える」対策)。1倍、生きていて登場が終わっていれば有効。デバッグ描画は黄色。`GetColliders()`は網→弱点→胴体の順。
- **放熱中の発光**(ユーザー): `IsWeakOpen()`のとき`Boss2EmissionVent`。赤く光る間と弾が当たる間を同じにした(赤いのに効かない時間を作らない)。
- 新しいファイルを作ったら: vcxproj/filtersへの登録と**UTF-8(BOM付き)**が要る(BOMなしだとC4819でコメントが壊れてエラー)。
- **ロックオン(ユーザーが作る)**: 胴体はロックオンできず、**放熱中の弱点だけ**ロックオンできる(1面のボスは今のまま)。今はボスがTargetManagerに登録されていない。敵の位置を見ているのはTargetManager・ReticleUI・ChargeBullet・ReflectedBulletの`GetPos()`。ボスを登録すると`DebugDefeatAllEnemies`がボスも倒すので注意。
- **網のエフェクトの軽量化**: Claude Desktop用のプロンプトを作った(見た目を変えずにノード数を減らす。コードは変えない)。変換ツールを[Tools/EffekseerEq/](Tools/EffekseerEq/Program.cs)に置いた(`dotnet build -c Release -o out`)。
- 閉じた装甲に当たったときの火花と硬い音はベータに回す案(未決定)。

### 2026-11-25 2面ボスの弱点のロックオン(コスト表36行目の途中。ユーザー実装+一部Claude)
- **しくみ**(ユーザー): `EnemyBase`に仮想関数を2つ追加。[CanLockOn()](Game/GameObjects/Actors/Character/Enemy/EnemyBase.h#L63)(普通の敵はtrue)と[GetLockablePos()](Game/GameObjects/Actors/Character/Enemy/EnemyBase.h#L65)(普通の敵は`GetPos()`)。
- **2面のボス**: [CanLockOn](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L188)は`IsWeakOpen()`をそのまま返す(ユーザー)。[GetLockablePos](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L194)はプレイヤーに近い方の弱点の球の位置を返す(Claude、ユーザー依頼)。**第二形態で右のポッドが壊れたら左だけを返すようにする**(コメントで残してある)。
- **TargetManager**: ロック中の敵を外す条件に`CanLockOn()`を追加([TargetManager.cpp:76](Manager/TargetManager.cpp#L76)、ユーザー)、候補探しで`CanLockOn()`がfalseなら飛ばす([105行目](Manager/TargetManager.cpp#L105)、ユーザー)。前・画面内・距離の判定を全部`GetLockablePos()`に置き換えた([59行目](Manager/TargetManager.cpp#L59)、[108〜119行目](Manager/TargetManager.cpp#L108-L119)、Claude)。照準の表示も`GetLockablePos()`([ReticleUI.cpp:87](Game/UI/ReticleUI.cpp#L87)、ユーザー)。
- **ボスの登録**: [GameScene.cpp:757](Scene/GameScene.cpp#L757)で、**1面(Spider)以外のボスだけ**TargetManagerに登録(Claude。1面のボスは今のままロックオンしない)。
- **デバッグの全滅**: [DebugDefeatAllEnemies](Manager/TargetManager.cpp#L156)はボスを飛ばす(登録したボスまで倒してしまうため、Claude)。
- ビルドOK。**ゲームでの確認はまだ**: 放熱中だけ弱点に照準が付く・放熱が終わると外れる、胴体と弱点のダメージ、胴体のカプセルの大きさと前への出し方。
- **次の手**: チャージショットと跳ね返した弾が弱点を追いかけるように、[ChargeBullet.cpp:80](Game/GameObjects/Bullet/ChargeBullet.cpp#L80)と[ReflectedBullet.cpp:45](Game/GameObjects/Bullet/ReflectedBullet.cpp#L45)の`GetPos()`を`GetLockablePos()`にする(ユーザーが書く)。
- **注意**: [TitleScene.cpp:80](Scene/TitleScene.cpp#L80)の`first_stage_no`が**1(2面から始まる)**になっている。2面のテスト用。終わったら0に戻す。

### 2026-11-25 跳ね返した弾・チャージショットが放熱中の弱点を追う(「放熱の隙・弱点・ロックオン位置」完了)
- **変更(ユーザー+Claude)**: `ChargeBullet`と`ReflectedBullet`がターゲットを`std::weak_ptr<GameObject>`で持っていて`GetLockablePos()`(`EnemyBase`の関数)を呼べなかったので、**持つ型を`std::weak_ptr<EnemyBase>`に変更**(前方宣言+`EnemyBase.h`のinclude)。ホーミング先を`GetPos()`→`GetLockablePos()`に。`ChargeBullet`はユーザー、`ReflectedBullet`はClaude(ユーザー依頼)。呼び出し側(`ChargeReadyState`の`GetForcusTarget`、`CollisionManager`の`pShooter`)はもう`EnemyBase`なので変更なし。ビルドOK。
- **ゲームで確認OK(ユーザー)**: 放熱中だけ弱点に照準が付く、弱点と胴体のダメージ、弾の追従、ほかの敵・1面ボスへの影響なし。
- **コスト表**: 「放熱の隙・弱点・ロックオン位置」を実コスト2・完了に(Claude、ユーザー申告)。控えは`%TEMP%\NovaWing_詳細_backup_20261125.xlsx`。
- **注意(次に使うとき)**: 反射弾は開閉を見ずに近い方の弱点を狙う(`GetLockablePos`が開閉を見ないため)。2面ボスのロケットを跳ね返せるようにするとき、閉じている間は胴体を狙わせたいかを決める。
- **次**: 形態変化(ポッドが壊れる、3h)・扉(4h)・ロケット(2h)・攻撃のランダム選び・第二形態のリズム(1h)・2面の登場演出(登場ムービーの接続、2h)・ボスの死亡(1h)。`TitleScene.cpp`の`first_stage_no`が1(2面から)のまま。

### 2026-11-25 自機エフェクトの豪華化 1/N: PlayerBullet(通常弾)(Claude、Effekseer MCPで編集)
- **きっかけ**: 友達に「自機のエフェクトがしょぼい」と言われた。軽さを保ったまま豪華にする。1エフェクトずつ、比較画像を見せてOKをもらってから次へ進める。
- **調べたこと**: HitEffectは[BulletBase.cpp:87](Game/GameObjects/Bullet/BulletBase.cpp#L87)で敵弾([EnemyBullet.cpp:63](Game/GameObjects/Bullet/EnemyBullet.cpp#L63))にも使われる共通エフェクト。触るかどうかはユーザーの確認待ち。自機側は緑、敵弾はマゼンタ。色は案A(緑のまま芯を白く)で進めた(未承認の仮決め)。
- **PlayerBulletの変更**: 芯のテクスチャを`PB_Core.png`(64px。白い芯＋緑のにじみ＋横の短い光)に、軌跡を`PB_TrailCore.png`(64×16。中心に白い線)に変えた。Headは出た瞬間に2.8倍で出し、5フレームで1.5倍まで縮める(スケールカーブ。出始めを強く)。軌跡の寿命は17→12(TrailBackのフェードは9→7)、太さは弾側が太く後ろが細くなるように調整。生成間隔は2のまま(3にすると弾と軌跡の間にすき間ができた)。暗い縁取り(Backing、TrailBack)は残した。
- **数値**: ノード数 5→5、同時インスタンス 21→17(エディタでの実測)、テクスチャは64px×4→64px×4(古い`BulletHead_v1b.png`/`BulletTrail_v1b.png`は削除)。`.efk`は拡大率50で書き出した。
- **注意**: ゲームはエフェクトに回転を渡していない([PlayerBullet.cpp:36](Game/GameObjects/Bullet/PlayerBullet.cpp#L36)は位置だけ)ので、芯を進行方向に伸ばす表現はできない。
- **未確認**: ゲームでの見た目(明るい背景で埋もれないか、ロックオンの照準と紛れないか)。

### 2026-11-25 自機エフェクトの豪華化 2/2: 残りをまとめて(Claude、Effekseer MCP。ユーザー「全部終わるまで確認しなくていい、あとで修正を頼む」)
- **仮決め**: 色は案A(緑のまま芯を白く)。**HitEffectは触っていない**(敵弾と共通。自機弾だけ変えるならコードで分ける必要あり: `PlayerHitEffect`などのIDを足し、[PlayerBullet.cpp:48](Game/GameObjects/Bullet/PlayerBullet.cpp#L48)で`BulletBase::OnHitEnemy`の代わりにそれを`PlayOneShot`)。**StarFlareは変更なし**(もう軽くてきれい。光の筋のテクスチャは対象外のStarFlareBoostと共有なので縮めず、gitで戻した)。
- **ChargeExplosion**: 球全体が白く飛ぶFillFlashを削除(敵弾が見えなくなるため)。閃光は中心のCore(0〜1Fで大きく)と、外へ広がる殻のShellFlash(球の外まで広がる衝撃波)で出す。球・網・背景の寿命40→28F、火花24→14個、稲妻12→8個。CE_Net 1024×512→512×256、CE_Shell/CS_Arc 512→256。ダメージは1フレーム目だけ(`IsExplosionActive`)なので、寿命を変えてもゲームの判定には影響しない。
- **Charging**: HaloGrow/StarGrowをHalo/Starに統合(スケールカーブ＋フェードイン)。完了の20F(コードの`charge_comp_frame`と同じ。タイミングは変えていない)にHalo/Starが一瞬大きくなり、リング(ChargedRing、新規、`CS_Ring.png` 128px)が広がる。吸い込みの火花(Inflow)の生成間隔を少し広げた。Wisp 1024→512(128pxのコマ×16。256にするとコマが64pxになりぼやけるので、原則の例外)、Arc 512→256。古い`Charging_v5/`を削除。
- **PlayerChargeBullet**: 分身5本の軌跡の点を毎フレーム→2フレームごとに(いちばんの重さの原因)。Wisp2(重なっていた層)を削除。発射の瞬間にHalo(9→4.6)とCore(3.6→1.7)が縮む、LaunchRing/LaunchFlashを大きく短く。軌跡のテクスチャを通常弾の`PB_TrailCore.png`と共有(`../PlayerBullet/Texture/`を参照。中心が白い)。Wisp 1024→512、Arc 512→256。
- **BarrelRoll L/R**: ほぼ見えない水滴(DropletR/L)を削除。霧(Mist)の生成間隔を2→3、寿命32→約23。最初の光(CoreFlash)・大きな弧(BigArc)・波紋(AirRipple)を少し緑がかった白に(自機の動きだと分かるように)。Swirl/BigArc 512→256、使わなくなった`BR_Dot.png`を削除。
- **WingSpray L/R**(海のステージで海面近くだけ出る): 高さで変わる式(`SetDynamicInput`)はそのまま、元の値を変えて減らした。噴き出し・刃・水滴・霧の生成間隔を広げ、蒸気は1本あたり4→2個(かわりに濃く大きく)、白い航跡(CutLine/WakeV/Foam)の寿命を約3割短く。Smoke 512→256。しぶきの雲は前より少し小さい。
- **数値(エディタで実測した同時インスタンスのピーク)**:
  | エフェクト | ノード | 同時数 | 一番大きいテクスチャ |
  |---|---|---|---|
  | PlayerBullet | 5→5 | 21→17/発 | 64→64 |
  | ChargeExplosion | 11→10 | 35→24 | 1024×512→512×256 |
  | Charging | 18→17 | 75→68(溜め完了後60→46) | 1024→512 |
  | PlayerChargeBullet | 21→20 | 153→113 | 1024→512 |
  | BarrelRoll(片方) | 14→12 | 79→63 | 512→256 |
  | WingSpray(片方) | 9→9 | 341→191 | 512→256 |
  | StarFlare | 8→8 | 変更なし | 変更なし |
- すべて`.efk`を拡大率50で書き出し、参照しているテクスチャがすべてあることを確認した。
- **未確認**: どれもゲームの中では見ていない。特に、チャージ完了のリングが機体にかぶらないか、分身の軌跡がカクカクして見えないか(点を2フレームごとにしたため)、水しぶきが小さくなりすぎていないか。
- `Data/Effect/WindStreak/`(gitに入っていないフォルダ)はClaudeが作ったものではないので、触っていない。

### 2026-11-25 自機エフェクトをゼロから作り直す(途中。次はChargeExplosionから)
- **依頼**: 前回の調整では「豪華さがほとんど変わらない」と言われたので、既存のノードを捨てて、見た目の設計からやり直す。順番は PlayerBullet → ChargeExplosion → Charging → PlayerChargeBullet → BarrelRoll L/R → WingSpray L/R → StarFlare。HitEffectは敵弾と共通なので対象外。ユーザーは「案の選択や途中の確認は任せる、聞かなくていい」と言っている(最後にまとめて報告すればよい)。
- **統一の方向性(以降のエフェクトもこれに合わせる)**:
  - 色は3色: 芯=白、縁=鮮やかな緑(30,255,120)、影=深い緑(0,40,20)を通常ブレンドで1枚だけ敷く。
  - 形の言葉: 縦長の鋭い菱形の光(NW_Diamond)、細い4方向の十字の閃光(NW_Cross)、くっきりしたリング(NW_Ring)。丸いシルエットは敵弾(マゼンタの丸)と紛れるので避ける。
  - 時間: 出始めの数フレームを一番大きく・明るくして、すぐ縮める(イージングはEaseOutCubic、またはFカーブ)。余韻は短く切る。
- **共通テクスチャ**: [gen_nw_textures.ps1](Data/Effect/_Tools/gen_nw_textures.ps1)で作る(`powershell -NoProfile -ExecutionPolicy Bypass -File Data/Effect/_Tools/gen_nw_textures.ps1 -OutDir <出力先>`)。11枚: Diamond(128)、Cross(128)、Rays(256、放射状の光条18本)、Ring(256)、Plasma(256、ギザギザの爆炎の形)、Spark(32×128)、Trail(64×16、中心に白い線＋緑の縁)、Blades(256、三日月の刃2枚)、Spray(128、扇形の水しぶき)、Soft(64)、Streak(128×16、横の光の筋)。各エフェクトのTextureフォルダに必要な分だけコピーして使う(フォルダ内への相対パスにするため)。`_Tools`フォルダは全部終わったら消してよい。
- **PlayerBullet(完了、保存・書き出し済み)**: コンセプト「光の矢じり」。ノードは4つ(すべてRoot直下)。
  - Muzzle: 十字の閃光。生成時の位置に残る(銃口で弾ける)。5F、7.5→1.0倍、白→緑で消える。
  - Shadow: 菱形、通常ブレンド、深い緑で、少し大きく敷く(暗い縁取り)。
  - Head: 菱形、加算。0Fで3.6倍→4Fで1.8倍。
  - Trail: Track、2Fごと、寿命10。
  - 同時に約8個(前は17)。テクスチャは3枚(128/128/64×16)。古いPB_*.pngは削除。
- **次にやること(この順で)**:
  1. **ChargeExplosion**(ノード10以内、粒子150以内。ワンショットで、ダメージは1F目だけ): 放射状の光条(白、加算)と、同じ形の暗い光条(通常ブレンド)を0〜4Fだけ出す → 中心の芯の閃光 → くっきりしたリングの衝撃波(2→19倍、8F) → 2つ目の緑のリング(遅れて、ゆっくり) → ギザギザのプラズマの本体(6→16倍で回転、約18F) → 放射状に飛ぶSparkを16個(減速) → 薄い緑の残光(約24F)。見た目の半径は今の殻(スケール約17.8)と同じくらいにする。歪み(ディストーション)はDxLib側で動くか分からないので使わない。
  2. **Charging**: タイミングを変えない(完了は20F。コードの`charge_comp_frame`)。暗い菱形＋芯の菱形が0〜20Fで大きくなる、外から縮んで芯に吸い込まれるリング(20Fまで)、吸い込まれる火花、20Fで十字の閃光とリング、完了後は小さく回る十字のきらめき。ノード8つくらい。
  3. **PlayerChargeBullet**: 発射の閃光とリングはその場に残す、大きな菱形＋影、背後でゆっくり回る十字、太いTrack、分身3本(PhantomArmの仕組みは残す。コードが`SetDynamicInput(0)`で集まり具合を渡していて、式(DynamicEquation 2)が位置に効いている)。ノード10以内。**注意**: 今のPlayerChargeBulletは、消した`../PlayerBullet/Texture/PB_TrailCore.png`を参照しているので、作り直すまでは軌跡のテクスチャが表示されない。
  4. **BarrelRoll L/R**: 開始の十字の閃光、リング、回転する三日月の刃(Blades)、両翼の先の螺旋のTrack(Spin→TipR/TipL→Contrailの構造は使ってよい)。ノード8つくらい。
  5. **WingSpray L/R**: ノード4つ以内(Track、扇形のしぶき、水滴、霧)。高さの式(SetDynamicInputの0番。式0〜5: SprayAmount、MistAmount、SprayVelocity、MistSize、VaporCount、BladeLength)はエフェクトの中に残っているので、新しいノードでもXMLの`DynamicEquationMin/Max`で同じ番号を指定する。
  6. **StarFlare**: ノード4つ以内。ZTestはFalseのまま。確認用の機体モデル(Shipノード)は書き出す前に消して、書き出した後に戻す。**SF_*.pngは対象外のStarFlareBoostと共有しているので、縮めたり消したりしない**(新しいテクスチャは別の名前で足す)。
- **作業の注意**: XMLでテクスチャの相対パスを入れると保存に失敗する(param_setで絶対パスを入れる)。テクスチャを縮めたり作り直したりしたら、エディタを再起動して確かめる(エディタがパスごとに覚えているため)。フェードの種類の名前は`WithinLifetime`。比較用の変更前の状態は`git archive HEAD`でスクラッチに取り出して撮る。

### 2026-11-25 自機エフェクトのゼロからの作り直し: 全部完了(上の「途中」の続き)
- 残りの6つも作り直し、保存と`.efk`の書き出し(拡大率50)まで済ませた。すべての`.efk`で、参照しているテクスチャがすべてあることと、確認用の機体モデル(DummyJet)が入っていないことを確認した。**ゲームの中ではまだ見ていない**。
- **ChargeExplosion「溜めた力が一点で爆ぜる」**(9ノード): 0〜5Fに白い光条と暗い光条(通常ブレンド)を、最後の球より大きく出す(明暗の閃光) → 中心の白い芯 → 白いリングの衝撃波(2→20倍) → 遅れて緑のリング → ギザギザのプラズマ(加算と暗い層の2枚、回転しながら6→16倍) → 16本の火花がイーズアウトで飛び散る → 薄い緑の残光(約26F)。古い殻・網・稲妻と、球のモデル(Model/CE_Sphere)は削除。
- **Charging「外から吸い込んで収束する」**(8ノード、完了は今までどおり20F): 暗い菱形＋芯の菱形が0〜20Fで大きくなる。外から縮んで芯に入るリングを4回(5Fごと)、吸い込まれる火花は20Fまで。20Fで十字の閃光とリングが弾け、菱形が一瞬大きくなる。そのあとは、小さく回って脈打つ十字(溜まった状態)。
- **PlayerChargeBullet「大きな光の矢じり＋護衛の分身」**(9ノード): 発射の十字の閃光とリングは発射位置に残る。大きな菱形(7.5→3.6倍)＋暗い縁取り、背後でゆっくり回る十字、太いTrack。分身(PhantomArm)は5本→3本、頭を菱形に。集まり具合の式(コードの`SetDynamicInput(0)`)はそのまま使っている。
- **BarrelRoll L/R「螺旋の光の刃」**(各8ノード): 開始の十字の閃光、リング(機体の向きに固定)、回転する三日月の刃2枚(RはY180°で左右反転)。両翼の先の螺旋のTrackは、中心が白い緑のテクスチャにして寿命を34→20F。
- **WingSpray L/R「水面を切る白い扇」**(各4ノード): CutLine(水面の白い線)、SprayJet(扇形の水の膜、NW_Spray)、Droplets(鋭い水滴)、MistTrail(霧)。高さの式はノードに残したまま。
- **StarFlare「緑白の星」**(4ノード＋確認用のShip): にじみ、横の光の筋、脈打つ十字(90Fで一周)、小さな菱形の芯。ZTestはFalseのまま。SF_*.pngは対象外のStarFlareBoostと共有しているので残した。
- **数値(エディタで実測した同時インスタンス)**:
  | エフェクト | ノード(最初→今) | 同時数(最初→今) | テクスチャ |
  |---|---|---|---|
  | PlayerBullet | 5→4 | 21→約8/発 | 3枚(128,128,64×16) |
  | ChargeExplosion | 11→9 | 35→約24 | 5枚(256×3,32×128,64) |
  | Charging | 18→8 | 75→約22 | 4枚(128,256,32×128,128) |
  | PlayerChargeBullet | 21→9 | 153→約39 | 4枚(128,256,128,64×16) |
  | BarrelRoll(片方) | 14→8 | 79→約27 | 4枚(128,256,256,64×16) |
  | WingSpray(片方) | 9→4 | 341→約40 | 4枚(既存のCutLine/Smokeと、128,32×128) |
  | StarFlare | 8→5(Shipを含む) | 8→5 | 新しく4枚(SF_*は残す) |
- **ゲームで見てほしいところ**: 通常弾の菱形が大きすぎないか(当たり判定より見た目が少し大きい)、チャージ爆発の最初の閃光(画面の大きさに対して)、BarrelRollのリングと刃の向き(「固定」の板なので、機体の向きと合っているか)、WingSprayの扇の向き、StarFlareの光が機体の後ろを隠しすぎないか。
- `Data/Effect/_Tools/`(テクスチャを作るスクリプト)と、WingSprayの古い`.efkproj`は、いらなければ消してよい。

### 2026-11-25 チャージ爆発をもっと強く(ユーザー「しょぼい。画面の邪魔になるぐらいでいい」「歪みも入れていい」)
- 全体を約1.6倍に: 光条 26→42倍(暗い光条も)、芯 20→32倍、衝撃波 20→34倍、2つ目のリング 15→26倍、プラズマ 16→27倍(寿命18→26F、最初は白)、残光 13→22倍(36F)。火花は16→40本、太さ約1.5倍、飛ぶ距離 15→26。
- **ScreenFlash**(新規): 0〜5Fだけ画面全体を緑白に光らせる、大きなソフトな光(90→110倍)。
- **Distortion**(新規): 背景を外側へ押し広げる歪みのリング(`NW_DistortRing.png` 128px、マテリアル「歪み(背景)」、強さ3、4→40倍、16F)。
- ノード 9→11(目安の10を1つ超えた。画面の白飛びと歪みを足したため)。同時に出る数は最大で約50。
- **コードの変更が必要(ユーザーが書く)**: 歪みは、今のコードでは表示されない。[Application.cpp:115](Main/Application.cpp#L115)の`Effekseer_Init(8000)`が成功した後に`Effekseer_InitDistortion();`を1行足す必要がある(DxLibの画面を歪み用にコピーする準備)。毎フレーム画面をコピーするので、少し重くなる。足す前のゲームで爆発の中心に黒い輪が出たら、Distortionノードを非表示にする。
- **未確認**: ゲームでの見た目(白飛びが強すぎないか、敵弾が見えなくなる時間)。

### 2026-11-25 チャージ爆発を長く(ユーザー「一瞬すぎる。継続的な『おーー』がない」)
- 約25F → 約85F(約1.4秒)に。ダメージは1F目だけで、エフェクトはPlayOneShotで出しっぱなしなので、コードの変更は要らない。
- 流れ: 0〜5F 閃光(画面の白飛び・光条・歪み) → **6〜56F 持続**(エネルギーの球`Sustain`がゆっくり脈打ちながら膨らむ。暗い層`DarkPlasma`も同じ長さに延長。脈打つリング`Shock2`を10Fごとに5回、球の中で光の筋`Crackle`がバチバチ光る) → 56〜58F 一気に収束 → 58F〜 大きな十字の閃光`FinalFlash`(64→22倍) → 残光`Afterglow`。
- 中心の芯`Core`は、光条・画面の白飛びと役割がかぶるので削除。ノードは13(目安の10を超えた。持続の層・バチバチ・最後の閃光を足したため)。同時に出る数は最大で約50。
- **未確認**: ゲームで見たときの長さと、球が画面をふさぐ時間(1秒近く、敵弾が見えにくくなる)。長すぎたら、Sustain/DarkPlasmaの寿命と、FinalFlash/Afterglowの出る時間(58F)を同じだけ縮める。

### 2026-11-25 チャージ爆発のぐにゃぐにゃを光球に(ユーザー「ぐにゃぐにゃしたやつがダサい」)
- ギザギザのプラズマ(`NW_Plasma`)をやめて、輪郭のくっきりした光球`NW_Orb.png`(256px。鋭く明るい縁＋中がふわっと光る)にした。最初の爆発(Plasma)と持続の球(Sustain)の2つがこれを使う。暗い層(DarkPlasma)はソフトな暗い円(`NW_Soft`)にして、光球の中を深い緑に見せる。
- `NW_Orb`は[gen_nw_textures.ps1](Data/Effect/_Tools/gen_nw_textures.ps1)にも足した。使わなくなった`NW_Plasma.png`は削除。長さ・ノード数・粒子数は前回と同じ。

### 2026-11-25 ロケットのステートの骨組み、2面ボスをプレイヤーへ2000近づけた
- **ロケットのステート(雛形はClaude、中身はユーザー)**: [GunshipBossRocketState.h](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossRocketState.h)/[.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossRocketState.cpp)(`New-GameClass.ps1`で作成・vcxproj登録)。段階`RocketPhase`(Open/Close)。`Enter()`で`Boss2_Armature|RocketOpen`を再生→`IsEnd()`で`RocketClose`を再生→`IsEnd()`で`GunshipBossMoveState`へ戻す(網のステートと同じ形)。**撃つ処理はまだない**(`Open`と`Close`の間に「撃つ」の段階を挟む予定)。**ゲームでの確認はまだ**(ハッチの開閉アニメーションが見えるか)。
- **次(学校で)**: 移動のステートの攻撃への移り([GunshipBossMoveState.cpp:80-86](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossMoveState.cpp#L80-L86))を、`GetRand(1)`で網/ロケットのランダムにする(`GunshipBossRocketState.h`をinclude)→ゲームでハッチの開閉を確認。「同じ攻撃は2回まで」はボスに前の攻撃と続いた回数を覚えさせて足す。そのあと: 撃つ段階(`Muzzle_Rocket_0〜3`から間隔をあけて、最初はまっすぐ飛ぶ`EnemyBullet`)→少し追う弾(`EnemyBullet`を継ぐ新しいクラス)→跳ね返したロケットのボスへのダメージ(仕様書4-3の【跳ね返したロケットのダメージ】)。
- **2面ボスをプレイヤーへ近づけた(ユーザー依頼)**: [Stage2/Boss.csv](Data/CSV/Stage2/Boss.csv)のzを60460→**58460**(-2000)。出現Z 56300からの距離は4160→**2160**。**Unityから書き出すとCSVが戻るので、Unity側のボスの位置も同じ値(z 58460)に直す**。ムービー(`BossAppear2`)の最後のカメラ(ボスの2700手前)や、網のz(ボスの手前1200)との位置関係が、ボスが近くなって変わるので、ゲームで見え方を確認する(網がプレイヤーに近すぎないか、ムービー明けのボスの大きさ)。

### 2026-11-26 2面ボスの攻撃をランダムで選ぶ(ユーザー実装)
- [GunshipBossMoveState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossMoveState.cpp#L83)の`NextAction()`: 3回移動したら`GetRand(AttackType::Max - 1)`で網/ロケットを選ぶ。`AttackType`(Net/Rocket/Max)は[GunshipBossMoveState.h](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossMoveState.h#L30)のprivate。攻撃を足すときはenumに1行と`case`を1つ。
- ゲームで確認OK: 網とロケット(ハッチの開閉)の両方が出て、どちらの後も移動に戻る。
- **次**: 同じ攻撃は2回まで。移動のステートは攻撃のたびに作り直されるので、前の攻撃と続いた回数はボスに覚えさせる。

### 2026-11-26 2面ボスのポッド(弱点)を黄色に統一、ダメージ用の赤い発光を追加(Claude、ユーザー依頼。モデルだけ)
- ユーザーの指定: 有効な部位・弱点は黄色にする。左右のポッド(紫と水色)を黄色に統一し、開く蓋と、開いた中の放熱口も黄色にする。ダメージのときは蓋と中の両方を赤にする。
- **色**([build_boss2.py:386](Tools/Boss2Model/build_boss2.py#L386)〜): ポッドの光る所は左右とも`pod_glow`(黄色)。レンズは中心が白っぽい黄色。蓋(放熱の装甲と上の板)は新しいマテリアル`pod_armor`([build_boss2.py:408](Tools/Boss2Model/build_boss2.py#L408)。今までは肩と同じ灰色`accent`)。放熱口の羽板`vent`/`vent_hot`は放熱中に黄色く光る([build_boss2.py:403](Tools/Boss2Model/build_boss2.py#L403))。角・爪・蝶番・ポッドの胴は今までどおり暗い色(シルエットと黄色の対比のため)。
- 蓋は**閉じていても黄色く光る**(発光あり)。発光は「ベース色×明るさ＋発光」の**足し算**なので、ベース色を明るい黄色にすると、ダメージの赤を足しても橙にしかならない。そのためベース色は暗い黄土色にして、黄色の大部分を発光で出した(ダメージのとき、ほぼ赤になる)。そのぶん蓋の陰影は弱く、平たく光って見える。
- **発光の画像は3枚**(`make_mat`に`emit_dmg`/`emit_dmg_fn`、出力`OUT_EMIT_DAMAGE`を追加。[bake_boss2.py:182-184](Tools/Boss2Model/bake_boss2.py#L182-L184)):
  - `Boss2_Emission.png`(通常): ポッドの光る所と蓋が黄色、放熱口は光らない。
  - `Boss2_Emission_Vent.png`(放熱中): 放熱口も黄色(今までは赤)。
  - **`Boss2_Emission_Damage.png`(新規・弱点にダメージ)**: 放熱中の画像のうち、ポッドの黄色(蓋・放熱口・光る所)を全部赤にしたもの。ダメージが入るのは放熱中だけなので、放熱中の画像が土台。目・胴の噴射口・壊れたポッドの水色の火花は変えていない。
- 作り直し: build→bake→プレビュー→export(Blender 5.2をバックグラウンドで実行)→`make_all.ps1`。`Data/Model/Boss2.mv1`と`Boss2.fbm/`の6枚を差し替えた。DxLibで確認: フレーム23・アニメーション9本・三角形13064枚は同じ、法線マップあり(接線つき)。プレビューに`game_zoom_damage`/`vent_open_near_damage`を足した([render_preview.py](Tools/Boss2Model/render_preview.py))。
- **ゲーム側(ユーザーが書く)**: ダメージの画像の登録(名前の候補`Boss2EmissionDamage`/`Boss2EmissionHit`)。`Boss2EmissionVent`と同じ4か所: [ResourceConstants.h:130](Constants/ResourceConstants.h#L130)の隣にパス、[ResourceLoader.h:132](Manager/ResourceLoader.h#L132)の隣に`GraphicID`、[ResourceLoader.cpp:90](Manager/ResourceLoader.cpp#L90)の表、[ResourceLoader.cpp:549](Manager/ResourceLoader.cpp#L549)の読み込み一覧。描画の切り替えは[GunshipBoss.cpp:247-255](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L247-L255)。**胴体に当たったとき(`GunshipBossBodyCollider`)にも赤くすると、弱点でない所に当てたのにポッドが赤くなる**。
- 気になる点(未決定): ロケットのハッチの前の縁が**黄と黒の縞**(`hazard`)。弱点=黄色のルールにすると、ハッチも弱点に見える可能性がある。跳ね返したロケットを当てる場所でないなら、色を変える案がある。
- **未確認**: ゲームの中での見え方(雨・曇り空の中での黄色の目立ち方、ダメージの赤)。
- **(同日・ユーザー「黄色すぎる、機体から浮いている」)黄色をくすませた**: 蓋`pod_armor`はベース色(0.34,0.27,0.09)のくすんだ黄色の塗装にし、汚れ・塗装はがれ・パネル割りをほかの装甲と同じ強さで入れ、発光はごく弱く(0.08,0.06,0.01)した(陰影が残る)。光る所`pod_yellow`は(0.75,0.6,0.08)→(0.62,0.5,0.14)、放熱口は(0.95,0.72,0.06)→(0.82,0.6,0.12)。ダメージの赤はそのまま(蓋のベース色が明るくなったぶん、少し橙寄り)。形・UVは同じなので、差し替えたのは画像6枚だけ(`.mv1`はそのまま)。

### 2026-11-26 ポッドにいつでも当たるように(Claude、ユーザー依頼)・コスト表に3行
- `AttackType`を[GunshipBoss.h:22-29](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h#L22-L29)のpublicへ移した(Claude。前の攻撃をボスが覚えるため)。移動のステートは`GunshipBoss::AttackType`で使う。関数名は`DecideNextAction`(ユーザー)。
- **ポッドの当たり判定**([GunshipBossWeakCollider.cpp](Game/Collision/GunshipBossWeakCollider.cpp)): 有効になる条件を胴体と同じ「生きていて登場が終わっている」に。ダメージは`IsWeakOpen()`なら2倍(`weak_open_damage_rate`)、閉じていれば**1/10**(`weak_closed_damage_rate`、通常弾10→1、チャージ100→10)。攻撃力が10より小さい弾だと0になる。胴体は1倍のまま。網のステートのincludeは要らなくなったので外した。コンパイルOK(ゲームが起動中でリンクだけ失敗)。**ゲームで未確認**。
- **コスト表に追加(すべてS・アルファ・未完了)**: 37行目「攻撃を受けたら放熱部分を赤く点滅(1面のボスと同じ仕組み)」1、38行目「ポッドにいつでも当たる」0.5、101行目「チャージショットがボスに当たったときも爆発の見た目を出す(当たり判定は出さない)」0.5。今はボスに当たると、わざと爆発を出さずに弾を消している([ChargeBullet.cpp:143-149](Game/GameObjects/Bullet/ChargeBullet.cpp#L143-L149))。控えは`%TEMP%\NovaWing_詳細_backup_20261126.xlsx`。
- **途中**: 同じ攻撃は2回まで。「2回続いた攻撃を外して残りからランダム」に決定(ユーザー)。`GetRand(Max - 2)`で選び、出た数字を外した番号を飛ばして直す所を考えている。
- **(決定)閉じているポッドに当たったときも赤くする**(ユーザー)。注意: 今の`Boss2_Emission_Damage.png`は放熱中の画像が土台なので、閉じているときに使うと放熱口の光も一緒に出てしまう。37行目を作るときに、閉じている用の赤い画像が要るか確かめる。

### 2026-11-26 通常弾の形を「回る三枚刃」に作り直した(Claude、ユーザー依頼。案の選択はClaudeに任せる、と言われた)
- **(同日、ユーザー「もっとダサくなった」→ 下の「らせんの軌跡」に作り直した。このエントリの三枚刃・テクスチャはもう使っていない)**
- **理由**: 菱形(`NW_Diamond`)＋十字の閃光(`NW_Cross`)がクリスタルに見える。丸(敵の弾)・菱形・前の形(`PB_Core`、`BulletHead_v1b`=緑のぼやけた玉)は使わない、という条件。
- **形**: 白い芯から、鉤のように曲がった緑の刃が3枚出る風車の形(`NW_Pinwheel.png` 128px、刃の中心に白い筋、縁は緑30,255,120)。エフェクトの中で1フレームに15°回る(3枚なので8フレームで同じ形に戻る)。ゲームは位置しか渡していない([PlayerBullet.cpp:36](Game/GameObjects/Bullet/PlayerBullet.cpp#L36))ので、どの向きでも同じに見える形にした。
- **敵の弾との違い**: 形(止まった丸 ↔ とがった三角の風車)、動き(回らない ↔ 回る)、色(マゼンタ ↔ 白と緑)。
- **ノード**([PlayerBullet.efkefc](Data/Effect/PlayerBullet/PlayerBullet.efkefc)。構成と寿命は前と同じ):
  - Muzzle: `NW_Pinwheel`、発射位置に残る、5F、5.5→1倍、60°から-24°/Fで逆向きに回る、白→緑で消える(前は十字7.5倍)。
  - Shadow: 同じ形をひと回り太らせた`NW_PinwheelShadow.png`(白。ノードの色で深い緑0,40,20,200)、通常ブレンド。Headと同じ大きさ・同じ回転で重ねる(前は大きめの菱形だったが、曲がった刃を拡大すると形がずれて縁が片側にしか出なかったため、太らせた専用の絵にした)。
  - Head: `NW_Pinwheel`、加算。0Fで3.6倍→4Fで1.85倍(出た直後に速く縮むカーブ)。刃の先は半径約41(Shadowの縁で約44)で、当たり判定の半径45の内側。
  - Trail: 変更なし(`NW_Trail`、Track、2Fごと、寿命10)。
- **数値**: ノード 4→4、同時インスタンス 約8→約8/発(構成が同じなので数えた値。エディタでは測っていない)、テクスチャ 3枚(128,128,64×16)→3枚(128,128,64×16)。
- **テクスチャ**: `NW_Pinwheel.png`と`NW_PinwheelShadow.png`を追加。PlayerBulletの`NW_Diamond.png`/`NW_Cross.png`は、どの`.efk`からも参照されていないことを確かめて削除した(Charging・PlayerChargeBullet・StarFlareなどは自分のフォルダにある同じ名前の画像を使っているので、影響しない)。生成スクリプトはリポジトリの外(`%TEMP%\nw_pb\tools\gen_triblade.ps1`。`-Width 0.27 -Sweep 0.95`で出力)。
- `.efk`は拡大率50で書き出し、参照している3枚がすべてあることを確認した。エディタを再起動しても新しいテクスチャで表示された。
- **確認(エディタ)**: ゲームのカメラを再現(機体の約410後ろ・250上、弾は機体の200前から出て、カメラから見て72/Fで離れる。エディタ距離=ゲーム距離×0.0244)。明るい空(215,222,232)と海(40,110,160)で、今の菱形・新しい形・敵の弾を0/6/12/18Fで並べた。比較画像はチャットに添付(リポジトリには置いていない)。
- **未確認**: ゲームの中での見え方。特に、①連射したとき(5Fごと)に刃が重なってごちゃごちゃしないか、②回転(15°/F)が速すぎたり遅すぎたりしないか、③発射の閃光(Muzzle)が機体にかぶらないか、④遠くで小さくなったときに丸く見えないか。
- **提案(未着手)**: チャージ弾(PlayerChargeBullet)・Chargingはまだ菱形。統一するなら、チャージ弾の大きな菱形を大きな三枚刃(刃を太く・回転をゆっくり)に替えると、通常弾と同じ仲間に見える。
### 2026-11-26 通常弾を作り直し(2回目): 動きを軌跡に入れた「らせんの軌跡」(Claude、ユーザー依頼)
- **(同日、ユーザー「やっぱ丸型でいいかも。いいのが出来上がらない」→ 下の「丸型」に作り直した。このエントリのらせん・プラズマのテクスチャはもう使っていない)**
- **経緯**: 三枚刃は「もっとダサくなった」。ユーザーの説明は「クリスタルは固形に見えたが、三枚刃はそういう形のものが飛んでいるように見えた。そういう形にするなら動きをつけてほしい。クリスタルは動きがなくても納得できた」。次に揺れるプラズマの光弾(フリップブック)と、こぼれる光の粒を試したが、ゲームのカメラでは6F後に弾が約30px、18Fで20px以下になり、揺れも粒も見えず、菱形と同じ「点＋軌跡」になった。**飛んでいる間の見た目は、ほぼ軌跡と色で決まる**ので、ユーザーの選択で「動きを軌跡で見せる」方向にした。
- **構成**([PlayerBullet.efkefc](Data/Effect/PlayerBullet/PlayerBullet.efkefc)、6ノード):
  - Muzzle: `NW_Burst.png`(128px。長さの不ぞろいな7本の光の筋。十字ではない)、発射位置に残る、5F、5→1倍、回転は乱数で+6°/F、白→緑で消える。
  - Shadow: `NW_PlasmaShadow.png`(下の光弾を太らせた形、白。ノードの色で深い緑0,40,20,200)、通常ブレンド。Headと同じ大きさ・同じコマ。
  - Head: `NW_Plasma.png`(512×512、128pxのコマを4×4=16コマ。白い芯の頭から緑の尾が後ろへ流れ、輪郭が波打ち、頭が脈打つ。整数周期なのでつなぎ目なくループ)、加算、フリップブックは1F/コマでループ。0Fで3.8倍→4Fで2.0倍。上が頭・下が尾(画面では弾が上=奥へ進むので、尾が後ろに来る)。
  - Spin(None): Z軸まわりに48°/Fで回る。子のStrandA/Bを、弾の通り道から半径0.5(ゲームで25)の左右に置く。
  - Spin/StrandA・StrandB: Track、2Fごと、寿命10、`NW_Trail.png`、加算、曲線の分割(SplineDivision)6。**位置も回転も「生成したとき」(WhenCreating)で固定**するのが要点(回転を追従にすると、残った点がすべて今の角度に並んで、まっすぐな平行線になった)。こうすると、残った点がらせん状に並び、2本がからみ合う二重らせんになる。
  - 前のまっすぐな軌跡(Trail)は削除した。
- **前提**: ゲームはエフェクトに位置しか渡さない([PlayerBullet.cpp:36](Game/GameObjects/Bullet/PlayerBullet.cpp#L36))ので、らせんの軸はエディタのZ軸に固定(ゲームの前方＝エディタの-Z)。弾は機体の前方へ飛ぶので、軸とだいたいそろう。機体が左右を向いて(最大22.5°)撃つと、軸が少し斜めになる。
- **数値**: ノード 4→6(描かれるのは5。SpinはNone)。同時インスタンス 約8→**約14**/発(Muzzle1・Shadow1・Head1・Spin1・筋5×2。数えた値で、エディタでは測っていない)。テクスチャ 3枚(128,128,64×16)→4枚(**512×512を2枚**,128,64×16)。目安(4〜5ノード、約8個)を超えているのは、軌跡を2本にしたため(ユーザーには事前に約14と伝えた)。
- **テクスチャの整理**: `NW_Plasma.png`/`NW_PlasmaShadow.png`/`NW_Burst.png`を追加。途中の`NW_Pinwheel*`/`NW_PlasmaBolt*`、元の`NW_Diamond`/`NW_Cross`(PlayerBulletのフォルダの分だけ)は、どの`.efk`からも参照されていないことを確かめて削除。生成スクリプトはリポジトリの外(`%TEMP%\nw_pb\tools\gen_bolt.ps1`)。
- `.efk`は拡大率50で書き出し、参照している4枚がすべてあることを確認。エディタを再起動しても正しく表示された。
- **確認(エディタ、ゲームのカメラ再現。明るい空・海)**: 後ろから見ると、軌跡が小さなカールに見え、毎フレーム形が変わる(回って見える)。敵の弾(まっすぐな尾の丸)とは形も動きも違う。明るい空では、加算の筋が白っぽく薄くなる(前のまっすぐな軌跡と同じ程度)。
- **未確認**: ゲームの中での見え方。特に、①連射(5Fごと)でらせんが重なってうるさくないか、②らせんの回る速さ(48°/F)と太さ、③明るい空での筋の見え方、④機体が横を向いて撃ったときに、らせんの軸のずれが気にならないか、⑤発射の瞬間の光弾(0〜4Fの大きな姿)の見た目。
- **提案(未着手)**: チャージ弾(PlayerChargeBullet)・Chargingはまだ菱形。通常弾と仲間に見せるなら、チャージ弾の太い軌跡を3本のらせんにする、など。
### 2026-11-26 同じ攻撃は2回まで(ユーザー実装)
- ボス([GunshipBoss.h](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h#L65-L70)): `m_prevAttackType`(最初は`Max`=まだ攻撃していない)、`m_sameAttackCount`、`RecordAttack()`([GunshipBoss.cpp:213-226](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L213-L226)。同じなら+1、違えば入れ替えて1)。移動のステートは攻撃のたびに作り直されるのでボスに持たせた。
- 移動のステート: [DecideRandomAttack()](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossMoveState.cpp#L98)。回数が`max_same_attack_count`(2)以上なら`GetRand(Max - 2)`で選び、**外した番号 <= 出た数字なら1足す**(どの攻撃を外しても使える。ユーザーが導いた)。それ以外は`GetRand(Max - 1)`。選んだら`RecordAttack`→`ChangeState`。`Max - 2`の2は「GetRandが0〜n」と「1つ外す」の−1が2つなので定数にしない(コメントで理由を書いた)。
- ビルドOK、ゲームで確認OK(ユーザー「たぶん大丈夫」)。
### 2026-11-26 通常弾とチャージ弾を丸型に(3回目。Claude、ユーザー依頼)
- **(同日の続きで、小さく・鋭くした。テクスチャはNW_OrbEdge/NW_OrbEdgeShadow/NW_BladeRingに替わり、このエントリのNW_Orb/NW_OrbShadow/NW_ArcRingは削除済み。下を参照)**
- **依頼**: 「丸でいいからかっこよくして。チャージショットもクリスタルだから丸型にして」「主要な形は丸で、それ以外の部分でかっこよさを出そう」。敵の弾(マゼンタの丸)とは、色(緑と白)・まわりを回る弧のリング・まっすぐな緑の軌跡で見分ける。
- **共通テクスチャ**(どちらのTextureフォルダにも同じものを置いた。生成スクリプトはリポジトリの外`%TEMP%\nw_pb\tools\gen_orb.ps1`): `NW_Orb.png`(128。白い芯→緑の球、内側に明るい縁の線、外に短いにじみ)、`NW_OrbShadow.png`(64。ふちのやわらかい円。ノードの色で深い緑にする)、`NW_ArcRing.png`(256。円周上の3本の弧(両端が細い)と、内側の6つの小さな目盛り)、`NW_Burst.png`(128。長さの不ぞろいな7本の光の筋)。
- **通常弾**([PlayerBullet.efkefc](Data/Effect/PlayerBullet/PlayerBullet.efkefc)、5ノード、描画順): Trail(Track、2Fごと、寿命10、`NW_Trail`、加算、太さ0.04/0.5/0.9) → Shadow(`NW_OrbShadow`、通常ブレンド、0,40,20,200、0Fで2.9倍→4Fで1.55倍) → Head(`NW_Orb`、加算、3.8倍→2.0倍) → Arcs(`NW_ArcRing`、加算、60,255,140,235、-20°/Fで回る、4.6倍→5Fで2.1倍) → Muzzle(`NW_Burst`、発射位置に残る、5F、5→1倍。暗い影に隠れないよう最後に描く)。見た目の大きさ: 球の直径約62・弧のリングの直径約88(当たり判定の直径90の内側)。
- **チャージ弾**([PlayerChargeBullet.efkefc](Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efkefc)、ノード構成は9のまま): Head(菱形→`NW_Orb`、8.0倍→6Fで4.2倍)、Shadow(菱形→`NW_OrbShadow`、6.2倍→3.3倍)、Halo(十字→`NW_ArcRing`、緑60,255,140,235、-9°/F、9.5倍→7Fで4.6倍)、LaunchFlash(十字→`NW_Burst`、回転は乱数)、PhantomArm/PhantomHead(菱形→`NW_Orb`)。LaunchRing・Trail・PhantomTrailと、分身の集まり具合の式はそのまま。球の直径は約130(当たり判定の半径は32。前の菱形と同じくらいの見た目の大きさにした)。
- **数値**: 通常弾 ノード4→5、同時インスタンス 約8→**約9**/発(数えた値)。テクスチャ3枚→5枚(128,64,256,128,64×16)。チャージ弾 ノード9→9、同時数は変わらない。テクスチャは菱形・十字(128,128)を消して、球・影・弧・筋(128,64,256,128)を足した。
- **テクスチャの整理**: どの`.efk`からも参照されていないことを確かめて削除: PlayerBulletの`NW_Plasma*`(らせん版のもの)、PlayerChargeBulletの`NW_Diamond`/`NW_Cross`。ほかのエフェクト(Charging・StarFlareなど)は自分のフォルダの同じ名前の画像を使っているので影響しない。
- 2つとも`.efk`を拡大率50で書き出し、参照しているテクスチャがすべてあることを確認。エディタを再起動しても正しく表示された。
- **確認(エディタ、ゲームのカメラ再現。明るい空・海)**: 遠くでも「緑の球＋白い縁取り＋まっすぐな軌跡」で、敵のマゼンタの球とは色で見分けられる。明るい空では、加算の弧のリングが白っぽくなる。比較画像はチャットに添付(リポジトリには置いていない)。
- **未確認**: ゲームの中での見え方。特に、①連射したときに弧のリングがうるさくないか、②敵の弾と同じ丸なので、弾が多い場面で見分けられるか(色だけに頼っていないか)、③チャージ弾の発射の瞬間の大きさ(直径約400)、④明るい空での弧のリング。
- **提案(未着手)**: Charging(溜め中)はまだ菱形と十字。溜めた玉がそのまま飛ぶ流れなので、Chargingも`NW_Orb`＋`NW_ArcRing`にそろえるとつながる。
### 2026-11-26 弾を小さく・鋭く、Chargingも丸型に(Claude、ユーザー依頼「ちょっと弾でかいかも。もうちょっと鋭さが欲しい。チャージングも直して」)
- **(同日、ユーザー「ちゃっちい。丸っぽいとかっこよくない。丸でもいいが丸っぽいと思わせないように」→ 下の「光点＋光芒」に作り直した。このエントリの球・刃のリングのテクスチャは削除済み)**
- **新しいテクスチャ**(3つのTextureフォルダに同じものを置いた。生成スクリプトはリポジトリの外`%TEMP%\nw_pb\tools\gen_sharp.ps1`):
  - `NW_OrbEdge.png`(128): 縁が硬い球(半径0.80)。縁のすぐ内側に白い細い線、白い芯、縁に向かって深くなる緑、外のにじみはごく短い。
  - `NW_OrbEdgeShadow.png`(64): 縁のはっきりした円(半径0.88)。
  - `NW_BladeRing.png`(256): 3枚の三日月の刃。進む側の頭は太く、まっすぐに切り落とし、後ろは針のように細くなる(明るさも後ろへ少し落とす)。前の四角い目盛りはやめた。
- **通常弾**([PlayerBullet.efkefc](Data/Effect/PlayerBullet/PlayerBullet.efkefc)): Head 3.8→2.0倍 を **2.5→1.25倍**(球の直径 約62→約50)、Shadow 1.28倍、Arcs(刃のリング) 2.1→**1.7倍**(直径 約88→約72)・回転 -20→**-24°/F**、Trailの太さ 0.04/0.5/0.9→**0.02/0.32/0.6**。ノード5・同時数約9は同じ。
- **チャージ弾**([PlayerChargeBullet.efkefc](Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efkefc)): Head 8.0→4.2倍 を **5.0→2.6倍**(球の直径 約130→約104)、Shadow 4.95→2.57倍、Halo(刃のリング) 7.6→**3.7倍**・-12°/F、分身の頭(PhantomHead)を`NW_OrbEdge`の0.75倍に。
- **Charging**([Charging.efkefc](Data/Effect/Charging/Charging.efkefc)、8ノードのまま、溜め完了20Fは変えていない): Core 菱形→`NW_OrbEdge`(大きさのカーブはそのまま。0F 0.3→19F 1.9→21F 3.0→27F 2.1)、Shadow 菱形→`NW_OrbEdgeShadow`(0.41→1.87→2.87→2.05)、ChargedFlash 十字→`NW_Burst`(回転は乱数)、ChargedStar 十字→`NW_BladeRing`(緑60,255,140,230、-10°/F、2.7↔3.2倍で16Fごとに脈打つ)。Converge・Inflow/Spark・ChargedRingはそのまま。
- **削除**(どの`.efk`からも参照されていないことを確認): PlayerBullet・PlayerChargeBulletの`NW_Orb`/`NW_OrbShadow`/`NW_ArcRing`、Chargingの`NW_Diamond`/`NW_Cross`。これで自機の弾まわりに菱形・十字は残っていない(StarFlareなどは自分のフォルダに持っているので別)。
- 3つとも`.efk`を拡大率50で書き出し、参照しているテクスチャがすべてあることを確認。エディタを再起動しても正しく表示された。
- **確認**: ゲームのカメラ再現は、今回は海の背景だけで撮った(明るい空は前回の版で確認済み。縁の白い線と暗い影は残っているので同じように見えるはず、ただし未確認)。
- **未確認**: ゲームの中での見え方。特に、①通常弾の大きさ(まだ大きい/小さすぎるか)、②刃のリングの回る速さ、③溜め中の球からチャージ弾の球へのつながり(溜め完了後は直径約1.7、発射の瞬間は約4.0→約2.1)、④明るい空。- **(同日・ユーザー依頼)閉じたポッドを1/15に、小数も使う**: `weak_closed_damage_rate = 1.0f / 15.0f`。HPは`int`なので、小数のダメージを[GunshipBossWeakCollider.h](Game/Collision/GunshipBossWeakCollider.h)の`m_storedDamage`にため、整数の部分だけ`TakeDamage`して小数は持ち越す(通常弾10→約0.67、3発で2。チャージ100→約6.67)。0ダメージでも`TakeDamage`を呼ぶ(赤い点滅など当たった反応をあとで足せるように)。ビルドOK、**ゲームで未確認**。
### 2026-11-26 自機の弾を「光点＋光芒」に(5回目。Claude、ユーザー依頼「丸っぽいと思わせないように」→ 試作を見て「光点で進める」)
- **考え方**: 球やリングは輪郭のはっきりした「図形」なので、照準マークのようで安っぽく見えた。丸は白熱したごく小さな点だけにして、そこから**上下に細い光芒**(後ろ側を長く。画面では弾が奥＝上へ進むので、進む向きと形がそろう)を伸ばし、緑のにじみで包む。光芒の長さが毎フレーム変わって瞬く。
- **ぶつかる点(重要)**: 明るい空で見えることと、図形っぽく見えないことはぶつかる。元の菱形が空で見えたのは、はっきりした暗い縁取りがあったから(その硬い輪郭が「固形」に見せていた)。今回は、**光点と光芒の形にそった暗いぼかし**(`NW_LightBoltDark`)を後ろに敷き、外側の広い緑のにじみを弱めた(広い加算のにじみは空で白く飛び、暗い層を打ち消すため)。暗い芯をくっきりした円にすると、輪「o」に見えて丸い図形に戻ったので、ぼかしにした。
- **テクスチャ**(3つのTextureフォルダに同じもの。生成スクリプトはリポジトリの外`%TEMP%\nw_pb\tools\gen_flare.ps1`):
  - `NW_LightBolt.png`(256、128pxの2×2=4コマ): 白い点、緑のにじみ、白い光芒(上0.50〜0.68、下0.80〜0.95でコマごとに長さが違う)、光芒を包む緑。
  - `NW_LightBoltDark.png`(同じ4コマ): 芯の後ろのやわらかい暗いぼかし＋光芒の暗いさや(光芒の0.82倍の長さ)。ノードの色で深い緑0,40,20,240。
  - 本体と暗い層は、同じコマ割り(1F/コマ、ループ、開始コマ0)にして、コマがずれないようにした。
- **通常弾**([PlayerBullet.efkefc](Data/Effect/PlayerBullet/PlayerBullet.efkefc)、4ノード): Trail(変更なし: 0.02/0.32/0.6) → Shadow(`NW_LightBoltDark`、通常ブレンド) → Head(`NW_LightBolt`、加算) → Muzzle(`NW_Burst`)。Head・Shadowとも0Fで6.0倍→4Fで3.2倍。回る刃のリング(Arcs)は削除。同時インスタンス 約9→**約8**/発。テクスチャ4枚(256,256,128,64×16)。見た目: 光芒の上下の端まで約±75、緑のにじみの半径は約20(当たり判定の半径45)。
- **チャージ弾**([PlayerChargeBullet.efkefc](Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efkefc)、9ノード): Head・Shadowを光点に(13倍→6Fで8倍)、Halo(刃のリング→`NW_Burst`の放射状の光の筋、緑60,255,140,220、-3°/F、10倍→7Fで5.5倍)、分身の頭(PhantomHead)も光点(1.6倍、開始コマは乱数)。LaunchFlash・LaunchRing・Trail・分身の軌跡と式はそのまま。
- **Charging**([Charging.efkefc](Data/Effect/Charging/Charging.efkefc)、8ノード、溜め完了20Fは同じ): Core・Shadowを光点に(0F 0.8→19F 4.3→21F 6.8→27F 4.8倍)、ChargedStar(刃のリング→`NW_Burst`、緑60,255,140,200、-3°/F、3.4↔4.0倍で16Fごとに脈打つ)。ChargedFlashは`NW_Burst`のまま。
- **削除**: 3つのフォルダの`NW_OrbEdge*`/`NW_BladeRing`と、途中の試作(`NW_Flare*`/`NW_FlareBolt*`)。どの`.efk`からも参照されていないことを確認した。
- 3つとも`.efk`を拡大率50で書き出し、参照しているテクスチャがすべてあることを確認。エディタを再起動しても正しく表示された。
- **確認(エディタ、ゲームのカメラ再現。明るい空・海)**: 通常弾は、空でも海でも「緑の光点＋白い光芒＋軌跡」として見える。ただし6F以降は20〜30pxの小さな点で、元の菱形と同じくらいの目立ち方。チャージ弾は、発射の瞬間は大きな星形の閃光、そのあとは光点を分身の軌跡が回る見え方。
- **未確認**: ゲームの中での見え方。特に、①通常弾が小さすぎないか(遠くで点になる)、②明るい空で埋もれないか(実際のスカイボックスと海)、③チャージ弾の光点が分身の軌跡に負けていないか、④瞬き(1F/コマ)がうるさくないか、⑤溜め中から発射へのつながり。
### 2026-11-26 軌跡が板に見える → やわらかい光の筋に(Claude、ユーザー依頼「軌跡が板すぎるように見える」)
- **原因**: `NW_Trail.png`(64×16)が、幅いっぱいにほぼ同じ濃さの緑の帯で、縁が硬かった。Trackは平たい帯なので、そのまま板に見えた。
- **対応**: 新しい`NW_TrailGlow.png`(64×16。中心に細い白い線、外へ緑がガウスぼかしで消え、端で完全に透明)に差し替えた(生成スクリプトはリポジトリの外`%TEMP%\nw_pb\tools\gen_trailglow.ps1`)。重さは変わらない(テクスチャの差し替えだけ)。
  - 通常弾 [PlayerBullet.efkefc](Data/Effect/PlayerBullet/PlayerBullet.efkefc) のTrail: 太さ 0.02/0.32/0.6 → **0.04/0.7/1.25**(縁が透明になって細く見える分を広げた)。
  - チャージ弾 [PlayerChargeBullet.efkefc](Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efkefc) のTrailとPhantomArm/PhantomTrail: テクスチャを差し替え、曲線の分割(SplineDivision)を3に。分身の軌跡の太さ 0.04/0.35/0.55 → **0.06/0.5/0.8**。
- 使わなくなった`NW_Trail.png`は、2つのフォルダとも`.efk`から参照されていないことを確かめて削除。2つとも`.efk`を拡大率50で書き出し、参照しているテクスチャがすべてあることを確認。エディタの再起動後も正しく表示された。
- **未確認**: ゲームの中での見え方(細くなりすぎていないか、明るい空で軌跡が見えるか)。
### 2026-11-26 2面ボスを1.5倍に、プレイヤーから1500遠ざけた(Claude、ユーザー依頼「網に着くまでが速すぎてよけにくい」と指摘を受けた)
- 大きさ: [GunshipBoss.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L33-L34)に`boss_scale = 1.5f`、コンストラクタで`SetScale`。CSVの`scaleX〜Z`はどのDataSetterも読んでいないので、コードの定数にした。弱点の球の半径・胴体のカプセル(長さ・半径・前に出す量)も`* boss_scale`。弱点・胴体・発射口の位置はフレームから取るので、そのまま大きさに付いてくる。
- 距離: [Stage2/Boss.csv](Data/CSV/Stage2/Boss.csv)のzを58460→**59960**(+1500)。出現Z 56300からの距離は2160→3660。**Unity側のボスの位置もz 59960に直す**(Unityから書き出すとCSVが戻る)。
- ビルドOK、**ゲームで未確認**: 網に着くまでの時間、網の出始め(網はボスの中心の400手前に出るが、1.5倍だとボスの前の面が約500手前なので、出た瞬間は網がボスの中に入って見えるかもしれない。ボスはプレイヤーと同じ速さで進むので、すぐ離れる)、左右に動いたとき(x ±1400)に画面からはみ出さないか、登場ムービー明け(カメラはボスの2700手前)のボスの大きさ。- **(同日)コスト表に追加**: 79行目「ブースト中はカメラレンズの水滴が速く流れる」0.5・A・ベータ・未完了(ユーザー依頼)。`ShaderEffect/WaterDropEffect`にブースト中かを渡して流れる速さを変える想定。
- **(同日)ボス1.5倍・+1500はゲームで確認OK**(ユーザー「大丈夫そう」)。

### 2026-11-26 モデルの「フレーム」を「ボーン」にそろえた(Claude、ユーザー依頼「時間のフレームと見間違える」)
- 名前の変更(中身は同じ): GunshipBoss(`weak_bone_name_left/right`、`body_bone_name`、`broken_pod_bone_name`、`m_weakBoneIdxLeft/Right`、`m_bodyBoneIdx`、`m_brokenPodBoneIdx`、`weakBonePosL/R`)、網のステート(`muzzle_bone_name_left/right`、`GetMuzzlePos(boneName)`)、ロケットのステート(`rocket_muzzle_bone_names`、`boneIdx`。`born`の打ち間違いも直した)、SpiderBoss(`leg_bone_stride`)、SpiderBossBeamState(`muzzle_bone_name_r/l`)、TurretEnemy(`turret_bone_name`、`barrel_bone_name`、`turretBoneIndex`/`barrelBoneIndex`、`ApplyBoneRotation()`)。コメントの「フレーム名・番号・位置」も「ボーン」に。
- `Actor::DrawFrameWithLighting`は残した(DxLibの`MV1DrawFrame`で部品ごとに描画する意味で、ボーンではないため)。DxLibの関数名(`MV1SearchFrame`など)もそのまま。
- これからは名前でも説明でも、モデルの目印は「ボーン」、「フレーム」は時間だけ。
### 2026-11-26 プレイヤーの取得をEnemyBaseにまとめた(Claude、ユーザー依頼)
- [EnemyBase.h](Game/GameObjects/Actors/Character/Enemy/EnemyBase.h#L67-L68)に`std::weak_ptr<Player> GetPlayer() const`を足した(`m_pPlayer`はもともとEnemyBaseにある)。全部の敵・ボスで使える。
- SpiderBossの同じ関数は消した(EnemyBaseのものを使う)。FloatingEnemyの`GetPlayerPos`/`GetPlayerVel`/`GetPlayerFoward`を消し、ActiveState・HideStateを`GetPlayer().lock()->GetPos()`などに変えた(`Player.h`をinclude)。
- ロケットのステート([GunshipBossRocketState.cpp:125](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossRocketState.cpp#L125))はユーザーが書いている途中なので触っていない。
### 2026-11-26 ロケットの撃つ段階(ユーザー実装)
- [GunshipBossRocketState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossRocketState.cpp): `RocketPhase`にFireを追加し、Open→Fire→Closeを`switch`で関数に分けた。開き終わったら1発目→`rocket_fire_interval`(30F)ごとに撃つ→`max_rocket_fire_num`(3発)撃ったら閉じる。撃つ位置は`rocket_muzzle_bone_names`(Muzzle_Rocket_0〜3)を撃った数%4で順番に。向きはタレットと同じ(プレイヤーへの向き×`rocket_move_speed`(20)+プレイヤーのzの速さ)、攻撃力`rocket_attack_power`(15)。`EnemyBullet`で撃ち、`pTarget`(撃った敵)にボスを渡したので、跳ね返すとボス(の`GetLockablePos`=近い弱点)へ飛ぶ。
- ボスに`GetBulletManager()`/`GetCamera()`を追加(ユーザー)。
- ゲームで確認OK(ユーザー「問題ありません」)。
- **次**: 少し追うロケット(`EnemyBullet`を継ぐ新しいクラス、仕様書4-3【ロケットの追いかける強さ】)→跳ね返したロケットのボスへのダメージ(【跳ね返したロケットのダメージ】)。見た目は今は普通の敵の弾(ロケットのモデルは仕様書「長さ約150、正面は-Z」)。
### 2026-11-26 追うロケットの弾 RocketBullet の土台(ユーザー実装)・SpiralBulletのコンストラクタを構造体に(Claude、ユーザー依頼「引数多すぎ」)
- **撃ち方を変えた**: ロケットの速度に足していた「プレイヤーのzの速さ」をやめた(ユーザー「プレイヤーの場所に合わせて撃たなくていい」)。[GunshipBossRocketState.cpp](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossRocketState.cpp#L142-L151)は「プレイヤーへの向き×`rocket_move_speed`」だけ。プレイヤーは前へ約18で進むので、プレイヤーから見るとロケットは約38で迫ってくる(タレットの弾の約2倍の感覚)。**ゲームで速すぎないか確認**、速ければ`rocket_move_speed`を下げる。
  - zの速さを足さないので、追うときは`ReflectedBullet`と同じく速度全体を`Lerp`で曲げればよい(足していると、曲げる前にzの速さを引いて足し戻す必要があった)。
- [RocketBullet.h](Game/GameObjects/Bullet/RocketBullet.h)/[.cpp](Game/GameObjects/Bullet/RocketBullet.cpp)(ユーザー): `EnemyBullet`を継ぐ。`RocketBulletData`(pos, vel, attackPower, pCamera, pShooter(shared_ptr), pEffectManager)を受け取るコンストラクタ、`m_speed = vel.Length()`。`New-GameClass.ps1`で作成・vcxproj登録。ビルドOK。
- [SpiralBullet.h](Game/GameObjects/Bullet/SpiralBullet.h#L8-L18): `SpiralBulletData`(pos, vel, attackPower, pCamera, pShooter(weak_ptr), startAngle)＋エフェクトの管理者を受け取る形に(ReflectedBulletと同じ形)。[BulletManager.cpp](Manager/BulletManager.cpp#L110-L127)の`CreateSpiralBullet`の中でデータを詰める。`CreateSpiralBullet`の引数は変えていない(構造体で受けるにはBulletManager.hにSpiralBullet.hのincludeが要るため)。蝶の敵の弾は**ゲームで未確認**。
- **次**: RocketBulletの`Update`(`EnemyBullet::Update()`のあと、撃った時のプレイヤーとの距離の8割を進むまでプレイヤーへ`Lerp`で曲げる。撃った時の距離をコンストラクタで覚える)→BulletManagerにロケットの作り方を足す(`m_pEnemyBullets`と`m_pAllBullets`に入れる)→ロケットのステートで使う。
- **(11/27)RocketBulletで撃つようにした(ユーザー実装、ゲームで確認OK「いい感じ」)**: [RocketBullet.cpp:15-19](Game/GameObjects/Bullet/RocketBullet.cpp#L15-L19)の`Update`は今は`EnemyBullet::Update()`だけ(まっすぐ飛ぶ)。[BulletManager.h:32](Manager/BulletManager.h#L32)に`BulletType::RocketBullet`、[BulletManager.cpp:92-114](Manager/BulletManager.cpp#L92-L114)で`RocketBulletData`を詰めて作り`m_pEnemyBullets`/`m_pAllBullets`へ(`CreateBullet`の`switch`に足したのでBulletManager.hにincludeは増えていない)。[GunshipBossRocketState.cpp:147](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossRocketState.cpp#L147)をRocketBulletに。**次**: 追う動き(8割進んだらやめる)。
- **(11/27)追うロケット(ユーザー実装)**: [RocketBullet.cpp](Game/GameObjects/Bullet/RocketBullet.cpp)の`HomingPlayer`。撃った時のプレイヤーとの距離`m_firstDistance`をコンストラクタで覚え、進んだ割合`progress = 1 - 今の距離/撃った時の距離`が`homing_end_ratio`(0.8)より小さい間だけ、`Lerp`(`homing_strength` 0.1)でプレイヤーへ曲げる。撃った敵が消えていたら追わない。
- **(11/27)跳ね返したロケットは閉じたポッドにも50入る(Claude、ユーザー依頼「調整の話っぽいので実装して」)**: 跳ね返したロケットは閉じたポッドに当たるので、1/15が掛かると1ダメージしか入らなかった。ロケット中にダメージを入れる手段は跳ね返しだけ(仕様書「放熱はロケットの後には来ない」)なので、1/15を掛けない。ダメージ50はユーザー決定(3発で150=HPの7.5%)。
  - [BulletBase.h:42](Game/GameObjects/Bullet/BulletBase.h#L42) `virtual bool IsIgnoreArmor()`(ふだんfalse)、[EnemyBullet.h:25](Game/GameObjects/Bullet/EnemyBullet.h#L25) `virtual int GetReflectAttackPower()`(ふだんは元の攻撃力)。
  - [RocketBullet.h:25-27](Game/GameObjects/Bullet/RocketBullet.h#L25-L27)で両方上書き(true、[RocketBullet.cpp:13](Game/GameObjects/Bullet/RocketBullet.cpp#L13)の`reflect_attack_power` 50)。
  - [ReflectedBullet.h](Game/GameObjects/Bullet/ReflectedBullet.h#L16)の`ReflectBulletData::isIgnoreArmor`→メンバー`m_isIgnoreArmor`→`IsIgnoreArmor`の上書きで返す。[CollisionManager.cpp:475・479](Manager/CollisionManager.cpp#L475-L479)で元の弾から渡す(仮想関数なので、CollisionManagerはロケットを知らなくていい)。
  - [GunshipBossWeakCollider.cpp:14・47-50](Game/Collision/GunshipBossWeakCollider.cpp#L47-L50): 装甲を無視する弾なら閉じていても`ignore_armor_damage_rate`(1.0)、開いていれば今までどおり2倍。
  - ビルドOK、**ゲームで未確認**(跳ね返したロケットでHPが50ずつ減るか、通常弾は閉じたポッドにほぼ効かないままか)。
- **(11/27)上の「装甲を無視する」を作り直した(ユーザー指摘「装甲という言い方はしていない、ダメージが減少するかの方が分かりやすく限定的じゃない」)**: `IsIgnoreArmor`→**`IsDamageReduced`**(ダメージが減らされるか)。[BulletBase.h:42](Game/GameObjects/Bullet/BulletBase.h#L42)でふだんは`true`(どの弾にもある性質なのでBulletBaseに置く。ポッドから見えるのは`BulletBase&`だけなので`dynamic_cast`も要らない)、[RocketBullet.h:25](Game/GameObjects/Bullet/RocketBullet.h#L25)で`false`。跳ね返した弾は[ReflectedBullet.h:16・26・36](Game/GameObjects/Bullet/ReflectedBullet.h#L16-L36)の`isDamageReduced`/`m_isDamageReduced`(初期値true)で受け取って返す。[CollisionManager.cpp:479](Manager/CollisionManager.cpp#L479)で渡す。
  - [GunshipBossWeakCollider.cpp:43-52](Game/Collision/GunshipBossWeakCollider.cpp#L43-L52): 倍率は1倍から始め、開いていれば2倍、閉じていてダメージが減らされる弾なら1/15。`ignore_armor_damage_rate`は消した。
  - やめた案: 跳ね返した弾に専用タグ(ほかの敵の判定で`PlayerBullet`を見ている所が多い)、敵の弾が跳ね返した弾のデータを作る関数(EnemyBullet.hにReflectedBullet.hのincludeが要る)。
- **(11/27)顔も閉じたポッドと同じく1/20にした(Claude、ユーザー依頼「顔もポッドも1/20で、ポッドだけ放熱したら基準の2倍」)**: 今まで胴体(顔)は倍率なしの10をそのまま受けていたので、ポッドの倍率(1/15→1/20、ユーザーが変更)を変えても減り方が変わらなく見えていた。
  - 計算をボスの1か所にまとめた: [GunshipBoss.cpp:199-221](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L199-L221)の`TakeBulletDamage(弾, isPodHit)`。倍率は1倍から始め、ポッドに当たった＋放熱中なら2倍、それ以外でダメージが減らされる弾なら1/20。定数は[GunshipBoss.cpp:50-53](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L50-L53)(`weak_open_damage_rate`、`reduced_damage_rate`)、小数をためる`m_storedDamage`も[GunshipBoss.h:121](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h#L121)へ移した(顔とポッドで共通)。コライダーごとに書くと1/20を2か所で調整することになるため。
  - [GunshipBossWeakCollider.cpp:34](Game/Collision/GunshipBossWeakCollider.cpp#L34)は`true`、[GunshipBossBodyCollider.cpp:35](Game/Collision/GunshipBossBodyCollider.cpp#L35)は`false`で呼ぶ。GunshipBoss.hは`class BulletBase;`の前方宣言だけ。
  - 跳ね返したロケットは減らされない弾なので、顔に当たっても50。ビルドOK、**ゲームで未確認**。
- **ボスのHPゲージ画像の余白(調査)**: [Boss_HP_Ber.png](Data/Image/HPGauge/BossHPGauge/Boss_HP_Ber.png)は180x1280で、色があるのはy=62〜1235だけ(上62px=約4.8%、下44px=約3.4%が透明)。[GraphShaderDraw.cpp:87-117](Utility/GraphShaderDraw.cpp#L87-L117)は画像全体の高さに対して上から削るので、最初の約4.8%のダメージは透明な所を削るだけで減って見えず、残り約3.4%でゲージが見えなくなる。
- **(11/27)ゲージの中身の画像の余白を消した(Claude、ユーザー決定「全てのゲージ、Bにしましょう」＝一般的な作り方に合わせ、中身は色の所だけに切り抜いて、枠の中の位置をコードで指定)**: 余白があると、割合を掛けたときに透明な所も0〜100%に含まれ、減って見えない区間ができる。
  - 切り抜いた位置(元の画像での左上、枠から見た位置になる)と切り抜いた後の大きさ: [Boss_HP_Ber.png](Data/Image/HPGauge/BossHPGauge/Boss_HP_Ber.png) x=30,y=62 → 120x1174(元180x1280)、[HP_Gauge_fix.png](Data/Image/HPGauge/PlayerHPGauge/HP_Gauge_fix.png) x=48,y=25 → 1185x91(元1280x140)、[Special_Gauge.png](Data/Image/SpecialGauge/Special_Gauge.png) x=38,y=27 → 648x87(元720x140)。枠の画像はそのまま。
  - **コードはまだ**: [GaugeUIBase.cpp:90](Game/UI/GaugeUIBase.cpp#L90)が中身を枠の左上に置いているので、今は3つとも中身が左上にずれて見える。`DrawGauge`に「枠から見た中身の位置(元の画像のpx)」を渡し、`hp_gauge_size * uiScale`を掛けて足す予定。
  - **(11/27)コード**: ユーザーが[GaugeUIBase.h:40](Game/UI/GaugeUIBase.h#L40)に`gaugePosInFrame`を足し、[GaugeUIBase.cpp:90](Game/UI/GaugeUIBase.cpp#L90)で`drawPos + gaugePosInFrame * hp_gauge_size * uiScale`(中身の大きさと同じ縮め方)。Claudeが3つのUIに定数`gauge_pos_in_frame`を足して渡した([BossHPGaugeUI.cpp:16](Game/UI/BossHPGaugeUI.cpp#L16) (30,62)、[PlayerHPGaugeUI.cpp:20](Game/UI/PlayerHPGaugeUI.cpp#L20) (48,25)、[SpecialGaugeUI.cpp:16](Game/UI/SpecialGaugeUI.cpp#L16) (38,27))。ビルドOK(90行にdouble→floatの警告)、**ゲームで未確認**。画像を作り直したら、この数字も切り抜いた左上に合わせて直す。
  - **(11/27)ゲームで確認OK**(ユーザー「大丈夫そうです」): 3つのゲージの位置、ボスのゲージがすぐ減って見える、顔も1/20。コメントはユーザーが[GaugeUIBase.cpp:89-93](Game/UI/GaugeUIBase.cpp#L89-L93)を書き直し、定数は`//ゲージを置く位置`(切り抜いた経緯はコードに書かない)。
- **(11/27)ボスのHPゲージを当たるたびに揺らす 1手目(ユーザー実装、ゲームで確認OK「いい感じ」)**: ユーザー決定「A: 当たるたびに、ダメージに合わせた強さで揺らす」(桜井『ノーリアクションを排除せよ』『8つのヒットストップ仕様』『画面振動』)。
  - ボスは受けたダメージの合計を小数のまま数える: [BossBase.h:37-38・46-47](Game/GameObjects/Actors/Character/Enemy/Boss/BossBase.h#L37-L47)の`m_totalHitDamage`/`GetTotalHitDamage()`、[GunshipBoss.cpp:214-219](Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.cpp#L214-L219)で`finalDamage`(攻撃力×倍率)を足す。小数なのでHPが減らない弾(閉じた所の通常弾0.5)でも増える。ゲージはボスの値を読むだけ(ボスはUIを知らない)。
  - ゲージ: [BossHPGaugeUI.cpp:37-52](Game/UI/BossHPGaugeUI.cpp#L37-L52)で前のフレームの合計`m_prevTotalHitDamage`より増えていたら`m_shakeFrame = shake_frame`(10)、毎フレーム減らす。[70-85行](Game/UI/BossHPGaugeUI.cpp#L70-L85)で残りがある間`frameDrawPos`を`GetRand(10) - 5`ずらす。揺れの大きさはまだ固定。
  - UIの更新([GameScene.cpp:530](Scene/GameScene.cpp#L530))は当たり判定([718行](Scene/GameScene.cpp#L718))より先なので、揺れは当たった次のフレームから(1F遅れ、気にならない)。
  - **次**: 揺れの強さをダメージに合わせる(ダメージは0.5〜200まで幅がある)、だんだん収める。1面のクモのボスも揺らすかは未決定。
  - **(11/27)揺れの強さをダメージに合わせるのはやめた**(ユーザー「正直やらなくてもいい、ボスを完成させたほうがいい」)。揺れは固定の±5px・10Fのまま。
### 2026-11-27 ロケットの見た目(Claude、ユーザー依頼「モデルから描画まで」。跳ね返した後もロケットのままにする所はユーザー)
- **突進敵のモデルを使わず新しく作った理由**: 突進敵は顔付きで「撃って倒す敵」。ロケットは「跳ね返す弾」なので、同じ見た目だとルールの違いが分からなくなる(桜井「シルエットが命」)。
- **モデル**: [Tools/RocketModel/build_rocket.py](Tools/RocketModel/build_rocket.py)(Blenderをバックグラウンドで実行、形・画像・FBXまで)→[make_all.ps1](Tools/RocketModel/make_all.ps1)(モデルビューアでmv1にして`Data/Model/Rocket/`へ)。全長150(z-75〜75)、機首が-Z、羽根込みの太さ約39、原点は真ん中。赤い機首(先が光る)・黄色い帯・灰色の胴・X字の羽根(後ろに大4枚、機首の後ろに小4枚)・噴射口(中が光る)。プレイヤーからはほぼ機首側しか見えないので機首の先も光らせた。色は64x64の画像を4x4のマスに塗り分けてUVをマスに置く(焼き付けなし)。画像は`Rocket_Albedo/Normal(平ら)/Metallic/Emission.png`。直すときは`build_rocket.py`の`CELLS`(色)や形を変えて`make_all.ps1`を実行するだけ。
- **登録**: [ResourceConstants.h:62・89-91](Constants/ResourceConstants.h#L62)、[ResourceLoader.h:70・97-99](Manager/ResourceLoader.h#L70)の`ModelID::Rocket`/`GraphicID::Rocket*Map`、[ResourceLoader.cpp:46-48・147・545・561](Manager/ResourceLoader.cpp#L46-L48)。
- **描画**: [RocketModel.h](Game/GameObjects/Bullet/RocketModel.h)/[.cpp](Game/GameObjects/Bullet/RocketModel.cpp)(`Actor`を継ぐ、`New-GameClass.ps1`で作成・vcxproj登録)。弾は`Actor`ではないので、見た目だけを`Actor`の仕組み(ライティングのシェーダー)で描く部品にした。使い方は「作る(カメラを渡す)→毎フレーム`SetPos`・`LookAtDir(速度)`・`Draw`」。`LookAtDir`は突進敵と同じ作り(正面`model_forward`(0,0,-1)から向きたい方向へ回す)。デストラクタで定数バッファを消す(`Actor`は消していない、既存の敵も同じ)。
- [RocketBullet.cpp:29-32](Game/GameObjects/Bullet/RocketBullet.cpp#L29-L32)で敵の弾のエフェクトを止めてモデルを作り、[58行〜](Game/GameObjects/Bullet/RocketBullet.cpp#L58)の`Draw`で描く。ビルドOK、**ゲームで未確認**(大きさ・明るさ・光り方)。
- **残り(ユーザー)**: 跳ね返した後もロケットのまま。今は跳ね返すと`ReflectedBullet`(普通の敵の弾のエフェクト)になる。`ReflectedBullet`にも`RocketModel`を持たせて同じように描けばよい(元がロケットかどうかを跳ね返すときに渡す)。
- **(11/27)向きを「画面の中で進んで見える方向」にした(ユーザー「プレイヤーの方を見るんじゃなくて進む方向に向いてほしい」)**: ロケットの速度はいつもプレイヤーの方を向いている(撃つ向きも追いかけもプレイヤーへ)ので、速度に向けるとずっとプレイヤーを見ているように見えた。プレイヤーも前へ約18で進んでいるので、画面の中での動きは「ロケットの速度−プレイヤーの速度」。[RocketBullet.cpp:58-81](Game/GameObjects/Bullet/RocketBullet.cpp#L58-L81)でこれに向ける(撃った敵かプレイヤーがいなければ速度のまま)。跳ね返した後も同じ考え方で向ける。ビルドOK、**ゲームで未確認**。
- **(11/27)色と大きさを直した(ユーザー「背景とかぶって何も見えない」)**: 灰色の胴(0.30)が曇り空・灰色の街とほぼ同じだった。[build_rocket.py](Tools/RocketModel/build_rocket.py)の`CELLS`で、胴を濃い赤オレンジ+少し発光、機首全体を発光(先は黄白)、帯は黄色の発光に。[RocketModel.cpp:10](Game/GameObjects/Bullet/RocketModel.cpp#L10)の`model_scale`を2倍(全長300)。ビルドOK、**ゲームで未確認**。
- **(11/27)「プレイヤーを狙っているように見えない」の原因**: 追いかけを、プレイヤーが前へ進んでいることを考えずに、ロケット自身の速度で曲げている。プレイヤーから見るとロケットは前後に38で迫るのに、横は横の速さ分しか寄らないので、横のずれが縮みきらないまま近づき、8割で追いかけをやめてかすめて抜ける(横500ずれて撃つと、やめる時点でまだ約200ずれている)。プレイヤーがロケットを正確に追われても、カメラがプレイヤーについてくるので、強さ1.0だと画面の真ん中へまっすぐ来るだけに見える。
  - 直し方(提案): `HomingPlayer`で「`m_velocity`−プレイヤーの速度」(プレイヤーから見た動き)をプレイヤーの方へ`Lerp`で曲げ、曲げた後にプレイヤーの速度を足して戻す。強さは0.03〜0.05くらいで弧が見える。撃ち方(zの速さを足さない)はそのまま。誰が直すかはユーザーに確認中。
- **(11/27)蛇行の修正**: プレイヤーの速度を全部(x・y・z)引いて追いかけると、プレイヤーが左右に動いたぶんロケットもすぐ同じ向きへ寄せ、一緒に蛇行した(ユーザー「きもい」)。機首の向きもプレイヤーの横の速度で首を振っていた。原因はClaudeの「xとyも引く」という説明の誤り。**引くのはプレイヤーの前に進む速度(z)だけ**: 前後に迫る見え方は直り、左右は遅れてじわじわ曲がる(ミサイルらしい弧)。`Draw`の向き(Claude)は[RocketBullet.cpp:72](Game/GameObjects/Bullet/RocketBullet.cpp#L72)でzだけにした。`HomingPlayer`の110・118行はユーザーが直す。`m_speed`はユーザーが消した。
- **(11/27)ゲームで確認OK(ユーザー「いい感じ」)**: [RocketBullet.cpp:108-119](Game/GameObjects/Bullet/RocketBullet.cpp#L108-L119)(ユーザー)でプレイヤーのzの速度`playerVelZ`だけを引いて追いかけ、`homingDir.Normalized()`で速さが落ちないように。強さ`homing_strength`はユーザーが調整中の値。色・大きさ2倍・向きもOK。**残り**: 跳ね返した後もロケットの見た目のまま(ユーザー)。
### 2026-11-27 帰宅時点の状態(跳ね返したロケットの見た目、途中)
- **方針(ユーザー決定「D」)**: `EnemyBullet`には共通のものだけを置く(ユーザーの考え)。「ロケットのモデルを手放す」関数を`EnemyBullet`に足す案(A)は戻り値が`RocketModel`でロケット専用なのでやめた。全部の敵の弾の見た目を共通のクラスにする案は、普通の弾の見た目がエフェクトでモデルではないため新しいクラスが3つ要り、1.5〜2時間かかるのでやめた。ロケット自身が向きを変えて戻る案(B)もタグの切り替えなどで大きいのでやめた。
  - **D**: 跳ね返す所([CollisionManager.cpp:454-483](Manager/CollisionManager.cpp#L454-L483)の`ReflectEnemyBullet`)で、`dynamic_cast`で元の弾が`RocketBullet`か調べ、ロケットなら`RocketModel`を新しく作って`ReflectBulletData`に入れる(持っているモデルを渡すと、`RocketBullet`に手放す関数が要り、手放した後の描画で空のモデルを触る心配があるため、作り直す)。`ReflectedBullet`は「モデルをもらっていればモデル、なければエフェクト」だけで、ロケットを知らない。
- **できている所(ユーザー)**: [ReflectedBullet.h:6・19・43](Game/GameObjects/Bullet/ReflectedBullet.h#L19)(`class RocketModel;`、データの`pModel`、メンバーの`m_pModel`)、[ReflectedBullet.cpp:21-33](Game/GameObjects/Bullet/ReflectedBullet.cpp#L21-L33)(モデルがないときだけエフェクトを出す)、[67-72行](Game/GameObjects/Bullet/ReflectedBullet.cpp#L67-L72)(モデルがないときだけエフェクトの位置合わせ)。
- **次にやること(ユーザー)**:
  1. [ReflectedBullet.cpp:81-86](Game/GameObjects/Bullet/ReflectedBullet.cpp#L81-L86)の3行に`m_pModel->`を付ける(今は`ReflectedBullet`自身の`SetPos`/`Draw`を呼んでいて、**`Draw()`は自分を呼び続けて落ちる**。`LookAtDir`はコメントアウト中)。`RocketBullet::Draw`と同じ形。
  2. [ReflectedBullet.cpp](Game/GameObjects/Bullet/ReflectedBullet.cpp#L1-L3)に`#include "Game/GameObjects/Bullet/RocketModel.h"`を足す。
  3. [CollisionManager.cpp:479](Manager/CollisionManager.cpp#L479)の下で、`dynamic_cast<RocketBullet*>`ならデータの`pModel`に`std::make_shared<RocketModel>(m_pCamera)`を入れる(`RocketBullet.h`と`RocketModel.h`をinclude)。
  - 今は3がまだなので`m_pModel`はいつも空で、1の落ちる所は通らない(ビルドOK、ゲームも今までどおり動く)。
  4. ゲームで確認: 跳ね返したロケットがロケットの見た目のままボスへ飛ぶか、普通の敵の弾(砲台・蝶)は今までどおりエフェクトか。
- **その後**: コスト表の38行(文を「1/20、顔も同じ」に直して完了)・41行(ロケットの攻撃、完了)の実際の時間をユーザーに聞いて完了にする。次の大きいものは39行の形態変化(3h)→42行の第二形態のリズム。
- 今日の残りの確認待ち: 跳ね返したロケットでHPが50ずつ減るか(顔でも50)、ロケットの速さ。

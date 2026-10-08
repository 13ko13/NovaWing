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
- **メンバを前方宣言だけで持つには`std::unique_ptr`**（値で持つとサイズ計算に完全な定義が必要で.hに`#include`が要る）。ただし生成(`make_unique`)・`->`での呼び出し・破棄（デストラクタ）には完全な定義が要るので、`#include`は.cppに移す。デストラクタは必ず.cppで定義する（.hで`= default`にすると不完全型エラー）。
- `unique_ptr`はコピーできず、戻り値にすると所有権ごと渡すことになる。外に使わせるだけなら`*ptr`で中身の参照を返して「貸す」。`weak_ptr`だけで持つと所有者がいなくて即破棄される。`shared_ptr`は共有する相手がいないなら不要で、オーナーへの参照を持つオブジェクトが持ち主より長生きする危険もある。
- 前方宣言しかしていないクラスは、基底クラスへの変換（`PlayerCollider&`→`ICollider&`、ポインタも同じ）ができない。継承関係はそのクラスのヘッダを読むまでわからないため。
- 値を返す関数・代入のつもりで`x - y;`や`obj->Func();`と書いて`return`や`=`を忘れるミスが今日2回あった（`m_rotationZ - X;`、`IsRolling()`の`return`漏れ）。コンパイルが通ってしまうので、警告C4715（値を返さないパス）やC4552（式の結果が使われていない）に注意する。

## 未解決・保留中のタスク

**2026-08-08、コードを確認して完了済みのタスクを削除しました**（BossEnemyのボーン行列コード整理・model_scale復元、Splashエフェクトの空白対応、CSV化はすべて確認済み・完了）。

1. **`FloatingEnemy`だけライティングが不自然に暗い問題（2026-10-01、素材側の調整で対応）**
   - 否定済みの仮説: 180度回転、スキニング設定(`DrawWithLighting(..., true)`にすると消える＝Drone_fix.mv1はスキンメッシュではなく`false`が正しい)、法線マップ(`normalTS=(0,0,1)`にしても暗いまま)。FBXのメッシュノード(Cylinder001)は回転なし・スケール2.54のみで、向きの問題でもない。
   - 原因: `Drone_Albedo.png`自体が暗い(平均RGB 76,49,14)。Unityの Standard(Specular setup) 前提の素材で、Unityでは`Drone_Specular.tga`(アルファ=滑らかさ平均0.78)による艶・スカイボックスの環境光/映り込みで明るく見えていたが、自前シェーダーにはそれらが無い。加えてライト`(1,-1,0.6)`は上から当たるため、カメラ向きの正面はdiffuse約0.38しか受けない。
   - 対処: ゲームが読んでいる`Data/Model/Drone.fbm/Drone_Albedo.png`(mv1内の参照パス。`Drone_fix.fbm`側ではない)を、色相・彩度を保ったまま明度(HSVのV)だけガンマ0.45で持ち上げた(平均RGB 132,92,44)。正面は屋根状パーツの下で法線が下向き気味のため、ユーザー判断で「かなり明るく」に。元画像はgit履歴から復元可能。
   - それでも左側の敵・正面が暗い(ライトが左上手前から当たり、右側の敵は左側面が見えるので明るい)。Albedoの黄色は明度上限に張り付いていてこれ以上上げられないため、`Drone.fbm/Drone_Emission.png`に`Albedo×0.4`を加算した(光の向きに関係なく底上げされる、FloatingEnemyだけに効く)。足りなければ係数を上げて作り直す。
   - シェーダー側でUnityの見た目に寄せるなら、`Drone_Specular.tga`を`tex_metalic`に渡す(ただし今のシェーダーは滑らかさを`.r`から読むので、アルファとの慣習差の調整が要る)のが次の候補。

2. **`near_clip`/`far_clip`が`CapturePS.hlsl`/`WaterPS.hlsl`の2ファイルに重複定義**されている（`LightingPS.hlsl`は既に対象外と確認）。共通`.hlsli`への切り出しが未着手。

3. **DataSetter群（Rock/FloatingEnemy/WormEnemy/BossEnemy）のCSV読み込み〜生成の骨格が重複**。テンプレート基底クラスかフリー関数での共通化が候補、未着手。
   - **`CSVData`基底クラス（`CSVData/CSVData.h`、`virtual void Conversion()`を持つ）が、どのDataSetterからも継承されず活用されていないことが判明（2026-08-18）。** `CSVDataLoader::LoadCSV`は常に`CSVData`そのもの（生の`std::wstring`配列の入れ物）を作って返すだけで、各`XxxDataSetter`（`RockDataSetter`等）はそこから`GetData()`で生配列を取り出し、列番号を直接指定して自前でパースしている。
   - **参考プロジェクト`C:\Users\SakamotoKou\Documents\GitHub\ProjectNeaR`で本来の使い方を確認済み。** `CSVDataLoader::LoadCSV`自体は同じ実装（常に`CSVData`を生成）だが、呼び出し側で`ActorData(std::shared_ptr<CSVData> data)`のように**`CSVData`を受け取るサブクラスのコンストラクタ**を用意し、そこで`Conversion()`相当の変換処理（列番号パース→意味のあるメンバへの変換）を行うパターンだった。`CSVDataLoader`自体の変更は不要で、サブクラス（例:`RockCSVData : public CSVData`）を新設し、今`RockDataSetter::CreateRock`に直書きされている列番号パースをそちらに移す形が本家に忠実な直し方。
   - **保留中のタスクとして記録のみ。着手はまだ先。**

4. **`LightingPS.hlsl`(共通化済み)と`WaterPS.hlsl`間で視線ベクトル・反射ベクトル・specular計算式が重複**。`WaterPS.hlsl`側だけまだ独自実装のまま、`.hlsli`共通化が候補、未着手。

5. **ボスの本格実装（2026-08-21完了）。** 雑魚召喚・ビーム攻撃・登場演出・死亡エフェクトまで実装・動作確認済み。多段ヒット防止は見送り、ダメージ値を1に固定。ビームの当たり判定球配列は判定終端を通り過ぎたものを削除する対応済み。

6. **VS CodeのIntelliSenseで`DxLib::VECTOR`等の型が「不完全な型」表示になる問題、原因未特定のまま保留。** ビルド自体は正常なため実害なし。深追いは費用対効果が低いと判断済み。

7. **`CameraBase`のGameCamera/TitleCamera分割、GameCamera側は完了（2026-08-21）。** 詳細は下記「進捗（2026-08-21・タイトルシーン着手）」参照。`TitleCamera`はまだ空の骨格のみ、これから中身を実装する。

8. ~~リプレイ等で2回目のGameSceneに入ったとき、前回のEffekseerエフェクトが残る問題~~ → **完了（2026-10-01確認）。** `SceneController::ResetScene()`で`m_scenes.clear()`の直前に`GameObjectManager::ClearAll()`を呼ぶ修正済み(詳細は下記「Boostエフェクト追加、シーン切り替え時のEffekseerエフェクト残留バグ修正」)。番号は他の記述から参照されているため欠番にせず残す。

9. **シングルトンクラスが増えすぎている件、いずれ集約用クラス(例: GameServices)への移行を検討したい（2026-09-28・ユーザー提起、未着手）。** 現状`Application`/`InputManager`/`ResourceLoader`/`DebugManager`等が個別に`GetInstance()`を持つ素朴なMeyer'sシングルトン。今回`DebugManager`を追加する際、影響範囲を絞るためいったん同じパターンで作ったが、根本対応としては全マネージャーを1箇所で保持・提供する集約クラスへの置き換えが候補。既存コードへの影響が大きいため別タスクとして着手予定。

10. **影の追加（2026-10-01・方針のみ決定、未着手）。** DxLibのシャドウマップ(`MakeShadowMap`/`ShadowMap_DrawSetup`等)を使う方針。
   - 影を落とす側: オブジェクト全部（プレイヤー・敵・ボス・岩）。受ける側: 理想は海面・ステージ・オブジェクト同士の全部。
   - 前提: 3D描画は全て独自シェーダー（LightingPS/VS・StagePS・WaterPS）なので、`SetUseShadowMap`だけでは影は出ない。受ける処理（シャドウマップを読んで暗くする）は自前のシェーダーに書く必要がある（参考: [Shader/MV1Default_PixelLighting_PS.hlsl](Shader/MV1Default_PixelLighting_PS.hlsl)の242〜307行目、`t8`〜`t10`・`g_ShadowMap`）。シャドウマップ作成中に独自シェーダーが有効のままだと、深度ではなく色が書き込まれる点にも注意。
   - 負荷: 主な増加はシャドウマップ作成で`DrawAll()`相当がもう1回増えること（今は水の透過キャプチャで2回→3回に）。受ける面の数はほぼ影響しない。ただし影の縁を何回も読んでぼかす場合は、画面を広く占める海面が一番重くなる。
   - 手間: ステージ(小) < オブジェクト同士(中、通常/スキニングのVS2本、シャドウアクネの調整) < 海面(大、MV1でないためシャドウマップ・行列が自動で渡らない可能性が高い、要確認)。
   - ステージはZ方向に長い(1200〜27000)ので、影を描く範囲はプレイヤー周辺に限定する必要がある。
   - 進める順の案: ステージかオブジェクト同士で動く形を作る → 海面は最後。

11. **EffectManagerの新設（2026-10-01・方針のみ相談、未着手）。** 現状はEffekseerのAPI(`PlayEffekseer3DEffect`/`SetPos...`/`Stop...`等)を17ファイル・83箇所で直接呼び、再生ハンドルも各クラスが保持して自前で`Stop`している。`Sync3DSetting`/`UpdateEffekseer3D`/`DrawEffekseer3D`も各シーンが直接呼んでいる。ロード側(`ResourceLoader::EffectID`)は一元化済みなのでそのまま使う。
   - 所有: `SoundManager`/`BulletManager`と同じく、シーンが`shared_ptr<EffectManager>`を持ち、各オブジェクト・ステートには`weak_ptr`で渡す。
   - 機能案: `Update()`/`Draw()`(Sync3DSetting込み)、`Play(EffectID, pos)`→ハンドル(Play+SetPosの2行セットを1つに)、`PlayOneShot(EffectID, pos)`(被弾・死亡・召喚など撃ちっぱなし用、ハンドル不要)、`SetPos`/`SetRotation`/`SetScale`/`SetColor`/`IsPlaying`/`Stop`、`StopAll()`(シーン遷移・リトライ時の残留対策)。
   - 検討点: 再生ハンドルを`int`のまま返すか、RAIIのハンドル型(中身は`int`1つ、破棄時に自動`Stop`、ムーブのみ)にするか。Claudeの推奨はRAII(止め忘れが原理的になくなり、デストラクタの`Stop`群が消える)。
   - 移行順の案: ①シーンのUpdate/Drawを置き換え → ②撃ちっぱなし系を`PlayOneShot`へ → ③追従系(弾・ブースト・チャージ・ビーム・Player.cppの水しぶき)を移行。

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
- **`DamageSource`構造体**: 新規ファイル`Game/GameObjects/Actors/Character/DamageSource.h`をユーザーが作成（ソリューションエクスプローラー経由）。中身は`enum class DamageSourceType { Rock, Worm, Beam }`と、`type`+`id`を持つ`DamageSource`構造体、`std::set`で使うための`operator<`（まず`type`で比較し、同じ`type`なら`id`で比較する2段階比較）。**この`operator<`の実装意図をユーザーに解説済み、DamageSource.hへの実装はこれから。**
- **API配置場所**: 多段ヒット防止のAPI（`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`想定）は、`Player`単体ではなく`Character`基底クラスに置く方針（将来敵側の多段ヒット防止にも使い回せるように、という判断）。

**次回やること:**
1. `DamageSource.h`の中身を実装する（enum + struct + operator<）。
2. `Character`に`std::set<DamageSource> m_takingDamageSources`と`IsTakingDamageFrom`/`StartTakingDamage`/`OnLeaveDamaging`を追加する。
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
2. 多段ヒット防止の実装（`DamageSource.h`〜、`Character`基底へのAPI追加、`CollisionManager`への組み込み）は引き続き未着手。
3. `Brake`もBoostと同じ「Exit()での音の停止」が必要か確認する（未確認のまま）。
4. リプレイ等でEffekseerエフェクトが残留する問題（旧タスク8）はまだ未着手のまま。

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

**次回やること:**
1. `TargetManager.cpp`の未使用`vector`宣言の削除（軽微、まだ未対応）。
2. `Brake`もBoostと同じ「Exit()での音の停止」が必要か確認する（未確認のまま）。
3. リプレイ等でEffekseerエフェクトが残留する問題（旧タスク8）はまだ未着手のまま。
4. 効果音・BGM実装は一区切り。全体を通しでプレイして音量バランス・タイミングを最終確認するとよい。

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

**→ 2026-10-02、操作説明UIは不要になり全削除（ユーザー依頼でClaudeが削除）:** `GraphicID::HowToPlay`、`how_to_play_path`、`KeepGraph`の読み込み、`GameScene.h`の`m_howToControllOpenProgress`、`GameScene.cpp`の`how_to_*`定数、画像`Data/Image/Button/HowTo.png`・`Data/Image/SelectFrame/How_To_Controll(_OnCursor).png`（ごみ箱へ）。以下の記述は経緯として残す。

**操作説明UI（LBボタン長押しで左下からスライドイン、離すとスライドアウト）に着手・実装場所の認識違いあり、次回続き。**
- 当初`PauseScene`側で実装しようとしていたが、正しくは**`GameScene`側（ポーズを開かなくてもゲームプレイ中に使える機能）**という認識のズレが発覚。`GameScene.h`には既に`m_howToControllOpenProgress`(0〜1の進行度)が用意されており、`GameScene.cpp`の`Update()`にも`InputEvent::how_to`(LBボタン、`InputManager`に新規登録済み)の押下判定・進行度の増減処理が実装されていた。
- **発覚した未修正のバグ**: `m_howToControllOpenProgress++`/`--`が**1フレームあたり1.0**の増減になっており、0〜1のクランプと合わさって「ボタンを押した瞬間に一気に全開、離した瞬間に一気に全閉」という動作になってしまう（「にょきっとスライドする」演出になっていない）。`1.0f / 何らかのフレーム数`という緩やかな係数（他の`wipeProgress`と同じ考え方）に直す必要がある。**次回作業再開時、この修正がまだ反映されていないか確認すること。**

**次回やること:**
1. `m_howToControllOpenProgress`の増減を`++`/`--`ではなく緩やかな係数（例: `1.0f / 15.0f`等）に修正する。
2. `m_howToControllOpenProgress`(0〜1)を使って、実際に操作説明画像を左下からスライドイン/アウトさせる`Draw()`側の実装（画面外の位置から目標位置までLerpで座標を計算する）はまだ未着手。
3. 操作説明用の画像自体（`GraphicID::HowToControll`/`HowToControllOnCursor`は登録済みだが、内容がスライド演出向けかどうかは未確認）の内容確認。

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

**次回やること:**
1. Visual Studioでビルドし、全パターン（弾×浮遊敵/ワーム、弾×ボスのダメージ・無敵判定、敵弾・ビーム・ワーム接触・岩×プレイヤー、多段ヒット防止）を実機確認。
2. 確認後、ユーザーと一緒に`CounterCollider`（ローリング中に敵弾を跳ね返す）を統一ループに組み込む。
3. ビルド確認は現状Visual Studio上で行うこと。コマンドラインMSBuildはDxLib/EffekseerForDXLibのインクルードパスがVS IDE設定と噛み合わず`C1083`で失敗するため、この方法での確認は避ける。

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

**固まった実装方針（次回、学校でここから着手）:**
1. `BossBeamState.h`: `m_isReflectedL/R`(bool)、`m_tipSphereL/R`(先端専用の当たり判定球、毎フレーム`m_beamPosL/R`に追従)、`ReflectLeft(const Vector3& hitPos)`/`ReflectRight(...)`(反射開始、内部で反射方向計算)、`GetTipSphereL/R()`、`IsReflectedL/R()`を追加する設計まで合意。**まだヘッダ・実装ともに未着手（コード未変更）。**
2. `BossBeamState.cpp`の`Update()`: 先端球を毎フレーム更新する処理を追加。反射後は「ボスへの方向に上限角度以内で向きを寄せていく」分岐を追加。反射中の球がボスに届いたら削除+大ダメージ。
3. `BossBeamCollider`: L/R毎にタグを分け、反射中かどうかで返すタグを切り替える。
4. `CollisionManager`: `hit_pairs`に先端球用のペア(`{BossBeamTip, Counter}`)と反射後命中用のペア(`{BossBeamReflected, BossDamage}`)を追加。`OnHit`に反射トリガー処理(反射方向計算→`BossBeamState::ReflectLeft/Right`呼び出し)を追加。

**次回やること:** 上記1〜4を順にコードとして実装していく（ユーザーが自分で書く、Claudeはレビュー役）。

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

**次回やること:**
1. Debug構成・Release構成それぞれで実際にゲームを起動し、タイトル/クリア/ゲームオーバー/ポーズ画面のUIサイズが一致しているか目視確認する。
2. もし他の画面(ゲームプレイ中のUI等)でも同様のサイズ差に気づいた場合、`GaugeUIBase`以外にまだ見つかっていない`scale`定数の使用箇所がないか再度洗い出す。
4. 左回転(`m_pushLeftRollFrame`)側は今回まだ手をつけていない。右回転のロジックが固まってから同じパターンで実装する。

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
- **ゲーム内での動作確認（実際にポーズ画面を開いてトグルする操作）は未実施。次回起動時に確認予定。**

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
- **ゲーム内での動作確認は未実施。次回起動時にポーズ→最初からやり直す、および元のPauseボタンが正常にポーズを開くことを確認予定。**
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
- **ゲーム内での動作確認（実際にポーズ→プレイヤーを移動→物差し操作→ワープ）は未実施。次回起動時に確認予定。** 特に物差しの見た目の座標比率(`warp_ruler_ratio_left/right/y`)や、左スティックの移動速度(`warp_cursor_move_speed`)は実機で触ってみて微調整が要る可能性がある。

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
- **(済・2026-10-05)** 下の「次回」のうち、エフェクト制作・書き出し・登録まで完了。詳細は「進捗（2026-10-05・ChargeExplosion制作）」。
- **次回（新しいセッションで）**: スキル`effekseer-effect-production`の手順(MCPでエディタ操作・4方向確認)で`Data/Effect/ChargeExplosion/`を制作→`.efk`書き出し→登録(`ResourceConstants.h`のパスとscale、`ResourceLoader.h`の`EffectID`、`ResourceLoader.cpp`の対応表・ゲームシーンの一覧・個別の読み込み。`PlayerChargeBullet`で検索して同じ場所すべて)→`ChargeBullet.cpp`の`OnHitEnemy()`に`PlayOneShot(ResourceLoader::EffectID::ChargeExplosion, m_pos)`の1行(**ユーザーが編集中なので触る前に必ず確認**、仮のHitEffectがあれば差し替え)→ビルド→ゲーム内でデバッグ表示の球と並べて確認→NOTES.md追記。編集してよいのは登録・再生の箇所だけ。

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

**次にやること(ユーザーが書く。Claudeはレビュー役)**
1. `EnemyStatus.csv`を作る(ワームのHPは`WormEnemy.cpp`の現在値を確認)。
2. `EnemyStatusData`(`CSVData`派生、ヘッダ名引き＋assert)を作る。→形を見せてレビューを受ける。
3. `modelID`で引く取り出し口＋キャッシュを作る。
4. 浮遊敵→ワーム→ボスの順に直書き定数・`5`/`2000`を置き換え、1体ごとにビルド確認。
5. 同じ形で岩の球を`RockShape.csv`へ(`RockDataSetter.cpp`の匿名名前空間の球設定と、旧`Data/CSV/RockData.csv`の整理も含む)。
- プレイヤーの定数(23個)をどうするかは未決定。ProjectNeaR方式なら「移動速度など種類ごとに変えたい値」だけが対象になる。
- 参考: `C:\Users\Admin\Documents\GitHub\ProjectNeaR\source\Project\General\CSV\CharaStatusData.h/.cpp`、`bin\Data\CSV\Stage1\CharaStatusData.csv`。

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
- **BOM**: 最初Excelの「CSV UTF-8」で保存しBOM付きになった→VS Codeで「エンコード付きで保存→UTF-8」で除去。原因の一つはワークスペース設定`files.encoding: utf8bom`。`.vscode/settings.json`に`"[csv]": {"files.encoding": "utf8"}`と`rainbow_csv.virtual_alignment_mode: always`(見た目だけ列をそろえる。ステータスバーの「Align」は実際にスペースを書き込むので押さない)を追加。**読み込み側のBOM除去はユーザー判断で見送り**(ExcelでCSV UTF-8保存すると1列目のヘッダ名が`﻿modelID`になりassertで止まる、と覚えておく)。旧`Data/CSV/RockData.csv`だけBOM付き(旧形式、整理予定)。
- **ユーザーが実装・動作確認済み**: `GetWStringList`がヘッダ行も先頭に入れて返す / `CSVData`に`m_header`と`SetHeader` / `LoadCSV`が空チェック→`[0]`をヘッダ→`i=1`から`SetData`+`SetHeader`。既存の岩・敵・ボスは今までどおり出る。
- **次**: `CSVData`に「ヘッダ名から何列目かを探す関数」(無い名前はassert)→`EnemyStatusData`。
- **(済・ユーザー実装)** `CSVData::GetColumnIndex(name) const`(protected、無ければassert→-1)、`GetHeader()`、デストラクタをvirtualに。`CSVData/EnemyStatusData`(`CSVData`継承、`shared_ptr<CSVData>`を受けるコンストラクタでデータ+ヘッダを写して`Conversion()`、modelIDは`ResourceLoader::ModelID`で保持＝.hで`ResourceLoader.h`をincludeするのはユーザー判断で許容)。
- **キャッシュの置き場所(ユーザー決定)**: 新クラス(`GameScene`が持つ`EnemyStatusTable`)は「敵のステータスごときにクラスを作りたくない」で不採用。`EnemyStatusData`の`static const EnemyStatusData& FindByModelID(ModelID)`の中の`static const auto table = CreateStatusTable()`(private static)で初回だけ読む。**CSVを変えたらゲーム再起動が必要**。
- **(済・動作確認済み)** 浮遊敵のHP: `FloatingEnemyDataSetter`と`EnemyFactory`(ボスの召喚)の`5`を`FindByModelID(...).GetHp()`に置き換え。
- **次**: 浮遊敵の`col_radius`(132)/`true_dead_frame`(10)(`FloatingEnemy.cpp:16,18`)→ワーム→ボス。
- **(済・ビルドOK)設計変更**: `FindByModelID`は`EnemyBase`のコンストラクタで呼ぶ(ユーザー提案)。HPは`Character`に`FindByModelID(modelID).GetHp()`を渡し、`EnemyBase`の`maxHealth`引数は削除。`EnemyBase`に`protected`の`m_colRadius`/`m_trueDeadFrame`(コンストラクタ本体で代入。初期化子リストは長くなるのでユーザーは使わない方針)。→**`EnemyBase`継承の敵は全員`EnemyStatus.csv`に行が必要(無いとassert)**。浮遊敵の`health`引数・定数2つ、`FloatingEnemyDataSetter`/`EnemyFactory`の`hp`、`BossEnemy`の`EnemyBase`への`data.health`を削除済み。`BossData::health`と`BossEnemyDataSetter`の`2000`は未使用のまま残っている(ボスの番で片付け)。
- 残り: ワーム(`WormEnemy.cpp`の`sphere_radius`40・`death_effect_interval`13)→ボス(`true_dead_frame`、health整理)→`RockShape.csv`。
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
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

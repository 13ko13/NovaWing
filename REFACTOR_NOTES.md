# リファクタ候補メモ（設計・責務分離・くさい箇所）

2026-10-03 作成。セッション中に見つけたものを随時追記していく。
**コードは触らず、見つけた事実と改善の方向だけを書く**（直すのはユーザー本人）。

優先度: ★★★ バグ・クラッシュの可能性 / ★★ 保守性への影響が大きい / ★ 気になる程度

---

## A. バグ・クラッシュの可能性（先に確認したい）

### A-1 ★★★ 状態切り替えを毎フレーム呼んでいる（GameScene::Update）
- `Scene/GameScene.cpp:296` `ChangeAllStateToDisabled()` が、ボス出現前の `!m_isApearBoss` の間**毎フレーム**呼ばれる。`:413` のボス死亡待機中も毎フレーム。
- `ChangeAllStateToDisabled` は3つのステートを `make_shared` し、`Exit()`→`Enter()` を毎回呼ぶ。毎フレーム割り当て＋Enter/Exitが走る。
- 今は `DisabledXxxState::Enter/Exit` が軽いので動いているだけ。Enterに音やエフェクトを入れた瞬間に壊れる。
- 改善: 「一度だけ呼ぶ」ことをGameScene側のフラグではなく、Player側で冪等にする（すでにDisabledなら何もしない）か、演出ステートのEnterで1回だけ呼ぶ。

### A-2 ★★★ 現在のステートを `dynamic_pointer_cast` して null チェックなしで使っている
- `BossEnemy.cpp:358,364` `GetBeamSphereL/R` は `std::dynamic_pointer_cast<BossBeamState>(m_pState)->...`。ビームステートでないときに呼ばれると nullptr 参照でクラッシュ。
- `BossBeamCollider.cpp:37` も `GetBeamDamage()` を null チェックなしで呼ぶ（直前の `IsCollisionActive` が true のときだけ呼ばれる前提に依存）。
- 呼び出し側の前提（「ビーム中にしか呼ばれない」）がコードに現れていない。関連は B-3。

### A-3 ★★ GameScene が「ボス戦の進行」と「プレイヤー到達位置」のネストを取り違えている
- `GameScene.cpp:290` の `if (player.z > boss_appear_z)` ブロックの**中**に、`m_isChangedToBossBGM` と `IsDying` 時のBGMフェードアウトが入っている。条件が「ボスの状態」なのに「プレイヤーz」の内側にある。今は結果的に動くが意図が読めない。

### A-4 ★★ 毎フレーム `ChangeScene` を呼ぶ構造
- `GameScene.cpp:395,425`: `IsDead()` の間、フェード中も毎フレーム `std::make_shared<ClearScene>` / `GameoverScene` を作って `m_nextScene` を上書きしている（Fade は2回目以降を無視するので動作はするが、毎フレーム無駄に生成）。
- 「遷移要求済み」フラグを持つか、SceneController側で `m_nextScene` が既にあれば無視する。

### A-5 ★★ シェーダ・定数バッファ・MakeScreen を解放していない
- `DeleteShader` / `DeleteShaderConstantBuffer` がリポジトリ全体で0件。`LoadPixelShader`/`CreateShaderConstantBuffer` は Player・Stage・Water・Lighting・各Scene・各UIで毎回作成。
- `DeleteGraph` があるのは ClearScene と WarningUI のみ（それ以外の `MakeScreen` 相当は WaterRevealManager）。
- リトライ・タイトルとゲームの往復のたびにGPUリソースが増える。リソースのシーン別ロード（進行中）と同時に直したい。
- 改善: ハンドルを持つRAIIラッパ（`ShaderHandle`/`CBuffer`）を作るか、ロードを1か所にまとめてキャッシュする（B-2と一緒に）。

---

## B. 責務分離できていない箇所

### B-1 ★★★ GameScene が「全部」をやっている（God Class）
`Scene/GameScene.cpp`（528行・include45本）が担っていること:
1. 全オブジェクトの生成と配線（Init 約150行。Player→Target→Camera→Boss→Collision→Factory→敵→UI→水→空→岩→ステージ）
2. 敵の登録（`FloatingEnemy` と `WormEnemy` のforeachが完全に同形。Collision / Target / m_pEnemies / GameObjectManager の4か所に登録）
3. **ボス登場演出のステートマシン**（None→Start→Warning→Movie→CameraZoom、BGM切替、動画描画、カメラ、プレイヤー無効化、ボス出現許可）
4. ボス死亡演出（ズーム・BGMフェード）、クリア/ゲームオーバー判定、ポーズ遷移
5. 描画順（キャプチャパス→空→グリッド→全オブジェクト→水→プレイヤー再描画→Effekseer→UI→ムービー）

改善の方向:
- **`BossAppearDirector`（仮）** を切り出す。状態enum、`m_isApearBoss`、`m_isChangedToBossBGM`、`m_isBossDeathBGMFadeOut`、`m_isDrawBossMovie`、ムービー関連定数をまとめて持たせる。GameScene は `director.Update()` と `director.DrawMovie()` だけ呼ぶ。
- 敵の登録は `RegisterEnemy(pEnemy)` の1関数にまとめる。EnemyFactory の `Create` が既に同じ登録をしているので、**初期配置の敵も Factory 経由にして登録処理を一本化**できる。
- ついでに `BossApearState` → `BossAppearState`（Apear は typo）。

### B-2 ★★★ 「グリッチ演出（シェーダ+定数バッファ）」が6か所にコピペ
- TitleScene / PauseScene / ClearScene / GameoverScene / GaugeUIBase / WarningUI すべてが
  `LoadPixelShader(L"GlitchPS.pso")` → `CreateShaderConstantBuffer(sizeof(GlitchBuffer))` → `scanlineFrequency` セット → `time` を毎フレーム更新 → Draw で Set/解除、を各自で実装。
- `scanline_frequency` / `time_speed` の定数も各ファイルで再定義（Clearでは `280.0f` と `0.1f`）。
- 改善: `GlitchEffect` クラス（`Begin()`/`End()`/`Update()`）にして共有。解放（A-5）もここ1か所で済む。

### B-3 ★★★ コライダーが「ボスの現在ステートの具象型」に依存している
- `BossBeamTipCollider.cpp` に `dynamic_pointer_cast<BossBeamState>(m_owner.GetCurrentState())` が5回、`BossBeamCollider` に2回。
- コライダー → BossEnemy → 現在ステート → 具象ステートの内部（球リスト、反射フラグ、先端球）と、辿るチェーンが長く、ステートの実装変更がコライダーに波及する。
- 改善案: ビームの状態（先端位置、判定球、反射済みか）を `BossBeam`（またはビーム用の小さなクラス）に持たせ、BossEnemy が常に保持。ステートはそれを操作するだけ。コライダーは「ビームがアクティブか」だけ聞けばよくなり、A-2 も同時に解決する。

### B-4 ★★ BossBeamState の左右が全部コピペ
- `m_beamPosL/R`、`m_beamMoveDirL/R`、`m_prevBeamPosL/R`、`m_isReflectedL/R`、`m_isHitBossL/R`、`m_hitBossFrameL/R`、`m_beamSpheresL/R`、`m_leftBeamEffectPlayH/m_rightBeamEffectPlayH`…
- `Enter` / `Update`（追尾・反射後の旋回・先端移動・球生成・球削除・エフェクト更新）/ `OnReflectLeft/Right` / `GetTipSphereL/R` / `IsReflectedL/R` がL/Rで一字違いの重複。`Update` のL/R追尾部分は約30行ずつ同じ。
- 改善: `struct BeamSide { pos, prevPos, dir, effectH, spheres, tipSphere, isReflected, isHitBoss, hitFrame; }` を `BeamSide m_sides[2]` にして、`UpdateSide(BeamSide&)` を1つ書く。行数は約半分になり、片方だけ直し忘れるバグがなくなる。
- 細かい点: `SetBeamEffectDir` のインデントが崩れている。`prevBeamPos` が実際に使われているか要確認（使っていなければ削除）。

### B-5 ★★ Player が「入力・状態管理・VFX・描画シェーダ」を全部持っている
`Player.cpp`（753行）の内訳:
- **入力判定**を Update 内で直接（`Somersault`/`Boost`/`Brake` が `InputManager` を読んでステートを差し替える）。一方でシューティングや回転は State 側が入力を読む。入力を見る場所が2種類ある。
- **水しぶきVFX**（`UpdateWingSplash` 50行、羽ボーン位置の取得とオフセット計算、`sea_splash_*` 定数6個）がプレイヤーの更新に混在。
- **ダメージ演出シェーダ**（`m_cbufferDamage`、`m_damageShaderPSH`、`m_damageTime`、`sinf` 計算）もプレイヤーが所有。
- **描画**: `DrawPlayer` は `Actor::DrawWithLighting` とほぼ同じ処理（テクスチャSet→Apply→Bind→描画→解除）を自前で書き直している。違いは「ダメージ時にPSを差し替える」だけ。
- 改善: ① 水しぶきは `WingSplashEffect` に切り出す ② ダメージ演出は `DamageFlash` に切り出す ③ `DrawWithLighting` に「PS差し替え/追加バッファ」のフックを用意して `DrawPlayer` を削る ④ Somersault/Boost/Brake の入力はどこかのStateへ寄せる。

### B-6 ★★ 4本のステートマシンが同じコードの繰り返し
- `Player::ChangeMovementState/ChangeShootState/ChangeRotationState/ChangeSpecialState` の本体は同じ（Exit→代入→Enter）。型だけ違う。
- 敵（`BossEnemy::Update`、`FloatingEnemy::Update`）にも「Update→GetNextState→Exit→代入→Enter」が丸ごと重複。
- `ChangeAllStateToDisabled/Normal` に `std::static_pointer_cast<Player>(shared_from_this())` が各6回。
- 改善: `template<class TState> class StateMachine { void Change(shared_ptr<TState>); void Update(); }` を1つ作る。

### B-7 ★★ 状態の「種類の問い合わせ」を `dynamic_pointer_cast` で行っている
- `Player::IsSomersault/IsChargeReady/IsRolling` はステートを型でキャストして判定。`Player::Somersault()` 内にも同じキャスト（`IsSomersault()` があるのに使っていない）。
- 改善: Stateに `virtual bool IsSomersault() const { return false; }` のようなタグ、または Player 側が `enum class ShootMode` などを持つ。

### B-8 ★★ CollisionManager が「当たり判定」以外を知りすぎている
- `OnHit` 内で ① カウンター×敵弾の特殊処理 ② プレイヤー被弾の多段ヒット防止（`DamageSource`）③ **カメラ揺れ（ゲームフィール）** をやっている。
- `ReflectEnemyBullet` は `dynamic_cast<EnemyBullet*>` して弾を生成する＝反射ゲームルール本体がここにある。
- `Update` は4種類の弾（Player/Charge/Enemy/Reflected）をそれぞれ別getter・別ループで集めている。
- 毎フレーム `std::map<ColliderTag, vector>` と `keepAlive` vector を作り直している。
- 改善: ① カメラ揺れはプレイヤー被弾イベント（`Player::TakeDamage`側またはコールバック）へ ② 弾マネージャーが `ICollider` のリストを一括で返す ③ 反射は Counter側コライダーの `OnCollision` か専用クラスへ。
- `m_pBoss` は敵リストとは別に必須扱いで null チェックなし（`pBoss->GetColliders()`）。

### B-9 ★★ 敵3種で `DrawEnemy` / 死亡処理 / SoundManager 保持が重複
- `FloatingEnemy::DrawEnemy`、`BossEnemy::DrawEnemy`、（`WormEnemy`も同系）が「テクスチャのペア配列を作って `DrawWithLighting`」を各自で書く。
- `m_pSoundManager`（weak_ptr）を Floating/Worm/Boss が**それぞれ**持つ。`EnemyBase` が `pBulletManager` を持っているのに Sound は持たない。
- `TakeDamage` 内の「死亡フラグ→死亡音→エフェクト再生→位置セット」も3クラスで同じ流れ。
- 改善: `EnemyBase` に `m_pSoundManager` を上げる。`DrawEnemy` はテクスチャ指定をデータ（`struct EnemyTextures`）にして共通化。

### B-10 ★★ シーン間の遷移が「具象クラスの new」で絡み合っている
- Clear→Game/Title、Pause→Title/Game、Game→Clear/Gameover が互いの `.h` を include して `make_shared<XxxScene>` している。循環include気味で、シーンを増やすたびに全部に波及。
- `SceneID` を新設中なので、`controller.RequestChange(SceneID::Title)` のようにIDで要求し、生成は `SceneController`（またはファクトリ）に1か所だけ置くと、リソース差分ロード（実装手順⑥）とも自然につながる。
- フェード時間のマジックナンバー（`60.0f` / `scene_change_frame` / `frame_per_second` / Restart の `0.0f`）が各所に散在。

### B-11 ★★ メニュー選択UIが4シーンに重複
- Title / Pause / Clear / Gameover が、`(index+1) % Max`、`(index-1+Max) % Max`、ワイプ進行度の `±1.0f/wipe_max` 更新ループ、選択音再生を**それぞれ実装**。
- `PauseScene.cpp:221,225` は `1.0f/15.0f` のマジックナンバー、Clear は `wipe_max` 定数（同じ15）。
- 改善: `SelectMenu`（項目数、現在index、前index、ワイプ進行度配列、`Update(input)` が上下移動と進行度更新を担当、選択変更時のコールバックorイベント）を1つ作る。

### B-12 ★ ClearScene の結果表示
- `DrawResultText` は**毎フレーム**2枚のオフスクリーンに全文字を描き直し、`GraphFilter(GAUSS)` を**2回**（コメントも「ぼかしをかける」が2行並ぶだけ）。数字が変化していない（lerp完了後）フレームも毎回再描画。
- 「描画テキストの組み立て」が4項目×2面（通常＋ぼかし）で計8ブロック。`struct ResultLine { text, pos, color }` の配列にしてループ1つで済む。
- `m_resultData.clearTime / 60` と `-m_resultData.hitCount` の変換が Update 中に8回以上出てくる。初期化時に表示用の目標値（`m_targetKill` 等）へ変換して持つ。
- lerp＋丸め（`lerp_guard_threshould`）が4項目で4回コピー → `AnimatedCounter` クラス（現在値/目標値/Update/IsFinished）に。
- `60`（FPS）が `clearTime / 60` と score式に直書き（`frame_per_second` は GameScene の無名namespaceにしかない）。FPS定数は `Constants/Game.h` へ。
- スコア式 `kill*5000 - (秒+被弾)*100` は負になりうる。仕様か確認。

### B-13 ★ SoundManager
- `SoundType`（SoundManager）と `ResourceLoader::SoundID` が**ほぼ1:1の別enum**で、33個の `InitData(type, id, true, volume, loop)` を手書き。`InitData` の引数 `isLoaded` は常に `true`。
- 音量定数が33個、ほとんど150。→ `{SoundType → SoundID, volume, loop}` のテーブル（`ResourceLoader` の `graphic_paths` と同じ形）に置き換える。enum重複も解消できる。
- `Init()` の先頭の `auto& loader` が未使用。`InitData` 内の「タイトルでのプレイヤーのブースト音」コメントは別の場所の使い回しで内容と合っていない。
- 各シーンがSoundManagerをnewして `Init()` で全33音をGetSound→ロード状況未判定（実装手順⑤で対応予定）。
- `PlayFadeIn` の `fadeTimer / fadeDuration` はint除算にならないか（型はfloatか）、`currentVolume` を毎フレーム再計算している点を確認。

---

## C. 設計全般

### C-1 ★★ 「シングルトン」と「ポインタ渡し」が混在（方針が決まっていない）
- `GetInstance()` 呼び出しが52ファイル142箇所。ResourceLoader / InputManager / LightingManager / WaterRevealManager / DebugManager / GameObjectManager / Application はシングルトン。
- 一方 SoundManager / BulletManager / TargetManager / CollisionManager はシーン所有で、`weak_ptr` を Player・各State・各敵・Pause まで**コンストラクタで数珠つなぎ**に渡している（Player のコンストラクタは4引数、Stateは3〜4引数）。
- どちらが良い悪いではなく、「全シーン共通→シングルトン／ゲーム内だけ→注入」といった**ルールを決めて**、例外を作らない。SoundManagerはシーンごとにnewしているのにBGMはシーンをまたぐので、シングルトンにして `StopAll` の責務を明確にする案もある。

### C-2 ★★ オブジェクトの生存管理が3系統ある
- (1) `GameObjectManager`（`Init()` で自動登録、`IsDead` で自動削除）(2) `UIManager`（Register＋Update/Draw）(3) GameScene が `shared_ptr` を保持して手動で Update/Draw（WaterManager, SkyBox, CollisionManager...）。
- さらに「GameObjectManager が持つ」＋「GameScene の `m_pEnemies`/`m_pRocks` も持つ」の二重所有。`m_pEnemies` は保持するだけで他に使われていない（`CollisionManager` は weak_ptr）。
- 2段階初期化（コンストラクタ→`Init()`で `shared_from_this`）が前提で、`Init()` を呼び忘れると更新されない。
- 改善: UpdateとDrawを持つ共通インターフェース（`IUpdatable/IDrawable`）で3系統を寄せる。`m_pEnemies` は削除。

### C-3 ★★ Draw に副作用がある／描画パスで Draw を2〜3回呼ぶ
- `GameScene::Draw` は水の透過表現のため `GameObjectManager::DrawAll()` を**2回**、さらに `Player::Draw()` を**3回目**。各Drawの先頭で `ApplyMatrix` と `UpdateShaderMatrixData`（定数バッファ更新）をやり直している。
- `WaterManager::UpdateShaderMatrixData` は名前が更新だが、中で**メッシュのワープ位置（`m_meshZOffset`）を進める**。しかも Draw から呼ばれる。ワープ判定は `Update` へ。
- 改善: Update側で行列・定数バッファを確定して、Draw は「セットして描くだけ」にする。パスが増えても副作用が出ない。

### C-4 ★★ 描画ステートのSet/解除が手書きで、解除漏れが起きやすい
- `SetUsePixelShader(-1)`、`SetShaderConstantBuffer(-1,...)`、`SetUseTextureToShader(i,-1)` を Actor / Player / Stage / Water / 各Scene に手書き。`WaterManager::Draw` は15行近くが後始末。
- 改善: `ScopedShaderState`（コンストラクタでSet、デストラクタでリセット）のRAIIを1つ作る。Playerのように途中で `return` が入っても漏れなくなる。

### C-5 ★ 海面・ライトなどの「世界の定数」が各所に散らばっている
- 海の高さ: `Player` の `sea_height = 30`、`BossEnemy` の `water_y = 0`、`Game::sea_player_margin`、GameScene の `boss_stand_y = 0`。数値が揃っていない（プレイヤーの海は30でボスは0）。
- ライト方向: `Stage` の `light_dir`（0,-0.5,-1）と `Game::light_direction` と `LightingManager`。Stage のほうは独自バッファを作って持っている。
- ステージレイアウト: `Stage first_pos z=17700`、`boss_appear_z=27000`、`RockDataSetter`、`WaterManager grid_size` が別々。2面目（雨のステージ）に進む前に `StageConfig`（海面高さ・ボス出現位置・ライト・スカイボックス）へまとめると、2面目の作成が楽になる。

### C-6 ★ Stage クラスがほぼ空
- `Stage::Draw` は `MV1DrawModel` がコメントアウトされ、シェーダのSet/解除だけをしている。`Update` も空。それでも毎回 `LoadPixelShader`×2 と 2種の定数バッファを作る。
- 将来の遠景用でなければ削除、遠景に使うなら実際に描く時に作る。

### C-7 ★ ResourceLoader：リソース1個の追加が3ファイルに跨る
- 追加手順: `ResourceLoader.h` の enum → `ResourceConstants.h` のパス定数 → `ResourceLoader.cpp` のテーブル。（エフェクトはさらに scale 定数も）
- `ResourceConstants.h` に `boss_beam_eff_patgh`（typo）、命名揺れ（`_effect_path` と `_eff_path` が混在）。
- 進行中のシーン別ロード（`SceneResources`）が入るなら、「ID・パス・シーン」を同じ表にまとめると、ロード対象の一覧とパス表が二重管理にならない。
- `ResourceLoader.cpp:2-4` に `<cassert>` と include の重複。

### C-8 ★ Application
- `Run()` のフレーム待ちが `while (GetNowHiPerformanceCount() - startTime < 16667) {}` の**ビジーウェイト**（CPU 1コア使い切り）。`SetWaitVSyncFlag` か `Sleep`+微調整に。
- ESC終了の `CheckHitKey(KEY_INPUT_ESCAPE)` がシーンの外のApplicationに直書き（ポーズ・デバッグの扱いと不整合）。
- `Application::GetUIScale`（Debug/Releaseのウィンドウサイズ差を補正する仕組み）と `Game::screen_width` の直接使用（`WaterManager::Init`、`WaterRevealManager`）が混在。サイズの取得元を1つにする。
- `namespace {}` が空のまま残っている。

### C-9 ★ その他の小さなこと
- `Player::OnInit` の `Vector3 axis` が未使用。`Player.cpp` の `gauge_max` と `max_gauge`、`stick_input_max` などが重複／未使用の疑い（grep で確認）。
- `Player::GetForcusTarget` の typo（Focus）。
- `GameObject::GameObject()` の `static int s_nextId` はスレッド安全でなく、コンストラクタ内ローカルstaticで読みづらい（クラスのstaticメンバへ）。
- `BossEnemy::m_isFirstLanding` と着地音は落下演出の廃止で不要（NOTES.mdにも「コードは残している」とある）。そろそろ削除しないと「なぜあるのか」が分からなくなる。
- `BossEnemy::m_effectPlayHandle` に複数本の脚のしぶきを上書き保存（デストラクタで止まるのは最後の1個だけ）。
- `GameScene::DrawGrid` は `#ifdef _DEBUG` の中にしか意味がない関数なのにメンバ関数として常に宣言。デバッグ描画は `DebugDraw` ユーティリティにまとめてもよい。
- `#ifdef _DEBUG` の中にゲームロジック（`killBoss`、`gaugeUp/Down`、`PauseScene` のワープ）が埋まっている。Debug専用コードをデバッグ機能クラスに隔離したい。
- `GameScene.cpp` の include に相対パスの書き方が3種混在（`"../Game/..."`、`"Game/..."`、`"Character/Enemy/..."`）。

---

## 優先して着手する場合の順序（提案）

1. **A-1, A-2**（小さく直せて事故防止）
2. **B-2 グリッチ共通化＋A-5 解放**（コピペ6か所が消え、リソースのシーン別ロードの準備にもなる）
3. **B-1 の `BossAppearDirector` 切り出し**（次の2面目で GameScene をもう一枚作る前にやらないと、2枚目もGod Classになる）
4. **B-4 ビームのL/R統合 → B-3**
5. 残りは機会があれば（B-5〜B-13、C-系）

※スケジュール（アルファ11/24、ベータ12/23提出）を考えると、2面目の作成に影響するもの（B-1, C-5, B-10）を先に、見た目に影響しないもの（B-12, C-9）は後回しでよい。

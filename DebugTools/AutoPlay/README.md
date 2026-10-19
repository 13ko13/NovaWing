# AIテストプレイ(デバッグ専用)

ルールベースのAIにNovaWingを自動で何回も遊ばせて、被弾・死亡・カウンターなどの出来事を記録し、グラフで見るためのツール。

- **Debugビルド専用**。全てのC++ファイルは `#ifdef _DEBUG` で囲んであり、Releaseでは中身が空になる。
- **ゲームの既存コードは1行も変えていない**。このフォルダの中だけで完結している(下の「仕組み」参照)。
- 起動引数に `--autoplay` を付けたときだけ動く。普通に起動したときは何もしない。

## 使い方(VS Codeから)

1. 画面下のステータスバーの **「AIテストプレイ」** ボタンを押す。
2. 出てくるメニューで、回数 → 速さ → 画面を出すか → AIの腕前 を順に選ぶ。
3. 自動でDebugビルドしてから始まる。終わるとブラウザで結果のグラフが開く。

結果だけもう一度見たいときは、ステータスバーの **「テスト結果」** ボタンを押す。

(ボタンは `.vscode/tasks.json` の「NovaWing: AIテストプレイ」「NovaWing: AIテストプレイの結果」。`Ctrl+Shift+P` →「タスクの実行」からも選べる。)

## 使い方(ターミナルから)

1. Debugでビルドする(`x64\Debug\NovaWing.exe` ができていればよい)。
2. リポジトリのフォルダでPowerShellから実行する。

```powershell
.\DebugTools\AutoPlay\Run-AutoPlay.ps1 -Runs 10 -Speed 4
```

3. 終わるとブラウザで `PlaytestReport.html` が開く。

| オプション | 意味 | 既定値 |
|---|---|---|
| `-Runs` | 何回プレイするか | 10 |
| `-Speed` | 速度。`1` `2` `4` `8` …の倍率、または `max`(最大速度) | 4 |
| `-Mode` | `visible`(ゲーム画面を表示) / `hidden`(画面を出さない) | visible |
| `-Skill` | AIの腕前。`beginner` / `normal` / `expert` | normal |
| `-Seed` | 乱数の種。同じ値なら似た流れを再現しやすい | 毎回変わる |
| `-MaxMinutes` | 1回のプレイの上限(ゲーム内の分)。超えたら時間切れとしてやり直す | 15 |
| `-NoReport` | 終わった後にレポートを開かない | - |

例:初心者のAIに画面なし・最大速度で20回遊ばせる

```powershell
.\DebugTools\AutoPlay\Run-AutoPlay.ps1 -Runs 20 -Speed max -Mode hidden -Skill beginner
```

- 画面ありのときは左上に進捗(何回目か、Z座標、HP、AIの行動)が出る。**ESC**で中断。
- 画面なしのときはPowerShellに進捗が出る。**Ctrl+C**で中断。
- 音は「等速・画面あり」のときだけ鳴る。
- 1回終わるごとにログを書き出すので、途中で止めてもそこまでの結果は残る。

## レポート

`PlaytestReport.html` をブラウザで開く(実行後は自動で開く)。最新のテストの結果が表示される。

- 数値:プレイ回数、クリア率、平均被弾数、カウンター成功回数、平均クリア時間
- どこで被弾したか / どこで死亡したか:Z座標1000ごとの回数を原因(敵弾・ワーム・岩・ボスのビーム)別に積み上げ。下の帯は岩と敵の配置
- どこでカウンターを使ったか:ローリングの回数と、カウンター成功の回数
- HPの推移:1回ごとの線と平均
- 1回ごとの結果の表

過去のテストは `Logs\session_*.json` に残っている。レポート画面の「ログ(JSON)を開く…」かドラッグ&ドロップで読み込むと、上のプルダウンで切り替えて比べられる。

Z座標は「通常ステージの開始位置からの距離」(敵や岩のCSVと同じ基準)。ボス戦の間もZは進み続けるので、ボス戦中の出来事は「ボス」の列にまとめている。

## AIの腕前

| 腕前 | 反応の遅れ | 先読み | カウンターする確率 | 狙いのずれ | チャージショット |
|---|---|---|---|---|---|
| beginner | 14F | 26F | 20% | 大きい | 使わない |
| normal | 8F | 40F | 60% | 中くらい | 使う |
| expert | 3F | 60F | 95% | 小さい | 使う |

数値は `AutoPlayBot.cpp` の `GetSkillPresets()` で変えられる。

AIがすること:
- 敵弾・ワーム・岩・ボスのビームの動きを少し先まで予測し、16方向に動いたときのすき間を比べて、一番安全な方向へ逃げる
- 敵を狙って移動し、機体の向きが敵を向いたときだけ撃つ(ボスの盾に当てると回復されるので、むやみに撃たない)
- 前方に敵が2体以上いればチャージショット
- 敵弾やボスのビームの先端が来たら、確率でカウンター(ローリング)

ブースト・ブレーキ・宙返りは使わない。

## 仕組み

| ファイル | 役割 |
|---|---|
| `AutoPlaySystem` | 本体。起動引数を読み、DxLibの関数を差し替え、メニュー操作・チュートリアルを飛ばす操作・プレイの区切りを管理する |
| `AutoPlayWorld` | `GameObjectManager` に登録されているオブジェクトを毎フレーム読み、プレイヤー・敵・弾・岩・ボスのビームをまとめる |
| `AutoPlayBot` | AI本体。状況を見てパッドの入力を決める |
| `PlaytestRecorder` | 出来事を記録し、`Logs` にJSONを書き出す |
| `Run-AutoPlay.ps1` | 起動スクリプト |
| `PlaytestReport.html` | レポート画面 |

既存コードを変えずに動かすために、次の2つを使っている。

1. **関数の差し替え**:Debugビルドは関数呼び出しが「ILT」という中継のジャンプ命令を通る。起動時(WinMainより前)にその飛び先を書き換えて、`GetJoypadXInputState`(パッド入力→AIの入力を返す)、`GetNowHiPerformanceCount`(時間→倍速)、`ScreenFlip`(倍速中は何フレームかに1回だけ表示・進捗を描く)、`PlaySoundMem`(消音)をこのツールの関数に置き換える。`InputManager::Update` が毎フレーム1回パッドを読むので、そこをAIの処理のきっかけにしている。
2. **privateな配列の読み取り**:`GameObjectManager` の `m_pGameObjects` はprivateだが、テンプレートの明示的インスタンス化ではアクセス指定が無視されるというC++の規則を使って読んでいる(読むだけで書き込まない)。

チュートリアルは、ポーズ画面のデバッグ項目「プレイヤーを移動」で物差しの左端(ステージ開始位置)を選んで飛ばしている。

## 注意(ゲーム側を変えたときに直すところ)

- **ポーズ画面の選択肢の順番**を変えたら、`AutoPlaySystem.cpp` の `skip_down_count`(「プレイヤーを移動」まで下を押す回数、今は4)と `restart_down_count`(「最初からやり直す」まで、今は3)を合わせる。
- **プレイヤーの当たり判定の大きさ・移動速度・ローリングのフレーム**などを変えたら、`AutoPlayBot.cpp` と `PlaytestRecorder.cpp` の先頭の同じ名前の定数を合わせる(ゲーム側の値はcppの中にあって読めないため、写してある)。
- 画面左上の表示の最後に `入力:NG` などと出たら、関数の差し替えに失敗している(インクリメンタルリンクを切った場合など)。そのときAIは動かない。
- 倍速でも、ボス登場のムービーは本当の時間で再生される。

## 消し方

どちらかのやり方で消す。ゲームの既存コードは変えていないので、これだけで元どおりになる。

どちらの場合も、`.vscode/tasks.json` から「NovaWing: AIテストプレイ」「NovaWing: AIテストプレイの結果」の2つのタスクと、`autoPlay` で始まる4つの input(`autoPlayRuns` `autoPlaySpeed` `autoPlayMode` `autoPlaySkill`)を消す。

**手で消す**
1. `NovaWing.vcxproj` と `NovaWing.vcxproj.filters` から、`DebugTools\AutoPlay\` を含む行(.cpp 4つ、.h 4つ)を消す。
2. このフォルダ(`DebugTools\AutoPlay`)ごと削除する。`DebugTools` に他に何もなければ `DebugTools` も消してよい。

**クラス削除スクリプトで消す**
`.vscode\scripts\Remove-GameClass.ps1` は他のファイルから `#include` されているクラスを消せないので、この順番で実行する(それぞれ確認が出る)。

```powershell
.\.vscode\scripts\Remove-GameClass.ps1 -Path DebugTools\AutoPlay\AutoPlaySystem
.\.vscode\scripts\Remove-GameClass.ps1 -Path DebugTools\AutoPlay\PlaytestRecorder
.\.vscode\scripts\Remove-GameClass.ps1 -Path DebugTools\AutoPlay\AutoPlayBot
.\.vscode\scripts\Remove-GameClass.ps1 -Path DebugTools\AutoPlay\AutoPlayWorld
```

最後に、残った `DebugTools\AutoPlay` フォルダ(スクリプト・レポート・ログ)を削除する。

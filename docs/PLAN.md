# pico2w-thermometer 計画・引き継ぎメモ

別スレッド（特に WSL 側 Claude Code）から再開するための正本。
方針は `../CLAUDE.md` を参照。ここには「現在地」と「ロードマップ」を書く。

---

## ゴール（段階）

1. **【今ここ】Pico 2 W が内蔵温度センサを読み、USB シリアルで PC に送る**（有線）
2. PC 側で受信プログラムを自作し、保存・表示する
3. Wi-Fi / MQTT 化して、自宅サーバーへ無線送信
4. 自宅サーバー（Docker: Mosquitto → 自作コレクタ → DB → Grafana）で蓄積・可視化

> サーバー機はまだ無い。まずは 1 の「temp → USB 有線」を完成させる。
> 将来サーバー化したとき、**受信コレクタ層を自作する**のが訓練の本丸。

---

## 既存プロジェクトの状態（調査済み）

名前は thermometer だが、中身は **FreeRTOS 土台 + LED トグル** まで。温度計の本体は未着手。
履歴: Lチカ → FreeRTOSラッパー整備 → 静的タスク生成整理 → assert実装 → **ラッパー廃止（生FreeRTOS化）** → **ビルド環境刷新（CMake+Ninja / out-of-source）**。

| 要素 | 状態 |
|---|---|
| FreeRTOS 土台（`src/system/`: `app_assert`, `freertos_hooks` ／ `src/config/`: `FreeRTOSConfig.h`, `app_config.h`） | ✅ 構築済み。**生FreeRTOS API**（旧ITRON流ラッパーは廃止済み） |
| タスク登録（`src/app/app_tasks`: Composition Root。`s_tasks[]` カタログ＋`initTasks()` で `xTaskCreate`） | ✅ あり。`cre_static_tasks` は廃止、現在は動的生成 |
| LED トグルタスク（`src/app/led_toggle`） | ✅ あり（app 層のサンプル） |
| ビルド環境（submodule: pico-sdk, FreeRTOS-Kernel ／ ルートの `CMakeLists.txt` + `build.sh`、Ninja） | ✅ あり（→「完了したバックログ」参照） |
| 開発環境（clangd + `compile_commands.json` で F12 ジャンプ） | ✅ あり |
| 通信モジュール（`src/system/`: `ComModule` 抽象 + `UsbCdcCom`） | ✅ 実装＋実機検証済み（→「完了したバックログ」参照）。温度計の USB 出力土台 |
| **温度計本体（ADC 読み・USB 出力）** | ❌ **未着手 ← 今回の訓練対象** |

**つまり「temp→USB を FreeRTOS タスクで作る」という、まさにやりたい所だけが空き地。**
土台はあるので再利用一択。新規プロジェクトは作らない。

### 設計レビューで突くべき既存の癖
- ~~`CMakeLists.txt` / `build.sh` / import.cmake が `build/` の中にある（非標準）~~
  → **2026-07-16 解消済み**。ルートへ移動し out-of-source 化（→「完了したバックログ」参照）。

---

## ロードマップ（design-first）

- [x] **1. 現状ビルド確認** … ビルド通過確認済み。その後ビルド環境を刷新（→「完了したバックログ」参照）。
- [~] **2. 既存アーキのリバース設計** … 過去の自分のコードを読んで図に落とす訓練。
   - [x] **静的依存（コンポーネント図）** … `docs/components.drawio.png`。モジュール間の依存方向を1枚に。レビュー通過済み。
   - [ ] **動的な振る舞い（起動シーケンス図）** … 未着手。コンポーネント図では描けない時系列を1枚に:
         `main` → `pico_init`（stdio/cyw43）→ `initTasks`（`s_tasks[]` を回して `xTaskCreate`）→ `vTaskStartScheduler` → 各タスク稼働。
         Composition Root（`app_tasks`）が「どの順で何を生成し、いつスケジューラに渡すか」を表現する。
- [ ] **3. 温度計モジュール** … 開発ループ（壁打ち → 実装 → レビュー → 動作確認 → デバッグ）で進める。
   - **壁打ちで決める**: app 層に新タスクを追加する前提（`src/app/thermometer/` 想定）、
     モジュールの IF（タスク関数・公開 API）、Composition Root（`app_tasks` の `s_tasks[]`）への登録、
     データフロー（ADC → ℃変換 → `Logger` / `UsbCdcCom` 出力）
   - **事前に書く例外**: 将来サーバーへ送る**メッセージフォーマット（契約）**。
     後から変えると受信側と両方壊れるので、ここだけは先に決めて残す
   - **`%f` を使わない**: 温度は固定小数点（整数）で扱う。float 整形は `_dtoa` を引き込みスタック・flash を食う
   - 記録は実装後に「決定と理由」を数行だけ

---

## 技術リファレンス（temp→USB 実装時に使う）

### 内蔵温度センサ（RP2350 / Pico 2 W）
```c
adc_init();
adc_set_temp_sensor_enabled(true);
adc_select_input(ADC_TEMPERATURE_CHANNEL_NUM); // 直打ち(4)せずマクロを使う
uint16_t raw = adc_read();                     // 12bit
float v = raw * 3.3f / (1 << 12);
float tempC = 27.0f - (v - 0.706f) / 0.001721f; // データシートの式
```
- 精度はそこそこ（±数℃級、個体差あり）。まずは「値が取れること」の確認台。
  精度が欲しくなったら外付け BME280 等へ。

### USB シリアル出力
- `printf` がそのまま USB CDC に出る。CMake で `pico_enable_stdio_usb(<target> 1)` / `pico_enable_stdio_uart(<target> 0)`。
- **ホストが接続する前の `printf` は失われる**（焼いた直後に無反応でも、ターミナルで COM を開いた瞬間から見える）。
- USB CDC のためボーレートは事実上無視される。

---

## 完了したバックログ

- **✅ CMake + Ninja によるビルド環境の刷新**（2026-07-16 完了）
  - 動機: 会社で新機種のビルド環境を検討中（2026-07 時点）で、CMake + Ninja 採用の話が出ている。その素振り・比較検証を、この個人プロジェクトで先に回して勘所を掴んだ。
  - 着手順序どおり 1→2→3 で積み上げ、全て完了:
    1. **✅ 配置整理**: `CMakeLists.txt` / import.cmake を**ルートへ移動**（import は `cmake/` へ集約）。`build/` は生成物専用に。`build.sh` を `cmake -S "$ROOT_DIR" -B "$BUILD_DIR"` の out-of-source 化（`cd build` の名残を撤去）。`.gitignore` に `build` を追加。→ ソースツリーへのキャッシュ汚染が解消し、stray cache の掃除処理も不要に。
    2. **✅ 増分ビルド化**: 毎回の `rm -rf` を廃止（`--fullbuild` 指定時のみクリーン）。`make` 直叩きをやめ `cmake --build "$BUILD_DIR"` に。→ 変更ファイルだけ再コンパイル。
    3. **✅ Ninja 導入**: configure に `-G Ninja` を追加。`ninja-build` を apt 導入。generator 切替のため初回のみ `--fullbuild` でクリーン再生成。`cmake --build` は generator 非依存なので build 側は無変更で済んだ。
  - 学び: 作業は数行だが本質は概念（in-source/out-of-source、`-S`/`-B` の役割分離、`cmake --build` による generator 抽象化）。ハマりどころは全て「CWD 依存」と「generator mismatch」だった。
  - 残タスク（app内整理フェーズで再考）: CMakeLists の `file(GLOB)` 維持 or 明示リスト化の判断、include パス平坦化（`src` 配下4ディレクトリを全て PRIVATE include）、空の `tests` include ディレクトリの扱い。

- **✅ 通信モジュール（ComModule / UsbCdcCom）**（2026-07-21 実装＋実機検証完了）
  - 経緯: 「温度計の前に出力機構を整備する」から派生。通信は実務でも効くので"ちゃんと"設計した。設計書 = `docs/ComModules.md` ＋ クラス図 `docs/ComModules.drawio.png`。
  - **層の選択**: TinyUSB CDC（`tud_cdc_*`）を直叩き。stdio は使わない（dispatch/yield を自前制御するため）。**USB スタック init は起動時所有**（`pico_init` → `stdio_init_all`）、`UsbCdcCom::Init()` は `tud_inited()` で検証のみ（`stdio_usb_init` は非冪等・二重呼び出しで panic するため呼ばない）。
  - **抽象（NVI パターン）**: `ComModule` 基底。public 非virtual `Send`/`Recv` が共通処理（引数検証・mutex・タイムアウト・コンテキスト分岐・リトライループ）、protected 純粋仮想 `WriteSome`/`ReadSome` が**ノンブロッキング・ベストエフォート**の IF 固有処理。**ComModule は dumb transport**（フレーミング・ログ整形・終端文字に関与しない）。
  - **契約**: 戻り値 = `ComStatus` enum、実バイト数は out 引数。`timeout=0`→ノンブロッキング、`UINT32_MAX`→無限。`WriteSome` 戻り値: `>=0` バイト数 / `-1` 切断 / `-2` エラー。タイムアウトは pico-sdk タイマ（`make_timeout_time_ms`＋`time_reached`、スケジューラ前でも動く）。
  - **並行性**: インスタンス毎に優先度継承 mutex。スケジューラ起動前は mutex/yield をスキップ（単一コンテキストで安全）。**ISR からの使用は禁止**。
  - **合成（重要な思想）**: インスタンスは **app 層の Composition Root で生成し、DI で注入**（タスクは `pvParameters` 経由、ロガーは init 注入）。**シングルトンにしない・`getComModules()` 的 Service Locator を作らない**。「どこからでもログ」は将来の **Logger ファサード**（ComModule を1個注入で保持）で満たす。1つの IF で複数インスタンスあり得る（UART=uart0/1、CDC=itf番号）＝インスタンスは「エンドポイントへの束縛」。
  - **検証**: `tests/com_smoketest.cpp` のスイープテスト（1〜1024B）で全サイズ ◎。部分送信・FIFO 満杯→ドレイン待ち・flush が実機で動作確認済み。（教訓: `snprintf` は newlib vfprintf 経由でスタック大食い。テストタスクは stack 2048 words 必要だった。本番タスクは `uxTaskGetStackHighWaterMark` で実測して right-size すること。）
  - **残タスク**:
    - **RX/`Recv` はスタブ**（`NotConnected` 返すだけ）。コマンド通信の要件が出るまで YAGNI。
    - **RX 実装時は TX/RX の mutex を分割**する（今は単一 mutex。ブロッキング `Recv` が全 `Send` を止めてしまう）。
    - **Logger 層は未着手**（dbg_print・ログレベル・整形・終端文字ポリシー。ComModule の上に別モジュールとして）。
    - **テスト後片付け**: `com_smoketest` を `#ifdef` で gate（今 `test_init()` が無条件呼び出し）、`new UsbCdcCom()` のリーク、`AppCom_Init` と二重生成の整理。`main.c` の起動デバッグ残骸（`printf`＋`sleep_ms(10000)`）も掃除。

---

## 次スレッドでの再開ポイント

ビルド環境・通信モジュール（ComModule/UsbCdcCom）は完了。**次はロードマップ本題（温度計）へ戻る**:
- **温度計本体の設計**（ロードマップ 3）… ADC → ℃変換 → **UsbCdcCom 経由で USB 出力**。将来のメッセージフォーマット（契約）を先に決める。
- 先に **Logger 層**を薄く作ると温度計デバッグが楽（任意）。
- 通信モジュールの残タスク（上記）は必要になった時に着手。設計書は `docs/ComModules.md`。
- 設計はオーナーが draw.io で描き、Claude はレビュー側に回る。

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
git 最新コミットは `assert実装`。履歴: Lチカ → FreeRTOSラッパー整備 → 静的タスク生成整理 → assert実装。

| 要素 | 状態 |
|---|---|
| FreeRTOS 土台（`src/kernel/`: freertos_wrapper, static_object, hooks, FreeRTOSConfig.h） | ✅ 構築済み。ITRON 流（`ER`/`E_OK`/`cre_static_tasks`） |
| LED トグルタスク（`src/app/led_toggle`） | ✅ あり（app 層のサンプル） |
| ビルド環境（submodule: pico-sdk, FreeRTOS-Kernel / `build/build.sh`） | ✅ あり |
| 設計の記録（`docs/build-errors.md`, `docs/freertos_wrapper.md`） | ✅ あり |
| **温度計本体（ADC 読み・USB 出力）** | ❌ **未着手 ← 今回の訓練対象** |

**つまり「temp→USB を FreeRTOS タスクで作る」という、まさにやりたい所だけが空き地。**
土台はあるので再利用一択。新規プロジェクトは作らない。

### 設計レビューで突くべき既存の癖
- `CMakeLists.txt` / `build.sh` / 各種 import.cmake が **`build/` の中**にある（非標準）。
  `build/` は本来「捨ててよい生成物置き場」なので、設定と生成物（`build/out`）の同居は将来事故りやすい。
  → リバース設計のときに「あるべき配置」を検討する題材にする。

---

## ロードマップ（design-first）

- [ ] **1. 現状ビルド確認** … `./build/build.sh` が通るか。submodule の pico-sdk が RP2350 対応（≥2.0.0）か確認。
      何かを足す前に「動く既知の状態」を確保する。←★オーナーが今ビルド中
- [ ] **2. 既存アーキのリバース設計** … kernel 層 / app 層 / 静的タスク登録の構造を draw.io で 1 枚に。
      `docs/design/architecture.drawio.svg`。過去の自分のコードを読んで図に落とす訓練。
- [ ] **3. 温度計モジュールの設計** … `docs/design/thermometer.drawio.svg`。
   - app 層に新タスクを追加する前提（`src/app/thermometer/` 想定）
   - モジュールのインターフェース（タスク関数・公開 API）
   - 既存 `cre_static_tasks` への登録方法
   - データフロー: ADC → ℃変換 → USB printf
   - 将来サーバーへ送るメッセージフォーマット（契約）を先に決める
- [ ] **4. 設計レビュー** … 上記を Claude にレビューさせる（考慮漏れ・境界値・エラー時挙動・タイミング）
- [ ] **5. 手で実装** … オーナーが実装。既存タスク登録に差し込む
- [ ] **6. 突き合わせ** … 設計と実装のズレ（と理由）を `docs/design/` に一行メモ

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

## バックログ（着手前・リポジトリ整理の後）

- **CMake + Ninja によるビルド環境の刷新**（リポジトリ整理が完了してから着手）
  - 動機: 会社で新機種のビルド環境を検討中（2026-07 時点）で、CMake + Ninja 採用の話が出ている。その素振り・比較検証を、この個人プロジェクトで先に回して勘所を掴みたい。
  - 前提: 現状 `build/` 配下に CMakeLists / build.sh / 各種 import.cmake が同居する非標準構成（→「既存の癖」参照）。この配置整理とセットで検討する。
  - 現状の実態（要改善）: `build.sh` は `cmake ..`（デフォルト = Unix Makefiles）＋ `make -j`。しかも毎回 `out/` を `rm -rf` する**フルクリーンビルド**で、増分ビルドを一切使っていない。build/ 配下で cmake を走らせるためキャッシュ（CMakeCache.txt / CMakeFiles）が汚染され、その全消し対処が毎回クリーンの一因になっている。
  - **着手順序（重要）**: 道具より使い方が先。この順で積み上げる。
    1. **配置整理**: out-of-source を徹底し、build/ にキャッシュを作らせない（設定と生成物を分離）
    2. **増分ビルド化**: 毎回の `rm -rf out` をやめる。これが速度に一番効く（make のままでも劇的に速くなる）
    3. **Ninja 導入**: `cmake -G Ninja` に切り替え（コードは触らない）。増分がさらに速く＋会社の CMake+Ninja 素振り＋ clangd 用 `compile_commands.json` 連携
  - 注意: Ninja 単体を先に入れても、毎回クリーンのままでは増分メリットが死ぬ（＝速くならない）。1→2 を済ませてから 3。

---

## 次スレッドでの再開ポイント

オーナーがビルド確認中。再開時はまず **「1 のビルドが通ったか」を聞く** → 通っていれば
**2 のリバース設計図**から。設計図はオーナーが draw.io で描き、Claude はレビュー側に回る。

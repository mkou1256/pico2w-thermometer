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
- ~~`CMakeLists.txt` / `build.sh` / import.cmake が `build/` の中にある（非標準）~~
  → **2026-07-16 解消済み**。ルートへ移動し out-of-source 化（→「完了したバックログ」参照）。

---

## ロードマップ（design-first）

- [x] **1. 現状ビルド確認** … ビルド通過確認済み。その後ビルド環境を刷新（→「完了したバックログ」参照）。
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

## 完了したバックログ

- **✅ CMake + Ninja によるビルド環境の刷新**（2026-07-16 完了）
  - 動機: 会社で新機種のビルド環境を検討中（2026-07 時点）で、CMake + Ninja 採用の話が出ている。その素振り・比較検証を、この個人プロジェクトで先に回して勘所を掴んだ。
  - 着手順序どおり 1→2→3 で積み上げ、全て完了:
    1. **✅ 配置整理**: `CMakeLists.txt` / import.cmake を**ルートへ移動**（import は `cmake/` へ集約）。`build/` は生成物専用に。`build.sh` を `cmake -S "$ROOT_DIR" -B "$BUILD_DIR"` の out-of-source 化（`cd build` の名残を撤去）。`.gitignore` に `build` を追加。→ ソースツリーへのキャッシュ汚染が解消し、stray cache の掃除処理も不要に。
    2. **✅ 増分ビルド化**: 毎回の `rm -rf` を廃止（`--fullbuild` 指定時のみクリーン）。`make` 直叩きをやめ `cmake --build "$BUILD_DIR"` に。→ 変更ファイルだけ再コンパイル。
    3. **✅ Ninja 導入**: configure に `-G Ninja` を追加。`ninja-build` を apt 導入。generator 切替のため初回のみ `--fullbuild` でクリーン再生成。`cmake --build` は generator 非依存なので build 側は無変更で済んだ。
  - 学び: 作業は数行だが本質は概念（in-source/out-of-source、`-S`/`-B` の役割分離、`cmake --build` による generator 抽象化）。ハマりどころは全て「CWD 依存」と「generator mismatch」だった。
  - 残タスク（app内整理フェーズで再考）: CMakeLists の `file(GLOB)` 維持 or 明示リスト化の判断、include パス平坦化（`src` 配下4ディレクトリを全て PRIVATE include）、空の `tests` include ディレクトリの扱い。

---

## 次スレッドでの再開ポイント

オーナーがビルド確認中。再開時はまず **「1 のビルドが通ったか」を聞く** → 通っていれば
**2 のリバース設計図**から。設計図はオーナーが draw.io で描き、Claude はレビュー側に回る。

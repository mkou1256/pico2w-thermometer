# pico2w-thermometer

Raspberry Pi Pico 2 W で温度を測り、最終的に自宅サーバーへ送る IoT プロジェクト。
趣味であると同時に、コーディング力・設計力の自主トレを兼ねる。

## このプロジェクトの目的（重要）

オーナーは組み込みエンジニア。会社では普段ほぼ Claude Code にコードを書かせているため、
自力の設計・実装力が鈍らないよう、家で鍛えるのが裏テーマ。
よって「何を作るか」より「どう作るか（進め方）」に価値がある。

## 進め方の方針（厳守）

- **回答は日本語**
- **design-first**: 実装に入る前に、必ず `docs/design/` に設計図を描く（draw.io / `.drawio.svg`）
- **実装はオーナーが手で書く**。Claude はコードを勝手に書かない。
  役割は「設計レビュー・考慮漏れの指摘・壁打ち・エラー解析」に徹する
- **設計 → 実装 → 突き合わせ** のループを回す。
  実装後、設計と実装のズレ（と理由）を一行メモとして残す
- 図ツールは **draw.io**（VS Code の Draw.io Integration 拡張 `hediet.vscode-drawio` で編集）。
  Mermaid は表現力不足のため不採用

## 開発環境メモ

- ビルドは **WSL ネイティブ FS 内**（`~/pico2w/pico2w-thermometer`）で行う（/mnt/c は遅いので避ける）
- 書き込みは BOOTSEL → Windows 側にマウントされるドライブへ `.uf2` をドラッグ&ドロップ
- ボード指定は `PICO_BOARD=pico2_w`（RP2350 ターゲット）
- シリアル受信は Windows 側のターミナル、または usbipd-win で WSL に USB をアタッチ

## 現在地と全体計画

→ `docs/PLAN.md` を参照（これが正本）

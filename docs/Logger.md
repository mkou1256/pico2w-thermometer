# ログ設計

## 機能概要

- 開発中に用いるログ出力
- ログレベルは、`DEBUG`, `INFO`, `WARN`, `ERROR`の4種類。`DEBUG`レベルは、releaseビルド時に無効化される
- 出力インターフェースは、`ComModule::Send()`であり、設定で出力先を切り替えられる。
- 割り込みコンテキストでの使用は禁止。

## 出力形式

```
[tick][LEVEL][LINE]<msg> \r\n
```

の形式で出力する。

- tick値は、pico-sdkの`time_us_64()`を使用する。

> [!note]
>
> - [TBD ]色も変えたいかも。DEBUG, INFO: 白, WARN: 黄色, ERROR: 赤 みたいな

## Loggerクラス

![](./Logger.drawio.png)

- `Logger`クラスはstaticなクラスで、グローバルアクセス可能なリソースとする。
- 初期化処理は、src/app/app_com.cpp の`App_comInit()`で`init(&comModule)`を行う。ここで、IFの登録と諸々の初期化を行う。
- 初期化前のlog呼び出しは空で抜ける。
- `ComModule`インスタンスを集約しメンバに持ち、出力IFとする。`ComModule::Send()`はtimeout=0で使用し、ノンブロッキングとする。戻り値による再送信も行わない。
- `Logger::log()`で、渡された文字列を`buffer`に書き込んでから`ComModule::Send()`する。その際、`mutex`を取り行ごとに排他する

## ラッパーマクロ

```C++
#define LOG_AT(lvl, ...) \
	do {if (lvl >= log_level) Logger::log(lvl, __LINE__, __VA_ARGS__)} while(0)
#define LOG_INFO(...) LOG_AT(LogLvl::Info, __VA_ARGS__)
// WARN, ERRORもINFOと同じ

// DEBUGはコンパイル時に切り替え
#if __DEBUG__ /* Debugビルド時にCMakeで宣言 */
#define LOG_DEBUG(...) \
	LOG_AT(LogLvl::Debug, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

enum class LogLvl {
    Debug = 0,
    Info,
    Warn,
    Error,
};
```

のようにマクロをLoggerクラスの関数に展開する。

- マクロへの引数は、可変長フォーマットを許容する。

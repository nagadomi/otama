# TIPS

気になりそうなことがあれば書いていきます。

## ログファイルを指定する

デフォルトではエラーログなどが標準エラー出力に出てきます。

Cの場合は、

    otama_log_set_file("./log.log");

Rubyの場合は、

    Otama.set_log_file("./log.log");

とすると、ログ出力をファイルに行うようになります。

## ログレベルを変更する

Cの場合は、

    otama_log_set_level(OTAMA_LOG_LEVEL_DEBUG);

Rubyの場合は、

    Otama.log_level = Otama::LOG_LEVEL_DEBUG

とするとログレベルがDEBUGになります。

ログレベルには以下のものがあります。

    OTAMA_LOG_LEVEL_DEBUG  : デバッグ情報まで表示する
    OTAMA_LOG_LEVEL_NOTICE : なにかお知らせがあれば表示する
    OTAMA_LOG_LEVEL_ERROR  : エラーがあれば表示する
    OTAMA_LOG_LEVEL_QUIET  : 何も表示しない

また環境変数`OTAMA_LOG_LEVEL`に

    export OTAMA_LOG_LEVEL=debug

などと設定すると一時的に変更することができます。

プロセス内でグローバルに持っているので、インスタンスごとには指定できません。

## 使うCPU数を制限する

環境変数`OMP_NUM_THREADS`が設定されている場合、otama並列数は`OMP_NUM_THREADS`になります。

    export OMP_NUM_THREADS=2

とすると2スレッドになるので2コアしか使われません。使うCPU数を制限するとその分遅くなります。

またLinuxの場合はcgroupの仕組みを使うと動的に使うCPUを変えることができます。

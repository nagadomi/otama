# Otama Web API

OtamaのWebAPI。
とりあえず作ってみた、程度のものです。

    bundle install
    thin start -p 4568 -P ./thin.pid -d
    ruby test.rb
    kill `cat thin.pid`

`otama_server.rb`がサーバーで、`otama_client.rb`がクライアントのクラス例です。

pullはこまめに行わないと時間かかるようになってタイムアウトするので
cronから定期的にpullしたほうがいいかもしれません。

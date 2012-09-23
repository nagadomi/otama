# example-webapp

適当なウェブアプリケーションのサンプルです。
ウェブ上から検索と登録ができます。

（sinatraよくわかってないし、例にしては汚いので、もっとシンプルなものに書きなおしたい）

## 必要なライブラリをインストール

必要なパッケージをインストールします。

    $ sudo apt-get install libgdbm-dev libmagickcore-dev libmagickwand-dev libmagic-dev libffi-dev

Rubyの依存ライブラリをインストールします。

    $ sudo gem install bundler
    $ sudo bundle install

## 設定を編集

`config.yaml`にotamaの設定があるので使用するドライバを設定するなどします。

## 画像をインポートする

あらかじめ用意した画像ファイルをデータベースにインポートします。

    find your-image-dir/ \( -name \*.jpg -o -name \*.png -o -name \*.jpeg \) -print > filelist.txt

などと画像ファイルの一覧を作って、

    nohup ./import.sh &

でインポートされます。
(終わるまで待つ)

    cat ./import.sh
    #!/bin/sh
    renice 19 -p $$
    otama_create_table -c ./config.yaml
    otama_import -c ./config.yaml ./filelist.txt > id.txt
    otama_pull -c ./config.yaml
    ruby make-database.rb

`otama_create_table`コマンドでテーブルを作って、
`otama_import`コマンドで画像を（マスタへ）インポートして、
`otama_pull`コマンドでローカルの検索インデックスを更新します。

`makr-database.rb`は`otama_import`の出力(id.txt)から画像ファイル名と`otama_id`の対応表(example.gdbm)を構築しています。

## ウェブサービス起動

    ruby example-webapp.rb -p 3000

ブラウザで http://localhost:3000/ にアクセスして表示されていれば成功です。
画像をクリックするとその画像をクエリに検索します。

    thin start --socket /dev/shm/example.socket

などでthinとUNIX Socketでも動かせます。

otamaはOpenMPを使っていてひとつのプロセスですべてのCPUを使おうとするのでシングルプロセスで動かしたほうがパフォーマンスが良いと思います。
マルチプロセスで動かす場合は、環境変数`OMP_NUM_THREADS`を設定してひとつのプロセスが使うCPUの数を制限しないと
複数のプロセスが同時に動いた時にプロセッサのオーバーサブスクリプションが発生してパフォーマンスが悪くなります。

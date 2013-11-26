# example-webapp

適当なウェブアプリケーションのサンプルです。
ウェブ上から検索と登録ができます。

## 必要なライブラリをインストール

必要なパッケージをインストールします。

(Ubuntu)

    $ sudo apt-get install libmagickcore-dev libmagickwand-dev libmagic-dev libffi-dev

(FreeBSD)

    $ sudo pkg_add -r ImageMagick libffi

Rubyの依存ライブラリをインストールします。

    $ sudo gem install bundler
    $ sudo bundle install

## 設定を編集

`config.yaml`にotamaの設定があるので使用するドライバを設定します。

`example-webapp.yaml`にアプリケーションの設定があるので編集します。

## 画像をインポートする

あらかじめ用意した画像ファイルをデータベースにインポートします。

    find image-dir-path/ \( -name \*.jpg -o -name \*.png -o -name \*.jpeg \) -print > filelist.txt

と画像ファイルの一覧を作って、

    nohup ./import.sh &

でインポートが開始されます。
(終わるまで待つ)

import.shの中身は、

    cat ./import.sh
    #!/bin/sh
    renice 19 -p $$
    otama_create_database -c ./config.yaml
    otama_import -c ./config.yaml ./filelist.txt > id.txt
    otama_pull -c ./config.yaml
    ruby make-database.rb

`otama_create_database`コマンドでテーブルを作って、
`otama_import`コマンドで画像を（マスタへ）インポートして、
`otama_pull`コマンドでローカルの検索インデックスを更新します。

`makr-database.rb`は`otama_import`の出力(id.txt)から画像ファイル名と`otama_id`の対応表(./data/example.ldb)を構築しています。

## ウェブサービス起動

    rackup

ブラウザで http://localhost:9292/ にアクセスして表示されていれば成功です。
画像をクリックするとその画像をクエリに検索します。

    thin start --socket /dev/shm/example.socket

でthinとUNIX Socketでも動かせます。

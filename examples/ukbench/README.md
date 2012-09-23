# ukbench

[Recognition Benchmark Images](http://vis.uky.edu/~stewe/ukbench/)を実行します。

## 必要なライブラリをインストール

(ubuntu)

    sudo apt-get install libgdbm-dev libffi-dev
    sudo gem install bundler
    sudo bundle install

otamaとotamaのruby拡張ライブラリはインストール済みとします。

./dataというディレクトリがない場合は作っておきます。（`config.yaml`で指定されている）

    mkdir data

## ukbenchを配置

ファイルは2GB以上あるので本当にやる気のある人だけ…
 
    wget http://www.vis.uky.edu/~stewe/ukbench/ukbench.zip
    unzip ukbench.zip -d ukbench
 
./filelist.txtに画像のパスが入っています。
ファイル名やパスが変わっている場合は

    find ./ukbench/full/ -name "*.jpg" > filelist.txt

などと作り直してください。

## 設定を編集

`config.yaml`がotamaの設定ファイルなので必要なら編集します。
編集しない場合`sim`ドライバで動きます。

## 画像をインポート

    time ./import.sh

終わるまで待ちます。マシンの性能とDBDによりますが3分～30分くらいだと思います。
import.shでやっていることについては、[example-webapp](https://github.com/nagadomi/otama/tree/master/examples/example-webapp/)を参照してください。

## ベンチマーク

ukbench.gdbmがキャッシュに載っていないと重いのでcatしときます。

    cat ukbench.gdbm > /dev/null

ベンチマークを実行。

    ruby ukbench.rb

うちの環境だと1000QPSくらい出て数秒で終わります。

`sim`ドライバだとスコアが3.6くらいだと思います。
（ドライバのパラメーターにも依存している）

`id`ドライバを使う場合は `hit_threshold` を`1`に設定しないとスコアが落ちます。

## vlad_nodbのベンチマーク

`vlad_nodb`は検索機能がないので、`otama_similarity`だけを使って無理やりベンチマークする処理を書いてみました。

    g++ ukbench_vlad.cpp  -Wall -fopenmp -D_GLIBCXX_PARALLEL -O2 -o ukbench_vlad -lotama

でコンパイルすると、`ukbench_vlad`という実行形式ができているので、

    ./ukbench_vlad

で実行されます。めちゃくちゃ遅いので注意してください。（最新のPCでも一晩くらいかかる）

## その他

otamaのBoVWやVLADの量子化テーブル（コードブック）はukbenchで学習しているわけではないですが、
試しにukbenchだけで学習したところほとんど変わらなかったので、あまり気にすることはないと思います。
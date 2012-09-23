# Ubuntu 12.04 へのインストール手順

## 依存しているパッケージをインストール

    sudo apt-get install gcc g++ make autoconf
    
gccなど開発環境がない場合はインストールしておきます。

    sudo apt-get install libpng-dev libjpeg-dev libgif-dev libssl-dev libyaml-dev libsqlite3-dev

各画像フォーマットのライブラリ、YAMLのライブラリ、SQLite3、OpenSSLのライブラリをインストールします。
libssl-devは必須ではありませんが、入れておくとSHA1の速い実装が使えます。
libgifはGIFが読めなくてもいい場合は入れなくていいです。
SQLite3はotamaのテストに最低限必要なRDBMSです。

マスタのRDBMSにPostgreSQLを使う場合はlibpqを入れます。

    sudo apt-get install libpq-dev

マスタのRDBにMySQLを使う場合はlibmysqlclientを入れます。

    sudo apt-get install  libmysqlclient-dev

## KyotoCabinetをインストール

otamaをKyotoCabinetのデータベースに対応させる場合はKyotoCabinetを入れておきます。

    wget http://fallabs.com/kyotocabinet/pkg/kyotocabinet-1.2.76.tar.gz
    tar -xzvf kyotocabinet-1.2.76.tar.gz
    cd kyotocabinet-1.2.76
    ./configure
    make
    sudo make install
    sudo ldconfig

## 画像入力ライブラリ eiio をインストール

画像の入力に使っているeiioというライブラリをインストールします。

    wget https://github.com/nagadomi/eiio/tarball/master -O eiio.tar.gz
    tar -xzvf eiio.tar.gz
    cd nagadomi-eiio-*
    ./configure
    make
    sudo make install
    sudo ldconfig

GIFを使わない場合は`configure`に `--disable-gif` を付けるとGIFの対応が外せます。
jpegやpngも同様に外せますが、外すとotamaのテストが通らなくなります。

## 機械学習と画像処理のライブラリ nv をインストール

otamaのコアな処理はほとんどこのライブラリを呼び出しているだけになっています。

    wget https://github.com/nagadomi/nv/tarball/master -O nv.tar.gz
    tar -xzvf nv.tar.gz
    cd nagadomi-nv-*
    ./configure
    make
    sudo make install
    sudo ldconfig

OpenSSLをリンクしない場合は、configure に `--disable-openssl` を付けます。SHA1にCの実装が使われるので少し遅くなります。

POPCNT命令に対応しているAMD CPUの場合は、configure に `--enable-popcnt` を付けるとビットベクトルの検索が高速になります(simやcolorなどビットベクトルを使うドライバで2～10倍くらい速くなります)。Intel CPUの場合はSSE4.2に対応していれば自動で有効になります。

## 画像検索エンジンのライブラリ otama をインストール

    wget https://github.com/nagadomi/otama/tarball/master -O otama.tar.gz
    tar -xzvf otama.tar.gz
    cd nagadomi-otama-*
    ./configure --enable-pgsql --enable-mysql --enable-kyotocabinet
    make
    make check
    sudo make install
    sudo ldconfig

PostgreSQLのDBドライバを有効にする場合は、configure に `--enable-pgsql` を付けます。
MySQLのDBドライバを有効にする場合は、configure に `--enable-mysql` を付けます。

KyotoCabinetに対応する場合は、configure に `--enable-kyotocabinet` を付けます。
KyotoCabinetを有効にした場合は、転置インデックスにKyotoCabinetを使ったドライバがコンパイルされ、idドライバがKyotoCabinetの実装に置き換わります。

POPCNT命令に対応しているAMD CPUの場合は、configure に `--enable-popcnt` を付けるとビットベクトルの検索が高速になります(simやcolorなどビットベクトルを使うドライバで2～10倍くらい速くなります)。Intel CPUの場合はSSE4.2に対応していれば自動で有効になります。

## Ruby拡張ライブラリをインストール

rubyがなければインストール。

    sudo apt-get install ruby1.9.3

otamaのソースコードのrubyディレクトリにRuby拡張ライブラリのソースコードがあります。

    cd ruby
    ruby extconf.rb
    make
    sudo make install

拡張ライブラリをコンパイルしてインストールします。
`extconf.rb`でlibotamaが見つからないと言われるときは`--with-otama-dir=`でotamaをインストールした時のprefixを指定します。

    ./test.sh

でテストが動きます。

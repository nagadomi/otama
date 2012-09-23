# MinGW32へのインストール手順

MinGWは使うライブラリのポーティングも伴うのでちょっと面倒です。
autotoolsが分かることを前提とします。

MinGW版は、OpenMPの実装がpthreads-win32になるためMSVCでコンパイルされたアプリケーションのスレッド上から呼び出すとスレッド関係のエラーが発生します。
MSVCのアプリケーション用にはMSVCでコンパイルしてください。
OpenMPはnvとotamaのコンパイル時に`--disable-openmp`をつけると無効にできますが、並列処理ができなくなるのでかなり遅くなります。

installation-ubuntu.md にeiio、nv、otamaのコンパイルオプションに関する詳しい説明があるので目を通しておいてください。

## MinGW

MinGWの環境をインストールします。
インストール時にdeveloper-toolsとgcc、g++を入れておきます。

http://www.mingw.org/wiki/Getting_Started

必要なライブラリをインストールします。

    mingw-get update
    mingw-get install msys-wget
    mingw-get install libz
    mingw-get install msys-libopenssl

## 環境変数

`_WIN32_WINNT` > 0x05001 などに依存しているライブラリがあるので設定しておきます。
またデフォルトのstackが小さくてコンパイルできないことがあるので増やしておきます。

    export CFLAGS=" -I/usr/include -I/usr/local/include -D_WIN32 -DWINVER=0x0501 -D_WIN32_WINNT=0x0501 "
    export CPPFLAGS=$CFLAGS
    export CXXFLAGS=$CXXFLAGS
    export LDFLAGS=" -L/usr/lib -L/usr/local/lib -Wl,--stack,10485760 "

## libyaml

http://pyyaml.org/wiki/LibYAML
 
    wget http://pyyaml.org/download/libyaml/yaml-0.1.4.tar.gz
    tar -xzvf yaml-0.1.4.tar.gz
    cd yaml-0.1.4

DLLが作成されないのでconfigure.acの`AM_INIT_AUTOMAKE`の下に

    LT_INIT([win32-dll])
    
src/Makefile.amのLDFLAGSに

    -no-undefined
    
を追加して

    autoreconf -fi
    export CFLAGS="$CFLAGS -DYAML_DECLARE_EXPORT"
    export CPPFLAGS=$CFLAGS
    ./configure
    make
    make check
    make install

でDLLがインストールされます。

## libpng

http://www.libpng.org/pub/png/libpng.html

    ./configure && make install

## libjpeg(jpegsrc.v8c.tar.gz)

http://www.ijg.org/

    ./configure && make install
    
## sqlite3

http://www.sqlite.org/

sqlite-autoconf-*をコンパイルします。

    ./configure && make install

## eiio

    ./configure --disable-gif
    make && make install

GIFは無効にします。
使いたい場合は、giflibのソースを取ってきて、libyamlと同じようにDLLが生成されるように修正した後ビルドすれば使えます。

## nv

    ./configure
    make && make install

## KyotoCabinet

kyotocabinetはちょっと面倒なので省略します。mingwのlibregexがセグフォするので手動で対策するか、pcreで置き換えれば使えます。

## otama

    ./configure
    make
    make check
    make install

## その他

Windowsはmmapの実装上、同じprefixのインスタンスがひとつしか作れません。


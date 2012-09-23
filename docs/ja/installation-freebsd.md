# FreeBSD 9.0 RELEASE へのインストール手順

まずは installation-ubuntu.mdを参照してください。

apt-getをportsかpackageに読み替えて必要なパッケージ入れたあとソースコードからインストールします。

prefixを指定せずにソースからインストールすると/usr/localに入るのでgcc環境変数の設定が必要です。

    export C_INCLUDE_PATH=/usr/local/include
    export CPLUS_INCLUDE_PATH=/usr/local/include
    export LIBRARY_PATH=/usr/local/lib

このあとeiio, nv, otamaの順でコンパイル、インストールすれば素直に動きました。

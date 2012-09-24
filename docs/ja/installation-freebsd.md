# FreeBSD 9.0 RELEASE へのインストール手順

まずは installation-ubuntu.mdを参照してください。

apt-getをportsかpackagesに読み替えて必要なパッケージ入れたあとソースコードからインストールします。

packagesの場合

    sudo pkg_add -r png jpeg giflib sqlite3 libyaml

prefixを指定せずにソースからインストールすると/usr/localに入るのでgccの環境変数の設定が必要です。

    export C_INCLUDE_PATH=/usr/local/include
    export CPLUS_INCLUDE_PATH=/usr/local/include
    export LIBRARY_PATH=/usr/local/lib

このあとeiio, nv, otamaの順でコンパイルすればmake checkまでは素直に動きました。

他、packageのRubyはiconvが入っていないのでportsから入れる必要があるとか
いろいろ細かいつまづきどころはありますが
そのへんはFreeBSDの環境の話なので省略します。

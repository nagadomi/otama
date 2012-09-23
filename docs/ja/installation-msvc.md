# Microsoft Visual Studio 2008 でのコンパイル

## Windows SDKのインストール

VisualSudio 2008 Expressの場合は、OpenMPのヘッダーファイルがないのでWindows SDKを入れておきます。

http://www.microsoft.com/en-us/download/details.aspx?id=11310

インストール後にVisualSudio 2008 Expressのコンソールから

    cd \Program Files\Microsoft SDKs\Windows\v6.1\Setup
    WindowsSdkVer.exe -version:v6.1

を実行して、コンフィギュレーションを行います。（GUIのツールからはできないため)

参照: http://blogs.msdn.com/b/windowssdk/archive/2008/02/22/using-visual-c-2008-express-with-the-windows-sdk-detailed-version.aspx

## libyaml

http://pyyaml.org/wiki/LibYAML

1. win32\vs2008にソリューションファイルがあるのでyamldllをビルドします。
2. 生成されたyaml.libとyaml.dll、yaml.hをそれぞれVSから読めるディレクトリへ移動させます。

## libpng

http://www.zlib.net/

http://www.libpng.org/pub/png/libpng.html

libzも必要なのでダウンロードします。

1. libpngと同じ階層にlibzを解答してフォルダ名を`zlib`にします。
2. libpngのprojects\visualc71からソリューションファイルを開いてzlibとlibpngをLIB Releaseでビルドします。このときランタイムライブラリをマルチスレッドDLLにしておきます。
3.  libz.lib libpng.lib png.h pngconf.h pnglibconf.h をそれぞれVSから読めるディレクトリへ移動させます。

## libjpeg

http://www.ijg.org/

1. jpegsr8d.zipをダウンロードする。
2. `move jconfig.vc jconfig.h` リネームします。
3. makefile.vcの`CFLAGS= $(cflags) $(cdebug) $(cvars) -I.`を`CFLAGS= -Ox -MD -I.`に書き換えます。
3. `namke -f makefile.vc nodebug=1` ビルドします。
4. `libjpeg.lib` `ejconfig.h` `jerror.h` `jmorecfg.h` `jpeglib.h` をそれぞれVSから読めるディレクトリへ移動させます。

## giflib

http://sourceforge.net/projects/giflib/

GifLib for Windows: http://gnuwin32.sourceforge.net/packages/giflib.htm

giflibはビルドが面倒なのでGnuWin32のバイナリを使用します。

1. giflibのページのBinaries、Developer filesに`giflib4.dll` `giflib.lib` `gif_lib.h`が入っているのでVSから読めるディレクトリへ移動させます。

## sqlite3

http://www.sqlite.org/

1. sqlite-dll-win32-x86-*.zipに`sqlite3.dll`と`sqlite3.def`が入っているのでVSから読めるディレクトリへ移動させます。
2. `lib /DEF:sqlite3.def /MACHINE:X86` で`sqlite3.lib`を生成してVSから読めるディレクトリへ移動させます。
3. sqlite-amalgamation-*.zipに入っている`sqlite3.h`をVSから読めるディレクトリへ移動させます。

## eiio

1. `vcproj/eiio.sln` をリリースモードでビルドします。設定を変える場合は、`eiio_config_msvc.h`を編集します。
2. `libeiio-0.lib` `eiio.h` `eiio_config.h` `eiio_config_msvc.h` をそれぞれVSから読めるディレクトリへ移動させます。

## nv

1. `vcproj/nv.sln` をリリースモードでビルドします。プリプロセッサで使用するSIMD命令を設定してください。`__SSE2__`までは多くのPCで動くと思いますが、それ以上のフラグを有効にすると新しめのCPUでなければ動きません。`__SSE_4_2__`を有効にするとAMD CPUでは動きません。またコンパイラの対応も必要です(VS2008ではAVXが使えない)。
2. `libnv-2.lib` `libnv-2.dll` `eiio.h` `eiio_config.h` `eiio_config_msvc.h` をそれぞれVSから読めるディレクトリへ移動させます。

## otama

1. `vcproj/otama.sln`をリリースモードでビルドします。
2. 必要なライブラリとヘッダーファイルを何かしら利用します。おわり。


ここからが面倒で、bovwのドライバが使っているデータファイルが otama/src/nvbovw/の`nv_bovw*.kmt.gz` `nv_bovw*.kmt.gz`にあるので、これらをDLLから読める形式に変更して適切な場所に配置しなければいけません。Linux、FreeBSD、MinGWの場合は、make時に以下のようなコマンドで自動で生成されます。

    gzip -d -c nv_bovw2k_idf.mat.gz > nv_bovw2k_idf.mat
    gzip -d -c nv_bovw8k_idf.mat.gz > nv_bovw8k_idf.mat
    gzip -d -c nv_bovw512k_idf.mat.gz > nv_bovw512k_idf.mat
    gzip -d -c nv_bovw2k_posi.kmt.gz > nv_bovw2k_posi.kmt
    gzip -d -c nv_bovw2k_nega.kmt.gz > nv_bovw2k_nega.kmt
    gzip -d -c nv_bovw8k_posi.kmt.gz > nv_bovw2k_posi.kmt
    gzip -d -c nv_bovw8k_nega.kmt.gz > nv_bovw2k_nega.kmt
    gzip -d -c nv_bovw512k_posi.kmt.gz > nv_bovw512k_posi.kmt
    gzip -d -c nv_bovw512k_nega.kmt.gz > nv_bovw512k_nega.kmt    

解凍して、

    nv_matrix_text2bin nv_bovw2k_idf.mat nv_bovw2k_idf.matb
    nv_matrix_text2bin nv_bovw8k_idf.mat nv_bovw8k_idf.matb
    nv_matrix_text2bin nv_bovw512k_idf.mat nv_bovw512_idf.matb
    nv_kmeans_tree_text2bin nv_bovw2k_posi.kmt nv_bovw2k_posi.kmtb
    nv_kmeans_tree_text2bin nv_bovw2k_nega.kmt nv_bovw2k_nega.kmtb
    nv_kmeans_tree_text2bin nv_bovw8k_posi.kmt nv_bovw8k_posi.kmtb
    nv_kmeans_tree_text2bin nv_bovw8k_nega.kmt nv_bovw8k_nega.kmtb
    nv_kmeans_tree_text2bin nv_bovw512k_posi.kmt nv_bovw512k_posi.kmtb
    nv_kmeans_tree_text2bin nv_bovw512k_nega.kmt nv_bovw512k_nega.kmtb

バイナリ形式に変換します。

`nv_matrix_text2bin` `nv_kmeans_tree_text2bin`というコマンドはnvのsrc/utilにソースコードがありますが、vcprojではコンパイルされないので、自前でコンパイルするか、他の環境で作った*.kmtb *.matbを持ってくるかします（CPUによっては互換性がないかもしれない）。

matbとkmtbはDLLがあるディレクトリの直下に`share`というディレクトリを作って、そこに入れておくとDLLから使えます。

## まとめ

非常に面倒なのである程度まとまったらバイナリを用意します。
上の手順で試してotama_testが通るところまでは確認しています。

## その他

Windowsはmmapの実装上、同じprefixのインスタンスがひとつしか作れません。

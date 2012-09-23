# otama

otamaはコンテンツベースの画像検索エンジンを構築するためのライブラリです。
画像をクエリに画像が検索できます。また画像同士の類似度を求めることもできます。

ドライバというレイヤに様々な画像特徴とその類似尺度での検索エンジンが実装されていて、それらを共通のAPIで扱います。

現在、C言語とRubyのインターフェースがあります。

## Usage

otamaは、リレーショナルデータベースをマスタとしたスレーブとして動作します。
基本的なAPIとしてinsert、remove、pull、searchがあり、insert、removeはマスタ側を操作し、pullによってマスタの更新情報をローカルに取得して画像検索用のインデックスを構築します。
searchではローカルに作られた検索インデックスを使って検索します。

リレーショナルデータベースは、PostgreSQL、MySQL、SQLite3から選択できます。
SQLite3を使った場合はマスタもスレーブもローカルになるのでアプリケーション組み込みの画像検索エンジンとして使えます。

### 画像検索 (Ruby API)

```ruby
# -*- coding: utf-8 -*-
require 'otama'

FILE1 = "./image/lena.jpg"
FILE2 = "./image/lena-rotate.jpg"

# 設定
CONFIG = {
  # 名前空間. 省略可能.
  # 指定した場合やテーブルやデータファイルなどにこの名称がprefixとして付加されるので
  # 同じデータベースやディレクトリに複数のインデックスを作ることができる.
  :namespace => "piyopiyo",
  
  # ドライバの設定
  :driver => {
    # 画像検索エンジンのドライバ名
    #
    # color: 画像の色の分布で検索するドライバ (雰囲気が似ている風景写真など）
    # id: 全く同じものが写っている画像を検索するドライバ (本やCD,お菓子の袋など印刷物のみ)
    # sim: colorとidの中間くらいの適当な類似検索ドライバ
    :name => :sim,
    
    # ドライバがローカルの検索インデックス構築などに使うディレクトリ
    :data_dir => "./data",
    
    # 他にドライバのパラメーターがあれば続く..
    # たとえばsim固有のパラメーター color_weight
    # 色の重み(0.0-1.0), BoVWの重み => 1.0 - color_weight
    :color_weight => 0.2
  },
  # データベースの設定
  # マスタとして使われる
  :database => {
    # データベースドライバ名
    # pgsql, mysql, sqlite3
    :driver => "pgsql",
    
    # データベース名
    # sqlite3の場合はデータベースのファイル名を指定する
    :name => "dbname",
    
    # MySQL, PostgreSQLの場合は接続情報が続く
    # 省略した場合はDBドライバ(libpq, libmysqlclient)のデフォルト値になる
    :host => "db8.example.net",
    :port => 5432,
    :user => "nagadomi",
    :password => "1234"
  }
}
# この設定はYAMLファイルに記述してOtama.openの引数にファイル名で渡すこともできる

kvs = {}
Otama.open(CONFIG) do |db|
  # マスタにテーブルを作成する(すでにある場合は無視）
  db.create_table

  # 画像をデータベースに登録
  #
  # 登録されるのは画像から抽出した特徴量と画像のID(SHA1)だけで
  # 画像自体はotama側では管理しない

  # ファイルから
  id1 = db.insert(:file => FILE1)
  # メモリ上のデータから
  id2 = db.insert(:data => File.read(FILE2))

  # insertからは追加した画像データを識別するためのIDが返ってくるので
  # ファイル名などと対応付けて保持しておく
  kvs[id1] = FILE1
  kvs[id2] = FILE2

  # この時点ではまだマスタのデータベースに追加されただけで検索できる保証はない
  
  # pullによってマスタの更新情報をローカルの検索インデックスに反映させる
  db.pull

  # pullのあとは検索できる

  # 画像をクエリに類似検索(似ているほうから10件検索)
  # クエリ :file => ファイル名, :data => 画像データ, :id => 登録済み画像ID, ...
  results = db.search(10, :file => FILE1)

  # 検索結果表示
  puts "---"
  results.each_with_index do |result, i|
    puts "#{i}: file => #{kvs[result.id]}, #{result.value.inspect}"
  end
  
  # FILE1を削除
  # (vacuum処理は行われないので
  # 大量に削除した場合は作りなおしたほうがいい...)
  db.remove(id1)
  
  # この時点ではまだマスタのデータベースで削除されただけで
  # 検索結果から消えている保証はない
  
  # pullによってマスタの更新情報をローカルの検索インデックスに反映させる
  db.pull
  
  # メモリ上の画像データをクエリに類似検索(似ているほうから10件検索)
  results = db.search(10, :data => File.read(FILE1))

  # 検索結果表示
  puts "---"
  results.each_with_index do |result, i|
    puts "#{i}: file => #{kvs[result.id]}, #{result.value.inspect}"
  end
end

```
    ---
    0: file => ./image/lena.jpg, {:similarity=>0.9999999403953552}
    1: file => ./image/lena-rotate.jpg, {:similarity=>0.548335611820221}
    ---
    0: file => ./image/lena-rotate.jpg, {:similarity=>0.548335611820221}

## 画像の比較 (Ruby API)

```ruby

# -*- coding: utf-8 -*-
require 'otama'
require 'json'

FILE1 = "./image/lena.jpg"
FILE2 = "./image/lena-rotate.jpg"
FILE3 = "./image/baboon.png"

# 設定
CONFIG = {
  # ドライバの設定
  :driver => {
    # 画像類似度を求めるドライバ名
    # vlad_nodb: Vector of Locally Aggregated Descriptorsによる画像比較のドライバ
    # color_nodb, sim_nodb, id_nodb ...
    #
    # 名前に_nodbがついているドライバは open, close, similarity, feature_string API
    # しか実装されていないドライバ。
    # 登録や検索はできないサブセット。なのでDBの設定は必要ない。
    :name => :vlad_nodb
  }
}

# openにブロックを渡さない場合はドライバのインスタンスが返る
# (この場合手動でcloseする.
#  特にKyotoCabinetが有効な場合のid(bovw512k_iv_kc)は
#  closeしないとKyotoCabinetのデータベースファイルが壊れるので注意)
vlad = Otama.open(CONFIG)

puts "---"
# 画像ファイル同士を比較
score = vlad.similarity({:file => FILE1}, {:file => FILE2})
puts "#{score.round(2)} : #{FILE1} x #{FILE2}"
# 画像ファイルとメモリ上の画像データを比較
score = vlad.similarity({:file => FILE1}, {:data => File.read(FILE3)})
puts "#{score.round(2)} : #{FILE1} x #{FILE3}"

# 生の特徴量を得る
# 特定の画像をリアルタイム映像と比較して対象が見つかった時にアラートを上げるなど
# 同じ画像で何度も比較する場合は、特徴抽出した結果をキャッシュして使いまわすと高速に比較できる
fv = vlad.feature_raw(:file => FILE1)
puts "---"
puts vlad.similarity({:raw => fv}, {:raw => fv})
puts vlad.similarity({:raw => fv}, {:file => FILE2})
puts vlad.similarity({:raw => fv}, {:file => FILE3})

# 特徴量のメモリを開放する
# RubyのGCで回収された時に自動で開放されるが
# 明示的にも開放できる
fv.dispose

# ドライバを閉じる
vlad.close
```

    ---
    0.71 : ./image/lena.jpg x ./image/lena-rotate.jpg
    0.0 : ./image/lena.jpg x ./image/baboon.png
    ---
    1.0000007152557373
    0.7062699794769287
    0.0038108634762465954

類似度の値域はドライバ依存ですが、今あるドライバはすべて0.0から1.0までを返します。
ただ、型の変換時などに誤差が生じるので上の例のように少し超えることがあります。

また特徴量は

    fv = vlad.feature_string(:file => FILE1)

で文字列にシリアライズされた形式でも取得できます。文字列なので外部のストレージなどに保存できます。

使うときはそのまま

    puts vlad.similarity({:string => fv}, {:string => fv})

としても使えますが、デシリアライズが毎回走って重いので

    fv = vlad.feature_raw(:string => fv)

と一度raw形式に変換すると、以後rawとして使えます。

まとめると、searchとsimilarityの引数には、

    :file => 画像ファイルのパス(PNG,JPEG,GIFなどeiioで読める形式)
    :data => 画像データ(PNG,JPEG,GIFなどeiioで読める形式)
    :raw => feature_rawで得た特徴量
    :string => feature_stringで得た特徴量
    :id => 登録済みの画像ID(otama_id)

で対象が指定できます。insertは、file、dataのみ、removeはidをスカラで指定します。
rawに変なデータや開放済みのデータを渡すので落ちたりするので注意してください。

### エラー処理

エラー処理はCのレベルではとりあえずエラーということを返すだけになっていて、Rubyだととりあえず例外(Otama::Errorを継承したクラス)を投げます。

## コマンドライン ユーティリティ

config.yamlというファイルに

    ---
    driver:
      name: sim
      data_dir: ./
  
    database:
      driver: sqlite3
      name: ./example.sqlite3

のような設定を書いておいて、

    otama_create_table -c config.yaml

でマスタ側にテーブルを作ります。

    otama_drop_table -c config.yaml

でマスタ側のテーブルを消します。（本当に１コマンドで消えてしまうので注意）

初期データとして大量の画像を追加する場合は、

    find your-image-dir/ \( -name \*.jpg -o -name \*.png -o -name \*.jpeg \) -print > filelist.txt

などと画像ファイル一覧を記述したファイルを作って、

    otama_import -c config.yaml filelist.txt > id.txt

で全件マスタにインポートします。（件数によっては数日かかったりするのでnohupなどで動かしたほうがいいかもしれません）

`otama_import`の出力は、1画像1行で

    [otama_id][スペース][画像のファイルパス]

というフォーマットなので、出力されたid.txtからアプリケーション用のデータへ関連付けたりします。

    otama_pull -c config.yaml

でマスタからデータを取り込みます。（ローカルのストレージを使うドライバの場合）

各コマンドは-dオプションをつけるとデバッグ用のメッセージを出します。僕以外には意味分からない情報がほとんどだと思いますが、発行しているSQLなども出るので、pullに異常に時間かかるけど動いているのかこれ、などと不安になった時につけてみるといいかもしれません。

[examples/example-webapp](https://github.com/nagadomi/otama/tree/master/examples/example-webapp)にウェブアプリケーションにデータをインポートしたあと検索したりの例があります。

## Installation

Ubuntu 12.04、FreeBSD 9.0 RELEASE、Windows XP/Windows 7 + MinGW32、 Windows XP/Windows 7 + Microsoft Visual C++ 2008 Express + Windows SDK for Windows Server 2008 and .NET Framework 3.5 で動作確認しています。

[docs](https://github.com/nagadomi/otama/tree/master/docs/ja)/installation-* を参照してください。

## 使用しているライブラリなど

- [eiio](https://github.com/nagadomi/eiio)
- [nv](https://github.com/nagadomi/nv)
- [libYAML](http://pyyaml.org/wiki/LibYAML)
- [SQLite3](http://www.sqlite.org/)
- [Kyoto Cabinet](http://fallabs.com/kyotocabinet/) (optional)
- [PostgreSQL](http://www.postgresql.org/) (optional)
- [MySQL](http://www.mysql.com/)  (optional)

## Drivers

各ドライバの紹介です。

[drivers](https://github.com/nagadomi/otama/blob/master/docs/ja/drivers.md)

## TIPS

ちょっと気になりそうなところを。

[tips](https://github.com/nagadomi/otama/blob/master/docs/ja/tips.md)

## APIリファレンス

mada nai

[src/tests](https://github.com/nagadomi/otama/tree/master/src/tests)

にC APIのテストがあります。

[ruby](https://github.com/nagadomi/otama/tree/master/ruby)

にRuby APIのテストがあります。

[examples](https://github.com/nagadomi/otama/tree/master/examples)

にはウェブアプリとRPC(WebAPI)サーバー/クライアントの例などがあります。(Ruby)


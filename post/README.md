Apache Post Module Sample
====

# 概要
Apache ModuleでPOSTリクエストを取得するサンプルです

# セットアップ

以下でコンパイルしてsoファイルを作成します
```
$ make install
```

httpd.confにディレクティブを追加します
```
$ sudo vim /etc/httpd/conf/httpd.conf
```

httpd.confに以下の内容を追加する
```
LoadModule blog_module modules/blog.so
<Location /blogpost>
    SetHandler blog 
</Location>
```

apacheを再起動します
```
$ sudo apachectl restart
```

テスト用のHTMLを配置します
```
$ sudo cp post.html  <YourDocumentRoot>/post.html
```

以下のURLにアクセスして、name,msgを入力してPOSTボタンを押下します。
```
http://<your_domain>/post.html
```

今回作成したハンドラが表示されます


# 参考
 - [プログラマでいたい](http://blog.livedoor.jp/matssaku/archives/50427866.html)

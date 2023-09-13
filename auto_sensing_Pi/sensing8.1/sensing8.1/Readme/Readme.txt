2019.0624 by NKD: sensing3 : dps 首を降ってdps 移動はしない
2019.0703 by NKD: sensing4 : 2ch用の修正
2019.0710 by NKD: sensing5 : FPGAの初期設定を start_carbat.pyに埋め込んだ。それに伴いstart.csh を消去。
2019.0711 by NKD: sensing7 : 保存ファイルをまとめた。定位方向算出式のミスを改善。その他プログラムをきれいに。
2019.0711 by NKD: sensing7.1 : ピーク値の大きさを使って、エコー各々に重み付け(pair_alpha)をおこなうことで回避優先度のある定位物体とした。

	起動コマンド:sudo python3 start_carbat.py
		*python3 を動かすためにはライブラリのインストールが必要です。python_lib_install.txtを参照してください
	核プログラムはsensing.bin だが、コマンドライン引数の設定をしているため、sudo ./sensing.bin はエラー出ます。
	pythonで表示される定位マップのカラーマップは危険な物体（pair_alphaが大きいもの）ほど濃色にしています。ペアリングミスを消すのではなく重み付けで対処してみました。（要検討）
	定位対象物がすくないときはsensing.cの l44 #define thd_limit   の値で調整できます。



2019.1015 by NKD: sensing8 角度と距離を細かい精度で出力する。また、車が動かないモードも追加。 
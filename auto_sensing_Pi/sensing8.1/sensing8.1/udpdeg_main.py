#!/usr/bin/env python3
import socket #UDP送信
import time #待機時間用
import struct #数値→バイト列変換用
import ipaddress #入力IPアドレスの形式確認用
from contextlib import closing #with用from contextlib import closing #with用
# -*- coding: utf-8 -*-
class senddeg():
    def sendreceive(avdeg):
        #IPアドレスの入力関係
        #送信の設定
        host = '169.254.220.205'  # 送信先（相手）IPアドレス
        send_port = 60000 # 送信ポート番号
        #受信の設定
        recv_ip = "" #このままでいい
        recv_port = 60000 #ポート番号
        #2つのsocketを設定
        socksend = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) #送信ソケットの設定
        sockrecv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) #受信ソケットの生成
        sockrecv.bind((recv_ip, recv_port)) #ソケットを登録する
        sockrecv.setblocking(1) #ブロッキング受信に設定
        #print("OK") #準備完了であることを示す

      #送受信
 #プログラム終了時にソケットを自動的に閉じる
   #      while True: #無限ループ
            #送信
            # 1秒ごとに一方的に送信する
     #       print("turn degrees =")
     #       print(">",end="")
      #     print("send: ", avdeg) #送信する数値を送信側に表示
        ss = struct.pack('>i', avdeg) #バイト列に変換
        socksend.sendto(ss, (host, send_port)) #ソケットにUDP送信
            #待機
#            time.sleep(1.5) #1秒待機
            #受信
            # パケットを受信した場合のみ、結果を表示する。それ以外は何もせずスルーする
#            try: #try構文内でエラーが起こるとexceptに飛ぶ、なければelseへ
        print("wait receive")
        sr, addr = sockrecv.recvfrom(1024) #受信する
           # except socket.error: #受信していなければなにもしない
            #  pass
            #else: #受信していたら表示
        r = struct.unpack('>d' , sr)[0] #バイト列を数値に変換
        print ( "receive: " , str( r )) #数値に変換して表示
       #      break
        closing(socksend)
        closing(sockrecv)

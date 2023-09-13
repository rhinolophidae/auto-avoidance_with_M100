#!/usr/bin/env python

import socket
import time
from datetime import datetime

HOST_IP = "169.254.12.113" # 接続するサーバーのIPアドレス
PORT = 12345 # 接続するサーバーのポート
DATESIZE = 1024  # 受信データバイト数

class SocketClient():

    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.socket = None
        
    def send_recv(self, input_data):
        
        # sockインスタンスを生成
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            # ソケットをオープンにして、サーバーに接続
            sock.connect((self.host, self.port))
            print('[{0}] input data : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), input_data) )
            # 入力データをサーバーへ送信
            sock.send(input_data.encode('utf-8'))
            # サーバーからのデータを受信
            rcv_data = sock.recv(DATESIZE)            
            rcv_data = rcv_data.decode('utf-8')
            print('[{0}] recv data : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), rcv_data) )

def main():
    with open('det_log.txt', 'r') as f:
        input_data = f.readlines()
    client = SocketClient(HOST_IP, PORT)
    client.send_recv(input_data)
"""
if __name__ == '__main__':
    client = SocketClient(HOST_IP, PORT)
    while True:
        input_data = input("send data:") # ターミナルから入力された文字を取得
#        input_data = dist_direc
        client.send_recv(input_data)
"""
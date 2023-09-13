#!/usr/bin/env python
# -*- coding: utf-8 -*-

import socket
import time
from datetime import datetime
import sys

HOST_IP = "169.254.12.113" # Connected IP
PORT = 12345 # Port number
DATESIZE = 1024  # recieved byte size

class SocketClient():

    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.socket = None
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #(TCI processing)
        self.conect_setup()
        
    def conect_setup(self):
        # making the sock instance
        print(self.host,self.port,self.sock)
        connect_flg = 0
        while connect_flg == 0:
            try:
                #sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #(TCIProcessing)
                #sock=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)   #(UDPProcessing)
                # Socket open、Connected server
                self.sock.connect((self.host, self.port))
                print()
                print("connection_success")
                print()
                connect_flg = 1
            except:
                print("miss_connection!! try again!")
                time.sleep(0.5)
        
    def send_recv(self, input_data):
        try:
            # send to server
            self.sock.send(input_data.encode('utf-8'))
            print()
            print("data sending")
            print('[{0}] send data : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'), input_data))
            # received from server
#            rcv_data = sock.recv(DATESIZE)            
#            rcv_data = rcv_data.decode('utf-8')
#            print('[{0}] recv data : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), rcv_data) )
        except:
            print()
            print("miss_sensding!!")
            print()
            self.conect_setup()
    def soc_close(self):
            self.sock.close

            
def flg_read_checker(Flg_now):
    read_flg=-1
    obj_file='flg.prm'
    
    while(read_flg <= Flg_now):
        try:
            with open(obj_file, mode='r') as f:
#                fcntl.flock(f.fileno(), fcntl.LOCK_EX)
                read_flg=int(f.readline())
#                fcntl.flock(f.fileno(), fcntl.LOCK_UN)
        except:
            print("read_open error!! read: "+ str(read_flg) )
    print("py_read_flg: " + str(read_flg))
    return read_flg

def flg_writer(flg):
    obj_file='flg.prm'
    try:
        with open(obj_file, mode='w') as f:
            f.write(str(flg))
    except:
        print("write_open error!! flg: "+ str(flg))
    print("py_write_flg: " + str(flg))
            
def read_txt(name):
    with open(name, 'r') as f:
        input_data = f.readline() #list ni naru no de readlines ha dame!!
    return input_data

def main():
    
    tim_ip=time.time()  
    client = SocketClient(HOST_IP, PORT)
    
    max_pulse_n=int(read_txt('pulse_n.prm'))
    flg_now=-1
    flg_writer(flg_now)
    
    try:
        while True:  
            while flg_now < max_pulse_n-1:
                flg_now = flg_read_checker(flg_now)
                val=read_txt('det_log.txt')

                input_data=val
                client.send_recv(input_data)

                tim_fp=time.time()
                print("total_tppsh: " + str(tim_fp-tim_ip))
                print()
                print()
                print()
                print()
    except KeyboardInterrupt:
        print()
        print('interrupted!')
        client.soc_close()
        print("socket_close!!")
        print()
        sys.exit()
    
    
if __name__ == '__main__':    
    main()
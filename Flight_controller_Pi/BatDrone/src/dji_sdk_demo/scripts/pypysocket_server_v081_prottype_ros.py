#!/usr/bin/env python2
###### for_avoid_calc_publish #########
import rospy
from dji_sdk_demo.msg import ObsInfo
import serial
from std_msgs.msg import Int16
from multiprocessing import Pool

##########################################

#DATESIZE to CLIENTNUM no setting de Jikan tanshuku hakareru?
import socket
import threading
from datetime import datetime
import numpy as np
import time
import concurrent.futures
#import pypy_com_v02_proto as acc #kakunin you
#import avoid_calc_com_v01_proto as acc #honban you
#import testtest as acc #honban you

HOST_IP ="169.254.12.113" # IP address for server
PORT=12345
CLIENTNUM=2 # muximum connection limits of the client
DATESIZE=1024 # receiving data bytes setting
results=[0]

class SocketServer():

	def __init__(self, host, port):
		self.host = host
		self.port = port


	def run_server(self):
		#making server socket instance
		try:
			server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) ##TCI connection; kakunin tsuki connection
#			with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket: ##UDP connection; kakunin nashi connection
			server_socket.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
			server_socket.bind((self.host, self.port))
			server_socket.listen(CLIENTNUM)
			print('[{}] run server'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))

			while True:
				# accept the requirement of connection from client
				client_socket, address = server_socket.accept()
				print('[{0}] connect client -> address : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), address))
				client_socket.settimeout(60)
				#activate the thread for every client, working send/rev
				t = threading.Thread(target = self.conn_client, args = (client_socket, address))
				t.setDaemon(True)
				t.start()
		except:
			print('[{0}] connection canceled -> address : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'), address))
		finally:
			server_socket.close()
		return t


	#activate thread function
	def conn_client(self, client_socket, address):
		try:
			while True:
				#receiving the data
				rcv_data= client_socket.recv(DATESIZE)
				if rcv_data: # data return to the client 
#					client_socket.send(rcv_data)
					print('[{0}] recv date : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'), rcv_data.decode('utf-8')))
					result = rcv_data.decode('utf-8')
					############# koko kara tobu beki #################################
					acc(result)
		except:
			print('[{0}] disconnect client -> address : {1}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'), address))
		finally:
			client_socket.close()
		return result

###############################################################################################
def avoid_calc(DIST, DIREC):
	k = 0.8	#0.6 #obs_width_para
	alpha = 1800#1300 #13.0*10000   #1.6	#rep_force_strength__para
	beta = 0.001
	dist=np.array(DIST)
	direc=np.array(DIREC)

	dist_fc1=np.ones(len(dist))/(dist**beta)
	dist_fc2=np.sin(np.arctan2(k,dist))
	repvec_len=alpha*dist_fc1*dist_fc2
	Sumvec_late=sum(repvec_len*np.sin(np.deg2rad(direc)))
	Sumvec_frt=sum(repvec_len*np.cos(np.deg2rad(direc)))
	avoid=np.rad2deg(np.arctan2(-Sumvec_late, 1-Sumvec_frt))
	return avoid

def acc(data):
	if not rospy.is_shutdown():
		det_dist_direc=data
		
		det_dist_direc_list1=det_dist_direc.split()[:-3]
#		print('')
#		print('det_dist_direc_list1')
#		print(type(det_dist_direc_list1))
#		print(det_dist_direc_list1)
#		print('')

		try: 
			det_dist_direc_list=list(map(float,det_dist_direc_list1))
			echo_n=int(len(det_dist_direc_list)/2)
			dist_list=det_dist_direc_list[:echo_n]
			direc_list=det_dist_direc_list[echo_n:]
			avoid_direc=avoid_calc(dist_list, direc_list)
		except:
			echo_n=0
			dist_list=[]
			direc_list=[]
			direc_list=[]
			avoid_direc=0.0

		print('')
		print("dist_list")
		print(dist_list)
		print("direc_list")
		print(direc_list)
		print('avoid')
		print(avoid_direc)
		print('')

#		with open("sensing_log.txt", "a") as f:
		try:
			f = open("sensing_log.txt", "a")
			f.write(" {} \t dist_direc= {} \t avoid = {}\n".format(datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'), det_dist_direc, str(int(avoid_direc))))
		except:
			print("sensing_log_writing_error!!")
		finally:
			f.close()

		obsmsg.obs_num=echo_n
#		obsmsg.obs_dist.data=pl.map(mm_func, dist_list)
		obsmsg.obs_dist.data= dist_list
		obsmsg.obs_direc.data=direc_list
		obspub.publish(obsmsg)
		avoidpub.publish(int(avoid_direc))


def serial_main():
	executor=concurrent.futures.ThreadPoolExecutor(max_workers=4)
	result_r=executor.submit(SocketServer(HOST_IP, PORT).run_server())
	print('')
	print('all_finished')
	print('')
	return str(results[0])


if __name__== "__main__":
	pl=Pool()
###################################################
	rospy.init_node('avoid_calc_com_v01_proto')
	obspub = rospy.Publisher("obs_info", ObsInfo, queue_size=10)
	avoidpub = rospy.Publisher("avoid_direc", Int16, queue_size=10)
	obsmsg = ObsInfo()
###################################################
	serial_main()


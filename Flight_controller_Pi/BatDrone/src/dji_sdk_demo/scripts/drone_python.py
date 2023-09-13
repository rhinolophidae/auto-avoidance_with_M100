#!/usr/bin/env python

import rospy
import tf
import numpy as np
import time
import datetime
import struct
import copy

from numpy import linalg as LA
from dji_sdk.srv import SDKControlAuthority, SetLocalPosRef, DroneTaskControl
from geometry_msgs.msg import QuaternionStamped, Quaternion, PointStamped
from sensor_msgs.msg import Joy
from dji_sdk_demo.msg import DroneInfo
from std_msgs.msg import Int16
from std_msgs.msg import Float32

class Drone:

    def __init__(self):
        self.start_yaw = None
        self.rotation_matrix = None
        self.target_yaw = 0.0
        self.avoid_direc_check = 0.0
        self.center_dist = 0.0 #distance to the circular origin(absolute)
        self.center_direc = 0.0 #direction to the circular origin
        #self.throttle = 1.0
        self.U_turn_flg = 0
        self.stop_count = 0
        self.stop_interval_sec = 10
        self.InterControlInterval_sec = 0.02
        self.stop_count_max = int(np.round(self.stop_interval_sec / self.InterControlInterval_sec))

        self.pos = np.array([0.0, 0.0, 0.0])
        self.center_pos = np.array([7.0, 0.0, 0.0])
        self.pos_diff = self.center_pos - self.pos
        self.attitude = Quaternion()
        self.RPY = np.array([0.0, 0.0, 0.0])

        rospy.Subscriber("/dji_sdk/attitude", QuaternionStamped, self.attitudeCallback)
        rospy.Subscriber("/dji_sdk/local_position", PointStamped, self.localPositionCallback)
        rospy.Subscriber("/avoid_direc", Int16, self.AvoidDirecCallback)

        #rospy.Subscriber("throttle", Float32, self.ThrottleCallback)

        self.ctrl_pub = rospy.Publisher("/dji_sdk/flight_control_setpoint_generic", Joy, queue_size=10)

        self.drone_info_pub = rospy.Publisher("/drone_info", DroneInfo, queue_size=10)



    def authorize(self):
        server_name = "dji_sdk/sdk_control_authority"
        try:
            rospy.wait_for_service(server_name, timeout=5)
        except rospy.exceptions.ROSException as e:
            print("time out")
        try:
            auth_srvs = rospy.ServiceProxy(server_name, SDKControlAuthority)
            resp = auth_srvs(control_enable=True)
            return resp.result
        except rospy.ServiceException as e:
            print("authorize failed: {}".format(e))

    def setOriginPos(self):
        server_name = "dji_sdk/set_local_pos_ref"
        rospy.wait_for_service(server_name, timeout=5)
        try:
            srvs = rospy.ServiceProxy(server_name, SetLocalPosRef)
            resp = srvs()
            return resp.result
        except rospy.ServiceException as e:
            print("set origin failed: {}".format(e))

    def attitudeCallback(self, data):
        self.attitude = data.quaternion
        roll, pitch, yaw = self.quat2euler(self.attitude)
        if self.start_yaw is None:
            self.start_yaw = yaw
            self.rotation_matrix = np.array([[np.cos(yaw), np.sin(yaw), 0],[-np.sin(yaw), np.cos(yaw), 0], [0, 0, 1]])
        self.RPY[:2] = roll, pitch
        self.RPY[2] = self.transAngleRange(yaw - self.start_yaw)
#        print("RPY = {}".format(self.RPY))

    def localPositionCallback(self, data):
        if self.start_yaw is None:
            return
        pos = np.array([data.point.y, -data.point.x, data.point.z])#localposition no shiyou
        
        self.pos = np.dot(self.rotation_matrix, pos.T)
        # self.pos = pos
        # print("pos = ", self.pos)

    def quat2euler(self, q):
        return np.array(tf.transformations.euler_from_quaternion((q.x, q.y, q.z, q.w))) - np.array([0, 0, np.pi / 2])

    def takeoff(self):
        server_name = "/dji_sdk/drone_task_control"
        rospy.wait_for_service(server_name, timeout=5)
        try:
            srvs = rospy.ServiceProxy(server_name, DroneTaskControl)
            resp = srvs(task=4)
            return resp.result
        except rospy.ServiceException as e:
            print("takeoff failed: {}".format(e))


    def startFly(self):
        
        if not self.takeoff():
            print("takeoff failed")
            return False
        else:
            print("takeoff success")
        
        time.sleep(10)
        self.fly_timer = rospy.Timer(rospy.Duration(1.0 / 50.0), self.flyCallback)
        self.drone_info_timer = rospy.Timer(rospy.Duration(1.0 / 50.0), self.droneInfoCallback)
        return True

    def droneInfoCallback(self, event):
        drone_info_msg = DroneInfo()
        drone_info_msg.pos.x = self.pos[0]
        drone_info_msg.pos.y = self.pos[1]
        drone_info_msg.pos.z = self.pos[2]
        drone_info_msg.yaw = self.RPY[2]
		
        print("pos = {}\nyaw = {}\t	avoid_direc = {}\n".format(self.pos, self.RPY[2], self.avoid_direc_check/np.pi*180))
        with open("flight_log.txt", "a") as f:
            f.write("\t {} \t pos = {} \t yaw = {}\n".format(datetime.datetime.now(), self.pos, self.RPY[2]))

        self.drone_info_pub.publish(drone_info_msg)

    #### 20220509_updated ######
    def transAngleRange(self, angle):
        ans = angle
        while ans < -np.pi:
            ans += 2 * np.pi
        while ans > np.pi:
            ans -= 2 * np.pi
        return ans
    #############################
    def flyCallback(self, event):
        msg = Joy()

        """
        Horizontal command: roll and pitch -> 0x00
                            velocity       -> 0x40

        Vertical command  : velocity       -> 0x00
                            altitude       -> 0x10

        Yaw command       : angle          -> 0x00
                            rate           -> 0x08

        Horizontal  kijun  : Ground   -> 0x00
                             Body     -> 0x02
        """
        mode = 0x40 | 0x00 | 0x08 | 0x02 | 0x01
        #mode = 0xC0 | 0x20 | 0x08 | 0x02 | 0x00

		#### turn_control_section #######
        self.pos_diff = self.center_pos - self.pos
        self.center_dist = np.sqrt(self.pos_diff[0]**2 + self.pos_diff[1]**2)
        self.center_direc = np.arctan2(self.pos_diff[1], self.pos_diff[0]) # aractan2(y, x)
#        print("cente_dist = {}".format(self.center_dist))
#        print("cente_direc= {}".format(self.center_direc))
        if self.center_dist >= 7.5:
            print("coming_back!!!!!")
            self.target_yaw = copy.deepcopy(self.center_direc)
            ### 20220510_updated ####
            if np.abs(self.transAngleRange(self.target_yaw - self.RPY[2])) > np.pi/4:
                self.U_turn_flg = 1
            #########################

        self.avoid_direc_check = self.transAngleRange(self.target_yaw - self.RPY[2]) #delta_yaw = target_yaw - present_yaw(RPY[2])		
        delta_yaw = self.transAngleRange(self.target_yaw - self.RPY[2]) #delta_yaw = target_yaw - present_yaw(RPY[2])		
        yaw_rate_gain = 1.5 
        yaw_rate = yaw_rate_gain * delta_yaw #turn_rate * delta_yaw

#        ### 20220509_updated ####
#        max_angle = np.pi
#        min_angle = -max_angle
#        if yaw_rate > max_angle:
#            yaw_rate = max_angle
#        elif yaw_rate < min_angle:
#            yaw_rate = min_angle
        #########################

        ### 20220510_updated ####
        if self.U_turn_flg == 1:
            speed = 0.0
            v_z = 0.0
            self.stop_count = self.stop_count + 1
        else:
            speed = self.calcFlyingSpeed(yaw_rate)
            v_z = 0.0
        #########################

        if self.stop_count >= self.stop_count_max:
            self.stop_count = 0
            self.U_turn_flg = 0

        with open("speed_log.txt", "a") as f:
            f.write("\t {} \t speed = {} \t yaw_rate = {} \t avoid_direc_check = {}\n".format(datetime.datetime.now(), speed, yaw_rate, self.avoid_direc_check))

		#height_const_control
#        if self.pos[2] < 1.3: 
#            speed = 0.3
#            v_z = 0.0

        ctrl_data = [speed, 0.0, v_z, yaw_rate, mode] #[0]~[2]:x, y, z velocity, [3]: yaw_rate, [4]: controll mode
        #ctrl_data = [0.0, 0.0, self.throttle, 0.0, mode]
        #print(ctrl_data[2])

        msg.axes = ctrl_data
        self.ctrl_pub.publish(msg)

    def ThrottleCallback(self, msg):
        self.throttle = msg.data


    def calcFlyingSpeed(self, yaw_rate):
        v_max = 0.31
#        v_max = 0.0
        k = 1.0
        v_min = 0.05
        return (v_max - v_min) * np.exp(-k * (yaw_rate ** 2)) + v_min

    def AvoidDirecCallback(self, msg):
        print("avoidprocess")
        print(msg.data)
        self.avoid_direc_check = msg.data
        self.target_yaw = self.transAngleRange(np.deg2rad(msg.data) + self.RPY[2]) #msg.data_is_relativ_avoid_angle, RPY[2] is absolute yaw angle
        print("target_yaw")
        print(self.target_yaw)




def main():
    rospy.init_node("drone_python")
    drone = Drone()

    print("authorize...")
   
    if not drone.authorize():
        print("authorize failed!")
        return
    print("authorize success")
    
    print("set origin position...")
    if not drone.setOriginPos():
        print("set origin positioin failed")
        return
    print("set origin position success")
    
    result = drone.startFly()
    if not result:
        print("Failed")
        return

    
    rospy.spin()
    print("end")


if __name__ == '__main__':
    main()

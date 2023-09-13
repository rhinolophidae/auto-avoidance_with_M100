#!/usr/bin/env python
import rospy
# from geometry_msgs.msg import Vector3
from dji_sdk_demo.msg import ObsInfo

import numpy as np

def main():
    rospy.init_node('obstacle_test', anonymous=True)

    pub = rospy.Publisher('obs_info', ObsInfo, queue_size=10)
    r = rospy.Rate(10)

    msg = ObsInfo()
    

    while not rospy.is_shutdown():
        num = np.random.randint(1,6)
        dist = np.random.rand(num) * 1000
        # dist = [1]
        direc = np.random.rand(num) * 90 - 45
        # direc = [np.pi / 4]
        msg.obs_num = num
        msg.obs_dist.data = dist
        msg.obs_direc.data = direc

        pub.publish(msg)
        r.sleep()

if __name__ == '__main__':
    try:
        main()

    except rospy.ROSInterruptException:
        pass

Date:2023/09/11 written by Yasufumi Yamada. 
These are back up programs for auto-avoidance flight controller system on M100 with Raspberrypi 3B+. 
----------------------------------------------------------------------------------------------------------------------

“Flight_controller Pi”
Work with
・Raspberrypi 3B+
・UBUNTU MATE (OS)
・ROS Indigo. http://wiki.ros.org/indigo
・DJI Onboard SDK (OSDK) ver 3.7. https://github.com/dji-sdk/Onboard-SDK-ROS/tree/3.7

Download and compile the program.
$ git clone https://github.com/(this program HP address).git 
$ cd BatDrone 
$ catkin_make 

usage
$ roslaunch dji_sdk sdk.launch
$ rosrun dji_sdk_demo pypysocket_server_v081_prottype_ros.py
$ rosrun dji_sdk_demo drone_python.py

Each core python programs are located on 
\Flight_controller_Pi\BatDrone\src\dji_sdk_demo\scripts

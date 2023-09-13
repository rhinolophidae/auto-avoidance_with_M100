Flight_controller_Pi
----------------------------------------------------------------------------------------------------------------------
These are backup programs for the auto-avoidance flight controller system on M100 with Raspberrypi 3B+. <br>
Date:2023/09/11 written by Yasufumi Yamada. <br>

“Flight_controller Pi” Work with <br>
・Raspberrypi 3B+ <br> 
・UBUNTU MATE (OS) <br>
・ROS Indigo. http://wiki.ros.org/indigo <br>
・DJI Onboard SDK (OSDK) ver 3.7. https://github.com/dji-sdk/Onboard-SDK-ROS/tree/3.7 <br>

## How to Compile
download the "Flight_controller_Pi" to your Rasp Pi <br>
$ cd BatDrone <br>
$ catkin_make <br>

## usage
$ roslaunch dji_sdk sdk.launch <br>
$ rosrun dji_sdk_demo pypysocket_server_v081_prottype_ros.py <br>
$ rosrun dji_sdk_demo drone_python.py <br>

Each core Python programs are located on <br>
\Flight_controller_Pi\BatDrone\src\dji_sdk_demo\scripts <br>

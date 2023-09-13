# auto-avoidance_with_M100

Date:2023/09/11 written by Yasufumi Yamada. <br>
These are back up programs for auto-avoidance flight controller system on M100 with Raspberrypi 3B+. <br>

## The system Overview
These auto-avoidance navigation programs are worked with two Raspberrypi connected with wired LAN cable. <br>
One is used for flight controller and the other is used for autonomous sensing. <br>
Programs for Each Raspberry pi is separately packaged as “Flight_controller_Pi” and “auto_sensing_Pi”. <br>

## Preset (Flight_controller_Pi)
“Flight_controller_Pi” <br>
Work with <br>
・Raspberrypi 3B+ <br>
・UBUNTU MATE (OS) <br>
・ROS Indigo. http://wiki.ros.org/indigo <br>
・DJI Onboard SDK (OSDK) ver 3.7. https://github.com/dji-sdk/Onboard-SDK-ROS/tree/3.7 <br>

## compile the program.
download the “Flight_controller_Pi” to your Rasp Pi <br>
$ cd BatDrone  <br>
$ catkin_make  <br>

## usage
$ roslaunch dji_sdk sdk.launch  <br>
$ rosrun dji_sdk_demo pypysocket_server_v081_prottype_ros.py  <br>
$ rosrun dji_sdk_demo drone_python.py  <br>
 
## Preset (“auto_sensing_Pi”)
“auto_sensing_Pi” <br>
Work with <br>
・Raspberrypi 3B+ <br>
・Raspbian(OS, https://www.raspberrypi.com/software/operating-systems/) <br>
・additional sensors with FPGA((XILINX, EDX-302B)) <br>

## compile the program.
download the “auto_sensing_Pi” to your Rasp Pi <br>
$ cd auto_sensing_Pi <br>
$ make <br>

## usage
$ python pypysocket_host_real_v2_endress.py <br>
$ python sensing_yy_C_based_LPM.py <br>


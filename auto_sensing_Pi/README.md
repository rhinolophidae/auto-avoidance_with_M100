Date:2023/09/11 written by Yasufumi Yamada. 
These are back up programs for auto-avoidance flight controller system on M100 with Raspberrypi 3B+. 
----------------------------------------------------------------------------------------------------------------------
 
Preset (“auto_sensing_Pi”)
“auto_sensing_Pi”
Work with
・Raspberrypi 3B+
・Raspbian(OS, https://www.raspberrypi.com/software/operating-systems/)
・additional sensors with FPGA((XILINX, EDX-302B))

Download and compile the program.
$ git clone https://github.com/ (program HP address).git 
$ cd auto_sensing_Pi
$ make

usage
$ python pypysocket_host_real_v2_endress.py
$ python sensing_yy_C_based_LPM.py


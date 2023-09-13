# BatDrone

Date:2022/01/13 written by Yasufumi Yamada.
These are back up programs for auto-avoidance navigation system on M100.
These programs are able to work on UBUNTU MATE
Initial setup in detailes are descrived as follows.

-----------------------------------------------------------------------------------------------------------------------------------
ドローン用RasPiの初期設定 

1. ubuntu mateの導入 
https://qiita.com/hiro-han/items/27c7b2c4bc123d1291ba　を参照 
カメラの設定は不要 
ターミナルで
$ sudo apt update
$ sudo apt upgrade
 
2. ROSのインストール(参照： http://wiki.ros.org/melodic/Installation/Ubuntu ） 
　ターミナルから以下を順に実行（$ は入力しない） 
$ sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list' 
$ sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654  
$ sudo apt update 
$ sudo apt install ros-melodic-desktop  
$ sudo rosdep init 
$ rosdep update 
$ echo "source /opt/ros/melodic/setup.bash" >> ~/.bashrc 
$ source ~/.bashrc 
$ sudo apt install python-rosinstall python-rosinstall-generator python-wstool build-essential  
$ sudo apt install ros-melodic-nmea-msgs
 
3. DJIOSDKのインストール 
　ターミナルから以下を順に実行（$ は入力しない） 
$ git clone https://github.com/dji-sdk/Onboard-SDK.git 
$ cd Onboard-SDK 
$ mkdir build 
$ cd build 
$ cmake .. 
$ make 
$ sudo make install 
 
4. ドローン動かすプログラムのダウンロード&コンパイル 
　ターミナルから以下を順に実行（$ は入力しない） 
$ cd ~ 
$ git clone https://github.com/KOCHOW/BatDrone.git 
ユーザー名とパスワードを聞かれるので
ユーザー名：yasufumiyamada
パスワード：k1kug@sh1r@
$ cd BatDrone 
$ catkin_make 
$ echo "source ~/BatDrone/devel/setup.bash" >> ~/.bashrc
$ export ROS_IP=`hostname -I`
$ export ROS_MASTER_URI=http://$ROS_IP:11311
 
5.ノートPCからRasPiへの接続 
　(1)ユーザー名とホストネームの確認 
　　RasPiのターミナルを開いて表示される文字列の@以前がユーザー名、@以降がホストネーム
　　例えば、　ubuntu@ubuntu-desktop:~ $　と表示されていればubuntuがユーザー名、ubuntu-desktopがホストネーム

　(2)ssh接続
　　ノートPCのターミナル（Windowsの場合はPowerShell）を開き
　　$ ssh (RasPiのユーザー名)@(RasPiのホストネーム).local
　　を実行。例えばユーザー名がubuntu、ホストネームがubuntu-desktopであれば
　　$ ssh ubuntu@ubuntu-desktop.local
　　パスワードを聞かれるので入力

6. 実行方法 
(0)　ROS_MASTER_URIの設定（RasPi側でPublishされた内容をノートPC側でSubscribeするための設定）
　ノートPC側でUbuntuを起動(WindowsはWSL、詳細は後述)
~/.bashrcを編集
export ROS_MASTER_URI=http://(RasPiのIPアドレス):11311
　を追記し、
　$source ~/.bashrc
　を実行。RasPiのIPアドレスはRasPiのターミナルで$ hostname -I を実行すればわかる
　RasPiと接続しないときは，追記した行をコメントアウトすること．
　RasPiのIPアドレスが変わったときはその都度編集

	とにかく飛ばすだけ(コントローラーの電源はいれましたか？arduinoのUSB portは電源側)
###########シミュレータを使う場合##############
Dji assistant2を起動
ドローンとPCを有線で繋ぐ
あとは下記の通り
#########################################
  ArduinoにFFT_05.41_short_y_teest_20191209.inoを書き込み
　RasPi側のターミナルで、
　$ roslaunch dji_sdk sdk.launch

　もう一つRasPiのターミナルを開いて
　$ rosrun dji_sdk_demo arduino_com.py
　これでArduinoとの通信を始める

　もう一つRasPiターミナルを開いて
　$ rosrun dji_sdk_demo drone_python.py
　これでドローンが飛び始める

(2) 飛んでる様子を描画
(1)とにかく飛ばすだけ をRasPiで実行したあと，
ノートPC上のUbuntuのターミナルで
　$ rosrun dji_sdk_demo flight_view.py
 を実行(XLaunchを起動しておくのを忘れないこと，(0)ROS_MASTER_URIの設定も)
 
(3)波形が見たい時
ArduinoにFFT_05.34_3D_test_thres_fit.inoを書き込み
　RasPiのターミナルで
　$ roscore

ノートPC側のUbuntuのターミナルで
　$ cd (ファイルを保存したい場所)
　$ rosrun dji_sdk_demo dataProcess.py
　を実行，readyが表示されるまで待機（(0)ROS＿MASTER_URIの設定を忘れないこと）

　もう一つRasPiのターミナルを開いて
　$ rosrun dji_sdk_demo wave_get.py
　Yを入力してEnterで超音波発射、波形取得　
　dataProcess.pyを実行したディレクトリにExcelファイルが保存される

7.　RasPiからノートPCにデータ転送
　ノートPCのターミナルで
　$ scp (RasPiのユーザー名)@(RasPiのホストネーム).local:(コピー元のファイルの場所)  (コピー先のファイルの場所)
例：
ユーザー名　ubuntu
ホストネーム　ubuntu-desktop
のRasPiの~/Test にあるtest.txtを ノートPCの~にコピーしたい時は
$ scp ubuntu@ubuntu-desktop.local:/home/ubuntu/Test/test.txt ~
を実行

８．git を使ったファイル同期
	githubへのアップロード
ターミナルで以下を順に実行
$ cd ~/BatDrone
$ git add .
$ git commit -m “適当な文字列” (Ex. git commit -m “20200313”)
$ git push
	githubからダウンロード
ターミナルで以下を順に実行
$ cd ~/BatDrone
$ git pull
(1),(2)ともに，ユーザー名とパスワードを聞かれたら
ユーザー名：yasufumiyamada
パスワード：k1kug@sh1r@
を入力
 

Windowsの設定
	WSLのセットアップ
https://www.atmarkit.co.jp/ait/articles/1608/08/news039.html
を参考にWSLを有効化してUbuntuをインストール
Ubuntuのターミナルで
$ sudo apt update
$ sudo apt upgrade
$ echo “DISPLAY=:0” >> ~/.bashrc
を実行($は入力しない)
RasPiの設定の手順2~4をUbuntuで実行

	VcXsrvのインストール（グラフィック描画用）
https://sourceforge.net/projects/vcxsrv/
からVxXsrvをインストール
描画する必要があるとき(pythonのmatplotlib)に，XLaunchを起動する（設定は全部そのままでOK）

	ArduinoIDEのインストール
https://www.arduino.cc/en/main/OldSoftwareReleases
からver1.6.11のWindowsInstallerでインストール
C:\ProgramFiles(x86)\Arduino\libraries\Servo\src\sam\
の中のServoTimers.hの中身の
#define _useTimer1
をコメントアウトする，また
typedef enum { _timer1, _timer2, _timer3, _timer4, _timer5, _Nbr_16timers } timer16_Sequence_t;
を
typedef enum { _timer2, _timer3, _timer4, _timer5, _timer1, _Nbr_16timers } timer16_Sequence_t;
に書き換え，保存

ArduinoIDEを開き，ツール→ボード→ボードマネージャーをクリック
dueで検索，インストール
スケッチ→ライブラリをインクルード→ライブラリを管理をクリック
arduinoFFTで検索，インストール

	WSLのUbuntuとWindowsのファイル共有
Ubuntuの/mnt/c がWindowsのCドライブ直下と対応している
 
プログラム解説
ROSについては
http://robot.isc.chubu.ac.jp/?p=1296　
https://kazuyamashi.github.io/ros_lecture/ros_study_py.html　
https://raspimouse-sim-tutorial.gitbook.io/project/ros_tutorial/how_to_write_service 
http://wiki.ros.org/ja/ROS/Tutorials
あたりを参考に

ドローンと通信するためのトピック名，サービス名，msgの型などは　
http://wiki.ros.org/dji_sdk　を参照

drone_python.py：ドローンを飛ばすプログラム
1行目：rosrunで実行するために必要
3~7行：モジュールのインポート
9~13行：使用するメッセージのインポート
15~165行：Droneクラスの定義
　　17~36行：コンストラクタの定義
　　　　26~30行：Subscriberの定義　上から順にドローンの姿勢，座標，回避方向を受け取るSubscriber．引数は順に，トピック名，msgの型，コールバック関数
　　　　32~36行：Publisherの定義．上から順にドローン制御信号，ドローンの情報を送信するPublisher．引数は順に，トピック名，msgの型，キューサイズ
　　38~49行：ドローンとの通信を許可するためのServiceを呼ぶ関数
　　51~59行：ドローンの現在位置を原点に設定するServiceを呼ぶ関数
　　61~70行：ドローンの姿勢を受け取ったときに実行されるコールバック関数
　　　　63行：クォータニオンからオイラー角に変換してメンバ変数に代入
　　　　64~68行：はじめに向いていた方向をヨー角の0度にするために初期方位を保存
	ENU座標からはじめに向いていた方向基準の座標にするための回転行列を定義
　　　　69行：ヨー角をはじめに向いていた方向基準の角度に変換
　　72~77行：ドローンの座標を受け取ったときに実行されるコールバック関数
　　　　73~74行：初期方位が定義されるまで何もしない
　　　　76行：ドローンのはじめに向いていた方向基準の座標に変換してメンバ変数に代入
　　79~81行：クォータニオンからオイラー角に変換する関数
　　83~91行：ドローンを離陸させるServiceを呼ぶ関数
　　93~104行：ドローンの飛行を開始する関数
　　　　94~98行：ドローンを離陸させる
　　　　99行：離陸し終わるまで待機
　　　　100行：ドローンに飛ばす命令を出す関数(flyCallback)を50Hzでバックグラウンドで実行
　　　　102行：ドローンの情報をPublishする関数(droneInfoCallback)を50Hzでバックグランドで実行
　　106~112行：ドローンの情報（座標，方位）をPublishする関数
　　114~120行：角度を-π~πの範囲に変換する関数
　　122~154行：ドローンを飛ばす命令を出す関数
　　　　138行：飛行モードを決める（参照：http://wiki.ros.org/dji_sdk　の2.1.5 Details on flight control setpoint）水平・垂直方向の移動方法，ヨー角回転の指定方法などを16進数で指定し，それらをビット演算のORをとる
　　　　140~142行：角速度の指定，ここでは目標方位と現在の方位の差に比例するように与える
　　　　145行：角速度に応じた飛行速度を与える
　　　　147~149行：高度が1mを超えるまではその場で上昇させる
　　　　151~153行：ドローンに与える命令の形にする．要素数5のlistの形で与える．0~2番はx,y,zの速度(もしくは移動距離・ロール角・ピッチ角など，モードによって異なる)，3番はヨー角の角速（もしくは角度），4番は飛行モードの指定
　　　　154行：ドローンに命令をPublish
　　156~160行：角速度から飛行速度を計算する関数
ここではv=(v_max-v_min)exp⁡〖(-kω^2 )+v_min 〗  の形で与えている
ただし，v：飛行速度，v_max：最高速度，v_min：最低速度，k：定数，ω：角速度
　　162~164行：回避角を受け取ったときに実行されるコールバック関数

166~188行：メイン関数
　　167行：ノードを立てる
　　168行：上で定義したDroneクラスをインスタンス化
　　170~174行：ドローンと通信する許可をもらう
　　176~180行：ドローンの原点を設定
　　182行：ドローンの飛行を開始
　　187行：プログラムが勝手に終了するのを防ぐ（多分　while True: continue　と同じ？）

以下drone_python.pyの中身
#!/usr/bin/env python

import rospy
import tf
import numpy as np
import time
import struct

from dji_sdk.srv import SDKControlAuthority, SetLocalPosRef, DroneTaskControl
from geometry_msgs.msg import QuaternionStamped, Quaternion, PointStamped
from sensor_msgs.msg import Joy
from dji_sdk_demo.msg import DroneInfo
from std_msgs.msg import Int16

class Drone:

    def __init__(self):
        self.start_yaw = None
        self.rotation_matrix = None
        self.target_yaw = 0.0

        self.pos = np.array([0.0, 0.0, 0.0])
        self.attitude = Quaternion()
        self.RPY = np.array([0.0, 0.0, 0.0])

        rospy.Subscriber("/dji_sdk/attitude", QuaternionStamped, self.attitudeCallback)
        rospy.Subscriber("/dji_sdk/local_position", PointStamped, self.localPositionCallback)
        rospy.Subscriber("/avoid_direc", Int16, self.AvoidDirecCallback)

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
        self.RPY = self.quat2euler(self.attitude)
        if self.start_yaw is None:
            yaw = self.RPY[2]
            self.start_yaw = self.RPY[2]
            self.rotation_matrix = np.array([[np.cos(yaw), -np.sin(yaw), 0],[np.sin(yaw), np.cos(yaw), 0], [0, 0, 1]])
        self.RPY[2] = self.transAngleRange(self.RPY[2] - self.start_yaw)
        # print("RPY = ", np.rad2deg(self.RPY))

    def localPositionCallback(self, data):
        if self.start_yaw is None:
            return
        pos = np.array([data.point.x, data.point.y, data.point.z])
        self.pos = np.dot(self.rotation_matrix, pos)
        print("pos = ", self.pos)

    def quat2euler(self, q):
        return np.array(tf.transformations.euler_from_quaternion((q.x, q.y, q.z, q.w)))

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
        time.sleep(5)
        self.fly_timer = rospy.Timer(rospy.Duration(1.0 / 50.0), self.flyCallback)
        self.drone_info_timer = rospy.Timer(rospy.Duration(1.0 / 50.0), self.droneInfoCallback)
        return True

    def droneInfoCallback(self, event):
        drone_info_msg = DroneInfo()
        drone_info_msg.pos.x = self.pos[0]
        drone_info_msg.pos.y = self.pos[1]
        drone_info_msg.pos.z = self.pos[2]
        drone_info_msg.yaw = self.RPY[2]
        self.drone_info_pub.publish(drone_info_msg)

    def transAngleRange(self, angle):
        ans = angle
        if ans < -np.pi:
            ans += 2 * np.pi
        elif ans > np.pi:
            ans -= 2 * np.pi
        return ans

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

        delta_yaw = self.transAngleRange(self.target_yaw - self.RPY[2])
        yaw_rate_gain = 0.9
        yaw_rate = yaw_rate_gain * delta_yaw
        # yaw_rate = np.pi/12

        speed = self.calcFlyingSpeed(yaw_rate)
        v_z = 0.0
        if self.pos[2] < 1.0:
            speed = 0.0
            v_z = 0.5

        ctrl_data = [speed, 0.0, v_z, yaw_rate, mode] #[0]~[2]:x, y, z velocity, [3]: yaw_rate, [4]: controll mode
        msg.axes = ctrl_data
        self.ctrl_pub.publish(msg)

    def calcFlyingSpeed(self, yaw_rate):
        v_max = 0.3
        k = 1.0
        v_min = 0.0
        return (v_max - v_min) * np.exp(-k * yaw_rate ** 2) + v_min

    def AvoidDirecCallback(self, msg):
        self.target_yaw = self.transAngleRange(np.deg2rad(msg.data) + self.RPY[2])

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
  
arduino_com.py：Arduinoと通信して，障害物の距離・方位から回避角を算出し，Publishするプログラム
Arduinoは1度センシングするごとに定位できた距離と方位をスペース区切りで吐き出しているものとする

1行：rosrunで実行するために必要
2~6行：モジュール，msgのインポート
8~42行：メイン関数
　　9行：ノードを立てる
　　10行：障害物の情報(数，距離，方位)をPublishするPublisherの生成
　　11行：回避方向をPublishするPublisherの生成
　　12行：障害物の情報を入れるmsgの生成
　　13~14行：回避角計算のための定数の定義
　　15行：Arduinoと通信を開始

#!/usr/bin/env python
import rospy
from dji_sdk_demo.msg import ObsInfo
import serial
import numpy as np
from std_msgs.msg import Int16

def main():
    rospy.init_node('arduino_com')
    obspub = rospy.Publisher("obs_info", ObsInfo, queue_size=10)
    avoidpub = rospy.Publisher("avoid_direc", Int16, queue_size=10)
    obsmsg = ObsInfo()
    k = 0.6
    beta = 1.6
    with serial.Serial('/dev/ttyACM0', 115200, timeout=None) as ser:
        flag = False
        print("start")
        while not rospy.is_shutdown():
            line = ser.readline()
            print(line.split())
            obsmsg.obs_num = 1
            if (len(line.split()) == 2 and flag):
                if line.split()[1] == 'nan':
                    obsmsg.obs_num=0
                    obsmsg.obs_dist.data = []
                    obsmsg.obs_direc.data = []
                    obspub.publish(obsmsg)
                    continue
                r, theta = [int(float(s))  if float(s) == float(s) else 0 for s in line.split()]
                obsmsg.obs_dist.data = [r]
                obsmsg.obs_direc.data = [theta]
                obspub.publish(obsmsg)
                r = r * 10 ** -3
                theta = np.deg2rad(theta)
                force = beta / r * np.sin(np.arctan2(k, r)) if r != 0 else 0
                force_vec = force * np.array([np.sin(theta), np.cos(theta)])
                # print(-force_vec)
                avoid_direc = np.arctan2(-force_vec[0], 1-force_vec[1])
                avoidpub.publish(int(np.rad2deg(-avoid_direc)))
                # print(- np.rad2deg(avoid_direc))
            flag = True

if __name__ == '__main__':
    main()

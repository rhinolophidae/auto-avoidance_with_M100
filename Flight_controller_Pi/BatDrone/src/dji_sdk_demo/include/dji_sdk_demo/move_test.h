#ifndef MOVE_TEST_H
#define MOVE_TEST_H

// ROS include
#include <ros/ros.h>
#include <geometry_msgs/QuaternionStamped.h>
#include <geometry_msgs/Vector3Stamped.h>
#include <sensor_msgs/NavSatFix.h>
#include <std_msgs/UInt8.h>
#include <std_srvs/Trigger.h>
#include <std_msgs/Int16.h>

// DJI SDK includes
#include <dji_sdk/DroneTaskControl.h>
#include <dji_sdk/SDKControlAuthority.h>
#include <dji_sdk/QueryDroneVersion.h>
#include <dji_sdk/SetLocalPosRef.h>

#include <tf/tf.h>
#include <sensor_msgs/Joy.h>
#include <cmath>


#define C_EARTH (double)6378137.0
#define C_PI (double)3.141592653589793
#define DEG2RAD(DEG) ((DEG) * ((C_PI) / (180.0)))
#define RAD2DEG(RAD) ((RAD) / ((C_PI) / (180.0)))

#define A 0.3
#define B 1.0
#define C 0.0

#define obs_pos_x 4.0
#define obs_pos_y 0.5


extern ros::ServiceClient sdk_ctrl_authority_service;
extern ros::ServiceClient drone_task_service;
extern ros::ServiceClient set_local_pos_reference;

extern ros::Publisher ctrlPub;
extern geometry_msgs::Vector3 obs_pos;

class Drone
{
    public:

    geometry_msgs::Point start_ENU_pos;
    geometry_msgs::Point current_pos;
    geometry_msgs::Point ENU_local_pos;

    sensor_msgs::NavSatFix current_gps;
    geometry_msgs::Vector3 current_RPY;

    geometry_msgs::Vector3 global_goal_pos;
    geometry_msgs::Vector3 local_goal_pos;
    geometry_msgs::Vector3 goal_direc;

    uint8_t flight_status;

    float start_yaw;
    float current_yaw;
    
    geometry_msgs::Vector3 target_pos;
    float target_yaw;
    float delta_yaw;

    bool detect_obs;
    int avoid_direc;
    int state;
    
    int stop_counter;
    int turn_counter;

    Drone() : state(0), flight_status(255), stop_counter(0), turn_counter(0)
    {
    }

    bool obtain_control(uint8_t control_enable);
    bool takeoff();
    bool landing();
    void sensing();
    void sensing_test();
    void go_ahead(double x_velocity);
    void stop();
    void turn();
    void fly();
};

class Obstacle
{
    private:
        geometry_msgs::Vector3 pos;

    public:
        Obstacle(float pos_x, float pos_y)
        {
            pos.x = pos_x;
            pos.y = pos_y;
        }

        float get_dist(Drone *drone);
        float get_direc(Drone *drone);
};

extern Drone drone;
extern Obstacle obs;

void set_subscriber(ros::NodeHandle& nh);

geometry_msgs::Vector3 toEulerAngle(geometry_msgs::Quaternion quat);
void attitude_callback(const geometry_msgs::QuaternionStamped::ConstPtr& msg);
void gps_callback(const sensor_msgs::NavSatFix::ConstPtr& msg);
void flight_status_callback(const std_msgs::UInt8::ConstPtr& msg);
void local_position_callback(const geometry_msgs::PointStamped::ConstPtr& msg);
void avoid_direc_callback(const std_msgs::Int16::ConstPtr& msg);
bool set_local_position();

float calc_angle_diff(float angle_diff);


#endif

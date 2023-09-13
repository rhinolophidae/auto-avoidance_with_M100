#include "dji_sdk_demo/demo_flight_control.h"
#include "dji_sdk_demo/move_test.h"
#include "dji_sdk/dji_sdk.h"

#include <std_srvs/Trigger.h>
#include "dji_sdk_demo/DroneInfo.h"
#include <random>

const float deg2rad = C_PI/180.0;
const float rad2deg = 180.0/C_PI;


ros::ServiceClient sdk_ctrl_authority_service;
ros::ServiceClient drone_task_service;
ros::ServiceClient set_local_pos_reference;
ros::ServiceClient sensing_service;
ros::ServiceClient sensing_info_service;

ros::Publisher ctrlPub;
ros::Publisher droneInfoPub;
ros::Publisher obsInfoPub;

Drone drone;
Obstacle obs(obs_pos_x, obs_pos_y);

int main(int argc, char** argv)
{
    ros::init(argc, argv, "move_test");
    ros::NodeHandle nh;

    // Subscriber
    ros::Subscriber attitudeSub
        = nh.subscribe("dji_sdk/attitude", 10, attitude_callback);

    ros::Subscriber gpsSub
        = nh.subscribe("dji_sdk/gps_position", 10, gps_callback);

    ros::Subscriber flightStatusSub
        = nh.subscribe("dji_sdk/flight_status", 10, flight_status_callback);

    ros::Subscriber localPosition
        = nh.subscribe("dji_sdk/local_position", 10, local_position_callback);

    // Publisher
    ctrlPub = nh.advertise<sensor_msgs::Joy>
                    ("dji_sdk/flight_control_setpoint_generic", 10);
    droneInfoPub = nh.advertise<dji_sdk_demo::DroneInfo>
                    ("drone_info", 10);
    obsInfoPub = nh.advertise<dji_sdk_demo::ObsInfo>
                    ("obs_info", 10);

    // Services
    sdk_ctrl_authority_service 
        = nh.serviceClient<dji_sdk::SDKControlAuthority> 
            ("dji_sdk/sdk_control_authority");

    drone_task_service
        = nh.serviceClient<dji_sdk::DroneTaskControl>
            ("dji_sdk/drone_task_control");

    set_local_pos_reference
        = nh.serviceClient<dji_sdk::SetLocalPosRef>
            ("dji_sdk/set_local_pos_ref");

    sensing_service
        = nh.serviceClient<dji_sdk_demo::pulse>
            ("sensing");

    sensing_info_service
        = nh.serviceClient<dji_sdk_demo::sensing_data>
            ("sensing_info");


    bool obtain_control_result = drone.obtain_control(1);
    bool takeoff_result;
    ros::Time sensing_time;
  
    if (!set_local_position()) // We need this for height
    {
        ROS_ERROR("GPS health insufficient - No local frame reference for height. Exiting.");
        return 1;
    }

    takeoff_result = drone.takeoff();

    if(takeoff_result)
    {
        drone.start_ENU_pos = drone.ENU_local_pos;
        drone.start_yaw = drone.current_RPY.z;
        sensing_time = ros::Time::now();
        drone.sensing_test();
        // drone.sensing();
        // obtain_control_result = drone.obtain_control(0);
        drone.state = 1;
    }

    dji_sdk_demo::DroneInfo drone_info;
    drone_info.pos = drone.current_pos;
    drone_info.yaw = drone.current_yaw;
    droneInfoPub.publish(drone_info);

    ros::Rate loop_rate(200);
    ros::Time control_time = ros::Time::now();

    while(ros::ok() && 
          drone.flight_status 
          == DJISDK::M100FlightStatus::M100_STATUS_IN_AIR)
    {
        if(ros::Time::now() - sensing_time > ros::Duration(0.1)
            && drone.state == 1)
        {
            sensing_time = ros::Time::now();
            drone.sensing_test();
            // drone.sensing();
            ROS_INFO("avoid_direc = %d", drone.avoid_direc);
        }

        if(ros::Time::now() - control_time > ros::Duration(0.02))
        {
            control_time = ros::Time::now();
            if (drone.state == 1)
            {
                drone.fly();
            }
            else if (drone.state == 0 || drone.state == 3)
            {
                if (drone.stop_counter < 50)
                {
                    drone.stop();
                    drone.stop_counter ++;
                }
                else
                {
                    if (drone.state == 0)
                    {
                        drone.state = 2;
                    }
                    else if (drone.state == 3)
                    {
                        drone.state = 1;
                    }
                    drone.stop_counter = 0;
                }
            }
            else if (drone.state == 2)
            {
                ROS_INFO("delta_yaw = %f", drone.delta_yaw);
                if (drone.turn_counter < 20)
                {
                    drone.turn();
                    if (std::abs(drone.delta_yaw < 2.0 * deg2rad))
                    {
                        drone.turn_counter ++;
                    }
                }
                else
                {
                    drone.state = 3;
                    drone.turn_counter = 0;
                    drone.stop_counter = 20;
                }
                ROS_INFO("turn_counter = %d", drone.turn_counter);
            }
            drone_info.pos = drone.current_pos;
            drone_info.yaw = drone.current_yaw;
            droneInfoPub.publish(drone_info);
        }

        ros::spinOnce();
        loop_rate.sleep();
    }

    ROS_INFO("pos = [%f, %f, %f]",
             drone.current_pos.x,
             drone.current_pos.y,
             drone.current_pos.z);
    ROS_INFO("yaw = %f", drone.current_yaw);
  return 0;
}

#include "dji_sdk_demo/move_test.h"
#include "dji_sdk/dji_sdk.h"
#include <geometry_msgs/Vector3.h>
#include <cmath>
#include <iostream>


geometry_msgs::Vector3 toEulerAngle(geometry_msgs::Quaternion quat)
{
    geometry_msgs::Vector3 ans;
    tf::Matrix3x3 R_FLU2ENU(tf::Quaternion(quat.x, quat.y, quat.z, quat.w));
    R_FLU2ENU.getRPY(ans.x, ans.y, ans.z);
    return ans;
}


bool Drone::obtain_control(uint8_t control_enable)
{
    dji_sdk::SDKControlAuthority authority;
    authority.request.control_enable = control_enable;
    sdk_ctrl_authority_service.call(authority);

    if(!authority.response.result)
    {
        ROS_ERROR("obtain control failed!");
        return false;
    }
    ROS_INFO("obtain control successful!");
    return true;
}

bool Drone::takeoff()
{
    ROS_INFO("TakeOff...");
   
    ros::Time start_time = ros::Time::now();
    float home_altitude = current_gps.altitude;
    
    dji_sdk::DroneTaskControl droneTaskControl;
   
    droneTaskControl.request.task
     = dji_sdk::DroneTaskControl::Request::TASK_TAKEOFF;
    
    drone_task_service.call(droneTaskControl);
    if(!droneTaskControl.response.result)
    {
        ROS_ERROR("TakeOff failed!");
        return false;
    }


    ros::Duration(0.01).sleep();
    ros::spinOnce();

    while(ros::Time::now() - start_time < ros::Duration(10))
    {
        ros::Duration(0.01).sleep();
        ros::spinOnce();
    }

    if(flight_status != DJISDK::M100FlightStatus::M100_STATUS_IN_AIR ||
                        current_gps.altitude - home_altitude < 1.0)
    {
        ROS_ERROR("Takeoff failed.");
        return false;
    }
    start_time = ros::Time::now();
    ROS_INFO("Successful takeoff!");

    ros::spinOnce();
    
    return true;
}

bool Drone::landing()
{
    ROS_INFO("landing...");

    dji_sdk::DroneTaskControl droneTaskControl;
   
    droneTaskControl.request.task
     = dji_sdk::DroneTaskControl::Request::TASK_LAND;
    
    drone_task_service.call(droneTaskControl);
    if(!droneTaskControl.response.result)
    {
        ROS_ERROR("TakeOff failed!");
        return false;
    }
    
    ROS_INFO("Succesfull land!");
}

// void Drone::sensing()
// {
//     dji_sdk_demo::pulse trig;
//     if(sensing_service.call(trig))
//     {
//         avoid_direc = trig.response.avoid_direc;
//
//         std::cout << "avoid_direc = " << avoid_direc << std::endl;
//         if (std::abs(avoid_direc) < 45)
//         {
//             state = 1;
//         }
//         else
//         {
//             state = 0;
//         }
//
//         dji_sdk_demo::sensing_data msg;
//         if(sensing_info_service.call(msg) &&
//                (msg.response.Echo_Num != 0))
//         {
//             dji_sdk_demo::ObsInfo obs_info;
//             obs_info.obs_num = msg.response.Echo_Num;
//             obs_info.obs_dist.data.resize(obs_info.obs_num);
//             obs_info.obs_direc.data.resize(obs_info.obs_num);
//             for(int i = 0; i < obs_info.obs_num; i++)
//             {
//                 // obs_info.obs_dist.data[i] = msg.response.Det_dis.data[i];
//                 // obs_info.obs_direc.data[i] = msg.response.Det_direc_holi.data[i];
//                 obs_info.obs_dist.data[i] = 500 * i;
//                 obs_info.obs_direc.data[i] = 15 * i;
//             }
//             obsInfoPub.publish(obs_info);
//         }
//     }
//     else
//     {
//         ROS_ERROR("sensing failed!");
//         state = 0;
//     }
// }
//
// void Drone::sensing_test()
// {
//     float k = 0.6;
//     float beta = 1.6;
//     float distance = obs.get_dist(this);
//     float direction = obs.get_direc(this);
//     float distant_force = 1.0 / distance;
//     float direction_force = std::sin(std::atan2(k, distance));
//     float vector_length = distant_force * direction_force * beta;
//
//     geometry_msgs::Vector3 force_vector;
//     force_vector.x = vector_length * std::sin(direction);
//     force_vector.y = vector_length * std::cos(direction);
//
//     avoid_direc = (int)(RAD2DEG(std::atan2(-force_vector.x,
//                                     1.0 - force_vector.y)));
//
//     if( (std::abs(direction) > C_PI / 2.0) || distance > 3.0 )
//     {
//         avoid_direc = 0;
//     }
//     target_yaw = calc_angle_diff(
//             current_yaw - DEG2RAD(avoid_direc));
//
//
//     std::cout << "dist = " << distance << ": direc = "<< direction << std::endl;
//     std::cout << "avoid_direc = " << avoid_direc << std::endl;
//
//     if (std::abs(avoid_direc) < 90)
//     {
//         state = 1;
//     }
//     else
//     {
//         state = 0;
//     }
// }

void Drone::go_ahead(double x_velocity)
{
    sensor_msgs::Joy control;
    uint8_t flag = (DJISDK::VERTICAL_VELOCITY   |
                    DJISDK::HORIZONTAL_VELOCITY |
                    DJISDK::YAW_RATE            |
                    DJISDK::HORIZONTAL_BODY     |
                    DJISDK::STABLE_ENABLE);
    control.axes.push_back(x_velocity);    //x_velocity
    control.axes.push_back(0);      //y_velocity
    control.axes.push_back(0);      //z_velocity
    control.axes.push_back(0);      //yaw_rate
    control.axes.push_back(flag);

    ctrlPub.publish(control);
}

void Drone::stop()
{
    sensor_msgs::Joy control;
    uint8_t flag = (DJISDK::VERTICAL_VELOCITY   |
                    DJISDK::HORIZONTAL_VELOCITY |
                    DJISDK::YAW_RATE            |
                    DJISDK::HORIZONTAL_BODY     |
                    DJISDK::STABLE_ENABLE);
    control.axes.push_back(0);
    control.axes.push_back(0);
    control.axes.push_back(0);
    control.axes.push_back(0);
    control.axes.push_back(flag);

    ctrlPub.publish(control);
}

void Drone::turn()
{
    float yaw_rate_factor = 5.0 * C_PI / 8.0;
    sensor_msgs::Joy control;
    uint8_t flag = (DJISDK::VERTICAL_VELOCITY   |
                    DJISDK::HORIZONTAL_VELOCITY |
                    DJISDK::YAW_RATE            |
                    DJISDK::HORIZONTAL_BODY     |
                    DJISDK::STABLE_ENABLE);
    delta_yaw = calc_angle_diff(target_yaw - current_yaw);
    // delta_yaw = calc_angle_diff(avoid_direc - current_yaw);

    float yaw_rate_gain = 0.9;
    float yaw_rate = yaw_rate_gain * delta_yaw;
    if (std::abs(yaw_rate) > yaw_rate_factor)
    {
        yaw_rate = (yaw_rate > 0) ? yaw_rate_factor : -yaw_rate_factor;
    }

    control.axes.push_back(0);
    control.axes.push_back(0);
    control.axes.push_back(0);
    control.axes.push_back(yaw_rate);
    control.axes.push_back(flag);

    ctrlPub.publish(control);
}

void Drone::fly()
{
    float yaw_rate_factor = 5.0 * C_PI / 8.0;
    sensor_msgs::Joy control;
    uint8_t flag = (DJISDK::VERTICAL_VELOCITY   |
                    DJISDK::HORIZONTAL_VELOCITY |
                    DJISDK::YAW_RATE            |
                    DJISDK::HORIZONTAL_BODY     |
                    DJISDK::STABLE_ENABLE);
    delta_yaw = calc_angle_diff(target_yaw - current_yaw);
    // delta_yaw = calc_angle_diff(avoid_direc - current_yaw);

    float yaw_rate;
    float yaw_rate_gain = 0.9;
    yaw_rate = yaw_rate_gain * delta_yaw;
    if (std::abs(yaw_rate) > yaw_rate_factor)
    {
        yaw_rate = (delta_yaw > 0) ? yaw_rate_factor : -yaw_rate_factor;
    }

    float velocity = A * std::exp(-B * yaw_rate * yaw_rate) + C;
//    std::cout << "velocity = " << velocity << std::endl;


    control.axes.push_back(velocity);
    control.axes.push_back(0);
    control.axes.push_back(0);
    control.axes.push_back(yaw_rate);
    control.axes.push_back(flag);

    ctrlPub.publish(control);
}


bool set_local_position()
{
  dji_sdk::SetLocalPosRef localPosReferenceSetter;
  set_local_pos_reference.call(localPosReferenceSetter);
  return localPosReferenceSetter.response.result;
}

float calc_angle_diff(float angle_diff)
{
    float ans = angle_diff;
    if (ans < -C_PI)
    {
        ans += 2.0 * C_PI;
    }
    else if (C_PI < ans)
    {
        ans -= 2.0 * C_PI;
    }
    return ans;
}

float Obstacle::get_dist(Drone *drone)
{
    float dx = pos.x - drone->current_pos.x;
    float dy = pos.y - drone->current_pos.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist;
}

float Obstacle::get_direc(Drone *drone)
{
    float dx = pos.x - drone->current_pos.x;
    float dy = pos.y - drone->current_pos.y;
    float direc = std::atan2(dy, dx);
    direc =  - calc_angle_diff(direc - drone->current_yaw);
    return direc;
}

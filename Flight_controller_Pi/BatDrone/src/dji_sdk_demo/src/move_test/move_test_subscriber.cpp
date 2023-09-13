#include "dji_sdk_demo/move_test.h"
#include "dji_sdk/dji_sdk.h"


void attitude_callback(const geometry_msgs::QuaternionStamped::ConstPtr& msg)
{
    geometry_msgs::Quaternion current_atti = msg->quaternion;
    drone.current_RPY = toEulerAngle(current_atti);
    drone.current_yaw = drone.current_RPY.z - drone.start_yaw;

    if (drone.current_yaw > C_PI)
    {
        drone.current_yaw = drone.current_yaw - 2.0 * C_PI;
    }
    else if (drone.current_yaw < -C_PI)
    {
        drone.current_yaw = 2.0 * C_PI + drone.current_yaw;
    }
//    ROS_INFO("Yaw of_ENU = %f, Yaw_of_local = %f",
//             RAD2DEG(drone.current_RPY.z), RAD2DEG(drone.current_yaw));
}

void gps_callback(const sensor_msgs::NavSatFix::ConstPtr& msg)
{
    drone.current_gps = *msg;
}

void flight_status_callback(const std_msgs::UInt8::ConstPtr& msg)
{
    drone.flight_status = msg->data;
}

void local_position_callback(const geometry_msgs::PointStamped::ConstPtr& msg)
{
    drone.ENU_local_pos = msg->point;
    geometry_msgs::Point local_pos_offset;
    local_pos_offset.x = drone.ENU_local_pos.x - drone.start_ENU_pos.x;
    local_pos_offset.y = drone.ENU_local_pos.y - drone.start_ENU_pos.y;
    local_pos_offset.z = drone.ENU_local_pos.z - drone.start_ENU_pos.z;

    drone.current_pos.x = cos(drone.start_yaw) * local_pos_offset.x
                        + sin(drone.start_yaw) * local_pos_offset.y;
    drone.current_pos.y = - sin(drone.start_yaw) * local_pos_offset.x
                        + cos(drone.start_yaw) * local_pos_offset.y;
    drone.current_pos.z = local_pos_offset.z;

    drone.local_goal_pos.x = drone.global_goal_pos.x - drone.current_pos.x;
    drone.local_goal_pos.y = drone.global_goal_pos.y - drone.current_pos.y;
    drone.local_goal_pos.z = drone.global_goal_pos.z - drone.current_pos.z;

    drone.goal_direc.x = std::cos(std::atan2(drone.local_goal_pos.y, drone.local_goal_pos.x));
    drone.goal_direc.y = std::sin(std::atan2(drone.local_goal_pos.y, drone.local_goal_pos.x));
}

void avoid_direc_callback(const std_msgs::Int16::ConstPtr& msg)
{
    if (drone.state == 1)
    {
        drone.avoid_direc = msg->data;
        drone.target_yaw = calc_angle_diff(
                drone.current_yaw - DEG2RAD(drone.avoid_direc));
        ROS_INFO("avoid_direc = %d", drone.avoid_direc);
    }
}

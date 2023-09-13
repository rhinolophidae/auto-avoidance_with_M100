#include <ros/ros.h>
#include <std_srvs/Trigger.h>
#include <random>
#include <cmath>

std::mt19937 mt(1);
std::uniform_real_distribution<float> dist(0, 1);

bool sensing(std_srvs::Trigger::Request &req,
             std_srvs::Trigger::Response &res)
{
    if (dist(mt) < 0.05)
    {
        res.success = true;
        res.message = "45";
    }
    else
    {
        res.success = false;
        res.message = "0";
    }

    int avoid_direc = (int)(dist(mt) * 45);
    res.message = std::to_string(avoid_direc);
//    std::cout << res.message << std::endl;
    return true;
}

int main(int argc, char** argv)
{
    ros::init(argc, argv, "sensing_service_server");
    ros::NodeHandle nh;

    ros::ServiceServer service = nh.advertiseService("sensing", sensing);

    ROS_INFO("Ready to sensing.");

 /*   while(ros::ok())
    {
        ros::spinOnce();
    }
    */
    ros::spin();

    return 0;
}

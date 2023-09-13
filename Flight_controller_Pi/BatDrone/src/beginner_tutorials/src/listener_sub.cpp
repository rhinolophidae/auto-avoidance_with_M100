#include "beginner_tutorials/listener.h"


void set_subscriber(ros::NodeHandle& nh)
{
    ros::Subscriber sub = nh.subscribe("chatter", 1000, chatterCallback);
}


void chatterCallback(const std_msgs::String::ConstPtr& msg)
{
    ROS_INFO("I heard: [%s]", msg->data.c_str());
}


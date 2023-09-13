#ifndef LISTENER_H
#define LISTENER_H


#include <ros/ros.h>
#include <std_msgs/String.h>


void set_subscriber(ros::NodeHandle& nh);
void chatterCallback(const std_msgs::String::ConstPtr& msg);

#endif

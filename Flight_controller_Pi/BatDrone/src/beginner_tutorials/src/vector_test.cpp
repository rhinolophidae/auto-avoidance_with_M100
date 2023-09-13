#include <ros/ros.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/Vector3.h>


geometry_msgs::Vector3 add(geometry_msgs::Vector3 a,
                           geometry_msgs::Vector3 b)
{
    geometry_msgs::Vector3 ans;
    
    ans.x
        = a.x + b.x;
    ans.y = a.y + b.y;
    ans.z = a.z + b.z;
    return ans;
}


int main(int argc, char** argv)
{
    ros::init(argc, argv, "vector_test");
    ros::NodeHandle nh;

    geometry_msgs::Vector3 u, v, ans;
    
    u.x = 1.0;
    u.y = 0.0;
    u.z = 0.0;

    v.x = 0.0;
    v.y = 1.0;
    v.z = 0.0;

    ans = add(u, v);

    /* ROS_INFO("u = [%f, %f, %f], v = [%f, %f, %f]\n
              u + v = [%f, %f, %f]",
             u.x, u.y, u.z, v.x, v.y, v.z,
             ans.x, ans.y, ans.z);
    */

    std::cout << "u = [" << u.x << ", " << u.y << ", " << u.z << "], "
              << "v = [" << v.x << ", " << v.y << ", " << v.z << "]\n"
              << "u + v = [" << ans.x << ", " << ans.y << ", " << ans.z << "]" << std::endl;
}

cmake_minimum_required(VERSION 2.8.3)
project(beginner_tutorials)


find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  geometry_msgs
  message_generation
)


add_message_files(
  FILES
  Num.msg
)

add_service_files(
  FILES
  AddTwoInts.srv
)

generate_messages(
  DEPENDENCIES
  std_msgs
  geometry_msgs
)


catkin_package()


include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

add_executable(talker src/talker.cpp)
target_link_libraries(talker ${catkin_LIBRARIES})
add_dependencies(talker beginner_tuttorials_generate_messages_cpp)

add_executable(listener src/listener.cpp
                        src/listener_sub.cpp)
target_link_libraries(listener ${catkin_LIBRARIES})
add_dependencies(listener beginner_tuttorials_generate_messages_cpp)

add_executable(vector_test src/vector_test.cpp)
target_link_libraries(vector_test ${catkin_LIBRARIES})
add_dependencies(vector_test beginner_tuttorials_generate_messages_cpp)

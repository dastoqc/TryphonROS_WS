# %Tag(FULLTEXT)%

cmake_minimum_required(VERSION 2.8.3)
project(controls)

## Find catkin and any catkin packages
find_package(catkin REQUIRED COMPONENTS geometry_msgs roscpp rospy std_msgs sensors state dynamic_reconfigure message_generation 
cmake_modules
)
find_package(Eigen REQUIRED)

## Declare ROS messages, services and actions
add_message_files(DIRECTORY msg FILES Commands.msg State.msg)
#add_service_files(FILES AddTwoInts.srv)

## Generate added messages and services
generate_messages(DEPENDENCIES std_msgs geometry_msgs sensor_msgs)

generate_dynamic_reconfigure_options(
  cfg/control.cfg
  cfg/trajectory.cfg
  cfg/fdistribution.cfg
  cfg/glideslope.cfg
  cfg/formation.cfg
  cfg/dock_control.cfg
  cfg/magnet.cfg
)
## Declare a catkin package
catkin_package( INCLUDE_DIRS include
# Libraries
CATKIN_DEPENDS  sensor_msgs std_msgs geometry_msgs sensors state message_runtime
DEPENDS eigen)

##Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories( ${catkin_INCLUDE_DIRS})


## Build talker and listener
include_directories(include ${catkin_INCLUDE_DIRS}
  ${EIGEN_INCLUDE_DIRS})


add_executable(control src/control.cpp)
target_link_libraries(control ${state_LIBRARIES} ${catkin_LIBRARIES})
add_dependencies(control sensors_gencpp std_msgs geometry_msgs sensors state)

add_executable(controlxzlc src/controlxzlc.cpp)
target_link_libraries(controlxzlc ${state_LIBRARIES} ${catkin_LIBRARIES})
add_dependencies(controlxzlc sensors_gencpp std_msgs geometry_msgs sensors state)

add_executable(ar_convert src/ar_convert.cpp)
target_link_libraries(ar_convert ${state_LIBRARIES} ${catkin_LIBRARIES})
add_dependencies(ar_convert sensors_gencpp std_msgs geometry_msgs sensors state)

add_executable(trajectory src/trajectory.cpp)
target_link_libraries(trajectory ${catkin_LIBRARIES})
add_dependencies(trajectory sensors_gencpp std_msgs geometry_msgs sensors state)

add_executable(ctr_fuzzy src/ctr_fuzzy.cpp)
target_link_libraries(ctr_fuzzy ${catkin_LIBRARIES})
add_dependencies(ctr_fuzzy sensors_gencpp std_msgs geometry_msgs sensors state)


add_executable(glideslope src/glideslope.cpp)
target_link_libraries(glideslope ${catkin_LIBRARIES})
add_dependencies(glideslope sensors_gencpp std_msgs geometry_msgs sensors)

add_executable(formation src/formation.cpp)
target_link_libraries(formation ${catkin_LIBRARIES})
add_dependencies(formation sensors_gencpp std_msgs geometry_msgs sensors)

add_executable(glideslope_test src/glideslope_test.cpp)
target_link_libraries(glideslope_test ${catkin_LIBRARIES})
add_dependencies(glideslope_test sensors_gencpp std_msgs geometry_msgs sensors)


add_executable(force_distribution src/force_distribution.cpp)
target_link_libraries(force_distribution ${catkin_LIBRARIES})
add_dependencies(force_distribution sensors_gencpp std_msgs geometry_msgs sensors state)

add_executable(dock_control src/dock_control.cpp)
target_link_libraries(dock_control ${state_LIBRARIES} ${catkin_LIBRARIES})
add_dependencies(dock_control sensors_gencpp std_msgs geometry_msgs sensors state)

add_executable(dock_control_target src/dock_control_target.cpp)
target_link_libraries(dock_control_target ${state_LIBRARIES} ${catkin_LIBRARIES})
add_dependencies(dock_control_target sensors_gencpp std_msgs geometry_msgs sensors state)

add_executable(magnet src/magnet.cpp)
target_link_libraries(magnet ${catkin_LIBRARIES})
add_dependencies(magnet sensors_gencpp std_msgs)

#%EndTag(FULLTEXT)%


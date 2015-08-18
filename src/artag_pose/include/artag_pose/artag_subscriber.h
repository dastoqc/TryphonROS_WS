#ifndef ARTAGSUBSCRIBER_H
#define ARTAGSUBSCRIBER_H


#include <string>
#include <vector>

#include "ros/ros.h"
#include <tf/transform_listener.h>

#include <Eigen/Geometry>

#include <geometry_msgs/Transform.h>
#include <geometry_msgs/Pose.h>
#include <ar_track_alvar/AlvarMarkers.h>

#include "artag_pose/markers_pose.h"

typedef std::vector<ar_track_alvar::AlvarMarker> TrackedMarker;

class ArtagSubscriber{

ros::Subscriber sub;

std::string cameraName, topicName;
MarkersPosePtr markers;

bool receiveIsFirstMsg;
ros::Timer timer;
ros::Time lastReception;


tf::StampedTransform cameraTf;
ar_track_alvar::AlvarMarkers oldMsg;
unsigned int emptyCount;

// Configuration
double jmpThreshold;

Eigen::Vector3d avgPose;
bool msgReceiveSincePull;

public:
	ArtagSubscriber(const std::string& camera_name,
					const std::string& topic_name,
					MarkersPosePtr markers,
					ros::NodeHandle & nh);

	void artagCallback(const ar_track_alvar::AlvarMarkers::ConstPtr& msg);
	void timerCallback(const ros::TimerEvent& event);

	bool wasMsgReceiveSinceLastPull();
	Eigen::Vector3d pullAveragePose();
private:
	void lookupCameraTf();
	TrackedMarker::iterator  findMarkerInOldMsgById(unsigned int id);
	double distanceBetweenPoint(geometry_msgs::Point A,
								geometry_msgs::Point B);
};

#endif // ARTAGSUBSCRIBER_H

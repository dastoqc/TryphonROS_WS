#include "artag_pose/artag_pose_node.h"



ArtagPoseNode::ArtagPoseNode():nodeHandle("~"){

	nodeHandle.param<int>("hz", frequency, 10);

	nodeHandle.param<int>("numMarkers", numMarkers, 1);

	ROS_INFO("Using TF for markers pose");
	markerConfig = new TfConfigLoader;
}

ArtagPoseNode::~ArtagPoseNode(){
	artagSubs.clear();
	delete markerConfig;
}


void ArtagPoseNode::start(){
	loadConfig();
	createPublishers();
	createSubscribers();
	loop();
}
void ArtagPoseNode::loadConfig(){
	markerConfig->load(numMarkers);
	markersPose = markerConfig->parse();
}

void ArtagPoseNode::createPublishers(){
	pubPose = nodeHandle.advertise<geometry_msgs::PoseStamped>("multitag_pose", 1000);
}

void ArtagPoseNode::createSubscribers(){
	// Get topic list from parameter
	//std::vector<std::string> camera_topics = loadCameraTopics();

	// Bypass the Yaml configuration for debugging
	std::vector<std::string> camera_topics;
	camera_topics.push_back("camera3");
	camera_topics.push_back("/192_168_10_243/artags1/artag1/ar_pose_marker");
	camera_topics.push_back("camera4");
	camera_topics.push_back("/192_168_10_243/artags2/artag1/ar_pose_marker");
	/*camera_topics.push_back("camera3");
	camera_topics.push_back("/192_168_10_243/artags/artag3/ar_pose_marker");*/

	// for each camera topics create a subscriber
	std::vector<std::string>::iterator topic_name;
	for(topic_name = camera_topics.begin(); topic_name != camera_topics.end(); topic_name++){
		std::string camera_name = *topic_name;
		// The following element is the topic name
		topic_name++;
		ArtagSubPtr ptr(new ArtagSubscriber(camera_name, *topic_name, markersPose, nodeHandle));
		artagSubs.push_back(ptr);
	}
}


std::vector<std::string> ArtagPoseNode::loadCameraTopics(){
	std::vector<std::string> camera_topics;

	bool parameter_provided = nodeHandle.getParam("camera_topics", camera_topics);

	if(!parameter_provided){
		ROS_ERROR("Can't load list of camera's topic from parameter \"camera_topics\"");
		ros::shutdown();
	}

	return camera_topics;
}

void ArtagPoseNode::loop(){
	ros::Rate loop_rate(frequency);
	while(ros::ok()){
		computePoseAndPublish();

		ros::spinOnce();
		loop_rate.sleep();
	}
}

void ArtagPoseNode::computePoseAndPublish(){
	std::vector<ArtagSubPtr>::iterator it;
	geometry_msgs::PoseStamped msg;
	msg.header.frame_id = "cafeteria";


	std::list<tagHandle> tagsDetected;

	for(it = artagSubs.begin(); it != artagSubs.end(); ++it){
		(*it)->pullTagDetected(tagsDetected);
	}

	if(tagsDetected.empty())
		return;

	calculateWeight(tagsDetected);


	Eigen::Affine3d pose = doWeightAverage(tagsDetected);
	// Eigen -> Ros msg
	tf::poseEigenToMsg(pose, msg.pose);

	// Publish pose
	pubPose.publish(msg);

	// Broadcast tf
	tf::Pose poseTf;
	tf::poseEigenToTF(pose, poseTf);
	/*br.sendTransform(
	            tf::StampedTransform(camToTagBroadcast,
	                                      ros::Time::now(),
	                                      "cafeteria",
	                                      "cube_estimate")
	            );*/
}

void ArtagPoseNode::calculateWeight(std::list<tagHandle>& tags){

	std::list<tagHandle>::iterator tag;
	std::cout << std::endl;
	for(tag = tags.begin(); tag != tags.end(); ++tag)
	{
		Eigen::Vector3d trans = tag->camPose.translation().matrix();
		double norm = trans.norm();

		// The yaw is extracted by rotating a unit vector and doing the atan2 of two of his axis
		Eigen::Vector3d vrot = tag->camPose.linear() * Eigen::Vector3d::UnitX();
		double artagYaw = atan2(vrot(2), vrot(0));

		// Calculation of the quaternion norm
		double angle, x, y, z;
		Eigen::Quaterniond q;
		q =  tag->camPose.linear();
		angle = 2 * acos(q.w());
		x = q.x() / sqrt(1 - q.w() * q.w());
		y = q.y() / sqrt(1 - q.w() * q.w());
		z = q.z() / sqrt(1 - q.w() * q.w());
		double qNorm = sqrt(x*x + y*y + z*z);

		/*
		std::cout << "Tag_" << tag->idTag << std::endl;
		ROS_INFO_STREAM("Norm: " << norm);
		ROS_INFO_STREAM("Yaw: " << artagYaw* 180.0 / M_PI);
		ROS_INFO_STREAM("Quat norm: " << qNorm);*/

		tag->weight = 1.0;
	}
}

Eigen::Affine3d ArtagPoseNode::doWeightAverage(const std::list<tagHandle>& tags){

	// Weight sum
	std::list<tagHandle>::const_iterator tag;
	double totalWeight = 0.0;
	for(tag = tags.begin(); tag != tags.end(); ++tag){
		totalWeight += tag->weight;
	}

	// Edge case where weight is zero
	if(totalWeight < 0.0001){
		ROS_ERROR("Total weight should be greater than zero");
		ros::shutdown();
	}

	// Calculation of the average pose baise on the multiple pose weight
	Eigen::Vector3d position;
	Eigen::Quaterniond rot, q;
	rot = tags.front().globalPose.linear().matrix();

	for(tag = tags.begin(); tag != tags.end(); ++tag){
		double normalizeWeight = tag->weight / totalWeight;

		position += tag->globalPose.translation() * normalizeWeight;

		// The averaging of the quaternion is based on Unity average function:
		// http://wiki.unity3d.com/index.php/Averaging_Quaternions_and_Vectors
		q = tag->globalPose.linear().matrix();

		// We need to check if the quaternion is inverted by doing a dot product
		// If the dot product is negative we reverse the quaternion
		if(rot.dot(q) < 0.0){
			q = Eigen::Quaterniond(-q.w(), -q.x(), -q.y(), -q.z());
		}

		rot.w() += q.w() * normalizeWeight;
		rot.x() += q.x() * normalizeWeight;
		rot.y() += q.y() * normalizeWeight;
		rot.z() += q.z() * normalizeWeight;
	}

	// The resulting quaternion need to be unitary
	rot.normalize();


	Eigen::Affine3d pose;
	pose.translation() = position;
	pose.linear() = rot.matrix();

	return pose;
}



int main(int argc, char **argv){

	/*
	 *			==========		General draft of how the node works		=========
	 *										  ***
	 * A) Take the position of an array of artag from an origin from a YAML File or a TF
	 *		1) ConfigLoader virtual class with two implementation YAMLConfigLoader and TFConfigLoader
	 *		2) Load and Parse
	 *			-> Parse return a object that will be pass to the subscriber
	 *				-> struct{ artag_id, pose: x, y, z}
	 *				-> Or map<int, geometry_msgs::pose>
	 *				-> Or the clean code interface of a map< ... > with the name MarkersPose
	 *					->
	 * B) After the configuration is done, it is send to another module which is the subscriber
	 *	  to the artag_track node output
	 * C) Since we might have multiple camera the best option is to have one instance of
	 *	  the subscriber per camera
	 *		1) Either we go throught all the topic to find the one publishing artag
	 *		2) Or we hardcode a list of those topics into a parameter field like this example:
	 *			http://answers.ros.org/question/12321/how-to-write-and-parse-yaml-file-for-ros/
	 *		3) Add it to the ConfigLoader
	 * D) The subscriber:
	 *		1) First off have a timeout to print a warning in the case of artag_track crash
	 *		2) Check for invalid tag, print WARN in consequence; Samething for no tag
	 *		3) If no timeout, compare the last tag to see if we look at new tags
	 *		4a) If old tag are found, check if the position did move more from certain threshold and warn
	 *		4b) If new tag, we could add warn or maybe a security to prevent a jump?
	 *		5) Do the reverse transformation from the camera relative to global position
	 *		6) Get the average pose and print the average deviation (maybe add warning when too large)
	 * E) Publisher: Pose Publisher that handle the multiple state that the subcriber can be in.
	 *		1) Timeout on all sub
	 *			-> No publishing of a pose WARN
	 *		2) Valid pose on all sub
	 *			-> Average of their average
	 *			-> or Average of their individual tag pose
	 *			-> Check for deviation or too big of jump t-1
	 *				-> ignore for t0 or timeout
	 *			-> Publish
	 *		3) Valid pose on some sub
	 *			-> Same than 2), except add some smoothing aka average from the previous pose to prevent jump
	 * F) Maybe use the confidence parameter from the ar_track_alvar message
	 *
	 */

	ros::init(argc, argv, "artag_pose_node");

	ArtagPoseNode mw;

	mw.start();

	return 0;
}
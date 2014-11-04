// Standard C/C++ libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctime>

//library for ros
#include <ros/ros.h>
#include "geometry_msgs/Wrench.h"
#include "geometry_msgs/Pose.h"
#include "geometry_msgs/PoseStamped.h"

//library for Android msg
#include <sensor_msgs/Imu.h>

//libraries for the sonar
#include "sensors/sonar.h"
#include "sensors/sonarArray.h"
#include "sensors/compass.h"
#include "sensors/imuros.h"
#include "sensors/imubuff.h"
#include "state/state.h"

//libraries for the array
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Int32MultiArray.h"

//libraries for the control
#include "std_msgs/String.h"
#include "geometry_msgs/Pose.h"
#include <sstream>

#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

//#include "i2c-dev.h"
//#include "motors.h"
#include "sensors/motor.h"
#include "sensors/motorArray.h"
//#include "cube.h"       // Cube geometry, inertia, etc.

bool start=true;
double dsz1=0;
double dsz2=0;
double dsz3=0;
double dsz4=0;
double dszt[5]={0,0,0,0,0};
double dsztf[5]={0,0,0,0,0};
//double dszt=0;
//double dszto=0;
double dsx1=0,dsx2=0;
double dsy1=0,dsy2=0;
double rz1=0,rz2=0;
double rz0=0;
double rzold=0;
double ax=0;
double ay=0;
double az=0;
double gx=0;
double gy=0;
double gz=0;
double mx=0;
double my=0;
double mz=0;


double hmax=6000;

void subSonar(const sensors::sonarArray::ConstPtr& msg)
{
    dsz1=0;
    dsz2=0;
    dsz3=0;
    dsz4=0;
    dsx1=0;
    dsy1=0;
    int okdsz=0;
    for(int i=0; i<4;i++){dszt[i]=dszt[i+1];
	dsztf[i]=dsztf[i+1];}
    //dszto=dszt;
    for (int i=0; i<msg->sonars.size(); ++i)
    {
        const sensors::sonar &sonar = msg->sonars[i];
        //ROS_INFO_STREAM("ID: " << sonar.id << " - D0: " << sonar.distance[0] <<
        //	               ", D1: " << sonar.distance[1]);
        
        // Average but not dividing by ten to convert cm into mm
        if (sonar.id == 112){
            for (int j=0;j<10;j++)
            {dsx1=dsx1+sonar.distance[j];}
        }
        if (sonar.id == 115){
            for (int j=0;j<10;j++)
            {dsy1=dsy1+sonar.distance[j];}
        }
        if (sonar.id == 124){
            for (int j=0;j<10;j++)
            {dsz3=dsz3+sonar.distance[j];}
	    if(dsz3>hmax){dsz3=0;}
	    else {++okdsz;}}
        
        if (sonar.id == 125){
            for (int j=0;j<10;j++)
            {dsz4=dsz4+sonar.distance[j];}
            if(dsz4>hmax){dsz4=0;} 
            else {++okdsz;}}
        
        if (sonar.id == 126){
            for (int j=0;j<10;j++)
            {dsz1=dsz1+sonar.distance[j];}
            if(dsz1>hmax){dsz1=0;} 
            else {++okdsz;}}
        
        if (sonar.id == 127){
            for (int j=0;j<10;j++)
            {dsz2=dsz2+sonar.distance[j];}
            if(dsz2>hmax){dsz2=0;} 
            else {++okdsz;}}
        
    }
    if(okdsz!=0){dszt[4]=(dsz1+dsz2+dsz3+dsz4)/okdsz;}
    else {dszt[4]=dszt[3];}
    //ROS_INFO("distance 1: %f, distance 2: %f, distance x : %f, distance y : %f",dsz1,dsz2,dsx1,dsy//
}

void subComp(const sensors::compass::ConstPtr& msg)
{
    //ROS_INFO_STREAM("ID: " << msg->id << " - RZ0: " << msg->rz[0] <<
    //                ", RZ1: " << msg->rz[1]);
	
    if (msg->id == 96){
	rz1=(msg->rz[0])-rz0;}
    if (start && rz1!=0){rz0=rz1;
	start=false;}
    
    //ROS_INFO("rotation: %f",rz); 
}

void subImu(const sensors::imubuff::ConstPtr& msg)
{
ax=msg->buffer[0].accel[0];
ay=msg->buffer[0].accel[1];
az=msg->buffer[0].accel[2];
gx=msg->buffer[0].gyro[0];
gy=msg->buffer[0].gyro[1];
gz=msg->buffer[0].gyro[2];
mx=msg->buffer[0].magn[0];
my=msg->buffer[0].magn[1];
mz=msg->buffer[0].magn[2];

//ROS_INFO("ax: %f, ay: %f, az: %f",ax,ay,az);
//ROS_INFO("gx: %f, gy: %f, gz: %f",gx,gy,gz);
//ROS_INFO("mx: %f, my: %f, mz: %f",mx,my,mz);
}

void poseCallback(geometry_msgs::PoseStamped ps){
  dsx2=ps.pose.position.x;
  dsy2=ps.pose.position.y;
  rz2=ps.pose.orientation.w;
}

int main(int argc, char **argv)
{
    	ros::init(argc, argv, "tryphon241_state_estimator");
    	ros::NodeHandle node;
    	ros::Publisher Controle_node = node.advertise<state::state>("/tryphon241/state",1);
    	ros::Rate loop_rate(10);
    	geometry_msgs::Pose pose;
    	state::state state;
	int print=0;

	//ros::Subscriber subA = node.subscribe("/android/imu", 1, poseCallback);
	ros::Subscriber subS = node.subscribe("/tryphon241/sonars", 1, subSonar);
	ros::Subscriber subC = node.subscribe("/tryphon241/compass",1,subComp);
	ros::Subscriber subI = node.subscribe("/tryphon241/imubuf",1,subImu);
	ros::Subscriber subSick = node.subscribe("/cubeA_pose", 1, poseCallback);
	
	while (ros::ok())
	{
        	////////////////////////////////////
        	////       State estimator      ////
        	////////////////////////////////////
		dsztf[4]=1.119*dsztf[3]-0.8843*dsztf[2]+0.2898*dsztf[1]-0.04457*dsztf[0]+0.03251*dszt[4]+0.13*dszt[3]+0.1951*dszt[2]+0.13*dszt[1]+0.03251*dszt[0];

        	state.pos[0]=dsx2;//dsx1;
        	state.pos[1]=dsy2;//dsy1;
        	state.pos[2]=dsztf[4];
        	state.quat[0]=rz2;//rz1
        	state.quat[1]=0;
        	state.quat[2]=0;
        	state.quat[3]=0;
		state.vel[0]=0;
		state.vel[1]=0;
		state.vel[2]=0;
		state.angvel[0]=0;
                state.angvel[1]=0;
                state.angvel[2]=0;
             	/////////////////////////////////
        	if(print==10){ROS_INFO("dist z(raw): %f, z(filtered): %f, dist x : %f, dist y : %f, rz : %f,z1:%f,z2:%f,z3:%f,z4:%f",dszt[4],dsztf[4],dsx2,dsy2,rz2,dsz1,dsz2,dsz3,dsz4);
		print=0;}
		else {print++;}
	        Controle_node.publish(state);
		ros::spinOnce();
		loop_rate.sleep();
	}
	return 0;
}
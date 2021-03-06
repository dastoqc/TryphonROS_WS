
#include <iostream>
#include <ros/ros.h>
#include "geometry_msgs/Pose.h"
#include "geometry_msgs/PoseStamped.h"
#include "geometry_msgs/TwistStamped.h"
#include <Eigen/Geometry>
#include <Eigen/Dense>
#include <cmath>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <signal.h>
#include "std_msgs/Bool.h"
#include <dynamic_reconfigure/server.h>
#include <controls/glideslopeConfig.h>
#include "controls/State.h"

#include "controls/Commands.h"
#include "vects2geoMsgs.cpp"

#define PI 3.14159265

ros::Publisher statedes_chaser;
ros::Publisher statedes_target;

ros::Publisher magnet;



//tryphon 244 has docking on x-direction, confirmed

geometry_msgs::PoseStamped p_chaser,p_target,p_rel, pdes_chaser, pdes_target, p_target_initial;
geometry_msgs::TwistStamped veldes_chaser,veldes_target;
Eigen::Matrix3d Rmatrix_chaser,CPCMIMUmatrix_chaser, RIMUmatrix_chaser,CPCMCmatrix_chaser, Rmatrix_target,CPCMIMUmatrix_target, RIMUmatrix_target,CPCMCmatrix_target;
Eigen::Vector3d pos_chaser,pos_target,angle_target, angle_chaser, posdesirT, posdesirC, angledesirT,angledesirC, veldesirT, aveldesirT, veldesirC, aveldesirC;
Eigen::Vector3d CMIMUpos(0,0,0);
Eigen::Vector3d CMCpos(0,0,0.18);
Eigen::Quaterniond quatIMU(1, 0,0, 0); // quat of the rotation matrix between the tryphon frame and the IMU frame
//Eigen::Vector3d CMCpos(0,0,0.18);
//Eigen::Vector3d CMIMUpos(1,0,-1.125);
//Eigen::Quaterniond quatIMU(0.99255, 0,0.12187, 0); // quat of the rotation matrix between the tryphon frame and the IMU frame
controls::State statedesirT,statedesirC; 

double chaser_rotation,target_rotation; //to make sure chaser settles before begining docking
double t=0;
double path_debut_time=0;
double sidelength_cube=2*1.125;
bool facing_each_other=false; //evaluates to true when line of sight guidance can begin for x_inital to be deteRmined
bool x_init_count=true; //ensures x init is set once
bool count_targ1=true;//this will ensure the orinal position of target is maintained for all info
std_msgs::Bool magnet_on;
bool start_T=true; //this will confiRm when target pose has been acquired at least once 
bool start_C=true;
bool starting_now=false; //starts with dynamic reconfigure
//////////glideslope parameters global for dynamic changes
double a_factor=.01; //glideslope <0, should probably be some function of initial distance since it controls the period
double a;
double x_initial; //should actually be once the tryphons are alligned
double v_max; //maximum allowed velocity upon docking
double v_init; 
double period; //The glideslope algorithm has a period (TBD)
///////////end of glideslope parameters

Eigen::Matrix3d CPM(Eigen::Vector3d vect) // return cross product matrix
{
	Eigen::Matrix3d CPM;
	CPM<<       0,  -vect(2),   vect(1),
		  vect(2),         0,  -vect(0),
		  -vect(1),  vect(0),         0;

	return CPM;
}


void subPose_target(const geometry_msgs::PoseStamped PoseS1) //only need this for inital positions once facing each other
{

geometry_msgs::PoseStamped Pose;

Pose.pose=PoseS1.pose;
  
  if(start_T)
  {
    RIMUmatrix_target=quatIMU.toRotationMatrix(); // need to be computed only once
    CPCMIMUmatrix_target=CPM(CMIMUpos);
    CPCMCmatrix_target=CPM(CMCpos);
    start_T=false;
  }

  pos_target(0)=Pose.pose.position.x; // defined in global frame
  pos_target(1)=Pose.pose.position.y;
  pos_target(2)=Pose.pose.position.z;
  Eigen::Quaterniond quat(Pose.pose.orientation.w,Pose.pose.orientation.x,Pose.pose.orientation.y,Pose.pose.orientation.z);
 
  //quat=quat*quatIMU.inverse();  // compute the quaternion between the vision world and the tryphon frame

  angle_target(0)=Pose.pose.orientation.x;
  angle_target(1)=Pose.pose.orientation.y;
  angle_target(2)=Pose.pose.orientation.z;


  //angle_target(0)=atan2(2*(quat.w()*quat.x()+quat.y()*quat.z()),1-2*(quat.x()*quat.x()+quat.y()*quat.y()));
 // angle_target(1)=asin(2*(quat.w()*quat.y()-quat.z()*quat.x()));
  //angle_target(2)=atan2(2*(quat.w()*quat.z()+quat.x()*quat.y()),1-2*(quat.z()*quat.z()+quat.y()*quat.y()));
  //Rmatrix_target=quat.toRotationMatrix();

  //pos_target=pos_target-Rmatrix_target*CMIMUpos;  // offset due to the fact that the pose is the one of the IMU, now at center of mass


p_target.pose.position.x=pos_target(0);
p_target.pose.position.y=pos_target(1);
p_target.pose.position.z=pos_target(2);
p_target.pose.orientation.x=angle_target(0); //remap inertial roll pitch yaw angles to the pose quaternions where w is not used
p_target.pose.orientation.y=angle_target(1);
p_target.pose.orientation.z=angle_target(2);
//keeps track that at least one pose has been called for target
}

void subPose_chaser(const geometry_msgs::PoseStamped PoseS2) 
{

geometry_msgs::PoseStamped Pose;

Pose.pose=PoseS2.pose;
  
  if(start_C)
  {
    RIMUmatrix_chaser=quatIMU.toRotationMatrix(); // need to be computed only once
    CPCMIMUmatrix_chaser=CPM(CMIMUpos);
    CPCMCmatrix_chaser=CPM(CMCpos);
    start_C=false;
  }

  pos_chaser(0)=Pose.pose.position.x; // defined in global frame
  pos_chaser(1)=Pose.pose.position.y;
  pos_chaser(2)=Pose.pose.position.z;
  Eigen::Quaterniond quat(Pose.pose.orientation.w,Pose.pose.orientation.x,Pose.pose.orientation.y,Pose.pose.orientation.z);
 
 // quat=quat*quatIMU.inverse();  // compute the quaternion between the vision world and the tryphon frame

  //angle_chaser(0)=atan2(2*(quat.w()*quat.x()+quat.y()*quat.z()),1-2*(quat.x()*quat.x()+quat.y()*quat.y()));
  //angle_chaser(1)=asin(2*(quat.w()*quat.y()-quat.z()*quat.x()));
  //angle_chaser(2)=atan2(2*(quat.w()*quat.z()+quat.x()*quat.y()),1-2*(quat.z()*quat.z()+quat.y()*quat.y()));
 // Rmatrix_chaser=quat.toRotationMatrix();

  angle_chaser(0)=Pose.pose.orientation.x;
  angle_chaser(1)=Pose.pose.orientation.y;
  angle_chaser(2)=Pose.pose.orientation.z;

  //pos_chaser=pos_chaser-Rmatrix_chaser*CMIMUpos;  // offset due to the fact that the pose is the one of the IMU


p_chaser.pose.position.x=pos_chaser(0);
p_chaser.pose.position.y=pos_chaser(1);
p_chaser.pose.position.z=pos_chaser(2);
p_chaser.pose.orientation.x=angle_chaser(0);
p_chaser.pose.orientation.y=angle_chaser(1);
p_chaser.pose.orientation.z=angle_chaser(2);
}

void subVel_chaser(const geometry_msgs::TwistStamped TwistC) 
{	
chaser_rotation=TwistC.twist.angular.z;
}
void subVel_target(const geometry_msgs::TwistStamped TwistT) 
{	
target_rotation=TwistT.twist.angular.z;
}

void callback(controls::glideslopeConfig &config, uint32_t level) 
{
	ROS_INFO("Reconfigure Request: a_factor: %f, v_max: %f",
             config.a_factor,
             config.v_max);

starting_now=config.starting;
a_factor=config.a_factor;
v_max=config.v_max;
}

void pose_zero(geometry_msgs::PoseStamped &p) //set any pose msg to zero
{
	p.pose.position.x=0;
	p.pose.position.y=0;
	p.pose.position.z=0;
	p.pose.orientation.w=0; 
	p.pose.orientation.x=0;
	p.pose.orientation.y=0;
	p.pose.orientation.z=0;
}

void twist_zero(geometry_msgs::TwistStamped &tw) //set any pose msg to zero
{
	tw.twist.linear.x=0;
	tw.twist.linear.y=0;
	tw.twist.linear.z=0;
	tw.twist.angular.x=0;
	tw.twist.angular.y=0;
	tw.twist.angular.z=0;
}


bool facing_error(const geometry_msgs::PoseStamped Pose, double yaw) //returns true when the yaw desired (range) is achieved 
{

double yaw_error_allowed=0.06; //set yaw range

double angle_chaser_z;
//Eigen::Quaterniond quat(Pose.orientation.w,Pose.orientation.x,Pose.orientation.y,Pose.orientation.z);
angle_chaser_z=Pose.pose.orientation.z;    ///atan2(2*(quat.w()*quat.z()+quat.x()*quat.y()),1-2*(quat.z()*quat.z()+quat.y()*quat.y()));

if (abs(angle_chaser_z-yaw)<=yaw_error_allowed) //abs(angle_chaser_z-yaw-PI/2) ensure last term is (-) of that added to the one on pdes_target and chaser 
	{
	return true;
	}
else {return false;}

}

int main(int argc, char **argv) //arguments should be IP of chaser then IP of target
{

ros::init(argc, argv,"glideslope_rel");
ros::NodeHandle node;
//////ROS node setup with IP of chaser
std::string s; //temp_arg;
magnet_on.data=false;

char rosname[100],rosnameC[100],rosnameT[100],ip[100]; 
s=argv[1];
//std::replace(s.begin(), s.end(), '.', '_');
sprintf(rosnameC,"%s",s.c_str());
s=argv[2];
//std::replace(s.begin(), s.end(), '.', '_');
sprintf(rosnameT,"%s",s.c_str()); //used for target and chaser

int maxThrust=100;
bool noInt=false;
double GainCP=1;



if (argc==3)
  {
	    ROS_INFO("CHASER IS: %s", argv[1]);
	    ROS_INFO("TARGET IS: %s", argv[2]);
  }
else
  {
    ROS_ERROR("Failed to get param 'target' or 'chaser'");
    return 0;
  }



/*
temp_arg = argv[1];
 std::replace(temp_arg.begin(), temp_arg.end(), '.', '_');


if (argc==2)
  {
    ROS_INFO("TARGET IS: %s", argv[1]);
  }
else
  {
    ROS_ERROR("Failed to get param 'target'");
    return 0;
  }
ros::Rate loop_rate(10); 
//////End of ROS Node setup
temp_arg = argv[1];
  std::replace(temp_arg.begin(), temp_arg.end(), '.', '_');
*/
magnet = node.advertise<std_msgs::Bool>("/magnet_on",1);

sprintf(rosname,"/%s/glide_des_state",rosnameC);
statedes_chaser = node.advertise<controls::State>(rosname,1);
sprintf(rosname,"/%s/glide_des_state",rosnameT);
statedes_target = node.advertise<controls::State>(rosname,1);

sprintf(rosname,"/%s/state_estimator/pose",rosnameC);
ros::Subscriber subP_chase = node.subscribe(rosname, 1, subPose_chaser); //c
sprintf(rosname,"/%s/state_estimator/vel",rosnameC);
ros::Subscriber subV_chase = node.subscribe(rosname, 1, subVel_chaser); //c
sprintf(rosname,"/%s/state_estimator/pose",rosnameT);
ros::Subscriber supP_targ = node.subscribe(rosname, 1 , subPose_target); //must change this accordingly
sprintf(rosname,"/%s/state_estimator/vel",rosnameT);
ros::Subscriber supV_targ = node.subscribe(rosname, 1 , subVel_target);

dynamic_reconfigure::Server<controls::glideslopeConfig> server;
dynamic_reconfigure::Server<controls::glideslopeConfig>::CallbackType f;

 f = boost::bind(&callback, _1, _2);
 server.setCallback(f);

ros::Rate loop_rate(10); 

pose_zero(p_chaser);
pose_zero(p_target);
pose_zero(p_rel);
pose_zero(pdes_target);
pose_zero(pdes_chaser);
pose_zero(p_target_initial);
twist_zero(veldes_chaser);
twist_zero(veldes_target);

bool path_debut=true;
///////////glideslope parameters
double temp_angle1,temp_angle2;
double yawd_target;
double yawd_chaser;
double r,r_dot;

while (ros::ok())
{
	ros::spinOnce(); 
	
if (starting_now)
	{
	if(!start_T && !start_C)
		{

		p_rel.pose.position.x=p_target.pose.position.x-p_chaser.pose.position.x;
		p_rel.pose.position.y=p_target.pose.position.y-p_chaser.pose.position.y;

		if(count_targ1) //count arg becomes false after first loop
			{
			yawd_chaser=atan2(p_rel.pose.position.y,p_rel.pose.position.x); //finds yaw desired, careful for 0,0, working in -PI to PI
			
			if(yawd_chaser<=0)
				{ 
				yawd_target=PI+yawd_chaser;	
				ROS_INFO("yawd_target: <0: %f", yawd_target);
				} //changed from - to +     //playing around with PI to keep mcptam localized, 
			else
				{ 
				yawd_target=yawd_chaser-PI;
				ROS_INFO("yawd_target: >0: %f", yawd_target);
				} //finds the yawd of target, make sure >PI works out, might be wrong
		
			
		//set initial position where chaser and target will face each other
			p_target_initial=p_target;
			pdes_target.pose.position.x=p_target.pose.position.x;
			pdes_target.pose.position.y=p_target.pose.position.y;
			pdes_target.pose.position.z=p_target.pose.position.z;
			
			temp_angle1=yawd_target-PI/2;
			if (temp_angle1>PI) {temp_angle1=temp_angle1-2*PI;}
			ROS_INFO("temp_angle1: %f, yawd:%f",temp_angle1,yawd_target);
			pdes_target.pose.orientation.z=temp_angle1; //+PI/2; //add angle to choose face for docking

			pdes_chaser.pose.position.x=p_chaser.pose.position.x;
			pdes_chaser.pose.position.y=p_chaser.pose.position.y;
			pdes_chaser.pose.position.z=p_target.pose.position.z; //chaser goes to height of target
			
			temp_angle2=yawd_chaser-PI/2;
			if (temp_angle2>PI) {temp_angle2=temp_angle2-2*PI;}
			if (temp_angle2<PI) {temp_angle2=temp_angle2+2*PI;}
			ROS_INFO("temp_angle2: %f, yawc:%f",temp_angle2,yawd_chaser);
			pdes_chaser.pose.orientation.z=temp_angle2; //add an angle to choose a face for docking
			
			count_targ1=false;
			}

		if (facing_error(p_target,temp_angle1) && facing_error(p_chaser,temp_angle2)) //this deteRmines whether the two tryphons are close to facing each other
			{

			if(chaser_rotation<0.03 && chaser_rotation>-0.03 && target_rotation<0.03 && target_rotation>-0.03) //abs() wasn't working
				{
				facing_each_other=true;
				}// compare the z angles and ensure facing each other as well as check that no overshoot is occuring
			} 

		if (facing_each_other) //glideslope loop for which begins once tryphons are facing each other
			{	

			if (path_debut) // sets up the inital conditions of glideslope
				{
				ROS_INFO("Facing each other now"); //moved slightly after to only show up once

    			path_debut_time=ros::Time::now().toSec();
   
				x_initial=-sqrt(pow(p_rel.pose.position.x,2.0)+pow(p_rel.pose.position.y,2.0))+sidelength_cube; //add sidelength here for x initial to be distance between two faces, sidelength must be added to r later
				a=(a_factor*x_initial); //define slope as a function of x_initial, (a distance of 4 m should take more time for docking than 3m) //maximum allowed velocity upon docking
				//v_max=.02;
				v_init=(a*x_initial+v_max); 
				period=1.0/a*log(v_max/v_init); //The glideslope algorithm has a period (TBD)
				ROS_INFO("Period=%f, V_init=%f, a=%f, x_inital=%f", period, v_init, a ,x_initial); //this must be possible with force of thrusters
				magnet_on.data=true;
				path_debut=false;
				}
		
			t= ros::Time::now().toSec() - path_debut_time; 

			//r is a negative number measured from the target and r_dot is positive
			double wiggle_room=0.2; //this is to provide some adjustement so that the faces slightly overlap for docking, with 1-exponential function so starts as 0 and effect more towards end
			r=x_initial*exp(a*t)+v_max/a*(exp(a*t)-1)-(sidelength_cube-wiggle_room*(1-exp(-t))); //x_initial should be defined as distance from //subtract sidelength from r so that final distance is fromf aces and not center of masses
			r_dot=a*r+v_max;

			ROS_INFO("r: %f,r_dot:%f",r, r_dot);
			//calulcate desired position and velocity of chaser
			
			if(yawd_target<PI/2 && yawd_target>-PI/2) //Addition of r_target with r_x, r_y of glideslope decomposed using yaw of target
				{
				ROS_INFO("check1");
				pdes_chaser.pose.position.x=p_target_initial.pose.position.x-(r)*cos(yawd_target);//will have to play around with this to make sure its proper
				pdes_chaser.pose.position.y=p_target_initial.pose.position.y-(r)*sin(yawd_target);
				veldes_chaser.twist.linear.x=r_dot*cos(yawd_chaser);
				veldes_chaser.twist.linear.y=r_dot*sin(yawd_chaser);
				}
			else
				{
					ROS_INFO("check2");
				pdes_chaser.pose.position.x=p_target_initial.pose.position.x-(r)*sin(yawd_target);
				pdes_chaser.pose.position.y=p_target_initial.pose.position.y-(r)*cos(yawd_target);	
				veldes_chaser.twist.linear.x=r_dot*cos(yawd_target);
				veldes_chaser.twist.linear.y=r_dot*sin(yawd_target);
				}


			} //end of glideslope alg
	

	/*if(t>=period) //attempt at stopping the glideslope after period
	{
		pose_zero(veld_chaser);
	} */



///set up to send to new control node
		posdesirT(0)=pdes_target.pose.position.x;
		posdesirT(1)=pdes_target.pose.position.y;
		posdesirT(2)=pdes_target.pose.position.z;
		posdesirC(0)=pdes_chaser.pose.position.x;
		posdesirC(1)=pdes_chaser.pose.position.y;
		posdesirC(2)=pdes_chaser.pose.position.z;

		angledesirT(0)=pdes_target.pose.orientation.x;
		angledesirT(1)=pdes_target.pose.orientation.y;
		angledesirT(2)=pdes_target.pose.orientation.z;
		angledesirC(0)=pdes_chaser.pose.orientation.x;
		angledesirC(1)=pdes_chaser.pose.orientation.y;
		angledesirC(2)=pdes_chaser.pose.orientation.z;

		veldesirT(0)=veldes_target.twist.linear.x;
		veldesirT(1)=veldes_target.twist.linear.y;
		veldesirT(2)=veldes_target.twist.linear.z;
		veldesirC(0)=veldes_chaser.twist.linear.x;
		veldesirC(1)=veldes_chaser.twist.linear.y;
		veldesirC(2)=veldes_chaser.twist.linear.z;


		aveldesirT(0)=veldes_target.twist.angular.x;
		aveldesirT(1)=veldes_target.twist.angular.y;
		aveldesirT(2)=veldes_target.twist.angular.z;
		aveldesirC(0)=veldes_chaser.twist.angular.x;
		aveldesirC(1)=veldes_chaser.twist.angular.y;
		aveldesirC(2)=veldes_chaser.twist.angular.z;

///end set up

////////////////

    	statedesirT.header.stamp=ros::Time::now();
    	statedesirT.pose=vects2pose(posdesirT,angledesirT);
    	statedesirT.vel=vects2twist(veldesirT,aveldesirT);
    	//statedesir.accel=vects2twist(acceldesir,angleAcceldesir);
    	statedesirT.maxThrust=maxThrust;
    	statedesirT.GainCP=GainCP;
    	statedesirT.noInt=noInt;


    	statedesirC.header.stamp=ros::Time::now();
    	statedesirC.pose=vects2pose(posdesirC,angledesirC);
    	statedesirC.vel=vects2twist(veldesirC,aveldesirC);
    	//statedesir.accel=vects2twist(acceldesir,angleAcceldesir);
    	statedesirC.maxThrust=maxThrust;
    	statedesirC.GainCP=GainCP;
    	statedesirC.noInt=noInt;
/////////////



		statedes_target.publish(statedesirT); //what is sent has orientation in euler z=yaw
		statedes_chaser.publish(statedesirC); //what is sent has orientation in euler z=yaw
		magnet.publish(magnet_on);

		if (t>=period && facing_each_other)
			{
			ROS_INFO("Algorithm ending");	 //might have to put desired velocity to zero before algo finishes
			break;
			} //break the loop after period so the desired pos,vel doesn't keep changing
	
		}
	}
loop_rate.sleep();
}
return 0;
}

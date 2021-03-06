#include "ctr_fuzzy.h"
#include <unistd.h>
#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <signal.h>

#include <Eigen/Dense>
#include <Eigen/Geometry>

bool start=true;
state::state init_state;
state::state des_state;

double Ls=(double)CUBE_LENGTH;				// Cube lengh
state::state E;		// The error
state::state oE;	// The old error
state::state I;		// Integral of error
state::state D;		// Derivative of error
double wlimit=((double)DISTANCE_SECURITE)/100;	// The collision limit
double zlimit=((double)DISTANCE_SECURITEZ)/100;
t_gainfuzzy gfuz;
double force_max=0.5;

ros::Publisher Controle_node;

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void mySigintHandler(int sig)
{
  geometry_msgs::Wrench F;
   // Do some custom action.
   // For example, publish a stop message to some other nodes.
   F.force.x=0;F.force.y=0;F.force.z=0;F.torque.x=0;F.torque.y=0;F.torque.z=0;
   Controle_node.publish(F);
   ROS_INFO("fx: %f, fy: %f, fz: %f,Tx: %f, Ty: %f, Tz: %f",F.force.x, F.force.y, F.force.z, F.torque.x, F.torque.y, F.torque.z);
   // All the default sigint handler does is call shutdown()
   ros::shutdown();
}

void initctr()
{
/*	init_state.pos[0]=4;
	init_state.pos[1]=-2.5;
	init_state.pos[2]=2;
	init_state.quat[0]=3;*/
  I.pos[0]=0.0;
  I.pos[1]=0.0;
  I.pos[2]=0.0;
  //I.theta_x=0.0;
  //I.theta_y=0.0;
  I.quat[0]=0.0;
  oE.pos[0]=0.0;
  oE.pos[1]=0.0;
  oE.pos[2]=0.0;
  //oE.theta_x=0.0;
  //oE.theta_y=0.0;
  oE.quat[0]=0.0;
  
  gfuz.time=1.0/10.0;
  gfuz.mef_xy=(double)MAX_ERROR_FORCE_XY;
  gfuz.mef_z=(double)MAX_ERROR_FORCE_Z;
  gfuz.mef_tz=(double)MAX_ERROR_FORCE_TZ;
  gfuz.mef_txy=(double)MAX_ERROR_FORCE_TXY;
  gfuz.exy=(double)ERROR_RANGE_XY;
  gfuz.ez=(double)ERROR_RANGE_Z;
  gfuz.etz=(double)ERROR_RANGE_TZ;
  gfuz.etxy=(double)ERROR_RANGE_TXY;
  gfuz.mixy=(double)MAX_INTEGRAL_FORCE_XY;
  gfuz.miz=(double)MAX_INTEGRAL_FORCE_Z;
  gfuz.mitz=(double)MAX_INTEGRAL_FORCE_TZ;
  gfuz.mitxy=(double)MAX_INTEGRAL_FORCE_TXY;
  gfuz.ixy=(double)INTEGRAL_RANGE_XY;
  gfuz.iz=(double)INTEGRAL_RANGE_Z;
  gfuz.itz=(double)INTEGRAL_RANGE_TZ;
  gfuz.itxy=(double)INTEGRAL_RANGE_TXY;
  gfuz.minc_xy=(double)MAX_INC_FORCE_XY;
  gfuz.minc_z=(double)MAX_INC_FORCE_Z;
  gfuz.minc_tz=(double)MAX_INC_FORCE_TZ;
  gfuz.minc_txy=(double)MAX_INC_FORCE_TXY;
  gfuz.incxy=(double)INC_RANGE_XY;
  gfuz.incz=(double)INC_RANGE_Z;
  gfuz.inctz=(double)INC_RANGE_TZ;
  gfuz.inctxy=(double)INC_RANGE_TXY;
  gfuz.incsxy=(double)INC_SLOPE_XY;
  gfuz.incsz=(double)INC_SLOPE_Z;
  gfuz.incstz=(double)INC_SLOPE_TZ;
  gfuz.incstxy=(double)INC_SLOPE_TXY;
  gfuz.mdec_xy=(double)MAX_DEC_FORCE_XY;
  gfuz.mdec_z=(double)MAX_DEC_FORCE_Z;
  gfuz.mdec_tz=(double)MAX_DEC_FORCE_TZ;
  gfuz.mdec_txy=(double)MAX_DEC_FORCE_TXY;
  gfuz.decxy=(double)DEC_RANGE_XY;
  gfuz.decz=(double)DEC_RANGE_Z;
  gfuz.dectz=(double)DEC_RANGE_TZ;
  gfuz.dectxy=(double)DEC_RANGE_TXY;
  gfuz.decsxy=(double)DEC_SLOPE_XY;
  gfuz.decsz=(double)DEC_SLOPE_Z;
  gfuz.decstz=(double)DEC_SLOPE_TZ;
  gfuz.decstxy=(double)DEC_SLOPE_TXY;
  gfuz.awinup_xy=(double)ANTI_WINDUP_XY;		
  gfuz.awinup_z=(double)ANTI_WINDUP_Z;		
  gfuz.awinup_tz=(double)ANTI_WINDUP_TZ;		
  gfuz.awinup_txy=(double)ANTI_WINDUP_TXY;
}

double isig(double value, double peak)			// Membership function, inverse triangle
{
double isigma;

   isigma=value/peak;

if (isigma>=0)
   isigma=1-isigma;
if (isigma<0)
   isigma=0; 
return isigma;
}

 
double sig(double value, double peak)			// Membership function Triangle zero at zero
{
double sigma;

   sigma=value/peak;
   
if (sigma<0)
   sigma=0;
   
if (sigma>1)
   sigma=1;
 
return sigma;
}

inline double wrapto2pi( double angle )
{
    double twoPi = 2.0 * 3.141592865358979;
    return angle - twoPi * floor( angle / twoPi );
}

inline double wraptopi( double angle )
{
    double Pi = 3.141592865358979;
    return  wrapto2pi( angle + Pi ) - Pi;
}

void fuzzy_control(const state::state state)
{
		geometry_msgs::Wrench F;
	
		// Update old error
		oE.pos[0]=E.pos[0];
		oE.pos[1]=E.pos[1];
		oE.pos[2]=E.pos[2];
		//oE.theta_x=E.theta_x;
		//oE.theta_y=E.theta_y;
		oE.quat[0]=E.quat[0];

		// Calculate the error
		E.pos[0]=des_state.pos[0]-state.pos[0];
		E.pos[1]=des_state.pos[1]-state.pos[1];
		E.pos[2]=des_state.pos[2]-state.pos[2];
		E.quat[0]=wraptopi(des_state.quat[0]-state.quat[0]);
		
		/*...Integral of Error.................................................................*/

		I.pos[0]=I.pos[0]+E.pos[0]*gfuz.time;
		I.pos[1]=I.pos[1]+E.pos[1]*gfuz.time;
		I.pos[2]=I.pos[2]+E.pos[2]*gfuz.time;
		I.quat[0]=I.quat[0]+E.quat[0]*gfuz.time;
		
		// Anti-Windup
		if (I.pos[0]>ANTI_WINDUP_XY)
		   I.pos[0]=ANTI_WINDUP_XY;
		if (I.pos[0]<-ANTI_WINDUP_XY)
		   I.pos[0]=-ANTI_WINDUP_XY;
	   	if (I.pos[1]>ANTI_WINDUP_XY)
		   I.pos[1]=ANTI_WINDUP_XY;
		if (I.pos[1]<-ANTI_WINDUP_XY)
		   I.pos[1]=-ANTI_WINDUP_XY;
		if (I.pos[2]>ANTI_WINDUP_Z)
		   I.pos[2]=ANTI_WINDUP_Z;
		if (I.pos[2]<-ANTI_WINDUP_Z)
		   I.pos[2]=-ANTI_WINDUP_Z;   
/*		if (I.theta_x>gfuz->awinup_txy)
		   I.theta_x=gfuz->awinup_txy;
		if (I.theta_x<-gfuz->awinup_txy)
		   I.theta_x=-gfuz->awinup_txy;
		if (I.theta_y>gfuz->awinup_txy)
		   I.theta_y=gfuz->awinup_txy;
		if (I.theta_y<-gfuz->awinup_txy)
		   I.theta_y=-gfuz->awinup_txy;*/
		 if (I.quat[0]>ANTI_WINDUP_TZ)
		   I.quat[0]=ANTI_WINDUP_TZ;
		if (I.quat[0]<-ANTI_WINDUP_TZ)
		   I.quat[0]=-ANTI_WINDUP_TZ;     

		/*...Derivative of Error................................................................*/
		D.pos[0]=(E.pos[0]-oE.pos[0])/gfuz.time;
		D.pos[1]=(E.pos[1]-oE.pos[1])/gfuz.time;
		D.pos[2]=(E.pos[2]-oE.pos[2])/gfuz.time;
/*		D.theta_x=(E.theta_x-oE.theta_x)/time;
		D.theta_y=(E.theta_y-oE.theta_y)/time;*/
		D.quat[0]=(E.quat[0]-oE.quat[0])/gfuz.time;


		// Calculate force command, fuzzy style
		F.force.x=0;
		F.force.y=0;
		F.force.z=0;
		F.torque.x=0;
		F.torque.y=0;
		F.torque.z=0;
F.force.x=gfuz.mef_xy*(sig(E.pos[0], gfuz.exy)-sig(E.pos[0], -gfuz.exy))+gfuz.mixy*(sig(E.pos[0], gfuz.ixy)*sig(I.pos[0], gfuz.awinup_xy)-sig(E.pos[0], -gfuz.ixy)*sig(I.pos[0], -gfuz.awinup_xy))+gfuz.minc_xy*(sig(E.pos[0], gfuz.incxy)*sig(D.pos[0], gfuz.incsxy)-sig(E.pos[0], -gfuz.incxy)*sig(D.pos[0], -gfuz.incsxy))-gfuz.mdec_xy*(isig(E.pos[0], gfuz.decxy)*sig(D.pos[0], -gfuz.decsxy)-isig(E.pos[0], -gfuz.decxy)*sig(D.pos[0], gfuz.decsxy));
F.force.y=gfuz.mef_xy*(sig(E.pos[1], gfuz.exy)-sig(E.pos[1], -gfuz.exy))+gfuz.mixy*(sig(E.pos[1], gfuz.ixy)*sig(I.pos[1], gfuz.awinup_xy)-sig(E.pos[1], -gfuz.ixy)*sig(I.pos[1], -gfuz.awinup_xy))+gfuz.minc_xy*(sig(E.pos[1], gfuz.incxy)*sig(D.pos[1], gfuz.incsxy)-sig(E.pos[1], -gfuz.incxy)*sig(D.pos[1], -gfuz.incsxy))-gfuz.mdec_xy*(isig(E.pos[1], gfuz.decxy)*sig(D.pos[1], -gfuz.decsxy)-isig(E.pos[1], -gfuz.decxy)*sig(D.pos[1], gfuz.decsxy));
F.force.z=gfuz.mef_z*(sig(E.pos[2], gfuz.ez)-sig(E.pos[2], -gfuz.ez))+gfuz.miz*(sig(E.pos[2], gfuz.iz)*sig(I.pos[2], gfuz.awinup_z)-sig(E.pos[2], -gfuz.iz)*sig(I.pos[2], -gfuz.awinup_z))+gfuz.minc_z*(sig(E.pos[2], gfuz.incz)*sig(D.pos[2], gfuz.incsz)-sig(E.pos[2], -gfuz.incz)*sig(D.pos[2], -gfuz.incsz))-gfuz.mdec_z*(isig(E.pos[2], gfuz.decz)*sig(D.pos[2], -gfuz.decsz)-isig(E.pos[2], -gfuz.decz)*sig(D.pos[2], gfuz.decsz));
//F.Tx=-2*gfuz->mef_txy*(sig(E.theta_x, gfuz->etxy)-sig(E.theta_x, -gfuz->etxy));
//F.Tx=gfuz->mef_txy*(sig(E.theta_x, gfuz->etxy)-sig(E.theta_x, -gfuz->etxy))+gfuz->mitxy*(sig(E.theta_x, gfuz->itxy)*sig(I.theta_x, gfuz->awinup_txy)-sig(E.theta_x, -gfuz->itxy)*sig(I.theta_x, -gfuz->awinup_txy))+gfuz->minc_txy*(sig(E.theta_x, gfuz->inctxy)*sig(D.theta_x, gfuz->incstxy)-sig(E.theta_x, -gfuz->inctxy)*sig(D.theta_x, -gfuz->incstxy))-gfuz->mdec_txy*(isig(E.theta_x, gfuz->dectxy)*sig(D.theta_x, -gfuz->decstxy)-isig(E.theta_x, -gfuz->dectxy)*sig(D.theta_x, gfuz->decstxy));
//F.Ty=-2*gfuz->mef_txy*(sig(E.theta_y, etxy)-sig(E.theta_y, -etxy));
//F.Ty=gfuz->mef_txy*(sig(E.theta_y, gfuz->etxy)-sig(E.theta_y, -gfuz->etxy))+gfuz->mitxy*(sig(E.theta_y, gfuz->itxy)*sig(I.theta_y, gfuz->awinup_txy)-sig(E.theta_y, -gfuz->itxy)*sig(I.theta_y, -gfuz->awinup_txy))+gfuz->minc_txy*(sig(E.theta_y, gfuz->inctxy)*sig(D.theta_y, gfuz->incstxy)-sig(E.theta_y, -gfuz->inctxy)*sig(D.theta_y, -gfuz->incstxy))-gfuz->mdec_txy*(isig(E.theta_y, gfuz->dectxy)*sig(D.theta_y, -gfuz->decstxy)-isig(E.theta_y, -gfuz->dectxy)*sig(D.theta_y, gfuz->decstxy));
F.torque.z=gfuz.mef_tz*(sig(E.quat[0], gfuz.etz)-sig(E.quat[0], -gfuz.etz))+gfuz.mitz*(sig(E.quat[0], gfuz.itz)*sig(I.quat[0], gfuz.awinup_tz)-sig(E.quat[0], -gfuz.itz)*sig(I.quat[0], -gfuz.awinup_tz))+gfuz.minc_tz*(sig(E.quat[0], gfuz.inctz)*sig(D.quat[0], gfuz.incstz)-sig(E.quat[0], -gfuz.inctz)*sig(D.quat[0], -gfuz.incstz))-gfuz.mdec_tz*(isig(E.quat[0], gfuz.dectz)*sig(D.quat[0], -gfuz.decstz)-isig(E.quat[0], -gfuz.dectz)*sig(D.quat[0], gfuz.decstz));

	
	if(fabs(F.force.x)>force_max){
	  F.force.x=sgn(F.force.x)*force_max;}
	if(fabs(F.force.y)>force_max){
	  F.force.y=sgn(F.force.y)*force_max;}
	if(fabs(F.force.z)>force_max*2){
	  F.force.z=sgn(F.force.z)*force_max*2;}
	if(fabs(F.torque.z)>force_max/2){
	  F.torque.z=sgn(F.torque.z)*force_max/2;}
	
	//ROS_INFO("GAINS: %f,%f,%f,%f,%f,%f,%f",gfuz.mef_xy,gfuz.mef_z,gfuz.mef_tz,gfuz.exy,gfuz.ez,gfuz.etz,gfuz.time);
	ROS_INFO("fx: %f, fy: %f, fz: %f,Tx: %f, Ty: %f, Tz: %f",F.force.x, F.force.y, F.force.z, F.torque.x, F.torque.y, F.torque.z);
	ROS_INFO("ex: %f, ey: %f, ez: %f,eTx: %f, eTy: %f, eTz: %f",E.pos[0], E.pos[1], E.pos[2], E.quat[1], E.quat[2], E.quat[0]);
        
	Controle_node.publish(F);
} 

void subState(const state::state state)
{

	if(start){
	init_state=state;des_state=state;
    	start=false;}

	fuzzy_control(state);
}

void subdPose(const geometry_msgs::Pose dPose)
{

  if(start)
    return;
  
  if(dPose.position.x+init_state.pos[0]<=MAX_X && dPose.position.x+init_state.pos[0]>=MIN_X)
	des_state.pos[0]=init_state.pos[0]+dPose.position.x;
  
  if(dPose.position.y+init_state.pos[1]<=MAX_Y && dPose.position.y+init_state.pos[1]>=MIN_Y)
	des_state.pos[1]=init_state.pos[1]+dPose.position.y;
  
  if(dPose.position.z+init_state.pos[2]<=MAX_Z && dPose.position.z+init_state.pos[2]>=MIN_Z)
	des_state.pos[2]=init_state.pos[2]+dPose.position.z;
	
  Eigen::Quaterniond quad(dPose.orientation.x,dPose.orientation.y,dPose.orientation.z,dPose.orientation.w);
  float ang = atan2(quad.toRotationMatrix()(1, 2), quad.toRotationMatrix()(2, 2));
  
  if(ang+init_state.quat[0]<=MAX_TZ && ang+init_state.quat[0]>=MIN_TZ)
    des_state.quat[0]=init_state.quat[0]+atan2(quad.toRotationMatrix()(1, 2), quad.toRotationMatrix()(2, 2));
  
  
  //ROS_INFO("Dx: %f, Dy: %f, Dz: %f,DTx: %f, DTy: %f, DTz: %f",des_state.pos[0], des_state.pos[1], des_state.pos[2], des_state.quat[1], des_state.quat[2], des_state.quat[0]);
}

const char* get_ip()
{
  int fd;
 struct ifreq ifr;
 char *ip = new char[100];

 fd = socket(AF_INET, SOCK_DGRAM, 0);

 /* I want to get an IPv4 IP address */
 ifr.ifr_addr.sa_family = AF_INET;

 /* I want IP address attached to "eth0" */
 strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

 ioctl(fd, SIOCGIFADDR, &ifr);

 close(fd);

 /* display result */
 sprintf(ip,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
 std::string s = ip;
 std::replace(s.begin(), s.end(), '.', '_');
 //ip=s.c_str();
 return s.c_str();
}

int main(int argc, char **argv)
{
    char rosname[100];
    sprintf(rosname,"control_%s",get_ip());
    printf(rosname);
    std::string temp_arg ;
	
    ros::init(argc, argv, rosname);
    //ros::init(argc, argv, "control");
    ros::NodeHandle node;
    initctr();
    
    signal(SIGINT, mySigintHandler);
        if (argc==2)
        {
          ROS_INFO("TARGET IS: %s", argv[1]);
        }
        else
        {
          ROS_ERROR("Failed to get param 'target'");
	  	return 0;
        }
    temp_arg = argv[1];
    std::replace(temp_arg.begin(), temp_arg.end(), '.', '_');
    
    sprintf(rosname,"/%s/command_control",temp_arg.c_str());
    Controle_node = node.advertise<geometry_msgs::Wrench>(rosname,1);
    ros::Rate loop_rate(5); //CHANGE TIME OF FUZZY CONTROL OF DIFF zOF 5H

    sprintf(rosname,"/%s/state",temp_arg.c_str());
	ros::Subscriber subS = node.subscribe(rosname, 1, subState);

	ros::Subscriber subdP = node.subscribe("/desired_deltapose", 1, subdPose);

	while (ros::ok())
	{
	  
		//F.force.x=0;F.force.y=0.25;F.force.z=0;F.torque.x=0;F.torque.y=0;F.torque.z=0; //FOR TEST!!!!
       		 /////////////////////////////////
		ros::spinOnce();
		loop_rate.sleep();
	}
	
	return 0;
	
}




#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctime>

#include <ros/ros.h>
#include <geometry_msgs/Wrench.h>
#include <geometry_msgs/Pose.h>
#include "std_msgs/String.h"
#include "std_msgs/Bool.h"
#include <sstream>
#include "sensors/sonar.h"
#include "sensors/compass.h"
#include "sensors/sonarArray.h"
#include "sensors/imuros.h"


//libraries for the array
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Int32MultiArray.h"
#include "std_msgs/Int32.h"
#include "std_msgs/Float32.h"

#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

//#include "i2c-dev.h"
#include "motors.h"
#include "sensors/motor.h"
#include "sensors/motorArray.h"
#include "cube.h"       // Cube geometry, inertia, etc.
#include "robocomm.h"

#include "capteurs.h"
/*
#define	VERSION		"1.1a"
#define	DEVICE_I2C	"/dev/i2c-3"
#define	I2C_LOCK()		{pthread_mutex_lock(&mutex_i2c);}
#define	I2C_UNLOCK()	{pthread_mutex_unlock(&mutex_i2c);}
*/
//#define DEVICE_I2C      "/dev/i2c-3"
typedef enum {FALSE, TRUE}
                BOOLEAN;
typedef unsigned char BYTE;
#define INTERACTION     0

//ROBOCOMM robovero;


std_msgs::Bool magnet;
int file;
int mode;
float incx,incy,incz;

/* typedef struct
                {
                double Fx;                              // The Force in x-Direction
                double Fy;                              // The Force in y-Direction
                double Fz;                              // The Force in z-Direction
                double Tx;                              // The Torque around x-Direction
                double Ty;                              // The Torque around y-Direction
                double Tz;                              // The Torque around z-Direction
                }
                t_forces;


t_forces Fcontrol, Fmanual,Foff, Fdyn;
*/

geometry_msgs::Wrench  Fcontrol,Fps3, Fmanual,Foff, Fdyn;

bool ecriture_moteur2(BYTE adresse, BYTE commande, BYTE argument,bool sens)
{
        int inf_forward = VIT_POS;
        int inf_reverse = VIT_NEG;
        int info1 = inf_forward;                // Default info1 value
 
       // long sys_time;
       // struct tm *today;
 
     //   time(&sys_time);
       // today = localtime(&sys_time);
       // int debut = today->tm_sec;
 
        bool    ret = 0;
 
 
//      printf("\nVIT:%f",(float)argument);
//      I2C_LOCK();
        if((int)argument >0 && (int)argument> VITESSE_MAX){argument = VITESSE_MAX;} 
    if((int)argument <0 && (int)argument< -VITESSE_MAX){argument =  -VITESSE_MAX;}
     if (abs((double)argument) < (((double)VITESSE_MIN)/2))  //      Commands bellow half the minimal speed are set to zero
        {
                argument = 0;
 
        }
 
        if (sens)  //DS : Les ESC prennent une commande sur 0-255 non-sign
        {
                //argument = 2*argument;
                info1 = inf_forward;
//       printf("POS:info1=%i,arg=%d\n",info1,argument);
        }
        else
        {
                //argument = (255 - (double)argument)*2;
               // argument = argument*2;
                info1 = inf_reverse;
//          printf("NEG:info1=%i,arg=%d\n",info1,argument);
        }
        /*if (ioctl(file, I2C_SLAVE, adresse >> 1) >= 0)
                        {
                        if (i2c_smbus_write_word_data(file, commande, (argument << 8) | info1) >= 0) {                  // send info1 AND PWM
                                ret = TRUE;
                                //printf("Envoi a %x : %i %i\n", adresse, info1, argument);
                                }
                        }
        else printf("Erreur adressage moteur %x\n", adresse);
 
        if (!(ret == TRUE))
                 printf("Erreur ecriture moteur %x\n", adresse);
//      I2C_UNLOCK();
 
        if (!ret)
                perror("ecriture_moteur2()");
 */
ret=TRUE; 
robovero.setMotor(adresse>>1,argument,info1);
//ROS_INFO("Thrust (%): %d",argument);
        return(ret);
}
 

void controlcallback (const geometry_msgs::Wrench Force)
{

//ROS_INFO("Ca marche (control): %f,%f,%f,%f,%f,%f",Force.force.x,Force.force.y,Force.force.z,Force.torque.x,Force.torque.y,Force.torque.z);

Fcontrol.force.x=Force.force.x;
Fcontrol.force.y=Force.force.y;
Fcontrol.force.z=Force.force.z;
Fcontrol.torque.x=Force.torque.x;
Fcontrol.torque.y=Force.torque.y;
Fcontrol.torque.z=Force.torque.z;
}


void ps3callback (const geometry_msgs::Wrench Force)
{ 

//ROS_INFO("Ca marche (ps3): %f,%f,%f,%f,%f,%f",Force.force.x,Force.force.y,Force.force.z,Force.torque.x,Force.torque.y,Force.torque.z);

Fps3.force.x=Force.force.x;
Fps3.force.y=Force.force.y;
Fps3.force.z=Force.force.z;
Fps3.torque.x=Force.torque.x;
Fps3.torque.y=Force.torque.y;
Fps3.torque.z=Force.torque.z;
}


void manualcallback (const geometry_msgs::Wrench ForceMan)
{

//ROS_INFO("Ca marche (manual): %f,%f,%f,%f,%f,%f",ForceMan.force.x,ForceMan.force.y,ForceMan.force.z,ForceMan.torque.x,ForceMan.torque.y,ForceMan.torque.z);

Fmanual.force.x=ForceMan.force.x;
Fmanual.force.y=ForceMan.force.y;
Fmanual.force.z=ForceMan.force.z;
Fmanual.torque.x=ForceMan.torque.x;
Fmanual.torque.y=ForceMan.torque.y;
Fmanual.torque.z=ForceMan.torque.z;

}


void forcecallback (const geometry_msgs::Wrench ForceDyn)
{

//ROS_INFO("Ca marche (ForceDyn): %f,%f,%f,%f,%f,%f",ForceDyn.force.x,ForceDyn.force.y,ForceDyn.force.z,ForceDyn.torque.x,ForceDyn.torque.y,ForceDyn.torque.z);

Fdyn.force.x=ForceDyn.force.x;
Fdyn.force.y=ForceDyn.force.y;
Fdyn.force.z=ForceDyn.force.z;
Fdyn.torque.x=ForceDyn.torque.x;
Fdyn.torque.y=ForceDyn.torque.y;
Fdyn.torque.z=ForceDyn.torque.z;
}


void magnetcallback (const std_msgs::Bool on)
{
ROS_INFO("Ca marche ");
if(magnet.data!=on.data){
robovero.swRelay((int) 0x40/2);
ROS_INFO("Switch relay : %i",on.data);
magnet=on;
}

}

void forces2motors(geometry_msgs::Wrench F)
{
double L=(double)CUBE_LENGTH;
int etat = INTERACTION;
 
//...Local variables.............................................................
float Cxl=0;                                                                            // Command Variable for Motor x left
float Cxr=0;                                                                            // Command Variable for Motor x right
float Cyf=0;                                                                            // Command Variable for Motor y front
float Cyb=0;                                                                            // Command Variable for Motor y back
float Cxtl=0;                                                                           // Command Variable for Motor x top left
float Cxtr=0;                                                                           // Command Variable for Motor x top right
float Cytf=0;                                                                           // Command Variable for Motor y top front
float Cytb=0;                                                                           // Command Variable for Motor y top back
float Czfl=0;                                                                           // Command Variable for Motor z front left
float Czfr=0;                                                                           // Command Variable for Motor z front right
float Czbl=0;                                                                           // Command Variable for Motor z back left
float Czbr=0;                                                                           // Command Variable for Motor z back right
 
// The following variables need to be ajusted such that force.x=1 produces about 1N of thrust
float Mxr=MOTOR_GAIN;                                                                   // Motor gain for Motor x right
float Mxl=MOTOR_GAIN;                                                                   // Motor gain for Motor x left
float Myf=MOTOR_GAIN;                                                                   // Motor gain for Motor y front
float Myb=MOTOR_GAIN;                                                                   // Motor gain for Motor y back
float Mzfl=MOTOR_GAIN;                                                                  // Motor gain Variable for Motor z front left
float Mzfr=MOTOR_GAIN;                                                                  // Motor gain Variable for Motor z front right
float Mzbl=MOTOR_GAIN;                                                                  // Motor gain Variable for Motor z back left
float Mzbr=MOTOR_GAIN;                                                                  // Motor gain Variable for Motor z back right
 
//...Calculations......................................................................

if(CFG_6DDL) {
        Cxr=Mxr*(float)(F.force.x/4+F.torque.z/(4*L)-F.torque.y/(4*L));                  // The Command for Motor x right/left With 12M
        Cxl=Mxl*(float)(F.force.x/4-F.torque.z/(4*L)-F.torque.y/(4*L));
        Cxtr=Mxr*(float)(F.force.x/4+F.torque.z/(4*L)+F.torque.y/(4*L));                 // The Command for TOP Motor x right/left With 12M
        Cxtl=Mxl*(float)(F.force.x/4-F.torque.z/(4*L)+F.torque.y/(4*L));
} else {
        Cxr=Mxr*(float)(F.force.x/2+F.torque.z/(2*L));                             // Idem With 8M
        Cxl=Mxl*(float)(F.force.x/2-F.torque.z/(2*L));
}
if(CFG_6DDL) {
        Cyf=Myf*(float)(F.force.y/4+F.torque.z/(4*L)+F.torque.x/(4*L));                          // The Command for Motor y front/back With 12M
        Cyb=Myb*(float)(F.force.y/4-F.torque.z/(4*L)+F.torque.x/(4*L));
        Cytf=Myf*(float)(F.force.y/4+F.torque.z/(4*L)-F.torque.x/(4*L));                         // The Command for TOP Motor y front/back With 12M
        Cytb=Myb*(float)(F.force.y/4-F.torque.z/(4*L)-F.torque.x/(4*L));
} else {
        Cyf=Myf*(float)(F.force.y/2-F.torque.z/(2*L));                             // The Command for Motor y front/back With 12M
        Cyb=Myb*(float)(F.force.y/2+F.torque.z/(2*L));                              ///// BIG SIGN INVERSION //////////
}
if(CFG_6DDL) {
        Czfl=Mzfl*(float)(F.force.z/4+(F.torque.x-F.torque.y)/(4*L));                            // The Command for Motor z front left
        Czfr=Mzfr*(float)(F.force.z/4+(-F.torque.x-F.torque.y)/(4*L));                           // The Command for Motor z front right
        Czbl=Mzbl*(float)(F.force.z/4+(F.torque.x+F.torque.y)/(4*L));                            // The Command for Motor z back left
        Czbr=Mzbr*(float)(F.force.z/4+(-F.torque.x+F.torque.y)/(4*L));                           // The Command for Motor z back right
} else {
        Czfl=Mzfl*(float)(F.force.z/4+(F.torque.x-F.torque.y)/(2*L));                            // The Command for Motor z front left
        Czfr=Mzfr*(float)(F.force.z/4+(-F.torque.x-F.torque.y)/(2*L));                           // The Command for Motor z front right
        Czbl=Mzbl*(float)(F.force.z/4+(F.torque.x+F.torque.y)/(2*L));                            // The Command for Motor z back left
        Czbr=Mzbr*(float)(F.force.z/4+(-F.torque.x+F.torque.y)/(2*L));                           // The Command for Motor z back right
}
//int collision = safetorque.yv2();
//...Send commands......................................................................
if (Cxl!=0 || Cxr!=0)//(pos_des.x!=0)||(pos_des.theta_z_ref!=0))        // Send command to motors in x Direction
{
      if(Cxl>=0)
     { ecriture_moteur2(ADR_XLEFT, COMMAND_SEND_RPM, Cxl,true);}
    else{ ecriture_moteur2(ADR_XLEFT, COMMAND_SEND_RPM,-Cxl,false);}                      // Send command to motor x left
       if(Cxr>=0) { ecriture_moteur2(ADR_XRIGHT, COMMAND_SEND_RPM, Cxr,true);}
    else{ecriture_moteur2(ADR_XRIGHT, COMMAND_SEND_RPM,-Cxr,false);}                    // Send command to motor x right
       // ecriture_moteur2(ADR_XTLEFT, COMMAND_SEND_RPM, Cxtl);                   // Send command to motor x left TOP
       // ecriture_moteur2(ADR_XTRIGHT, COMMAND_SEND_RPM, Cxtr);          // Send command to motor x right TOP
        //printf("\nXL:%3.3f,XR:%3.3f",Cxl,Cxr);        
}       
else if (etat!=INTERACTION)
{
        ecriture_moteur2(ADR_XLEFT, COMMAND_SEND_RPM, 0,true);                       // Send command to motor x left
        ecriture_moteur2(ADR_XRIGHT, COMMAND_SEND_RPM, 0,true);
       // ecriture_moteur2(ADR_XTLEFT, COMMAND_SEND_RPM, 0);                      // Send command to motor x left
       // ecriture_moteur2(ADR_XTRIGHT, COMMAND_SEND_RPM, 0);
}
if (Cyb!=0 || Cyf!=0)//(pos_des.y!=0)||(pos_des.theta_z_ref!=0))        // Send command to motors in y Direction
{
        if(Cyb>=0){ecriture_moteur2(ADR_YFRONT, COMMAND_SEND_RPM, Cyb,true);}
    else{ecriture_moteur2(ADR_YFRONT, COMMAND_SEND_RPM,-Cyb,false);}
        if(Cyf>=0){ecriture_moteur2(ADR_YBACK, COMMAND_SEND_RPM, Cyf,true);}
    else{ecriture_moteur2(ADR_YBACK, COMMAND_SEND_RPM,-Cyf,false);}                      // Send command to motor y back
        //ecriture_moteur2(ADR_YTFRONT, COMMAND_SEND_RPM, Cytb);
        //ecriture_moteur2(ADR_YTBACK, COMMAND_SEND_RPM, Cytf);                   // Send command to motor y back
        //printf("\nYF:%3.3f,YB:%3.3f",Cyf,Cyb);
}
else if (etat!=INTERACTION)
{
        ecriture_moteur2(ADR_YFRONT, COMMAND_SEND_RPM, 0,true);              // Send command to motor y front
        ecriture_moteur2(ADR_YBACK, COMMAND_SEND_RPM, 0,true);                       // Send command to motor y back
        //ecriture_moteur2(ADR_YTFRONT, COMMAND_SEND_RPM, 0);             // Send command to motor y front
        //ecriture_moteur2(ADR_YTBACK, COMMAND_SEND_RPM, 0);                      // Send command to motor y back
}
if (Czfl!=0)//pos_des.z!=0)//Send command to motors in z Direction
{
        if(Czfl>=0){ecriture_moteur2(ADR_ZFLEFT, COMMAND_SEND_RPM, Czfl,true);}
    else{ecriture_moteur2(ADR_ZFLEFT, COMMAND_SEND_RPM,-Czfl,false);}            // Send command to motor z front left
        if(Czfr>=0){ecriture_moteur2(ADR_ZFRIGHT, COMMAND_SEND_RPM, Czfr,true);}
    else{ecriture_moteur2(ADR_ZFRIGHT, COMMAND_SEND_RPM, -Czfr,false);}  // Send command to motor z front right
        if(Czbl>=0){ecriture_moteur2(ADR_ZBLEFT, COMMAND_SEND_RPM, Czbl,true);}           // Send command to motor z back left
        else{ecriture_moteur2(ADR_ZBLEFT, COMMAND_SEND_RPM, -Czbl,false);} 
    if(Czbr>=0){ecriture_moteur2(ADR_ZBRIGHT, COMMAND_SEND_RPM, Czbr,true);}          // Send command to motor z back right
    else{ecriture_moteur2(ADR_ZBRIGHT, COMMAND_SEND_RPM, -Czbr,false);} 
        //printf("\nZFL:%3.3f,ZFR:%3.3f",Czfl,Czfr);    
}
else if (etat!=INTERACTION)
{
        ecriture_moteur2(ADR_ZFLEFT, COMMAND_SEND_RPM, 0,true);              // Send command to motor z front left
        ecriture_moteur2(ADR_ZFRIGHT, COMMAND_SEND_RPM, 0,true);     // Send command to motor z front right
        ecriture_moteur2(ADR_ZBLEFT, COMMAND_SEND_RPM, 0,true);              // Send command to motor z back left
        ecriture_moteur2(ADR_ZBRIGHT, COMMAND_SEND_RPM, 0,true);     // Send command to motor z back right                           
}
return;
}





 //////////////////////////////////////////////////////////////////
// Convert 6DDL Forces to motors commands
////////////////////////////////////////////////////////////////

void scan_i2c_motors(int file, sensors::motorArray *mArray)                              //$
{
        int adr, ret=0;
        sensors::motor motorInst;
        //int deb, fin;
	int i=0;
        //long sys_time;
        //struct tm *today;
 
        //time(&sys_time);
        //today = localtime(&sys_time);
        //deb = today->tm_sec;
 
        printf("Motors I2C detectes:");
        for (adr=80; adr<96; adr++) { // de E0 a FC
        //  if((ret=ioctl( file , I2C_SLAVE , adr )) >= 0){
             if(robovero.scanI2C(adr)>0)// if (i2c_smbus_write_byte(file, 0x00) >= 0){
                        {
			printf(" 0x%02x", adr*2);
                        motorInst.id = adr;
                        mArray->motors.push_back(motorInst);
                        //printf("Add %i",sArray->sonars[i].id);
                        i++;
                }
         // }
        }
 
        //time(&sys_time);
        //today = localtime(&sys_time);
        //fin = today->tm_sec;
        //printf("\nTEMPS DE REACTION:%i",fin-deb);
        //printf("\n");
}

void modeinfo(std_msgs::Int32 modei)
{
	mode=modei.data;
}

//sensors voids

void getimudata(sensors::imuros *imuptr )
{
    
    imu::Vector<3> a;
    imu::Vector<3> g;
    imu::Vector<3> m;
    
    a=robovero.getIMUdata(0,'a');//0 for no print
    g=robovero.getIMUdata(0,'g');
    m=robovero.getIMUdata(0,'m');
    
    imuptr->accel[0]=a[0];
    imuptr->accel[1]=a[1];
    imuptr->accel[2]=a[2];
    imuptr->gyro[0]=g[0];
    imuptr->gyro[1]=g[1];
    imuptr->gyro[2]=g[2];
    imuptr->magn[0]=m[0];
    imuptr->magn[1]=m[1];
    imuptr->magn[2]=m[2];
    
}




void init_sonar(sensors::sonarArray *sArray)
{
	/*float gain=SONAR_GAIN_DEFAULT;
     float range=SONAR_RANGE_DEFAULT;*/
	int debut;
    long sys_time;
    struct tm *today;
    time(&sys_time);
    today = localtime(&sys_time);
	for(int i=0; i<sArray->sonars.size(); ++i){
        /* if(ioctl(file,I2C_SLAVE,sArray->sonars[i].id)>=0){
         debut = today->tm_sec;
         if(i2c_smbus_write_byte_data(file,SONAR_GAIN, SONAR_GAIN_DEFAULT)>=0 && i2c_smbus_write_byte_data(file,SONAR_RANGE, SONAR_RANGE_DEFAULT)>=0){
         while (i2c_smbus_read_byte_data(file, SONAR_SOFTWARE_REVISION) < 0){
         usleep(1000);   // 1000
         if (today->tm_sec-debut>1)
         {
         printf("Delai depasse en ecriture sonar %02x\n", sArray->sonars[i].id*2);
         return;
         }
         }
         }
         else
         printf("Erreur ecriture sonar\n");
         }*/
        robovero.init_sonar(sArray->sonars[i].id);
        usleep(25);
    }
    
	/*
     if (ecriture_sonar(sonar[i].id, SONAR_GAIN, gain))
     printf("\tCapteur %x : gain=%d, range=%d, revision firmware=%d\n", sonar[i].id, gain, range, i2c_smbus_read_byte_data(file, SONAR_SOFTWARE_REVISION));
     else printf("\tErreur initialisation gain sonar %x\n", sonar[i].id);
     if (!ecriture_sonar(sonar[i].id, SONAR_RANGE, range))
     printf("\tErreur initialisation range sonar %x\n", sonar[i].id);*/
}

/*
 void check_i2c_functions()
 {
 unsigned long	funcs;
 
 if (ioctl(file,I2C_FUNCS,&funcs) >= 0)
 {
 if (funcs & I2C_FUNC_I2C) puts("I2C-3");
 if (funcs & I2C_FUNC_10BIT_ADDR) puts("I2C_FUNC_10BIT_ADDR");
 if (funcs & I2C_FUNC_PROTOCOL_MANGLING) puts("I2C_FUNC_PROTOCOL_MANGLING");
 if (funcs & I2C_FUNC_SMBUS_QUICK) puts("I2C_FUNC_SMBUS_QUICK");
 if (funcs & I2C_FUNC_SMBUS_READ_BYTE) puts("I2C_FUNC_SMBUS_READ_BYTE");
 if (funcs & I2C_FUNC_SMBUS_WRITE_BYTE) puts("I2C_FUNC_SMBUS_WRITE_BYTE");
 if (funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA) puts("I2C_FUNC_SMBUS_READ_BYTE_DATA");
 if (funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA) puts("I2C_FUNC_SMBUS_WRITE_BYTE_DATA");
 if (funcs & I2C_FUNC_SMBUS_READ_WORD_DATA) puts("I2C_FUNC_SMBUS_READ_WORD_DATA");
 if (funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA) puts("I2C_FUNC_SMBUS_WRITE_WORD_DATA");
 if (funcs & I2C_FUNC_SMBUS_PROC_CALL) puts("I2C_FUNC_SMBUS_PROC_CALL");
 if (funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA) puts("I2C_FUNC_SMBUS_READ_BLOCK_DATA");
 if (funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA) puts("I2C_FUNC_SMBUS_WRITE_BLOCK_DATA");
 if (funcs & I2C_FUNC_SMBUS_READ_I2C_BLOCK) puts("I2C_FUNC_SMBUS_READ_I2C_BLOCK");
 if (funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK) puts("I2C_FUNC_SMBUS_WRITE_I2C_BLOCK");
 }
 else perror("ioctl() I2C_FUNCS failed");
 }*/

void askdistance(sensors::sonarArray *sArray){
        short distance;
        //int debut;
        //long sys_time;
        //struct tm *today;
        //time(&sys_time);
//      today = localtime(&sys_time);
        for(int i=0; i<sArray->sonars.size(); ++i){
        robovero.askSonar(sArray->sonars[i].id);}
	}



void getdistance(sensors::sonarArray *sArray){
	short distance;
	//int debut;
	//long sys_time;
	//struct tm *today;
	//time(&sys_time);
//	today = localtime(&sys_time);
	//for(int i=0; i<sArray->sonars.size(); ++i){
        //robovero.askSonar(sArray->sonars[i].id);}
	for(int i=0; i<sArray->sonars.size(); ++i){
		/*if(ioctl(file,I2C_SLAVE,sArray->sonars[i].id)>=0){
         debut = today->tm_sec;
         if(i2c_smbus_write_byte_data(file,SONAR_COMMAND, SONAR_RANGING_CM)>=0){
         while (i2c_smbus_read_byte_data(file, SONAR_SOFTWARE_REVISION) < 0){
         usleep(1000);	// 1000
         if (today->tm_sec-debut>1)
         {
         printf("Delai depasse en ecriture sonar %02x\n", sArray->sonars[i].id*2);
         return;
         }
         }
         distance = i2c_smbus_read_byte_data(file,SONAR_FIRST_ECHO_HIGH);
         distance <<= 8;
         distance |= i2c_smbus_read_byte_data(file, SONAR_FIRST_ECHO_LOW);
         for(int j=0;j<9;j++)
         sArray->sonars[i].distance[j+1]=sArray->sonars[i].distance[j];
         sArray->sonars[i].distance[0]=distance;
         }
         else
         printf("Erreur ecriture sonar\n");
         }*/
        
        distance = robovero.readSonar(sArray->sonars[i].id);
        if ((distance <=0) ||(distance> DISTANCE_MAX))
        {distance=DISTANCE_MAX;}
		for(int j=0;j<9;j++)
        {
			sArray->sonars[i].distance[j+1]=sArray->sonars[i].distance[j];
        }
        sArray->sonars[i].distance[0]=distance;
        
        
  //      usleep(25);
	}
}

void getrz(sensors::compass *boussole)
{
    int     high, low, bearing;
    
    /* if (ioctl(file, I2C_SLAVE, boussole->id)>=0)
     high = i2c_smbus_read_byte_data(file, BOUSSOLE_BEARING_WORD_HIGH);
     else{
     printf("Erreur adressage boussole %x\n", boussole->id*2);
     return;
     }
     usleep(25);
     
     if (high >= 0){
     if (ioctl(file,I2C_SLAVE,boussole->id)>=0)
     low = i2c_smbus_read_byte_data(file, BOUSSOLE_BEARING_WORD_LOW);
     else{
     printf("Erreur adressage boussole %x\n", boussole->id*2);
     return;
     }
     
     if (low >= 0){
     Combine the two bytes, and get the heading in degrees. */
    //bearing = (high * 256) + low;
    //bearing /= 10.0F;
    bearing=robovero.readCompass(boussole->id);
    //printf("\nAngle absolut: %d", bearing);
    
    if ((boussole->rz[0]-(float)bearing) < -180)
        boussole->head_init=boussole->head_init+360;
    else if ((boussole->rz[0]-(float)bearing) > 180)
        boussole->head_init=boussole->head_init-360;
    
    if ((float)bearing-boussole->head_init > 180)
        boussole->head_init=boussole->head_init+360;
    else if ((float)bearing-boussole->head_init < -180)
        boussole->head_init=boussole->head_init-360;
    
    boussole->rz[0] = (float)bearing;
    //	}
    //	}
}


void getmotor_data( sensors::motorArray *mArray){

int *data=new int[3];

for(int i=0; i<mArray->motors.size(); ++i){
	data = robovero.getMotorData(mArray->motors[i].id);
	mArray->motors[i].rpm=data[0];
	mArray->motors[i].volt=data[1];
	mArray->motors[i].curr=data[2];
}
}



void scan_i2c_sensors(int file, sensors::sonarArray *sArray, sensors::compass *comp)                              //$
{
    int adr, ret=0;
    sensors::sonar sonarInst;
    //int deb, fin;
	int i=0;
    //long sys_time;
    //struct tm *today;
    
    //time(&sys_time);
    //today = localtime(&sys_time);
    //deb = today->tm_sec;
    
    printf("Sonars I2C detectes:");
    for (adr=112; adr<128; adr++) { // de E0 a FC
        // if((ret=ioctl( file , I2C_SLAVE , adr )) >= 0){
        if(robovero.scanI2C(adr)>0){// (i2c_smbus_write_byte(file, 0x00) >= 0){
            printf(" 0x%02x", adr*2);
            sonarInst.id = adr;
            sArray->sonars.push_back(sonarInst);
            //printf("Add %i",sArray->sonars[i].id);
            i++;
        }
        //   }
    }
    printf("\n");
	adr=96; //C0
    // if((ret=ioctl(file,I2C_SLAVE,adr))>=0){
    if(robovero.scanI2C(adr)>0){//i2c_smbus_write_byte(file, 0x00) >=0){
        printf("Compass at 0x%02x", 96*2);
        comp->id=adr;
        getrz(comp);
        comp->head_init=comp->rz[0];
    }
    // }
    
    //time(&sys_time);
    //today = localtime(&sys_time);
    //fin = today->tm_sec;
    //printf("\nTEMPS DE REACTION:%i",fin-deb);
    //printf("\n");
}





//     MAIN     ///

 int icantbelieveitsnotthemain(int argc, char **argv)
 {
     ros::init(argc,argv, "sensors_thruster");
     robovero.Init();
     //robovero.GYROzeroCalibrate(120,15); //# of data and millisecond to initiliaze
     //robovero.mag_calibration();
     
     ros::NodeHandle n;
     ros::Rate loop_rate(10);

     //      Declaration variables    //
     //sensors::imuros imuInst; //ros geo msg
     sensors::sonar sonarInst;
     sensors::sonarArray sArray;
     sensors::compass compInst;

     
     mode=2; //O off; 1 Manual; 2 stabilizing; 3 ps3;
     incx=0;incy=0;incz=0;
     geometry_msgs::Wrench Ftotal;
     sensors::motor motorInst;
     sensors::motorArray mArray;

     Foff.force.x=0;
     Foff.force.y=0;
     Foff.force.z=0;
     Foff.torque.x=0;
     Foff.torque.y=0;
     Foff.torque.z=0;
     
     Fcontrol=Foff;
     Fdyn=Foff; //added this after residency for thruster tests -P.A
     Fmanual=Foff;
     Ftotal=Foff;
     int print=0;

     printf("Ouverture bus I2C\n");


     //if ((file = open(DEVICE_I2C, O_RDWR)) < 0)
     //    perror("Erreur lors de l'ouverture");
     scan_i2c_motors(file, &mArray);
     scan_i2c_sensors(file, &sArray, &compInst);
     init_sonar(&sArray);
     
     ///              Publisher      ///
     //ros::Publisher pubI = n.advertise<sensors::imuros>("imu", 1);
     ros::Publisher pubS = n.advertise<sensors::sonarArray>("sonars", 1);
     ros::Publisher pubC = n.advertise<sensors::compass>("compass",1);
     ros::Publisher force_dyn_comp = n.advertise<geometry_msgs::Wrench>("final_forces",1);
     
     //             Subscriber      ////
       ros::Subscriber Command_int= n.subscribe("mode",1,modeinfo);
       ros::Subscriber control_vector= n.subscribe("command_control", 1, controlcallback);
       ros::Subscriber ps3_vector = n.subscribe("ps3_control",1, ps3callback);
       ros::Subscriber manual_vector= n.subscribe("manual_control", 1, manualcallback);
       ros::Subscriber force_vector=n.subscribe("dyn_comp_control",1,forcecallback);
	while(ros::ok())
	{
        	getdistance(&sArray);
		//getimudata(&imuInst);
		getrz(&compInst);
		pubS.publish(sArray);
		//pubI.publish(imuInst);
		if(compInst.id!=0){pubC.publish(compInst);}
        
        ///     Motor Commands     ///
        	if(print==16){ROS_INFO("mode : %d",mode);
		ROS_INFO("Ca marche (Ftota)l: fx:%f,fy:%f,fz:%f,tx:%f,ty:%f,tz:%f",Ftotal.force.x,Ftotal.force.y,Ftotal.force.z,Ftotal.torque.x,Ftotal.torque.y,Ftotal.torque.z);
		print=0;}
		else {++print;}
		switch(mode){
			case 0: forces2motors(Foff);
				force_dyn_comp.publish(Foff);
				break;
			case 1: forces2motors(Fmanual);
				force_dyn_comp.publish(Fmanual);
				break;
			case 2: Ftotal.force.x=Fcontrol.force.x+Fdyn.force.x;
				Ftotal.force.y=Fcontrol.force.y+Fdyn.force.y;
				Ftotal.force.z=Fcontrol.force.z+Fdyn.force.z;
				Ftotal.torque.x=Fcontrol.torque.x+Fdyn.torque.x;
				Ftotal.torque.y=Fcontrol.torque.y+Fdyn.torque.y;
				Ftotal.torque.z=Fcontrol.torque.z+Fdyn.torque.z;
				force_dyn_comp.publish(Ftotal);
				forces2motors(Ftotal);
				break;
			case 3: forces2motors(Fps3);
				force_dyn_comp.publish(Fps3);
				break;
			default:forces2motors(Foff);
				force_dyn_comp.publish(Foff);
				break;
		}
        ros::spinOnce();
		loop_rate.sleep();
	}
        return 0;
}

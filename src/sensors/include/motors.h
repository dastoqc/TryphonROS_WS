
// modele de controleur : ce parametre simplifie la creation des structures de donnees de motorisation
//#define	MOTORISATION_KOREMOTOR		// si defini, cree les structures pour 8 moteurs via KoreMotor, sinon via Devantech
#define	MOTORISATION_PICBL


// adresse I2C
#define MIN_CONTROLEUR				0
#define	ADR_DEVANTECH1				0xB0	
#define	ADR_DEVANTECH2				0xB2
#define	ADR_DEVANTECH3				0xB4
#define	ADR_DEVANTECH4				0xB6
#define	ADR_KOREMOTOR1				(0x0B << 1)		// For Motor X Left
#define	ADR_KOREMOTOR2				(0x0C << 1)		// For Motor Y Front
#define	ADR_KOREMOTOR3				(0x0D << 1)		// For Motor Z Back Left
#define	ADR_KOREMOTOR4				(0x0E << 1)		// For Motor Z Front Left
#define	ADR_KOREMOTOR5				(0x0F << 1)		// For Motor X right
#define	ADR_KOREMOTOR6				(0x10 << 1)		// For Motor Y Back
#define	ADR_KOREMOTOR7				(0x11 << 1)		// For Motor Z Back right
#define	ADR_KOREMOTOR8				(0x12 << 1)		// For Motor Z Front right



//-----------Code for PICBL Motorisation------------------------------
// I2C Adresses
#define	ADR_XLEFT				0xB0
#define	ADR_XRIGHT  				0xB2
#define	ADR_YFRONT  				0xB4
#define	ADR_YBACK  				0xB6
#define	ADR_ZFRIGHT  				0xB8
#define	ADR_ZBRIGHT  				0xBA
#define	ADR_ZBLEFT  				0xBC
#define	ADR_ZFLEFT  				0xBE
#define	ADR_XTLEFT  				0xA0
#define	ADR_XTRIGHT  				0xA2
#define	ADR_YTFRONT  				0xA4
#define	ADR_YTBACK  				0xA6
#define MAX_CONTROLEUR				12

// I2C Commands						// Must correspond to T_OPERATION in the PIC prgramm (PICBL)
#define	COMMAND_INIT				0x01
#define	COMMAND_SEND_RPM		        0x02
#define MOT_RPM					0x03
#define MOT_VOLT				0x04
#define MOT_CURR				0x05
#define MOT_TEMP				0x06
#define MOT_STOP				0x07
// Integer equivalent of 0b00010000 for forward - ESC Tchek
#define VIT_POS					112
// Integer equivalent of 0b10010000 for reverse - ESC 
#define VIT_NEG					240
//-------------------------------------------------------------------


// valeurs
//MAXPWR ou MAXAUT
#define MAXAUT

#define	VITESSE_MIN 				10
//#ifdef MAXPWR
#define	VITESSE_MAX 				255
//#else
//#define	VITESSE_MAX 				255//45*2  //added *2 to be between 0 and 255 
//#endif
#define	VITESSE_MAX_CONTROLEUR			127
#define	OFFSET_MOTEUR				0		// valeur a ajouter dans la direction negative pour la rendre equivalente a la positive
#define ACCELERATION_DEFAULT			0
#define	MAX_PALIER				256
#define	DUREE_PALIER				250	//50		// millisecondes

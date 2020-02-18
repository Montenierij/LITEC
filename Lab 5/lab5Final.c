/*
Lab 6

Group: Jordan Cabahug-Almonte, Jacob Montenieri, Kristen Weatherbee
Section: 4
Side: B
TA: Teng Liu

Description: For this lab we were tasked with driving a car up or down a ramp using
the accelerometer. Depending on the gx and gy values that are recieved from the 
accelerometer the car will either drive or turn more harshly if a higher value is returned
and the car will stop with the plane that it is on returns to its origin plane orientation.
*/
#include <stdio.h>
#include <c8051_SDCC.h>
#include<i2c.h>
#define PCA_start 28672
#define drive_pw_center 2765
#define drive_pw_max 3502
#define drive_pw_min 2028
#define steering_pw_center 2765
#define steering_pw_max 3100
#define steering_pw_min 1700
//-----------------------------------------------------------------------------
// 8051 Initialization Functions
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void SMB_Init (void);
void PCA_ISR ( void ) __interrupt 9;
void XBR0_Init(void);
void Accel_Init_C(void);
void read_accels (void); //Sets global variables gx & gy
void set_servo_PWM (void);
void set_drive_PWM(void);
void updateLCD(void);
void calibrate(void);
unsigned int pick_kdx(void);
unsigned int pick_ks(void);
unsigned char read_AD_input(unsigned char); 
void ADC_Init(void);
void buzz(void);
void pause(void);
unsigned char new_accel = 0; // flag for count of accel timing
unsigned char new_lcd = 0; // flag for count of LCD timing
unsigned int range;
unsigned char a_count; // overflow count for acceleration
unsigned char lcd_count; // overflow count for LCD updates
signed int avg_gx;
signed int avg_gy;
signed int cal_gx;
signed int cal_gy;
signed int gx;
signed int gy;
unsigned int ks,kdx,kdy;
short i;
short k;
short stopflag = 0;
unsigned char Data[4];
unsigned int steering_pw;
unsigned int drive_pw;
unsigned char choice;
unsigned int timer = 0;
unsigned char keypad;
unsigned int n_count = 0;
unsigned char buzzer_count = 0;
unsigned char AD_value;
unsigned int keypad_kdx;
__sbit __at (0xB7) run;
__sbit __at (0xB0) BUZZER;
//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void)
{
	unsigned char run_stop; // define local variables
	Sys_Init(); // initialize board
	putchar(' ');
	Port_Init();
	PCA_Init();
	XBR0_Init();
	SMB_Init();
	Accel_Init_C();
	ADC_Init();
	printf("All good");
	a_count = 0;
	lcd_count = 0;
	PCA0CP0 = 0xFFFF - drive_pw_center;
	PCA0CP1 = 0xFFFF - steering_pw_center;
	k=0;
	while(k<50);//Pause for 1s

	AD_value=read_AD_input(4);
	printf("AD_value: %u ", AD_value);
	//set gain  
	kdy = AD_value/5;
	printf("kdy: %u",kdy);

	
	calibrate(); //calibrates accelerometer at the beginning of the run

	//Sets gain values
	kdx = pick_kdx(); 
	ks = pick_ks();

	drive_pw = drive_pw_center;
	steering_pw = steering_pw_center;

	printf("kdx gain: %u, kdy gain: %u, ks gain: %u /r/n", kdx,kdy,ks);

	printf("Type '1' to go up hill or type '2' to go down hill\r\n");
	choice = getchar();
	while (1)
	{
		//kdx = 6, ks = 5, kdy = 3 for uphill for good run

		run_stop = 0;
		while (!run) // make run an sbit for the run/stop switch
		{ // stay in loop until switch is in run position
			if (run_stop == 0)	
			{
				//set_gains(); // function adjusting feedback gains
				run_stop = 1; // only try to update once
			}
		}
		if (new_accel) // enough overflows for a new reading
		{
			if(stopflag==0)//If we haven't set off flag
			{
				read_accels();
				set_servo_PWM(); // set the servo PWM
				set_drive_PWM(); // set drive PWM
				new_accel = 0;
				a_count = 0;
				printf("Steering pw: %u",steering_pw);
			}
			else//If we set off the flag to stop
			{
				PCA0CP0 = 0xFFFF - drive_pw_center;
				PCA0CP1 = 0xFFFF - steering_pw_center;
			}
		}
		if (new_lcd) // enough overflow to write to LCD
		{
			printf("gx: %d	 gy: %d\r\n",gx,gy);
		}
	}
}
//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
void Port_Init()
{
	P1MDIN &= ~0x40;
 	P1MDOUT &= ~0x40;
	P1 |= 0x40;
	P1MDOUT |= 0x03; //Set output pin for CEX0/CEX1 in push-pull mode (HOLE 12/13)
	P0MDOUT &= ~0xC0; //Sets 0.6 and 0.7
	P0 |= 0xC0; //Sets 0.6 and 0.7 to high impendence
	P3MDOUT &= ~0x80; //Allows the usage of Slide Switch
	P3MDOUT |= 0x01;
	P3 |= 0x80;
}
//-----------------------------------------------------------------------------
// XBR0_Init
//-----------------------------------------------------------------------------
void XBR0_Init()
{
	XBR0 = 0x27; //configure crossbar with UART, SPI, SMBus, and CEX channels as
	// in worksheet
}
//-----------------------------------------------------------------------------
// PCA_Init
//-----------------------------------------------------------------------------
void PCA_Init(void)
{
	//Use a 16 bit counter with SYSCLK/12.
	PCA0MD = 0x81; //Enable CF interrupt and SYSCLK/12
	PCA0CPM0 = PCA0CPM1 = 0xC2; //CEX0 is Driving CEX1 is Steering
	PCA0CN = 0x40;
	EA = 1;//Enables global interrupts
	EIE1 |= 0x08; //Enables PCA interrupt
}
//-----------------------------------------------------------------------------
// SMB_Init
//-----------------------------------------------------------------------------
void SMB_Init(void)
{
	SMB0CR = 0x93; //Set SCL to 100KHz

	ENSMB=1; //Enables SMBus
}
//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
void PCA_ISR ( void ) __interrupt 9
{
	if (CF)
	{
		CF = 0; // clear overflow indicator
		a_count++;
		timer++;
		n_count++;
		buzzer_count++;
		k++;
		if(a_count>=1)
		{	
			new_accel = 1;
			a_count = 0;
		}
		lcd_count++;
		if (lcd_count>=25)//might need to change
		{
			new_lcd = 1;
			lcd_count = 0;
		}
		PCA0 = PCA_start;
	}
	// handle other PCA interrupt sources
	PCA0CN &= 0xC0;
}
//-----------------------------------------------------------------------------
// Initializing ADC
//-----------------------------------------------------------------------------
void ADC_Init(void)
{
 	REF0CN = 0x03; //Necessary 3 for control register
	ADC1CF |= 0x01;
	ADC1CF &= ~0x02; //sets gain to 1
 	ADC1CN = 0x80; //Enables A/D converter
}
//-----------------------------------------------------------------------------
// read_AD_input
//-----------------------------------------------------------------------------
unsigned char read_AD_input(unsigned char pin_number)
{
 	AMX1SL = pin_number; //Sets pin_number to analog for ADC1
	ADC1CN &= ~0x20; //Clears conversion complete flag
	ADC1CN |= 0x10; //Starts conversion
	while((ADC1CN & 0x20) == 0x00); //ends conversion
	return ADC1; //returns ADC value
}
//-----------------------------------------------------------------------------
// calibration
//-----------------------------------------------------------------------------
void calibrate(void)//This method will give us a base value that is our origin at the beginning of every trial
{	
	cal_gx=0;
	cal_gy=0;
	k=0;
	while(k<2);
	for(i=0;i<64;i++)
	{
		do
		{
			Data[0] = 0x00;
			i2c_read_data(0x3A,0x27,Data,0x01);
			k=0;
			while(k<2);
		}while((Data[0]&0x03)!=0x03);
		i2c_read_data(0x3A,0x28|0x80,Data,0x04);
		cal_gx += ((Data[1] << 8) >> 4); 
		cal_gy += ((Data[3] << 8) >> 4);
	}	
	cal_gx=cal_gx/64;
	cal_gy=cal_gy/64;
}
//-----------------------------------------------------------------------------
// read_accels
//-----------------------------------------------------------------------------
void read_accels()
{	
	avg_gx=0;
	avg_gy=0;
	for(i=0;i<8;i++)
	{
		do
		{
			Data[0] = 0x00;
			i2c_read_data(0x3A,0x27,Data,0x01);
			k=0;
			while(k<2);
		}while((Data[0]&0x03)!=0x03);
		i2c_read_data(0x3A,0x28|0x80,Data,0x04);
		avg_gx += ((Data[1] << 8) >> 4); 
		avg_gy += ((Data[3] << 8) >> 4);
	}
	avg_gx=avg_gx >> 3; //Averages the values
	avg_gy=avg_gy >> 3;
	gx=avg_gx-cal_gx; //Assigns global variable the corrected value of gx based on calibration
	gy=avg_gy-cal_gy; //Assigns global variable the corrected value of gy based on calibration
}
//--------------------------------------------------------------------------
// set_sevo_PWM
//-----------------------------------------------------------------------------
void set_servo_PWM()
{	
	if (choice == '1')//Choice to go uphill
	{
		steering_pw=steering_pw_center+ks*gx;
	}
	else if (choice == '2')//Downhill
	{
		steering_pw=steering_pw_center-ks*gx;
	}
	PCA0CP1 = 0xFFFF - steering_pw;
}
//-----------------------------------------------------------------------------
// set_drive_PWM
//-----------------------------------------------------------------------------
void set_drive_PWM()
{
	if ((gy>-20 && gy<20) && (gx <10 && gx>-10))//50 gy for downhill
		stopflag = 1;	
	if(stopflag == 1)
	{
		PCA0CP0 = 0xFFFF - drive_pw_center;
		buzzer_count = 0;
		buzz();
	}
	if (gy < 0  && stopflag == 0)
	{
		drive_pw=drive_pw_center-kdy*gy;
	}
	else if (gy >= 0 && stopflag == 0)
	{
		gy*=-1;
		drive_pw=drive_pw_center-kdy*gy;
	}

	drive_pw += kdx * abs(gx);

	if (drive_pw < drive_pw_center && stopflag == 0)
	{
		drive_pw += (drive_pw_center -drive_pw);
	}
	PCA0CP0 = 0xFFFF - drive_pw;
}
//-----------------------------------------------------------------------------
// Pick Heading
//-----------------------------------------------------------------------------
unsigned int pick_kdx(void)
{

	lcd_clear();
	lcd_print("Pick kdx");
	while(read_keypad() == 0xFF)
	{
		pause();
	}

	//reads from keypad
	keypad = read_keypad();
	pause();
	while(read_keypad() != 0xFF)
	{
		pause();
	}

	//if keypad reads 1, enter another key for predefined heading
	if (keypad == '1')
	{
		lcd_clear();
        lcd_print("Your key was: 1, pick 0, 5,7, or 9");

		//sets predefined heading
		while(read_keypad() == 0xFF)
		{
			pause();
		}
		keypad_kdx = read_keypad();
		pause();
		if 	(keypad_kdx == '0')
		{
			keypad_kdx = 0;
			lcd_clear();
        	lcd_print("Your keypad kdx was: 0");
		}
		else if (keypad_kdx == '5')
		{
			keypad_kdx = 5; 
			lcd_clear();
        	lcd_print("Your keypad kdx was: 5");
		}
		else if (keypad_kdx == '7')
		{
			keypad_kdx = 10;
			lcd_clear();
        	lcd_print("Your keypad kdx was: 7");
		}
		else if (keypad_kdx == '9')
		{
			keypad_kdx = 15;
			lcd_clear();
        	lcd_print("Your keypad kdx was: 9");
		}

		while(read_keypad() != 0xFF)
		{
			pause();
		}
		kdx = keypad_kdx;

	}

	else
	{
		//sets specific desired angle
		//read keypad from 4 keys
		kdx = kpd_input(0);
	}
	return kdx;
}
//-----------------------------------------------------------------------------
// pick_ks
//-----------------------------------------------------------------------------
unsigned int pick_ks(void)
{
	
	lcd_clear();
	lcd_print("Pick ks");

	while(read_keypad() == 0xFF)
	{
		pause();
	}

	//reads from keypad
	keypad = read_keypad();
	pause();
	while(read_keypad() != 0xFF)
	{
		pause();
	}

	ks = kpd_input(0);
	return ks;
}
//-----------------------------------------------------------------------------
// Pause
//-----------------------------------------------------------------------------
void pause(void)
{
    n_count = 0;
    while (n_count < 1);// 1 count -> (65536-PCA_START) x 12/22118400 = 20ms
} 
void buzz(void)
{
	while (buzzer_count <= 50)//Turns buzzer on for one second
	{
		BUZZER = 0;
	}
	BUZZER = 1;
}
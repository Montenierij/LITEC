/*

Lab 4

Group: Jordan Cabahug-Almonte, Jacob Montenieri, Kristen Weatherbee
Section: 4
Side: B
TA: Teng Liu

Description:
In this lab we use the compass and ultrasonic ranger, the wireless transceiver, analog-to-digital conversion,
the Programmable Counter Array (PCA), and the LCD keypad. The LCD keypad was used to enter and display
information without using PuTTY. The wireless RF transceiver was used to allow the Smart Car to run without
being wired to a laptop. Combining these components our task was to back up the car, drive towards the wall 
and turn before it hits the wall. Then, continue along the wall until it has to stop
when about to hit another wall.

*/


//-----------------------------------------------------------------------------
// Compiler Directives
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <c8051_SDCC.h>
#include<i2c.h>
#include<math.h>
#define PW_MIN 2028
#define PW_NEUT 2765
#define PW_drive 3300;
#define servo_min 1700 
#define servo_max 3100
#define servo_neut 2765
#define PCA_START 28672

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned short MOTOR_PW = 0;
char flag = 0;
unsigned int range;
unsigned char in_range = 60;
unsigned char stop_distance = 20;
unsigned char Data[2];
unsigned char addr = 0xE0;
unsigned int desired_heading;
unsigned int time =0;
unsigned char time_count = 0;
unsigned char h_count=0;
unsigned int n_count = 0;
unsigned char r_count =0;
unsigned char p_count = 0;
unsigned char print;
unsigned char new_heading;
signed int heading;
unsigned int Steering_PW;
unsigned char keypad;
float k;
unsigned char AD_value;
unsigned char inputchar;
unsigned char new_range;
unsigned int keypad_ang;
unsigned char backingFlag = 0;

__sbit __at (0xB7) RUN;


//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init(void);
void SMB_Init(void);
void ADC_Init(void);
void PCA_ISR(void) __interrupt 9;
unsigned int ReadRanger(void); 
void DriveMotor(unsigned int); 
unsigned int ReadCompass(void); 
void Steering_Servo(void); 
unsigned int pick_heading(void);
void BackMotor(unsigned int); 
unsigned char read_AD_input(unsigned char); 
void wait(void);
void pause(void);
void WarmUp(void);


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
void main(void)
{
	
	Sys_Init(); // initialize board
	putchar(' ');
	Port_Init();
	PCA_Init();
	SMB_Init();
	ADC_Init();

	printf("Hello\r\n");
	//read ad_value off of port 1 pin 6
	AD_value=read_AD_input(6);//6 is for port 1 pin 6
	printf("AD_value: %u ", AD_value);
	printf("Heading: %u", heading);

	//set gain
	k = AD_value/5;


	//set motor pulsewidth and steering pulsewidth to neutral
	MOTOR_PW = PW_NEUT;
	PCA0CP2 = 0xFFFF - MOTOR_PW;
	Steering_PW = PW_NEUT;
	PCA0CPL0 = 0xFFFF - Steering_PW;
	PCA0CPH0 = (0xFFFF - Steering_PW) >> 8;

	wait();
	WarmUp();
	WarmUp();

	//set desired heading
	desired_heading = pick_heading();

	while (1)
	{



		 // stay in loop until switch is in run position
		 while (!RUN)
		 { 
			

		//if finding new range
			if (new_range) // enough overflow for a new range
			{
				range = ReadRanger(); // get range
				//start a ping
				//write i2c_data
				Data[0] = 0x51;
				i2c_write_data(addr,0x00,Data,0x01);
			
				//if going backwards 
				if (!backingFlag)
				{
					BackMotor(range);
				}

				//if going fowards
				if (backingFlag)
				{
					DriveMotor(range);
				}
				//reset new range
				new_range = 0;

				Steering_Servo();

				//print time, heading, range, motor pulsewidth, and steering pulsewidth
				//prints every 400ms
				if (print)
				{
		
					printf("%u, %d, %u, %u,  %u\r\n" , time, heading, range, MOTOR_PW, Steering_PW);
					print = 0;

				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Pick Heading
//-----------------------------------------------------------------------------
unsigned int pick_heading(void)
{


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
        lcd_print("Your key was: 1");

		//sets predefined heading
		while(read_keypad() == 0xFF)
		{
			pause();
		}
		keypad_ang = read_keypad();
		pause();
		if 	(keypad_ang == '0')
		{
			desired_heading = 2700;
			lcd_clear();
        	lcd_print("Your keypad ang was: 0");
		}
		else if (keypad_ang == '5')
		{
			desired_heading = 900; 
			lcd_clear();
        	lcd_print("Your keypad ang was: 5");
		}
		else if (keypad_ang == '7')
		{
			desired_heading = 1800;
			lcd_clear();
        	lcd_print("Your keypad ang was: 7");
		}
		else if (keypad_ang == '9')
		{
			desired_heading = 0;
			lcd_clear();
        	lcd_print("Your keypad ang was: 9");
		}

		while(read_keypad() != 0xFF)
		{
			pause();
		}
		desired_heading = keypad_ang;

	}

	else
	{
		//sets specific desired angle
		//enters in value and press # to enter the heading
		desired_heading = kpd_input(0);
	}

	return desired_heading;

}



//-----------------------------------------------------------------------------
// Initializing Function
//-----------------------------------------------------------------------------
void Port_Init(void)
{
    XBR0 = 0x27;    // NOTE: Only UART0 & SMB enabled; SMB on P0.2 & P0.3
	//for AD
	P1MDIN &= ~0x40;
 	P1MDOUT &= ~0x40;
 	P1 |= 0x40;
	P1MDOUT |= 0x0F; ;//set output pport 1.0 for CEX0 in push-pull mode
	P0MDOUT	 &= ~0xC0; //set input pins for Compass in open-drain mode
    P0 |= 0xC0; //set input pins for Compass in open-dran mode
	P3MDOUT &= ~0x80; //sets slide switch in open-drain mode
	P3 |= 0x80; //sets input pins to high impedance
}                   // No CEXn are used; no ports to initialize



//-----------------------------------------------------------------------------
// Initializing PCA
//-----------------------------------------------------------------------------
void PCA_Init(void)
{
	PCA0CPM0 = PCA0CPM2 = 0xC2; //CCM0 in 16-bit mode
	PCA0CN = 0x40; //Enable PCA counter
	PCA0MD = 0x81; //sets to SYSCLK/12 and enables CF interrupt
	EA = 1;//Enables global interrupts
	EIE1 |= 0x08; //Enables PCA interrupt
}


//-----------------------------------------------------------------------------
// Initializing SMB
//-----------------------------------------------------------------------------
void SMB_Init(void)    // This was at the top, moved it here to call wait()
{
    SMB0CR = 0x93;      // Set SCL to 100KHz
    ENSMB = 1;          // Enable SMBUS0
}



//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
void PCA_ISR(void) __interrupt 9
{
	if (CF)
	{
		CF = 0; // clear overflow indicator
		h_count++;
		n_count++;
		time_count++;
		if (time_count>=1)
		{
			time +=20;
			time_count = 0;
		}

		if (h_count>=2)	
		{
			new_heading=1;
			h_count = 0;
		}

		r_count++;
		if (r_count>= 4) //when to read ranger
		{
			new_range = 1;
			r_count = 0;
		}

		p_count++;

		if (p_count>=20)
		{
			print = 1;
			p_count = 0;
		}

		PCA0 = PCA_START;
		}
		// handle other PCA interrupt sources
		PCA0CN &= 0xC0;
}


//-----------------------------------------------------------------------------
// Pause
//-----------------------------------------------------------------------------
void pause(void)
{
    n_count = 0;
    while (n_count < 1);// 1 count -> (65536-PCA_START) x 12/22118400 = 20ms
}                       // 6 counts avoids most of the repeated hits


//-----------------------------------------------------------------------------
// Wait
//-----------------------------------------------------------------------------
void wait(void)
{
    n_count = 0;
    while (n_count < 50);    // 50 counts -> 50 x 20ms = 1000ms
}


//-----------------------------------------------------------------------------
// Read Ranger
//-----------------------------------------------------------------------------
unsigned int ReadRanger()
{
	range=0;
	i2c_read_data(addr,0x02,Data,0x02); //reads range of ping
	range=(((unsigned int)Data[0] << 8) | Data[1]);
	return range;
}
//-----------------------------------------------------------------------------
// WarmUp
//-----------------------------------------------------------------------------
void WarmUp()
{
//waits 80ms
	pause();
	pause();
	pause();
	pause();
	range=ReadRanger(); //Find the range of the ping, first value will be useless because there is no ping
	Data[0] = 0x51; //sets value of range to be in cm
	i2c_write_data(addr,0x00,Data,0x01);
}
//-----------------------------------------------------------------------------
// Backing Motor
//-----------------------------------------------------------------------------
void BackMotor(unsigned int rangePassed)
{
	if(rangePassed<80)
	{
		MOTOR_PW = PW_MIN;
		PCA0CP2 = 0xFFFF - MOTOR_PW; //Sets the value into the motor
	}
	else if(rangePassed>=80 && rangePassed<255)
	{
		MOTOR_PW = PW_NEUT;
		PCA0CP2 = 0xFFFF - MOTOR_PW; //Sets the value into the motor
		backingFlag = 1;
	}
}
//-----------------------------------------------------------------------------
// Drive Motor
//-----------------------------------------------------------------------------
void DriveMotor(unsigned int rangePassed)
{
	if(rangePassed<20)
	{
		MOTOR_PW= PW_NEUT;
		PCA0CP2 = 0xFFFF - MOTOR_PW; //Sets the value into the motor
	}
	else
	{
		MOTOR_PW = PW_drive; //This is 1/8 of the difference between max and neutral added to neutral
		PCA0CP2 = 0xFFFF - MOTOR_PW; //Sets the value into the motor
	}

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
// Steering_Servo
//-----------------------------------------------------------------------------
void Steering_Servo(void)
{	signed int error;

	if (new_heading)
	{
		heading = ReadCompass(); //reads heading from the compass
		error = desired_heading - heading;
		//limits error from 0 to 180, allows for car to take shortest path
		if (error < -1800)
		{
			error +=3600;
		}
		else if (error > 1800)
		{
			error -=3600;
		}

		
		  //change with AD val
		   //sets steering pulsewidth
		if ((range < in_range) && backingFlag)
		{
			Steering_PW = servo_neut - (k * (in_range - range)) + error;
		}
		else if (backingFlag)
		{
			Steering_PW = servo_neut + error;
		}
		//adjusts wheels so that is pointing to desired heading
		
		if (Steering_PW < servo_min)
		{
			Steering_PW = servo_min;
		}
		
		else if (Steering_PW > servo_max)
		{
			Steering_PW = servo_max;
		}

		new_heading = 0;
		
	PCA0CPL0 = 0xFFFF - Steering_PW;
	PCA0CPH0 = (0xFFFF - Steering_PW) >> 8;
	}
}

//-----------------------------------------------------------------------------
// ReadCompass
//-----------------------------------------------------------------------------

unsigned int ReadCompass(void)
{
	//local variables
	unsigned char addr = 0xC0; //address of the compass
	unsigned char Data[2]; //reads a 2 byte data array
	unsigned int heading; //heading that's read from compass
		
			 
	i2c_read_data(addr, 0x02, Data, 2); //reads data
	//address, start register (2), buffer, and number of bytes read
	//reads from registers 2 and 3 and heading is from 0-3599
		
			
	heading = ((unsigned int)Data[0]<<8 | Data[1]); //combines two values
	
	return heading; //heading is from 0-3599
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
/*
Lab 6

Group: Jordan Cabahug-Almonte, Jacob Montenieri, Kristen Weatherbee
Section: 4
Side: B
TA: Teng Liu

Description: For this lab we were tasked with turning a Gondola using its side fans
one at a time only. To do this we have to use our knowledge on the XBAR for the C8051
and use the equation from worksheet 11 to help us equate the motor pulse width for 
the fans while using the proportional and damping gains.
*/
//compiler directives
#include <stdio.h>
#include <c8051_SDCC.h>
#include<i2c.h>
#include<math.h>
#define PW_MIN 2028
#define PW_NEUT 2765
#define PW_drive 3300;
#define servo_min 2028
#define servo_max 3502
#define servo_neut 2765
#define PCA_START 28672
#define FAN_ANGLE 3095
//define global variables
char flag = 0;
unsigned int range;
unsigned char Data[2];
unsigned char addr = 0xE0;
signed int desired_heading;
unsigned int FIRSTdesired_heading;
unsigned char h_count=0;
unsigned char r_count =0;
unsigned char new_heading;
signed int heading;
unsigned char new_range;

signed long Fan_PW;
float kp, kd;
signed int prev_error;
unsigned char new_print;
unsigned char p_count;
unsigned char ADVal;
unsigned char k;
int volt;
unsigned int time = 0;


//function prototypes
void Port_Init(void);
void PCA_Init(void);
void SMB_Init(void);
void PCA_ISR(void) __interrupt 9;
unsigned int ReadRanger(void); 
void DriveMotor(unsigned int); 
signed int ReadCompass(void); 
void Steering_Servo(void); 
unsigned int pick_heading(void);
void BackMotor(unsigned int); 
unsigned char read_AD_input(unsigned char); 
void WarmUp(void);
void pick_val(void);
unsigned int ReadRanger(void);
void RangeAndHeading (void);
void ADC_Init(void);


void main(void)
{

	Sys_Init(); // initialize board
	putchar(' ');
	Port_Init();
	PCA_Init();
	SMB_Init();
	ADC_Init();
	getchar();

	//set motor pulsewidth and steering pulsewidth to neutral
	PCA0CP2 = PCA0CP3 = 0xFFFF - PW_NEUT;
	PCA0CP1 = 0xFFFF - FAN_ANGLE;

	prev_error = 0;
	k=0;
	while(k<50);
	WarmUp();
	WarmUp();
	ADVal = read_AD_input(3);
	volt = 4.235 * (2.4 * ADVal)/255;
	volt = volt * 1000;
	printf("Battery Voltage in milivolts: %u\r\n", volt);
	pick_val();

	while(1)
	{	
		RangeAndHeading();
		while (desired_heading >3600)
		{
			desired_heading -=3600;
		}
		while (desired_heading <0)
		{
			desired_heading +=3600;
		}
		if (range > 100)
		{
			range = 100;
		}
		desired_heading = (36 * range - 1800) + FIRSTdesired_heading;
		
		Steering_Servo();
		if (new_print == 1) 
		{
			printf("range: %d \r\n", range);
			printf("desired heading: %d \r\n", desired_heading);
			printf("heading: %d \r\n", heading);
			printf("time: %d \r\n", time);
			new_print = 0;
		}	
	}
}


//-----------------------------------------------------------------------------
// Initializing Function
//-----------------------------------------------------------------------------
void Port_Init(void)
{
    XBR0 = 0x25;    //P0.4, P0.5, P0.6 and P0.7
	//for AD
	P1MDIN &= ~0x40;
 	P1MDOUT &= ~0x40;
 	P1 |= 0x40;
	P1MDOUT |= 0x0F; ;//set output pport 1.0 for CEX0 in push-pull mode
	P0MDOUT	 &= ~0xF0; //set input pins for Compass in open-drain mode
    P0 |= 0xF0; //set input pins for Compass in open-dran mode
	P3MDOUT &= ~0x80; //sets slide switch in open-drain mode
	P3 |= 0x80; //sets input pins to high impedance
}                   // No CEXn are used; no ports to initialize



//-----------------------------------------------------------------------------
// Initializing PCA
//-----------------------------------------------------------------------------
void PCA_Init(void)
{
	PCA0CPM1 = PCA0CPM2 = PCA0CPM3 = 0xC2; //CCM0 in 16-bit mode
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
// Waiting, ISR
//-----------------------------------------------------------------------------
void PCA_ISR(void) __interrupt 9
{
	if (CF)
	{
		CF = 0; // clear overflow indicator
		PCA0 = PCA_START;
		h_count++;
		p_count++;
		k++;


		if (h_count>=4)	
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
		if(p_count > 50) {
			new_print = 1;
			p_count = 0;
			time+=1;
		}

		
		}
		// handle other PCA interrupt sources
		PCA0CN &= 0x40;
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
	k=0;
	while(k<4);
	range=ReadRanger(); //Find the range of the ping, first value will be useless because there is no ping
	Data[0] = 0x51; //sets value of range to be in cm
	i2c_write_data(addr,0x00,Data,0x01);
}
//-----------------------------------------------------------------------------
// Steering_Servo
//-----------------------------------------------------------------------------
void Steering_Servo(void)
{	signed int error;

	if (new_heading)
	{
		heading = ReadCompass(); //reads heading from the compass

		error = desired_heading - heading;//Next if statements are to take most efficent path
		if (error < -1800)
		{
			error +=3600;
		}
		else if (error > 1800)
		{
			error -=3600;
		}

		//testing new equation
		Fan_PW =(signed long)kp * (signed long)(error) + (signed long)kd *(signed long)(error - prev_error);

		if (Fan_PW < 0)
		{
			PCA0CP2 = 0xFFFF - servo_neut;
			Fan_PW *= -1;
			Fan_PW +=  (signed long)servo_neut;
			if (Fan_PW > servo_max)
				PCA0CP3 = 0xFFFF - servo_max;
			else
				PCA0CP3 = 0xFFFF - Fan_PW;
		}

		else if (Fan_PW > 0)
		{
			PCA0CP3 = 0xFFFF - servo_neut;
			Fan_PW += (signed long)servo_neut;
			if (Fan_PW > servo_max)
				PCA0CP2 = 0xFFFF - servo_max;
			else
				PCA0CP2 = 0xFFFF - Fan_PW;
		}

		else if (heading == desired_heading)
		{
			PCA0CP3 = 0xFFFF - servo_neut;
			PCA0CP2 = 0xFFFF - servo_neut;
		}
		new_heading = 0;
		prev_error = error;
	}
}
//-----------------------------------------------------------------------------
// ReadCompass
//-----------------------------------------------------------------------------
signed int ReadCompass(void)
{
	//local variables
	unsigned char addr = 0xC0; //address of the compass
	unsigned char Data[2]; //reads a 2 byte data array
	signed int heading; //heading that's read from compass
		
			 
	i2c_read_data(addr, 0x02, Data, 2); //reads data
	//address, start register (2), buffer, and number of bytes read
	//reads from registers 2 and 3 and heading is from 0-3599
		
			
	heading = ((unsigned int)Data[0]<<8 | Data[1]); //combines two values
	
	return heading; //heading is from 0-3599
}
//-----------------------------------------------------------------------------
// Pick Value
//-----------------------------------------------------------------------------
void pick_val(void)
{
	lcd_clear();
    lcd_print("Pick desired heading, kp, then kd");
	FIRSTdesired_heading = kpd_input(0);
	kp = kpd_input(0);
	kd = kpd_input(0);
}
//-----------------------------------------------------------------------------
// RangeAndHeading
//----------------------------------------------------------------------------- 	
void RangeAndHeading(void)

{	if (new_range)
	{
		range = ReadRanger();
		Data[0] = 0x51;//Gets value in cm
		i2c_write_data(addr,0x00,Data,0x01);
		new_range = 0;
	}	
}
//-----------------------------------------------------------------------------
// ReadADINPUT
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
// Initializing ADC
//-----------------------------------------------------------------------------
void ADC_Init(void)
{
 	REF0CN = 0x03; //Necessary 3 for control register
	ADC1CF |= 0x01;
	ADC1CF &= ~0x02; //sets gain to 1
 	ADC1CN = 0x80; //Enables A/D converter
}
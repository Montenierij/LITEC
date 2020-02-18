/**
	Group Partners: Jordan Cabahug-Almonte, Jacob Montenieri, Kristen Weatherbee
	Section:4 B 25
	Date: 02/25/2018
	File name: Lab2.c

*/
#include <c8051_SDCC.h>// include files. This file is available online
#include <stdio.h>
#include <stdlib.h>


//-----------------------------------------------------------------------------
// Function Prototypes CHANGE THIS BEFORE RUNNING
//-----------------------------------------------------------------------------
void ADC_Init(void); //initializes AD Conversion
void Port_Init(void); //initializes Ports 1,2, and 3
void Timer_Init(void); //initializes Timer
void Interrupt_Init(void); //initializes interrupts
void Timer0_ISR(void) __interrupt 1; //increments different counts
unsigned char read_AD_input(unsigned char pin_number);//returns AD value
float dot_time_calculator(int value); //calculates dot time
void Rand_Seed(void); //seeds random character
void Mode2 (void);
void Mode1 (void);
int Get_Rand(void); //gets random number from 0-9
void buzz(int type);//buzz function for mode 1
void Mode2print(int random);//print statement for mode 2
unsigned int check(unsigned int input [4], unsigned int rand); //checks if answer for mode 2 is right
void morsecode(char letter); //uses cases to sound buzzer accordingly for each letter
void buzz2(int buzz_type); //buzz function for mode 2




//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

__sbit __at 0xB3 BILED0; // BILED0, associated with Port 3 Pin 3
__sbit __at 0xB4 BILED1; // BILED1, associated with Port 3 Pin 4
__sbit __at 0xB7 BUZZER; // Buzzer, associated with Port 3 Pin 7
__sbit __at 0xA0 SS;   // Slide switch, associated with Port 2 Pin 0
__sbit __at 0xB0 PBEnter;  // Push button 1, associated with Port 3, Pin 0
__sbit __at 0xB1 PBDot; // Push button 2, associated with Port 3 Pin 1
__sbit __at 0xB2 PBDash; //Push button 3, associated with Port 3 Pin 2



unsigned int counts2; //counts for mode 2
int AD_value;
int i; //index value
int j; //index value to clera input array
int k; //for the 4 loop for lopp
unsigned int input[4]; //input array for mode 2
unsigned char inputrand; //gets random number from getchar for mode 1
unsigned int overflow_count;
float dotTime; //how long buzzer should buzz
int generatedAnswer; //gets random number from Get_Rand for mode 2
int flag; //for mode 2 to exit while loop
unsigned int randomnum = 0;
char randchar;
unsigned int game_score = 0; //calculates round score
unsigned int final_score = 0; //calculates entire score for 4 rounds
unsigned int saved_overflow = 0;
unsigned int overflowCount =0;


//-----------------------------------------------------------------------------
// Main Function  CHANGE THIS BEFORE RUNNING
//-----------------------------------------------------------------------------
void main(void)
{
	Sys_Init();      // System Initialization
	putchar(' ');
	Port_Init();	//Port Initialization
	ADC_Init();		//ADC Initialization
	Timer_Init();	//Timer Initialization
	Interrupt_Init(); //Interrupt Initialization


	printf("Start \r\n");
	BUZZER = 1; //Turns off buzzer

    while (1) 				//Main loop to add to
    {

		BUZZER = 1; //turns off buzzer
		//turns off BILED
		BILED0 = 1;
		BILED1 = 1;
		printf("\r\nTurn slide switch off for mode 1\r\nTurn slide switch on for mode 2\r\n(adjust potentiometer for different speed) \r\n");
		printf("Press the enter pushbutton when ready to play: \r\n");
		while(PBEnter); //When PB Enter is pressed and released
		AD_value=read_AD_input(5);//5 is for reading ADC Value on port 1 pin 5
		printf("Digital value is: %d\r\n",AD_value); //prints AD_value
		dotTime=dot_time_calculator(AD_value); //calculates dotTime
		printf_fast_f("Dot time is: %4.3f\r\n",dotTime);
		Rand_Seed(); //seeds random number
		if(SS)
		{//plays mode 1
			printf("Playing mode 1: \r\n");
			Mode1();
		}
		else
		{//plays mode 2
			printf("Playing mode 2: \r\n");
			Mode2();
		}
    }
}


//-----------------------------------------------------------------------------
// Functions   CHANGE THIS BEFORE RUNNING
//-----------------------------------------------------------------------------

void Port_Init(void)
{
	// Port 1 Initialization (for analog to digital conversion)
 	P1MDIN &= ~0x20;
 	P1MDOUT &= ~0x20;
 	P1 |= 0x20;
	// Port 2 Initialization (for Slide Switch)
	P2MDOUT&=0xFE;
    P2|=~0xFE;
	// Port 3 Initialization (for PBEnter, PBDash, PBDot, LED's and BILED's)
	P3MDOUT |=0xF8;
  	P3MDOUT &=~0x07;
 	P3 |=0x07;
}

//***************
void ADC_Init(void)
{
 	REF0CN = 0x03; //Necessary 3 for control register
	ADC1CF |= 0x01;
	ADC1CF &= ~0x02; //sets gain to 1
 	ADC1CN = 0x80; //Enables A/D converter
}

//***************
unsigned char read_AD_input(unsigned char pin_number)
{
 	AMX1SL = pin_number; //Sets pin_number to analog for ADC1
	ADC1CN &= ~0x20; //Clears conversion complete flag
	ADC1CN |= 0x10; //Starts conversion
	while((ADC1CN & 0x20) == 0x00); //ends conversion
	return ADC1; //returns ADC value
}

//***************
float dot_time_calculator(int value)
{
	return (float)((value*.0035294118)+.1);//This will change the value from 0 to 1 second based on 0-255 of ADC1
}
//***************
void Rand_Seed(void)
//seeds random character
{
	printf("press any button on the keyboard to seed the number generator: ");
	inputrand=getchar();
	srand(inputrand);
}

//***************
int Get_Rand(void)
//gets random number from 0 to 9
{
	return rand()%10;
}



//***************

//Mode1
void Mode1 (void)
{
    unsigned char input;
	//clears scores
	final_score =0;
	game_score = 0;

	for (i=0; i<4; ++i){
	//four rounds
		printf("The board will generate morse code for a letter from a to j. Type the letter that is chosen when prompted\r\n");
		//turns off BILED's
		BILED0 = 1;
		BILED1 = 1;
		//clears game_score for the round
		game_score =0;
		randomnum = Get_Rand();

		//returns morse code on the buzzer depending on the random number generated
		if (randomnum == 0){
			morsecode('a');
			randchar = 'a';
		}
		if (randomnum == 1){
			morsecode('b');
			randchar = 'b';
		}
		if (randomnum == 2){
			morsecode('c');
			randchar = 'c';
		}
		if (randomnum == 3){
			morsecode('d');
			randchar = 'd';
		}
		if (randomnum == 4){
			morsecode('e');
			randchar = 'e';
		}
		if (randomnum == 5){
			morsecode('f');
			randchar = 'f';
		}
		if (randomnum == 6){
			morsecode('g');
			randchar = 'g';
		}
		if (randomnum == 7){
			morsecode('h');
			randchar = 'h';
		}
		if (randomnum == 8){
			morsecode('i');
			randchar = 'i';
		}
		if (randomnum == 9){
			morsecode('j');
			randchar = 'j';
		}

		//stops Timer
		TR0 = 0;
		//clears count
		overflowCount = 0;
		//starts Timer
		TR0 = 1;

		printf("Enter answer: \r\n");
		input = getchar();
		printf("\r\n");


		TR0 = 0;
		//saves overflow for time penalty generation
		saved_overflow = overflowCount;

		//if key is correct, BILED turns green
		if (input == randchar){
			BILED0 = 0;
			BILED1 = 1;
			printf("Correct \r\n");
		//if key is incorrect, BILED turns red, and 10 penalty points are added
		} else {
			BILED1 = 0;
			BILED0 = 1;
			game_score+=10;
			printf("Inorrect \r\n");
		}

		//time penalty points added to game_score
		//adds one point to game_score for every half a second it takes to input an answer
		if (saved_overflow/169 > 0){
			game_score += saved_overflow/169;
		}

		//adds game_score from round to final score
		final_score += game_score;

		//displays the score so far and the score for the round
		printf("Total points for this try: %u \r\n",game_score);
		printf("Total score: %u \r\n",final_score);
		overflowCount = 0;
		TR0 = 1;
		//turns off BILED's after half of a second
		while (overflowCount < 169);//delay for half a second
		BILED0 = 1;
		BILED1 = 1;
	}


	printf("Final score: %u \r\n",final_score);
	overflowCount=0;
	overflow_count=0;
	//BILEDs blink red and green for 1 second
	while (overflowCount < 338)
	{
			BILED0 = 0;
			BILED1 = 1;
			BILED1 = 0;
			BILED0 = 1;
	}

}


//***************
void morsecode(char letter){
	//for each case (letter) plays dots/dashes for corresponding letter
	//dots = 1, dash = 2, space = 0
	switch(letter){

	case 'a': //sounds morse code for A
			  buzz(1);
			  buzz(2);
			  buzz(0);
			  buzz(0);
			  break;

	case 'b': //sounds morse code for B
			  buzz(2);
			  buzz(1);
			  buzz(1);
			  buzz(1);
			  break;

	case 'c': //sounds morse code for C
			  buzz(2);
			  buzz(1);
			  buzz(2);
			  buzz(1);
			  break;


	case 'd': //sounds morse code for D
			  buzz(2);
			  buzz(1);
			  buzz(1);
			  buzz(0);
			  break;

	case 'e': //sounds morse code for E
			  buzz(1);
			  buzz(0);
			  buzz(0);
			  buzz(0);
			  break;

	case 'f': //sounds morse code for F
			  buzz(1);
			  buzz(1);
			  buzz(2);
			  buzz(1);
			  break;


	case 'g': //sounds morse code for G
			  buzz(2);
			  buzz(2);
			  buzz(1);
			  buzz(0);
			  break;


	case 'h': //sounds morse code for H
			  buzz(1);
			  buzz(1);
			  buzz(1);
			  buzz(1);
			  break;

	case 'i': //sounds morse code for I
			  buzz(1);
			  buzz(1);
			  buzz(0);
			  buzz(0);
			  break;

	case 'j': //sounds morse code for J
			  buzz(1);
			  buzz(2);
			  buzz(2);
			  buzz(2);
			  break;

	}
}

//***************
void buzz(int buzz_type)
{
	overflow_count = 0;

//Sounds buzzer for the duration of the dot
	if (buzz_type == 1)
	{

		BUZZER = 0;
		TR0 = 1;
		while ((int)overflow_count < ((dotTime*1000)/2.96));
		TR0 = 0;
		BUZZER = 1;
	}
//Sounds buzzer for the duration of the dash
	if (buzz_type == 2)
	{

		BUZZER = 0;
		TR0 = 1;
		while ((int)overflow_count < 3*((dotTime*1000)/2.96));
		TR0 = 0;
		BUZZER = 1;
	}
//Stops buzzer for a dot duration
	if (buzz_type == 0)
	{
		BUZZER = 1;
		TR0 = 1;
		while((int)overflow_count < ((dotTime*1000)/2.96));
		TR0 = 0;
	}
//Stops buzzer for space inbetween dots and dashes
	overflow_count = 0;
	TR0 = 1;
	while((int)overflow_count < 3*((dotTime*1000)/2.96));
	TR0 = 0;
	overflow_count = 0;
}



//***************

//Mode 2
void Mode2 (void)
{
	printf("There are 3 pushbuttons; Pushbutton enter is to enter the answer, Pushbutton dot is to enter a dot in morse code\r\n");
	printf("and Pushbutton dash is to enter a dash in morse code. Use these buttons when prompted to enter for a character:\r\n");
	final_score=0;
	for(k=0; k<4; k++)
	{
		//sets input array to all 0's
		for(j=0;j<4;j++)
			{
				input[j]=0;
			}

		//starts and stops timer and clears counts2
		TR0 = 0;
		counts2 = 0;
		TR0 = 1;

		//gets an answer from 0-9
		//prints what letter should be entered with push buttons
		generatedAnswer=Get_Rand();
		Mode2print(generatedAnswer);

		//clears index
		i = 0;

		//sets flag to false for the while loop and clears game_score
		flag=0;
		game_score = 0;

		//turns BILED off
		BILED1 = 1;
		BILED0 = 1;

		//enters loops when the flag is false
		while (!flag)
		{
			//sounds dot on buzzer if the PBDot is pressed
			//dots are kept track in an array
			if (!PBDot)
			{
				input[i] = 1;
				buzz2(1);
				i++;
				//takes away sound time for dot from counts
				counts2-=((dotTime*1000)/2.96);
			}
			//sounds dash on buzzer if the PBDash is pressed
			//dashes are kept track in an array
			if (!PBDash)
			{
				input[i] = 2;
				buzz2(2);
				i++;
				//takes away sound time for dash from counts
				counts2-=(3*((dotTime*1000)/2.96));
			}
			//PBEnter is pressed the input from the user is finished
			while (!PBEnter)
			{
				flag=1;
				TR0=0;
				overflow_count=0;
				TR0=1;
				//debounce for half a second
				while(overflow_count<169);
			}
		}

		//checks if user input from pushbutton is correct, green is correct, red is incorrect and 10 penalty points are added
		if(check(input,generatedAnswer))
		{
			printf("Correct\r\n");
			BILED1 = 1;
			BILED0 = 0;
		}
		else
		{
			printf("Incorrect\r\n");
			BILED0 = 1;
			BILED1 = 0;
			game_score+=10;
		}

		//adds time penalty points, similar to mode 1
		//adds 1 point for every half second user takes to input an answer
		game_score += (int)(counts2/169);
		final_score += game_score;
		//displays score for each round and final score
		printf("Round score: %u\r\n", game_score);
		printf("Final score: %u\r\n", final_score);
		counts2 = 0;
	}
		//displays final score
		printf("\r\nFinal score: %u\r\n", final_score);
		overflowCount=0;
		overflow_count=0;

	//BILED blinks from red to green for 1 second
	while (overflowCount < 338)
	{
			BILED0 = 0;
			BILED1 = 1;
			BILED1 = 0;
			BILED0 = 1;
	}
}


//***************
void Mode2print(int random)
{
	//makes print statement for each random number
	if(random == 0)
		printf("Please enter the morse code for A\r\n");
	else if(random == 1)
		printf("Please enter the morse code for B\r\n");
	else if(random == 2)
		printf("Please enter the morse code for C\r\n");
	else if(random == 3)
		printf("Please enter the morse code for D\r\n");
	else if(random == 4)
		printf("Please enter the morse code for E\r\n");
	else if(random == 5)
		printf("Please enter the morse code for F\r\n");
	else if(random == 6)
		printf("Please enter the morse code for G\r\n");
	else if(random == 7)
		printf("Please enter the morse code for H\r\n");
	else if(random == 8)
		printf("Please enter the morse code for I\r\n");
	else if(random == 9)
		printf("Please enter the morse code for J\r\n");
}

//***************
unsigned int check( unsigned int input [4], unsigned int rand) //make enter a global variable
{
	//checks the input array from the push buttons
	//returns true if it matches, otherwise it's false
	if ((input[0] == 1) & (input[1] == 2) & (input[2] == 0) & (input[3] == 0) & (rand == 0)) //A
		return 1;

	else if ((input[0] == 2) & (input[1] == 1) & (input[2] == 1) & (input[3] == 1) & (rand == 1)) //B
		return 1;

	else if ((input[0] == 2) & (input[1] == 1) & (input[2] == 2) & (input[3] == 1) & (rand == 2)) //C
		return 1;

	else if ((input[0] == 2) & (input[1] == 1) & (input[2] == 1) & (input[3] == 0) & (rand == 3)) //D
		return 1;

	else if ((input[0] == 1) & (input[1] == 0) & (input[2] == 0) & (input[3] == 0) & (rand == 4)) //E
		return 1;

	else if ((input[0] == 1) & (input[1] == 1) & (input[2] == 2) & (input[3] == 1) & (rand == 5)) //F
		return 1;

	else if ((input[0] == 2) & (input[1] == 2) & (input[2] == 1) & (input[3] == 0) & (rand == 6)) //G
		return 1;

	else if ((input[0] == 1) & (input[1] == 1) & (input[2] == 1) & (input[3] == 1) & (rand == 7)) //H
		return 1;

	else if ((input[0] == 1) & (input[1] == 1) & (input[2] == 0) & (input[3] == 0) & (rand == 8)) //I
		return 1;

	else if ((input[0] == 1) & (input[1] == 2) & (input[2] == 2) & (input[3] == 2) & (rand == 9)) //J
		return 1;

	else
		return 0;

}
//***************
void Interrupt_Init(void)
{
	IE |= 0x02;      // enable Timer0 Interrupt request
	EA = 1;       // enable global interrupts
}

//***************
void Timer_Init(void)
{

	CKCON |= 0x08;  // SYSCLK
	TMOD &= 0xF0;   // clear the 4 least significant bits
	TMOD |= 0x01;   // select TIMER0 mode (16-bit)
	TR0 = 0;         // Stop Timer0
	TL0 = 0;         // Clear low byte of register T0
	TH0 = 0;         // Clear high byte of register T0

}

//***************
void Timer0_ISR(void) __interrupt 1
{
	TF0 = 0;
	overflowCount++;
	overflow_count++;	//increments counts for buzzer, buzzer2, and mode 1
	counts2++;			//increments counts for mode2 for the penalty score


}
//***************
void buzz2(int buzz_type)
{	//second buzzer function that is quicker to implement because spaces are not implemented
	overflow_count = 0;

//dot
	if (buzz_type == 1)
	{

		BUZZER = 0;
		TR0 = 1;
		while ((int)overflow_count < ((dotTime*1000)/2.96));
		TR0 = 0;
		BUZZER = 1;
	}
//dash
	if (buzz_type == 2)
	{

		BUZZER = 0;
		TR0 = 1;
		while ((int)overflow_count < 3*((dotTime*1000)/2.96));
		TR0 = 0;
		BUZZER = 1;
	}
}

/*
----------------Brick Breaker Ultimate-----------------
ELEC317 FINAL PROJECT
-Cem Cengiz Yazýcý
-Kemal Barýþ Kesen
*/

//Define CPU CLock and Peripheral Pins for the GLCD
#define F_CPU 8000000UL
#define Data_Port PORTC
#define TotalPage 8
#define RS					PA2		/* Define control pins */
#define RW					PA3
#define EN					PD6
#define CS1					PB0
#define CS2					PB1
#define RST					PD7
// Define BAUD Rate 
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define uartReady UCSRA&(1<<RXC)

// Include Libraries
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include "Font_Header.h" // Library which holds fonts with their respective sizes on GLCD
#include <avr/interrupt.h>

// Define Global Variables and Arrays
const int speed = 4;
int stop = 0;
int direc = 5;
int coords[2];
int numblocks[10] = {1,1,1,1,1,1,1,1,1,1};
int numpowerups[10] = {1,1,1,1,1,1,1,1,1,1};
int block2[30] = {50,5,6, 42,5,6, 34,5,6, 26,15,6, 18,15,6, 10,15,6, 40,20,6, 32,20,6, 48,20,6, 4,20,6};
int block[30] = {10,30,6, 18,30,6, 26,30,6, 34,30,6, 42,10,6, 50,10,6, 10,10,6, 18,10,6, 26,10,6, 34,10,6};
int paddle[4] = {20, 60, 12, 1};
int paddleDirect = 0;
int powerup_1[30] = {1,9,5, 25,4,2, 12,30,3, 61,20,4, 15,43,5, 42,25,6, 10,50,2, 55,41,1, 36,51,3, 40,35,2};
int helper = 0;
char scr_txt[10];
char scr_txt2[5];

void GLCD_Init();
void GLCD_Command(char Command);
void GLCD_ClearAll();
void GLCD_Data(char Data);
void GLCD_Change(bool screen);
void glcd_print_pixel(unsigned char x, unsigned char y, int z);

void swap_arrays(int *y, int *z) // Swaps the Level Arrays (Used when LVL is Successfully Finished)
{
	int x,temp;

	for(x=0;x<30;x++)
	{
		// Arrays of Length 30, Swap Y with Z
		temp = y[x];
		y[x] = z[x];
		z[x] = temp;
	}
}
void GLCD_Init() // Enables Necessary Pins to Prepare the Display at the Startup (KS0108 Display Controller)
{
	// Commands used here can be found in KS0108 Data Sheet
	// Set Control Pins
	DDRA |= (1 << RS) | (1 << RW);
	DDRD |= (1 << EN) | (1 << RST);
	DDRB |= (1 << CS1) | (1 << CS2);
	DDRC = 0xFF; // Make data port all outputs

	_delay_ms(20); // delay to work properly

	// Clear Ports
	PORTA = 0x00; // Clear RS and RW
	PORTD = 0x00; // Clear EN and RST
	PORTB = 0x00; // Clear CS1 and CS2

	// Apply commands for Initialization
	_delay_ms(20);
	GLCD_Command(0b00111100); // 8-Bit Mode
	GLCD_Command(0b00001100); // Display On
	GLCD_Command(0b00000001); // Clear Display
	_delay_ms(2);
	GLCD_Command(0b00000110); // Increment Cursor Mode


	// After the initialization, turn on the display
	GLCD_Command(0b00001111); // Turn off Display
}  

void GLCD_Data(char Data) // Send data to GLCD Function
{
	Data_Port = Data;    // Data on PORTC
	// Peripheral Procedure for Data Output
	PORTA |= (1 << 2);   
	PORTA &= ~(1 << 3);  
	PORTD |= (1 << 6);   
	_delay_us(5);
	PORTD &= ~(1 << 6);  
	_delay_us(5);
}

void GLCD_Command(char Command) // Command Function for GLCD
{
	// Peripheral Procedure for Command Set
	Data_Port = Command;
	PORTA &= ~(1 << 2);  
	PORTA &= ~(1 << 3);  
	PORTD |= (1 << 6);   
	_delay_us(5);
	PORTD &= ~(1 << 6);
	_delay_us(5);
}

void GLCD_Change(bool screen) // Change the Active Screen (Select Left/Right Screen)
{


	PORTA = 0x0C;
	PORTD = 0x80;
	if(screen) PORTB = 0x01;
	else PORTB = 0x02;
	
	
	PORTD |= (1 << 7);	// Enable Port
	GLCD_Command(0x3E); // Turn off the Display
	GLCD_Command(0x42); // Y = 0
	GLCD_Command(0xB8); // X = 0
	GLCD_Command(0xC0); // Z = 0 (Not Used)
	GLCD_Command(0x3F); // Turn on the Display
}
void GLCD_String(char page_no, char *str, int display)// GLCD string write function 
{
	unsigned int i, column;
	unsigned int Page = ((0xB8) + page_no);
	unsigned int Y_address = 0;
	float Page_inc = 0.5;
	
	if(display == 1)
	{
		PORTB |= (1 << CS1);	//Select Left half of display 
		PORTB &= ~(1 << CS2);
	}
	else
	{
		PORTB |= (1 << CS2);	// Select Left half of display 
		PORTB &= ~(1 << CS1);
	}
	
	
	
	GLCD_Command(Page);
	for(i = 0; str[i] != 0; i++)	// Print char in string till null 
	{
		if (Y_address > (1024-(((page_no)*128)+FontWidth)))
		break;
		if (str[i]!=32)
		{
			for (column=1; column<=FontWidth; column++)
			{
				if ((Y_address+column)==(128*((int)(Page_inc+0.5))))
				{
					if (column == FontWidth)
					break;
					GLCD_Command(0x40);
					Y_address = Y_address + column;
					PORTB ^= (1 << CS1);
					PORTB ^= (1 << CS2);
					GLCD_Command(Page + Page_inc);
					Page_inc = Page_inc + 0.5;
				}
			}
		}
		if (Y_address>(1024-(((page_no)*128)+FontWidth)))
		break;
		if((font[((str[i]-32)*FontWidth)+4])==0 || str[i]==32)
		{
			for(column=0; column<FontWidth; column++)
			{
				GLCD_Data(font[str[i]-32][column]);
				if((Y_address+1)%64==0)
				{
					PORTB ^= (1 << CS1);
					PORTB ^= (1 << CS2);
					GLCD_Command((Page+Page_inc));
					Page_inc = Page_inc + 0.5;
				}
				Y_address++;
			}
		}
		else
		{
			for(column=0; column<FontWidth; column++)
			{
				GLCD_Data(font[str[i]-32][column]);
				if((Y_address+1)%64==0)
				{
					PORTB ^= (1 << CS1);
					PORTB ^= (1 << CS2);
					GLCD_Command((Page+Page_inc));
					Page_inc = Page_inc + 0.5;
				}
				Y_address++;
			}
			GLCD_Data(0);
			Y_address++;
			if((Y_address)%64 == 0)
			{
				PORTB ^= (1 << CS1);
				PORTB ^= (1 << CS2);
				GLCD_Command((Page+Page_inc));
				Page_inc = Page_inc + 0.5;
			}
		}
	}
	GLCD_Command(0x40);	// Set Y address (column=0) 
}
void GLCD_ClearAll() // GLCD Clear Function (all 0)
{
	for (int v = 0; v < 2; v++){
		if(v == 0) GLCD_Change(false);
		else if(v == 1) GLCD_Change(true);
		
		for (int i = 0; i < TotalPage; i++) {
			GLCD_Command((0xB8) + i); //Next Page
			for (int j = 0; j < 64; j++) {
				GLCD_Data(0x00);	// Write 0x00 to clear 8 pixels
			}
		}
		// Set X and Y address as 0 (Complete)
		GLCD_Command(0x40); 
		GLCD_Command(0xB8); 
	}
}

void glcd_print_pixel(unsigned char x, unsigned char y, int z) // Print Single Pixel on to Display
{
	// Calculate page and column
	unsigned char page = y / 8;
	unsigned char column = x % 64;
	
	// Set page address
	GLCD_Command(0xB8 + page);

	// Set column address
	GLCD_Command(0x40 + column);

	// Send pixel data
	GLCD_Data(z << (y % 8)); // Turn Single Pixel on
}

void printPaddle() // Print Paddle Function
{
	// If Helper Power up is active, also print shield on the last row
	for(int a = 0; a <=64; a++)
	{
		if(helper==1) 
		{
			glcd_print_pixel(a,paddle[1],0x08);
		}
		else{glcd_print_pixel(a,paddle[1],0);};
		
	}
	for(int i= 0; i<=paddle[2]; i++)
	{
		if(helper==1)
		{
			glcd_print_pixel(paddle[0]+i,paddle[1],0x09);
		}
		else{glcd_print_pixel(paddle[0]+i,paddle[1],1);};
	}
}
void printPowerUp() // Print Surviving Power Ups
{
	for(int i=0;i<10;i++)
	{
		if(numpowerups[i]==1) // Check if Power-Up is taken
		{
			// Print Power-Up Display Pattern on its coordinates
			glcd_print_pixel(powerup_1[i*3+0]-1,powerup_1[i*3+1],1);
			glcd_print_pixel(powerup_1[i*3+0]+1,powerup_1[i*3+1],1);
			glcd_print_pixel(powerup_1[i*3+0],powerup_1[i*3+1]-1,1);
			glcd_print_pixel(powerup_1[i*3+0],powerup_1[i*3+1]+1,1);
		}
	}
}
void movePaddle_Right() // Move paddle to right, then update its display
{
	if((paddle[0]<=63-paddle[2]-1)) // Check if next to wall
	{
		// Update X Coordinates of Paddle
		paddle[0] = paddle[0] + paddle[3];
	}
	// Print Paddle with updated coordinates
	printPaddle(); 
}
void movePaddle_Left() // Move paddle to left, then update its display
{
	if((paddle[0]>=paddle[3]))	// Check if next to wall
	{
		// Update X Coordinates of Paddle
		paddle[0] = paddle[0] - paddle[3];
	}
	// Print Paddle with updated coordinates
	printPaddle();
}
void stopPaddle() // If in state stopPaddle, don't update the coordinates of the paddle
{
	printPaddle();
}
void movePaddle() // Update Paddle Coordinates According to Paddle State
{
	if(paddleDirect == 2)
	{
		stopPaddle();
	}
	else if(paddleDirect == 0)
	{
		movePaddle_Right();
	}
	else if(paddleDirect == 1)
	{
		movePaddle_Left();
	}
	 
}

int move_up(int direc) // Move Ball Up
{
	// Update Coordinates
	glcd_print_pixel(coords[0],coords[1],0);
	coords[1] = coords[1] - 1;
	glcd_print_pixel(coords[0],coords[1],1);
	if(coords[1] < 2) // If collision with upper wall, change direction
	{
		return 3;
	}
	return direc;
}
int move_down(int direc) // Move Ball Down
{
	// Update Coordinates
	glcd_print_pixel(coords[0],coords[1],0);
	coords[1] = coords[1] + 1;
	glcd_print_pixel(coords[0],coords[1],1);
	return direc;
}
int move_topr(int direc) // Move Ball Top-Right Direction
{
	// Update Coordinates
	glcd_print_pixel(coords[0],coords[1],0);
	coords[0] = coords[0] + 1;
	coords[1] = coords[1] - 1;
	glcd_print_pixel(coords[0],coords[1],1);
	if(coords[0] == 64) // If collision, change direction
	{
		return 2;
	}
	return direc;
}

int move_topl(int direc) // Move Ball Top-Left Direction
{
	// Update Coordinates
	glcd_print_pixel(coords[0],coords[1],0);
	coords[0] = coords[0] - 1;
	coords[1] = coords[1] - 1;
	glcd_print_pixel(coords[0],coords[1],1);
	if(coords[0] == 0) // If collision, change direction
	{
		return 1;
	}
	return direc;
}

int move_downr(int direc) // Move Ball Down-Right Direction
{
	// Update Coordinates
	glcd_print_pixel(coords[0],coords[1],0);
	coords[0] = coords[0] +1;
	coords[1] = coords[1] +1;
	glcd_print_pixel(coords[0],coords[1],1);
	if(coords[0] >= 60) // If collision, change direction
	{
		return 4;
	}
	if(coords[1]==63&& helper!=1){stop=-2;} // Check collision with down wall, stop game if true
	else if(coords[1]==63 && helper==1){helper=0;}
	return direc;
}

int move_downl(int direc) // Move Ball Down-Left Direction
{
	// Update Coordinates
	glcd_print_pixel(coords[0],coords[1],0);
	coords[0] = coords[0] - 1;
	coords[1] = coords[1] + 1;
	glcd_print_pixel(coords[0],coords[1],1);
	if(coords[0] == 0) // Check collision with wall
	{
		return 3;
	}
	if(coords[1]==63 && helper!=1){stop=-2;} // Check collision with down wall, stop game if true
	else if(coords[1]==63 && helper==1){helper=0;} // Turn off Helper if active
	return direc;
}
int updateball() // General Function to Update the Position and the Direction of the Ball
{
	switch(direc){
		case 1:
		direc = move_topr(direc);
		break;
		case 2:
		direc = move_topl(direc);
		break;
		case 3:
		direc = move_downr(direc);
		break;
		case 4:
		direc = move_downl(direc);
		break;
		case 5:
		direc = move_up(direc);
		break;
		case 6:
		direc = move_down(direc);
		break;
	}

	// Check collision with side-walls
	if(coords[1]>=63){
		if(direc==4)
		{
			direc = 2;
		}
		if(direc==3)
		{
			direc = 1;
		}
	}
	if(coords[1]<=1){
		if(direc==2)
		{
			direc = 4;
		}
		if(direc==1)
		{
			direc = 3;
		}
	}
	return direc;
}

void reflectBall() // Reflect Ball from Blocks
{
	switch(direc){
		case 1:
		direc = 3;
		break;
		case 2:
		direc = 4;
		break;
		case 3:
		direc = 1;
		break;
		case 4:
		direc = 2;
		break;
		case 5:
		direc = 3;
		break;
		
	}
}
void reflectPaddleBall() // Reflect Ball From Paddle (Change Direction)
{
	switch(direc){
		case 1:
		direc = 3;
		break;
		case 2:
		direc = 4;
		break;
		case 3:
		direc = 1;
		break;
		case 4:
		direc = 2;
		break;
		case 5:
		direc = 6;
		break;
		case 6:
		direc = 5;
		break;
	}
}
void printbricks() // Print Surviving Bricks onto the Screen
{
	// Get Coordinates
	int x = coords[0];
	int y = coords[1];
	// Iterate over Block Array
	for(int i=0;i<10;i++)
	{
		if(numblocks[i]==1) // Check if Block Lives, Print if True
		{
			if((x<= block[3*i] + block[3*i+2] && x>=block[3*i]) && y==block[3*i+1]) // Check if Collision
			{
				numblocks[i]=0; // Kill the Block
				reflectBall(); // Reflect Ball From Block If Collision
			}
			for(int j= 0; j<=block[3*i+2]; j++)
			{
				// Print Block if Lives
				glcd_print_pixel(block[3*i]+j,block[3*i+1],1);
			}
		}
		else
		for(int j= 0; j<=block[3*i+2]; j++)
		{
			// Clear Pixels of Killed Blocks
			glcd_print_pixel(block[3*i]+j,block[3*i+1],0);
		}
	}
}
void paddleCollision() // Check Collision with Paddle
{
	// Get Coordinates
	int x = coords[0];
	int y = coords[1];
	// Check if Collision with Middle Pixels of Pedal
	if(((x<=(paddle[0] + (paddle[2]/2) + 1))&&(x>=(paddle[0] + (paddle[2]/2) - 1))) && y==paddle[1])
	{
		//If yes, direct the ball to the straight up
		direc = 5;
	}
	else if(x<=(paddle[0] + (paddle[2])) && x>=paddle[0] && y==paddle[1]-2)
	{
		// Else, reflect the ball as usual (if down-right, now top-right)
		reflectPaddleBall(); // Reflect Ball from Paddle According to its direction
	}
}
void pow_1()
{
	// Set Paddle Speed as 3
	paddle[3] = 3;
}
void pow_2()
{
	// Set Paddle Speed as 1
	paddle[3] = 1;
}
void pow_3()
{
	// Update Paddle Width (Bigger)
	paddle[2] += 2;
}
void pow_4()
{
	// Update Paddle Width (Smaller)
	paddle[2] -= 2;
}
void pow_5()
{
	// Turn-on Helper
	helper = 1;
}
void pow_6()
{
	// Break a random block
	for(int i = 0; i < 10; i++)
	{
		if(numblocks[i] == 1)
		{
			numblocks[i] = 0;
			break;
		}
	}
}

void powerupCollision() // Check if Collision with Power-Up
{
	// Get Coordinates
	int x = coords[0];
	int y = coords[1];
	for(int i=0;i<10;i++)
	{
		if(numpowerups[i]==1) // Check if Power-up Lives
		{
			// Check Collision
			if(((x== powerup_1[3*i])&&y==powerup_1[3*i+1]) ||((x== powerup_1[3*i]-1)&&y==powerup_1[3*i+1]) ||((x== powerup_1[3*i]+1)&&y==powerup_1[3*i+1])|| ((x== powerup_1[3*i])&&(y==powerup_1[3*i+1]+1))||((x== powerup_1[3*i])&&(y==powerup_1[3*i+1]-1)))
			{
				// Activate Power-up Depending on its type
				if(powerup_1[3*i+2] == 1)
				{
					pow_1();
				}
				if(powerup_1[3*i+2] == 2)
				{
					pow_2();
				}
				if(powerup_1[3*i+2] == 3)
				{
					pow_3();
				}
				if(powerup_1[3*i+2] == 4)
				{
					pow_4();
				}
				if(powerup_1[3*i+2] == 5)
				{
					pow_5();
				}
				if(powerup_1[3*i+2] == 6)
				{
					pow_6();
				}
				// Clear Dead Power-ups
				glcd_print_pixel(powerup_1[i*3+0]-1,powerup_1[i*3+1],0);
				glcd_print_pixel(powerup_1[i*3+0]+1,powerup_1[i*3+1],0);
				glcd_print_pixel(powerup_1[i*3+0],powerup_1[i*3+1]-1,0);
				glcd_print_pixel(powerup_1[i*3+0],powerup_1[i*3+1]+1,0);
				numpowerups[i]=0;
			}
			
		}
		
	}
}
void checkinput() // Check for PB2 Input
{
	for(int k = 0;k<10;k++)
	{
		_delay_ms(speed);
		if((PINB&0x04)==0b00000100) // If PB2 Input, End the Game
		{
			if(stop!=-2)
			{
				stop = -2;
			}
			
			
			_delay_ms(200);
		}
		
		if((PINB&0b10000000)==0b10000000) // IF PB7 Input, Move Paddle to left
		{
			paddleDirect = 1;
			movePaddle();
			//_delay_ms(10);
		}
		if((PINB&0b01000000)==0b01000000) // IF PB8 Input, Move Paddle to left
		{
			paddleDirect = 0;
			movePaddle();
		}
		
	}
}
void rightdisplay() // Fill Right Display with Pattern
{
GLCD_Change(1);
	for(int k = 0; k<63; k++)
	{
		for(int l = 0; l<8; l++)
		{
			if(l==0||l==7)
			{
				if(l==0){glcd_print_pixel(k,l*8,0x0f);}
				if(l==7){glcd_print_pixel(k,l*8,0xf0);}
			}
			if(k<=4 || k>=59)
			{
				glcd_print_pixel(k,l*8,0xFF);
				
			}
			
			
		}
	}
GLCD_Change(0);
}
void GLCD_fill() // Fill Display with Pattern (Left-half or Right-half)
{
	for(int k = 0; k<63; k++)
	{
		for(int l = 0; l<8; l++)
		{
			glcd_print_pixel(k,l*8,0b10101010);
			glcd_print_pixel(k,l*8,0b10101010);
		}
	}
}
void uartInit(unsigned long baud) // Initialize UART Interface
{
	unsigned int UBRR;
	/*Baud rate calculator*/
	UBRR=(F_CPU/(16*baud))-1;
	UBRRH=(unsigned char)(UBRR>>8);
	UBRRL=(unsigned char)UBRR;
	/*Enable the transmitter and receiver*/
	UCSRB=(1<<RXEN)|(1<<TXEN)|(1<<RXEN);
	/*asynchronous mode, 8-bit, 1-stop bit*/
	UCSRC=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);
}

void uartTransmit(unsigned char data) // Transmit Single Char over UART
{
	/*Stay here until the buffer is empty*/
	while(!(UCSRA&(1<<UDRE)));
	/*Put the data into the buffer*/
	UDR=data;
}

char uartReceive() // Receive Single Char over UART
{
	/*Wait until the buffer is full*/
	while(!(UCSRA&(1<<RXC)));
	/*Get the data ready to use*/
	return UDR;
}

void sendText(char *txt) // Sent Text over UART
{
	while(*txt) uartTransmit(*txt++);
}
void loadlevel(int lvl) // Load LVL by LVL variable
{
	for(int a = 0; a < 30; a++)
	{
		if(lvl == 1)
		{
			
		}
		if(lvl == 2)
		{
			swap_arrays(block,block2);
		}
	}
}
void printscore(int score) // Print Current Player Score
{
	sprintf(scr_txt," SCORE:%d",score);
	GLCD_String(1, scr_txt,1);
	GLCD_Change(0);
}
int calculateScore() // Calculate User Score
{
	// Calculate User Score Counting Killed Blocks and Power-Ups
	int b = 0;
	for(int a = 0; a < 10; a++)
	{
		if(numblocks[a] == 0)
		{
			b += 2;
		}
		if(numpowerups[a] == 0)
		{
			b += 1;
		}
	}
	return b;
}
void printusername() // Print Username to Left-Screen
{	
		char temps[8] = " USER:";
		GLCD_String(5, strcat(temps,scr_txt2),1);
		GLCD_Change(0);
}
int checkbricks() // Check if All Bricks are Killed
{
	for(int i=0;i<10;i++)
	{
		if(numblocks[i]!=0){return 0;}
	}
	return 1;
}
void GLCD_MENU() // Display Text Before Start
{
	for(int i = 0; i<2;i++)
	{
		GLCD_String(1,"----------",i);
		GLCD_String(2,"  PREPARE! ",i);
		GLCD_String(3,"  INCOMING ",i);
		GLCD_String(4,"----------",i);
	}
	
}
void credits(int score) // Display End-Game Credits
{
	GLCD_String(1,"     KBK    ",1);
	GLCD_String(2,"     CCY    ",1);
	GLCD_String(3,"  GAMEOVER ",1);
	sprintf(scr_txt,"  SCORE:%d  ",score);
	GLCD_String(4, "             ",1);
	GLCD_String(6, "             ",1);
	GLCD_String(5, scr_txt,1);
}
int game(int lvl) // Main Game Loop
{
	// Initialize necessary variables and Initialize GLCD
	stop = 0;
	loadlevel(lvl);
	GLCD_Init();
	GLCD_ClearAll();
	_delay_ms(20);
	PORTD |= (1 << PD2);
	
	int score = 0;
	int win = 0;
	coords[0] = 30;
	coords[1] = 30;
	GLCD_MENU();
	_delay_ms(2000);
	
	GLCD_ClearAll();
	GLCD_Change(0);
	helper = 0;
	// Game Start
	while(1)
	{
		// If PB2 Button Input, End the Game
		checkinput();
		if(stop!=-1&&stop!=-2) // Game Continues
		{
			
			checkinput();
			direc = updateball();
			paddleCollision();
			powerupCollision();
			printPaddle();
			printPowerUp();
			printbricks();
			score = calculateScore();
			printscore(score);
			printusername();
			win = checkbricks();
		}
		// Different End-Game Conditions
		if(stop==-1)
		{
			GLCD_ClearAll();
			GLCD_String(8,"Game Over",1);
		}
		// User Mistake, Game-Over
		if(stop == -2)
		{
			GLCD_ClearAll();
			GLCD_Change(0);
			GLCD_fill();
			GLCD_Change(1);
			GLCD_fill();
			credits(score);
			rightdisplay();
			_delay_ms(200);
			return -2;
		}
		// All Blocks Broken, Proceed Next LVL by ending the loop 
		if(win == 1)
		{
			GLCD_ClearAll();
			GLCD_Change(0);
			//GLCD_fill();
			GLCD_Change(1);
			GLCD_fill();
			GLCD_Change(0);
			GLCD_fill();
			credits(score);
			rightdisplay();
			//GLCD_MENU();
			_delay_ms(5000);
			if(lvl==1){return 2;}
			else if(lvl==2){return 1;}
			else{return 2;}
		}
	}
}

int main(void)
{
	// Initialize Screen with text and wait for UART Input
	GLCD_Init();
	GLCD_ClearAll();
	GLCD_String(1,"----------",0);
	GLCD_String(2,"   BRICK  ",0);
	GLCD_String(3,"  BREAKER ",0);
	GLCD_String(4,"  ULTIMATE ",0);
	GLCD_String(5,"----------",0);
	credits(0);
	int skips = 0;
	int lvl = 1;
	char tmp,i=0;
	char tmp2;
	stop = 0;
	DDRC=0xFF;
	DDRB |= (0 << PB2);
	uartInit(9600);
	// Ask for LVL Select Over UART
	sendText("\n\r<<Select Level 1-2>>\n\r");
	while (1)
	{
		if (uartReady&&(i<1)&&(stop!=-2)) // If Receive Char 2, Select LVL 2
		{
			
			tmp2=uartReceive();
			if(tmp2 == '2'){swap_arrays(block,block2);}
			i++;
			
		}
		checkinput();
		if(i==1||stop==-2) // If Receive PB2 Button Input, Skip LVL and Username
		{
			if(stop==-2){lvl=1;skips = 1;}
			break;
		}
	}
	if(skips!=1){sendText("\n\rUsername? ");}
	i = 0;
	while((skips!=1))
	{
		if (uartReady&&i<3)
		{
			tmp=uartReceive();
			scr_txt2[i] = tmp;
			i++;
			
		}
		
		if(i==3 || tmp == 32){

			break;
		}
		
	}
	while(1)
	{
		// Load LVL
		lvl = game(lvl);
		swap_arrays(block,block2);
		// Initialize all Blocks Alive
		for(int tmp2 = 0; tmp2<10;tmp2++)
		{
			numblocks[tmp2] = 1;
			numpowerups[tmp2] = 1;
		}
		// If GAMEOVER Condition, Break Game Loop
		if(stop==-2){break;}
	}
}

/*
 * ATmega_GLCD_TextFont
 * http://electronicwings.com
 */ 
 #define F_CPU 8000000UL	

#include <avr/io.h>			/* Include AVR std. library file */
#include <util/delay.h>			/* Include delay header file */
#include <stdio.h>			/* Include std i/o library file */

		/* Define CPU clock Freq 8MHz */
#define Data_Port PORTC

#define RS		 PA2		/* Define control pins */
#define RW		 PA3
#define EN		 PD6
#define CS1		 PB0
#define CS2		 PB1
#define RST		 PD7

#define TotalPage	8 

void glcd_Init(void);
void GLCD_Command(char Command);
void GLCD_Data(char Data);
void printPixel(unsigned char x, unsigned char y, int on);
void GLCD_Change(int x)


int main(void) {
	glcd_Init();		/* Initialize GLCD */
	_delay_ms(2);
	GLCD_Change(0);
	printPixel(30, 30, 0);

	while(1);

	return 0;
}


void printPixel(unsigned char x, unsigned char y, int on) {
	unsigned char page = y / 8;
	unsigned char column = x % 64;

	GLCD_Command(0xB8 + page);
	GLCD_Command(0x40 + column);

	GLCD_Data(on << (y % 8));
}

void GLCD_Command(char Command)		/* GLCD command function */
{
	Data_Port = Command;		/* Copy command on data pin */
	PORTA &= ~(1 << RS);	/* Make RS LOW for command register*/
	PORTA &= ~(1 << RW);	/* Make RW LOW for write operation */
	PORTD |=  (1 << EN);	/* HIGH-LOW transition on Enable */
	_delay_us(5);
	PORTD &= ~(1 << EN);
	_delay_us(5);
}

void GLCD_Data(char Data)		/* GLCD data function */
{
	Data_Port = Data;		/* Copy data on data pin */
	PORTA |= (1 << RS);	/* Make RS HIGH for data register */
	PORTA &= ~(1 << RW);	/* Make RW LOW for write operation */
	PORTD |= (1 << EN);	/* HIGH-LOW transition on Enable */
	_delay_us(5);
	PORTD &= ~(1 << EN);
	_delay_us(5);
}
void GLCD_Change(int screen) {
	//false = left screen, true = right screen

	PORTA = 0x0C;
	PORTD = 0x80;
	if(screen == 1) PORTB = 0x01;
	else PORTB = 0x02;

	PORTD |= (1 << 7);
	GLCD_Command(0x3E); /* Display OFF */
	GLCD_Command(0x42); /* Set Y address (column=0) */
	GLCD_Command(0xB8); /* Set x address (page=0) */
	GLCD_Command(0xC0); /* Set z address (start line=0) */
	GLCD_Command(0x3F); // Display ON
}

void glcd_Init() {
	
	// Set the direction of control and data pins
	DDRA |= (1 << RS) | (1 << RW);
	DDRD |= (1 << EN) | (1 << RST);
	DDRB |= (1 << CS1) | (1 << CS2);
	DDRC = 0xFF; // Assuming the data port is on PORTC

	_delay_ms(20);

	// Initialize control pin states
	PORTA = 0x00; // Clear RS and RW
	PORTD = 0x00; // Clear EN and RST
	PORTB = 0x00; // Clear CS1 and CS2

	// Apply initialization sequence
	_delay_ms(20);
	GLCD_Command(0b00111100); // Function Set (8-bit interface, 2 lines, normal instruction set)
	GLCD_Command(0b00001100); // Display On/Off Control (Display ON, Cursor OFF, Blink OFF)
	GLCD_Command(0b00000001); // Clear Display
	_delay_ms(2);
	GLCD_Command(0b00000110); // Entry Mode Set (Increment cursor, no display shift)

	// Your additional initialization steps may go here

	// After the initialization, turn on the display
	GLCD_Command(0b00001111); // Display On/Off Control (Display ON, Cursor ON, Blink ON)
}
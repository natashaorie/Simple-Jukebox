#include <avr/io.h>
#include <avr/interrupt.h>
#include "io.c"


unsigned long SM_time = 2000;
//------------------- BEGIN PWM SETUP----------------------------------
const double init = 0.0;
double c = 261.61;
double d = 293.66;
const double e = 329.63;
const double f = 349.23;
const double g = 392.00;
const double a = 440.00;
const double b = 493.88;
const double c5 = 523.25;
unsigned int k = 0;
//unsigned char speaker = 0x00;

double song1[10] = {261.61, 293.66, 329.63, 329.63, 349.23, 392.00, 493.88, 440.00, 392.00, 392.00};
unsigned int song1time[10] = {1,1,1,2,1,2,1,1,1,2};
unsigned char song1down[10]= {1,1,1,1,1,1,1,1};
unsigned char song1Length = 10;

double song2[10] = {261.61,523.25,493.88,392.00,440.00,493.88,523.25,261.61,440.00,392.00};
unsigned int song2time[10] = {1,3,1,1,1,1,2,2,2,3};
unsigned char song2down[10]= {1,1,1,1,1,1,1,1};
unsigned char song2Length = 10;

double song3[9] = {329.63,293.66,329.63,293.66,329.63,493.88,293.66,261.61,440.00};
unsigned int song3time[9] = {1,1,1,1,1,1,1,1,1,3};
unsigned char song3down[10]= {1,1,1,1,1,1,1,1};
unsigned char song3Length = 9;
	
unsigned int songTime = 0;
unsigned int i = 0;
unsigned int j = 0;
unsigned int song = 0;
double currNote = 0;
unsigned int noteTime = 0;


void set_PWM(double frequency) {
	static double current_frequency;
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; }
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		else if (frequency > 31250) { OCR3A = 0x0000; }
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
		TCNT3 = 0;
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//-------------------------BEGIN TIMER SETUP-----------------------------
unsigned char tmpS = 0x00;
volatile unsigned char TimerFlag = 0; 

unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;

	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//-----------------------------BEGIN STATE MACHINES--------------------------

enum SM_States {SM_Welcome, SM_Instruc, SM_Interrupt, SM_DispS1, SM_DispS2, SM_DispS3, SM_Song1, SM_Song2, SM_Song3, SM_Play, SM_Rest} SM_State;
void SongSelect() {
	switch(SM_State) {
		case SM_Welcome:
		if(~PINA & 0x01) {
			SM_State = SM_Instruc;
		}
		else {
			SM_State = SM_Welcome;
		}
		break;
		
		case SM_Instruc:
		SM_State = SM_DispS1;
		break;
		
		case SM_Interrupt:
		if(~PINA & 0x01) {
			if(song == 1) {
				SM_State = SM_Song2;
			}
			if(song == 2) {
				SM_State = SM_Song3;
			}
			if(song == 3) {
				SM_State = SM_Song1;
			}
		}
		else {
			SM_State = SM_Welcome;
		}
		break;
		case SM_DispS1:
		if(~PINA & 0x01) {
			SM_State = SM_Song1;
		}
		else {
			SM_State = SM_DispS2;
		}
		break;
		
		case SM_DispS2:
		if(~PINA & 0x01) {
			SM_State = SM_Song2;
		}
		else {
			SM_State = SM_DispS3;
		}
		break;
		
		case SM_DispS3:
		if(~PINA & 0x01) {
			SM_State = SM_Song3;
		}
		else {
			SM_State = SM_DispS1;
		}
		break;
		
		case SM_Song1:
		SM_State = SM_Play;
		break;
		
		case SM_Song2:
		SM_State = SM_Play;
		break;
		
		case SM_Song3:
		SM_State = SM_Play;
		break;
		
		case SM_Play:
		if(~PINA & 0x01) {
			SM_State = SM_Interrupt;
		}
		/*if(j < noteTime) {
			SM_State = SM_Play;
		}*/
		else {
			SM_State = SM_Rest;
		}
		break;
		
		case SM_Rest:
		if(~PINA & 0x01 && k >= 2) {
			SM_State = SM_Interrupt;
		}
		if(i == songTime) {
			SM_State = SM_Welcome;
		}
		else {
			SM_State = SM_Play;
		}
		break;
	}
	switch (SM_State) {
		case SM_Welcome:
		SM_time = 2000;
		k = 0;
		set_PWM(init);
		PWM_on();
		LCD_ClearScreen();
		LCD_DisplayString(1, "Hello Jukebox!  Press to start.");
		break;
		case SM_Instruc:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Press button to select song.");
		break;
		case SM_Interrupt:
		k = k + 1;
		break;
		case SM_DispS1:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Lakeside");
		break;
		case SM_DispS2:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Over the Rainbow");
		break;
		case SM_DispS3:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Contemporary");
		break;
		
		case SM_Song1:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Lakeside");
		i = 0;
		j = 0;
		song = 1;
		currNote = song1[i];
		songTime = song1Length;
		break;
		
		case SM_Song2:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Over the Rainbow");
		i = 0;
		j = 0;
		song = 2;
		currNote = song2[i];
		songTime = song2Length;
		break;
		
		case SM_Song3:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Contemporary");
		i = 0;
		j = 0;
		song = 3;
		currNote = song3[i];
		songTime = song3Length;
		break;
		
		case SM_Play:
		SM_time = 500;
		//set_PWM(init);
		j = j + 1;
		set_PWM(currNote);
		LCD_DisplayString(17, "                ");
		if(currNote == 261.61) {
			LCD_DisplayString(17, "**");
		}
		if(currNote == 293.66) {
			LCD_DisplayString(17, "****");
		}
		if(currNote == 329.63) {
			LCD_DisplayString(17, "******");
		}
		if(currNote == 349.23) {
			LCD_DisplayString(17, "********");
		}
		if(currNote == 392.00) {
			LCD_DisplayString(17, "**********");
		}
		if(currNote == 440.00) {
			LCD_DisplayString(17, "************");
		}
		if(currNote == 493.88) {
			LCD_DisplayString(17, "**************");
		}
		if(currNote == 523.25) {
			LCD_DisplayString(17, "****************");
		}
	
		
		case SM_Rest:
		j = 0;
		i = i + 1;
		//set_PWM(init);
		if(song == 1) {
			currNote = song1[i];
			noteTime = song1[i];
		}
		else if(song == 2) {
			currNote = song2[i];
			noteTime = song2[i];
		}
		if(song == 3) {
			currNote = song3[i];
			noteTime = song3[i];
		}
		break;
	
	}
}
//---------------------BEGIN MAIN------------------------------

int main(void)
{
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	DDRA = 0x00; PORTA = 0xFF;
	
		unsigned long SM_elaspedTime = 500;
		const unsigned long timePeriod = 500;
	
	SM_State = SM_Welcome;
	
	TimerSet(timePeriod);
	TimerOn();
	// Initializes the LCD display
	LCD_init();
	
	while(1) {
		if(SM_elaspedTime >= SM_time) {
			SongSelect();
			SM_elaspedTime = 0;
		}
		//PORTB = tmpS;
		//TimerFlag = 0;
		// while (1)
		while(!TimerFlag);
		TimerFlag = 0;
		SM_elaspedTime += timePeriod;
	}
}

/*
Jeffreyson Nguyen jnguy557@ucr.edu
EE/CS 120B Custom Project
Motion Security System

This project demonstrates a motion security system with complexities consisting of 
1. PIR Motion Sensor
2. 4x LED lights
3. LCD Screen
4. Keypad
5. 2x Atmega1284 Microchips
6. Buzzer
7. Pontimeter
8. 1x button
9. ISR Header
10. EEPROM (saves password) when system is turned off

Description:
This system initially starts at the disarmed phase. When the disarmed/armed button is pressed,
the system prompts via LCD screen, to enter the current pincode to disarmed/armed. 

A max of 5 character can be used for the pincode. "#", acts as the enter key.

Holding the "*" enters the system into the change pincode phase. Changing the pincode 
does not work while the system alarm is activated.

*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "buzzer.h"
#include "usart.h"
#include "bit.h"
#include "keypad.h"
#include "scheduler.h"
#include "timer.h"
#include "io.h"
#include "eeprom.h"
#include <math.h>
#include <avr/eeprom.h>


//State machines
enum KeyPad_SM{num}key_state;
enum Menu_SM{MENU_START, DISARMED, ARMED}menu_state;
enum Reset_SM{RESET}reset_state;
enum Alarm_SM{START_ALARM, BUZZER}alarm_state;
enum LED_SM{LED1, LED2, LED3, LED4}led_states;
enum TURN_OFF_SM{TURN_OFF}turn_off_state;

unsigned char a_btn;
unsigned char tmpC = 0x00;
unsigned char def_pw[] = {"00000"};//default pincode
unsigned char user_pw[5];//user entered pincode
unsigned char new_pw[5];//new pincode
unsigned char correct = 0;

unsigned char pw_flag = 0;
unsigned char pound_flag = 0;
unsigned char hashtag_flag = 0;
unsigned char new_pass_flag = 0;
unsigned char pin_flag = 0;
unsigned char enter_pw_flag = 0;
unsigned char pir_flag = 0;
unsigned char turn_off_flag = 0;
unsigned char prompt_flag = 0;
unsigned char armed_flag = 0;
//increment the cursor position on the lcd display
unsigned char i = 0x00;
unsigned char i2 = 0x00;
unsigned char counter = 0;
unsigned char menu_flag = 0;//set flag


//When the alarm is activated, and a_btn is pressed, bring up menu to turn off alarm(buzzer and led) by entering pincode
//Resetting(*) does not work when the alarm is activated
TurnOffTick() {
	
	switch (turn_off_state) {
		
		case TURN_OFF:
		if(a_btn && turn_off_flag == 1) {
			a_btn = 0;
			i = 0;
			i2 = 0;
			correct = 0;
			hashtag_flag = 0;
			LCD_ClearScreen();
			LCD_DisplayString(1, "Enter Pin: ");
			delay_ms(100);
		}
		if (i == 5 && hashtag_flag == 1 && turn_off_flag == 1) {
			for(unsigned int i = 0; i < sizeof(user_pw); ++i) {
				if(def_pw[i] == user_pw[i]) {
					++correct;
				}
			}
			if(correct == 5) {
				LCD_ClearScreen();
				LCD_DisplayString(1, "Alarm Turned Off");
				turn_off_flag = 0;
				pir_flag = 0;
				armed_flag = 1;
				PORTB = 0;
				PWM_off();
				i = 0;
				correct = 0;
				pound_flag = 0;
				turn_off_flag = 0;
				hashtag_flag = 0;
				prompt_flag = 1;
				delay_ms(500);
				LCD_ClearScreen();
				LCD_DisplayString(6, "ARMED");
				delay_ms(100);
				menu_state = ARMED;
				MenuTick();
			}
			else {
				LCD_ClearScreen();
				LCD_DisplayString(1, "Invalid Pin");
				hashtag_flag = 0;
				a_btn = 1;
				turn_off_flag = 1;
				i = 0;
				i2 = 0;
				correct = 0;
				delay_ms(500);
				turn_off_state = TURN_OFF;
				TurnOffTick();
			}
		}
		
		default:
		break;
	}
}


//Activates LED lights when PIR sensor is HIGH
void LedTick() {
	
	if(pir_flag == 1 && armed_flag == 1) {
		
		
		switch(led_states) {
			
			case LED1:
			led_states = LED2;
			break;
			
			case LED2:
			led_states = LED3;
			break;
			
			case LED3:
			led_states = LED4;
			break;
			
			case LED4:
			led_states = LED1;
			break;
			
			default:
			break;
		}
		
		switch(led_states) {
			
			case LED1:
			PORTB = ~PORTB & 0x01;
			break;
			
			case LED2:
			PORTB = ~PORTB & 0x02;
			break;
			
			case LED3:
			PORTB = ~PORTB & 0x04;
			break;
			
			case LED4:
			PORTB = ~PORTB & 0x08;
			break;
			
			default:
			break;
			
		}
	}
}

//Activates buzzer when PIR sensor is HIGH
void AlarmTick() {
	
	if(pir_flag == 1 && armed_flag == 1) {
		
		switch(alarm_state) {
			
			case START_ALARM:
			LCD_ClearScreen();
			LCD_DisplayString(1, "MOTION DETECTED!");
			delay_ms(100);
			PWM_on();
			alarm_state = BUZZER;
			break;
			
			case BUZZER:
			if(prompt_flag == 1) {
				prompt_flag = 0;
				alarm_state = START_ALARM;
			}
			else {
				PWM_on();
				alarm_state = BUZZER;
				delay_ms(10);
			}
			
			default:
			break;
		}
		
		switch(alarm_state) {
			
			case START_ALARM:
			break;
			
			case BUZZER:
			set_PWM(3000);
			break;
			
			default:
			break;
		}
	}
}

//Resets the password when "*" is hold
void ResetTick() {
	
	switch(reset_state) {
		
		case RESET:
		
		if(pound_flag && pir_flag == 0) {
			i = 0;
			i2 = 0;
			correct = 0;
			pound_flag = 0;
			hashtag_flag = 0;
			enter_pw_flag = 1;
			LCD_ClearScreen();
			LCD_DisplayString(1, "Enter Old Pin: ");
			delay_ms(100);
		}
		if(i == 5 && hashtag_flag == 1 && enter_pw_flag == 1 && pir_flag == 0) {
			for(unsigned int i = 0; i < sizeof(user_pw); ++i) {
				if(def_pw[i] == user_pw[i]) {
					++correct;
				}
			}
			if(correct == 5) {
				i = 0;
				i2 = 0;
				pound_flag = 0;
				hashtag_flag = 0;
				enter_pw_flag = 0;
				pin_flag = 1;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Enter new Pin: ")	;
				delay_ms(100);
			}
			else {
				LCD_ClearScreen();
				LCD_DisplayString(1, "Invalid Pin");
				pound_flag = 1;
				hashtag_flag = 0;
				enter_pw_flag = 0;
				pin_flag = 0;
				i = 0;
				i2 = 0;
				correct = 0;
				delay_ms(500);
				reset_state = RESET;
			}
		}
		if(i == 5 && hashtag_flag == 1 && pin_flag == 1 && pir_flag == 0) {
			for(unsigned int i = 0; i < sizeof(user_pw); ++i) {
				new_pw[i] = user_pw[i];
			}
			
			LCD_ClearScreen();
			LCD_DisplayString(1, "Reenter new Pin: ");
			delay_ms(100);
			i = 0;
			i2 = 0;
			correct = 0;
			pin_flag = 0;
			hashtag_flag = 0;
			new_pass_flag = 1;
		}
		if(i == 5 && hashtag_flag == 1 && new_pass_flag == 1 && pir_flag == 0) {
			for(unsigned int i = 0; i < sizeof(user_pw); ++i) {
				if(new_pw[i] == user_pw[i]) {
					++correct;
				}
			}
			if(correct == 5) {
				LCD_ClearScreen();
				LCD_DisplayString(1, "New Pin Set");
				for(unsigned int i = 0; i < sizeof(user_pw); ++i) {
					def_pw[i] = new_pw[i];
					EEPROM_Write(&def_pw[i], new_pw[i]);
				}
				i = 0;
				correct = 0;
				hashtag_flag = 0;
				new_pass_flag = 0;
				pin_flag = 0;
				delay_ms(500);
				LCD_DisplayString(1, "Rebooting...");
				delay_ms(1000);
				menu_state = MENU_START;
				MenuTick();
			}
			else {
				LCD_ClearScreen();
				LCD_DisplayString(1, "Invalid Pin");
				pound_flag = 1;
				hashtag_flag = 0;
				new_pass_flag = 0;
				pin_flag = 0;
				i = 0;
				i2 = 0;
				correct = 0;
				delay_ms(500);
				reset_state = RESET;
			}
			break;
		}
		default:
		break;
	}


}

//Arm or disarm, prompts to enter pincode
void MenuTick() {

	switch(menu_state) {
		
		//start

		case MENU_START:
		LCD_DisplayString(5, "DISARMED");
		delay_ms(100);
		pir_flag = 0;
		menu_state = DISARMED;
		break;
		
		case DISARMED:
		if(menu_flag == 1) {
			menu_flag = 0;
			hashtag_flag = 0;
			correct = 0;
			i = 0;
		}
		
		if(i == 5 && hashtag_flag == 1 && pir_flag == 0) {
			for(unsigned int i = 0; i < sizeof(user_pw); ++i) {
				if(user_pw[i] == def_pw[i]) {
					++correct;
				}
			}
			if(correct == 5) {
				LCD_ClearScreen();
				LCD_DisplayString(6, "ARMED");
				delay_ms(100);
				armed_flag = 1;
				menu_flag = 1;
				hashtag_flag = 0;
				correct = 0;
				menu_state = ARMED;
			}
			else {
				LCD_DisplayString(1, "Invalid Pin");
				delay_ms(500);
				i = 0;
				i2 = 0;
				hashtag_flag = 0;
				correct = 0;
				a_btn = 1;
				menu_state = DISARMED;
			}
		}
		
		if(a_btn && i != 5 && turn_off_flag == 0) {
			hashtag_flag = 0;
			i2 = 0;
			delay_ms(100);
			LCD_DisplayString(1, "Enter Pin: ");
			delay_ms(100);
		}
		break;

		case ARMED:
		
		if(menu_flag == 1) {
			menu_flag = 0;
			hashtag_flag = 0;
			i = 0;
		}
		
		if(i == 5 && hashtag_flag == 1 && pir_flag == 0) {
			LCD_ClearScreen();
			for(unsigned int i = 0; i < sizeof(user_pw); ++i) {
				if(user_pw[i] == def_pw[i]) {
					++correct;
				}
			}
			if(correct == 5) {
				LCD_ClearScreen();
				LCD_DisplayString(5, "DISARMED");
				delay_ms(100);
				menu_flag = 1;
				armed_flag = 0;
				pir_flag = 0;
				correct = 0;
				hashtag_flag = 0;
				menu_state = DISARMED;
			}
			else {
				LCD_DisplayString(1, "Invalid Pin: ");
				delay_ms(500);
				i = 0;
				i2 = 0;
				hashtag_flag = 0;
				correct = 0;
				a_btn = 1;
				menu_state = ARMED;
			}
		}
		
		if(a_btn && i != 5 && turn_off_flag == 0) {
			hashtag_flag = 0;
			i2 = 0;
			delay_ms(100);
			LCD_DisplayString(1, "Enter Pin: ");
			delay_ms(100);
		}
		break;
		
		default:
		break;
	}
}

//Keypad, using USART to communicate between two ATMEGA1284 chips
void KeyTick() {

	if(pw_flag == 1) {
		
		unsigned char x;
		
		x = GetKeypadKey();
		switch(key_state) {
			case num:
			if(USART_IsSendReady(0)) {
				USART_Send(x, 0);
			}
			if(USART_HasReceived(0)) {
				unsigned char X_TEMP = USART_Receive(0);

				//If pin code exceeds 5, stops input
				if(i2 >= 5 && X_TEMP != '*' && X_TEMP != '#') {break;}
				
				switch (X_TEMP) {
					case '\0':
					break;
					
					case '1':
					tmpC = 0x01;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '2':
					tmpC = 0x02;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '3':
					tmpC = 0x03;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '4':
					tmpC = 0x04;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '5':
					tmpC = 0x05;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '6':
					tmpC = 0x06;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '7':
					tmpC = 0x07;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '8':
					tmpC = 0x08;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '9':
					tmpC = 0x09;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case 'A':
					tmpC = 0x0A;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + 0x37;
					++i;
					++i2;
					break;
					
					case 'B':
					tmpC = 0x0B;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + 0x37;
					++i;
					++i2;
					break;
					
					case 'C':
					tmpC = 0x0C;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + 0x37;
					++i;
					++i2;
					break;
					
					case 'D':
					tmpC = 0x0D;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + 0x37;
					++i;
					++i2;
					break;
					
					case '*':
					++counter;
					if(counter == 5) {
						pound_flag = 1;
						counter = 0;
					}
					break;
					
					case '0':
					tmpC = 0x00;
					LCD_Cursor(17 + i);
					LCD_WriteData(0x0E + 0x1C);
					user_pw[i] = tmpC + '0';
					++i;
					++i2;
					break;
					
					case '#':
					hashtag_flag = 1;
					break;
					
					default:
					break;
				}

			}
			key_state = num;
			PORTC = tmpC;
			break;
		}
		USART_Flush(0);
	}
}


int main() {
	
	DDRA = 0xF0; PORTA = 0x0F;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	//Set Timer
	TimerSet(25);
	TimerOn();
	
	//Set USART Keypad
	initUSART(0);
	USART_Flush(0);
	
	//Set LCD
	LCD_init();
	
	//Stores new password as default password using EEPROM
	for(unsigned int i = 0; i < sizeof(def_pw); ++i) {
		def_pw[i] = EEPROM_Read(&def_pw[i]);
	}

	
	//State machines, state initializations
	menu_state = MENU_START;
	key_state = num;
	reset_state = RESET;
	alarm_state = START_ALARM;
	led_states = LED1;
	turn_off_state = TURN_OFF;
	
	
	while(1) {


		//PIR SENSOR
		if(GetBit(PINA, 1) && armed_flag == 1) {
			pir_flag = 1;
		}
		
		//PINA0, disarm/arm button
		a_btn = ~PINA & 0x01;
		
		//Prompts system to turn off alarm
		if(a_btn && pir_flag == 1) {
			turn_off_flag = 1;
		}
		
		//Disarm/armed
		if(a_btn) {
			pw_flag = 1;
		}
		
		TurnOffTick();
		MenuTick();
		KeyTick();
		ResetTick();
		AlarmTick();
		LedTick();
		
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 0;
}

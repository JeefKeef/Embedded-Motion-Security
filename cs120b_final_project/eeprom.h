/*
EEPROM Header File

Allows system to save password when the system is turned off

*/

#ifndef EEPROM_H_
#define EEPROM_H_
#include "avr\eeprom.h"


//Takes in an address to data into
void EEPROM_Write(unsigned char address, unsigned char data) {
	eeprom_write_word(address, data);
}

//takes in address to read data within the address
unsigned char EEPROM_Read(unsigned char address) {
	return eeprom_read_word(address);
}

#endif //EEPROM_H_
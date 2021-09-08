// Permission to copy is granted provided that this header remains intact.
// This software is provided with no warranties.
// Source: UCR CS/EE 120B/ IEEE
////////////////////////////////////////////////////////////////////////////////

#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef struct _task {
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct)(int);
} task;

unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}

#endif
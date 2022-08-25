#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <stdio.h>

const unsigned char Segment_Data[] =
{0xF9,0xA4,0xB0};
void Show4Digit(int number);
void ShowDigit(int i, int digit);
int num=1;

volatile unsigned char flag;
ISR(USART0_RX_vect)
{
	flag=UDR0;
}
ISR(INT0_vect)
{
	if(num==1 || num==2){
		num++;
		PORTB=0X20;
		PORTF=0X01;
		_delay_ms(20000);
		PORTB=0X20;
		PORTF=0X00;
	}
	else if(num==3){
		num=3;
		PORTB=0X20;
		PORTF=0X00;
	}
}
ISR(INT1_vect)
{
	if(num==3 || num==2){
		num--;
		PORTB=0X20;
		PORTF=0X02;
		_delay_ms(20000);
		PORTB=0X20;
		PORTF=0X00;
	}
	else if(num==1){
		num=1;
		PORTB=0X20;
		PORTF=0X00;
	}
}
ISR(INT2_vect)
{
	if (num==1){
		num=3;
		PORTB=0X20;
		PORTF=0X01;
		_delay_ms(40000);
		PORTB=0X20;
		PORTF=0X00;
	}
	else if(num==3 || num==2){
		PORTB=0X20;
		PORTF=0X00;
		num==num;
	}
}
ISR(INT3_vect)
{
	if(num==3){
		num=1;
		PORTB=0X20;
		PORTF=0X02;
		_delay_ms(40000);
		PORTB=0X20;
		PORTF=0X00;
	}
	else if(num==1 || num==2){
		PORTB=0X20;
		PORTF=0X00;
	}
}
void init()
{
	DDRA=0xff;
	UCSR0A=0x00;
	UCSR0B=0x98;
	UCSR0C=0x06;
	UBRR0H=0;
	UBRR0L=103;
	SREG=0x80;
}
int main(void)
{
	init();
	DDRC=0xFF;
	DDRA=0xFF;
	PORTC=0x07;
	EICRA |=(1<<ISC01) | (0<<ISC00) | (1<<ISC11) | (1<<ISC10) | (1<<ISC21) | (1<<ISC20) | (1<<ISC30) | (1<<ISC31);
	EIMSK |= (1<<INT0) | (1<<INT1) | (1<<INT2) | (1<<INT3);
	SREG |= 0x80;
	DDRF=0xFF;
	DDRB=0X00;
	while(1)
	{
		PORTA= Segment_Data[num-1];
		if(flag=='U')
		{
			PORTB=0X20;
			PORTF=0X01;
			_delay_ms(20000);
			PORTB=0X20;
			PORTF=0X00;
			num++;
		}
		if(flag=='D')
		{
			num--;
			PORTB=0X20;
			PORTF=0X02;
			_delay_ms(20000);
			PORTB=0X20;
			PORTF=0X00;
		}
	}
}


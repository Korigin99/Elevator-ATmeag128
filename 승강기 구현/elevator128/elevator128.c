

#define WEIGHT_REF 40 	//���� ���ذ� ���� g



#define F_CPU 16000000UL
#define sbi(PORTX,bitX) PORTX|=(1<<bitX)
#define cbi(PORTX,bitX) PORTX&=~(1<<bitX)


#include<avr/io.h>
#include<avr/delay.h>
#include<stdio.h>


int Adc_Channel(unsigned char Adc_input);
unsigned long ReadCout(void);
void LCD_command(char command);
void LCD_data(char data);
void LCD_init(void);
void LCD_string(char line, char *string);


char string1[16] = "hell";
char string2[16] = "Good Job!";
char buf[40] = {' ',};
double value = 0; 
double bef_val = 0.000;
double val = 0.000;
double a = 0.000;

double target = 0.000;
double error = 0.000;
double bef_error = 0.000;
double def_error = 0.000;
double P = 0.000;
double D = 0.000;
double pwm_data = 0.000;
double pwm_max = 1000;

int weight_cnt = 0;
volatile unsigned long weight = 0;
volatile unsigned long offset = 0;
volatile int offset_flag = 0;

int target_floor = 1;
int current_floor = 0;

int over_flag = 0;




void main()
{	
	//IO
	DDRA = 0b00000010;	//A0(DOUT): input, A1(SCK):output
	DDRB = 0xff;
	DDRC = 0xff;
	DDRD = 0x00;
	DDRE = 0xff;
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0xff;
	PORTE = 0xff;

	//ADC
	ADMUX = 0x00;
	ADCSRA = (1<<ADEN)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
	
	//TIMER/COUNTER 1
	TCCR1A = (1<<COM1A1)|(1<<COM1A0)|(1<<COM1B1)|(1<<COM1B0)|(1<<WGM11)|(0<<WGM10);
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(0<<CS12)|(1<<CS11)|(1<<CS10);
	ICR1 = 1000;
	OCR1A = 0;
	OCR1B = 0;

	//LCD
	LCD_init();



	while(1)
	{

		//���� ������ �޾ƿ�
		weight_cnt++;
		if(weight_cnt > 10)
		{
			weight = ReadCout()/2000;
			weight_cnt = 0;
		}

		//���� �� 
		if(weight > WEIGHT_REF)
		{
			over_flag = 1;
			cbi(PORTE,0);
			sbi(PORTE,1);
		}
		else
		{
			over_flag = 0;
			sbi(PORTE,0);
			cbi(PORTE,1);

		}

		//PSD ������ �޾ƿ�
		val = 100000.000 / (double)Adc_Channel(0);
		bef_val = value;
		a = 0.3;
		value = a*val + (1.000-a)*bef_val;

		//�������� �Ǵ�
		if(value < 300)
			current_floor = 1;
		else if(value < 500)
			current_floor = 2;
		else if(value < 660)
			current_floor = 3;
		else
			current_floor = 4;


		//����ġ �Է�  -  ���԰� ���� �ʾ������� ���ο� �Է� �޾ƿ�
		if(over_flag == 0)
		{
			if((PIND & 0b00010000) == 0x00)
				target_floor = 1;
			if((PIND & 0b00100000) == 0x00)
				target_floor = 2;
			if((PIND & 0b01000000) == 0x00)
				target_floor = 3;
			if((PIND & 0b10000000) == 0x00)
				target_floor = 4;
		}

		//���� ���� 
		if(target_floor == 1)
			target = 250.000;
		if(target_floor == 2)
			target = 465.000;
		if(target_floor == 3)
			target = 610.000;
		if(target_floor == 4)
			target = 750.000;

	
		//��� �����
		//target = 200.000;
		bef_error = error;
		error = target - value;
		def_error = error - bef_error;

		


		if(error < 0)	//��������
		{
			P = 2.000;
			D = 25.000;
		}
		else			//�ö󰥶�
		{
			P = 2.800;
			D = 8.000;

			if(value < 200)	//1�� �ٴ� �극��ũ 
				P = 20.000;
		}

			
		pwm_data = P*error + D*def_error; 
			
		pwm_max = 200;		//�ӵ� ���Ѱ� 
		if(pwm_data > pwm_max)
			pwm_data = pwm_max;
		if(pwm_data < -pwm_max)
			pwm_data = -pwm_max;

		if(pwm_data >= 0)
		{
			//������ 
			OCR1A = 0;
			OCR1B = pwm_data;
		}
		else
		{
			pwm_data = -pwm_data;

			//������ 
			OCR1A = pwm_data;
			OCR1B = 0;
		}


		//LCD�� ǥ��
		sprintf(buf, "[ %1dF ] now:  %dF ",target_floor,current_floor);
		LCD_string(1,buf);
		sprintf(buf, "[%3dg] now: %3dg",WEIGHT_REF,weight);
		LCD_string(2,buf);
		
		//10ms ������ 
		_delay_ms(10);

	}
}


int Adc_Channel(unsigned char Adc_input)
{
	ADMUX = (Adc_input | 0x40);    //ä�� ����
	ADCSRA |= 0x40;             //��ȯ ����!
	while((ADCSRA & 0x10) == 0);   //��ȯ�� �Ϸ�ɋ� ���� ��ٸ�.
	return ADC;
}


unsigned long ReadCout(void)
{
	unsigned long sum = 0,count = 0,data1 = 0,data2 = 0;

	for(int j=0; j<32; j++)
	{
		sbi(PORTA, 0);	//DOUT : 1
		cbi(PORTA, 1);	//SCK : 0
	
		count = 0;

		while((PINA & 0b00000001) == 0b00000001);
	
		for(int i=0; i<24; i++)
		{
			sbi(PORTA, 1);	//SCK : 1
			count = count<<1;
			cbi(PORTA, 1);	//SCK : 0
			if((PINA & 0b00000001) == 0b00000001)
				count++;
		}
		sbi(PORTA, 1);	//SCK : 1
		count = count^0x800000;
		cbi(PORTA, 1);	//SCK : 0

		sum += count;
	}
	data1 = sum/32;			//32�� ������ ��� 

	if(offset_flag == 0)
	{
		offset = data1;
		offset_flag = 1;
	}

	if(data1 > offset)
		data2 = data1 - offset;	//offset ���� 
	else
		data2 = 0;

	return data2;
}

void LCD_command(char command)
{
	PORTC = (command&0xF0);	// send High data
	cbi(PORTC,0); 			// RS=0
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable

	PORTC = (command&0x0F)<<4;// send Low data
	cbi(PORTC,0); 			// RS=0
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable
}



void LCD_data(char data)
{
	_delay_us(100);

	PORTC = (data&0xF0);	// send High data
	sbi(PORTC,0); 			// RS=1
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable

	PORTC = (data&0x0F)<<4;	// send Low data
	sbi(PORTC,0); 			// RS=1
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable
}



void LCD_init(void)
{
	_delay_ms(50);

	LCD_command(0x28);		// DL=0(4bit) N=1(2Line) F=0(5x7)
	_delay_ms(2);			// [function set] 0b00101000
							// 4:(DL) 1�̸� 8bit���, 0�̸� 4bit���
							// 3:(N) 0�̸� 1��¥��, 1�̸� 2��¥��
							// 2:(F) 0�̸� 5x8dots, 1�̸� 5x11dots

	LCD_command(0x0C);		// LCD ON, Cursor X, Blink X
	_delay_ms(2);			// [display on/off control] 0b00001100
							// 2:(D) 1�̸� display on, 0�̸� off
							// 1:(C) 1�̸� cursor on, 0�̸� off
							// 0:(B) 1�̸� cursor blink, 0�̸� off 

	LCD_command(0x06);		// Entry Mode
	_delay_ms(2);			// [entry mode set] 0b00000110
							// 1:(I/D) 1�̸� ����������, 0�̸� ����
							// 0:(SH) CGRAM ������ 
							
 	LCD_command(0x01);		// LCD Clear
	_delay_ms(2);
}



void LCD_string(char line, char *string)
{
	LCD_command(0x80+((line-1)*0x40));
	while(*string)
		LCD_data(*string++);
}























//PSD ����+LCD �׽�Ʈ 
/*
#define F_CPU 16000000UL
#define sbi(PORTX,bitX) PORTX|=(1<<bitX)
#define cbi(PORTX,bitX) PORTX&=~(1<<bitX)


#include<avr/io.h>
#include<avr/delay.h>
#include<stdio.h>


int Adc_Channel(unsigned char Adc_input);
void LCD_command(char command);
void LCD_data(char data);
void LCD_init(void);
void LCD_string(char line, char *string);


char string1[16] = "hell";
char string2[16] = "Good Job!";
char buf[32] = {' ',};
int value = 666; 



void main()
{	
	//IO
	DDRC = 0xFF;
	PORTC = 0x00;

	//ADC
	ADMUX = 0x00;
	ADCSRA = (1<<ADEN)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);

	//LCD
	LCD_init();


	while(1)
	{
		value = Adc_Channel(0);
	

		sprintf(buf, "Sensor : %d",value);
		LCD_string(1,buf);
		_delay_ms(100);

	}
}


int Adc_Channel(unsigned char Adc_input)
{
	ADMUX = (Adc_input | 0x40);    //ä�� ����
	ADCSRA |= 0x40;             //��ȯ ����!
	while((ADCSRA & 0x10) == 0);   //��ȯ�� �Ϸ�ɋ� ���� ��ٸ�.
	return ADC;
}


void LCD_command(char command)
{
	PORTC = (command&0xF0);	// send High data
	cbi(PORTC,0); 			// RS=0
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable

	PORTC = (command&0x0F)<<4;// send Low data
	cbi(PORTC,0); 			// RS=0
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable
}



void LCD_data(char data)
{
	_delay_us(100);

	PORTC = (data&0xF0);	// send High data
	sbi(PORTC,0); 			// RS=1
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable

	PORTC = (data&0x0F)<<4;	// send Low data
	sbi(PORTC,0); 			// RS=1
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable
}



void LCD_init(void)
{
	_delay_ms(50);

	LCD_command(0x28);		// DL=0(4bit) N=1(2Line) F=0(5x7)
	_delay_ms(2);			// [function set] 0b00101000
							// 4:(DL) 1�̸� 8bit���, 0�̸� 4bit���
							// 3:(N) 0�̸� 1��¥��, 1�̸� 2��¥��
							// 2:(F) 0�̸� 5x8dots, 1�̸� 5x11dots

	LCD_command(0x0C);		// LCD ON, Cursor X, Blink X
	_delay_ms(2);			// [display on/off control] 0b00001100
							// 2:(D) 1�̸� display on, 0�̸� off
							// 1:(C) 1�̸� cursor on, 0�̸� off
							// 0:(B) 1�̸� cursor blink, 0�̸� off 

	LCD_command(0x06);		// Entry Mode
	_delay_ms(2);			// [entry mode set] 0b00000110
							// 1:(I/D) 1�̸� ����������, 0�̸� ����
							// 0:(SH) CGRAM ������ 
							
 	LCD_command(0x01);		// LCD Clear
	_delay_ms(2);
}



void LCD_string(char line, char *string)
{
	LCD_command(0x80+((line-1)*0x40));
	while(*string)
		LCD_data(*string++);
}
*/





//���� �׽�Ʈ
/*

#include<avr/io.h>


void main()
{
	//IO
	DDRB = 0xff;
	PORTB = 0x00;
	
	//TIMER/COUNTER 1
	TCCR1A = (1<<COM1A1)|(1<<COM1A0)|(1<<COM1B1)|(1<<COM1B0)|(1<<WGM11)|(0<<WGM10);
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS11);
	ICR1 = 2000;
	OCR1A = 0;
	OCR1B = 0;

	while(1)
	{

		OCR1A = 0;
		OCR1B = 200;
	}
}
*/








//LCD �׽�Ʈ
/*

#define F_CPU 16000000UL
#define sbi(PORTX,bitX) PORTX|=(1<<bitX)
#define cbi(PORTX,bitX) PORTX&=~(1<<bitX)


#include<avr/io.h>
#include<avr/delay.h>
#include<stdio.h>

void LCD_command(char command);
void LCD_data(char data);
void LCD_init(void);
void LCD_string(char line, char *string);


char string1[16] = "Hell!";
char string2[16] = "Good Job!";
char buf[32] = {' ',};
int value = 666; 


void main()
{
	DDRC = 0xFF;
	PORTC = 0x00;

	LCD_init();

    while(1)
	{
		sprintf(buf, "Hello %s %d",string1,value);
		LCD_string(1,buf);
		_delay_ms(1000);

		LCD_command(0x01);		// LCD Clear
		_delay_ms(2);

		LCD_string(2,string2);
		_delay_ms(1000);

		LCD_command(0x01);		// LCD Clear
		_delay_ms(2);
	}
}


void LCD_command(char command)
{
	PORTC = (command&0xF0);	// send High data
	cbi(PORTC,0); 			// RS=0
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable

	PORTC = (command&0x0F)<<4;// send Low data
	cbi(PORTC,0); 			// RS=0
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable
}



void LCD_data(char data)
{
	_delay_us(100);

	PORTC = (data&0xF0);	// send High data
	sbi(PORTC,0); 			// RS=1
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable

	PORTC = (data&0x0F)<<4;	// send Low data
	sbi(PORTC,0); 			// RS=1
	//cbi(PORTC,1);			// RW=0
	sbi(PORTC,2);			// Enable
	_delay_us(1);
	cbi(PORTC,2);			// Disable
}



void LCD_init(void)
{
	_delay_ms(50);

	LCD_command(0x28);		// DL=0(4bit) N=1(2Line) F=0(5x7)
	_delay_ms(2);			// [function set] 0b00101000
							// 4:(DL) 1�̸� 8bit���, 0�̸� 4bit���
							// 3:(N) 0�̸� 1��¥��, 1�̸� 2��¥��
							// 2:(F) 0�̸� 5x8dots, 1�̸� 5x11dots

	LCD_command(0x0C);		// LCD ON, Cursor X, Blink X
	_delay_ms(2);			// [display on/off control] 0b00001100
							// 2:(D) 1�̸� display on, 0�̸� off
							// 1:(C) 1�̸� cursor on, 0�̸� off
							// 0:(B) 1�̸� cursor blink, 0�̸� off 

	LCD_command(0x06);		// Entry Mode
	_delay_ms(2);			// [entry mode set] 0b00000110
							// 1:(I/D) 1�̸� ����������, 0�̸� ����
							// 0:(SH) CGRAM ������ 
							
 	LCD_command(0x01);		// LCD Clear
	_delay_ms(2);
}



void LCD_string(char line, char *string)
{
	LCD_command(0x80+((line-1)*0x40));
	while(*string)
		LCD_data(*string++);
}

*/

















//���� ���� �׽�Ʈ
/*

#define sbi(PORTX , BitX) PORTX |= (1<<BitX) 
#define cbi(PORTX ,BitX) PORTX &= ~(1<<BitX) 


#include<avr/io.h>
#include<util/delay.h>


void UART1_TX_int(unsigned long data);
void UART1_TX(unsigned char data);
unsigned long ReadCout(void);


volatile unsigned long weight = 0;
volatile unsigned long offset = 0;
volatile int offset_flag = 0;

volatile int test_cnt = 0;

void main()
{
	DDRA = 0b00000010;	//A0(DOUT): input, A1(SCK):output
	
	UCSR1A = 0b00000000;
	UCSR1B = 0b00001000;	//TXEN1
	UCSR1C = 0b00000110;	//UCSZ11,UCSZ10//8bit
	UBRR1H = 0;
	UBRR1L = 103;			//'16Mhz' 9600bps

	while(1)
	{
		weight = ReadCout()/2000;

		test_cnt++;
		if(test_cnt > 1000)
			test_cnt = 0;
		_delay_ms(100);

		UART1_TX_int(weight);
		UART1_TX('\n');
		UART1_TX('\r');
	}
}

unsigned long ReadCout(void)
{
	unsigned long sum = 0,count = 0,data1 = 0,data2 = 0;

	for(int j=0; j<32; j++)
	{
		sbi(PORTA, 0);	//DOUT : 1
		cbi(PORTA, 1);	//SCK : 0
	
		count = 0;

		while((PINA & 0b00000001) == 0b00000001);
	
		for(int i=0; i<24; i++)
		{
			sbi(PORTA, 1);	//SCK : 1
			count = count<<1;
			cbi(PORTA, 1);	//SCK : 0
			if((PINA & 0b00000001) == 0b00000001)
				count++;
		}
		sbi(PORTA, 1);	//SCK : 1
		count = count^0x800000;
		cbi(PORTA, 1);	//SCK : 0

		sum += count;
	}
	data1 = sum/32;			//32�� ������ ��� 

	if(offset_flag == 0)
	{
		offset = data1;
		offset_flag = 1;
	}

	if(data1 > offset)
		data2 = data1 - offset;	//offset ���� 
	else
		data2 = 0;

	return data2;
}

void UART1_TX(unsigned char data)
{
	while((UCSR1A & 0x20 ) == 0x00 );
	UDR1 = data;
}

void UART1_TX_int(unsigned long data)
{
	unsigned long temp = 0;

	temp = data/10000;
	UART1_TX(temp+48);
	temp = (data%10000)/1000;
	UART1_TX(temp+48);
	temp = (data%1000)/100;
	UART1_TX(temp+48);
	temp = (data%100)/10;	
	UART1_TX(temp+48);
	temp = data%10; 
	UART1_TX(temp+48);
}
*/

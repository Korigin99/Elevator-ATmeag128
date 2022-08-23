

#define WEIGHT_REF 40 	//무게 기준값 단위 g



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

		//무게 센서값 받아옴
		weight_cnt++;
		if(weight_cnt > 10)
		{
			weight = ReadCout()/2000;
			weight_cnt = 0;
		}

		//무게 비교 
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

	

		//현재층수 판단
		if(value < 300)
			current_floor = 1;
		else if(value < 500)
			current_floor = 2;
		else if(value < 660)
			current_floor = 3;
		else
			current_floor = 4;


		//스위치 입력  -  무게가 넘지 않았을때만 새로운 입력 받아옴
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

		//층수 결정 
		if(target_floor == 1)
			target = 250.000;
		if(target_floor == 2)
			target = 465.000;
		if(target_floor == 3)
			target = 610.000;
		if(target_floor == 4)
			target = 750.000;

	
		//비례 제어기
		//target = 200.000;
		bef_error = error;
		error = target - value;
		def_error = error - bef_error;

		


		if(error < 0)	//내려갈때
		{
			P = 2.000;
			D = 25.000;
		}
		else			//올라갈때
		{
			P = 2.800;
			D = 8.000;

			if(value < 200)	//1층 바닥 브레이크 
				P = 20.000;
		}

			
		pwm_data = P*error + D*def_error; 
			
		pwm_max = 200;		//속도 제한값 
		if(pwm_data > pwm_max)
			pwm_data = pwm_max;
		if(pwm_data < -pwm_max)
			pwm_data = -pwm_max;

		if(pwm_data >= 0)
		{
			//정방향 
			OCR1A = 0;
			OCR1B = pwm_data;
		}
		else
		{
			pwm_data = -pwm_data;

			//역방향 
			OCR1A = pwm_data;
			OCR1B = 0;
		}


		//LCD에 표시
		sprintf(buf, "[ %1dF ] now:  %dF ",target_floor,current_floor);
		LCD_string(1,buf);
		sprintf(buf, "[%3dg] now: %3dg",WEIGHT_REF,weight);
		LCD_string(2,buf);
		
		//10ms 딜레이 
		_delay_ms(10);

	}
}


int Adc_Channel(unsigned char Adc_input)
{
	ADMUX = (Adc_input | 0x40);    //채널 결정
	ADCSRA |= 0x40;             //변환 시작!
	while((ADCSRA & 0x10) == 0);   //변환이 완료될 까지 기다림.
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
	data1 = sum/32;			//32개 데이터 평균 

	if(offset_flag == 0)
	{
		offset = data1;
		offset_flag = 1;
	}

	if(data1 > offset)
		data2 = data1 - offset;	//offset 제외 
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
							// 4:(DL) 1이면 8bit모드, 0이면 4bit모드
							// 3:(N) 0이면 1줄짜리, 1이면 2줄짜리
							// 2:(F) 0이면 5x8dots, 1이면 5x11dots

	LCD_command(0x0C);		// LCD ON, Cursor X, Blink X
	_delay_ms(2);			// [display on/off control] 0b00001100
							// 2:(D) 1이면 display on, 0이면 off
							// 1:(C) 1이면 cursor on, 0이면 off
							// 0:(B) 1이면 cursor blink, 0이면 off 

	LCD_command(0x06);		// Entry Mode
	_delay_ms(2);			// [entry mode set] 0b00000110
							// 1:(I/D) 1이면 오른쪽으로, 0이면 왼쪽
							// 0:(SH) CGRAM 사용관련 
							
 	LCD_command(0x01);		// LCD Clear
	_delay_ms(2);
}



void LCD_string(char line, char *string)
{
	LCD_command(0x80+((line-1)*0x40));
	while(*string)
		LCD_data(*string++);
}

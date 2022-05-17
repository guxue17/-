#include "reg51.H"
#include <intrins.h> 
#include <string.h>           // �����ͷ�ļ���,��ʹ��strstr�⺯��
#include <stdio.h>

sbit LCD_RS     = P1^0;  //�Ĵ���ѡ������ˣ�LCD1602�ĵ��Ľ�
sbit LCD_RW     = P1^1;  //��д��������ˣ�LCD1602�ĵ����
sbit LCD_E      = P1^2;  //ʹ���ź������, LCD1602�ĵ�����

sbit DS18B20_DAT  = P1^3;  //DS18B20 DATA�ź�

sbit ADC_CS=P2^7;   //ADC0832 CS��
sbit ADC_CLK=P2^6;   //ADC0832 CS��
sbit ADC_DO_DI=P2^5;   //ADC0832 CS��

sbit Key_SET    = P3^2;  //����
sbit Key_Plus   = P3^3;  //��(+)
sbit Key_Dec    = P3^4;  //��(-)

sbit Key_Fan    = P3^5;  //���Ȱ���
sbit Key_Hot    = P3^6;  //���Ȱ���
sbit Key_LED    = P3^7;  //��������
sbit Key_Light  = P2^3;  //�ڹⰴ��

sbit RY_Fan     = P2^0;  //���ȼ̵���
sbit RY_Hot     = P2^1;  //���ȼ̵���
sbit RY_LED     = P2^2;  //�����̵���
 
//������� 
sbit Motor1     = P1^4;
sbit Motor2     = P1^5;
sbit Motor3     = P1^6;
sbit Motor4     = P1^7;


sbit BEEP=P2^4; //������


#define u8  unsigned char
#define u16 unsigned int

#define   LCD_Data		 P0      		//LCD1602���ݽӿ�

u16 KeyValue,KeyState;

u8 xdata buff[20];//������ʾ��������

u16 ctemp;        //ʵʱ�¶ȱ���
u16 cLight;       //ʵʱ���ձ���
u16 cHumi;       //ʵʱʪ�ȱ���

u8 stemp_H=25;      //���������¶�������ֵ����,��ֵΪ25
u8 stemp_L=20;      //���������¶�������ֵ����,��ֵΪ20
u8 sLight_H=70;     //�������ù���������ֵ����,��ֵΪ70
u8 sLight_L=30;     //�������ù���������ֵ����,��ֵΪ30
u8 sHumi=30;     //��������ʪ����ֵ����,��ֵΪ30

u8 presskeynum=0;    //�������� 
u8 setflag=0;
u8 workmode=0; //����ģʽ 0 �Զ� 1�ֶ�

u8 tempflag=0; //�¶�״̬��־
u8 lightflag=0; //����״̬��־
u8 humiflag=0; //����״̬��־
u8 fanflag=0; //����״̬
u8 hotflag=0; //����״̬
u8 ledflag=0; //led״̬
u16 motortime=0;
u8 motorflag=0; //�������״̬

//��ʱ��غ���
void delay_us(unsigned int q)//��ʱ����1us
{
	while(q--);
}

void Delay_ms(u16 x) 	//��ʱxms
{  
	u16 j,i;   
	for(j=0;j<x;j++)   
	{    
		for(i=0;i<110;i++);   
	}  
}

//LCD��ʾ���
void LCD_WriteData(u8 WrData)  //LCDģ��д����
{
	LCD_E    = 0;
	LCD_RS   = 1;
	LCD_RW   = 0;
	LCD_Data = WrData;
	Delay_ms(1);    //��ʱһС�������1602׼����������
	LCD_E    = 1;
	Delay_ms(2);    
	LCD_E    = 0;
}
void LCD_WriteCMD(u8 WrCmd)  //LCDģ��д����
{
	LCD_E    = 0;
	LCD_RS   = 0;
	LCD_RW   = 0;
	LCD_Data = WrCmd;
	Delay_ms(1);      //��ʱһС�������1602׼����������
	LCD_E    = 1; 
	Delay_ms(2);    
	LCD_E    = 0;
}

void LCD_Init(void)    //LCD��ʼ��             
{ 
	LCD_WriteCMD(0x38);       //8λ���ݣ�˫�У�5*7����  
	Delay_ms(5);
	LCD_WriteCMD(0x38);       //8λ���ݣ�˫�У�5*7����       
  Delay_ms(5);
	LCD_WriteCMD(0x38);       //8λ���ݣ�˫�У�5*7����  
	Delay_ms(5);
	LCD_WriteCMD(0x0c);       //������ʾ�����ع�꣬��겻��˸ 
  LCD_WriteCMD(0x06);       //��ʾ��ַ��������дһ�����ݺ���ʾλ������һλ 
  LCD_WriteCMD(0x01);       //���� 
} 		

void DisplayOneChar(u8 X, u8 Y, u8 DData)	 //LCD1602��ָ��λ����ʾһ���ַ�
{
	Y &= 0x1;
	X &= 0xF;                //����X���ܴ���15��Y���ܴ���1
	if (Y) X |= 0x40;        //��Ҫ��ʾ�ڶ���ʱ��ַ��+0x40;
	X |= 0x80;               //���ָ����
	LCD_WriteCMD(X);         //��������
	LCD_WriteData(DData);    //������
}

void DisplayListChar(u8 X, u8 Y, u8 *DData)	//LCD1602��ָ��λ����ʾһ���ַ�
{
	u8 ListLength;
	ListLength = 0;
	Y &= 0x1;
	X &= 0xF; //����X���ܴ���15��Y���ܴ���1
	while (DData[ListLength]>0x19) //�������ִ�β���˳�
	{
		if (X <= 0xF)        //X����ӦС��0xF
		{
			DisplayOneChar(X, Y, DData[ListLength]); //��ʾ�����ַ�
			ListLength++;
			X++;
		}
	}
}

u8 ADC(bit mode,bit channel)     //ADת�������ؽ��
{
	u8 i,dat,ndat;
	
	ADC_CS = 0;//����CS��
	_nop_();
	_nop_();
	
	ADC_DO_DI = 1;	//��1���½���Ϊ�ߵ�ƽ
	ADC_CLK = 1;//����CLK��
	_nop_();
	_nop_();
	ADC_CLK = 0;//����CLK��,�γ��½���1
	_nop_();
	_nop_();
	
	ADC_DO_DI = mode;	//�͵�ƽΪ���ģʽ���ߵ�ƽΪ��ͨ��ģʽ��	
	ADC_CLK = 1;//����CLK��
	_nop_();
	_nop_();
	ADC_CLK = 0;//����CLK��,�γ��½���2
	_nop_();
	_nop_();
	
	ADC_DO_DI = channel;	//�͵�ƽΪCH0���ߵ�ƽΪCH1	
	ADC_CLK = 1;//����CLK��
	_nop_();
	_nop_();
	ADC_CLK = 0;//����CLK��,�γ��½���3
	
	ADC_DO_DI = 1;//�����������(���������)
	dat = 0;
	//���濪ʼ��ȡת��������ݣ������λ��ʼ���������D7~D0��
	for(i = 0;i < 8;i++)
	{
		dat <<= 1;
		ADC_CLK=1;//����ʱ�Ӷ�
		_nop_();
		_nop_();
		ADC_CLK=0;//����ʱ�Ӷ��γ�һ��ʱ������
		_nop_();
		_nop_();
		dat |= ADC_DO_DI;
	}
	ndat = 0; 	   //��¼D0
	if(ADC_DO_DI == 1)
	ndat |= 0x80;
	//���濪ʼ������ȡ��������ݣ���D1��D7�� 
	for(i = 0;i < 7;i++)
	{
		ndat >>= 1;
		ADC_CLK = 1;//����ʱ�Ӷ�
		_nop_();
		_nop_();
		ADC_CLK=0;//����ʱ�Ӷ��γ�һ��ʱ������
		_nop_();
		_nop_();
		if(ADC_DO_DI==1)
		ndat |= 0x80;
	}	  
	ADC_CS=1;//����CS��,����ת��
	ADC_CLK=0;//����CLK��
	ADC_DO_DI=1;//�������ݶ�,�ص���ʼ״̬
	if(dat==ndat)
	return(dat);
	else
	return 0;   
}


/***********************18b20��ʼ������*****************************/
void init_18b20()
{
	bit q;
	DS18B20_DAT = 1;                                //�������ø�
	delay_us(1);            //15us
	DS18B20_DAT = 0;                                //����λ����
	delay_us(80);                //750us
	DS18B20_DAT = 1;                                //�������ø� �ȴ�
	delay_us(10);                //110us
	q = DS18B20_DAT;                                //��ȡ18b20��ʼ���ź�
	delay_us(20);                //200us
	DS18B20_DAT = 1;                                //�������ø� �ͷ�����
}

/*************д18b20�ڵ�����***************/
void write_18b20(u8 dat)
{
	u8 i;
	for(i=0;i<8;i++)
	{                                         //д�����ǵ�λ��ʼ
		DS18B20_DAT = 0;                         //�������õ�дʱ��϶��ʼ 
		DS18B20_DAT = dat & 0x01; //��18b20����д������
		delay_us(5);         // 60us
		DS18B20_DAT = 1;     //�ͷ�����
		dat >>= 1;
	}        
}

/*************��ȡ18b20�ڵ�����***************/
u8 read_18b20()
{
	u8 i,value;
	for(i=0;i<8;i++)
	{
		DS18B20_DAT = 0;                         //�������õͶ�ʱ��϶��ʼ 
		value >>= 1;         //�������ǵ�λ��ʼ
		DS18B20_DAT = 1;                         //�ͷ�����
		if(DS18B20_DAT == 1)                 //��ʼ��д���� 
				value |= 0x80;
		delay_us(5);         //60us        ��һ��ʱ��϶����Ҫ����60us��ʱ��
	}
	return value;                 //��������
}

/*************��ȡ�¶ȵ�ֵ ����������С��***************/
int read_temp() 
{
	int temp;     //����ֵ
	u8 low,high;  //��ȡ���ĵ�8λ�͸�8λ����
	
	init_18b20();                   //��ʼ��18b20

	write_18b20(0xcc);//����������ȡ64λROMָ��
	write_18b20(0x44);           //����һ���¶�ת������
	delay_us(50);                   //500us

	init_18b20();                   //��ʼ��18b20
	write_18b20(0xcc);//����������ȡ64λROMָ��
	write_18b20(0xbe);           //������ȡ�ݴ�������
	
	low  = read_18b20();         //���¶ȵ��ֽ�
	high = read_18b20();         //���¶ȸ��ֽ�
	temp = high*256+low;         //����

	return temp;                 //���ض������¶� ��С��
}

void Uart_Init() 	 //��ʱ�������ڳ�ʼ��
{
	SCON  = 0x50;   // SCON: ģʽ 1, 8-bit UART, ʹ�ܽ��� �����ڽ��գ�0x40 ��ֹ���ڽ��գ�
	TMOD |= 0x20;  // TMOD: timer 1, mode 2, 8-bit reload 	��ʱ��Y/C1������ʽ2 //PCON=0x80���ò�����9600 
	TH1   = 253;   //��ʱ����ֵ��8λ����     
	TL1   = 253;   //��ʱ����ֵ��8λ����
	TR1   = 1;     //��ʱ��1���ش򿪣���ʱ������       	   
	ES    = 1;     //�򿪴����ж� ������UART�����ж�
	EA    = 1;     //�����жϡ��������ж�     
	TI    = 0;	   // ����жϱ�־λΪ0
	RI    = 0;		//������жϱ�־λΪ0
}
   //�򴮿ڷ���һ���ַ�
void SendASC(unsigned char d)
{	
	SBUF=d;	 //���ڽ��յ������ݷ��ͳ�ȥ
	while(!TI);	  //��鷢���жϱ�־λ
	TI=0;  // ����жϱ�־λΪ0
}
  //�򴮿ڷ���һ���ַ��������޳���
void SendString(unsigned char *str)	
{
	while(*str)
	{
		SendASC(*str) ;
		str++;
	}
}

void GSM_work()
{
	SendString("AT\r\n");  
	Delay_ms(200);
	SendString("AT+CMGF=1\r\n");//���ö���ģʽΪ�ı�ģʽ	
	Delay_ms(200);
	SendString("AT+CSCS=\"IRA\"\r\n");	 //�����ַ�������
	Delay_ms(200);
	SendString("AT+CSMP=17,11,0,0\r\n");//����Ӣ��ģʽ	
	Delay_ms(200);
	SendString("AT+CMGS=\"13358822086\"\r\n");	//��Ϣ����ָ�� AT+CMGS=//
	Delay_ms(500);
	SendString("The soil moisture is too low, please note!");					   
	Delay_ms(200);		
	SendASC(0x1a);
}	

void MotorFunction(u8 mode,u16 time)
{
	if(mode==0) //��ת
	{
		Motor1=1; Motor2=0; Motor3=0;Motor4=0;
		Delay_ms(time);
		Motor1=0; Motor2=1; Motor3=0;Motor4=0;
		Delay_ms(time);
		Motor1=0; Motor2=0; Motor3=1;Motor4=0;
		Delay_ms(time);
		Motor1=0; Motor2=0; Motor3=0;Motor4=1;
		Delay_ms(time);
		Motor4=0;
	}
	if(mode==1) //��ת
	{
		Motor1=0; Motor2=0; Motor3=0;Motor4=1;
		Delay_ms(time);
		Motor1=0; Motor2=0; Motor3=1;Motor4=0;
		Delay_ms(time);
		Motor1=0; Motor2=1; Motor3=0;Motor4=0;
		Delay_ms(time);
		Motor1=1; Motor2=0; Motor3=0;Motor4=0;
		Delay_ms(time);
		Motor1=0;
	}
}


void main()
{
	u8 i;
	int temp;
	LCD_Init();  
	DisplayListChar(0,0,"System Init....");
	Delay_ms(300);
	Uart_Init();
	cLight=(255-(u16)ADC(1,0))*100/255; //�������ֵ
	cHumi=(255-(u16)ADC(1,1))*100/255; //����ʪ��ֵ
	temp=read_temp(); //����¶�
	Delay_ms(5000);
	Delay_ms(5000);
	Delay_ms(5000);
	SendString("AT\r\n");  //AT\r\n���������Ƿ�ɹ�
	DisplayListChar(0,1,"Init OK!");
	Delay_ms(5000);
	LCD_WriteCMD(0x01);       //���� 
	while(1)
	{
		if(presskeynum==0)
		{
			cLight=(255-(u16)ADC(1,0))*100/255; //�������ֵ
			cHumi=(255-(u16)ADC(1,1))*100/255; //����ʪ��ֵ
			temp=read_temp(); //�����¶�ֵ
			ctemp=temp/16;
			
			DisplayListChar(0,0,"T");  //��ʾ�¶�
			DisplayOneChar(1,0,ctemp/10+0x30);
			DisplayOneChar(2,0,ctemp%10+0x30);
			DisplayListChar(3,0,"-");
			DisplayOneChar(4,0,stemp_H/10+0x30);
			DisplayOneChar(5,0,stemp_H%10+0x30);
			DisplayListChar(6,0,"-");
			DisplayOneChar(7,0,stemp_L/10+0x30);
			DisplayOneChar(8,0,stemp_L%10+0x30);

			DisplayListChar(10,1,"H"); //��ʾʪ��
			DisplayOneChar(11,1,cHumi/10%10+0x30);
			DisplayOneChar(12,1,cHumi%10+0x30);
			DisplayOneChar(13,1,'-');
			DisplayOneChar(14,1,sHumi/10%10+0x30);
			DisplayOneChar(15,1,sHumi%10+0x30);		
			
			DisplayListChar(0,1,"L"); //��ʾ����
			DisplayOneChar(1,1,cLight/10%10+0x30);
			DisplayOneChar(2,1,cLight%10+0x30);
			DisplayOneChar(3,1,'-');
			DisplayOneChar(4,1,sLight_H/10%10+0x30);
			DisplayOneChar(5,1,sLight_H%10+0x30);
			DisplayOneChar(6,1,'-');
			DisplayOneChar(7,1,sLight_L/10%10+0x30);
			DisplayOneChar(8,1,sLight_L%10+0x30);
			
			if((cHumi<sHumi)&&(humiflag==0))
			{
				
				GSM_work(); //��������ʪ�ȵͣ���ע�⡣
				Delay_ms(2000);
				humiflag=1;
			}
			else if(cHumi>=sHumi)
			{
				humiflag=0;
				i=0;
			}
			
			if((lightflag==1)&&(motorflag==1))
			{
				MotorFunction(1,5);
				if(motortime<300)
					motortime++;
				if(motortime==300)
				{
					motorflag=0;
				}
			}
			else if((lightflag!=1)&&(motorflag==1))
			{
				MotorFunction(0,5);
				if(motortime>0)
					motortime--;
				if(motortime==0)
				{
					motorflag=0;
				}
			}
			
			if(workmode==0) 
			{
				DisplayListChar(10,0,"Auto"); 
				if((ctemp>stemp_H)&&(tempflag!=1)) 
				{
					RY_Fan=0;
					fanflag=1;
					tempflag=1;
				}
				else if((ctemp<stemp_H)&&(ctemp>=stemp_L)&&(tempflag==1))
				{
					RY_Fan=1;
					fanflag=0;
					tempflag=0;
				}
				else if((ctemp<stemp_L)&&(tempflag!=2))
				{
					RY_Hot=0;
					hotflag=1;
					tempflag=2;
				}
				else if((ctemp>stemp_L)&&(ctemp<=stemp_H)&&(tempflag==2))
				{
					RY_Hot=1;
					hotflag=0;
					tempflag=0;
				}
				
				if((cLight<sLight_L)&&(lightflag!=2)) 
				{
					lightflag=2;
					RY_LED=0;
					ledflag=1;
				}
				else if((cLight>sLight_L)&&(cLight<=sLight_H)&&(lightflag==2))
				{
					lightflag=0;
					RY_LED=1;
					ledflag=0;
				}
				else if((cLight>sLight_H)&&(lightflag!=1))
				{
					lightflag=1;
					motorflag=1;
				}
				else if((cLight<sLight_H)&&(cLight>=sLight_L)&&(lightflag==1))
				{
					lightflag=0;
					motorflag=1;
				}
			}
			else if(workmode==1)  //�ֶ�ģʽ
			{
				DisplayListChar(10,0,"Manu"); 
				if(Key_Fan==0)
				{
					Delay_ms(5);
					if(Key_Fan==0)
					{
						if(fanflag==0) //����״̬Ϊ�أ����
						{
							RY_Fan=0;
							fanflag=1;
						}
						else if(fanflag==1)//����
						{
							RY_Fan=1;
							fanflag=0;
						}
						while(Key_Fan==0);
					}
				}
				if(Key_Hot==0)
				{
					Delay_ms(5);
					if(Key_Hot==0)
					{
						if(hotflag==0) //����״̬Ϊ�أ����
						{
							RY_Hot=0;
							hotflag=1;
						}
						else if(hotflag==1)//����
						{
							RY_Hot=1;
							hotflag=0;
						}
						while(Key_Hot==0);
					}
				}
				if(Key_Light==0)
				{
					Delay_ms(5);
					if(Key_Light==0)
					{
						if(lightflag==0) //������״̬Ϊ�أ����
						{
							motorflag=1;
							lightflag=1;
						}
						else if(lightflag==1)//����
						{
							motorflag=1;
							lightflag=0;
						}
						while(Key_Light==0);
					}
				}
				if(Key_LED==0)
				{
					Delay_ms(5);
					if(Key_LED==0)
					{
						if(ledflag==0) //������״̬Ϊ�أ����
						{
							RY_LED=0;
							ledflag=1;
						}
						else if(ledflag==1)//����
						{
							RY_LED=1;
							ledflag=0;
						}
						while(Key_LED==0);
					}
				}
			}
		}
		if(presskeynum==1)  //�����¶���ֵ
		{
			if(Key_Plus==0)    // ��
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					stemp_H++;
					if(stemp_H>50)
					{
						stemp_H=stemp_L+5;
					}
					while(Key_Plus==0); //���ϴ˾�����ɰ����Ŵ���
				}
			}
			if(Key_Dec==0)    // ��
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					stemp_H--;
					if(stemp_H<stemp_L+5)
					{
						stemp_H=50;
					}
					while(Key_Dec==0);       //���ϴ˾�����ɰ����Ŵ���
				}
			}
			DisplayListChar(0,0,"Set Temp High:");	
			DisplayOneChar(0,1,stemp_H/10+0x30);
			DisplayOneChar(1,1,stemp_H%10+0x30);
		}	
		
		if(presskeynum==2)  //�����¶���ֵ
		{
			if(Key_Plus==0)    // ��
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					stemp_L++;
					if(stemp_L>stemp_H-5)
					{
						stemp_L=10;
					}
					while(Key_Plus==0); //���ϴ˾�����ɰ����Ŵ���
				}
			}
			if(Key_Dec==0)    // ��
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					stemp_L--;
					if(stemp_L<10)
					{
						stemp_L=stemp_H-5;
					}
					while(Key_Dec==0);       //���ϴ˾�����ɰ����Ŵ���
				}
			}
			DisplayListChar(0,0,"Set Temp Low:");	
			DisplayOneChar(0,1,stemp_L/10+0x30);
			DisplayOneChar(1,1,stemp_L%10+0x30);
		}	
		
		if(presskeynum==3)  //��������ʪ����ֵ
		{
			if(Key_Plus==0)    // ��
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					sHumi+=5;
					if(sHumi>90)
					{
						sHumi=10;
					}
					while(Key_Plus==0); //���ϴ˾�����ɰ����Ŵ���
				}
			}
			if(Key_Dec==0)    // ��
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					sHumi-=5;
					if(sHumi<10)
					{
						sHumi=90;
					}
					while(Key_Dec==0);       //���ϴ˾�����ɰ����Ŵ���
				}
			}
			DisplayListChar(0,0,"Set Humi Value:");	
			DisplayOneChar(0,1,sHumi/10+0x30);
			DisplayOneChar(1,1,sHumi%10+0x30);
		}	
		if(presskeynum==4)  //���ù���������ֵ
		{
			if(Key_Plus==0)    // ��
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					sLight_H+=5;
					if(sLight_H>90)
					{
						sLight_H=sLight_L+10;
					}
					while(Key_Plus==0); //���ϴ˾�����ɰ����Ŵ���
				}
			}
			if(Key_Dec==0)    // ��
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					sLight_H-=5;
					if(sLight_H<sLight_L-10)
					{
						sLight_H=90;
					}
					while(Key_Dec==0);       //���ϴ˾�����ɰ����Ŵ���
				}
			}
			DisplayListChar(0,0,"Set Light High:");	
			DisplayOneChar(0,1,sLight_H/10+0x30);
			DisplayOneChar(1,1,sLight_H%10+0x30);
		}	
		if(presskeynum==5)  //���ù���������ֵ
		{
			if(Key_Plus==0)    // ��
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					sLight_L+=5;
					if(sLight_L>sLight_H-10)
					{
						sLight_L=90;
					}
					while(Key_Plus==0); //���ϴ˾�����ɰ����Ŵ���
				}
			}
			if(Key_Dec==0)    // ��
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					sLight_L-=5;
					if(sLight_L<10)
					{
						sLight_L=sLight_H-10;
					}
					while(Key_Dec==0);       //���ϴ˾�����ɰ����Ŵ���
				}
			}
			DisplayListChar(0,0,"Set Light Low:");	
			DisplayOneChar(0,1,sLight_L/10+0x30);
			DisplayOneChar(1,1,sLight_L%10+0x30);
		}
		if(presskeynum==6)  //���ù���ģʽ
		{
			if(Key_Plus==0)    // ��
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					if(workmode==0) workmode=1;
					else workmode=0;
					while(Key_Plus==0); //���ϴ˾�����ɰ����Ŵ���
				}
			}
			if(Key_Dec==0)    // ��
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					if(workmode==0) workmode=1;
					else workmode=0;
					while(Key_Dec==0);       //���ϴ˾�����ɰ����Ŵ���
				}
			}
			DisplayListChar(0,0,"Set WorkMode:");	
			if(workmode==0)  DisplayListChar(0,1,"AutoMode");
			else if(workmode==1)  DisplayListChar(0,1,"Manual  ");
		}	
		if(presskeynum==7)
		{
			presskeynum=0;
			
		}

	}
}

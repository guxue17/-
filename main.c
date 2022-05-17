#include "reg51.H"
#include <intrins.h> 
#include <string.h>           // 加入此头文件后,可使用strstr库函数
#include <stdio.h>

sbit LCD_RS     = P1^0;  //寄存器选择输入端，LCD1602的第四脚
sbit LCD_RW     = P1^1;  //读写控制输入端，LCD1602的第五脚
sbit LCD_E      = P1^2;  //使能信号输入端, LCD1602的第六脚

sbit DS18B20_DAT  = P1^3;  //DS18B20 DATA信号

sbit ADC_CS=P2^7;   //ADC0832 CS脚
sbit ADC_CLK=P2^6;   //ADC0832 CS脚
sbit ADC_DO_DI=P2^5;   //ADC0832 CS脚

sbit Key_SET    = P3^2;  //设置
sbit Key_Plus   = P3^3;  //加(+)
sbit Key_Dec    = P3^4;  //减(-)

sbit Key_Fan    = P3^5;  //风扇按键
sbit Key_Hot    = P3^6;  //加热按键
sbit Key_LED    = P3^7;  //照明按键
sbit Key_Light  = P2^3;  //遮光按键

sbit RY_Fan     = P2^0;  //风扇继电器
sbit RY_Hot     = P2^1;  //加热继电器
sbit RY_LED     = P2^2;  //照明继电器
 
//步进电机 
sbit Motor1     = P1^4;
sbit Motor2     = P1^5;
sbit Motor3     = P1^6;
sbit Motor4     = P1^7;


sbit BEEP=P2^4; //蜂鸣器


#define u8  unsigned char
#define u16 unsigned int

#define   LCD_Data		 P0      		//LCD1602数据接口

u16 KeyValue,KeyState;

u8 xdata buff[20];//参数显示缓存数组

u16 ctemp;        //实时温度变量
u16 cLight;       //实时光照变量
u16 cHumi;       //实时湿度变量

u8 stemp_H=25;      //保存设置温度上限阈值变量,初值为25
u8 stemp_L=20;      //保存设置温度下限阈值变量,初值为20
u8 sLight_H=70;     //保存设置光照上限阈值变量,初值为70
u8 sLight_L=30;     //保存设置光照下限阈值变量,初值为30
u8 sHumi=30;     //保存设置湿度阈值变量,初值为30

u8 presskeynum=0;    //按键计数 
u8 setflag=0;
u8 workmode=0; //工作模式 0 自动 1手动

u8 tempflag=0; //温度状态标志
u8 lightflag=0; //光照状态标志
u8 humiflag=0; //光照状态标志
u8 fanflag=0; //风扇状态
u8 hotflag=0; //加热状态
u8 ledflag=0; //led状态
u16 motortime=0;
u8 motorflag=0; //步进电机状态

//延时相关函数
void delay_us(unsigned int q)//延时函数1us
{
	while(q--);
}

void Delay_ms(u16 x) 	//延时xms
{  
	u16 j,i;   
	for(j=0;j<x;j++)   
	{    
		for(i=0;i<110;i++);   
	}  
}

//LCD显示相关
void LCD_WriteData(u8 WrData)  //LCD模块写数据
{
	LCD_E    = 0;
	LCD_RS   = 1;
	LCD_RW   = 0;
	LCD_Data = WrData;
	Delay_ms(1);    //延时一小会儿，让1602准备接收数据
	LCD_E    = 1;
	Delay_ms(2);    
	LCD_E    = 0;
}
void LCD_WriteCMD(u8 WrCmd)  //LCD模块写命令
{
	LCD_E    = 0;
	LCD_RS   = 0;
	LCD_RW   = 0;
	LCD_Data = WrCmd;
	Delay_ms(1);      //延时一小会儿，让1602准备接收数据
	LCD_E    = 1; 
	Delay_ms(2);    
	LCD_E    = 0;
}

void LCD_Init(void)    //LCD初始化             
{ 
	LCD_WriteCMD(0x38);       //8位数据，双列，5*7字形  
	Delay_ms(5);
	LCD_WriteCMD(0x38);       //8位数据，双列，5*7字形       
  Delay_ms(5);
	LCD_WriteCMD(0x38);       //8位数据，双列，5*7字形  
	Delay_ms(5);
	LCD_WriteCMD(0x0c);       //开启显示屏，关光标，光标不闪烁 
  LCD_WriteCMD(0x06);       //显示地址递增，即写一个数据后，显示位置右移一位 
  LCD_WriteCMD(0x01);       //清屏 
} 		

void DisplayOneChar(u8 X, u8 Y, u8 DData)	 //LCD1602按指定位置显示一个字符
{
	Y &= 0x1;
	X &= 0xF;                //限制X不能大于15，Y不能大于1
	if (Y) X |= 0x40;        //当要显示第二行时地址码+0x40;
	X |= 0x80;               //算出指令码
	LCD_WriteCMD(X);         //发命令字
	LCD_WriteData(DData);    //发数据
}

void DisplayListChar(u8 X, u8 Y, u8 *DData)	//LCD1602按指定位置显示一串字符
{
	u8 ListLength;
	ListLength = 0;
	Y &= 0x1;
	X &= 0xF; //限制X不能大于15，Y不能大于1
	while (DData[ListLength]>0x19) //若到达字串尾则退出
	{
		if (X <= 0xF)        //X坐标应小于0xF
		{
			DisplayOneChar(X, Y, DData[ListLength]); //显示单个字符
			ListLength++;
			X++;
		}
	}
}

u8 ADC(bit mode,bit channel)     //AD转换，返回结果
{
	u8 i,dat,ndat;
	
	ADC_CS = 0;//拉低CS端
	_nop_();
	_nop_();
	
	ADC_DO_DI = 1;	//第1个下降沿为高电平
	ADC_CLK = 1;//拉高CLK端
	_nop_();
	_nop_();
	ADC_CLK = 0;//拉低CLK端,形成下降沿1
	_nop_();
	_nop_();
	
	ADC_DO_DI = mode;	//低电平为差分模式，高电平为单通道模式。	
	ADC_CLK = 1;//拉高CLK端
	_nop_();
	_nop_();
	ADC_CLK = 0;//拉低CLK端,形成下降沿2
	_nop_();
	_nop_();
	
	ADC_DO_DI = channel;	//低电平为CH0，高电平为CH1	
	ADC_CLK = 1;//拉高CLK端
	_nop_();
	_nop_();
	ADC_CLK = 0;//拉低CLK端,形成下降沿3
	
	ADC_DO_DI = 1;//控制命令结束(经试验必需)
	dat = 0;
	//下面开始读取转换后的数据，从最高位开始依次输出（D7~D0）
	for(i = 0;i < 8;i++)
	{
		dat <<= 1;
		ADC_CLK=1;//拉高时钟端
		_nop_();
		_nop_();
		ADC_CLK=0;//拉低时钟端形成一次时钟脉冲
		_nop_();
		_nop_();
		dat |= ADC_DO_DI;
	}
	ndat = 0; 	   //记录D0
	if(ADC_DO_DI == 1)
	ndat |= 0x80;
	//下面开始继续读取反序的数据（从D1到D7） 
	for(i = 0;i < 7;i++)
	{
		ndat >>= 1;
		ADC_CLK = 1;//拉高时钟端
		_nop_();
		_nop_();
		ADC_CLK=0;//拉低时钟端形成一次时钟脉冲
		_nop_();
		_nop_();
		if(ADC_DO_DI==1)
		ndat |= 0x80;
	}	  
	ADC_CS=1;//拉高CS端,结束转换
	ADC_CLK=0;//拉低CLK端
	ADC_DO_DI=1;//拉高数据端,回到初始状态
	if(dat==ndat)
	return(dat);
	else
	return 0;   
}


/***********************18b20初始化函数*****************************/
void init_18b20()
{
	bit q;
	DS18B20_DAT = 1;                                //把总线拿高
	delay_us(1);            //15us
	DS18B20_DAT = 0;                                //给复位脉冲
	delay_us(80);                //750us
	DS18B20_DAT = 1;                                //把总线拿高 等待
	delay_us(10);                //110us
	q = DS18B20_DAT;                                //读取18b20初始化信号
	delay_us(20);                //200us
	DS18B20_DAT = 1;                                //把总线拿高 释放总线
}

/*************写18b20内的数据***************/
void write_18b20(u8 dat)
{
	u8 i;
	for(i=0;i<8;i++)
	{                                         //写数据是低位开始
		DS18B20_DAT = 0;                         //把总线拿低写时间隙开始 
		DS18B20_DAT = dat & 0x01; //向18b20总线写数据了
		delay_us(5);         // 60us
		DS18B20_DAT = 1;     //释放总线
		dat >>= 1;
	}        
}

/*************读取18b20内的数据***************/
u8 read_18b20()
{
	u8 i,value;
	for(i=0;i<8;i++)
	{
		DS18B20_DAT = 0;                         //把总线拿低读时间隙开始 
		value >>= 1;         //读数据是低位开始
		DS18B20_DAT = 1;                         //释放总线
		if(DS18B20_DAT == 1)                 //开始读写数据 
				value |= 0x80;
		delay_us(5);         //60us        读一个时间隙最少要保持60us的时间
	}
	return value;                 //返回数据
}

/*************读取温度的值 读出来的是小数***************/
int read_temp() 
{
	int temp;     //返回值
	u8 low,high;  //读取到的低8位和高8位数据
	
	init_18b20();                   //初始化18b20

	write_18b20(0xcc);//发送跳过读取64位ROM指令
	write_18b20(0x44);           //启动一次温度转换命令
	delay_us(50);                   //500us

	init_18b20();                   //初始化18b20
	write_18b20(0xcc);//发送跳过读取64位ROM指令
	write_18b20(0xbe);           //发出读取暂存器命令
	
	low  = read_18b20();         //读温度低字节
	high = read_18b20();         //读温度高字节
	temp = high*256+low;         //计算

	return temp;                 //返回读出的温度 带小数
}

void Uart_Init() 	 //定时器、串口初始化
{
	SCON  = 0x50;   // SCON: 模式 1, 8-bit UART, 使能接收 允许串口接收（0x40 禁止串口接收）
	TMOD |= 0x20;  // TMOD: timer 1, mode 2, 8-bit reload 	定时器Y/C1工作方式2 //PCON=0x80设置波特率9600 
	TH1   = 253;   //定时器初值高8位设置     
	TL1   = 253;   //定时器初值低8位设置
	TR1   = 1;     //定时器1开关打开，定时器启动       	   
	ES    = 1;     //打开串口中断 ，允许UART串口中断
	EA    = 1;     //打开总中断、允许总中断     
	TI    = 0;	   // 令发送中断标志位为0
	RI    = 0;		//令接收中断标志位为0
}
   //向串口发送一个字符
void SendASC(unsigned char d)
{	
	SBUF=d;	 //将在接收到的数据发送出去
	while(!TI);	  //检查发送中断标志位
	TI=0;  // 令发送中断标志位为0
}
  //向串口发送一个字符串，不限长度
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
	SendString("AT+CMGF=1\r\n");//设置短信模式为文本模式	
	Delay_ms(200);
	SendString("AT+CSCS=\"IRA\"\r\n");	 //设置字符集编码
	Delay_ms(200);
	SendString("AT+CSMP=17,11,0,0\r\n");//设置英文模式	
	Delay_ms(200);
	SendString("AT+CMGS=\"13358822086\"\r\n");	//信息发送指令 AT+CMGS=//
	Delay_ms(500);
	SendString("The soil moisture is too low, please note!");					   
	Delay_ms(200);		
	SendASC(0x1a);
}	

void MotorFunction(u8 mode,u16 time)
{
	if(mode==0) //正转
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
	if(mode==1) //反转
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
	cLight=(255-(u16)ADC(1,0))*100/255; //计算光照值
	cHumi=(255-(u16)ADC(1,1))*100/255; //计算湿度值
	temp=read_temp(); //检测温度
	Delay_ms(5000);
	Delay_ms(5000);
	Delay_ms(5000);
	SendString("AT\r\n");  //AT\r\n测试连接是否成功
	DisplayListChar(0,1,"Init OK!");
	Delay_ms(5000);
	LCD_WriteCMD(0x01);       //清屏 
	while(1)
	{
		if(presskeynum==0)
		{
			cLight=(255-(u16)ADC(1,0))*100/255; //计算光照值
			cHumi=(255-(u16)ADC(1,1))*100/255; //计算湿度值
			temp=read_temp(); //计算温度值
			ctemp=temp/16;
			
			DisplayListChar(0,0,"T");  //显示温度
			DisplayOneChar(1,0,ctemp/10+0x30);
			DisplayOneChar(2,0,ctemp%10+0x30);
			DisplayListChar(3,0,"-");
			DisplayOneChar(4,0,stemp_H/10+0x30);
			DisplayOneChar(5,0,stemp_H%10+0x30);
			DisplayListChar(6,0,"-");
			DisplayOneChar(7,0,stemp_L/10+0x30);
			DisplayOneChar(8,0,stemp_L%10+0x30);

			DisplayListChar(10,1,"H"); //显示湿度
			DisplayOneChar(11,1,cHumi/10%10+0x30);
			DisplayOneChar(12,1,cHumi%10+0x30);
			DisplayOneChar(13,1,'-');
			DisplayOneChar(14,1,sHumi/10%10+0x30);
			DisplayOneChar(15,1,sHumi%10+0x30);		
			
			DisplayListChar(0,1,"L"); //显示光照
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
				
				GSM_work(); //发送土壤湿度低，请注意。
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
			else if(workmode==1)  //手动模式
			{
				DisplayListChar(10,0,"Manu"); 
				if(Key_Fan==0)
				{
					Delay_ms(5);
					if(Key_Fan==0)
					{
						if(fanflag==0) //风扇状态为关，则打开
						{
							RY_Fan=0;
							fanflag=1;
						}
						else if(fanflag==1)//否则
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
						if(hotflag==0) //风扇状态为关，则打开
						{
							RY_Hot=0;
							hotflag=1;
						}
						else if(hotflag==1)//否则
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
						if(lightflag==0) //照明灯状态为关，则打开
						{
							motorflag=1;
							lightflag=1;
						}
						else if(lightflag==1)//否则
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
						if(ledflag==0) //照明灯状态为关，则打开
						{
							RY_LED=0;
							ledflag=1;
						}
						else if(ledflag==1)//否则
						{
							RY_LED=1;
							ledflag=0;
						}
						while(Key_LED==0);
					}
				}
			}
		}
		if(presskeynum==1)  //设置温度阈值
		{
			if(Key_Plus==0)    // 加
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					stemp_H++;
					if(stemp_H>50)
					{
						stemp_H=stemp_L+5;
					}
					while(Key_Plus==0); //加上此句必须松按键才处理
				}
			}
			if(Key_Dec==0)    // 减
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					stemp_H--;
					if(stemp_H<stemp_L+5)
					{
						stemp_H=50;
					}
					while(Key_Dec==0);       //加上此句必须松按键才处理
				}
			}
			DisplayListChar(0,0,"Set Temp High:");	
			DisplayOneChar(0,1,stemp_H/10+0x30);
			DisplayOneChar(1,1,stemp_H%10+0x30);
		}	
		
		if(presskeynum==2)  //设置温度阈值
		{
			if(Key_Plus==0)    // 加
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					stemp_L++;
					if(stemp_L>stemp_H-5)
					{
						stemp_L=10;
					}
					while(Key_Plus==0); //加上此句必须松按键才处理
				}
			}
			if(Key_Dec==0)    // 减
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					stemp_L--;
					if(stemp_L<10)
					{
						stemp_L=stemp_H-5;
					}
					while(Key_Dec==0);       //加上此句必须松按键才处理
				}
			}
			DisplayListChar(0,0,"Set Temp Low:");	
			DisplayOneChar(0,1,stemp_L/10+0x30);
			DisplayOneChar(1,1,stemp_L%10+0x30);
		}	
		
		if(presskeynum==3)  //设置土壤湿度阈值
		{
			if(Key_Plus==0)    // 加
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					sHumi+=5;
					if(sHumi>90)
					{
						sHumi=10;
					}
					while(Key_Plus==0); //加上此句必须松按键才处理
				}
			}
			if(Key_Dec==0)    // 减
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					sHumi-=5;
					if(sHumi<10)
					{
						sHumi=90;
					}
					while(Key_Dec==0);       //加上此句必须松按键才处理
				}
			}
			DisplayListChar(0,0,"Set Humi Value:");	
			DisplayOneChar(0,1,sHumi/10+0x30);
			DisplayOneChar(1,1,sHumi%10+0x30);
		}	
		if(presskeynum==4)  //设置光照上限阈值
		{
			if(Key_Plus==0)    // 加
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					sLight_H+=5;
					if(sLight_H>90)
					{
						sLight_H=sLight_L+10;
					}
					while(Key_Plus==0); //加上此句必须松按键才处理
				}
			}
			if(Key_Dec==0)    // 减
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					sLight_H-=5;
					if(sLight_H<sLight_L-10)
					{
						sLight_H=90;
					}
					while(Key_Dec==0);       //加上此句必须松按键才处理
				}
			}
			DisplayListChar(0,0,"Set Light High:");	
			DisplayOneChar(0,1,sLight_H/10+0x30);
			DisplayOneChar(1,1,sLight_H%10+0x30);
		}	
		if(presskeynum==5)  //设置光照下限阈值
		{
			if(Key_Plus==0)    // 加
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					sLight_L+=5;
					if(sLight_L>sLight_H-10)
					{
						sLight_L=90;
					}
					while(Key_Plus==0); //加上此句必须松按键才处理
				}
			}
			if(Key_Dec==0)    // 减
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					sLight_L-=5;
					if(sLight_L<10)
					{
						sLight_L=sLight_H-10;
					}
					while(Key_Dec==0);       //加上此句必须松按键才处理
				}
			}
			DisplayListChar(0,0,"Set Light Low:");	
			DisplayOneChar(0,1,sLight_L/10+0x30);
			DisplayOneChar(1,1,sLight_L%10+0x30);
		}
		if(presskeynum==6)  //设置工作模式
		{
			if(Key_Plus==0)    // 加
			{
				Delay_ms(5);
				if(Key_Plus==0)
				{
					if(workmode==0) workmode=1;
					else workmode=0;
					while(Key_Plus==0); //加上此句必须松按键才处理
				}
			}
			if(Key_Dec==0)    // 减
			{
				Delay_ms(5);
				if(Key_Dec==0)
				{
					if(workmode==0) workmode=1;
					else workmode=0;
					while(Key_Dec==0);       //加上此句必须松按键才处理
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

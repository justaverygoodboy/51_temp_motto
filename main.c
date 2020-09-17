#include "reg52.h"
#include "lcd.h"
#include "temp.h"
#include "i2c.h"
#include <string.h>
typedef unsigned int u16;
typedef unsigned char u8;

u8 Temp[5]={'0','0','0','0'};
u8 trigTemp[5]={'0','0','0','0'};
u8 tempnow[6]={'t','e','m','p',':'};
u8 setting[10]={'s','e','t',' ','t','e','m','p',':'};
unsigned char* trigTempPtr;
u8 isSetMode;
u8 keyState;
u8 ptrIndex;

uint state=0;
uint count=0;
sbit k6=P3^0;				//减
sbit k2=P3^1;				//加
sbit k3=P3^2;				//左移
sbit k4=P3^3;				//右移
sbit k7=P3^5;				//进入设置
sbit k8=P3^6;				//确定
sbit kReset=P1^0;			//重置
sbit motto=P1^2;
sbit beep=P1^5;
sfr WDT_CONTR=0xe1;
void UsartInit()
{
	SCON=0X50;			//设置为工作方式1
	TMOD=0X20;			//设置计数器工作方式2
	PCON=0X80;			//波特率加倍
	TH1=0XF3;				//计数器初始值设置，注意波特率是4800的
	TL1=0XF3;						//打开总中断
	TR1=1;					//打开计数器
}
void delay(u16 i)
{
	while(i--);	
}
void Di(){
	u8 i=100;
	while(i--)
	{
		beep=~beep;
		delay(100);	
	}
}
unsigned char *Itoa(unsigned int ni,int dd){	   //整数转字符串
	char i=0,j=0,temp[16],outstr[16];
	unsigned int n,num=ni;
	while(num>=dd){
		n=num%dd;
  		if(n>9)
			temp[i]=n+0x37;
		else 
			temp[i]=n+0x30;
  		num=num/dd;
  		i++;
  	}
  	n=num;
	if(n>9)
		temp[i]=n+0x37;
	else
		temp[i]=n+0x30;
  	j=0;
  	for(;i>=0;i--){
		outstr[j]=temp[i];
		j++;
	}
  	outstr[j]=0;
  	return outstr;
}
int GetRealTemp(temp){
	float tp;
	if(temp<0){
		temp=temp-1;
		temp=~temp;
		tp=temp;
		temp=tp*0.0625*100+0.5;
	}
	if(temp>0){
		tp=temp;
		temp=tp*0.0625*100+0.5;
	}
	return temp;
}
void SetTrigTempButton(){
	WDT_CONTR=0x35;//喂狗
	if(k2==0)
	{
		delay(1000);
		if(k2==0)
		{
			Di();
			keyState=1;
			if(*trigTempPtr<'9')
				*trigTempPtr=*trigTempPtr+1;
			else if(*trigTempPtr=='9')
				*trigTempPtr='0';
		}
		while(!k2);
	}				  
	if(k6==0)
	{
		delay(1000);
		if(k6==0)
		{ 	
			Di();
			keyState=1;
			if(*trigTempPtr>'0')
				*trigTempPtr=*trigTempPtr-1;
			else if(*trigTempPtr=='0')
				*trigTempPtr='9';
		}
		while(!k6);
	}
	if(k3==0)
	{
		delay(1000);
		if(k3==0)
		{	
			Di();
			keyState=1;
			if(ptrIndex>0){
				trigTempPtr=trigTempPtr-1;
				ptrIndex=ptrIndex-1;
			}
		}
		while(!k3);
	}
	if(k4==0)
	{
		delay(1000);
		if(k4==0)
		{	
			Di();
			keyState=1;
			if(ptrIndex<3){
				trigTempPtr=trigTempPtr+1;
				ptrIndex=ptrIndex+1;
			}	
		}
		while(!k4);
	}
}
void TempDisplay(){
	u8 i;
	int temp=GetRealTemp(Ds18b20ReadTemp());
	strcpy(Temp,Itoa(temp,10));
	for(i=0;i<5;i++){
		LcdWriteData(tempnow[i]);
	}
	for(i=0;i<4;i++)
	{
		if(i==2)
			LcdWriteData('.');
		LcdWriteData(Temp[i]);
	}	

}
void intoSetMode(){
	u8 i;
	delay(5000);
	trigTemp[0]=At24c02Read(1);
	delay(5000);
	trigTemp[1]=At24c02Read(2);
	delay(5000);
	trigTemp[2]=At24c02Read(3);
	delay(5000);
	trigTemp[3]=At24c02Read(4);
	delay(5000);
	LcdInit();
	while(isSetMode){
		if(keyState){
			LcdInit();
			keyState=0;
			for(i=0;i<9;i++){
				LcdWriteData(setting[i]);
			}
			for(i=0;i<4;i++){
				if(i==2)
					LcdWriteData('.');
				LcdWriteData(trigTemp[i]);
			}
		}
		SetTrigTempButton();
		if(k8==0)
		{
			isSetMode=0;
			Di();
			At24c02Write(1,trigTemp[0]);
			delay(5000);
			At24c02Write(2,trigTemp[1]);
			delay(5000);
			At24c02Write(3,trigTemp[2]);
			delay(5000);
			At24c02Write(4,trigTemp[3]);
			delay(5000);
			At24c02Write(5,'n');
			LcdInit();
		}
	}
	TempDisplay();
}
void TrigMoto(){
	uint i;

	if(strcmp(trigTemp,Temp)<0){
		motto=1;
		if(count==0&&state==0){
			state=1;
			count++;
			for(i=0;i<4;i++)
			{
			if(i==2)
				SBUF='.';
		 	SBUF=Temp[i];//将接收到的数据放入到发送寄存器
			while (!TI);//等待发送数据完成
			TI = 0;
			}
			state=1;
		}
	} else{
		motto=0;
		state=0;
		count=0;
	}
	
}
void SetTrigTempInit(){
	isSetMode = 0;//未进入设置模式
	delay(5000);
	if(At24c02Read(10)!='y'){//设置下次进入不会初始化trigTemp
		trigTemp[0]='3'; //默认设置触发温度为30度
		trigTemp[1]='0';
		trigTemp[2]='0';
		trigTemp[3]='0';
		delay(5000);
		At24c02Write(1,trigTemp[0]);
		delay(5000);
		At24c02Write(2,trigTemp[1]);
		delay(5000);
		At24c02Write(3,trigTemp[2]);
		delay(5000);
		At24c02Write(4,trigTemp[3]);
		delay(5000);
		At24c02Write(10,'y');
		delay(5000);
	}else{
		delay(5000);
		trigTemp[0]=At24c02Read(1);
		delay(5000);
		trigTemp[1]=At24c02Read(2);
		delay(5000);
		trigTemp[2]=At24c02Read(3);
		delay(5000);
		trigTemp[3]=At24c02Read(4);
		delay(5000);
	}
}

void SetTrigTemp(){
	if(k7==0){
		delay(1000);
		if(k7==0){
			isSetMode=1;
			trigTempPtr = trigTemp;
			ptrIndex=0;
			Di();
			keyState=1;
			intoSetMode();
		}
		while(!k7);
	}
	if(kReset==0){
		delay(1000);
		if(kReset==0){
			Di();
			trigTemp[0]='3';
			trigTemp[1]='0';
			trigTemp[2]='0';
			trigTemp[3]='0';
			At24c02Write(1,trigTemp[0]);
			delay(5000);
			At24c02Write(2,trigTemp[1]);
			delay(5000);
			At24c02Write(3,trigTemp[2]);
			delay(5000);
			At24c02Write(4,trigTemp[3]);
			delay(5000);
			At24c02Write(10,'y');
			delay(5000);
			keyState=1;
		}
		while(!kReset);
	}
}

void main(void)
{
	u8 count=0;
	motto=0;
	LcdInit();
	SetTrigTempInit();
	UsartInit();	
	while(1){
		TempDisplay();
		SetTrigTemp();												 	
		delay(5000);
		if(count>5)
			TrigMoto();
		WDT_CONTR=0x35;//喂狗
		LcdInit();
		count=count+1;
	}				
}

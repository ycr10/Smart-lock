#include"rfid1.h" //调用读卡器模块库文件
#include "Adafruit_MLX90614.h" //调用温度传感器库文件
#include "LiquidCrystal_I2C.h" //调用LCD库文件
#include <Wire.h> //调用库文件
#define uchar unsigned char
#define uint  unsigned int
#define ull unsigned long long
#define Red  45
#define Green 47
#define Yellow 49
#define Buzzer 2
#define Relay 5
#define IRQ 10
#define SCK 8
#define MOSI 9
#define MISO 11
#define SDA 7
#define RST 12 //接口定义 详情见说明文档接线图
RFID1 rfid;
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); //测温模块初始化
LiquidCrystal_I2C lcd(0x27,16,2); //lcd模块初始化
uchar serNum[5];
uchar a;
uint b; 
double tem; //一些用于记录数据的变量 数组
void setup() //初始化函数
{
  Serial.begin(9600); //电脑(测试)串口初始化
  Serial2.begin(9600); //蓝牙模块串口初始化
  pinMode(Buzzer,OUTPUT);
  pinMode(Relay,OUTPUT); 
  pinMode(Red,OUTPUT);
  pinMode(Yellow,OUTPUT);
  pinMode(Green,OUTPUT); //IO引脚声明
  rfid.begin(IRQ,SCK,MOSI,MISO,SDA,RST); //RFID模块引脚声明
  mlx.begin(); //温度计初始化
  rfid.init(); //RFID初始化
  lcd.init(); //lcd模块初始化
  lcd.backlight(); //lcd模块背光
  lcd.print("   Smart-lock   "); //lcd输出
}
void TEM() //测温函数
{
  Serial.println(mlx.readObjectTempC()+5); //调试串口输出
  tem=mlx.readObjectTempC()+5; //常量赋值
  return tem+5; //返回读取的温度值
}
void open() //开门函数
{
  if(tem<=37.3) //如果体温不超过37.3摄氏度，即可开门
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temperature OK");
    lcd.setCursor(0,1);
    lcd.print("Temp is: ");
    lcd.print(tem);
    lcd.print("*C"); //lcd输出
    Serial.println("Temperature OK"); //调试串口输出
    Serial2.println("Temperature OK"); //蓝牙串口输出
    Serial.print("Turn on."); //调试串口输出
    Serial2.print("Turn on."); //蓝牙串口输出
    digitalWrite(Red,LOW);
    digitalWrite(Green,HIGH);
    digitalWrite(Relay,LOW);
    digitalWrite(Buzzer,LOW); //绿灯亮起，并打开电磁锁（继电器）
    delay(100);
    digitalWrite(Buzzer,HIGH);
    delay(100);
    digitalWrite(Buzzer,LOW);
    delay(100);
    digitalWrite(Buzzer,HIGH); //蜂鸣器鸣叫两声，代表锁已开
    digitalWrite(Relay,HIGH); //锁关闭
    digitalWrite(Green,LOW);
    digitalWrite(Red,HIGH); //红灯重新亮起
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("   Smart-lock   "); //lcd输出
    Serial.print("Turn off."); //调试串口输出
    Serial2.print("Turn off."); //蓝牙串口输出
  }
  else //体温超过37.3摄氏度
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temperature HIGH");
    lcd.setCursor(0,1);
    lcd.print("Temp is: ");
    lcd.print(tem);
    lcd.print("*C"); //lcd输出
    Serial.println("Temperature HIGH"); //调试串口输出
    Serial2.println("Temperature HIGH"); //蓝牙串口输出
    digitalWrite(Red,LOW);
    digitalWrite(Yellow,HIGH);
    delay(100);
    digitalWrite(Yellow,LOW);
    delay(100);
    digitalWrite(Yellow,HIGH);
    delay(100);
    digitalWrite(Yellow,LOW);
    digitalWrite(Red,HIGH); //黄灯闪烁两次，红灯重新亮起
    digitalWrite(Buzzer,LOW);
    delay(100);
    digitalWrite(Buzzer,HIGH);
    delay(100);
    digitalWrite(Buzzer,LOW);
    delay(100);
    digitalWrite(Buzzer,HIGH);
    delay(100);
    digitalWrite(Buzzer,LOW);
    delay(100);
    digitalWrite(Buzzer,HIGH); //蜂鸣器鸣叫三声，表示温度过高
    lcd.clear();
    lcd.print("   Smart-lock   "); //lcd输出
  }
}
void loop() //主循环函数
{
  uchar status;
  uchar str[MAX_LEN];
  digitalWrite(Relay,HIGH);
  digitalWrite(Buzzer,HIGH);
  digitalWrite(Red,HIGH);
  digitalWrite(Yellow,LOW);
  digitalWrite(Green,LOW); //初始化
  TEM(); //测温
  status = rfid.request(PICC_REQIDL, str);
  rfid.showCardType(str);
  status=rfid.anticoll(str);
  if(status==MI_OK)
  {
    Serial.print("The card's number is: "); //调试串口输出
    memcpy(serNum,str,SCK);
    rfid.showCardID(serNum);
    Serial.println();
    Serial.println();
  }
  uchar* id=serNum; //读取卡的ID  
  if(Serial2.available()>0)  //如果蓝牙模块串口有读到信息
  {
    a=Serial2.read(); //蓝牙串口读取
    b=1; //一个用于记录是否执行的变量
  }
  if(id[0]==0xCA && id[1]==0xD9 && id[2]==0xF0 && id[3]==0x3F) //如果读到了正确的卡
  {
    open(); //执行开门函数
    for(int i=0;i<=3;i++) id[i]=0; //初始值返回0
    for(int i=0;i<=3;i++) serNum[i]=0; //初始值返回0
  }
  else //如果不是正确的卡
  {
    if(status==MI_OK) //且有读到卡，执行不开门
    {
      lcd.clear();
      lcd.println("Password Error");  //lcd输出
      digitalWrite(Yellow,HIGH);
      digitalWrite(Green,HIGH);
      digitalWrite(Buzzer,LOW); //红黄蓝三灯常亮，蜂鸣器响起
      delay(3000);
      digitalWrite(Buzzer,HIGH);
      digitalWrite(Yellow,LOW);
      digitalWrite(Green,LOW); //灯熄灭，蜂鸣器停止，代表密码错误
      lcd.clear();
      lcd.print("   Smart-lock   ");  //lcd输出
    }
  }
  if(a=='a') //如果蓝牙串口读到密码
  {
    b=0; //记录的变量值归零
    open(); //执行开门函数
  }
  else //若密码错误
  {
    if(b==1) //且有接收到数据，执行不开门
    {
      lcd.clear();
      lcd.println("Password Error");  //lcd输出
      digitalWrite(Yellow,HIGH);
      digitalWrite(Green,HIGH);
      digitalWrite(Buzzer,LOW); //红黄蓝三灯常亮，蜂鸣器响起
      delay(3000);
      digitalWrite(Buzzer,HIGH);
      digitalWrite(Yellow,LOW);
      digitalWrite(Green,LOW); //灯熄灭，蜂鸣器停止，代表密码错误
      lcd.clear(); 
      lcd.println("Password Error");  //lcd输出
    }
  }
  rfid.halt(); //RFID-RC522重置
}

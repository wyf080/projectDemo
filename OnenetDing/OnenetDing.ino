#include "ESP8266.h"
#include "SoftwareSerial.h"

//配置ESP8266WIFI设置
#define SSID "S8"                                          //填写2.4GHz的WIFI名称，不要使用校园网
#define PASSWORD "123456790"                               //填写自己的WIFI密码
#define HOST_NAME "api.heclouds.com"                       //API主机名称，连接到OneNET平台，无需修改
#define DEVICE_ID "642678301"                              //填写自己的OneNet设备ID
#define HOST_PORT (80)                                     //API端口，连接到OneNET平台，无需修改
String APIKey = "ZjanXjXJFhttuTr=N2gbkoCUGtw=";            //与设备绑定的APIKey

#define INTERVAL_SENSOR 5000                               //定义传感器采样及发送时间间隔

//定义ESP8266所连接的软串口
/*********************
 * 该项目需要使用软串口
 * Arduino上的软串口RX定义为D3,
 * 接ESP8266上的TX口,
 * Arduino上的软串口TX定义为D2,
 * 接ESP8266上的RX口.
 * D3和D2可以自定义,
 * 但接ESP8266时必须恰好相反
 *********************/
SoftwareSerial mySerial(3, 2);
ESP8266 wifi(mySerial);




//以下为硬件的代码
const int TrigPin=4;
const int EchoPin=5;
const int ledPin=13;
int ledState=LOW;
float cm;
int distance=40;                           //此处设置判定坐下的范围，即在此距离内，程序才会开始判定使用者已落座
unsigned long ledTime=0;
unsigned long lastSitTime=0;
unsigned long tool;
unsigned long sitTime=3000;                //此处设置的是判定坐下所需的时间
unsigned long lastStandTime=0;
unsigned long standTime=20000;             //此处设置的是误差缓冲时间
unsigned long longSitTimeMin=15000;       //3600000为一个小时，本组认为单次坐下的时间超过此时长时，即可判定久坐，应站起来运动了
unsigned long longSitTimeMax=15000+15500; //3600000后的15500是本组为了记录久坐次数所做出的判定区间，比2*7753小，故可以不遗漏的记录使用者的久坐次数
int longSitNum=0;
bool song=0;

//以下为提示音的设置
#define NOTE_D0 -1
#define NOTE_D1 294
#define NOTE_D2 330
#define NOTE_D3 350
#define NOTE_D4 393
#define NOTE_D5 441
#define NOTE_D6 495
#define NOTE_D7 556

#define NOTE_DL1 147
#define NOTE_DL2 165
#define NOTE_DL3 175
#define NOTE_DL4 196
#define NOTE_DL5 221
#define NOTE_DL6 248
#define NOTE_DL7 278

#define NOTE_DH1 589
#define NOTE_DH2 661
#define NOTE_DH3 700
#define NOTE_DH4 786
#define NOTE_DH5 882
#define NOTE_DH6 990
#define NOTE_DH7 112

#define WHOLE 1
#define HALF 0.5
#define QUARTER 0.25
#define EIGHTH 0.25
#define SIXTEENTH 0.625

//整首曲子的音符部分
int tune[] =
{
  NOTE_DH1, NOTE_D6, NOTE_D5, NOTE_D6, NOTE_D0,
  NOTE_DH1, NOTE_D6, NOTE_D5, NOTE_DH1, NOTE_D6, NOTE_D0, NOTE_D6,
  NOTE_D6, NOTE_D6, NOTE_D5, NOTE_D6, NOTE_D0, NOTE_D6,
  NOTE_DH1, NOTE_D6, NOTE_D5, NOTE_DH1, NOTE_D6, NOTE_D0,
};

//曲子的节拍，即音符持续时间
float duration[] =
{
  1, 1, 0.5, 0.5, 1,
  0.5, 0.5, 0.5, 0.5, 1, 0.5, 0.5,
  0.5, 1, 0.5, 1, 0.5, 0.5,
  0.5, 0.5, 0.5, 0.5, 1, 1,
};

int length;//定义一个变量用来表示共有多少个音符
int tonePin = 8; //蜂鸣器的pin
//到这里



void setup()
{
  mySerial.begin(115200);                         //初始化软串口
  Serial.begin(9600);                             //初始化串口
  Serial.print("setup begin\r\n");



  //以下为初始化测距模块接口的代码
  pinMode(TrigPin,OUTPUT);
  pinMode(EchoPin,INPUT);


  //以下为初始化蜂鸣器的设置
  pinMode(tonePin, OUTPUT); //设置蜂鸣器的pin为输出模式
  length = sizeof(tune) / sizeof(tune[0]); //这里用了一个sizeof函数，查出数组里有多少个音符

  

  //以下为ESP8266初始化的代码
  Serial.print("FW Version: ");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStation()) {
    Serial.print("to station ok\r\n");
  } else {
    Serial.print("to station err\r\n");
  }

  //ESP8266接入WIFI
  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  Serial.println("");

  mySerial.println("AT+UART_CUR=9600,8,1,0,0");
  mySerial.begin(9600);
  Serial.println("setup end\r\n");
}

unsigned long net_time1 = millis();               //数据上传服务器时间
void loop(){

  
  
  
  //贴入硬件代码
  digitalWrite(TrigPin,LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPin,LOW);
  cm = pulseIn(EchoPin, HIGH) / 58.0;
  Serial.println(cm);
  if(cm>20){                                   //统计坐下的时间
      lastSitTime=millis();
  }
  //Serial.println(millis()-lastSitTime);
  if(millis()-lastSitTime>sitTime){
    ledState=HIGH;  
  }
  if(cm<distance){                            //为防止测距模块过于灵敏，将误差当作使用者离开座位运动，从而写入以下代码
    lastStandTime=millis();                   //统计使用者离开测距区域的时间，若此时间大于设定时间，则可以认为使用者离开座位运动了
  }
  //Serial.println(millis()-lastStandTime);
  if(millis()-lastStandTime>standTime){
    ledState=LOW;  
  }
  digitalWrite(ledPin,ledState);
  if(ledState!=HIGH){                         //这里可以直接通过统计灯的亮起时间来确定使用者落座的时常，在排除了误差带来的测量数值的不准确后
    ledTime=millis();                         //这里统计出的结构是相当准确的
  }                                           //精确数字为millis()-ledTime+sitTime,但由于3秒对于使用者的久坐时间可忽略不计，在写代码时便不将其考虑在内
  Serial.println(millis()-ledTime);
  delay(2000);                                //在此处delay了2000毫秒，故每次loop循环时长约为2000加上传数据的延迟，共7703毫秒，在上面定义了判定次数的区间时用到了此数字
                                              //要记录使用者的久坐时间，要定一个合理的时间范围，在此范围内才记为一次久坐，在此前提下才进行提醒
  if((millis()-ledTime)>longSitTimeMin&&(millis()-ledTime)<longSitTimeMax){
      longSitNum+=1;
      song=1;
  }  
  if((millis()-ledTime)<=longSitTimeMin){
      song=0;  
  }
  if(song==1){
    for (int x = 0; x < length; x++){
      tone(tonePin, tune[x]); //依次播放tune数组元素，即每个音符
      delay(400 * duration[x]); //每个音符持续的时间，即节拍duration，400是调整时间的越大，曲子速度越慢，越小曲子速度越快
      noTone(tonePin);//停止当前音符，进入下一音符
    }  
  }
  //到这里
  

  if (net_time1 > millis())
    net_time1 = millis();

  if (millis() - net_time1 > INTERVAL_SENSOR) //发送数据时间间隔
  {
    
    }

    //float sensor_time = (float)((millis()-ledTime)/60000);  //此处为初稿时，上传久坐时间的代码
    float sensor_time=(float)(longSitNum);;                    //此处为定稿时，上传久坐次数的代码
    float sensor_distance = (float)cm;
    Serial.print("time(ms): ");
    Serial.println(sensor_time, 2);

    Serial.print("distance: ");
    Serial.println(sensor_distance, 2);
    Serial.println("");

    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {    //建立TCP连接，如果失败，不能发送该数据
      Serial.print("create tcp ok\r\n");
      char buf[10];
      //拼接发送data字段字符串
      String jsonToSend = "{\"time\":";
      dtostrf(sensor_time, 1, 2, buf);
      jsonToSend += "\"" + String(buf) + "\"";
      jsonToSend += ",\"distance\":";
      dtostrf(sensor_distance, 1, 2, buf);
      jsonToSend += "\"" + String(buf) + "\"";
      jsonToSend += "}";

      //拼接POST请求字符串
      String postString = "POST /devices/";
      postString += DEVICE_ID;
      postString += "/datapoints?type=3 HTTP/1.1";
      postString += "\r\n";
      postString += "api-key:";
      postString += APIKey;
      postString += "\r\n";
      postString += "Host:api.heclouds.com\r\n";
      postString += "Connection:close\r\n";
      postString += "Content-Length:";
      postString += jsonToSend.length();
      postString += "\r\n";
      postString += "\r\n";
      postString += jsonToSend;
      postString += "\r\n";
      postString += "\r\n";
      postString += "\r\n";

      const char *postArray = postString.c_str();               //将str转化为char数组

      Serial.println(postArray);
      wifi.send((const uint8_t *)postArray, strlen(postArray)); //send发送命令，参数必须是这两种格式，尤其是(const uint8_t*)
      Serial.println("send success");
      if (wifi.releaseTCP()) {                                  //释放TCP连接
        Serial.print("release tcp ok\r\n");
      } else {
        Serial.print("release tcp err\r\n");
      }
      postArray = NULL;                                         //清空数组，等待下次传输数据
    } else {
      Serial.print("create tcp err\r\n");
    }

    Serial.println("");

    net_time1 = millis();
  }

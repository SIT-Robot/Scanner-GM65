/// 扫描13位条形码
/// 只会记录两次的扫描结果
/// 扫描结果是FIFO -- 最后一次独特的扫描结果会覆盖掉最先得到的结果

#include <ESP8266WiFi.h>      // 本程序使用 ESP8266WiFi库
#include <ESP8266WiFiMulti.h> //  ESP8266WiFiMulti库
#include <ESP8266WebServer.h> //  ESP8266WebServer库

ESP8266WiFiMulti wifiMulti; // 建立ESP8266WiFiMulti对象,对象名称是'wifiMulti'

ESP8266WebServer esp8266_server(80); // 建立网络服务器对象，该对象用于响应HTTP请求。监听端口（80）

const int MAX_BARCODE_LENGTH = 14;
char serialData1[MAX_BARCODE_LENGTH];
char serialData2[MAX_BARCODE_LENGTH];
char serialDataTemp[MAX_BARCODE_LENGTH];
static int serialDataIndex = 0;
void goNextSerialData()
{
  serialDataIndex = !serialDataIndex;
}
char* getCurrentSerialDataSaved(){
  return serialDataIndex == 0? serialData2 : serialData1;
} 
char* getCurrentSerialData4Write()
{
  return serialDataIndex == 0? serialData1 : serialData2;
}

/// check the equality of two bar codes with first-[MAX_BARCODE_LENGTH characters identity.
int isBarCodeEqual(const char a[],const char b[])
{
  int index = 0;
  for (int i = 0; i < MAX_BARCODE_LENGTH; i++)
  {
    if (a[i] != b[i])
    {
      return 0;
    }
    if (a[i] == '\0')
    {
      return b[i] == '\0';
    }
    if (b[i] == '\0')
    {
      return a[i] == '\0';
    }
  }
  return 1;
}
void copyCharArray(char from[], char to[], int length)
{
  for (int i = 0; i < length; i++)
  {
    to[i] = from[i];
  }
}
void keepString(char *str, int length)
{
  str[length - 1] = '\0';
}
void setup()
{
  Serial.begin(9600);
  Serial.println("Please input serial data.");
  wifiMulti.addAP("Robot", "fightforfuture"); // 将需要连接的WiFi ID和密码输入这里
                                              // ESP8266-NodeMCU在启动后会扫描当前网络
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED)
  {              // 此处的wifiMulti.run()是重点。通过wifiMulti.run()，NodeMCU将会在当前
    delay(1000); // 环境中搜索addAP函数所存储的WiFi。如果搜到多个存储的WiFi那么NodeMCU
    Serial.print(i++);
    Serial.print(' '); // 将会连接信号最强的那一个WiFi信号。
  }                    // 一旦连接WiFI成功，wifiMulti.run()将会返回“WL_CONNECTED”。这也是
                       // 此处while循环判断是否跳出循环的条件。
  // WiFi连接成功后将通过串口监视器输出连接成功信息
  Serial.println('\n');           // WiFi连接成功后
  Serial.print("Connected to ");  // NodeMCU将通过串口监视器输出。
  Serial.println(WiFi.SSID());    // 连接的WiFI名称
  Serial.print("IP address:\t");  // 以及
  Serial.println(WiFi.localIP()); // NodeMCU的IP地址

  esp8266_server.begin();
  esp8266_server.on("/", handleRoot);
  esp8266_server.onNotFound(handleNotFound);

  Serial.println("HTTP esp8266_server started"); //  告知用户ESP8266网络服务功能已经启动
}

int i = 0;
int isReadingEnd()
{
  return i >= MAX_BARCODE_LENGTH;
}
void resetReadingState()
{
  i = 0;
}
void loop()
{
  if (Serial.available() > 0)
  {                                  //检查串口缓存中是否有数据等待读取
    char serialData = Serial.read(); //读取串口缓存中等待的字符
    serialDataTemp[i++] = serialData;
    Serial.print(serialData); //显示刚刚读取到的字符
    keepString(serialDataTemp,MAX_BARCODE_LENGTH);
  }
  if (isReadingEnd())
  {
    resetReadingState();
    if (!isBarCodeEqual(serialDataTemp, getCurrentSerialDataSaved()))
    {
      copyCharArray(/*from*/ serialDataTemp, /*to*/ getCurrentSerialData4Write(), MAX_BARCODE_LENGTH);
      goNextSerialData();
    }
  }
  esp8266_server.handleClient(); // 处理http服务器访问
}

void handleRoot()
{ //处理网站目录“/”的访问请求
  esp8266_server.send(200, "text/html", sendHTML(serialData1, serialData2));
}
const String PAGE_HEAD = "<!DOCTYPE html> <html><head><meta http-equiv='refresh' content='1'/><title>ESP8266 Butoon State</title><style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}</style></head><body>";
const String PAGE_TAIL = "</body></html>";

void append(String *html, const String addition)
{
  *html = *html + addition;
}

void appendBarCodeLine(String *html, const char barcode[])
{
  if (barcode[0] != '\0')
  {
    append(html, "<h1>");
    String data = barcode;
    *html += data;
    append(html, "</h1>");
  }
}

/*
建立用于发送给客户端浏览器的HTML代码。此代码将会每隔1秒刷新页面。
通过页面刷新，引脚的最新状态也会显示于页面中
*/
String sendHTML(const char serialData1[], const char serialData2[])
{

  String d = PAGE_HEAD;
  appendBarCodeLine(&d, serialData1);
  appendBarCodeLine(&d, serialData2);
  append(&d, PAGE_TAIL);
  return d;
}

// 设置处理404情况的函数'handleNotFound'
void handleNotFound()
{                                                           // 当浏览器请求的网络资源无法在服务器找到时，
  esp8266_server.send(404, "text/plain", "404: Not found"); // NodeMCU将调用此函数。
}

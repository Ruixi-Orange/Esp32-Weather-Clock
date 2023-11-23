//
//  头文件
//
#include <string.h>
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <FreeRTOS.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>
#include "astronaut.h"
#include "noto_sans_cjk20.h"

//
//  宏定义
//
#define LED_NUM 1
#define LED_Ctrl_PIN 33

//
//  全局变量定义
//
TaskHandle_t LEDCtrl_Handle;
TaskHandle_t DisplayCtrl_Handle;
TaskHandle_t NetCtrl_Handle;
String url;
String weather;
String temperature;
String timezone;
String unixtime;  //unix时间戳
uint8_t update_flag = 0;
uint8_t frameIndex = 0; // 太空人动画帧索引

//
//  Init
//
Adafruit_NeoPixel pixels(LED_NUM, LED_Ctrl_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();
ESP32Time rtc(3600 * 8); // 时区偏移量 GMT+8

//
//  函数
//
void LEDinit();
void Displayinit();
void Netinit();
void Get_weather_data();
void Sync_time();
void Show_astronaut();
void Show_weather();
void Show_time();
void Show_timezone();

void init()
{
  LEDinit();
  Displayinit();
  Netinit();
}

void LEDinit()
{
  pinMode(34, OUTPUT); // NeoPixel电源脚
  digitalWrite(34, HIGH);

  pixels.begin(); // NeoPixel灯珠初始化
  pixels.clear(); // 关闭灯珠
}

void Netinit()
{
  WiFi.mode(WIFI_AP_STA);               // Esp32 WiFi模式设置
  WiFi.begin("RuixisNet", "PASSWORD"); // Esp32 联网ssid和passwd设置
  WiFi.softAP("qwert", "password");     // Esp32 Access Point ssid和passwd设置
}

void Displayinit()
{
  pinMode(21, OUTPUT); // TFT电源脚
  digitalWrite(21, 1);
  tft.init();         // st7789屏幕初始化
  tft.setRotation(1); // 屏幕旋转90度
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
}

void Get_weather_data()
{
  HTTPClient http;
  url = "https://api.seniverse.com/v3/weather/now.json?key=SRv-auGmyT6lOOE-B&location=hefei&language=zh-Hans&unit=c"; // 设置目标网站的URL
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    // 获取并解析响应的Json数据
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);

    JsonObject results_0 = doc["results"][0];
    JsonObject results_0_now = results_0["now"];
    weather = results_0_now["text"].as<String>();
    temperature = results_0_now["temperature"].as<String>();
    DeserializationError error = deserializeJson(doc, response);
  }
  else if (httpCode < 0)
  {
    weather = "error";
    temperature = "error";
  }

  http.end(); // 关闭HTTP连接
}

void Sync_time()
{
  HTTPClient http;
  url = "http://worldtimeapi.org/api/timezone/Asia/Shanghai"; // 设置目标网站的URL
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    // 获取并解析响应的Json数据
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);

    timezone = doc["timezone"].as<String>();
    unixtime = doc["unixtime"].as<String>();

    rtc.setTime(unixtime.toInt());
  }
  else if (httpCode < 0)
  {
    timezone = "error";
    unixtime = "error";
  }
  http.end();
}

void Show_astronaut()
{
  tft.fillRoundRect(180, 40, 40, 40, 0, TFT_BLACK);
  tft.drawXBitmap(180, 40, frameData[frameIndex], 40, 40, TFT_WHITE, TFT_BLACK);
  frameIndex = (frameIndex + 1) % FRAME_NUM;
}

void Show_weather()
{
  tft.fillRoundRect(15, 15, 160, 45, 0, TFT_BLACK);
  tft.setCursor(15, 15);
  tft.print("天气:");
  tft.print(weather);
  tft.setCursor(15, 40);
  tft.print("温度:");
  tft.print(temperature);
  tft.print("℃");
}

void Show_time()
{
  tft.fillRoundRect(15, 75, 160, 45, 0, TFT_BLACK);
  tft.setCursor(15, 75);
  tft.print("时间:");
  tft.print(rtc.getTime());
}

void Show_timezone()
{
  tft.fillRoundRect(15, 135, 160, 45, 0, TFT_BLACK);
  tft.setCursor(15, 100);
  tft.print("时区:");
  tft.print(timezone);
}

void LEDCtrl(void *Parameters)
{
  while (1)
  {
    // NeoPixel 流水灯实现
    for (int i = 0; i < 256; i++)
    {
      pixels.setPixelColor(0, pixels.Color(255 - i, i, 0));
      pixels.show();
      vTaskDelay(10);
    }
    for (int i = 0; i < 256; i++)
    {
      pixels.setPixelColor(0, pixels.Color(0, 255 - i, i));
      pixels.show();
      vTaskDelay(10);
    }
    for (int i = 0; i < 256; i++)
    {
      pixels.setPixelColor(0, pixels.Color(i, 0, 255 - i));
      pixels.show();
      vTaskDelay(10);
    }
  }
  vTaskDelete(LEDCtrl_Handle);
}

void DisplayCtrl(void *Parameter)
{
  tft.fillScreen(TFT_BLACK);
  while (1)
  {
    for (int i = 0; i < 5; i++)
    {
      Show_astronaut();
      vTaskDelay(200);
    }
    tft.loadFont(noto_sans_cjk20);
    Show_weather();
    Show_time();
    Show_timezone();
    tft.unloadFont();
  }

  vTaskDelete(DisplayCtrl_Handle);
};

void NetCtrl(void *Parameter)
{
  Sync_time();
  while (1)
  {
    Get_weather_data();
    vTaskDelay(300000);  //每隔5分钟更新一次天气数据
  }

  vTaskDelete(NetCtrl_Handle);
}

void setup()
{
  init();

  pixels.setPixelColor(0, pixels.Color(255, 0, 0)); //LED显示红色,标志WiFI尚未连接
  pixels.show();

  tft.loadFont(noto_sans_cjk20);
  tft.setCursor(15, 10);
  tft.print("WiFi connecting");

  uint8_t dot_num = 0;
  while (WiFi.status() != WL_CONNECTED) // 未连接时阻塞程序，直到连接成功
  {
    if (dot_num >= 5)
    {
      tft.fillScreen(TFT_BLACK); // 清空屏幕
      tft.setCursor(15, 10);
      tft.print("WiFi connecting");
    }
    delay(500);
    tft.print(".");
    dot_num++;
  }

  pixels.setPixelColor(0, pixels.Color(0, 255, 0)); //LED显示绿色,标志WiFI已连接
  pixels.show();

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(15, 10);
  tft.print("WiFi connected");
  delay(1000);

  tft.setCursor(15, 35);
  tft.print("IP address: ");
  tft.setCursor(15, 60);
  tft.print(WiFi.localIP());
  tft.setCursor(15, 85);
  tft.print("你好,Ruixi");
  tft.setCursor(15, 110);
  tft.print("太空人天气时钟");
  tft.unloadFont(); // 释放字库,节省RAM
  delay(2000);

  xTaskCreate(
      LEDCtrl,        // 任务函数
      "LEDCtrl",      // 任务名字
      1536,           // 任务堆栈大小
      NULL,           // 传递给任务函数的参数
      3,              // 任务优先级
      &LEDCtrl_Handle // 任务句柄
  );
  xTaskCreate(
      NetCtrl,
      "NetCtrl",
      8192,
      NULL,
      4,
      &NetCtrl_Handle);
  //vTaskDelay(500);
  xTaskCreate(
      DisplayCtrl,
      "DisplayCtrl",
      2048,
      NULL,
      2,
      &DisplayCtrl_Handle
  );
}

void loop()
{
}
#include <TimeLib.h>
#include "WiFi.h"
// #include "EEPROM.h"
#include "NTPClient.h"
#include <Wire.h>
#include "BL8025_RTC.h"
#include <U8g2lib.h>
#include <SPI.h>
#include "alarm.h"

#define SECOND_ADJUSTMENT 16
#define EEPROM_SIZE 64

// SPI LED GPIO
#define LCD_SDA 3
#define LCD_SCL 2
#define LCD_DC 6
#define LCD_CS 7
#define LCD_RST 10
#define LCD_BL 11

#define BTN_1 0
#define BTN_2 12
#define BTN_3 13

#define ALARM_GPIO 8

U8G2_ST7565_LX12864_F_4W_SW_SPI u8g2(U8G2_R0, LCD_SCL, LCD_SDA, LCD_CS, LCD_DC, LCD_RST);

AlarmSetting alarmSetting;

const char* ssid     = "xxxxxx"; // Change this to your WiFi SSID
const char* password = "xxxxxx"; // Change this to your WiFi password

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 60*60*8, 30*60*1000);

BL8025_RTC rtc;
tmElements_t tm;

int alert = 0;
int alertCnt = 0;
int alertRepeat = 5;

int blLignt = 0; // 背景光，默认关

void drawStatus(const char* str) {
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.firstPage();
  do {
    u8g2.drawUTF8(20, 20, str);
  } while (u8g2.nextPage() );
}

/**
 * 配置网络
 */
void configWifi() {
  //Init WiFi as Station, start SmartConfig
  WiFi.begin(ssid, password);
  drawStatus("开始联网...");
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
  }
  drawStatus("联网成功");
  delay(1000);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // 闹钟输出，默认拉低
  pinMode(ALARM_GPIO, OUTPUT);
  pinMode(LCD_BL, OUTPUT);
  pinMode(BTN_1, INPUT);

  u8g2.begin();
  u8g2.setContrast(200);

  digitalWrite(ALARM_GPIO, 0);

  // 默认背景光设置
  digitalWrite(LCD_BL, blLignt);

  Serial.begin(115200);
  Wire.begin(4, 5);

  // 启动时配置网络
  configWifi();
  delay(1000);

  drawStatus("开始校准时间...");
  initDateTime();
  drawStatus("时间校准完成");

  // 断开网络
  WiFi.disconnect(true);

  // 初始化持久存储
  initStore();

  // 初始化蓝牙通信
  startBLEService(&alarmSetting);

  alarmSetting.on = 1;
  alarmSetting.hour = 6;
  alarmSetting.minute = 3;
  writeAlarmSetting(&alarmSetting);
  readAlarmSetting(&alarmSetting);
  printAlarmSetting(&alarmSetting);
}

// 默认响{alertRepeat}次后自动停止
void checkAlarm() {
  if (alert) {
    if (alertCnt < alertRepeat) {
      digitalWrite(ALARM_GPIO, 1);
      delay(500);
      digitalWrite(ALARM_GPIO, 0);
      alertCnt++;
    } else {
      alertCnt = 0;
      alert = 0;
    }
  }
}

void loop() {
  int btn1Pressed = !digitalRead(BTN_1);
  int btnBL = digitalRead(LCD_BL);

  if (alert && !blLignt) {
    digitalWrite(LCD_BL, 1);
  } else {
    if (btn1Pressed) {
      blLignt = !blLignt;
      digitalWrite(LCD_BL, blLignt);
    }
  }

  char btn_status[14];
  tmElements_t tm2;
  tm2 = rtc.read(false);
  char s[3];
  // sprintf(s, "%d:%d", tm2.Hour,tm2.Minute);
  // Serial.println(s);

  // 检查闹钟，只有不在响的时候检查
  if (!alert && alarmSetting.on == 1) {
    // 0 秒的时候开始闹钟
    if (alarmSetting.hour == tm2.Hour && alarmSetting.minute == tm2.Minute && tm2.Second == 0) {
      alertCnt = 0;
      alert = 1;
    }
  }

  char hour_str[3];
  strcpy(hour_str, u8x8_u8toa(tm2.Hour, 2));		/* convert m to a string with two digits */
  char minute_str[3];
  strcpy(minute_str, u8x8_u8toa(tm2.Minute, 2));		/* convert m to a string with two digits */

  u8g2.firstPage();
  do {
    // 显示闹钟
    if (alarmSetting.on) {
      char alarm_str[6];
      // u8g2.drawXBMP(10, 8, 8, 8, bitmap_alarm);
      u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
      u8g2.drawGlyph(0, 8, 65);
      sprintf(alarm_str, "%d:%d", alarmSetting.hour, alarmSetting.minute);
      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.drawStr(10, 7, alarm_str);
    }

    u8g2.setFont(u8g2_font_logisoso34_tf); // u8g2_font_logisoso46_tf
    u8g2.drawStr(10, 50, hour_str);
    u8g2.drawStr(53, 50, ":");
    u8g2.drawStr(70, 50, minute_str);
  } while (u8g2.nextPage() );

  checkAlarm();
  delay(1000);
}

bool initDateTime(){
  delay(1000);
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());

  const char* monthNames[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  char mon[12];
  int Year, Month, Day, Hour, Minute, Second;

  Hour = timeClient.getHours();
  Minute = timeClient.getMinutes();
  Second = timeClient.getSeconds();

  if(sscanf(__DATE__,"%s %d %d",mon, &Day, &Year) != 3){
    return false;
  }
  if(sscanf(__TIME__,"%d:%d:%d",&Hour, &Minute, &Second) != 3){
    return false;
  }

  uint8_t idx;
  Month = 0;
  for(idx = 0; idx < 12; idx++){
    if(strcmp(mon, monthNames[idx]) == 0){
      Month = idx + 1;
      break;
    }
  }
  if(Month == 0){
      return false;
  }
  Second += SECOND_ADJUSTMENT;
  tm.Year = CalendarYrToTm(Year);
  tm.Month = Month;
  tm.Day = Day;
  tm.Hour = Hour;
  tm.Minute = Minute;
  tm.Second = Second;
  rtc.write(tm);

  drawStatus("时间校准完成");
  delay(1000);
  return true;
}

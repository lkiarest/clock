#include "EEPROM.h"
#include "alarm.h"

#define EEPROM_SIZE 64

void initStore() {
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
}

void readAlarmSetting(AlarmSetting *alarmSetting) {
  alarmSetting->on = EEPROM.read(0);
  alarmSetting->hour = EEPROM.read(1);
  alarmSetting->minute = EEPROM.read(2);
}

void writeAlarmSetting(AlarmSetting *alarmSetting) {
  EEPROM.write(0, alarmSetting->on);
  EEPROM.write(1, alarmSetting->hour);
  EEPROM.write(2, alarmSetting->minute);
  EEPROM.commit();
}

void printAlarmSetting(AlarmSetting *alarmSetting) {
  char s[20];
  sprintf(s, "alarm:%d-%d:%d", alarmSetting->on, alarmSetting->hour, alarmSetting->minute);
  Serial.println(s);
}

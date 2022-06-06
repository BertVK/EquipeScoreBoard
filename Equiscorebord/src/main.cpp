#include <Arduino.h>

// epd
#include "epd_driver.h"

// wifi
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

// data
#include <Meeting.h>

// battery
#include <driver/adc.h>
#include "esp_adc_cal.h"

// font
#include "opensans8b.h"
#include "opensans9b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"

#include "configurations.h"
#include "epd_drawing.h"

int wifi_signal = 0;

// colors
#define White 0xFF
#define LightGrey 0xBB
#define Grey 0x88
#define DarkGrey 0x44
#define Black 0x00

// IO pins
#define BATT_PIN 36

// required for NTP time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp;

// deep sleep configurations
long SleepDuration = 1; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int WakeupHour = 6;     // Wakeup after 06:00 to save battery power
int SleepHour = 23;     // Sleep  after 23:00 to save battery power

long StartTime = 0;
long SleepTimer = 0;
int CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
String CurrentYear = "";
String CurrentMonth = "";
String CurrentDay = "";

Meeting SelectedMeeting;
int selectedMeetingId = 0

int vref = 1100; // default battery vref

uint8_t StartWiFi()
{
  Serial.println("\r\nConnecting to: " + String(ssid));
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection)
  {
    connectionStatus = WiFi.status();
    if (millis() > start + 15000)
    { // Wait 15-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED)
    {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED)
  {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    timeClient.begin();
    timeClient.setTimeOffset(gmtOffset_sec);
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else
  {
    wifi_signal = 0;
    Serial.println("WiFi connection *** FAILED ***");
  }
  return connectionStatus;
}

void StopWiFi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

void SetupTime()
{
  Serial.println("Getting time...");

  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

  CurrentHour = timeClient.getHours();
  CurrentMin = timeClient.getMinutes();
  CurrentSec = timeClient.getSeconds();
  CurrentYear = dayStamp.substring(0, 4);
  CurrentMonth = dayStamp.substring(5, 7);
  CurrentDay = dayStamp.substring(8, 10);

  Serial.print("Date stamp : ");
  Serial.println(dayStamp);
  Serial.print("Current Date : ");
  Serial.print(CurrentDay);
  Serial.print("/");
  Serial.print(CurrentMonth);
  Serial.print("/");
  Serial.println(CurrentYear);
  Serial.print("Current time : ");
  Serial.print(CurrentHour);
  Serial.print(":");
  Serial.print(CurrentMin);
  Serial.print(":");
  Serial.println(CurrentSec);
}

void DrawBattery(int x, int y)
{
  uint8_t percentage = 100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
  {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
  float voltage = analogRead(36) / 4096.0 * 6.566 * (vref / 1000.0);
  if (voltage > 1)
  { // Only display if there is a valid reading
    Serial.println("\nVoltage = " + String(voltage));
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20)
      percentage = 100;
    if (voltage <= 3.20)
      percentage = 0; // orig 3.5
    drawRect(x + 55, y - 15, 40, 15, Black);
    fillRect(x + 95, y - 9, 4, 6, Black);
    fillRect(x + 57, y - 13, 36 * percentage / 100.0, 11, Black);
    drawString(x, y, String(percentage) + "%", LEFT);
    drawString(x + 130, y, String(voltage, 2) + "v", CENTER);
  }
}

void DrawRSSI(int x, int y, int rssi)
{
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20)
  {
    if (_rssi <= -20)
      WIFIsignal = 20; //            <-20dbm displays 5-bars
    if (_rssi <= -40)
      WIFIsignal = 16; //  -40dbm to  -21dbm displays 4-bars
    if (_rssi <= -60)
      WIFIsignal = 12; //  -60dbm to  -41dbm displays 3-bars
    if (_rssi <= -80)
      WIFIsignal = 8; //  -80dbm to  -61dbm displays 2-bars
    if (_rssi <= -100)
      WIFIsignal = 4; // -100dbm to  -81dbm displays 1-bar

    if (rssi != 0)
      fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    else // draw empty bars
      drawRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    xpos++;
  }
  if (rssi == 0)
    drawString(x, y, "x", LEFT);
}

String CurrentDateAsString()
{
  // return dayStamp;
  return "2022-05-29";
}

void GetMeetingsData()
{
  Serial.println("Getting meetings data");
  // Get the stream
  HTTPClient http;
  http.setUserAgent(userAgent);
  http.begin("https://online.equipe.com/api/v1/meetings.json");
  http.useHTTP10(true);
  http.GET();
  Stream &input = http.getStream();

  StaticJsonDocument<96> filter;

  JsonObject filter_0 = filter.createNestedObject();
  filter_0["id"] = true;
  filter_0["display_name"] = true;
  filter_0["start_on"] = true;
  filter_0["discipline"] = true;
  filter_0["venue_country"] = true;

  DynamicJsonDocument doc(200000);

  DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));

  if (error)
  {
    Serial.print("deserializeJson() of meetings failed: ");
    Serial.println(error.c_str());
    return;
  }
  long id = 0;
  for (int i = 0; i < doc.size(); i++)
  {
    JsonObject item = doc[i];
    const String country = item["venue_country"];
    const String date = item["start_on"];
    if (country.equals("BEL") && date.equals(CurrentDateAsString()))
    {
      id = item["id"];
      const String name = item["display_name"];
      const String type = item["discipline"];
      Serial.print(date);
      Serial.print(" - ");
      Serial.print(id);
      Serial.print(" - ");
      Serial.print(type);
      Serial.print(" - ");
      Serial.println(name);
    }
  }

  http.end();

  // TODO : get the selected meeting id from the touch screen!!
  SelectedMeeting.id = id;
  Serial.print("Selected meeting id = ");
  Serial.println(SelectedMeeting.id);
}

void getSeries()
{
  // https://online.equipe.com/api/v1/meetings/47228/schedule

  // Get the stream
  HTTPClient http;
  http.setUserAgent(userAgent);
  String url = "https://online.equipe.com/api/v1/meetings/" + String(SelectedMeeting.id) + "/schedule";
  Serial.print("URL : ");
  Serial.println(url);
  http.begin(url);
  http.addHeader("Accept-Language", "nl");
  http.useHTTP10(true);
  http.GET();
  Stream &input = http.getStream();

  // Create the filter
  StaticJsonDocument<256> filter;
  filter["display_name"] = true;
  filter["discipline"] = true;

  JsonObject filter_days_0 = filter["days"].createNestedObject();
  filter_days_0["date"] = true;

  JsonObject filter_days_0_meeting_classes_0 = filter_days_0["meeting_classes"].createNestedObject();
  filter_days_0_meeting_classes_0["id"] = true;
  filter_days_0_meeting_classes_0["name"] = true;
  filter_days_0_meeting_classes_0["display_time"] = true;
  filter_days_0_meeting_classes_0["horse_ponies"] = true;
  // filter_days_0_meeting_classes_0["position"] = true;

  JsonObject filter_days_0_meeting_classes_0_class_sections_0 = filter_days_0_meeting_classes_0["class_sections"].createNestedObject();
  filter_days_0_meeting_classes_0_class_sections_0["id"] = true;
  filter_days_0_meeting_classes_0_class_sections_0["total"] = true;

  DynamicJsonDocument doc(10000);

  DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));

  if (error)
  {
    Serial.print("deserializeJson() of riders failed: ");
    Serial.println(error.c_str());
    return;
  }
  const String name = doc["display_name"];
  const String type = doc["discipline"];
  SelectedMeeting.name = name;
  SelectedMeeting.type = type;
  if (doc["days"].size() > 0)
  {
    JsonObject meetingDay = doc["days"][0];
    const String date = meetingDay["date"];
    SelectedMeeting.date = date;
    for (int i = 0; i < doc["days"][0]["meeting_classes"].size(); i++)
    {
      JsonObject item = meetingDay["meeting_classes"][i];
      if (item["class_sections"].size() > 0)
      {
        String name = item["name"];
        // String horse_ponies = item["horse_ponies"][0];
        String horse_ponies;
        for (int j = 0; j < item["horse_ponies"].size(); j++)
        {
          String temp = item["horse_ponies"][j];
          horse_ponies += temp;
          if (j < item["horse_ponies"].size() - 1)
          {
            horse_ponies += " | ";
          }
        }
        String display_time = item["display_time"];
        int id = item["id"];
        int meetingClassId = item["class_sections"][0]["id"];
        MeetingClass meetingClass;
        meetingClass.name = name;
        meetingClass.id = id;
        meetingClass.classSection0Id = meetingClassId;
        meetingClass.horse_ponies = horse_ponies;
        meetingClass.display_time = display_time;
        SelectedMeeting.classes.push_back(meetingClass);
      }
    }
  }

  Serial.print(SelectedMeeting.id);
  Serial.print(" - ");
  Serial.print(SelectedMeeting.date);
  Serial.print(" - ");
  Serial.print(SelectedMeeting.type);
  Serial.print(" - ");
  Serial.println(SelectedMeeting.name);
  for (int i = 0; i < SelectedMeeting.classes.size(); i++)
  {
    MeetingClass meetingClass = SelectedMeeting.classes[i];
    Serial.print("Class : ");
    Serial.print(meetingClass.classSection0Id);
    Serial.print(" - ");
    Serial.print(meetingClass.display_time);
    Serial.print(" - ");
    Serial.print(meetingClass.horse_ponies);
    Serial.print(" - ");
    Serial.println(meetingClass.name);
  }

  http.end();
}

void GetRiderData()
{
  // https://online.equipe.com/api/v1/class_sections/690661

  // Get the stream
  HTTPClient http;
  http.setUserAgent(userAgent);
  http.begin("https://online.equipe.com/api/v1/class_sections/690661");
  http.useHTTP10(true);
  http.GET();
  Stream &input = http.getStream();

  // Create the filter
  StaticJsonDocument<176> filter;
  filter["total"] = true;
  filter["remains"] = true;

  JsonArray filter_categories = filter.createNestedArray("categories");

  JsonArray filter_officials = filter.createNestedArray("officials");

  JsonObject filter_starts_0 = filter["starts"].createNestedObject();
  filter_starts_0["rank"] = true;
  filter_starts_0["start_no"] = true;
  filter_starts_0["start_at"] = true;
  filter_starts_0["rider_name"] = true;
  filter_starts_0["horse_name"] = true;

  JsonArray filter_starts_0_results = filter_starts_0.createNestedArray("results");

  DynamicJsonDocument doc(6144);

  DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));

  if (error)
  {
    Serial.print("deserializeJson() of riders failed: ");
    Serial.println(error.c_str());
    return;
  }
  int total = doc["total"]; // 24
  Serial.printf("total: %d\n", total);

  setFont(OpenSans18B);
  // drawString(10, 60, "WEDSTRIJDRESULTATEN", LEFT);
  String name = SelectedMeeting.name;
  name.replace("BEL - ", "");
  name.replace("HROV - ", "");
  drawString(10, 60, name, LEFT);
  setFont(OpenSans12B);
  drawString(950, 60, SelectedMeeting.date, RIGHT);
  drawLine(0, 70, 960, 70, Black);
  int x = 10;
  int y = 120;
  setFont(OpenSans12B);
  MeetingClass thisClass;
  for (int i = 0; i < SelectedMeeting.classes.size(); i++)
  {
    if (SelectedMeeting.classes[i].id == 690661)
    {
      thisClass = SelectedMeeting.classes[i];
    }
  }
  for (JsonObject start : doc["starts"].as<JsonArray>())
  {

    const String start_horse_name = start["horse_name"]; // "Corianne G Z", "jiliandra", "Pantanera", "Jolie ...
    String lowerHorse = start_horse_name;
    lowerHorse.toLowerCase();
    if (
        lowerHorse.indexOf("hacienda") > 0 || lowerHorse.equals("tizziana"))
    {
      int start_rank = start["rank"];                  // 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, ...
      const String start_start_no = start["start_no"]; // "15", "19", "14", "2", "24", "21", "4", "11", "9", ...
      const String start_start_at = start["start_at"];
      const String start_rider_name = start["rider_name"]; // "Zoë Vande Walle", "Dirk Saverwyns", "Kyrsten ...
      const int start_horse_combination_no = start["horse_combination_no"];
      String start_time = start_start_at.substring(11, 16);
      if (thisClass.id > 0)
      {
        Rider rider;
        rider.rider_name = start_rider_name;
        rider.horse_name = start_horse_name;
        rider.horse_combination_no = start_horse_combination_no;
        rider.rank = start_rank;
        rider.start_at = start_start_at;
        rider.start_no = start_start_no;
        thisClass.riders.push_back(rider);
      }

      Serial.print(start_rank);
      Serial.print(" - ");
      Serial.print(start_start_no);
      Serial.print(" - ");
      Serial.print(start_time);
      Serial.print(" - ");
      Serial.print(start_rider_name);
      Serial.print(" - ");
      Serial.println(start_horse_name);
      drawString(x + 20, y, String(start_start_no), RIGHT);
      if (String(start_rank).equals("0"))
      {
        drawString(x + 100, y, String(start_time), RIGHT);
      }
      else
      {
        drawString(x + 100, y, String(start_rank), RIGHT);
      }
      drawString(x + 120, y, String(start_rider_name), LEFT);
      drawString(x + 500, y, String(start_horse_name), LEFT);

      y = y + 40;
    }
  }
  http.end();
}

void DisplayStatusSection()
{
  setFont(OpenSans8B);
  DrawBattery(5, 18);
  DrawRSSI(900, 18, wifi_signal);
}

void getData()
{
  GetMeetingsData();
  getSeries();
  GetRiderData();
}

void DrawWifiErrorScreen()
{
  Serial.println("Drawing error screen");
  epd_clear();
  DisplayStatusSection();
  epd_update();
}

void DrawScreen()
{
  Serial.println("Drawing screen");
  epd_clear();
  DisplayStatusSection();
  getData();
  epd_update();
}

void InitialiseSystem()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println(String(__FILE__) + "\nStarting...");
  epd_init();
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer)
    Serial.println("Memory alloc failed!");
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

  setFont(OpenSans9B);
}

void setup()
{
  InitialiseSystem();
  if (StartWiFi() == WL_CONNECTED)
  {
    SetupTime();

    // bool WakeUp = false;
    // if (WakeupHour > SleepHour)
    //   WakeUp = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour);
    // else
    //   WakeUp = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);
    // if (WakeUp)
    // {
    DrawScreen();
    // }
  }
  else
  {
    DrawWifiErrorScreen();
  }
}

void loop()
{
  // Serial.println("TEST");
  // delay(5000);
}
/* FLASH SETTINGS
Board: LOLIN D32
Flash Frequency: 80MHz
Partition Scheme: Minimal SPIFFS
https://www.online-utility.org/image/convert/to/XBM
*/

#include "configs.h"

#ifndef HAS_SCREEN
  #define MenuFunctions_h
  #define Display_h
#endif

#include <WiFi.h>
#include <Wire.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <Arduino.h>

#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif

#include "Assets.h"
#include "WiFiScan.h"
#ifdef HAS_SD
  #include "SDInterface.h"
#endif
#include "Buffer.h"



#include "settings.h"
#include "lang_var.h"

#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif

#ifdef HAS_SCREEN
  #include "Display.h"
  #include "MenuFunctions.h"
#endif

#ifdef HAS_BUTTONS
  #include "Switches.h"
  
  #if (U_BTN >= 0)
    Switches u_btn = Switches(U_BTN, 1000, U_PULL);
  #endif
  #if (D_BTN >= 0)
    Switches d_btn = Switches(D_BTN, 1000, D_PULL);
  #endif
  #if (L_BTN >= 0)
    Switches l_btn = Switches(L_BTN, 1000, L_PULL);
  #endif
  #if (R_BTN >= 0)
    Switches r_btn = Switches(R_BTN, 1000, R_PULL);
  #endif
  #if (C_BTN >= 0)
    Switches c_btn = Switches(C_BTN, 1000, C_PULL);
  #endif

#endif

WiFiScan wifi_scan_obj;
Buffer buffer_obj;
Settings settings_obj;
CommandLine cli_obj;

#ifdef HAS_GPS
  GpsInterface gps_obj;
#endif

#ifdef HAS_BATTERY
  BatteryInterface battery_obj;
#endif

#ifdef HAS_SCREEN
  Display display_obj;
  MenuFunctions menu_function_obj;
#endif

#ifdef HAS_SD
  SDInterface sd_obj;
#endif


#ifdef HAS_NEOPIXEL_LED
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);
#endif

uint32_t currentTime  = 0;



void setup()
{

  
  #ifdef HAS_SCREEN
    pinMode(TFT_BL, OUTPUT);
  #endif
  
  backlightOff();
#if BATTERY_ANALOG_ON == 1
  pinMode(BATTERY_PIN, OUTPUT);
  pinMode(CHARGING_PIN, INPUT);
#endif
  
  // Preset SPI CS pins to avoid bus conflicts
  #ifdef HAS_SCREEN
    digitalWrite(TFT_CS, HIGH);
  #endif
  
  #ifdef HAS_SD
    pinMode(SD_CS, OUTPUT);

    delay(10);
  
    digitalWrite(SD_CS, HIGH);

    delay(10);
  #endif

  Serial.begin(115200);

  while(!Serial)
    delay(10);

  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));

  #ifdef HAS_SCREEN
    display_obj.RunSetup();
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  backlightOff();

 
  #ifdef HAS_SCREEN
    display_obj.tft.drawCentreString("ESP32 Wifi Analyzer", TFT_WIDTH/2, TFT_HEIGHT * 0.33, 1);
  #endif


  backlightOn(); // Need this

  #ifdef HAS_SCREEN
    //delay(2000);

    // Do some stealth mode stuff
    #ifdef HAS_BUTTONS
      if (c_btn.justPressed()) {
        display_obj.headless_mode = true;

        backlightOff();

        Serial.println("Headless Mode enabled");
      }
    #endif

  #endif

  settings_obj.begin();

  wifi_scan_obj.RunSetup();

  //#ifdef HAS_SCREEN
  //  display_obj.tft.println(F(text_table0[2]));
  //#endif

  buffer_obj = Buffer();
  #if defined(HAS_SD)
    // Do some SD stuff
    if(sd_obj.initSD()) {
      #ifdef HAS_SCREEN
        //display_obj.tft.println(F(text_table0[3]));
      #endif
    } else {
      Serial.println(F("SD Card NOT Supported"));
      #ifdef HAS_SCREEN
        //display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
        //display_obj.tft.println(F(text_table0[4]));
        //display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      #endif
    }
  #endif

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawCentreString("Initializing...", TFT_WIDTH/2, TFT_HEIGHT * 0.82, 1);
  #endif

  evil_portal_obj.setup();

  #ifdef HAS_BATTERY
    battery_obj.RunSetup();
  #endif
  
  #ifdef HAS_SCREEN
    //display_obj.tft.println(F(text_table0[5]));
  #endif

  #ifdef HAS_SCREEN
    //display_obj.tft.println(F(text_table0[6]));
  #endif

  #ifdef HAS_BATTERY
    battery_obj.battery_level = battery_obj.getBatteryLevel();
  #endif


  #ifdef HAS_SCREEN
    //display_obj.tft.println(F(text_table0[7]));

    //delay(500);
  #endif

  #ifdef HAS_GPS
    gps_obj.begin();
    //#ifdef HAS_SCREEN
      //if (gps_obj.getGpsModuleStatus())
        //display_obj.tft.println("GPS Module connected");
      //else
        //display_obj.tft.println("GPS Module NOT connected");
    //#endif
  #endif

  #ifdef HAS_SCREEN
    //display_obj.tft.println(F(text_table0[8]));
  
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
    //delay(2000);
  #endif

  #ifdef HAS_SCREEN
    menu_function_obj.RunSetup();
  #endif

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  
  Serial.println(F("CLI Ready"));
  cli_obj.RunSetup();
}


void loop()
{
  currentTime = millis();
  bool mini = false;

  #ifdef SCREEN_BUFFER
    mini = true;
  #endif

  #if defined(HAS_ILI9341) || defined(HAS_ST7796)
    #ifdef HAS_BUTTONS
      if (c_btn.isHeld()) {
        if (menu_function_obj.disable_touch)
          menu_function_obj.disable_touch = false;
        else
          menu_function_obj.disable_touch = true;

        menu_function_obj.updateStatusBar();

        while (!c_btn.justReleased())
          delay(1);
      }
    #endif
  #endif

  // Update all of our objects
  /*#ifdef HAS_SCREEN
    bool do_draw = display_obj.draw_tft;
  #else
    bool do_draw = false;
  #endif*/
  
  //if ((!do_draw) && (wifi_scan_obj.currentScanMode != ESP_UPDATE))
  //{
  cli_obj.main(currentTime);
  #ifdef HAS_SCREEN
    display_obj.main(wifi_scan_obj.currentScanMode);
  #endif
  wifi_scan_obj.main(currentTime);
  //evil_portal_obj.main(wifi_scan_obj.currentScanMode);

  #ifdef HAS_GPS
    gps_obj.main();
  #endif
  
  // Detect SD card
  #if defined(HAS_SD)
    sd_obj.main();
  #endif

  // Save buffer to SD and/or serial
  buffer_obj.save();

  #ifdef HAS_BATTERY
    battery_obj.main(currentTime);
    //temp_obj.main(currentTime);
  #endif
  settings_obj.main(currentTime);
  if (((wifi_scan_obj.currentScanMode != WIFI_PACKET_MONITOR) && (wifi_scan_obj.currentScanMode != WIFI_SCAN_EAPOL)) ||
      (mini)) {
    #ifdef HAS_SCREEN
      menu_function_obj.main(currentTime);
    #endif
    //cli_obj.main(currentTime);
  }


  //if (wifi_scan_obj.currentScanMode == OTA_UPDATE)
  //  web_obj.main();
  #ifdef HAS_SCREEN
    delay(1);
  #else
    delay(50);
  #endif
  //}
  /*else if (wifi_scan_obj.currentScanMode == ESP_UPDATE) {
    #ifdef HAS_SCREEN
      display_obj.main(wifi_scan_obj.currentScanMode);
      menu_function_obj.main(currentTime);
    #endif


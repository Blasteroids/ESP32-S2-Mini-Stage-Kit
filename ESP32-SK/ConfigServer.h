#ifndef _CONFIGSERVER_H_
#define _CONFIGSERVER_H_

#include <vector>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "WebpageBuilder.h"

const String HOTSPOT_SSID = "ESP32_StageKit";
const String HOTSPOT_PASS = "1234567890";  // Has to be minimum 10 digits?

const String CONFIG_FILENAME = "/config.json";

// Just to make the code a little more obvious
#define CONFIGSERVER_NOF_COLOURS           4   // Red, green, blue, yellow
#define CONFIGSERVER_NOF_COLOUR_GROUPS     5   // Red, green, blue, yellow, strobe
#define CONFIGSERVER_NOF_GROUPS_PER_COLOUR 8

class ConfigServer {
public:
  ConfigServer();

  ~ConfigServer();
  
  void Init();

  // Returns true if connected to WiFi, false if gone into WiFi setup AP.
  bool ConnectToWiFi();
  
  bool IsConnectedToWiFi();

  void StartWebServer( WebServer* ptr_WebServer );

  bool Update();
  
  void HandleWebServerData();

  String m_wifi_ssid;
  String m_wifi_pass;
  String m_wifi_ip;
  String m_wifi_subnet;

  // Actual Config Data
  int m_gpio_mosi;
  int m_gpio_sclk;
  int m_dma_channel;
  int m_clock_speed_hz;

  // Makes the main loop pause to conserve CPU.
  // Need loop to be quicker when STROBE_ON has been issued.
  int m_looptime_target_strobe_off_ms;
  int m_looptime_target_strobe_on_ms;

  // Set to 0.0.0.0 to accept data from anywhere
  String m_rb3e_source_ip;

  // RB3E X360 PORT = 21070
  // RB3E XENIA     = 20050  (Byte flipped because endianness isn't checked in my Xenia version)
  int m_rb3e_listening_port;

  // Amount of SK9822 LED segments.
  int m_total_led_amount;
  
  // If -1 then use individual group brightness, else use this.
  int m_pod_brightness;
  
  // Strobe
  bool m_strobe_enabled;
  int  m_strobe_rate_ms[ 4 ];
  bool m_strobe_leds_all;

  // Colourstrip RGB values
  int   m_colour_rgb[ 3 * CONFIGSERVER_NOF_COLOUR_GROUPS ];

  // Brightness per group, plus 1 for strobe.
  int   m_group_brightness[ 8 * CONFIGSERVER_NOF_COLOURS + 1 ];

  // Amount of leds in each group, plus 1 for strobe.
  int   m_group_led_amount[ 8 * CONFIGSERVER_NOF_COLOURS + 1 ];

  // The ledstrip led positions for each grouping
  int** m_group_leds;

private:
  void ResetConfigToDefault();
  
  // Basic auto setup.
  void AutoAssignLEDs( int amount_of_leds_per_group, int led_gap_between_groups );

  void SettingsSave();
  bool SettingsLoad();
  
  void SendInitialWiFiSetupPage();

  void SendMainWebPage();
  bool SelectLEDGroupPage( int colour_number, int group_number, int* led_numbers, int amount_of_led_numbers, int brightness_value, int* strobe_rate_ms );
  void SendLEDGroupWebPage( const String& colour, int colour_number, int group_number, int* led_numbers, int amount_of_led_numbers, int brightness_value, int* strobe_rate_ms );

  bool HandleWebGet();
  bool HandleWebPost();

  bool HandleWebInit();
  bool HandleWebMain();
  bool HandleWebGroup();

  bool ProcessInitPage();


  WebServer*     m_ptr_WebServer;
  WebpageBuilder m_WebpageBuilder;
  
  bool m_settings_changed;
  bool m_is_connected_to_wifi;
};

#endif

#ifndef _ESP32STAGEKIT_
#define _ESP32STAGEKIT_

//
#include <iostream>
#include <string>
#include <WebServer.h>

//
#include "ConfigServer.h"
#include "StageKitConsts.h"
#include "RB3E_Network.h"
#include "SK9822.h"

class ESP32StageKit {
public:
  ESP32StageKit();

  ~ESP32StageKit();

  void Init( WebServer* ptr_WebServer );

  bool Start();

  bool IsStarted();

  long Update( const long time_passed_ms ); // Returns time to sleep in ms

  void Stop();

  void HandleWebServerData();

  void ConfigChanged();

private:  
  void RB3ENetwork_Poll();

  void HandleRumbleData( uint8_t left_weight, uint8_t right_weight );

  void HandleColourUpdate( uint8_t colour_number, uint8_t groups_to_set );

  long HandleTimeUpdate( const long time_passed_ms );

  bool               m_is_started;

  // LEDs
  SK9822             mSK9822;
  
  // Networking
  RB3E_Network       mRB3E_Network;
  
  // For strobe timing
  uint8_t            m_leds_strobe_speed_current;
  long               m_leds_strobe_next_on_ms;
  
  // LED quick lookup table
  uint8_t            m_SK_LED_Number[ 8 ];
  
  // For loop waiting
  long               m_sleep_time;
  
  // Temp vars that are reused a lot.
  int                m_tmp_number_leds;
  int*               m_tmp_led_numbers;
  uint8_t            m_tmp_brightness;
  uint8_t            m_tmp_red_value;
  uint8_t            m_tmp_green_value;
  uint8_t            m_tmp_blue_value;
  int                m_tmp_array_position;

  // No Data counter, to switch all LEDs off or a certain colour.
  long               m_nodata_ms_count;

  WebServer*    m_ptr_WebServer;
  
  // Config
  ConfigServer  m_ConfigServer;
};

#endif



#include "ESP32StageKit.h"

ESP32StageKit::ESP32StageKit() {
}

ESP32StageKit::~ESP32StageKit() {
  this->Stop();
}

void ESP32StageKit::Init( WebServer* ptr_WebServer ) {

  // Init must be called because class constructor is not called by default on global var.
  m_ConfigServer.Init();

  // Attempt to connect to WiFi.  On failure will create a hotspot.
  m_ConfigServer.ConnectToWiFi();

  // Startup the webserver.
  m_ConfigServer.StartWebServer( ptr_WebServer );

  m_leds_strobe_speed_current   = 0;
  m_leds_strobe_next_on_ms      = 0;
  m_nodata_ms_count             = 0;
  m_sleep_time                  = m_ConfigServer.m_looptime_target_strobe_off_ms;

  // Quick lookup array  
  m_SK_LED_Number[ 0 ] = SK_LED_1;
  m_SK_LED_Number[ 1 ] = SK_LED_2;
  m_SK_LED_Number[ 2 ] = SK_LED_3;
  m_SK_LED_Number[ 3 ] = SK_LED_4;
  m_SK_LED_Number[ 4 ] = SK_LED_5;
  m_SK_LED_Number[ 5 ] = SK_LED_6;
  m_SK_LED_Number[ 6 ] = SK_LED_7;
  m_SK_LED_Number[ 7 ] = SK_LED_8;
  
  this->ConfigChanged();

  mRB3E_Network.Init();
  
  m_is_started = false;
}

bool ESP32StageKit::Start() {
  m_is_started = mRB3E_Network.StartReceiver( m_ConfigServer.m_rb3e_source_ip, m_ConfigServer.m_rb3e_listening_port );
  
  return m_is_started;
}

bool ESP32StageKit::IsStarted() {
  return m_is_started;
}

long ESP32StageKit::Update( long time_passed_ms ) {

  if( m_ConfigServer.Update() ) {
    if( m_ConfigServer.IsConnectedToWiFi() && !m_is_started ){
      this->Start();
    }
    this->ConfigChanged();
  }

  if( m_is_started ) {
    this->RB3ENetwork_Poll();
  }

  m_sleep_time = this->HandleTimeUpdate( time_passed_ms );

  // Taken from StageKitPied code but not implemented.
  // TODO: Maybe implement this?
/*
  if( NODATA_MS > 0 ) {
    m_nodata_ms_count += m_sleep_time;
    if( m_nodata_ms_count > NODATA_MS ) {
      mSK9822.SetColourAll( LEDS_RGB_NODATA[ 0 ], LEDS_RGB_NODATA[ 1 ], LEDS_RGB_NODATA[ 2 ], NODATA_BRIGHTNESS );
      m_nodata_ms_count = 0;
    }
  }
*/
  return m_sleep_time;
}

void ESP32StageKit::HandleWebServerData() {
  m_ConfigServer.HandleWebServerData();
}

void ESP32StageKit::ConfigChanged() {
  void Setup( const int number_leds, const int gpio_sclk, const int gpio_mosi, const int clock_speed_hz, const int dma_channel );

  mSK9822.Setup( m_ConfigServer.m_total_led_amount, m_ConfigServer.m_gpio_sclk, m_ConfigServer.m_gpio_mosi, m_ConfigServer.m_clock_speed_hz, m_ConfigServer.m_dma_channel );
}

void ESP32StageKit::Stop() {
  mRB3E_Network.Stop();
  return;
}

void ESP32StageKit::RB3ENetwork_Poll() {
  if( mRB3E_Network.Poll() ) {
    if( mRB3E_Network.EventWasStagekit() ) {
      this->HandleRumbleData( mRB3E_Network.GetWeightLeft(), mRB3E_Network.GetWeightRight() );
    }
  }
}

// left_weight = leds    right_weight = colour
void ESP32StageKit::HandleRumbleData( uint8_t left_weight, uint8_t right_weight ) {
  switch( right_weight ) {
    case SKRUMBLEDATA::SK_LED_RED:
      this->HandleColourUpdate( 0, left_weight );
      break;
    case SKRUMBLEDATA::SK_LED_GREEN:
      this->HandleColourUpdate( 1, left_weight );
      break;
    case SKRUMBLEDATA::SK_LED_BLUE:
      this->HandleColourUpdate( 2, left_weight );
      break;
    case SKRUMBLEDATA::SK_LED_YELLOW:
      this->HandleColourUpdate( 3, left_weight );
      break;
    case SKRUMBLEDATA::SK_FOG_ON:
      break;
    case SKRUMBLEDATA::SK_FOG_OFF:
      break;
    case SKRUMBLEDATA::SK_STROBE_OFF:
      m_leds_strobe_speed_current = 0;
      m_leds_strobe_next_on_ms = 0;
      mSK9822.AllOff();
      mSK9822.Update();
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_1:
      m_leds_strobe_speed_current = 1;
      m_leds_strobe_next_on_ms = 0;
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_2:
      m_leds_strobe_speed_current = 2;
      m_leds_strobe_next_on_ms = 0;
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_3:
      m_leds_strobe_speed_current = 3;
      m_leds_strobe_next_on_ms = 0;
      break;
    case SKRUMBLEDATA::SK_STROBE_SPEED_4:
      m_leds_strobe_speed_current = 4;
      m_leds_strobe_next_on_ms = 0;
      break;
    case SKRUMBLEDATA::SK_ALL_OFF:
      m_leds_strobe_speed_current = 0;
      m_leds_strobe_next_on_ms = 0;
      mSK9822.AllOff();
      mSK9822.Update();
      break;
    default:
      break;
  }

  // Anything other than fog off counts as new data since fog off is constantly sent.
  if( right_weight != SKRUMBLEDATA::SK_FOG_OFF ) {
    m_nodata_ms_count = 0;
  }
}

void ESP32StageKit::HandleColourUpdate( uint8_t colour_number, uint8_t groups_to_set ) {

  m_tmp_array_position = colour_number * CONFIGSERVER_NOF_GROUPS_PER_COLOUR;

  m_tmp_red_value   = m_ConfigServer.m_colour_rgb[ colour_number * 3 + 0 ];
  m_tmp_green_value = m_ConfigServer.m_colour_rgb[ colour_number * 3 + 1 ];
  m_tmp_blue_value  = m_ConfigServer.m_colour_rgb[ colour_number * 3 + 2 ];

  for( int sk_led_number = 0; sk_led_number < 8; sk_led_number++ ) {
    m_tmp_led_numbers = m_ConfigServer.m_group_leds[ m_tmp_array_position + sk_led_number ];

    if( ( groups_to_set & m_SK_LED_Number[ sk_led_number ] ) == 0 ) {
      m_tmp_brightness = 0;
    } else {
      if( m_ConfigServer.m_pod_brightness != -1 ) {
        m_tmp_brightness = m_ConfigServer.m_pod_brightness;
      } else {
        m_tmp_brightness = m_ConfigServer.m_group_brightness[ m_tmp_array_position + sk_led_number ];
      }
    }

    m_tmp_number_leds = m_ConfigServer.m_group_led_amount[ m_tmp_array_position + sk_led_number ];

    for( int i = 0; i < m_tmp_number_leds; i++ ) {
      mSK9822.SetColour( m_tmp_led_numbers[ i ],  m_tmp_red_value, m_tmp_green_value, m_tmp_blue_value, m_tmp_brightness );
    }
  }

  mSK9822.Update();
}

long ESP32StageKit::HandleTimeUpdate( long time_passed_ms ) {

  uint16_t time_to_sleep = m_ConfigServer.m_looptime_target_strobe_off_ms;

  // Check strobe status
  if( m_leds_strobe_speed_current > 0 ) {
  
    if( time_passed_ms < m_leds_strobe_next_on_ms ) {
      m_leds_strobe_next_on_ms -= time_passed_ms;
    } else {
      // Strobe ON
      if( m_ConfigServer.m_strobe_enabled ) {
        if( m_ConfigServer.m_pod_brightness != -1 ) {
          m_tmp_brightness  = m_ConfigServer.m_pod_brightness;
        } else {
          m_tmp_brightness  = m_ConfigServer.m_group_brightness[ 32 ];
        }
        m_tmp_red_value   = m_ConfigServer.m_colour_rgb[ 12 ];
        m_tmp_green_value = m_ConfigServer.m_colour_rgb[ 13 ];
        m_tmp_blue_value  = m_ConfigServer.m_colour_rgb[ 14 ];

        if( m_ConfigServer.m_strobe_leds_all ) {
          mSK9822.SetColourAll( m_tmp_red_value, m_tmp_green_value, m_tmp_blue_value, m_tmp_brightness );
        } else {
          m_tmp_number_leds = m_ConfigServer.m_group_led_amount[ 4 * 8 ];  // Strobe is stored as colour_number 4
          m_tmp_led_numbers = m_ConfigServer.m_group_leds[ 4 * 8 ];
          for( int i = 0; i <  m_tmp_number_leds; i++ ) {
            mSK9822.SetColour( m_tmp_led_numbers[ i ], m_tmp_red_value, m_tmp_green_value, m_tmp_blue_value, m_tmp_brightness );
          }
        }
        mSK9822.Update();
      }
      delay( 10 );

      m_leds_strobe_next_on_ms += m_ConfigServer.m_strobe_rate_ms[ m_leds_strobe_speed_current - 1 ];
      m_leds_strobe_next_on_ms -= time_passed_ms;

      // Strobe OFF
      mSK9822.AllOff();
      mSK9822.Update();
    }

    // How long till the strobe needs to be checked?
    if( m_leds_strobe_next_on_ms < time_to_sleep ) {
      time_to_sleep = m_leds_strobe_next_on_ms;
    } else {
      time_to_sleep = m_ConfigServer.m_looptime_target_strobe_on_ms;
    }
  }
    
  return time_to_sleep;
}

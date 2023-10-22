#include "ConfigServer.h"

ConfigServer::ConfigServer() {
}

ConfigServer::~ConfigServer() {
  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    delete [] m_group_leds[ i ];
  }
  delete [] m_group_leds;
}

void ConfigServer::Init() {
  m_is_connected_to_wifi = false;

  m_group_leds = new int*[ CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1 ];

  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    m_group_leds[ i ] = new int[ 1 ];
  }

  if( !this->SettingsLoad() ) {
    Serial.println( "Settings failed to load - Resetting to default." );
    this->ResetConfigToDefault();
  }

  m_settings_changed = true;
}

void ConfigServer::ResetConfigToDefault() {
  // No wifi setup on default, force AP
  m_wifi_ssid   = "";
  m_wifi_pass   = "";
  m_wifi_ip     = "";
  m_wifi_subnet = "";

  // Default config of 32 LEDs, with LED 1 being first LED in the string.
  // Each stage kit LED is 1:1 mapped to the 32 leds in the order of (red, green, blue, yellow) repeated 8 times
  // Strobe is enabled by flashing all LEDs
  m_gpio_mosi      = 35;
  m_gpio_sclk      = 36;
  m_dma_channel    =  1;
  m_clock_speed_hz = 1000000 * 10;

  m_looptime_target_strobe_off_ms = 10;
  m_looptime_target_strobe_on_ms  =  5;


  m_rb3e_source_ip      = "0.0.0.0";
  m_rb3e_listening_port = 20050;  // Needs to be 21070 for real X360.

  // Amount of SK9822 LED segments.
  m_total_led_amount = 32;

  // Default to individual colour group brightness
  m_pod_brightness = -1;
  
  // Red
  m_colour_rgb[  0 ] = 255;
  m_colour_rgb[  1 ] =   0;
  m_colour_rgb[  2 ] =   0;
  // Green
  m_colour_rgb[  3 ] =   0;
  m_colour_rgb[  4 ] = 255;
  m_colour_rgb[  5 ] =   0;
  // Blue
  m_colour_rgb[  6 ] =   0;
  m_colour_rgb[  7 ] =   0;
  m_colour_rgb[  8 ] = 255;
  // Yellow
  m_colour_rgb[  9 ] = 255;
  m_colour_rgb[ 10 ] = 255;
  m_colour_rgb[ 11 ] =   0;
  // Strobe
  m_colour_rgb[ 12 ] = 255;
  m_colour_rgb[ 13 ] = 255;
  m_colour_rgb[ 14 ] = 255;

  // The brightness of each LED group.  Range = 0 - 31  where 0 is off & 31 is max brightness.
  // Default is 1 due to power draw during testing.
  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    m_group_brightness[ i ] = 1;
  }

  // Auto assign 1 led-strip led per group with no gaps between them.
  this->AutoAssignLEDs( 1, 0 );
  
  m_strobe_enabled      = true;
  m_strobe_leds_all     = true;
  m_strobe_rate_ms[ 0 ] = 160;
  m_strobe_rate_ms[ 1 ] = 120;
  m_strobe_rate_ms[ 2 ] = 100;
  m_strobe_rate_ms[ 3 ] =  80;

}

void ConfigServer::AutoAssignLEDs( int amount_of_leds_per_group, int led_gap_between_groups ) {
  int i;
  for( i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS; i++ ) {
    m_group_led_amount[ i ] = amount_of_leds_per_group;
  }
  
  // Strobe only has 1 group.
  for( ; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    m_group_led_amount[ i ] = 0; // Assign to 0.  Auto assign strobe is all leds.
  }

  for( i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS; i++ ) {
    delete [] m_group_leds[ i ];
    m_group_leds[ i ] = new int[ m_group_led_amount[ i ] ];
  }

  // Strobe only has 1 group.
  for( ; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    delete [] m_group_leds[ i ];
    m_group_leds[ i ] = new int[ 1 ];  // It's really 0 ;)
  }

  // Quick & dirty formula to auto assign LEDS base on amount_of_leds_per_group.  TODO: Make a real auto-assign based on number of leds only.
  // ledstrip_id = ( group_number * TOTAL_COLOURS * amount_of_leds_per_group ) + ( colour_number * amount_of_leds_per_group ) + n
  //               where n = 0 to amount_of_leds_per_group
  int ledstrip_led_position;
  int colour_group_position;

  // Colours 0 - 3
  for( int colour_number = 0; colour_number < CONFIGSERVER_NOF_COLOURS; colour_number++ ) {
    for( int group_number = 0; group_number < 8; group_number++ ) {
      // Group 0 - 7
      colour_group_position = colour_number * 8 + group_number;
      ledstrip_led_position = 1 + ( group_number * CONFIGSERVER_NOF_COLOURS * amount_of_leds_per_group ) + ( colour_number * amount_of_leds_per_group );
      ledstrip_led_position += ( group_number * CONFIGSERVER_NOF_COLOURS + colour_number ) * led_gap_between_groups;
      for( int led_item = 0; led_item < amount_of_leds_per_group; led_item++ ) {
        // LED amount
        m_group_leds[ colour_group_position ][ led_item ] = ledstrip_led_position++;
      }
    }
  }
}

void ConfigServer::SettingsSave() {
  // 512 LEDs should generate a config below 16k
  DynamicJsonDocument doc( 32768 );
  doc[ "wifi_ssid" ]         = m_wifi_ssid;
  doc[ "wifi_pass" ]         = m_wifi_pass;
  doc[ "wifi_ip" ]           = m_wifi_ip;
  doc[ "wifi_subnet" ]       = m_wifi_subnet;
  doc[ "gpio_mosi" ]         = m_gpio_mosi;
  doc[ "gpio_sclk" ]         = m_gpio_sclk;
  doc[ "dma_channel" ]       = m_dma_channel;
  doc[ "speed_hz" ]          = m_clock_speed_hz;
  doc[ "rb3e_ip" ]           = m_rb3e_source_ip;
  doc[ "rb3e_port" ]         = m_rb3e_listening_port;
  doc[ "led_total" ]         = m_total_led_amount;
  doc[ "pod_brightness" ]    = m_pod_brightness;
  doc[ "strobe_enabled" ]    = m_strobe_enabled;
  doc[ "strobe_all" ]        = m_strobe_leds_all;

  JsonArray strobe_rates = doc.createNestedArray( "strobe_rates" );
  for( int i = 0; i < 4; i++ ) {
    strobe_rates.add( m_strobe_rate_ms[ i ] );
  }
  JsonArray colour_rgb = doc.createNestedArray( "colour_rgb" );
  for( int i = 0; i < 3 * CONFIGSERVER_NOF_COLOUR_GROUPS; i++ ) {
    colour_rgb.add( m_colour_rgb[ i ] );
  }
  JsonArray grp_brightness = doc.createNestedArray( "grp_brightness" );
  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    grp_brightness.add( m_group_brightness[ i ] );
  }
  JsonArray grp_leda = doc.createNestedArray( "grp_ledamounts" );
  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    grp_leda.add( m_group_led_amount[ i ] );
  }

  JsonArray grp_leds = doc.createNestedArray( "grp_lednumbers" );
  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    for( int led_item = 0; led_item < m_group_led_amount[ i ]; led_item++ ) {
      grp_leds.add( m_group_leds[ i ][ led_item ] );
    }
  }

  //serializeJsonPretty( doc, Serial );

  // Start LittleFS
  if( !LittleFS.begin( false ) ) {
    Serial.println( "LittleFS failed.  Attempting format." );
    if( !LittleFS.begin( true ) ) {
      Serial.println( "LittleFS failed format. Config saving aborted." );
      return;     
    } else {
      Serial.println( "LittleFS: Formatted" );
    }
  }

  File config_file = LittleFS.open( CONFIG_FILENAME, "w" );
  serializeJson( doc, config_file );
  config_file.close();
}

bool ConfigServer::SettingsLoad() {
  if( !LittleFS.begin( false ) ) {
    // Failed to start LittleFS, probably no save.
    Serial.println( "Failed to start LittleFS" );
    return false;
  }

  DynamicJsonDocument doc( 32768 );
  File config_file = LittleFS.open( CONFIG_FILENAME, "r" );

  if( !config_file ) {
    // File not exist.
    Serial.println( "Failed to load file" );
    return false;
  }

  deserializeJson( doc, config_file );
  config_file.close();

  m_wifi_ssid           = doc[ "wifi_ssid" ].as<String>();
  m_wifi_pass           = doc[ "wifi_pass" ].as<String>();
  m_wifi_ip             = doc[ "wifi_ip" ].as<String>();
  m_wifi_subnet         = doc[ "wifi_subnet" ].as<String>();
  m_gpio_mosi           = doc[ "gpio_mosi" ];
  m_gpio_sclk           = doc[ "gpio_sclk" ];
  m_dma_channel         = doc[ "dma_channel" ];
  m_clock_speed_hz      = doc[ "speed_hz" ];
  m_rb3e_source_ip      = doc[ "rb3e_ip" ].as<String>();
  m_rb3e_listening_port = doc[ "rb3e_port" ];
  m_total_led_amount    = doc[ "led_total" ];
  m_pod_brightness      = doc[ "pod_brightness" ];
  m_strobe_enabled      = doc[ "strobe_enabled" ];
  m_strobe_leds_all     = doc[ "strobe_all" ];

  for( int i = 0; i < 4; i++ ) {
    m_strobe_rate_ms[ i ] = doc[ "strobe_rates" ][ i ];
  }

  for( int i = 0; i < 3 * CONFIGSERVER_NOF_COLOUR_GROUPS; i++ ) {
    m_colour_rgb[ i ] = doc[ "colour_rgb" ][ i ];
  }

  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    m_group_brightness[ i ] = doc[ "grp_brightness" ][ i ];
  }

  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    m_group_led_amount[ i ] = doc[ "grp_ledamounts" ][ i ];
  }

  int pos = 0;
  for( int i = 0; i < CONFIGSERVER_NOF_GROUPS_PER_COLOUR * CONFIGSERVER_NOF_COLOURS + 1; i++ ) {
    delete [] m_group_leds[ i ];
    if( m_group_led_amount[ i ] == 0 ) {
      m_group_leds[ i ] = new int[ 1 ];
    } else {
      m_group_leds[ i ] = new int[ m_group_led_amount[ i ] ];
      for( int led_item = 0; led_item < m_group_led_amount[ i ]; led_item++ ) {
        m_group_leds[ i ][ led_item ] = doc[ "grp_lednumbers" ][ pos++ ];
      }
    }
  }

  return true;
}

void ConfigServer::StartWebServer( WebServer* ptr_WebServer ) {
  m_ptr_WebServer = ptr_WebServer;
  m_ptr_WebServer->begin();
}

bool ConfigServer::ConnectToWiFi() {
  IPAddress ip( 192, 168, 1, 1 );
  IPAddress subnet( 255, 255, 255, 0 );

  if( m_wifi_ssid.length() != 0 ) {
    WiFi.mode( WIFI_STA );
    WiFi.begin( m_wifi_ssid, m_wifi_pass );
  
    int timeout = 20;
    Serial.print( "Connecting to WiFi" );
    while( WiFi.status() != WL_CONNECTED && timeout-- > 0 ) {
      delay( 1000 );
      Serial.print( "." );
    }

    if( timeout > 0 ) {
      if( m_wifi_ip.length() > 0 ) {
        ip.fromString( m_wifi_ip );
        subnet.fromString( m_wifi_subnet );

        WiFi.config( ip, ip, subnet );
      }
      Serial.printf( "\nConnected to WiFi. IP = " );
      Serial.println( WiFi.localIP() );
      m_is_connected_to_wifi = true;
      return true;
    }
  }

  // Ensure any old wifi ssid gets removed.
  m_wifi_ssid = "";

  Serial.println( "No config found or WiFi timeout." );
  WiFi.mode( WIFI_AP );
  WiFi.softAP( HOTSPOT_SSID, HOTSPOT_PASS );  
  WiFi.softAPConfig( ip, ip, subnet );
  
  Serial.printf( "WiFi started in AP mode with IP = " );
  Serial.println( ip );
  
  m_is_connected_to_wifi = false;

  return false;
}

bool ConfigServer::IsConnectedToWiFi() {
  return m_is_connected_to_wifi;
}

bool ConfigServer::Update() {
  m_ptr_WebServer->handleClient();

  if( m_settings_changed ) {
    m_settings_changed = false;
    return true;
  }

  return false;
}

void ConfigServer::SendInitialWiFiSetupPage() {
  String mac_address = "Device MAC = " + WiFi.macAddress();

  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "WiFi Setup Page" );
  m_WebpageBuilder.StartBody();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "WiFi Setup Page" );
  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.AddText( mac_address );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddFormAction( "/init", "POST" );
  m_WebpageBuilder.AddLabel( "wifi_ssid", "WiFi ssid : " );
  m_WebpageBuilder.AddInputType( "text", "wifi_ssid", "wifi_ssid", "", "", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "wifi_pass", "Password : " );
  m_WebpageBuilder.AddInputType( "password", "wifi_pass", "wifi_pass", "", "", true );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "ip", "IP (Leave blank if DHCP assigned) : " );
  m_WebpageBuilder.AddInputType( "text", "ip", "ip", "", String( "xxx.xxx.xxx.xxx" ), false );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddLabel( "subnet", "Subnet (Leave blank if DHCP assigned) : " );
  m_WebpageBuilder.AddInputType( "text", "subnet", "subnet", "", String( "xxx.xxx.xxx.xxx" ), false );
  m_WebpageBuilder.AddBreak( 2 );
  m_WebpageBuilder.AddButton( "submit", "Submit" );
  m_WebpageBuilder.EndFormAction();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();
  
  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::SendMainWebPage() {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddTitle( "ESP32 Stage Kit Config" );
  m_WebpageBuilder.StartBody();

  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.AddHeading( "ESP32 Stage Kit Config" );
  m_WebpageBuilder.EndCenter();
  
  m_WebpageBuilder.AddGridStyle( "grid-prefs", 4 );
  m_WebpageBuilder.AddFormAction( "/config", "POST" );
  m_WebpageBuilder.StartDivClass( "grid-prefs" );
  
  // RB3E IP | RB3E Port | Ledstrip Leds (Nof)
  m_WebpageBuilder.AddGridCellText( "RB3E IP" );
  m_WebpageBuilder.AddGridCellText( "RB3E Port" );
  m_WebpageBuilder.AddGridCellText( "Ledstrip LEDs" );
  m_WebpageBuilder.AddGridCellText( "POD Brightness" );
  m_WebpageBuilder.AddGridEntryTextCell( "rb3e_ip", m_rb3e_source_ip, false );
  m_WebpageBuilder.AddGridEntryNumberCell( "rb3e_port", m_rb3e_listening_port, 0, 65535, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "led_amount", m_total_led_amount, 1, 512, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "pod_bright", m_pod_brightness, -1, 31, true );

  // MOSI | SCLK | DMA
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "GPIO MOSI" );
  m_WebpageBuilder.AddGridCellText( "GPIO SCLK" );
  m_WebpageBuilder.AddGridCellText( "DMA" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridEntryNumberCell( "gpio_mosi", m_gpio_mosi, 1, 50, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "gpio_sclk", m_gpio_sclk, 1, 50, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "dma_channel", m_dma_channel, 1, 2, true );

  // RGB Headings
  m_WebpageBuilder.AddGridCellText( "Colours" );
  m_WebpageBuilder.AddGridCellText( "Red" );
  m_WebpageBuilder.AddGridCellText( "Green" );
  m_WebpageBuilder.AddGridCellText( "Blue" );
  // Red values [ 0 - 2 ]
  m_WebpageBuilder.AddGridCellText( "SK Red" );
  m_WebpageBuilder.AddGridEntryNumberCell(  "0", m_colour_rgb[  0 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell(  "1", m_colour_rgb[  1 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell(  "2", m_colour_rgb[  2 ], 0, 255, true );
  // Green values [ 3 - 5 ]
  m_WebpageBuilder.AddGridCellText( "SK Green" );
  m_WebpageBuilder.AddGridEntryNumberCell(  "3", m_colour_rgb[  3 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell(  "4", m_colour_rgb[  4 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell(  "5", m_colour_rgb[  5 ], 0, 255, true );
  // Blue values [ 6 - 8 ]
  m_WebpageBuilder.AddGridCellText( "SK Blue" );
  m_WebpageBuilder.AddGridEntryNumberCell(  "6", m_colour_rgb[  6 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell(  "7", m_colour_rgb[  7 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell(  "8", m_colour_rgb[  8 ], 0, 255, true );
  // Yellow values [ 9 - 11 ]
  m_WebpageBuilder.AddGridCellText( "SK Yellow" );
  m_WebpageBuilder.AddGridEntryNumberCell(  "9", m_colour_rgb[  9 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "10", m_colour_rgb[ 10 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "11", m_colour_rgb[ 11 ], 0, 255, true );
  // Strobe values [ 12 - 14 ]
  m_WebpageBuilder.AddGridCellText( "SK Strobe" );
  m_WebpageBuilder.AddGridEntryNumberCell( "12", m_colour_rgb[ 12 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "13", m_colour_rgb[ 13 ], 0, 255, true );
  m_WebpageBuilder.AddGridEntryNumberCell( "14", m_colour_rgb[ 14 ], 0, 255, true );

  // Save | | Cancel  
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddButtonAction( "main", "SAVE" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddButtonAction( "", "CANCEL" );
  
  m_WebpageBuilder.EndDiv();
  m_WebpageBuilder.EndFormAction();
  
  m_WebpageBuilder.AddBreak( 2 );

  // POD Circle Style
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.StartCircleStyle( "circle-container" );

  m_WebpageBuilder.AddCircleButtonStyle(  1, -225, -525 );  // R1
  m_WebpageBuilder.AddCircleButtonStyle(  2, -100, -400 );  // G1
  m_WebpageBuilder.AddCircleButtonStyle(  3,    0, -300 );  // B1  
  m_WebpageBuilder.AddCircleButtonStyle(  4,    0, -600 );  // Y1  
  m_WebpageBuilder.AddCircleButtonStyle(  5,  225, -525 );  // R2
  m_WebpageBuilder.AddCircleButtonStyle(  6,  100, -400 );  // G2
  m_WebpageBuilder.AddCircleButtonStyle(  7,  150, -150 );  // B2
  m_WebpageBuilder.AddCircleButtonStyle(  8,  400, -400 );  // Y2
  m_WebpageBuilder.AddCircleButtonStyle(  9,  525, -225 );  // R3
  m_WebpageBuilder.AddCircleButtonStyle( 10,  400, -100 );  // G3
  m_WebpageBuilder.AddCircleButtonStyle( 11,  300,    0 );  // B3
  m_WebpageBuilder.AddCircleButtonStyle( 12,  600,    0 );  // Y3
  m_WebpageBuilder.AddCircleButtonStyle( 13,  525,  225 );  // R4
  m_WebpageBuilder.AddCircleButtonStyle( 14,  400,  100 );  // G4
  m_WebpageBuilder.AddCircleButtonStyle( 15,  150,  150 );  // B4
  m_WebpageBuilder.AddCircleButtonStyle( 16,  400,  400 );  // Y4
  m_WebpageBuilder.AddCircleButtonStyle( 17,  225,  525 );  // R5
  m_WebpageBuilder.AddCircleButtonStyle( 18,  100,  400 );  // G5
  m_WebpageBuilder.AddCircleButtonStyle( 19,    0,  300 );  // B5
  m_WebpageBuilder.AddCircleButtonStyle( 20,    0,  600 );  // Y5
  m_WebpageBuilder.AddCircleButtonStyle( 21, -225,  525 );  // R6
  m_WebpageBuilder.AddCircleButtonStyle( 22, -100,  400 );  // G6
  m_WebpageBuilder.AddCircleButtonStyle( 23, -150,  150 );  // B6
  m_WebpageBuilder.AddCircleButtonStyle( 24, -400,  400 );  // Y6
  m_WebpageBuilder.AddCircleButtonStyle( 25, -525,  225 );  // R7
  m_WebpageBuilder.AddCircleButtonStyle( 26, -400,  100 );  // G7
  m_WebpageBuilder.AddCircleButtonStyle( 27, -300,    0 );  // B7
  m_WebpageBuilder.AddCircleButtonStyle( 28, -600,    0 );  // Y7
  m_WebpageBuilder.AddCircleButtonStyle( 29, -525, -225 );  // R8
  m_WebpageBuilder.AddCircleButtonStyle( 30, -400, -100 );  // G8
  m_WebpageBuilder.AddCircleButtonStyle( 31, -150, -150 );  // B8
  m_WebpageBuilder.AddCircleButtonStyle( 32, -400, -400 );  // Y8

  m_WebpageBuilder.EndCircleStyle();
  m_WebpageBuilder.EndCenter();
  m_WebpageBuilder.EndBody();

  // The actual POD circles
  m_WebpageBuilder.AddStandardViewportScale();
  m_WebpageBuilder.StartCenter();
  m_WebpageBuilder.StartDivClass( "circle-container" );

  for( int i = 1; i < 9; i++ ) {
    m_WebpageBuilder.AddCircleContainer( i, "red",    "0" + String( i - 1 ) );
    m_WebpageBuilder.AddCircleContainer( i, "green",  "1" + String( i - 1 ) );
    m_WebpageBuilder.AddCircleContainer( i, "blue",   "2" + String( i - 1 ) );
    m_WebpageBuilder.AddCircleContainer( i, "yellow", "3" + String( i - 1 ) );
  }

  // Strobe button
  m_WebpageBuilder.AddButtonActionForm( "40", "STROBE" );

  m_WebpageBuilder.EndDiv();

  m_WebpageBuilder.AddBreak( 2 );

  // Reset button
  m_WebpageBuilder.AddButtonActionForm( "reset", "RESET TO HOTSPOT" );
  m_WebpageBuilder.EndCenter();

  m_WebpageBuilder.EndPage();

  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
}

bool ConfigServer::SelectLEDGroupPage( int colour_number, int group_number, int* led_numbers, int amount_of_led_numbers, int brightness_value, int* strobe_rate_ms ) {
  
  if( colour_number == ( CONFIGSERVER_NOF_COLOUR_GROUPS - 1 ) ) {
    group_number = 0;
  } else if ( group_number >= CONFIGSERVER_NOF_GROUPS_PER_COLOUR || colour_number >= CONFIGSERVER_NOF_COLOUR_GROUPS ) {
    return false;
  }

  if( led_numbers == NULL ) {
    led_numbers = m_group_leds[ colour_number * CONFIGSERVER_NOF_GROUPS_PER_COLOUR + group_number ];
    amount_of_led_numbers = m_group_led_amount[ colour_number * CONFIGSERVER_NOF_GROUPS_PER_COLOUR + group_number ];
  }

  if( brightness_value == -1 ) {
    brightness_value = m_group_brightness[ colour_number * CONFIGSERVER_NOF_GROUPS_PER_COLOUR + group_number ];
  }

  if( strobe_rate_ms == NULL ) {
    strobe_rate_ms = m_strobe_rate_ms;
  }

  switch( colour_number ) {
    case 0:
      this->SendLEDGroupWebPage( "Red", colour_number, group_number, led_numbers, amount_of_led_numbers, brightness_value, strobe_rate_ms );
      return true;
      break;
    case 1:
      this->SendLEDGroupWebPage( "Green", colour_number, group_number, led_numbers, amount_of_led_numbers, brightness_value, strobe_rate_ms );
      return true;
      break;
    case 2:
      this->SendLEDGroupWebPage( "Blue", colour_number, group_number, led_numbers, amount_of_led_numbers, brightness_value, strobe_rate_ms );
      return true;
      break;
    case 3:
      this->SendLEDGroupWebPage( "Yellow", colour_number, group_number, led_numbers, amount_of_led_numbers, brightness_value, strobe_rate_ms );
      return true;
      break;
    case 4:
      this->SendLEDGroupWebPage( "STROBE", colour_number, group_number, led_numbers, amount_of_led_numbers, brightness_value, strobe_rate_ms );
      return true;
      break;
  }
  
  return false;
}

void ConfigServer::SendLEDGroupWebPage( const String& colour, int colour_number, int group_number, int* led_numbers, int amount_of_led_numbers, int brightness_value, int* strobe_rate_ms ) {
  m_WebpageBuilder.StartPage();
  m_WebpageBuilder.AddStandardViewportScale();
  
  if( colour == "STROBE" ) {
    m_WebpageBuilder.AddTitle( "Strobe config" );
    m_WebpageBuilder.StartBody();
    m_WebpageBuilder.StartCenter();
    m_WebpageBuilder.AddHeading( "Strobe config" );
    m_WebpageBuilder.EndCenter();
  } else {
    m_WebpageBuilder.StartCenter();
    m_WebpageBuilder.AddHeading( "Colour config" );
    m_WebpageBuilder.EndCenter();
    m_WebpageBuilder.StartBody();
    m_WebpageBuilder.AddText( "LEDs for colour group : " + colour + " " + String( group_number + 1 ) );
  }
  
  m_WebpageBuilder.AddBreak( 1 );
  m_WebpageBuilder.AddGridStyle( "grid-container", 3 );
  m_WebpageBuilder.AddFormAction( "/config_group", "POST" );
  m_WebpageBuilder.StartDivClass( "grid-container" );
  
  // For strobe, rates
  if( colour == "STROBE" ) {
    m_WebpageBuilder.AddGridCellText( "Use Strobe" );
    m_WebpageBuilder.AddEnabledSelection( "strobe_enabled", "strobe_enabled", m_strobe_enabled );
    m_WebpageBuilder.AddGridCellText( "" );
    m_WebpageBuilder.AddGridCellText( "All LEDs" );
    m_WebpageBuilder.AddEnabledSelection( "strobe_leds_all", "strobe_leds_all", m_strobe_leds_all );
    m_WebpageBuilder.AddGridCellText( "" );
    m_WebpageBuilder.AddGridCellText( "Slow" );
    m_WebpageBuilder.AddGridEntryNumberCell( "Rate0", strobe_rate_ms[ 0 ], 50, 500, true );
    m_WebpageBuilder.AddGridCellText( "" );
    m_WebpageBuilder.AddGridCellText( "Medium" );
    m_WebpageBuilder.AddGridEntryNumberCell( "Rate1", strobe_rate_ms[ 1 ], 50, 500, true );
    m_WebpageBuilder.AddGridCellText( "" );
    m_WebpageBuilder.AddGridCellText( "Fast" );
    m_WebpageBuilder.AddGridEntryNumberCell( "Rate2", strobe_rate_ms[ 2 ], 50, 500, true );
    m_WebpageBuilder.AddGridCellText( "" );
    m_WebpageBuilder.AddGridCellText( "Faster" );
    m_WebpageBuilder.AddGridEntryNumberCell( "Rate3", strobe_rate_ms[ 3 ], 50, 500, true );
    m_WebpageBuilder.AddGridCellText( "" );


  }
  
  // Brightness line  
  m_WebpageBuilder.AddGridCellText( "Brightness" );
  m_WebpageBuilder.AddGridEntryNumberCell( "Brightness", brightness_value, 0, 31, true );
  m_WebpageBuilder.AddGridCellText( "" );
  
  // Gap
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddGridCellText( "" );
  
  // LEDs
  m_WebpageBuilder.AddButtonAction( String ( colour_number ) + String( group_number ) + "-", "LED -" );
  m_WebpageBuilder.AddGridCellText( "LED Strip Position" );
  m_WebpageBuilder.AddButtonAction( String ( colour_number ) + String( group_number ) + "+", "LED +" );  
  
  // Leds are number args
  for( int i = 0; i < amount_of_led_numbers; i++ ) {
    m_WebpageBuilder.AddGridCellText( String( i + 1 ) );
    m_WebpageBuilder.AddGridEntryNumberCell( "L" + String( i ), led_numbers[ i ], 1, m_total_led_amount, true );
    m_WebpageBuilder.AddGridCellText( "" );
  }

  // Save | | Cancel  
  m_WebpageBuilder.AddButtonAction( String ( colour_number ) + String( group_number ) + "=", "SAVE" );
  m_WebpageBuilder.AddGridCellText( "" );
  m_WebpageBuilder.AddButtonAction( "/", "CANCEL" );

  m_WebpageBuilder.EndDiv();
  m_WebpageBuilder.EndFormAction();
  m_WebpageBuilder.EndBody();
  m_WebpageBuilder.EndPage();
  
  m_ptr_WebServer->send( 200, "text/html", m_WebpageBuilder.m_html );
}

void ConfigServer::HandleWebServerData() {
  bool handled = false;
  
  // Allow reset before anything else.
  if( m_ptr_WebServer->uri() == String( "/reset" ) ) {
    m_ptr_WebServer->send( 200, "text/plain", "Resetting to defaults - Reconnect to hotspot to setup wifi." );
    delay( 2000 );
    this->ResetConfigToDefault();
    if( LittleFS.begin( false ) ) {
      LittleFS.remove( CONFIG_FILENAME );
    }
    this->ConnectToWiFi();
    return;
  }

  // GET  - For LED group setup pages
  // POST - For setting changes
  if( m_ptr_WebServer->method() == HTTP_GET ) {
    // GET
    handled = this->HandleWebGet();
  } else {
    // POST
    handled = this->HandleWebPost();
  }

  if( !handled ) {
    m_ptr_WebServer->send( 200, "text/plain", "Not found!" );
  }
}

bool ConfigServer::HandleWebGet() {
  // Serial.printf("GET called with %s\n", m_ptr_WebServer->uri() );
  
  // Get starts with a /
  
  if( m_ptr_WebServer->uri().length() == 3 )
  {
    int colour_number = m_ptr_WebServer->uri()[ 1 ] - 48;
    int group_number = m_ptr_WebServer->uri()[ 2 ] - 48;
    
    if( colour_number < CONFIGSERVER_NOF_COLOUR_GROUPS && group_number < CONFIGSERVER_NOF_GROUPS_PER_COLOUR ) {
      if( colour_number == ( CONFIGSERVER_NOF_COLOUR_GROUPS - 1 ) ) {
        group_number = 0;
      }
      this->SelectLEDGroupPage( colour_number, group_number, NULL, -1, -1, NULL );
      return true;
    }
  } else {
    // If no wifi then hotpot.
    if( m_wifi_ssid.length() == 0 ) {
      this->SendInitialWiFiSetupPage();
    } else {
      this->SendMainWebPage();
    }
    return true;
  }
  
  return false;
}

bool ConfigServer::HandleWebPost() {
  // Serial.printf("POST called with %s\n", m_ptr_WebServer->uri() );
  
  if( m_ptr_WebServer->uri() == "/init" ) {
    return this->HandleWebInit();
  } else if( m_ptr_WebServer->uri() == "/main" ) {
    return this->HandleWebMain();
  } else if( m_ptr_WebServer->uri().length() == 4 ) {
    // [0-4][0-7][-|+|=]  : - for less 1  + for plus 1  = for save
    return this->HandleWebGroup();
  } 
  
  // Anything else, just reset back to main page
  this->SendMainWebPage();

  return true;
}

bool ConfigServer::HandleWebInit() {  
  String wifi_ssid_new   = "";
  String wifi_pass_new   = "";
  String wifi_ip_new     = "";
  String wifi_subnet_new = "";

  for( int i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ) == "wifi_ssid" ) {
      wifi_ssid_new = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "wifi_pass" ) {
      wifi_pass_new = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "ip" ) {
      wifi_ip_new = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "subnet" ) {
      wifi_subnet_new = m_ptr_WebServer->arg( i );
    }
  }

  if( wifi_ssid_new.length() > 0 && wifi_pass_new.length() > 0 ) {
    m_wifi_ssid = wifi_ssid_new;
    m_wifi_pass = wifi_pass_new;
    m_wifi_ip   = wifi_ip_new;
    if( m_wifi_ip.length() > 0 && wifi_subnet_new.length() == 0 ) {
      m_wifi_subnet = "255.255.255.0";
    } else {
      m_wifi_subnet = wifi_subnet_new;
    }

    m_ptr_WebServer->send( 200, "text/plain", "Attempting to connect to WiFi. On failure hotspot will re-appear." );
     
    if( this->ConnectToWiFi() ) {
      this->SettingsSave();
    }
    return true;
  }
  return false;
}

bool ConfigServer::HandleWebMain() {
  String rb3e_source_ip_new   = m_rb3e_source_ip;
  int rb3e_listening_port_new = m_rb3e_listening_port;
  int total_led_amount_new    = m_total_led_amount;
  int pod_brightness_new      = m_pod_brightness;
  int gpio_mosi_new           = m_gpio_mosi;
  int gpio_sclk_new           = m_gpio_sclk;
  int dma_channel_new         = m_dma_channel;
  
  uint8_t colour_rgb_new[ 3 * CONFIGSERVER_NOF_COLOUR_GROUPS ];
  for( int i = 0; i < 3 * CONFIGSERVER_NOF_COLOUR_GROUPS; i++ ) {
    colour_rgb_new[ i ] = m_colour_rgb[ i ];
  }
  
  int tmp_colour_pos;
  int tmp_colour_value;

  for( int i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ).length() < 3 ) {
      tmp_colour_pos = m_ptr_WebServer->argName( i ).toInt();
      tmp_colour_value = m_ptr_WebServer->arg( i ).toInt();
      if( tmp_colour_pos < 3 * CONFIGSERVER_NOF_COLOUR_GROUPS && tmp_colour_value < 256 ) {
        colour_rgb_new[ tmp_colour_pos ] = tmp_colour_value;
      }
    } else if( m_ptr_WebServer->argName( i ) == "rb3e_ip" ) {
      rb3e_source_ip_new = m_ptr_WebServer->arg( i );
    } else if( m_ptr_WebServer->argName( i ) == "rb3e_port" ) {
      rb3e_listening_port_new = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "led_amount" ) {
      total_led_amount_new = m_ptr_WebServer->arg( i ).toInt();
      if( total_led_amount_new < 0 || total_led_amount_new > 512 ) {
        total_led_amount_new = m_total_led_amount;
      }
    } else if( m_ptr_WebServer->argName( i ) == "pod_bright" ) {
      pod_brightness_new = m_ptr_WebServer->arg( i ).toInt();
      if( pod_brightness_new < -1 || pod_brightness_new > 31 ) {
        pod_brightness_new = m_pod_brightness;
      }
    } else if( m_ptr_WebServer->argName( i ) == "gpio_mosi" ) {
      gpio_mosi_new = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "gpio_sclk" ) {
      gpio_sclk_new = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "dma_channel" ) {
      dma_channel_new = m_ptr_WebServer->arg( i ).toInt();
    }
  }

  m_rb3e_source_ip      = rb3e_source_ip_new;
  m_rb3e_listening_port = rb3e_listening_port_new;
  m_total_led_amount    = total_led_amount_new;
  m_pod_brightness      = pod_brightness_new;
  m_gpio_mosi           = gpio_mosi_new;
  m_gpio_sclk           = gpio_sclk_new;
  m_dma_channel         = dma_channel_new;
  
  for( int i = 0; i < 3 * CONFIGSERVER_NOF_COLOUR_GROUPS; i++ ) {
    m_colour_rgb[ i ] = colour_rgb_new[ i ];
  }
  
  // Brightness set for all : TODO: New button for this.
/*
  if( m_pod_brightness > -1 ) {
    for( int group_number = 0; group_number < ( 8 * CONFIGSERVER_NOF_COLOUR_GROUPS ) + 1; group_number++ ) {
      m_group_brightness[ group_number ] = pod_brightness_new;
    }
  }
*/

  this->SettingsSave();
  
  this->SendMainWebPage();
  
  return true;
}

bool ConfigServer::HandleWebGroup() {
  // URI = [/][0-4][0-7][-|+|=]  : - for less 1  + for plus 1  = for save
  // L[n] : Leds
  int i;

  int colour_number = m_ptr_WebServer->uri()[ 1 ] - 48;
  if( colour_number >= CONFIGSERVER_NOF_COLOUR_GROUPS ) {
    return false;
  }
  
  int group_number;
  if( colour_number == ( CONFIGSERVER_NOF_COLOUR_GROUPS - 1 ) ) {
    group_number = 0;
  } else {
    group_number = m_ptr_WebServer->uri()[ 2 ] - 48;
    if( group_number >= CONFIGSERVER_NOF_GROUPS_PER_COLOUR ) {
      return false;
    }
  }
  
  int array_position = colour_number * CONFIGSERVER_NOF_GROUPS_PER_COLOUR;

  // If strobe, there's no group number.
  if( colour_number < 4 ) {
    array_position += group_number;
  }

  
  bool strobe_enabled_new  = m_strobe_enabled;
  bool strobe_leds_all_new = m_strobe_leds_all;
  int group_brightness_new = m_group_brightness[ array_position ];
  int group_led_amount_new = 0;

  int strobe_rate_ms_new[ 4 ];
  for( i = 0; i < 4; i++ ) {
      strobe_rate_ms_new[ i ] = m_strobe_rate_ms[ i ];
  }

  
  for( i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ) == "Rate0" ) {
      strobe_rate_ms_new[ 0 ] = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "Rate1" ) {
      strobe_rate_ms_new[ 1 ] = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "Rate2" ) {
      strobe_rate_ms_new[ 2 ] = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "Rate3" ) {
      strobe_rate_ms_new[ 3 ] = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "Brightness" ) {
      group_brightness_new = m_ptr_WebServer->arg( i ).toInt();
    } else if( m_ptr_WebServer->argName( i ) == "strobe_enabled" ) {
      if( m_ptr_WebServer->arg( i ) == "Enabled" ) {
        strobe_enabled_new = true;
      } else {
        strobe_enabled_new = false;
      }
    } else if( m_ptr_WebServer->argName( i ) == "strobe_leds_all" ) {
      if( m_ptr_WebServer->arg( i ) == "Enabled" ) {
        strobe_leds_all_new = true;
      } else {
        strobe_leds_all_new = false;
      }
    } else {
      if( m_ptr_WebServer->argName( i ).length() > 1 ) {
        if( m_ptr_WebServer->argName( i )[ 0 ] == 'L' ) {
          // count led_amount
          group_led_amount_new++;
        }
      }
    }
  }

  // Adding or removing an entry cell?
  if( m_ptr_WebServer->uri()[ 3 ] == '+' && group_led_amount_new < 255 ) {
    // Add 1 LED entry field
    group_led_amount_new++;
  } else if( m_ptr_WebServer->uri()[ 3 ] == '-' && group_led_amount_new > 0 ) {
    // Less 1 LED entry field
    group_led_amount_new--;
  }

  int group_leds_new[ group_led_amount_new ];

  // Populate with exising saved data
  int  group_led_amount_old  = m_group_led_amount[ array_position ];

  for( i = 0; i < group_led_amount_new && i < group_led_amount_old; i++ ) {
    group_leds_new[ i ] = m_group_leds[ array_position ][ i ];
  }

  for( ; i < group_led_amount_new; i++ ) {
    group_leds_new[ i ] = 1;
  }

  // Populate with web data
  int led_position_id;
  for( i = 0; i < m_ptr_WebServer->args(); i++ ) {
    if( m_ptr_WebServer->argName( i ).length() > 1 ) {
      if( m_ptr_WebServer->argName( i )[ 0 ] == 'L' ) {
        led_position_id = m_ptr_WebServer->argName( i ).substring( 1 ).toInt();
        if( led_position_id < group_led_amount_new ) {
          group_leds_new[ led_position_id ] = m_ptr_WebServer->arg( i ).toInt();
        }
      }
    }
  }

  if( m_ptr_WebServer->uri()[ 3 ] == '=' ) {
    // Save all the things
    m_strobe_enabled  = strobe_enabled_new;
    m_strobe_leds_all = strobe_leds_all_new;

    for( i = 0; i < 4; i++ ) {
        m_strobe_rate_ms[ i ] = strobe_rate_ms_new[ i ];
    }

    m_group_brightness[ array_position ] = group_brightness_new;
    m_group_led_amount[ array_position ] = group_led_amount_new;

    delete [] m_group_leds[ array_position ];
    if( group_led_amount_new == 0 ) {
      m_group_leds[ array_position ] = new int[ 1 ];
    } else {
      m_group_leds[ array_position ] = new int[ group_led_amount_new ];
      for( i = 0; i < group_led_amount_new; i++ ) {
        m_group_leds[ array_position ][ i ] = group_leds_new[ i ];
      }
    }

    this->SettingsSave();

    this->SendMainWebPage();
    
    return true;
  }

  // Send all the data back.
  return this->SelectLEDGroupPage( colour_number, group_number, group_leds_new, group_led_amount_new, group_brightness_new, strobe_rate_ms_new );
}

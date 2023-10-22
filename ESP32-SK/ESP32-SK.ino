#include "Arduino.h"

#include <WebServer.h>

#include "ESP32StageKit.h"

WebServer     g_WebServer;
ESP32StageKit g_StageKit;

unsigned long g_time_current_ms;
unsigned long g_time_last_ms;
unsigned long g_time_sleep_ms;

void setup() {
  if( !Serial ) {
    Serial.begin( 115200 );
  }
  delay( 1000 );

  // Callback for when web page is requested but not found.
  // All pages are just sent to the same function for processing.
  g_WebServer.onNotFound( HandleWebServerData );
  
  g_StageKit.Init( &g_WebServer );
  
  // Start time
  g_time_last_ms = millis();
}

void HandleWebServerData() {
  // Let the config server know a request has been made.
  g_StageKit.HandleWebServerData();
}

void loop() {
  // Update current time
  g_time_current_ms = millis();

  // Process time difference
  g_time_sleep_ms = g_StageKit.Update( g_time_current_ms - g_time_last_ms );

  // Update new last/start time.
  g_time_last_ms = g_time_current_ms;

  // Sleepy time
  delay( g_time_sleep_ms );
}

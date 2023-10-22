#include "Arduino.h"

#include "Config.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>

class SK9822 {
public:
  SK9822();

  ~SK9822();

  void Init();

  void Setup( const int number_leds, const int gpio_sclk, const int gpio_mosi, const int clock_speed_hz, const int dma_channel );

  void Update();

  void SetColourAll( const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness );

  void SetColour( const int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness );

  void AllOff();

  void Dump();

private:
  void SetColourNC( int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t brightness );

  int                           m_number_leds;
  uint8_t*                      m_led_buffer;
  int                           m_led_buffer_size;
  spi_device_handle_t           m_spi;
  spi_transaction_t             m_spi_transaction;
  spi_bus_config_t              m_bus_config;
  spi_device_interface_config_t m_device_interface_config;
};

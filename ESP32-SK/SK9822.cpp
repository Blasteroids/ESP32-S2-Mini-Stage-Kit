#include "SK9822.h"

SK9822::SK9822() {
}

SK9822::~SK9822(){
  delete [] m_led_buffer;
}

void SK9822::Init() {
  m_number_leds = 0;
  m_led_buffer = new uint8_t[ 1 ];
}

void SK9822::Setup( const int number_leds, const int gpio_sclk, const int gpio_mosi, const int clock_speed_hz, const int dma_channel ) {

  if( m_number_leds == number_leds ) {
    return;
  }

  m_number_leds     = number_leds;

  m_led_buffer_size =  4 * ( m_number_leds + 2 + 1 + ( ( m_number_leds / 2 ) / 32 ) );

  delete [] m_led_buffer;
  m_led_buffer = new uint8_t[ m_led_buffer_size ];

	m_bus_config.sclk_io_num     = gpio_sclk;
	m_bus_config.mosi_io_num     = gpio_mosi;
	m_bus_config.miso_io_num     = -1;
	m_bus_config.quadwp_io_num   = -1;
	m_bus_config.quadhd_io_num   = -1;
	m_bus_config.max_transfer_sz = m_led_buffer_size;
	
	m_device_interface_config.clock_speed_hz = clock_speed_hz;
	m_device_interface_config.mode           =  0;
	m_device_interface_config.spics_io_num   = -1;
	m_device_interface_config.queue_size     =  1;

	spi_bus_initialize( SPI2_HOST, &m_bus_config, dma_channel );
	spi_bus_add_device( SPI2_HOST, &m_device_interface_config, &m_spi );

  memset( &m_spi_transaction, 0, sizeof( m_spi_transaction ) );
	m_spi_transaction.length    = m_led_buffer_size * 8;
	m_spi_transaction.tx_buffer = m_led_buffer;

	spi_device_queue_trans( m_spi, &m_spi_transaction, portMAX_DELAY );

  memset( m_led_buffer, 0, m_led_buffer_size );

  this->AllOff();

  this->Update();
}

void SK9822::Update() {
  spi_device_queue_trans( m_spi, &m_spi_transaction, portMAX_DELAY );
}

void SK9822::SetColour( const int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness ) {
  if( led_number > 0 && led_number <= m_number_leds ) {
    brightness |= 0xE0;
    this->SetColourNC( led_number, red, green, blue, brightness );
  }
};

// NC - No range check & brightness must already have first 3 bits set to 1
void SK9822::SetColourNC( int led_number, const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t brightness ) {
  int offset = ( led_number * 4 );
  m_led_buffer[ offset++ ] = brightness;
  m_led_buffer[ offset++ ] = blue;
  m_led_buffer[ offset++ ] = green;
  m_led_buffer[ offset++ ] = red;
};

// Brightness = 0-31
void SK9822::SetColourAll( const uint8_t red, const uint8_t green, const uint8_t blue, uint8_t brightness ) {
  brightness |= 0xE0;
  int offset = 4;
  for( int led_number = 1; led_number <= m_number_leds; led_number++ ) {
    m_led_buffer[ offset++ ] = brightness;
    m_led_buffer[ offset++ ] = blue;
    m_led_buffer[ offset++ ] = green;
    m_led_buffer[ offset++ ] = red;
  }
};

void SK9822::AllOff() {
  this->SetColourAll( 0, 0, 0, 0 );
};

void SK9822::Dump() {
  int i = 0;
  int offset = 0;
  while( i < m_number_leds + 2 ) {
    Serial.printf( "( %i, %i, %i, %i )\n", m_led_buffer[ offset++ ], m_led_buffer[ offset++ ], m_led_buffer[ offset++ ], m_led_buffer[ offset++ ] );
    i++;
  }
};
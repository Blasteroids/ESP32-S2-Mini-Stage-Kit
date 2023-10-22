#ifndef _RB3E_NETWORK_H_
#define _RB3E_NETWORK_H_

#include "WiFi.h"
#include <WiFiUdp.h>

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <bitset>

#include "RB3E_NetworkHelpers.h"

// Replace with your network credentials
static const char* g_ssid = "InfernoMesh";
static const char* g_password = "alongtimeagocameamanonatrack";

class RB3E_Network
{
public:
  RB3E_Network();

  ~RB3E_Network();

  void Init();

  bool StartReceiver( String source_ip, uint16_t listening_port );
  
  void Stop();

  bool Poll();  // Returns true if data.
 
  bool EventWasSongName();

  bool EventWasArtist();

  bool EventWasScore();

  bool EventWasStagekit();

  bool EventWasBandInfo();

  uint8_t GetWeightLeft();
  
  uint8_t GetWeightRight();

  uint32_t GetBandScore();

  uint8_t GetBandStars();

  bool PlayerExists( const uint8_t player_id );

  uint32_t GetPlayerScore( const uint8_t player_id );

  uint8_t GetPlayerDifficulty( const uint8_t player_id );

  uint8_t GetPlayerTrackType( const uint8_t player_id );

private:
  WiFiUDP            m_wifi_network_udp;

  int                m_network_socket;
  IPAddress          m_expected_source_ip;

  uint8_t            m_data_buffer[ 512 ];
  ssize_t            m_data_buffer_last_size;

  uint8_t            m_event_type_last;
  uint8_t            m_game_state; // 0 - In menu   1 - In game

  std::string        m_song_name;
  std::string        m_song_name_short;
  std::string        m_song_artist;

  uint8_t            m_weight_left;
  uint8_t            m_weight_right;

  uint32_t           m_band_score;
  uint8_t            m_band_stars;
  uint8_t            m_player_exists[ 4 ];
  uint32_t           m_player_score[ 4 ];
  uint8_t            m_player_difficulty[ 4 ];
  uint8_t            m_player_track_type[ 4 ];  
};

#endif

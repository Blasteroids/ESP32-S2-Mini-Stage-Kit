
#include "RB3E_Network.h"

RB3E_Network::RB3E_Network() {
  m_network_socket = -1;
  m_data_buffer_last_size = 0;

  m_event_type_last = 0;
  m_game_state      = 0;
  m_weight_left     = 0;
  m_weight_right    = 0;
  m_band_score      = 0;
  m_band_stars      = 0;

  for( int i = 0; i < 4; i++ ) {
    m_player_exists[ i ] = 0;
    m_player_score[ i ] = 0;
    m_player_difficulty[ i ] = 0;
    m_player_track_type[ i ] = 0;
  }  
}

RB3E_Network::~RB3E_Network() {
  this->Stop();
}

void RB3E_Network::Init() {
  m_network_socket = -1;
  m_data_buffer_last_size = 0;

  m_event_type_last = 0;
  m_game_state      = 0;
  m_weight_left     = 0;
  m_weight_right    = 0;
  m_band_score      = 0;
  m_band_stars      = 0;

  for( int i = 0; i < 4; i++ ) {
    m_player_exists[ i ] = 0;
    m_player_score[ i ] = 0;
    m_player_difficulty[ i ] = 0;
    m_player_track_type[ i ] = 0;
  }  
}

// source_ip = The IP of the machine running RB3E
// listening_port = The local port to listen on for the RB3E data
bool RB3E_Network::StartReceiver( String source_ip, uint16_t listening_port ) {

  m_wifi_network_udp.begin( listening_port );

  // Save expected source ip
  if( source_ip.length() > 0 ) {
    m_expected_source_ip.fromString( source_ip );
  }

  return true;
};

void RB3E_Network::Stop() {
  m_wifi_network_udp.stop();
  WiFi.disconnect();
};

bool RB3E_Network::Poll() {
  m_data_buffer_last_size = m_wifi_network_udp.parsePacket();

  if( m_data_buffer_last_size < 1 ) {
    return false;
  }

  m_wifi_network_udp.read( m_data_buffer, 512 );

  // TODO: Implement & check this.
/*
  if( m_expected_source_ip != 0 ) {
    if( m_expected_source_ip != ntohl( senders_address.sin_addr.s_addr ) ) {
      char source_ip[ INET_ADDRSTRLEN ];
      inet_ntop( AF_INET, &( senders_address.sin_addr ), source_ip, INET_ADDRSTRLEN );
      return false;
    }
  }
*/

  RB3E_EventPacket* packet = (RB3E_EventPacket*)&m_data_buffer;
  if( ntohl( packet->Header.ProtocolMagic ) != RB3E_NETWORK_MAGICKEY ) {
    m_data_buffer_last_size = 0;
    return false;
  }

  // Process received data
  m_event_type_last = packet->Header.PacketType;

  switch( packet->Header.PacketType ) {
    case RB3E_EVENT_ALIVE:
      break;
    case RB3E_EVENT_STATE: {
      // content is a char - 00=menus, 01=ingame
      m_game_state = packet->Data[ 0 ];
      break;
    }
    case RB3E_EVENT_SONG_NAME: {
      // content is a string of the current song name
      m_song_name.assign( (char *)packet->Data, packet->Header.PacketSize );
      //Serial.printf( "Song Name = %s\n", m_song_name.c_str() );
      break;
    }
    case RB3E_EVENT_SONG_ARTIST: {
      // content is a string of the current song artist
      m_song_artist.assign( (char *)packet->Data, packet->Header.PacketSize );
      //Serial.printf( "Song Artist = %s\n", m_song_artist.c_str() );
      break;
    }
    case RB3E_EVENT_SONG_SHORTNAME: {
      // content is a string of the current shortname
      m_song_name_short.assign( (char *)packet->Data, packet->Header.PacketSize );
      //Serial.printf( "Short name = %s\n", m_song_name_short.c_str() );
      break;
    }
    case RB3E_EVENT_SCORE: {
      // content is a RB3E_EventScore struct with score info
      RB3E_EventScore* score_event_data = (RB3E_EventScore *)packet->Data;
      m_player_score[ 0 ] = score_event_data->MemberScores[ 0 ];
      m_player_score[ 1 ] = score_event_data->MemberScores[ 1 ];
      m_player_score[ 2 ] = score_event_data->MemberScores[ 2 ];
      m_player_score[ 3 ] = score_event_data->MemberScores[ 3 ];
      m_band_stars = score_event_data->Stars;
      break;
    }
    case RB3E_EVENT_STAGEKIT: {
      // content is a RB3E_EventStagekit struct with stagekit info
      RB3E_EventStagekit* stagekit_event_data = (RB3E_EventStagekit *)packet->Data;
      m_weight_left  = stagekit_event_data->LeftChannel;
      m_weight_right = stagekit_event_data->RightChannel;
      //Serial.printf( "Rumble Data = %i : %i\n", m_weight_left, m_weight_right );
      break;
    }
    case RB3E_EVENT_BAND_INFO: {
      // content is a RB3E_EventBandInfo struct with band info
      RB3E_EventBandInfo* bandinfo_event_data = (RB3E_EventBandInfo *)packet->Data;
      for( int i = 0; i < 4; i++ ) {
        m_player_exists[ i ]     = bandinfo_event_data->MemberExists[ i ];
        m_player_difficulty[ i ] = bandinfo_event_data->Difficulty[ i ];
        m_player_track_type[ i ] = bandinfo_event_data->TrackType[ i ];
      }
      break;
    }
    default: {
      break;
    }
  }

  return true;
};

bool RB3E_Network::EventWasSongName() {
  return m_event_type_last == RB3E_EVENT_SONG_NAME;
};

bool RB3E_Network::EventWasArtist() {
  return m_event_type_last == RB3E_EVENT_SONG_ARTIST;
};

bool RB3E_Network::EventWasScore() {
  return m_event_type_last == RB3E_EVENT_SCORE;
};

bool RB3E_Network::EventWasStagekit() {
  return m_event_type_last == RB3E_EVENT_STAGEKIT;
};

bool RB3E_Network::EventWasBandInfo() {
  return m_event_type_last == RB3E_EVENT_BAND_INFO;
};

uint8_t RB3E_Network::GetWeightLeft() {
  return m_weight_left;
};

uint8_t RB3E_Network::GetWeightRight() {
  return m_weight_right;
};

uint32_t RB3E_Network::GetBandScore() {
  return m_band_score;
};

uint8_t RB3E_Network::GetBandStars() {
  return m_band_stars;
};

bool RB3E_Network::PlayerExists( const uint8_t player_id ) {
  if( player_id < 4 ) {
    return m_player_exists[ player_id ] == 0 ? false:true;
  }

  return false;
};

uint32_t RB3E_Network::GetPlayerScore( const uint8_t player_id ) {
  if( player_id < 4 ) {
    return 0;
  }

  return m_player_score[ player_id ];
};

uint8_t RB3E_Network::GetPlayerDifficulty( const uint8_t player_id ) {
  if( player_id < 4 ) {
    return 0;
  }

  return m_player_difficulty[ player_id ];
};

uint8_t RB3E_Network::GetPlayerTrackType( const uint8_t player_id ) {
  if( player_id < 4 ) {
    return 0;
  }

  return m_player_track_type[ player_id ];
};

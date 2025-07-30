/*
  Vita-compatible Network Library
  Simplified version for PS Vita
*/

#ifndef NET_VITA_H
#define NET_VITA_H

#if defined(VITA_PLATFORM) || defined(__vita__)
  #define PLATFORM_VITA
  #include <psp2/net/net.h>
  #include <psp2/net/netctl.h>
  #include <psp2/kernel/threadmgr.h>
  #include <psp2/libdbg.h>
#else
  #error "This file is for PS Vita only"
#endif

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <list>
#include <vector>

#include <include/utility.hpp>

namespace net
{
  // internet address
  class Address
  {
  public:
    Address()
    {
      address = 0;
      port = 0;
    }

    Address( unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port )
    {
      this->address = ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
      this->port = port;
    }

    Address( unsigned int address, unsigned short port )
    {
      this->address = address;
      this->port = port;
    }

    unsigned int GetAddress() const
    {
      return address;
    }

    std::string GetA() const
    {
      return std::to_string( ( unsigned char ) ( address & 0xFF ) );
    }

    std::string GetB() const
    {
      return std::to_string( ( unsigned char ) ( ( address >> 8 ) & 0xFF ) );
    }

    std::string GetC() const
    {
      return std::to_string( ( unsigned char ) ( ( address >> 16 ) & 0xFF ) );
    }

    std::string GetD() const
    {
      return std::to_string( ( unsigned char ) ( ( address >> 24 ) & 0xFF ) );
    }

    unsigned short GetPort() const
    {
      return port;
    }

    bool operator == ( const Address & other ) const
    {
      return address == other.address && port == other.port;
    }

    bool operator != ( const Address & other ) const
    {
      return !( *this == other );
    }

    std::string ToString( bool includePort = true ) const
    {
      std::stringstream stream;
      stream << GetA() << "." << GetB() << "." << GetC() << "." << GetD();
      
      if ( includePort )
        stream << ":" << port;
      
      return stream.str();
    }

    static Address FromString( const std::string& addrStr, const std::string& portStr )
    {
      std::stringstream stream {addrStr};
      int a, b, c, d;
      char ch;
      stream >> a >> ch >> b >> ch >> c >> ch >> d;

      return
      {
        static_cast <uint8_t> (a),
        static_cast <uint8_t> (b),
        static_cast <uint8_t> (c),
        static_cast <uint8_t> (d),
        static_cast <uint16_t> (std::stoi(portStr)),
      };
    }

    static Address ResolveHostname( const std::string& hostname )
    {
      SCE_DBG_LOG_INFO("NETWORK: Resolving hostname '%s'", hostname.c_str());

      // For Vita, we'll use a simple approach
      // In a real implementation, you might want to use sceNetGetHostByName
      // For now, we'll just return an empty address
      SCE_DBG_LOG_INFO("NETWORK: Hostname resolution not implemented for Vita yet");
      return {};
    }

  private:
    unsigned int address;
    unsigned short port;
  };

  // sockets
  inline bool InitializeSockets()
  {
    // Vita doesn't need explicit socket initialization
    return true;
  }

  inline void ShutdownSockets()
  {
    // Vita doesn't need explicit socket shutdown
  }

  class Socket
  {
  public:
    Socket()
    {
      socketHandle = -1;
    }

    ~Socket()
    {
      Close();
    }

    bool Open( unsigned short port )
    {
      assert( !IsOpen() );

      // create socket
      socketHandle = sceNetSocket( "UDP", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, SCE_NET_IPPROTO_UDP );

      if ( socketHandle < 0 )
      {
        SCE_DBG_LOG_ERROR( "NETWORK: Failed to create socket!" );
        socketHandle = -1;
        return false;
      }

      // bind to port
      SceNetSockaddrIn address;
      address.sin_family = SCE_NET_AF_INET;
      address.sin_addr.s_addr = SCE_NET_INADDR_ANY;
      address.sin_port = sceNetHtons( port );

      if ( sceNetBind( socketHandle, (SceNetSockaddr*)&address, sizeof(address) ) < 0 )
      {
        SCE_DBG_LOG_ERROR( "NETWORK: Failed to bind a socket!" );
        Close();
        return false;
      }

      // set non-blocking io
      int nonBlocking = 1;
      if ( sceNetSetsockopt( socketHandle, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &nonBlocking, sizeof(nonBlocking) ) < 0 )
      {
        SCE_DBG_LOG_ERROR( "NETWORK: Failed to set Non-Blocking mode for a socket!" );
        Close();
        return false;
      }

      return true;
    }

    void Close()
    {
      if ( socketHandle >= 0 )
      {
        sceNetSocketClose( socketHandle );
        socketHandle = -1;
      }
    }

    bool IsOpen() const
    {
      return socketHandle >= 0;
    }

    bool Send( const Address & destination, const void * data, int size )
    {
      assert( data );
      assert( size > 0 );

      if ( socketHandle < 0 )
        return false;

      assert( destination.GetAddress() != 0 );
      assert( destination.GetPort() != 0 );

      SceNetSockaddrIn address;
      address.sin_family = SCE_NET_AF_INET;
      address.sin_addr.s_addr = sceNetHtonl( destination.GetAddress() );
      address.sin_port = sceNetHtons( destination.GetPort() );

      int sent_bytes = sceNetSendto( socketHandle, (const char*)data, size, 0, (SceNetSockaddr*)&address, sizeof(address) );

      return sent_bytes == size;
    }

    int Receive( Address & sender, void * data, int size )
    {
      assert( data );
      assert( size > 0 );

      if ( socketHandle < 0 )
        return 0;

      SceNetSockaddrIn from;
      unsigned int fromLength = sizeof( from );

      int received_bytes = sceNetRecvfrom(
        socketHandle,
        (char*)data,
        size, 0,
        (SceNetSockaddr*)&from,
        &fromLength );

      if ( received_bytes <= 0 )
        return 0;

      unsigned int address = sceNetNtohl( from.sin_addr.s_addr );
      unsigned short port = sceNetNtohs( from.sin_port );

      sender = Address( address, port );

      return received_bytes;
    }

  private:
    int socketHandle;
  };

  // connection
  class Connection
  {
  public:
    Socket socket {};

    enum Mode
    {
      None,
      Client,
      Server
    };

    Connection( unsigned int protocolId, float timeout )
    {
      this->protocolId = protocolId;
      this->timeout = timeout;
      mode = None;
      running = false;
      ClearData();
    }

    virtual ~Connection()
    {
      if ( IsRunning() )
        Stop();
    }

    bool Start( int port )
    {
      if ( !running )
      {
        SCE_DBG_LOG_INFO( "NETWORK: Opening connection on port %d...", port );
        if ( !socket.Open( port ) )
        {
          SCE_DBG_LOG_ERROR( "NETWORK: Could not start connection on port %d", port );
          return false;
        }
        running = true;
        OnStart();
        return true;
      }
      else
        return true;
    }

    void Stop()
    {
      if ( running )
      {
        SCE_DBG_LOG_INFO( "NETWORK: Ceasing connection" );

        bool connected = IsConnected();
        ClearData();
        socket.Close();
        running = false;
        if ( connected )
          OnDisconnect();
        OnStop();
      }
    }

    bool IsRunning() const
    {
      return running;
    }

    void Listen()
    {
      SCE_DBG_LOG_INFO( "NETWORK: Server is listening for connections..." );
      bool connected = IsConnected();
      ClearData();
      if ( connected )
        OnDisconnect();
      mode = Server;
      state = Listening;
    }

    void Connect( const Address & address )
    {
      SCE_DBG_LOG_INFO( "NETWORK: Client is connecting to %s...", address.ToString().c_str() );

      bool connected = IsConnected();
      ClearData();
      if ( connected )
        OnDisconnect();
      mode = Client;
      state = Connecting;
      this->address = address;
    }

    bool IsConnecting() const
    {
      return state == Connecting;
    }

    bool ConnectFailed() const
    {
      return state == ConnectFail;
    }

    bool ConnectTimedOut() const
    {
      return state == ConnectTimeout;
    }

    bool ConnectHasErrors() const
    {
      return state == ConnectFail || state == ConnectTimeout;
    }

    bool IsConnected() const
    {
      return state == Connected;
    }

    bool IsListening() const
    {
      return state == Listening;
    }

    Mode GetMode() const
    {
      return mode;
    }

    virtual void Update( const double deltaTime )
    {
      assert( running );
      timeoutAccumulator += deltaTime;
      if ( timeoutAccumulator > timeout )
      {
        if ( state == Connecting )
        {
          SCE_DBG_LOG_ERROR( "NETWORK: Connection failed!" );
          ClearData();
          state = ConnectFail;
          OnDisconnect();
        }
        else if ( state == Connected )
        {
          SCE_DBG_LOG_ERROR( "NETWORK: Connection timed out!" );
          ClearData();
          if ( state == Connecting )
            state = ConnectTimeout;
          OnDisconnect();
        }
      }
    }

    virtual bool SendPacket( const unsigned char data[], const int size )
    {
      assert( running );

      if ( address.GetAddress() == 0 )
        return false;

      static std::vector <uint8_t> packet {};
      packet.resize(size + 4);
      packet[0] = (unsigned char) ( protocolId >> 24 );
      packet[1] = (unsigned char) ( ( protocolId >> 16 ) & 0xFF );
      packet[2] = (unsigned char) ( ( protocolId >> 8 ) & 0xFF );
      packet[3] = (unsigned char) ( ( protocolId ) & 0xFF );
      memcpy( &packet[4], data, size );

      return socket.Send( address, packet.data(), size + 4 );
    }

    virtual int ReceivePacket( unsigned char data[], const int size )
    {
      assert( running );

      Address sender;
      static std::vector <uint8_t> packet {};
      packet.resize(size + 4);

      int bytes_read = socket.Receive( sender, packet.data(), size + 4 );
      if ( bytes_read == 0 )
        return 0;

      if ( bytes_read <= 4 )
        return 0;

      if ( packet[0] != (unsigned char) ( protocolId >> 24 ) ||
           packet[1] != (unsigned char) ( ( protocolId >> 16 ) & 0xFF ) ||
           packet[2] != (unsigned char) ( ( protocolId >> 8 ) & 0xFF ) ||
           packet[3] != (unsigned char) ( protocolId & 0xFF ) )
        return 0;

      if ( mode == Server && !IsConnected() )
      {
        state = Connected;
        address = sender;
        SCE_DBG_LOG_INFO( "NETWORK: New client connected from %s", address.ToString().c_str() );
        OnConnect();
      }

      if ( sender == address )
      {
        if ( mode == Client && state == Connecting )
        {
          SCE_DBG_LOG_INFO( "NETWORK: Successfully connected to server!" );
          state = Connected;
          OnConnect();
        }

        timeoutAccumulator = 0.0f;
        memcpy( data, &packet[4], bytes_read - 4 );

        return bytes_read - 4;
      }

      return 0;
    }

    int GetHeaderSize() const
    {
      return 4;
    }

  protected:
    virtual void OnStart()		{}
    virtual void OnStop()		{}
    virtual void OnConnect()    {}
    virtual void OnDisconnect() {}

  private:
    void ClearData()
    {
      state = Disconnected;
      timeoutAccumulator = 0.0f;
      address = Address();
    }

    enum State
    {
      Disconnected,
      Listening,
      Connecting,
      ConnectFail,
      ConnectTimeout,
      Connected
    };

    unsigned int protocolId;
    float timeout;

    bool running;
    Mode mode;
    State state;
    float timeoutAccumulator;
    Address address;
  };

  // reliability system
  struct PacketData
  {
    unsigned int sequence;			// packet sequence number
    float time;					    // time offset since packet was sent or received (depending on context)
    int size;						// packet size in bytes
  };

  inline bool sequence_more_recent( unsigned int s1, unsigned int s2, unsigned int max_sequence )
  {
    return ( ( s1 > s2 ) && ( s1 - s2 <= max_sequence/2 ) ) ||
           ( ( s2 > s1 ) && ( s2 - s1 > max_sequence/2 ) );
  }

  class PacketQueue : public std::list <PacketData>
  {
  public:
    bool exists( unsigned int sequence )
    {
      for ( iterator itor = begin(); itor != end(); ++itor )
        if ( itor->sequence == sequence )
          return true;
      return false;
    }

    void insert_sorted( const PacketData & p, unsigned int max_sequence )
    {
      if ( empty() )
      {
        push_back( p );
      }
      else
      {
        if ( !sequence_more_recent( p.sequence, front().sequence, max_sequence ) )
        {
          push_front( p );
        }
        else if ( sequence_more_recent( p.sequence, back().sequence, max_sequence ) )
        {
          push_back( p );
        }
        else
        {
          for ( PacketQueue::iterator itor = begin(); itor != end(); ++itor )
          {
            assert( itor->sequence != p.sequence );
            if ( sequence_more_recent( itor->sequence, p.sequence, max_sequence ) )
            {
              insert( itor, p );
              break;
            }
          }
        }
      }
    }

    void verify_sorted( unsigned int max_sequence )
    {
      PacketQueue::iterator prev = end();
      for ( PacketQueue::iterator itor = begin(); itor != end(); ++itor )
      {
        if ( prev != end() )
        {
          assert( prev->sequence != itor->sequence );
          assert( sequence_more_recent( itor->sequence, prev->sequence, max_sequence ) );
        }
        prev = itor;
      }
    }
  };

  class ReliabilitySystem
  {
  public:
    ReliabilitySystem( unsigned int max_sequence = 0xFFFFFFFF )
    {
      this->max_sequence = max_sequence;
      Reset();
    }

    void Reset()
    {
      local_sequence = 0;
      remote_sequence = 0;
      sent_queue.clear();
      received_queue.clear();
      pending_ack_queue.clear();
      acked_queue.clear();
      sent_packets = 0;
      recv_packets = 0;
      lost_packets = 0;
      acked_packets = 0;
      sent_bandwidth = 0.0f;
      acked_bandwidth = 0.0f;
      rtt = 0.0f;
      rtt_maximum = 1.0f;
    }

    void PacketSent( int size )
    {
      if ( sent_queue.exists( local_sequence ) )
      {
        SCE_DBG_LOG_ERROR( "NETWORK: local sequence %d exists", local_sequence );
        for ( PacketQueue::iterator itor = sent_queue.begin(); itor != sent_queue.end(); ++itor )
        {
          SCE_DBG_LOG_ERROR( "NETWORK: + %d", itor->sequence );
        }
      }
      assert( !sent_queue.exists( local_sequence ) );
      assert( !pending_ack_queue.exists( local_sequence ) );
      PacketData data;
      data.sequence = local_sequence;
      data.time = 0.0f;
      data.size = size;
      sent_queue.push_back( data );
      pending_ack_queue.push_back( data );
      sent_packets++;
      local_sequence++;
      if ( local_sequence > max_sequence )
        local_sequence = 0;
    }

    void PacketReceived( unsigned int sequence, int size )
    {
      recv_packets++;
      if ( received_queue.exists( sequence ) )
        return;
      PacketData data;
      data.sequence = sequence;
      data.time = 0.0f;
      data.size = size;
      received_queue.insert_sorted( data, max_sequence );
      if ( sequence_more_recent( sequence, remote_sequence, max_sequence ) )
        remote_sequence = sequence;
    }

    unsigned int GenerateAckBits()
    {
      return generate_ack_bits( remote_sequence, received_queue, max_sequence );
    }

    void ProcessAck( unsigned int ack, unsigned int ack_bits )
    {
      process_ack( ack, ack_bits, pending_ack_queue, acked_queue, acks, acked_packets, rtt, max_sequence );
    }

    void Update( const double deltaTime )
    {
      acks.clear();
      AdvanceQueueTime( deltaTime );
      UpdateQueues();
      UpdateStats();
    }

    void Validate()
    {
      sent_queue.verify_sorted( max_sequence );
      received_queue.verify_sorted( max_sequence );
      pending_ack_queue.verify_sorted( max_sequence );
      acked_queue.verify_sorted( max_sequence );
    }

    static bool sequence_more_recent( unsigned int s1, unsigned int s2, unsigned int max_sequence )
    {
      return ( ( s1 > s2 ) && ( s1 - s2 <= max_sequence/2 ) ) ||
             ( ( s2 > s1 ) && ( s2 - s1 > max_sequence/2 ) );
    }

    static int bit_index_for_sequence( unsigned int sequence, unsigned int ack, unsigned int max_sequence )
    {
      assert( sequence != ack );
      if ( sequence_more_recent( sequence, ack, max_sequence ) )
      {
        if ( sequence > ack )
          return (int) ( ack + ( max_sequence - sequence ) );
        else
          return (int) ( ack - sequence );
      }
      else
      {
        if ( ack > sequence )
          return (int) ( sequence + ( max_sequence - ack ) );
        else
          return (int) ( sequence - ack );
      }
    }

    static unsigned int generate_ack_bits( unsigned int ack, const PacketQueue & received_queue, unsigned int max_sequence )
    {
      unsigned int ack_bits = 0;
      for ( PacketQueue::const_iterator itor = received_queue.begin(); itor != received_queue.end(); ++itor )
      {
        if ( itor->sequence == ack || sequence_more_recent( itor->sequence, ack, max_sequence ) )
        {
          int bit_index = bit_index_for_sequence( itor->sequence, ack, max_sequence );
          if ( bit_index <= 31 )
            ack_bits |= 1 << bit_index;
        }
      }
      return ack_bits;
    }

    static void process_ack( unsigned int ack, unsigned int ack_bits,
                 PacketQueue & pending_ack_queue, PacketQueue & acked_queue,
                 std::vector<unsigned int> & acks, unsigned int & acked_packets,
                 float & rtt, unsigned int max_sequence )
    {
      if ( pending_ack_queue.empty() )
        return;

      PacketQueue::iterator itor = pending_ack_queue.begin();
      while ( itor != pending_ack_queue.end() )
      {
        bool acked = false;

        if ( itor->sequence == ack )
        {
          acked = true;
        }
        else if ( sequence_more_recent( itor->sequence, ack, max_sequence ) )
        {
          int bit_index = bit_index_for_sequence( itor->sequence, ack, max_sequence );
          if ( bit_index <= 31 )
          {
            if ( ack_bits & ( 1 << bit_index ) )
              acked = true;
          }
        }

        if ( acked )
        {
          rtt += ( itor->time - rtt ) * 0.1f;

          acked_queue.insert_sorted( *itor, max_sequence );
          acks.push_back( itor->sequence );
          acked_packets++;
          itor = pending_ack_queue.erase( itor );
        }
        else
          ++itor;
      }
    }

    unsigned int GetLocalSequence() const
    {
      return local_sequence;
    }

    unsigned int GetRemoteSequence() const
    {
      return remote_sequence;
    }

    unsigned int GetMaxSequence() const
    {
      return max_sequence;
    }

    void GetAcks( unsigned int ** acks, int & count )
    {
      if ( acks )
        *acks = &this->acks[0];
      count = (int) this->acks.size();
    }

    unsigned int GetSentPackets() const
    {
      return sent_packets;
    }

    unsigned int GetReceivedPackets() const
    {
      return recv_packets;
    }

    unsigned int GetLostPackets() const
    {
      return lost_packets;
    }

    unsigned int GetAckedPackets() const
    {
      return acked_packets;
    }

    float GetSentBandwidth() const
    {
      return sent_bandwidth;
    }

    float GetAckedBandwidth() const
    {
      return acked_bandwidth;
    }

    float GetRoundTripTime() const
    {
      return rtt;
    }

    int GetHeaderSize() const
    {
      return 12;
    }

  private:
    void AdvanceQueueTime( float deltaTime )
    {
      for ( PacketQueue::iterator itor = sent_queue.begin(); itor != sent_queue.end(); ++itor )
        itor->time += deltaTime;

      for ( PacketQueue::iterator itor = received_queue.begin(); itor != received_queue.end(); ++itor )
        itor->time += deltaTime;

      for ( PacketQueue::iterator itor = pending_ack_queue.begin(); itor != pending_ack_queue.end(); ++itor )
        itor->time += deltaTime;

      for ( PacketQueue::iterator itor = acked_queue.begin(); itor != acked_queue.end(); ++itor )
        itor->time += deltaTime;
    }

    void UpdateQueues()
    {
      const float epsilon = 0.001f;

      while ( sent_queue.size() && sent_queue.front().time > rtt_maximum + epsilon )
      {
        sent_queue.pop_front();
      }

      if ( received_queue.size() )
      {
        const unsigned int latest_sequence = received_queue.back().sequence;
        const unsigned int minimum_sequence = latest_sequence >= 34 ? ( latest_sequence - 34 ) : max_sequence - ( 34 - latest_sequence );
        while ( received_queue.size() && ( !sequence_more_recent( received_queue.front().sequence, minimum_sequence, max_sequence ) ) )
        {
          received_queue.pop_front();
        }
      }

      while ( acked_queue.size() && acked_queue.front().time > rtt_maximum * 2 - epsilon )
      {
        acked_queue.pop_front();
      }

      while ( pending_ack_queue.size() && pending_ack_queue.front().time > rtt_maximum + epsilon )
      {
        pending_ack_queue.pop_front();
        lost_packets++;
      }
    }

    void UpdateStats()
    {
      int sent_bytes_per_second = 0;
      for ( PacketQueue::iterator itor = sent_queue.begin(); itor != sent_queue.end(); ++itor )
        sent_bytes_per_second += itor->size;
      int acked_bytes_per_second = 0;
      for ( PacketQueue::iterator itor = acked_queue.begin(); itor != acked_queue.end(); ++itor )
      {
        if ( itor->time >= rtt_maximum )
          acked_bytes_per_second += itor->size;
      }
      sent_bandwidth = sent_bytes_per_second / rtt_maximum;
      acked_bandwidth = acked_bytes_per_second / rtt_maximum;
    }

    unsigned int max_sequence;
    unsigned int local_sequence;
    unsigned int remote_sequence;
    PacketQueue sent_queue;
    PacketQueue received_queue;
    PacketQueue pending_ack_queue;
    PacketQueue acked_queue;
    std::vector<unsigned int> acks;
    unsigned int sent_packets;
    unsigned int recv_packets;
    unsigned int lost_packets;
    unsigned int acked_packets;
    float sent_bandwidth;
    float acked_bandwidth;
    float rtt;
    float rtt_maximum;
  };

  class ReliableConnection : public Connection
  {
  public:
    ReliableConnection( unsigned int protocolId, float timeout, unsigned int max_sequence = 0xFFFFFFFF )
      : Connection( protocolId, timeout ), reliabilitySystem( max_sequence )
    {
      ClearData();
    }

    ~ReliableConnection()
    {
      if ( IsRunning() )
        Stop();
    }

    // overriden functions from "Connection"

    bool SendPacket( const unsigned char data[], int size )
    {
      const int header = 12;
      static std::vector <uint8_t> packet {};
      packet.resize(header + size);

      unsigned int seq = reliabilitySystem.GetLocalSequence();
      unsigned int ack = reliabilitySystem.GetRemoteSequence();
      unsigned int ack_bits = reliabilitySystem.GenerateAckBits();

      WriteHeader( packet.data(), seq, ack, ack_bits );
      memcpy( packet.data() + header, data, size );

      if ( !Connection::SendPacket( packet.data(), size + header ) )
        return false;

      reliabilitySystem.PacketSent( size );
      return true;
    }

    int ReceivePacket( unsigned char data[], int size )
    {
      const int header = 12;
      static std::vector <uint8_t> packet {};
      packet.resize(header + size);

      int received_bytes = Connection::ReceivePacket( packet.data(), size + header );

      if ( received_bytes == 0 )
        return 0;

      if ( received_bytes <= header )
        return 0;

      unsigned int packet_sequence = 0;
      unsigned int packet_ack = 0;
      unsigned int packet_ack_bits = 0;

      ReadHeader( packet.data(), packet_sequence, packet_ack, packet_ack_bits );
      reliabilitySystem.PacketReceived( packet_sequence, received_bytes - header );
      reliabilitySystem.ProcessAck( packet_ack, packet_ack_bits );

      if ( packet_sequence != reliabilitySystem.GetRemoteSequence() )
        return 0;

      memcpy( data, packet.data() + header, received_bytes - header );
      return received_bytes - header;
    }

    void Update( const double deltaTime )
    {
      Connection::Update(deltaTime);
      reliabilitySystem.Update(deltaTime);
    }

    int GetHeaderSize() const
    {
      return Connection::GetHeaderSize() + reliabilitySystem.GetHeaderSize();
    }

    ReliabilitySystem & GetReliabilitySystem()
    {
      return reliabilitySystem;
    }

  protected:
    void WriteInteger( unsigned char * data, unsigned int value )
    {
      data[0] = (unsigned char) ( value >> 24 );
      data[1] = (unsigned char) ( ( value >> 16 ) & 0xFF );
      data[2] = (unsigned char) ( ( value >> 8 ) & 0xFF );
      data[3] = (unsigned char) ( value & 0xFF );
    }

    void WriteHeader( unsigned char * header, unsigned int sequence, unsigned int ack, unsigned int ack_bits )
    {
      WriteInteger( header, sequence );
      WriteInteger( header + 4, ack );
      WriteInteger( header + 8, ack_bits );
    }

    void ReadInteger( const unsigned char * data, unsigned int & value )
    {
      value = ( ( (unsigned int)data[0] << 24 ) | ( (unsigned int)data[1] << 16 ) |
              ( (unsigned int)data[2] << 8 )  | ( (unsigned int)data[3] ) );
    }

    void ReadHeader( const unsigned char * header, unsigned int & sequence, unsigned int & ack, unsigned int & ack_bits )
    {
      ReadInteger( header, sequence );
      ReadInteger( header + 4, ack );
      ReadInteger( header + 8, ack_bits );
    }

    virtual void OnStop()
    {
      ClearData();
    }

    virtual void OnDisconnect()
    {
      ClearData();
    }

  private:
    void ClearData()
    {
      reliabilitySystem.Reset();
    }

    ReliabilitySystem reliabilitySystem;	// reliability system: manages sequence numbers and acks, tracks network stats etc.
  };

  class FlowControl
  {
  public:
    FlowControl()
    {
      SCE_DBG_LOG_INFO( "NETWORK: Flow control initialized!" );
      Reset();
    }

    void Reset()
    {
      mode = Bad;
      penalty_time = 4.0f;
      good_conditions_time = 0.0f;
      penalty_reduction_accumulator = 0.0f;
    }

    void Update( const float rtt, const double deltaTime )
    {
      const float RTT_Threshold = 250.0f;

      if ( mode == Good )
      {
        if ( rtt > RTT_Threshold )
        {
          SCE_DBG_LOG_INFO( "NETWORK: Dropping to bad mode!" );
          mode = Bad;
          if ( good_conditions_time < 10.0f && penalty_time < 60.0f )
          {
            penalty_time *= 2.0f;
            if ( penalty_time > 60.0f )
              penalty_time = 60.0f;

            SCE_DBG_LOG_INFO( "NETWORK: Penalty time increased to %.1f seconds", penalty_time );
          }
          good_conditions_time = 0.0f;
          penalty_reduction_accumulator = 0.0f;
          return;
        }

        good_conditions_time += deltaTime;
        penalty_reduction_accumulator += deltaTime;

        if ( penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f )
        {
          penalty_time /= 2.0f;
          if ( penalty_time < 1.0f )
            penalty_time = 1.0f;

          SCE_DBG_LOG_INFO( "NETWORK: Penalty time reduced to %.1f seconds", penalty_time );
          penalty_reduction_accumulator = 0.0f;
        }
      }

      if ( mode == Bad )
      {
        if ( rtt <= RTT_Threshold )
          good_conditions_time += deltaTime;
        else
          good_conditions_time = 0.0f;

        if ( good_conditions_time > penalty_time )
        {
          SCE_DBG_LOG_INFO( "NETWORK: Upgrading to good mode" );
          good_conditions_time = 0.0f;
          penalty_reduction_accumulator = 0.0f;
          mode = Good;
          return;
        }
      }
    }

    bool IsConnectionStable() const
    {
      return mode == Good;
    }

  private:
    enum Mode
    {
      Good,
      Bad
    };

    Mode mode;
    float penalty_time;
    float good_conditions_time;
    float penalty_reduction_accumulator;
  };

} // namespace net

#endif // NET_VITA_H 
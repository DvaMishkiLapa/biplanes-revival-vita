/*
  PS Vita Network Library
  Optimized network library specifically for PlayStation Vita
  Based on "Networking for Game Programmers" by Glenn Fiedler
*/

#ifndef NET_VITA_H
#define NET_VITA_H

// Ensure this library is only used on PS Vita platform
#if defined(VITA_PLATFORM) || defined(__vita__)
  #define PLATFORM_VITA
  
  // PS Vita specific network includes
  #include <psp2/net/net.h>
  #include <psp2/net/netctl.h>
  #include <psp2/kernel/threadmgr.h>
  #include <psp2/libdbg.h>
  
  // Standard network includes
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <errno.h>
#else
  #error "Net-vita.h: This file is designed exclusively for PlayStation Vita platform"
#endif

// Standard C++ includes
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <list>
#include <vector>
#include <include/utility.hpp>

// PS Vita network configuration constants - always available for cross-platform compatibility
namespace VitaConfig {
  // PS Vita reserved port ranges that should be avoided
  constexpr uint16_t SYSTEM_PORT_MIN = 1;
  constexpr uint16_t SYSTEM_PORT_MAX = 1023;
  constexpr uint16_t RESERVED_PORT_MIN = 9293;
  constexpr uint16_t RESERVED_PORT_MAX = 9308;
  constexpr uint16_t HIGH_PORT_MIN = 40000;
  constexpr uint16_t HIGH_PORT_MAX = 65535;
  
  // Safe port ranges for applications
  constexpr uint16_t SAFE_PORT_MIN = 1024;
  constexpr uint16_t SAFE_PORT_MAX = 9292;
  constexpr uint16_t SAFE_PORT_ALT_MIN = 9309;
  constexpr uint16_t SAFE_PORT_ALT_MAX = 39999;
  
  // Default network configuration
  constexpr uint16_t DEFAULT_PORT = 8080;
  constexpr int MAX_PORT_ATTEMPTS = 100;
}

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

    // Extract IP address octets efficiently for PS Vita
    inline std::string GetA() const
    {
      return std::to_string( static_cast<uint8_t>( address >> 24 ) );
    }

    inline std::string GetB() const
    {
      return std::to_string( static_cast<uint8_t>( address >> 16 ) );
    }

    inline std::string GetC() const
    {
      return std::to_string( static_cast<uint8_t>( address >> 8 ) );
    }

    inline std::string GetD() const
    {
      return std::to_string( static_cast<uint8_t>( address ) );
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
      if ( checkIp(addrStr).empty() == true )
        return {};

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
      log_message( "NETWORK: Resolving hostname '", hostname, "'\n" );

      struct addrinfo hints = {};
      struct addrinfo* addressInfo = nullptr;

      hints.ai_family = AF_INET;      // IPv4 only
      hints.ai_socktype = SOCK_DGRAM; // UDP
      hints.ai_protocol = IPPROTO_UDP;

      const int errorCode = getaddrinfo(
        hostname.c_str(),
        nullptr, 
        &hints,
        &addressInfo );

      if ( errorCode != 0 )
      {
        log_message( "NETWORK: Failed to resolve hostname '", hostname, "': getaddrinfo returned " + std::to_string(errorCode), "\n" );
        return {};
      }

      if ( addressInfo == nullptr )
      {
        log_message( "NETWORK: Failed to resolve hostname '", hostname, "': no address info returned\n" );
        return {};
      }

      // Check if we got an IPv4 address
      if ( addressInfo->ai_family != AF_INET )
      {
        log_message( "NETWORK: Failed to resolve hostname '", hostname, "': expected AF_INET family\n" );
        freeaddrinfo( addressInfo );
        return {};
      }

      const auto resolvedAddress = reinterpret_cast<struct sockaddr_in*>( addressInfo->ai_addr );


      const Address result
      {
        ntohl( resolvedAddress->sin_addr.s_addr ),
        ntohs( resolvedAddress->sin_port )
      };

      log_message( "NETWORK: Resolved hostname '", hostname, "' is " + result.ToString( false ), "\n" );

      freeaddrinfo( addressInfo );
      return result;
    }

  private:
    unsigned int address;
    unsigned short port;
  };

  // sockets
  inline bool InitializeSockets()
  {
    // VitaSDK automatically initializes the network subsystem
    // We don't need to call sceNetInit or sceNetCtlInit explicitly
    log_message( "NETWORK: Vita network subsystem initialization skipped (handled by VitaSDK)\n" );
    return false; // Return false (0) to indicate success
  }

  inline void ShutdownSockets()
  {
    // VitaSDK handles network shutdown automatically
    log_message( "NETWORK: Vita network subsystem shutdown skipped (handled by VitaSDK)\n" );
  }

  // PS Vita specific network utilities
#ifdef VITA_PLATFORM
  // Check if port is available for PS Vita applications
  inline bool isPortAvailable(uint16_t port) noexcept
  {
    using namespace VitaConfig;
    
    // Check against PS Vita reserved port ranges
    if (port >= SYSTEM_PORT_MIN && port <= SYSTEM_PORT_MAX) return false;
    if (port >= RESERVED_PORT_MIN && port <= RESERVED_PORT_MAX) return false;
    if (port >= HIGH_PORT_MIN && port <= HIGH_PORT_MAX) return false;

    // Port is in safe application range
    return true;
  }

  // Find next available port starting from given port, optimized for PS Vita
  inline uint16_t findAvailablePort(uint16_t startPort = ::VitaConfig::DEFAULT_PORT) noexcept
  {
    using namespace ::VitaConfig;
    uint16_t port = startPort;

    // Efficient port search within safe ranges
    for (int attempt = 0; attempt < MAX_PORT_ATTEMPTS; ++attempt)
    {
      if (isPortAvailable(port))
      {
        log_message( "NETWORK: Found available port: ", std::to_string(port), "\n" );
        return port;
      }

      ++port;

      // Smart range skipping to avoid reserved areas
      if (port == RESERVED_PORT_MIN) port = SAFE_PORT_ALT_MIN;
      if (port == HIGH_PORT_MIN) port = SAFE_PORT_MIN; // Wrap to beginning of safe range
    }

    log_message( "NETWORK: Could not find available port, using fallback: ", std::to_string(DEFAULT_PORT), "\n" );
    return DEFAULT_PORT;
  }
#endif

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

#ifdef VITA_PLATFORM
      // Validate port against PS Vita reserved ranges
      if (!isPortAvailable(port))
      {
        log_message( "NETWORK: Port ", std::to_string(port), " is reserved on PS Vita!\n" );
        log_message( "NETWORK: Use ports in ranges: " + std::to_string(::VitaConfig::SAFE_PORT_MIN) + "-" + 
                    std::to_string(::VitaConfig::SAFE_PORT_MAX) + " or " + std::to_string(::VitaConfig::SAFE_PORT_ALT_MIN) + 
                    "-" + std::to_string(::VitaConfig::SAFE_PORT_ALT_MAX) + "\n" );
        return false;
      }
#endif

      log_message( "NETWORK: Creating UDP socket for port ", std::to_string(port), "\n" );

      // Create UDP socket optimized for PS Vita
      socketHandle = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

      if ( socketHandle < 0 )
      {
        log_message( "NETWORK: Socket creation failed! Error: ", std::to_string(errno), "\n" );
        socketHandle = -1;
        return false;
      }

      // Configure socket address structure
      struct sockaddr_in address = {};  // Zero-initialize for safety
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port = htons( port );

      // Attempt to bind socket to port
      if ( bind( socketHandle, reinterpret_cast<struct sockaddr*>(&address), sizeof(address) ) < 0 )
      {
        const int error = errno;
        log_message( "NETWORK: Socket bind failed! Error: ", std::to_string(error), "\n" );

#ifdef VITA_PLATFORM
        // Provide PS Vita specific error handling
        switch(error) {
          case EADDRINUSE:
            log_message( "NETWORK: Port already in use. Try findAvailablePort()\n" );
            break;
          case EACCES:
            log_message( "NETWORK: Permission denied. Port may be system reserved\n" );
            break;
          case EINVAL:
            log_message( "NETWORK: Invalid socket address\n" );
            break;
          default:
            log_message( "NETWORK: Unknown bind error\n" );
            break;
        }
#endif

        Close();
        return false;
      }

      // Configure socket for non-blocking I/O (PS Vita compatible)
      const int nonBlocking = 1;
      if ( setsockopt( socketHandle, SOL_SOCKET, SO_NONBLOCK, &nonBlocking, sizeof(nonBlocking) ) < 0 )
      {
        log_message( "NETWORK: Failed to set non-blocking mode! Error: ", std::to_string(errno), "\n" );
        Close();
        return false;
      }

      log_message( "NETWORK: Socket opened successfully on port ", std::to_string(port), "\n" );
      return true;
    }

    void Close()
    {
      if ( socketHandle >= 0 )
      {
        close( socketHandle );
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

      struct sockaddr_in address;
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = htonl( destination.GetAddress() );
      address.sin_port = htons( destination.GetPort() );

      int sent_bytes = sendto( socketHandle, (const char*)data, size, 0, (struct sockaddr*)&address, sizeof(address) );

      return sent_bytes == size;
    }

    int Receive( Address & sender, void * data, int size )
    {
      assert( data );
      assert( size > 0 );

      if ( socketHandle < 0 )
        return 0;

      struct sockaddr_in from;
      unsigned int fromLength = sizeof( from );

      int received_bytes = recvfrom(
        socketHandle,
        (char*)data,
        size, 0,
        (struct sockaddr*)&from,
        &fromLength );

      if ( received_bytes <= 0 )
        return 0;

      unsigned int address = ntohl( from.sin_addr.s_addr );
      unsigned short port = ntohs( from.sin_port );

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
        log_message( "NETWORK: Opening connection on port ", std::to_string(port), "...\n" );
        if ( !socket.Open( port ) )
        {
          log_message( "NETWORK: Could not start connection on port ", std::to_string(port), "\n" );
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
        log_message( "NETWORK: Ceasing connection\n" );

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
      log_message( "NETWORK: Server is listening for connections...\n" );
      bool connected = IsConnected();
      ClearData();
      if ( connected )
        OnDisconnect();
      mode = Server;
      state = Listening;
    }

    void Connect( const Address & address )
    {
      log_message( "NETWORK: Client is connecting to ", address.ToString(), "...\n" );

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
          log_message( "NETWORK: Connection failed!\n" );
          ClearData();
          state = ConnectFail;
          OnDisconnect();
        }
        else if ( state == Connected )
        {
          log_message( "NETWORK: Connection timed out!\n" );
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
        log_message( "NETWORK: New client connected from ", address.ToString(), "\n" );
        OnConnect();
      }

      if ( sender == address )
      {
        if ( mode == Client && state == Connecting )
        {
          log_message( "NETWORK: Successfully connected to server!\n" );
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

  // Optimized sequence comparison for PS Vita
  // Determines if sequence s1 is more recent than s2, handling sequence number wrap-around
  // This algorithm works correctly even when sequence numbers wrap from max_sequence to 0
  inline bool sequence_more_recent( unsigned int s1, unsigned int s2, unsigned int max_sequence ) noexcept
  {
    const unsigned int half_sequence = max_sequence / 2;
    return ( ( s1 > s2 ) && ( s1 - s2 <= half_sequence ) ) ||
           ( ( s2 > s1 ) && ( s2 - s1 > half_sequence ) );
  }

  // Packet queue optimized for PS Vita memory constraints
  // Maintains packets in sequence order, handling sequence wrap-around correctly
  class PacketQueue : public std::list<PacketData>
  {
  public:
    // Check if a packet with given sequence number exists in the queue
    // Optimized linear search - suitable for typical game network packet counts
    bool exists( unsigned int sequence ) const noexcept
    {
      for ( const auto& packet : *this )
        if ( packet.sequence == sequence )
          return true;
      return false;
    }

    // Insert packet data maintaining sequence order
    // Uses optimized insertion strategy: check ends first, then search middle
    void insert_sorted( const PacketData& p, unsigned int max_sequence )
    {
      if ( empty() )
      {
        push_back( p );
        return;
      }

      // Fast path: check if packet belongs at the beginning
      if ( !sequence_more_recent( p.sequence, front().sequence, max_sequence ) )
      {
        push_front( p );
        return;
      }

      // Fast path: check if packet belongs at the end
      if ( sequence_more_recent( p.sequence, back().sequence, max_sequence ) )
      {
        push_back( p );
        return;
      }

      // Slower path: find correct position in middle
      for ( auto itor = begin(); itor != end(); ++itor )
      {
        assert( itor->sequence != p.sequence ); // Duplicate sequences not allowed
        if ( sequence_more_recent( itor->sequence, p.sequence, max_sequence ) )
        {
          insert( itor, p );
          return;
        }
      }
    }

    // Debug function to verify queue is properly sorted
    // Only compiled in debug builds to avoid performance impact
    void verify_sorted( unsigned int max_sequence ) const
    {
#ifdef _DEBUG
      auto prev = end();
      for ( auto itor = begin(); itor != end(); ++itor )
      {
        if ( prev != end() )
        {
          assert( prev->sequence != itor->sequence );
          assert( sequence_more_recent( itor->sequence, prev->sequence, max_sequence ) );
        }
        prev = itor;
      }
#endif
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
        log_message( "NETWORK: local sequence ", std::to_string(local_sequence), " exists\n" );
        for ( PacketQueue::iterator itor = sent_queue.begin(); itor != sent_queue.end(); ++itor )
        {
          log_message( "NETWORK: + ", std::to_string(itor->sequence), "\n" );
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

    // Calculate bit index for sequence number in acknowledgment bitfield
    // Returns the bit position (0-31) for a sequence in the 32-bit ack_bits field
    // Returns -1 if sequence shouldn't be represented in the bitfield
    static int bit_index_for_sequence( unsigned int sequence, unsigned int ack, unsigned int max_sequence ) noexcept
    {
      // Sequence equal to ack is handled separately, not in bitfield
      if ( sequence == ack )
        return -1;

      // Calculate bit index based on sequence age relative to ack
      // This handles wrap-around correctly for sequence numbers
      if ( !sequence_more_recent( sequence, ack, max_sequence ) )
      {
        // Sequence is older than ack, calculate its bit position
        if ( ack >= sequence )
        {
          const unsigned int diff = ack - sequence;
          return diff <= 32 ? static_cast<int>(diff - 1) : -1;
        }
        else
        {
          // Handle wrap-around case
          const unsigned int diff = (max_sequence - sequence) + ack + 1;
          return diff <= 32 ? static_cast<int>(diff - 1) : -1;
        }
      }
      
      // Sequence is newer than ack, shouldn't be in bitfield
      return -1;
    }

    // Generate 32-bit acknowledgment bitfield for reliable packet delivery
    // Each bit represents whether a packet sequence was received (1) or not (0)
    // Bit 0 = ack-1, Bit 1 = ack-2, etc. (up to 32 sequences back)
    static unsigned int generate_ack_bits( unsigned int ack, const PacketQueue& received_queue, unsigned int max_sequence ) noexcept
    {
      unsigned int ack_bits = 0;
      
      // Iterate through received packets to build acknowledgment bitfield
      for ( const auto& packet : received_queue )
      {
        // Skip packets that are the ack sequence itself or newer
        if ( packet.sequence == ack || sequence_more_recent( packet.sequence, ack, max_sequence ) )
          continue;

        // Calculate bit position for this sequence
        const int bit_index = bit_index_for_sequence( packet.sequence, ack, max_sequence );
        if ( bit_index >= 0 && bit_index <= 31 )
        {
          ack_bits |= (1U << bit_index);
        }
      }
      
      return ack_bits;
    }

    // Process acknowledgment packet to update RTT and move packets from pending to acked
    // This is the core of the reliability system - handles both direct acks and bitfield acks
    static void process_ack( unsigned int ack, unsigned int ack_bits,
                           PacketQueue& pending_ack_queue, PacketQueue& acked_queue,
                           std::vector<unsigned int>& acks, unsigned int& acked_packets,
                           float& rtt, unsigned int max_sequence )
    {
      if ( pending_ack_queue.empty() )
        return;

      // Process all packets in pending acknowledgment queue
      auto itor = pending_ack_queue.begin();
      while ( itor != pending_ack_queue.end() )
      {
        bool packet_acked = false;

        // Check if this packet is directly acknowledged
        if ( itor->sequence == ack )
        {
          packet_acked = true;
        }
        // Check if packet is acknowledged via bitfield (for older packets)
        else if ( !sequence_more_recent( itor->sequence, ack, max_sequence ) )
        {
          const int bit_index = bit_index_for_sequence( itor->sequence, ack, max_sequence );
          if ( bit_index >= 0 && bit_index <= 31 )
          {
            packet_acked = (ack_bits & (1U << bit_index)) != 0;
          }
        }

        if ( packet_acked )
        {
          // Update RTT using exponential smoothing (10% of new sample)
          rtt += ( itor->time - rtt ) * 0.1f;

          // Move packet from pending to acked queue
          acked_queue.insert_sorted( *itor, max_sequence );
          acks.push_back( itor->sequence );
          ++acked_packets;
          itor = pending_ack_queue.erase( itor );
        }
        else
        {
          ++itor;
        }
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

    // Get list of acknowledged packet sequences from last update cycle
    // Used by higher-level code to determine which packets were successfully delivered
    void GetAcks( unsigned int** acks_ptr, int& count ) noexcept
    {
      if ( acks_ptr && !acks.empty() )
        *acks_ptr = acks.data();
      else if ( acks_ptr )
        *acks_ptr = nullptr;
      count = static_cast<int>( acks.size() );
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

    // Update bandwidth statistics based on recent packet history
    // Calculates both sent and acknowledged bandwidth for network monitoring
    void UpdateStats() noexcept
    {
      // Calculate total bytes sent within the RTT window
      int total_sent_bytes = 0;
      for ( const auto& packet : sent_queue )
        total_sent_bytes += packet.size;

      // Calculate bytes that were acknowledged within reasonable time
      int total_acked_bytes = 0;
      for ( const auto& packet : acked_queue )
      {
        if ( packet.time >= rtt_maximum )
          total_acked_bytes += packet.size;
      }

      // Convert to bandwidth (bytes per second, then to kilobits per second)
      const float time_window = rtt_maximum > 0.0f ? rtt_maximum : 1.0f;
      sent_bandwidth = static_cast<float>(total_sent_bytes) / time_window;
      acked_bandwidth = static_cast<float>(total_acked_bytes) / time_window;
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
      log_message( "NETWORK: Flow control initialized!\n" );
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
          log_message( "NETWORK: Dropping to bad mode!\n" );
          mode = Bad;
          if ( good_conditions_time < 10.0f && penalty_time < 60.0f )
          {
            penalty_time *= 2.0f;
            if ( penalty_time > 60.0f )
              penalty_time = 60.0f;

            log_message( "NETWORK: Penalty time increased to ", std::to_string(penalty_time), " seconds\n" );
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

          log_message( "NETWORK: Penalty time reduced to ", std::to_string(penalty_time), " seconds\n" );
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
          log_message( "NETWORK: Upgrading to good mode\n" );
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

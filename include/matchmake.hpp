/*
  Biplanes Revival
  Copyright (C) 2019-2025 Regular-dev community
  https://regular-dev.org
  regular.dev.org@gmail.com

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <include/fwd.hpp>
#include <include/enums.hpp>
#include <include/timer.hpp>

#if defined(VITA_PLATFORM)
  #include <lib/Net-vita.h>
#else
  #include <lib/Net.h>
#endif

#include <string>


#define MATCHMAKE_SOCKET_PORT 8008 // 50005 not available on PS Vita
#define MATCHMAKE_MSG_TYPE "type"
#define MATCHMAKE_MSG_PASS "matchpass"
#define MATCHMAKE_MSG_CID  "client_id"
#define MATCHMAKE_SRV_HOSTNAME "regular-dev.org"
#define MATCHMAKE_SRV_IP "194.76.37.102"
#define MATCHMAKE_SRV_PORT 2000
#define MATCH_MAKE_TIMEOUT 15


enum class MatchConnectStatus
{
  FIND = 1000,
  CONNECTED,
  CANNOTCONNECT,
  P2PACCEPT,
  GOODBYE,
  MMSTREAM,
  MMECHO,
  MMOPPONENT,
};

enum class MatchMakerState
{
  IDLE,
  FIND_BEGIN,
  FIND_END,
  MATCH_WAIT,
  MATCH_NAT_PUNCH_0,
  MATCH_NAT_PUNCH_1,
  MATCH_NAT_PUNCH_2,
  MATCH_NAT_PUNCH_3,
  MATCH_READY,
  MATCH_TIMEOUT,
};


class MatchMaker
{
  net::Socket mSocket {};
  std::string mPassword {};
  int32_t mClientId {};

  MatchMakerState mState {};
  Timer mTimer {0.0f};

  net::Address mOpponentAddress {};
  SRV_CLI mClientNodeType {};


public:
  MatchMaker() = default;


  std::string password() const;
  void setPassword( const std::string& );

  bool initNewSession();
  void sendStatus( const MatchConnectStatus, const net::Address to );

  void Update();
  void Reset();

  MatchMakerState state();
  SRV_CLI clientNodeType();
  net::Address opponentAddress();

  static net::Address GetServerAddress();
};

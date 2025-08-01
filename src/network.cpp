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

#include <include/network.hpp>
#include <include/plane.hpp>
#include <include/controls.hpp>
#include <include/game_state.hpp>
#include <include/network_data.hpp>
#include <include/network_state.hpp>
#include <include/variables.hpp>

#if defined(VITA_PLATFORM)
  #include <lib/Net-vita.h>
#else
  #include <lib/Net.h>
#endif

#include <deque>


static std::deque <unsigned char> eventsLocal( 32, 'n' );
static bool eventsTickFinished {true};

static uint8_t eventCounterLocal {};
static uint8_t eventCounterRemote {};

static bool sentGameParams {};


Packet& operator << (
  Packet& packet,
  const Controls& controls )
{
  packet.throttle = controls.throttle;
  packet.pitch    = controls.pitch;

  return packet;
}

Packet& operator << (
  Packet& packet,
  const PlaneNetworkData& data )
{
  packet.x       = data.x;
  packet.y       = data.y;
  packet.dir     = data.dir;
  packet.pilot_x = data.pilot_x;
  packet.pilot_y = data.pilot_y;

  return packet;
}

void
sendDisconnectMessage()
{
  Packet localData {};
  localData.disconnect = true;
  eventsPack(localData);

  uint8_t packet[sizeof(Packet)];
  memcpy( packet, &localData, sizeof(packet) );

  networkState().connection->SendPacket(
    packet, sizeof(packet) );
}

void
eventPush(
  const EVENTS newEvent )
{
  if (  gameState().gameMode != GAME_MODE::HUMAN_VS_HUMAN ||
        networkState().isOpponentConnected == false )
    return;


  if ( eventsTickFinished == true )
  {
    eventsLocal.pop_front();
    eventsLocal.push_back(eventCounterLocal);
    eventsTickFinished = false;

    if ( ++eventCounterLocal >= 64 )
      eventCounterLocal = 0;
  }

  eventsLocal.pop_front();
  eventsLocal.push_back( (unsigned char) newEvent );
}

void
eventsPack(
  Packet& packet )
{
  if ( sentGameParams == false )
  {
    auto& features = gameState().features;

    if ( features.alternativeHitboxes == false )
      eventPush(EVENTS::NO_ALT_HITBOXES);

    if ( features.extraClouds == false )
      eventPush(EVENTS::NO_EXTRA_CLOUDS);

    if ( features.oneShotKills == false )
      eventPush(EVENTS::NO_ONESHOT_KILLS);

    sentGameParams = true;
  }

  for ( uint8_t i = 0; i < sizeof(packet.events); i++ )
    packet.events[i] = eventsLocal.at(i);
}

void
eventsReset()
{
  eventCounterLocal = 0;
  eventCounterRemote = 0;
  eventsTickFinished = true;
  sentGameParams = false;

  eventsLocal.clear();
  eventsLocal.resize( 32, 'n' );
}

void
eventsNewTick()
{
  eventsTickFinished = true;
}

void
processOpponentData(
  const Packet& opponentData,
  const Packet& opponentDataPrev )
{
  PlaneNetworkData data {};
  data.dir = opponentData.dir;
  data.pilot_x = opponentData.pilot_x;
  data.pilot_y = opponentData.pilot_y;

  auto& planeRed = planes.at(PLANE_TYPE::RED);
  auto& planeBlue = planes.at(PLANE_TYPE::BLUE);

  auto& planeLocal =
    planeRed.isLocal() == true
    ? planeRed : planeBlue;

  auto& planeRemote =
    planeRed.isLocal() == false
    ? planeRed : planeBlue;

  planeRemote.setDir( data.dir );
  planeRemote.pilot.setX( data.pilot_x );
  planeRemote.pilot.setY( data.pilot_y );


  const bool eventsChanged = std::equal(
    std::begin(opponentData.events),
    std::end(opponentData.events),
    std::begin(opponentDataPrev.events) ) == false;

  if ( eventsChanged == true )
  {
    bool foundNewEventIndex {};

    for ( uint8_t i = 0;
          i < sizeof(opponentData.events);
          ++i )
    {
      if ( foundNewEventIndex == false )
      {
        if ( opponentData.events[i] == eventCounterRemote )
          foundNewEventIndex = true;

        continue;
      }

      switch ( opponentData.events[i] )
      {
        case (uint8_t) EVENTS::NONE:
        {
          continue;
        }

        case (uint8_t) EVENTS::NO_EXTRA_CLOUDS:
        {
          gameState().features.extraClouds = false;
          continue;
        }

        case (uint8_t) EVENTS::NO_ONESHOT_KILLS:
        {
          gameState().features.oneShotKills = false;
          continue;
        }

        case (uint8_t) EVENTS::NO_ALT_HITBOXES:
        {
          gameState().features.alternativeHitboxes = false;
          continue;
        }

        case (uint8_t) EVENTS::SHOOT:
        {
          planeRemote.input.Shoot();
          continue;
        }

        case (uint8_t) EVENTS::EJECT:
        {
          planeRemote.input.Jump();
          continue;
        }

        case (uint8_t) EVENTS::HIT_PLANE:
        {
          planeRemote.Hit(planeLocal);
          continue;
        }

        case (uint8_t) EVENTS::HIT_CHUTE:
        {
          planeRemote.pilot.ChuteHit(planeLocal);
          continue;
        }

        case (uint8_t) EVENTS::HIT_PILOT:
        {
          planeRemote.pilot.Kill(planeLocal);
          continue;
        }

        case (uint8_t) EVENTS::PLANE_DEATH:
        {
          planeRemote.Crash();
          continue;
        }

        case (uint8_t) EVENTS::PILOT_DEATH:
        {
          planeRemote.pilot.Death();

          planeRemote.ScoreChange(-1);
          planeRemote.mStats.falls++;

          continue;
        }

        case (uint8_t) EVENTS::PLANE_RESPAWN:
        {
          planeRemote.Respawn();
          continue;
        }

        case (uint8_t) EVENTS::PILOT_RESPAWN:
        {
          planeRemote.pilot.Rescue();
          continue;
        }

        case (uint8_t) EVENTS::PILOT_LAND:
        {
          planeRemote.pilot.FallSurvive();
          continue;
        }

        default:
        {
          if ( ++eventCounterRemote >= 64 )
            eventCounterRemote = 0;

          break;
        }
      }
    }

    if ( foundNewEventIndex == true )
    {
      if ( ++eventCounterRemote >= 64 )
        eventCounterRemote = 0;
    }
    else
    {
      const std::string eventBufferStr
      {
        opponentData.events,
        opponentData.events + sizeof(Packet::events)
      };

      const std::string emptyEventBufferStr(
        sizeof(Packet::events), 'n' );

      if ( eventBufferStr != emptyEventBufferStr )
      {
        log_message("NETWORK: Events desynchronization detected!\n");
        log_message("NETWORK: Expected opponent event index " + std::to_string(eventCounterRemote) + "\n");
        log_message("NETWORK: Opponent event buffer: '" + eventBufferStr + "'\n");
      }
    }
  }

  if ( networkState().sendCoordsTimer.isReady() == false )
    return;


  data.x = opponentData.x;
  data.y = opponentData.y;

  planeRemote.setCoords(data);
}

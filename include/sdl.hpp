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

#if !defined(__EMSCRIPTEN__)
  #include <SDL.h>
  #include <SDL_image.h>
  #include <SDL_mixer.h>
#else
  #include <SDL2/SDL.h>
  #include <SDL2/SDL_image.h>
  #include <SDL2/SDL_mixer.h>
#endif

#include <string>


#if !SDL_VERSION_ATLEAST(2, 0, 22)

SDL_FORCE_INLINE SDL_bool SDL_PointInFRect(const SDL_FPoint *p, const SDL_FRect *r)
{
    return ( (p->x >= r->x) && (p->x < (r->x + r->w)) &&
             (p->y >= r->y) && (p->y < (r->y + r->h)) ) ? SDL_TRUE : SDL_FALSE;
}

#endif


extern int DISPLAY_INDEX;

extern SDL_Window* gWindow;
extern SDL_Renderer* gRenderer;
extern SDL_Event windowEvent;


bool SDL_init( const bool enableVSync, const bool enableSound );
void SDL_close();

void show_warning(
  const std::string&,
  const std::string& );

void setVSync( const bool enabled );

SDL_Texture* loadTexture( const std::string& );
Mix_Chunk* loadSound( const std::string& );

int playSound(
  Mix_Chunk* sound,
  const int channel = -1 );

int loopSound(
  Mix_Chunk* sound,
  const int channel );

void panSound(
  const int channel,
  const float pan );

int stopSound( const int channel );

void setSoundVolume( const float normalizedVolume );


void setRenderColor( const Color& );
void queryWindowSize();
void recalculateVirtualScreen();

SDL_FPoint toWindowSpace( const SDL_FPoint& );
float toWindowSpaceX( const float );
float toWindowSpaceY( const float );

SDL_FPoint scaleToScreen( const SDL_FPoint& );
float scaleToScreenX( const float );
float scaleToScreenY( const float );

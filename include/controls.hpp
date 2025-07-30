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

#include <SDL_scancode.h>
#include <SDL_gamecontroller.h>

// Vita-specific gamepad mappings
#ifdef VITA_PLATFORM
struct VitaGamepadState
{
  bool current[SDL_CONTROLLER_BUTTON_MAX] {};
  bool previous[SDL_CONTROLLER_BUTTON_MAX] {};

  VitaGamepadState() = default;
};

extern SDL_GameController* vitaController;
#endif

struct Controls
{
  PLANE_PITCH pitch {};
  PLANE_THROTTLE throttle {};
  bool shoot {};
  bool jump {};

  Controls() = default;
};

struct KeyBindings
{
#ifdef VITA_PLATFORM
  SDL_GameControllerButton throttleUp {};
  SDL_GameControllerButton throttleDown {};
  SDL_GameControllerButton turnLeft {};
  SDL_GameControllerButton turnRight {};
  SDL_GameControllerButton fire {};
  SDL_GameControllerButton jump {};
#else
  SDL_Scancode throttleUp {};
  SDL_Scancode throttleDown {};
  SDL_Scancode turnLeft {};
  SDL_Scancode turnRight {};
  SDL_Scancode fire {};
  SDL_Scancode jump {};
#endif

  KeyBindings() = default;

  void verifyAndFix( const KeyBindings& fallback );
};

namespace bindings
{

extern KeyBindings player1;
extern KeyBindings player2;

#ifdef VITA_PLATFORM
extern VitaGamepadState vitaGamepadState;
#endif

namespace defaults
{

extern const KeyBindings player1;
extern const KeyBindings player2;

} // namespace defaults

} // namespace bindings

void readKeyboardInput();

// Universal input functions
bool isUniversalKeyDown();
bool isUniversalKeyPressed();
bool isUniversalKeyReleased();

#ifdef VITA_PLATFORM
// Vita-specific gamepad functions
void readGamepadInput();
bool isGamepadButtonDown(const SDL_GameControllerButton button);
bool isGamepadButtonPressed(const SDL_GameControllerButton button);
bool isGamepadButtonReleased(const SDL_GameControllerButton button);

// Vita controller management
SDL_GameController* getVitaController();
void setVitaController(SDL_GameController* controller);

// Direct gamepad button functions
bool isButtonDown(const SDL_GameControllerButton button);
bool isButtonPressed(const SDL_GameControllerButton button);
bool isButtonReleased(const SDL_GameControllerButton button);

// Button name function
const char* getButtonName(const SDL_GameControllerButton button);
#endif

bool isKeyDown( const SDL_Scancode );
bool isKeyPressed( const SDL_Scancode );
bool isKeyReleased( const SDL_Scancode );

// Universal input functions that work on all platforms
bool isUniversalKeyDown( const SDL_Scancode key );
bool isUniversalKeyPressed( const SDL_Scancode key );
bool isUniversalKeyReleased( const SDL_Scancode key );

Controls getLocalControls();
Controls getLocalControlsWithBindings(const KeyBindings& bindings);
void processPlaneControls( Plane&, const Controls& );

void assignKeyBinding(
  SDL_Scancode& targetBinding,
  const SDL_Scancode newBinding );

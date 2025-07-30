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

#include <include/controls.hpp>
#include <include/plane.hpp>

#include <SDL_keyboard.h>
#include <SDL_gamecontroller.h>

#ifdef VITA_PLATFORM
#include <psp2/libdbg.h>
#endif

#include <cstring>


namespace bindings
{

KeyBindings player1 = defaults::player1;
KeyBindings player2 = defaults::player2;

#ifdef VITA_PLATFORM
VitaGamepadState vitaGamepadState {};
#endif

namespace defaults
{
  const KeyBindings player1
  {
#ifdef VITA_PLATFORM
    SDL_CONTROLLER_BUTTON_DPAD_UP,    // throttleUp
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,  // throttleDown
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,  // turnLeft
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT, // turnRight
    SDL_CONTROLLER_BUTTON_A,          // fire
    SDL_CONTROLLER_BUTTON_X           // jump
#else
    SDL_SCANCODE_W,
    SDL_SCANCODE_S,
    SDL_SCANCODE_A,
    SDL_SCANCODE_D,
    SDL_SCANCODE_SPACE,
    SDL_SCANCODE_LCTRL
#endif
  };

  const KeyBindings player2
  {
#ifdef VITA_PLATFORM
    SDL_CONTROLLER_BUTTON_DPAD_UP,    // throttleUp
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,  // throttleDown
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,  // turnLeft
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT, // turnRight
    SDL_CONTROLLER_BUTTON_A,          // fire
    SDL_CONTROLLER_BUTTON_X           // jump
#else
    SDL_SCANCODE_UP,
    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_RETURN,
    SDL_SCANCODE_RSHIFT
#endif
  };

#ifdef VITA_PLATFORM
// Vita gamepad state is managed separately
#endif

} // namespace defaults

} // namespace bindings

#ifdef VITA_PLATFORM
// Vita-specific gamepad state
VitaGamepadState vitaGamepadState {};

// Vita controller instance
SDL_GameController* vitaController = nullptr;

// Vita-specific gamepad functions
void readGamepadInput()
{
  if (vitaController == nullptr)
  {
    SCE_DBG_LOG_ERROR("Vita: Controller is null in readGamepadInput");
    return;
  }

  // Update previous state
  for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
  {
    vitaGamepadState.previous[i] = vitaGamepadState.current[i];
  }

  // Read current state
  for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
  {
    bool currentState = SDL_GameControllerGetButton(vitaController, static_cast<SDL_GameControllerButton>(i));
    vitaGamepadState.current[i] = currentState;
    
    // Log state changes for debugging
    if (vitaGamepadState.current[i] != vitaGamepadState.previous[i])
    {
      SCE_DBG_LOG_TRACE("Vita Button %d state changed: %s", i, 
        vitaGamepadState.current[i] ? "PRESSED" : "RELEASED");
    }
  }
}

SDL_GameController* getVitaController()
{
  return vitaController;
}

void setVitaController(SDL_GameController* controller)
{
  vitaController = controller;
}

// Direct gamepad button functions
bool isButtonDown(const SDL_GameControllerButton button)
{
  return vitaGamepadState.current[button];
}

bool isButtonPressed(const SDL_GameControllerButton button)
{
  return vitaGamepadState.current[button] && !vitaGamepadState.previous[button];
}

bool isButtonReleased(const SDL_GameControllerButton button)
{
  return !vitaGamepadState.current[button] && vitaGamepadState.previous[button];
}

// Button name function
const char* getButtonName(const SDL_GameControllerButton button)
{
  switch (button)
  {
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
      return "Up";
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
      return "Down";
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      return "Left";
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      return "Right";
    case SDL_CONTROLLER_BUTTON_A:
      return "Cross";
    case SDL_CONTROLLER_BUTTON_X:
      return "Square";
    case SDL_CONTROLLER_BUTTON_Y:
      return "Triangle";
    case SDL_CONTROLLER_BUTTON_B:
      return "Circle";
    case SDL_CONTROLLER_BUTTON_START:
      return "Start";
    case SDL_CONTROLLER_BUTTON_BACK:
      return "Select";
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
      return "L1";
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
      return "R1";
    default:
      return "Unknown";
  }
}
#endif

struct
{
  Uint8 current[SDL_NUM_SCANCODES] {};
  decltype(current) previous {};

} static keyboardState {};


void
KeyBindings::verifyAndFix(
  const KeyBindings& fallback )
{
#ifdef VITA_PLATFORM
  // For Vita, we don't need to check bounds since SDL_GameControllerButton is an enum
  // Just verify that the buttons are valid
  if ( fire == SDL_CONTROLLER_BUTTON_INVALID )
    fire = fallback.fire;
  if ( jump == SDL_CONTROLLER_BUTTON_INVALID )
    jump = fallback.jump;
  if ( throttleDown == SDL_CONTROLLER_BUTTON_INVALID )
    throttleDown = fallback.throttleDown;
  if ( throttleUp == SDL_CONTROLLER_BUTTON_INVALID )
    throttleUp = fallback.throttleUp;
  if ( turnLeft == SDL_CONTROLLER_BUTTON_INVALID )
    turnLeft = fallback.turnLeft;
  if ( turnRight == SDL_CONTROLLER_BUTTON_INVALID )
    turnRight = fallback.turnRight;
#else
  // Original keyboard logic for non-Vita platforms
  if ( fire >= SDL_NUM_SCANCODES )
    fire = fallback.fire;
  if ( jump >= SDL_NUM_SCANCODES )
    jump = fallback.jump;
  if ( throttleDown >= SDL_NUM_SCANCODES )
    throttleDown = fallback.throttleDown;
  if ( throttleUp >= SDL_NUM_SCANCODES )
    throttleUp = fallback.throttleUp;
  if ( turnLeft >= SDL_NUM_SCANCODES )
    turnLeft = fallback.turnLeft;
  if ( turnRight >= SDL_NUM_SCANCODES )
    turnRight = fallback.turnRight;
#endif
}

void
readKeyboardInput()
{
  std::memcpy(
    keyboardState.previous,
    keyboardState.current,
    sizeof(keyboardState.previous) );

  std::memcpy(
    keyboardState.current,
    SDL_GetKeyboardState({}),
    sizeof(keyboardState.current) );
}

bool
isKeyDown(
  const SDL_Scancode key )
{
  return keyboardState.current[key] == true;
}

bool
isKeyPressed(
  const SDL_Scancode key )
{
  return
    isKeyDown(key) == true &&
    keyboardState.previous[key] == false;
}

bool
isKeyReleased(
  const SDL_Scancode key )
{
  return
    isKeyDown(key) == false &&
    keyboardState.previous[key] == true;
}

// Universal input functions
bool isUniversalKeyDown(const SDL_Scancode key)
{
#ifdef VITA_PLATFORM
  return isKeyDown(key);  // Only keyboard for Vita, no emulation needed
#else
  return isKeyDown(key);
#endif
}

bool isUniversalKeyPressed(const SDL_Scancode key)
{
#ifdef VITA_PLATFORM
  return isKeyPressed(key);  // Only keyboard for Vita, no emulation needed
#else
  return isKeyPressed(key);
#endif
}

bool isUniversalKeyReleased(const SDL_Scancode key)
{
#ifdef VITA_PLATFORM
  return isKeyReleased(key);  // Only keyboard for Vita, no emulation needed
#else
  return isKeyReleased(key);
#endif
}


void
assignKeyBinding(
  SDL_Scancode& targetBinding,
  const SDL_Scancode newBinding )
{
#ifdef VITA_PLATFORM
  // For Vita, we don't use this function since we work directly with SDL_GameControllerButton
  // This function is only used for non-Vita platforms
  targetBinding = newBinding;
#else
  // Original logic for non-Vita platforms
  if ( false )
    ;
  else if ( newBinding == bindings::player1.throttleUp )
    bindings::player1.throttleUp = targetBinding;
  else if ( newBinding == bindings::player2.throttleUp )
    bindings::player2.throttleUp = targetBinding;
  else if ( newBinding == bindings::player1.throttleDown )
    bindings::player1.throttleDown = targetBinding;
  else if ( newBinding == bindings::player2.throttleDown )
    bindings::player2.throttleDown = targetBinding;
  else if ( newBinding == bindings::player1.turnLeft )
    bindings::player1.turnLeft = targetBinding;
  else if ( newBinding == bindings::player2.turnLeft )
    bindings::player2.turnLeft = targetBinding;
  else if ( newBinding == bindings::player1.turnRight )
    bindings::player1.turnRight = targetBinding;
  else if ( newBinding == bindings::player2.turnRight )
    bindings::player2.turnRight = targetBinding;
  else if ( newBinding == bindings::player1.fire )
    bindings::player1.fire = targetBinding;
  else if ( newBinding == bindings::player2.fire )
    bindings::player2.fire = targetBinding;
  else if ( newBinding == bindings::player1.jump )
    bindings::player1.jump = targetBinding;
  else if ( newBinding == bindings::player2.jump )
    bindings::player2.jump = targetBinding;

  targetBinding = newBinding;
#endif
}

Controls getLocalControls()
{
  Controls controls {};

#ifdef VITA_PLATFORM
  // Use direct gamepad button functions
  if (isButtonDown(bindings::player1.throttleUp) && !isButtonDown(bindings::player1.throttleDown))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_INCREASE;
    SCE_DBG_LOG_TRACE("Vita: Throttle UP");
  }
  else if (isButtonDown(bindings::player1.throttleDown) && !isButtonDown(bindings::player1.throttleUp))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_DECREASE;
    SCE_DBG_LOG_TRACE("Vita: Throttle DOWN");
  }
  else
    controls.throttle = PLANE_THROTTLE::THROTTLE_IDLE;

  if (isButtonDown(bindings::player1.turnLeft) && !isButtonDown(bindings::player1.turnRight))
  {
    controls.pitch = PLANE_PITCH::PITCH_LEFT;
    SCE_DBG_LOG_TRACE("Vita: Turn LEFT");
  }
  else if (isButtonDown(bindings::player1.turnRight) && !isButtonDown(bindings::player1.turnLeft))
  {
    controls.pitch = PLANE_PITCH::PITCH_RIGHT;
    SCE_DBG_LOG_TRACE("Vita: Turn RIGHT");
  }
  else
    controls.pitch = PLANE_PITCH::PITCH_IDLE;

  // SHOOT
  if (isButtonDown(bindings::player1.fire))
  {
    controls.shoot = true;
    SCE_DBG_LOG_TRACE("Vita: FIRE");
  }
  else
    controls.shoot = false;

  // EJECT
  if (isButtonDown(bindings::player1.jump))
  {
    controls.jump = true;
    SCE_DBG_LOG_TRACE("Vita: JUMP");
  }
  else
    controls.jump = false;
#else
  // Original keyboard logic for non-Vita platforms
  if (isUniversalKeyDown(bindings::player1.throttleUp) && !isUniversalKeyDown(bindings::player1.throttleDown))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_INCREASE;
  }
  else if (isUniversalKeyDown(bindings::player1.throttleDown) && !isUniversalKeyDown(bindings::player1.throttleUp))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_DECREASE;
  }
  else
    controls.throttle = PLANE_THROTTLE::THROTTLE_IDLE;

  if (isUniversalKeyDown(bindings::player1.turnLeft) && !isUniversalKeyDown(bindings::player1.turnRight))
  {
    controls.pitch = PLANE_PITCH::PITCH_LEFT;
  }
  else if (isUniversalKeyDown(bindings::player1.turnRight) && !isUniversalKeyDown(bindings::player1.turnLeft))
  {
    controls.pitch = PLANE_PITCH::PITCH_RIGHT;
  }
  else
    controls.pitch = PLANE_PITCH::PITCH_IDLE;

  // SHOOT
  if (isUniversalKeyDown(bindings::player1.fire))
  {
    controls.shoot = true;
  }
  else
    controls.shoot = false;

  // EJECT
  if (isUniversalKeyDown(bindings::player1.jump))
  {
    controls.jump = true;
  }
  else
    controls.jump = false;
#endif

  return controls;
}

Controls getLocalControlsWithBindings(const KeyBindings& bindings)
{
  Controls controls {};

#ifdef VITA_PLATFORM
  // Use direct gamepad button functions with provided bindings
  if (isButtonDown(bindings.throttleUp) && !isButtonDown(bindings.throttleDown))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_INCREASE;
    SCE_DBG_LOG_TRACE("Vita: Throttle UP");
  }
  else if (isButtonDown(bindings.throttleDown) && !isButtonDown(bindings.throttleUp))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_DECREASE;
    SCE_DBG_LOG_TRACE("Vita: Throttle DOWN");
  }
  else
    controls.throttle = PLANE_THROTTLE::THROTTLE_IDLE;

  if (isButtonDown(bindings.turnLeft) && !isButtonDown(bindings.turnRight))
  {
    controls.pitch = PLANE_PITCH::PITCH_LEFT;
    SCE_DBG_LOG_TRACE("Vita: Turn LEFT");
  }
  else if (isButtonDown(bindings.turnRight) && !isButtonDown(bindings.turnLeft))
  {
    controls.pitch = PLANE_PITCH::PITCH_RIGHT;
    SCE_DBG_LOG_TRACE("Vita: Turn RIGHT");
  }
  else
    controls.pitch = PLANE_PITCH::PITCH_IDLE;

  // SHOOT
  if (isButtonDown(bindings.fire))
  {
    controls.shoot = true;
    SCE_DBG_LOG_TRACE("Vita: FIRE");
  }
  else
    controls.shoot = false;

  // EJECT
  if (isButtonDown(bindings.jump))
  {
    controls.jump = true;
    SCE_DBG_LOG_TRACE("Vita: JUMP");
  }
  else
    controls.jump = false;
#else
  // Original keyboard logic for non-Vita platforms
  if (isUniversalKeyDown(bindings.throttleUp) && !isUniversalKeyDown(bindings.throttleDown))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_INCREASE;
  }
  else if (isUniversalKeyDown(bindings.throttleDown) && !isUniversalKeyDown(bindings.throttleUp))
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_DECREASE;
  }
  else
    controls.throttle = PLANE_THROTTLE::THROTTLE_IDLE;

  if (isUniversalKeyDown(bindings.turnLeft) && !isUniversalKeyDown(bindings.turnRight))
  {
    controls.pitch = PLANE_PITCH::PITCH_LEFT;
  }
  else if (isUniversalKeyDown(bindings.turnRight) && !isUniversalKeyDown(bindings.turnLeft))
  {
    controls.pitch = PLANE_PITCH::PITCH_RIGHT;
  }
  else
    controls.pitch = PLANE_PITCH::PITCH_IDLE;

  // SHOOT
  if (isUniversalKeyDown(bindings.fire))
  {
    controls.shoot = true;
  }
  else
    controls.shoot = false;

  // EJECT
  if (isUniversalKeyDown(bindings.jump))
  {
    controls.jump = true;
  }
  else
    controls.jump = false;
#endif

  return controls;
}


void
processPlaneControls(
  Plane& plane,
  const Controls& controls )
{
  if ( controls.throttle == PLANE_THROTTLE::THROTTLE_INCREASE )
    plane.input.Accelerate();

  else if ( controls.throttle == PLANE_THROTTLE::THROTTLE_DECREASE )
    plane.input.Decelerate();


  if ( controls.pitch == PLANE_PITCH::PITCH_LEFT )
    plane.input.TurnLeft();

  else if ( controls.pitch == PLANE_PITCH::PITCH_RIGHT )
    plane.input.TurnRight();

  else
    plane.input.TurnIdle();


  if ( controls.shoot == true )
    plane.input.Shoot();

  if ( controls.jump == true )
    plane.input.Jump();
  else
    plane.pilot.ChuteUnlock();
}

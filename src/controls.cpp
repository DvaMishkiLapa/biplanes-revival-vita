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
VitaGamepadBindings vitaGamepad = defaults::vitaGamepad;
VitaGamepadState vitaGamepadState {};
#endif

namespace defaults
{

const KeyBindings player1
{
//  THROTTLE_UP
  SDL_SCANCODE_UP,

//  THROTTLE_DOWN
  SDL_SCANCODE_DOWN,

//  TURN_LEFT
  SDL_SCANCODE_LEFT,

//  TURN_RIGHT
  SDL_SCANCODE_RIGHT,

//  FIRE
  SDL_SCANCODE_SPACE,

//  JUMP
  SDL_SCANCODE_LCTRL,
};

const KeyBindings player2
{
//  THROTTLE_UP
  SDL_SCANCODE_I,

//  THROTTLE_DOWN
  SDL_SCANCODE_K,

//  TURN_LEFT
  SDL_SCANCODE_J,

//  TURN_RIGHT
  SDL_SCANCODE_L,

//  FIRE
  SDL_SCANCODE_E,

//  JUMP
  SDL_SCANCODE_Q,
};

#ifdef VITA_PLATFORM
const VitaGamepadBindings vitaGamepad
{
//  THROTTLE_UP - D-pad Up
  SDL_CONTROLLER_BUTTON_DPAD_UP,

//  THROTTLE_DOWN - D-pad Down
  SDL_CONTROLLER_BUTTON_DPAD_DOWN,

//  TURN_LEFT - D-pad Left
  SDL_CONTROLLER_BUTTON_DPAD_LEFT,

//  TURN_RIGHT - D-pad Right
  SDL_CONTROLLER_BUTTON_DPAD_RIGHT,

//  FIRE - X button (Cross)
  SDL_CONTROLLER_BUTTON_A,

//  JUMP - Square button
  SDL_CONTROLLER_BUTTON_X,
};
#endif

} // namespace defaults

} // namespace bindings

#ifdef VITA_PLATFORM
SDL_GameController* vitaController = nullptr;
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

#ifdef VITA_PLATFORM
void
readGamepadInput()
{
  // Copy previous state
  std::memcpy(
    bindings::vitaGamepadState.previous,
    bindings::vitaGamepadState.current,
    sizeof(bindings::vitaGamepadState.previous) );

  // Update current state - Vita has built-in gamepad
  SDL_GameController* controller = getVitaController();
  if (controller)
  {
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
    {
      bool buttonState = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(i)) != 0;
      bindings::vitaGamepadState.current[i] = buttonState;
      
      // Log important button states for debugging
      if (i == SDL_CONTROLLER_BUTTON_DPAD_UP || 
          i == SDL_CONTROLLER_BUTTON_DPAD_DOWN ||
          i == SDL_CONTROLLER_BUTTON_DPAD_LEFT ||
          i == SDL_CONTROLLER_BUTTON_DPAD_RIGHT ||
          i == SDL_CONTROLLER_BUTTON_A ||
          i == SDL_CONTROLLER_BUTTON_X)
      {
        if (buttonState != bindings::vitaGamepadState.previous[i])
        {
          SCE_DBG_LOG_TRACE("Vita Button %d state changed: %s", i, buttonState ? "PRESSED" : "RELEASED");
        }
      }
    }
  }
  else
  {
    // Reset all buttons if no controller
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
    {
      bindings::vitaGamepadState.current[i] = false;
    }
    SCE_DBG_LOG_ERROR("Vita controller not available!");
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

// Vita key emulation - map gamepad buttons to keyboard keys
bool isVitaKeyPressed(const SDL_Scancode key)
{
  switch (key)
  {
    case SDL_SCANCODE_UP:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP);
    case SDL_SCANCODE_DOWN:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    case SDL_SCANCODE_LEFT:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    case SDL_SCANCODE_RIGHT:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_SPACE:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_A);
    case SDL_SCANCODE_ESCAPE:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_START);
    case SDL_SCANCODE_F1:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_Y);
    case SDL_SCANCODE_DELETE:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_X);
    case SDL_SCANCODE_W:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP);
    case SDL_SCANCODE_S:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    case SDL_SCANCODE_A:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    case SDL_SCANCODE_D:
      return isGamepadButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    default:
      return false;
  }
}

bool isVitaKeyDown(const SDL_Scancode key)
{
  switch (key)
  {
    case SDL_SCANCODE_UP:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_UP);
    case SDL_SCANCODE_DOWN:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    case SDL_SCANCODE_LEFT:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    case SDL_SCANCODE_RIGHT:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_SPACE:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_A);
    case SDL_SCANCODE_ESCAPE:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_START);
    case SDL_SCANCODE_F1:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_Y);
    case SDL_SCANCODE_DELETE:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_X);
    case SDL_SCANCODE_W:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_UP);
    case SDL_SCANCODE_S:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    case SDL_SCANCODE_A:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    case SDL_SCANCODE_D:
      return isGamepadButtonDown(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    default:
      return false;
  }
}

bool isGamepadButtonDown(const SDL_GameControllerButton button)
{
  return bindings::vitaGamepadState.current[button] == true;
}

bool isGamepadButtonPressed(const SDL_GameControllerButton button)
{
  return isGamepadButtonDown(button) == true &&
         bindings::vitaGamepadState.previous[button] == false;
}

bool isGamepadButtonReleased(const SDL_GameControllerButton button)
{
  return isGamepadButtonDown(button) == false &&
         bindings::vitaGamepadState.previous[button] == true;
}
#endif

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
  return isKeyDown(key) || isVitaKeyDown(key);
#else
  return isKeyDown(key);
#endif
}

bool isUniversalKeyPressed(const SDL_Scancode key)
{
#ifdef VITA_PLATFORM
  return isKeyPressed(key) || isVitaKeyPressed(key);
#else
  return isKeyPressed(key);
#endif
}

bool isUniversalKeyReleased(const SDL_Scancode key)
{
#ifdef VITA_PLATFORM
  return isKeyReleased(key) || isGamepadButtonReleased(SDL_CONTROLLER_BUTTON_DPAD_UP); // Simplified for now
#else
  return isKeyReleased(key);
#endif
}


void
assignKeyBinding(
  SDL_Scancode& targetBinding,
  const SDL_Scancode newBinding )
{
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
}

Controls
getLocalControls(
  const KeyBindings& bindings )
{
  Controls controls {};

#ifdef VITA_PLATFORM
  // Use gamepad input for Vita
  if (  isGamepadButtonDown(bindings::vitaGamepad.throttleUp) == true &&
        isGamepadButtonDown(bindings::vitaGamepad.throttleDown) == false )
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_INCREASE;
    SCE_DBG_LOG_TRACE("Vita: Throttle UP");
  }
  else if ( isGamepadButtonDown(bindings::vitaGamepad.throttleDown) == true &&
            isGamepadButtonDown(bindings::vitaGamepad.throttleUp) == false )
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_DECREASE;
    SCE_DBG_LOG_TRACE("Vita: Throttle DOWN");
  }
  else
    controls.throttle = PLANE_THROTTLE::THROTTLE_IDLE;

  if (  isGamepadButtonDown(bindings::vitaGamepad.turnLeft) == true &&
        isGamepadButtonDown(bindings::vitaGamepad.turnRight) == false )
  {
    controls.pitch = PLANE_PITCH::PITCH_LEFT;
    SCE_DBG_LOG_TRACE("Vita: Turn LEFT");
  }
  else if ( isGamepadButtonDown(bindings::vitaGamepad.turnRight) == true &&
            isGamepadButtonDown(bindings::vitaGamepad.turnLeft) == false )
  {
    controls.pitch = PLANE_PITCH::PITCH_RIGHT;
    SCE_DBG_LOG_TRACE("Vita: Turn RIGHT");
  }
  else
    controls.pitch = PLANE_PITCH::PITCH_IDLE;

  // SHOOT
  if ( isGamepadButtonDown(bindings::vitaGamepad.fire) == true )
  {
    controls.shoot = true;
    SCE_DBG_LOG_TRACE("Vita: FIRE");
  }
  else
    controls.shoot = false;

  // EJECT
  if ( isGamepadButtonDown(bindings::vitaGamepad.jump) == true )
  {
    controls.jump = true;
    SCE_DBG_LOG_TRACE("Vita: JUMP");
  }
  else
    controls.jump = false;

#else
  // Use keyboard input for other platforms
  if (  isKeyDown(bindings.throttleUp) == true &&
        isKeyDown(bindings.throttleDown) == false )
    controls.throttle = PLANE_THROTTLE::THROTTLE_INCREASE;

  else if ( isKeyDown(bindings.throttleDown) == true &&
            isKeyDown(bindings.throttleUp) == false )
    controls.throttle = PLANE_THROTTLE::THROTTLE_DECREASE;

  else
    controls.throttle = PLANE_THROTTLE::THROTTLE_IDLE;

  if (  isKeyDown(bindings.turnLeft) == true &&
        isKeyDown(bindings.turnRight) == false )
    controls.pitch = PLANE_PITCH::PITCH_LEFT;

  else if ( isKeyDown(bindings.turnRight) == true &&
            isKeyDown(bindings.turnLeft) == false )
    controls.pitch = PLANE_PITCH::PITCH_RIGHT;

  else
    controls.pitch = PLANE_PITCH::PITCH_IDLE;

  // SHOOT
  if ( isKeyDown(bindings.fire) == true )
    controls.shoot = true;
  else
    controls.shoot = false;

  // EJECT
  if ( isKeyDown(bindings.jump) == true )
    controls.jump = true;
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

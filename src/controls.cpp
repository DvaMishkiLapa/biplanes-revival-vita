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
// Vita gamepad state is managed separately
#endif

} // namespace defaults

} // namespace bindings

#ifdef VITA_PLATFORM
SDL_GameController* vitaController = nullptr;

// Vita key to gamepad button mapping
static SDL_GameControllerButton vitaKeyMap[SDL_NUM_SCANCODES];

// Initialize the mapping array at module load time
static void initVitaKeyMap()
{
  // Initialize all to invalid button
  for (int i = 0; i < SDL_NUM_SCANCODES; ++i)
  {
    vitaKeyMap[i] = SDL_CONTROLLER_BUTTON_INVALID;
  }
  
  // Set up the mappings
  vitaKeyMap[SDL_SCANCODE_UP] = SDL_CONTROLLER_BUTTON_DPAD_UP;
  vitaKeyMap[SDL_SCANCODE_DOWN] = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
  vitaKeyMap[SDL_SCANCODE_LEFT] = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
  vitaKeyMap[SDL_SCANCODE_RIGHT] = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
  vitaKeyMap[SDL_SCANCODE_RETURN] = SDL_CONTROLLER_BUTTON_A;
  vitaKeyMap[SDL_SCANCODE_SPACE] = SDL_CONTROLLER_BUTTON_A;
  vitaKeyMap[SDL_SCANCODE_ESCAPE] = SDL_CONTROLLER_BUTTON_START;
  vitaKeyMap[SDL_SCANCODE_F1] = SDL_CONTROLLER_BUTTON_BACK;
  vitaKeyMap[SDL_SCANCODE_DELETE] = SDL_CONTROLLER_BUTTON_Y;  // Changed from X to Y to avoid conflict
  vitaKeyMap[SDL_SCANCODE_W] = SDL_CONTROLLER_BUTTON_DPAD_UP;
  vitaKeyMap[SDL_SCANCODE_S] = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
  vitaKeyMap[SDL_SCANCODE_A] = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
  vitaKeyMap[SDL_SCANCODE_D] = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
  
  // Player 1 controls
  vitaKeyMap[SDL_SCANCODE_LCTRL] = SDL_CONTROLLER_BUTTON_X;  // Jump/Catapult for player 1
  
  // Additional buttons
  vitaKeyMap[SDL_SCANCODE_LSHIFT] = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;  // L1
  vitaKeyMap[SDL_SCANCODE_RSHIFT] = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;  // R1
}

// Helper function to get gamepad button for a key
static SDL_GameControllerButton getVitaGamepadButton(const SDL_Scancode key)
{
  // Check bounds for safety
  if (key < 0 || key >= SDL_NUM_SCANCODES)
    return SDL_CONTROLLER_BUTTON_INVALID;
    
  return vitaKeyMap[key];
}

// Helper function to get Vita button name for a scancode
const char* getVitaButtonName(const SDL_Scancode key)
{
#ifdef VITA_PLATFORM
  SDL_GameControllerButton button = getVitaGamepadButton(key);
  
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
      return SDL_GetScancodeName(key);
  }
#else
  return SDL_GetScancodeName(key);
#endif
}

// Initialize the mapping at module load
static struct VitaKeyMapInitializer {
  VitaKeyMapInitializer() { initVitaKeyMap(); }
} vitaKeyMapInitializer;
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
  SDL_GameControllerButton button = getVitaGamepadButton(key);
  if (button == SDL_CONTROLLER_BUTTON_INVALID)
    return false;
  return isGamepadButtonPressed(button);
}

bool isVitaKeyDown(const SDL_Scancode key)
{
  SDL_GameControllerButton button = getVitaGamepadButton(key);
  if (button == SDL_CONTROLLER_BUTTON_INVALID)
    return false;
  return isGamepadButtonDown(button);
}

bool isVitaKeyReleased(const SDL_Scancode key)
{
  SDL_GameControllerButton button = getVitaGamepadButton(key);
  if (button == SDL_CONTROLLER_BUTTON_INVALID)
    return false;
  return isGamepadButtonReleased(button);
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
  return isKeyReleased(key) || isVitaKeyReleased(key);
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

  // Use universal input functions for all platforms
  if (  isUniversalKeyDown(bindings.throttleUp) == true &&
        isUniversalKeyDown(bindings.throttleDown) == false )
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_INCREASE;
#ifdef VITA_PLATFORM
    SCE_DBG_LOG_TRACE("Vita: Throttle UP");
#endif
  }
  else if ( isUniversalKeyDown(bindings.throttleDown) == true &&
            isUniversalKeyDown(bindings.throttleUp) == false )
  {
    controls.throttle = PLANE_THROTTLE::THROTTLE_DECREASE;
#ifdef VITA_PLATFORM
    SCE_DBG_LOG_TRACE("Vita: Throttle DOWN");
#endif
  }
  else
    controls.throttle = PLANE_THROTTLE::THROTTLE_IDLE;

  if (  isUniversalKeyDown(bindings.turnLeft) == true &&
        isUniversalKeyDown(bindings.turnRight) == false )
  {
    controls.pitch = PLANE_PITCH::PITCH_LEFT;
#ifdef VITA_PLATFORM
    SCE_DBG_LOG_TRACE("Vita: Turn LEFT");
#endif
  }
  else if ( isUniversalKeyDown(bindings.turnRight) == true &&
            isUniversalKeyDown(bindings.turnLeft) == false )
  {
    controls.pitch = PLANE_PITCH::PITCH_RIGHT;
#ifdef VITA_PLATFORM
    SCE_DBG_LOG_TRACE("Vita: Turn RIGHT");
#endif
  }
  else
    controls.pitch = PLANE_PITCH::PITCH_IDLE;

  // SHOOT
  if ( isUniversalKeyDown(bindings.fire) == true )
  {
    controls.shoot = true;
#ifdef VITA_PLATFORM
    SCE_DBG_LOG_TRACE("Vita: FIRE");
#endif
  }
  else
    controls.shoot = false;

  // EJECT
  if ( isUniversalKeyDown(bindings.jump) == true )
  {
    controls.jump = true;
#ifdef VITA_PLATFORM
    SCE_DBG_LOG_TRACE("Vita: JUMP");
#endif
  }
  else
    controls.jump = false;

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

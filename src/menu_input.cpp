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

#include <include/menu.hpp>
#include <include/sdl.hpp>
#include <include/constants.hpp>
#include <include/game_state.hpp>
#include <include/network_state.hpp>
#include <include/network_data.hpp>
#include <include/ai_stuff.hpp>
#include <include/controls.hpp>
#include <include/variables.hpp>
#include <include/utility.hpp>

#ifdef VITA_PLATFORM
#include <SDL_gamecontroller.h>
#include <psp2/libdbg.h>
#endif


void
Menu::UpdateControls()
{
  auto& game = gameState();

#if defined(BIPLANES_STEP_DEBUGGING_ENABLED)
  game.debug.stepByStepMode = isUniversalKeyDown(SDL_SCANCODE_Q);

  if ( isUniversalKeyPressed(SDL_SCANCODE_E) == true )
      game.debug.advanceOneTick = true;
#endif


  if ( mCurrentRoom == ROOMS::GAME )
  {
#ifdef VITA_PLATFORM
    // Vita gamepad controls for game
    if (isButtonPressed(SDL_CONTROLLER_BUTTON_START))
      Select();  // This will trigger pause menu
    else if (isButtonPressed(SDL_CONTROLLER_BUTTON_B))
      GoBack();
#else
    // Original keyboard controls for game
    if ( isUniversalKeyPressed(SDL_SCANCODE_ESCAPE) == true )
      GoBack();
#endif

    return;
  }


  if ( mIsTyping == true )
  {
    UpdateTyping();

#ifdef VITA_PLATFORM
    if (isButtonPressed(SDL_CONTROLLER_BUTTON_A))
      Select();
#else
    if ( isUniversalKeyPressed(SDL_SCANCODE_RETURN) == true )
      Select();
#endif

    return;
  }

  else if ( mIsEditingSlider == true )
  {
    UpdateSliderEditing();

#ifdef VITA_PLATFORM
    if (isButtonPressed(SDL_CONTROLLER_BUTTON_A))
      Select();
#else
    if ( isUniversalKeyPressed(SDL_SCANCODE_RETURN) == true )
      Select();
#endif

    return;
  }

  else if ( mIsDefiningKey == true )
    return UpdateDefiningKey();

  else
  {
    auto& game = gameState();

#ifdef VITA_PLATFORM
    // Vita gamepad navigation
    if (isButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN))
      MenuItemNext();
    else if (isButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP))
      MenuItemPrevious();
    else if (isButtonPressed(SDL_CONTROLLER_BUTTON_B))
      GoBack();
    else if (isButtonPressed(SDL_CONTROLLER_BUTTON_A))
      Select();
    else if (isButtonPressed(SDL_CONTROLLER_BUTTON_Y))
      ResetKey();
    else if (isButtonPressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
      ToggleDefiningKey(static_cast<MENU_SETTINGS_CONTROLS>(mSelectedItem));
    else if (isButtonPressed(SDL_CONTROLLER_BUTTON_START))
    {
      // Exit to main menu or pause
      if (mCurrentRoom == ROOMS::GAME)
        GoBack();
      else
        GoBack();
    }
#else
    // Original keyboard navigation
    if (  isUniversalKeyPressed(SDL_SCANCODE_DOWN) == true ||
          isUniversalKeyPressed(SDL_SCANCODE_S) == true )
      MenuItemNext();

    else if ( isUniversalKeyPressed(SDL_SCANCODE_UP) == true ||
              isUniversalKeyPressed(SDL_SCANCODE_W) == true )
      MenuItemPrevious();

    else if ( isUniversalKeyPressed(SDL_SCANCODE_ESCAPE) == true ||
              isUniversalKeyPressed(SDL_SCANCODE_LEFT) == true ||
              isUniversalKeyPressed(SDL_SCANCODE_A) == true )
      GoBack();

    else if ( isUniversalKeyPressed(SDL_SCANCODE_DELETE) == true )
      ResetKey();

    else if ( isUniversalKeyPressed(SDL_SCANCODE_RETURN) == true ||
              isUniversalKeyPressed(SDL_SCANCODE_SPACE) == true ||
              isUniversalKeyPressed(SDL_SCANCODE_RIGHT) == true ||
              isUniversalKeyPressed(SDL_SCANCODE_D) == true )
      Select();

    else if ( isUniversalKeyPressed(SDL_SCANCODE_F1) == true )
      ToggleDefiningKey(static_cast<MENU_SETTINGS_CONTROLS>(mSelectedItem));
#endif
  }

  // F1 for stats (non-Vita only)
#ifndef VITA_PLATFORM
  if (isUniversalKeyPressed(SDL_SCANCODE_F1) == true && mCurrentRoom == ROOMS::MENU_MAIN)
  {
    setMessage(MESSAGE_TYPE::NONE);
    ChangeRoom(ROOMS::MENU_RECENT_STATS_PAGE1);

    auto& stats = game.stats;
    calcDerivedStats(stats.recent[PLANE_TYPE::RED]);
    calcDerivedStats(stats.recent[PLANE_TYPE::BLUE]);
    calcDerivedStats(stats.total);
  }
#endif

#ifdef VITA_PLATFORM
  // Vita F1 equivalent - Select button for stats
  if (isButtonPressed(SDL_CONTROLLER_BUTTON_BACK) && mCurrentRoom == ROOMS::MENU_MAIN)
  {
    setMessage(MESSAGE_TYPE::NONE);
    ChangeRoom(ROOMS::MENU_RECENT_STATS_PAGE1);

    auto& stats = game.stats;
    calcDerivedStats(stats.recent[PLANE_TYPE::RED]);
    calcDerivedStats(stats.recent[PLANE_TYPE::BLUE]);
    calcDerivedStats(stats.total);
  }
#endif
}

void
Menu::ToggleTyping(
  const MENU_SPECIFY varToSpecify )
{
  if ( mIsTyping == true )
    return EndTyping(varToSpecify);

#ifdef VITA_PLATFORM
  // For PS Vita, we need to handle text input differently
  // Clear the current input field and prepare for new input
  switch (varToSpecify)
  {
    case MENU_SPECIFY::IP:
      mInputIp.clear();
      break;
    case MENU_SPECIFY::PORT:
      if ( mCurrentRoom == ROOMS::MENU_MP_DC_HOST )
        mInputPortHost.clear();
      else
        mInputPortClient.clear();
      break;
    case MENU_SPECIFY::PASSWORD:
      mInputPassword.clear();
      break;
    default:
      break;
  }
#endif

  mIsTyping = true;
  mSpecifyingVarState[varToSpecify] = true;
  SDL_StartTextInput();
}

void
Menu::EndTyping(
  const MENU_SPECIFY varToSpecify )
{
  mIsTyping = false;
  mSpecifyingVarState[varToSpecify] = false;
  SDL_StopTextInput();


  switch (varToSpecify)
  {
    case MENU_SPECIFY::IP:
    {
      SERVER_IP = checkIp(mInputIp);

      if ( SERVER_IP.empty() == true )
        SERVER_IP = DEFAULT_SERVER_IP;

      mInputIp = SERVER_IP;

      break;
    }

    case MENU_SPECIFY::PORT:
    {
      if ( mCurrentRoom == ROOMS::MENU_MP_DC_HOST )
      {
        if ( checkPort(mInputPortHost) == true )
          LOCAL_PORT = std::stoi(mInputPortHost);
        else
        {
          LOCAL_PORT = DEFAULT_LOCAL_PORT;
          mInputPortHost = std::to_string(LOCAL_PORT);
        }

        break;
      }

      if ( checkPort(mInputPortClient) == true )
        REMOTE_PORT = std::stoi(mInputPortClient);
      else
      {
        REMOTE_PORT = DEFAULT_REMOTE_PORT;
        mInputPortClient = std::to_string(REMOTE_PORT);
      }

      break;
    }

    case MENU_SPECIFY::PASSWORD:
    {
      if ( checkPass(mInputPassword) == true )
        MMAKE_PASSWORD = mInputPassword;
      else
        MMAKE_PASSWORD = {};

      break;
    }

    default:
      break;
  }

  settingsWrite();
}

void
Menu::UpdateTyping()
{
  const auto maxInputFieldTextLength = constants::menu::maxInputFieldTextLength;


  if ( mSpecifyingVarState[MENU_SPECIFY::IP] == true )
  {
#ifdef VITA_PLATFORM
    // On PS Vita, handle text input more carefully
    if ( windowEvent.type == SDL_TEXTINPUT )
    {
      // Clear the field first if it's empty (to avoid appending to old content)
      if ( mInputIp.empty() )
      {
        mInputIp.clear();
      }
      
      if ( mInputIp.length() < maxInputFieldTextLength )
      {
        for ( const auto& digit : windowEvent.text.text )
          if ( std::isdigit(digit) == true || digit == '.' )
            mInputIp += digit;
          else
            break;
      }
    }
#else
    if ( windowEvent.type == SDL_KEYDOWN )
    {
      if ( windowEvent.key.keysym.sym == SDLK_BACKSPACE && mInputIp.length() > 0 )
        mInputIp.pop_back();

      else if ( windowEvent.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL )
        SDL_SetClipboardText( mInputIp.c_str() );

      else if ( windowEvent.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL )
        if ( checkIp( SDL_GetClipboardText() ).empty() == false )
          mInputIp = SDL_GetClipboardText();
    }

    if ( windowEvent.type == SDL_TEXTINPUT )
    {
      if ( mInputIp.length() < maxInputFieldTextLength )
      {
        for ( const auto& digit : windowEvent.text.text )
          if ( std::isdigit(digit) == true || digit == '.' )
            mInputIp += digit;
          else
            break;
      }
    }
#endif

    return;
  }

  if ( mSpecifyingVarState[MENU_SPECIFY::PORT] == true )
  {
    std::string inputPort {};

    if ( mCurrentRoom == ROOMS::MENU_MP_DC_HOST )
      inputPort = mInputPortHost;
    else
      inputPort = mInputPortClient;

#ifdef VITA_PLATFORM
    // On PS Vita, handle text input more carefully
    if ( windowEvent.type == SDL_TEXTINPUT )
    {
      // Clear the field first if it's empty (to avoid appending to old content)
      if ( inputPort.empty() )
      {
        inputPort.clear();
      }
      
      if ( inputPort.length() < 5 )
      {
        for ( const auto& digit : windowEvent.text.text )
          if ( std::isdigit(digit) == true )
            inputPort += digit;
          else
            break;
      }
    }
#else
    if ( windowEvent.type == SDL_KEYDOWN )
    {
      if ( windowEvent.key.keysym.sym == SDLK_BACKSPACE && inputPort.length() > 0 )
        inputPort.pop_back();

      else if ( windowEvent.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL )
        SDL_SetClipboardText(inputPort.c_str());

      else if ( windowEvent.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL )
        if ( checkPort( SDL_GetClipboardText() ) == true )
          inputPort = SDL_GetClipboardText();
    }

    else if ( windowEvent.type == SDL_TEXTINPUT )
    {
      if ( inputPort.length() < 5 )
      {
        for ( const auto& digit : windowEvent.text.text )
          if ( std::isdigit(digit) == true )
            inputPort += digit;
          else
            break;
      }
    }
#endif

    if ( mCurrentRoom == ROOMS::MENU_MP_DC_HOST )
      mInputPortHost = inputPort;
    else
      mInputPortClient = inputPort;

    return;
  }

  if ( mSpecifyingVarState[MENU_SPECIFY::PASSWORD] == true )
  {
    if ( windowEvent.type == SDL_KEYDOWN )
    {
      if ( windowEvent.key.keysym.sym == SDLK_BACKSPACE && mInputPassword.length() > 0 )
        mInputPassword.pop_back();

      else if ( windowEvent.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL )
        SDL_SetClipboardText(mInputPassword.c_str());

      else if ( windowEvent.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL )
        if ( checkPass( SDL_GetClipboardText() ) )
          mInputPassword = SDL_GetClipboardText();
    }

    else if ( windowEvent.type == SDL_TEXTINPUT )
    {
      if (  mInputPassword.length() < maxInputFieldTextLength &&
            checkPass( {windowEvent.text.text} ) == true )
        mInputPassword += windowEvent.text.text;
    }

    return;
  }
}

void
Menu::ToggleSliderEditing( const MENU_SPECIFY varToSpecify )
{
  if ( mIsEditingSlider == true )
    return EndSliderEditing(varToSpecify);

  mIsEditingSlider = true;
  mSpecifyingVarState[varToSpecify] = true;
}

void
Menu::EndSliderEditing(
  const MENU_SPECIFY varToSpecify )
{
  mIsEditingSlider = false;
  mSpecifyingVarState[varToSpecify] = false;


  switch (varToSpecify)
  {
    case MENU_SPECIFY::WIN_SCORE:
    {
      break;
    }

    case MENU_SPECIFY::AUDIO_VOLUME:
    {
      setSoundVolume(gameState().audioVolume / 100.f);
      break;
    }

    case MENU_SPECIFY::STEREO_DEPTH:
    {
      break;
    }

    default:
      break;
  }


  settingsWrite();
}

void
Menu::UpdateSliderEditing()
{
  uint8_t* sliderValue {};

  if ( mSpecifyingVarState[MENU_SPECIFY::WIN_SCORE] == true )
    sliderValue = &gameState().winScore;

  else if ( mSpecifyingVarState[MENU_SPECIFY::AUDIO_VOLUME] == true )
    sliderValue = &gameState().audioVolume;

  else if ( mSpecifyingVarState[MENU_SPECIFY::STEREO_DEPTH] == true )
    sliderValue = &gameState().stereoDepth;


  if ( sliderValue == nullptr )
    return;


  const uint8_t maxSliderValue = 100;


  if ( isUniversalKeyPressed(SDL_SCANCODE_LEFT) == true ||
       isUniversalKeyPressed(SDL_SCANCODE_A) == true )
  {
    if ( *sliderValue >= 10 )
      *sliderValue -= 10;
    else
      *sliderValue = 0;
  }

  else if ( isUniversalKeyPressed(SDL_SCANCODE_DOWN) == true ||
            isUniversalKeyPressed(SDL_SCANCODE_S) == true )
  {
    if ( *sliderValue > 0 )
      --*sliderValue;
  }

  else if ( isUniversalKeyPressed(SDL_SCANCODE_RIGHT) == true ||
            isUniversalKeyPressed(SDL_SCANCODE_D) == true )
  {
    if ( *sliderValue < maxSliderValue - 10 )
      *sliderValue += 10;
    else
      *sliderValue = maxSliderValue;
  }

  else if ( isUniversalKeyPressed(SDL_SCANCODE_UP) == true ||
            isUniversalKeyPressed(SDL_SCANCODE_W) == true )
  {
    if ( *sliderValue < maxSliderValue )
      ++*sliderValue;
  }
}

void
Menu::ToggleDefiningKey(
  const MENU_SETTINGS_CONTROLS actionToDefine )
{
  if ( mIsDefiningKey == true )
  {
    mIsDefiningKey = false;
    return;
  }

  mIsDefiningKey = true;
  mKeyToDefine = actionToDefine;
}

void
Menu::UpdateDefiningKey()
{
  if ( mIsDefiningKey == false )
    return;

#ifdef VITA_PLATFORM
  SDL_GameControllerButton newButton = SDL_CONTROLLER_BUTTON_INVALID;

  // Check for gamepad button presses
  if (isButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP))
    newButton = SDL_CONTROLLER_BUTTON_DPAD_UP;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN))
    newButton = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT))
    newButton = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
    newButton = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_A))
    newButton = SDL_CONTROLLER_BUTTON_A;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_X))
    newButton = SDL_CONTROLLER_BUTTON_X;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_Y))
    newButton = SDL_CONTROLLER_BUTTON_Y;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_B))
    newButton = SDL_CONTROLLER_BUTTON_B;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
    newButton = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
    newButton = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
  else if (isButtonPressed(SDL_CONTROLLER_BUTTON_START))
  {
    // Exit key definition mode
    mIsDefiningKey = false;
    return;
  }

  if (newButton != SDL_CONTROLLER_BUTTON_INVALID)
  {
    auto& playerBindings =
      mCurrentRoom == ROOMS::MENU_SETTINGS_CONTROLS_PLAYER1
        ? bindings::player1
        : bindings::player2;

    switch (mKeyToDefine)
    {
      case MENU_SETTINGS_CONTROLS::ACCELERATE:
      {
        playerBindings.throttleUp = newButton;
        break;
      }

      case MENU_SETTINGS_CONTROLS::DECELERATE:
      {
        playerBindings.throttleDown = newButton;
        break;
      }

      case MENU_SETTINGS_CONTROLS::LEFT:
      {
        playerBindings.turnLeft = newButton;
        break;
      }

      case MENU_SETTINGS_CONTROLS::RIGHT:
      {
        playerBindings.turnRight = newButton;
        break;
      }

      case MENU_SETTINGS_CONTROLS::SHOOT:
      {
        playerBindings.fire = newButton;
        break;
      }

      case MENU_SETTINGS_CONTROLS::EJECT:
      {
        playerBindings.jump = newButton;
        break;
      }

      default:
        break;
    }

    mIsDefiningKey = false;
  }
#else
  SDL_Scancode newKey = SDL_SCANCODE_UNKNOWN;

  if (isUniversalKeyPressed(SDL_SCANCODE_ESCAPE) || isUniversalKeyPressed(SDL_SCANCODE_RETURN))
  {
    mIsDefiningKey = false;
    return;
  }

  // Check for key presses
  for (int i = 0; i < SDL_NUM_SCANCODES; ++i)
  {
    if (isUniversalKeyPressed(static_cast<SDL_Scancode>(i)))
    {
      newKey = static_cast<SDL_Scancode>(i);
      break;
    }
  }

  if (newKey != SDL_SCANCODE_UNKNOWN)
  {
    auto& playerBindings =
      mCurrentRoom == ROOMS::MENU_SETTINGS_CONTROLS_PLAYER1
        ? bindings::player1
        : bindings::player2;

    switch (mKeyToDefine)
    {
      case MENU_SETTINGS_CONTROLS::ACCELERATE:
        assignKeyBinding(playerBindings.throttleUp, newKey);
        break;
      case MENU_SETTINGS_CONTROLS::DECELERATE:
        assignKeyBinding(playerBindings.throttleDown, newKey);
        break;
      case MENU_SETTINGS_CONTROLS::LEFT:
        assignKeyBinding(playerBindings.turnLeft, newKey);
        break;
      case MENU_SETTINGS_CONTROLS::RIGHT:
        assignKeyBinding(playerBindings.turnRight, newKey);
        break;
      case MENU_SETTINGS_CONTROLS::SHOOT:
        assignKeyBinding(playerBindings.fire, newKey);
        break;
      case MENU_SETTINGS_CONTROLS::EJECT:
        assignKeyBinding(playerBindings.jump, newKey);
        break;
      default:
        break;
    }

    mIsDefiningKey = false;
  }
#endif
}

void
Menu::ResetKey()
{
  if ( mCurrentRoom != ROOMS::MENU_SETTINGS_CONTROLS_PLAYER1 &&
       mCurrentRoom != ROOMS::MENU_SETTINGS_CONTROLS_PLAYER2 )
    return;

  auto& playerBindings =
    mCurrentRoom == ROOMS::MENU_SETTINGS_CONTROLS_PLAYER1
      ? bindings::player1
      : bindings::player2;

  const auto defaultBindings =
    mCurrentRoom == ROOMS::MENU_SETTINGS_CONTROLS_PLAYER1
      ? bindings::defaults::player1
      : bindings::defaults::player2;

#ifdef VITA_PLATFORM
  // Direct assignment for Vita
  switch (mSelectedItem)
  {
    case MENU_SETTINGS_CONTROLS::ACCELERATE:
      playerBindings.throttleUp = defaultBindings.throttleUp;
      break;
    case MENU_SETTINGS_CONTROLS::DECELERATE:
      playerBindings.throttleDown = defaultBindings.throttleDown;
      break;
    case MENU_SETTINGS_CONTROLS::LEFT:
      playerBindings.turnLeft = defaultBindings.turnLeft;
      break;
    case MENU_SETTINGS_CONTROLS::RIGHT:
      playerBindings.turnRight = defaultBindings.turnRight;
      break;
    case MENU_SETTINGS_CONTROLS::SHOOT:
      playerBindings.fire = defaultBindings.fire;
      break;
    case MENU_SETTINGS_CONTROLS::EJECT:
      playerBindings.jump = defaultBindings.jump;
      break;
    default:
      break;
  }
#else
  // Use assignKeyBinding for non-Vita platforms
  switch (mSelectedItem)
  {
    case MENU_SETTINGS_CONTROLS::ACCELERATE:
      assignKeyBinding(playerBindings.throttleUp, defaultBindings.throttleUp);
      break;
    case MENU_SETTINGS_CONTROLS::DECELERATE:
      assignKeyBinding(playerBindings.throttleDown, defaultBindings.throttleDown);
      break;
    case MENU_SETTINGS_CONTROLS::LEFT:
      assignKeyBinding(playerBindings.turnLeft, defaultBindings.turnLeft);
      break;
    case MENU_SETTINGS_CONTROLS::RIGHT:
      assignKeyBinding(playerBindings.turnRight, defaultBindings.turnRight);
      break;
    case MENU_SETTINGS_CONTROLS::SHOOT:
      assignKeyBinding(playerBindings.fire, defaultBindings.fire);
      break;
    case MENU_SETTINGS_CONTROLS::EJECT:
      assignKeyBinding(playerBindings.jump, defaultBindings.jump);
      break;
    default:
      break;
  }
#endif
}

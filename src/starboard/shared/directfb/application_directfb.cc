// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/shared/directfb/application_directfb.h"

#include <algorithm>
#include <iomanip>

#include "starboard/input.h"
#include "starboard/key.h"
#include "starboard/log.h"
#include "starboard/memory.h"
#include "starboard/shared/directfb/window_internal.h"
#include "starboard/shared/posix/time_internal.h"
#include "starboard/shared/starboard/audio_sink/audio_sink_internal.h"
#include "starboard/time.h"

namespace starboard {

namespace {

SbKey DFBKeyEventToSbKey(const DFBInputEvent& event) {
  SB_DCHECK(event.type == DIET_KEYPRESS || event.type == DIET_KEYRELEASE);
  SB_DCHECK(event.flags & DIEF_KEYID);

  // The following Starboard keys are currently not being generated by the code
  // below:
  //   kSbKeyKana
  //   kSbKeyHangul
  //   kSbKeyHanja
  //   kSbKeyKanji
  //   kSbKeyConvert
  //   kSbKeyNonconvert
  //   kSbKeyDbeDbcschar
  //   kSbKeyExecute
  //   kSbKeyF13
  //   kSbKeyF14
  //   kSbKeyF15
  //   kSbKeyF16
  //   kSbKeyF17
  //   kSbKeyF18
  //   kSbKeyF19
  //   kSbKeyF20
  //   kSbKeyF21
  //   kSbKeyF22
  //   kSbKeyF23
  //   kSbKeyF24
  //   kSbKeyBrowserSearch
  //   kSbKeyBrowserHome
  //   kSbKeyMediaLaunchApp
  //   kSbKeyWla
  //   kSbKeyBrightnessDown
  //   kSbKeyBrightnessUp
  //   kSbKeyKbdBrightnessDown
  //   kSbKeyKbdBrightnessUp
  //   kSbKeyBrowserBack
  //   kSbKeyBrowserForward
  //   kSbKeyBrowserRefresh
  //   kSbKeyBrowserStop
  //   kSbKeyBrowserFavorites

  if (event.flags & DIEF_KEYSYMBOL) {
    switch (event.key_symbol) {
      case DIKS_CLEAR:
        return kSbKeyClear;
      case DIKS_SELECT:
        return kSbKeySelect;
      case DIKS_HELP:
        return kSbKeyHelp;
      case DIKS_MENU:
        return kSbKeyApps;

      // For supporting multimedia buttons on a USB keyboard.
      case DIKS_MUTE:
        return kSbKeyVolumeMute;
      case DIKS_VOLUME_DOWN:
        return kSbKeyVolumeDown;
      case DIKS_VOLUME_UP:
        return kSbKeyVolumeUp;
      case DIKS_NEXT:
        return kSbKeyMediaNextTrack;
      case DIKS_PREVIOUS:
        return kSbKeyMediaPrevTrack;
      case DIKS_STOP:
        return kSbKeyMediaStop;
      case DIKS_PLAY:
        return kSbKeyMediaPlayPause;
      case DIKS_MAIL:
        return kSbKeyMediaLaunchMail;
      case DIKS_CALCULATOR:
        return kSbKeyMediaLaunchApp2;
      case DIKS_POWER:
      case DIKS_POWER2:
        return kSbKeyPower;

      default: {
        // Follow through to switching on event.key_id.
      }
    }
  }

  switch (event.key_id) {
    case DIKI_BACKSPACE:
      return kSbKeyBack;
    case DIKI_DELETE:
      return kSbKeyDelete;
    case DIKI_TAB:
      return kSbKeyTab;
    case DIKI_ENTER:
    case DIKI_KP_ENTER:
      return kSbKeyReturn;
    case DIKI_SPACE:
      return kSbKeySpace;
    case DIKI_HOME:
      return kSbKeyHome;
    case DIKI_END:
      return kSbKeyEnd;
    case DIKI_PAGE_UP:
      return kSbKeyPrior;
    case DIKI_PAGE_DOWN:
      return kSbKeyNext;
    case DIKI_LEFT:
      return kSbKeyLeft;
    case DIKI_RIGHT:
      return kSbKeyRight;
    case DIKI_DOWN:
      return kSbKeyDown;
    case DIKI_UP:
      return kSbKeyUp;
    case DIKI_ESCAPE:
      return kSbKeyEscape;
    case DIKI_A:
      return kSbKeyA;
    case DIKI_B:
      return kSbKeyB;
    case DIKI_C:
      return kSbKeyC;
    case DIKI_D:
      return kSbKeyD;
    case DIKI_E:
      return kSbKeyE;
    case DIKI_F:
      return kSbKeyF;
    case DIKI_G:
      return kSbKeyG;
    case DIKI_H:
      return kSbKeyH;
    case DIKI_I:
      return kSbKeyI;
    case DIKI_J:
      return kSbKeyJ;
    case DIKI_K:
      return kSbKeyK;
    case DIKI_L:
      return kSbKeyL;
    case DIKI_M:
      return kSbKeyM;
    case DIKI_N:
      return kSbKeyN;
    case DIKI_O:
      return kSbKeyO;
    case DIKI_P:
      return kSbKeyP;
    case DIKI_Q:
      return kSbKeyQ;
    case DIKI_R:
      return kSbKeyR;
    case DIKI_S:
      return kSbKeyS;
    case DIKI_T:
      return kSbKeyT;
    case DIKI_U:
      return kSbKeyU;
    case DIKI_V:
      return kSbKeyV;
    case DIKI_W:
      return kSbKeyW;
    case DIKI_X:
      return kSbKeyX;
    case DIKI_Y:
      return kSbKeyY;
    case DIKI_Z:
      return kSbKeyZ;

    case DIKI_0:
    case DIKI_1:
    case DIKI_2:
    case DIKI_3:
    case DIKI_4:
    case DIKI_5:
    case DIKI_6:
    case DIKI_7:
    case DIKI_8:
    case DIKI_9:
      return static_cast<SbKey>(kSbKey0 + (event.key_id - DIKI_0));

    case DIKI_KP_0:
    case DIKI_KP_1:
    case DIKI_KP_2:
    case DIKI_KP_3:
    case DIKI_KP_4:
    case DIKI_KP_5:
    case DIKI_KP_6:
    case DIKI_KP_7:
    case DIKI_KP_8:
    case DIKI_KP_9:
      return static_cast<SbKey>(kSbKeyNumpad0 + (event.key_id - DIKI_KP_0));

    case DIKI_KP_MULT:
      return kSbKeyMultiply;
    case DIKI_KP_PLUS:
      return kSbKeyAdd;
    case DIKI_KP_SEPARATOR:
      return kSbKeySeparator;
    case DIKI_KP_MINUS:
      return kSbKeySubtract;
    case DIKI_KP_DECIMAL:
      return kSbKeyDecimal;
    case DIKI_KP_DIV:
      return kSbKeyDivide;
    case DIKI_KP_EQUAL:
    case DIKI_EQUALS_SIGN:
      return kSbKeyOemPlus;
    case DIKI_COMMA:
      return kSbKeyOemComma;
    case DIKI_MINUS_SIGN:
      return kSbKeyOemMinus;
    case DIKI_PERIOD:
      return kSbKeyOemPeriod;
    case DIKI_SEMICOLON:
      return kSbKeyOem1;
    case DIKI_SLASH:
      return kSbKeyOem2;
    case DIKI_QUOTE_LEFT:
      return kSbKeyOem3;
    case DIKI_BRACKET_LEFT:
      return kSbKeyOem4;
    case DIKI_BACKSLASH:
      return kSbKeyOem5;
    case DIKI_BRACKET_RIGHT:
      return kSbKeyOem6;
    case DIKI_QUOTE_RIGHT:
      return kSbKeyOem7;
    case DIKI_SHIFT_L:
    case DIKI_SHIFT_R:
      return kSbKeyShift;
    case DIKI_CONTROL_L:
    case DIKI_CONTROL_R:
      return kSbKeyControl;
    case DIKI_META_L:
    case DIKI_META_R:
    case DIKI_ALT_L:
    case DIKI_ALT_R:
      return kSbKeyMenu;
    case DIKI_PAUSE:
      return kSbKeyPause;
    case DIKI_CAPS_LOCK:
      return kSbKeyCapital;
    case DIKI_NUM_LOCK:
      return kSbKeyNumlock;
    case DIKI_SCROLL_LOCK:
      return kSbKeyScroll;
    case DIKI_PRINT:
      return kSbKeyPrint;
    case DIKI_INSERT:
      return kSbKeyInsert;
    case DIKI_SUPER_L:
      return kSbKeyLwin;
    case DIKI_SUPER_R:
      return kSbKeyRwin;
    case DIKI_F1:
    case DIKI_F2:
    case DIKI_F3:
    case DIKI_F4:
    case DIKI_F5:
    case DIKI_F6:
    case DIKI_F7:
    case DIKI_F8:
    case DIKI_F9:
    case DIKI_F10:
    case DIKI_F11:
    case DIKI_F12:
      return static_cast<SbKey>(kSbKeyF1 + (event.key_id - DIKI_F1));
    case DIKI_KP_F1:
    case DIKI_KP_F2:
    case DIKI_KP_F3:
    case DIKI_KP_F4:
      return static_cast<SbKey>(kSbKeyF1 + (event.key_id - DIKI_KP_F1));

    default: {
      SB_DLOG(WARNING) << "Unknown event.key_id: 0x" << std::hex
                       << event.key_id;
    }
  }

  return kSbKeyUnknown;
}  // NOLINT(readability/fn_size)

SbKeyLocation DFBKeyEventToSbKeyLocation(const DFBInputEvent& event) {
  SB_DCHECK(event.type == DIET_KEYPRESS || event.type == DIET_KEYRELEASE);
  SB_DCHECK(event.flags & DIEF_KEYID);

  switch (event.key_id) {
    case DIKI_SHIFT_L:
    case DIKI_CONTROL_L:
    case DIKI_META_L:
    case DIKI_ALT_L:
      return kSbKeyLocationLeft;
    case DIKI_SHIFT_R:
    case DIKI_CONTROL_R:
    case DIKI_META_R:
    case DIKI_ALT_R:
      return kSbKeyLocationRight;

    default: {}
  }

  return kSbKeyLocationUnspecified;
}

unsigned int DFBKeyEventToSbKeyModifiers(const DFBInputEvent& event) {
  unsigned int key_modifiers = kSbKeyModifiersNone;
  if (event.modifiers & DIMM_ALT) {
    key_modifiers |= kSbKeyModifiersAlt;
  }
  if (event.modifiers & DIMM_CONTROL) {
    key_modifiers |= kSbKeyModifiersCtrl;
  }
  if (event.modifiers & DIMM_SHIFT) {
    key_modifiers |= kSbKeyModifiersShift;
  }
  return key_modifiers;
}

}  // namespace

ApplicationDirectFB::ApplicationDirectFB()
    : directfb_(NULL), window_(kSbWindowInvalid) {
  SbAudioSinkPrivate::Initialize();
}

ApplicationDirectFB::~ApplicationDirectFB() {
  SbAudioSinkPrivate::TearDown();
}

SbWindow ApplicationDirectFB::CreateWindow(const SbWindowOptions* options) {
  if (SbWindowIsValid(window_)) {
    // Cannot create a window if one is already created.
    return kSbWindowInvalid;
  }

  window_ = new SbWindowPrivate(GetDirectFB(), options);
  return window_;
}

bool ApplicationDirectFB::DestroyWindow(SbWindow window) {
  if (!SbWindowIsValid(window)) {
    return false;
  }
  if (window != window_) {
    return false;
  }

  delete window_;
  window_ = kSbWindowInvalid;

  return true;
}

IDirectFB* ApplicationDirectFB::GetDirectFB() {
  return directfb_;
}

SbWindow ApplicationDirectFB::GetWindow() {
  return window_;
}

void ApplicationDirectFB::Initialize() {
  // We only support one DirectFB device, so set it up and return it here.
  int argc = 0;
  if (DirectFBInit(&argc, NULL) != DFB_OK) {
    SB_NOTREACHED() << "Error calling DirectFBInit().";
  }

  // Setup DirectFB to not provide any default window background.
  DirectFBSetOption("bg-none", NULL);
  // Setup DirectFB to not show their default mouse cursor.
  DirectFBSetOption("no-cursor", NULL);

  // Create the DirectFB object.
  if (DirectFBCreate(&directfb_) != DFB_OK) {
    SB_NOTREACHED() << "Error calling DirectFBCreate().";
  }
  if (directfb_->SetCooperativeLevel(directfb_, DFSCL_NORMAL) != DFB_OK) {
    SB_NOTREACHED() << "Error calling SetCooperativeLevel().";
  }
}

void ApplicationDirectFB::Teardown() {
  SB_DCHECK(!SbWindowIsValid(window_));
  directfb_->Release(directfb_);
}

shared::starboard::Application::Event*
ApplicationDirectFB::PollNextSystemEvent() {
  DFBInputEvent dfb_event;
  if (window_->event_buffer->GetEvent(window_->event_buffer,
                                      DFB_EVENT(&dfb_event)) == DFB_OK) {
    return DFBEventToEvent(dfb_event);
  } else {
    return NULL;
  }
}

shared::starboard::Application::Event*
ApplicationDirectFB::WaitForSystemEventWithTimeout(SbTime time) {
  unsigned int seconds = time / kSbTimeSecond;
  unsigned int milliseconds = (time % kSbTimeSecond) / kSbTimeMillisecond;
  window_->event_buffer->WaitForEventWithTimeout(window_->event_buffer, seconds,
                                                 milliseconds);

  return PollNextSystemEvent();
}

bool ApplicationDirectFB::MayHaveSystemEvents() {
  return SbWindowIsValid(window_);
}

void ApplicationDirectFB::WakeSystemEventWait() {
  if (IsCurrentThread()) {
    return;
  }

  SB_DCHECK(SbWindowIsValid(window_));

  // The window is valid, call WakeUp() to break out of
  // WaitForEventWithTimeout(), if a thread is waiting in that function right
  // now.
  window_->event_buffer->WakeUp(window_->event_buffer);
}

shared::starboard::Application::Event* ApplicationDirectFB::DFBEventToEvent(
    const DFBInputEvent& event) {
  const int kKeyboardDeviceId = 1;

  if (event.type == DIET_KEYPRESS || event.type == DIET_KEYRELEASE) {
    SB_DCHECK(event.flags & DIEF_KEYID);

    SbInputData* data = new SbInputData();
    SbMemorySet(data, 0, sizeof(*data));
    data->window = window_;
    SB_DCHECK(SbWindowIsValid(data->window));
    data->type = (event.type == DIET_KEYPRESS ? kSbInputEventTypePress
                                              : kSbInputEventTypeUnpress);
    data->device_type = kSbInputDeviceTypeKeyboard;
    data->device_id = kKeyboardDeviceId;
    data->key = DFBKeyEventToSbKey(event);
    data->key_location = DFBKeyEventToSbKeyLocation(event);
    data->key_modifiers = DFBKeyEventToSbKeyModifiers(event);
    return new Event(kSbEventTypeInput, data, &DeleteDestructor<SbInputData>);
  }

  return NULL;
}

}  // namespace starboard

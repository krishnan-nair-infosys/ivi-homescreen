// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_DESKTOP_KEYBOARD_HOOK_HANDLER_H
#define FLUTTER_SHELL_PLATFORM_DESKTOP_KEYBOARD_HOOK_HANDLER_H

#include <xkbcommon/xkbcommon.h>

#include "flutter_desktop_view_controller_state.h"
#include "view/flutter_view.h"

namespace flutter {

// RDK key metadata from display.cc (evdev, Flutter logical/physical, display name).
struct KeyboardHookMetadata {
  uint32_t evdev = 0;
  uint64_t logical = 0;
  uint64_t physical = 0;
  const char* name = "";
  // UTF-8 from xkb_state_key_get_utf8(); valid only for the synchronous hook call.
  const char* utf8 = nullptr;
  bool is_repeat = false;
};

// Abstract class for handling keyboard input events.
class KeyboardHookHandler {
 public:
  virtual ~KeyboardHookHandler() = default;

  // A function for hooking into keyboard input.
  virtual void KeyboardHook(bool released,
                            xkb_keysym_t keysym,
                            uint32_t xkb_scancode,
                            uint32_t modifiers,
                            const KeyboardHookMetadata& meta) = 0;

  // A function for hooking into unicode code point input.
  virtual void CharHook(unsigned int code_point) = 0;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_DESKTOP_KEYBOARD_HOOK_HANDLER_H

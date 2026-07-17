// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_DESKTOP_KEY_EVENT_HANDLER_H
#define FLUTTER_SHELL_PLATFORM_DESKTOP_KEY_EVENT_HANDLER_H

#include <memory>
#include <string>

#include "rapidjson/rapidjson.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "rapidjson/document.h"
#include "shell/platform/homescreen/keyboard_hook_handler.h"

namespace flutter {

// Implements a KeyboardHookHandler
//
// Handles key events and forwards them to the Flutter engine.
class KeyEventHandler final : public KeyboardHookHandler {
 public:
  KeyEventHandler(flutter::BinaryMessenger* messenger,
                  ::FlutterDesktopViewControllerState* view_state);

  ~KeyEventHandler() override;

  // |KeyboardHookHandler|
  void KeyboardHook(bool released,
                    xkb_keysym_t keysym,
                    uint32_t xkb_scancode,
                    uint32_t modifiers,
                    const KeyboardHookMetadata& meta) override;

  // |KeyboardHookHandler|
  void CharHook(unsigned int code_point) override;

 private:
#if defined(RDK_USE_FLUTTER_KEYDATA)
  void SendFlutterKeyData(bool released,
                          xkb_keysym_t keysym,
                          const KeyboardHookMetadata& meta);
#endif

  void SendLegacyKeyEvent(bool released,
                          xkb_keysym_t keysym,
                          uint32_t xkb_scancode,
                          uint32_t modifiers);

  // The Flutter system channel for key event messages.
  std::unique_ptr<flutter::BasicMessageChannel<rapidjson::Document>> channel_;
  ::FlutterDesktopViewControllerState* view_state_;
  std::string character_buffer_;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_DESKTOP_KEY_EVENT_HANDLER_H

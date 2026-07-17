// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/platform/homescreen/key_event_handler.h"

#include "engine.h"
#include "flutter/shell/platform/common/json_message_codec.h"
#include "flutter_desktop_view_controller_state.h"
#include "libflutter_engine.h"
#include "shell/task_runner.h"
#include "spdlog/spdlog.h"
#include "view/flutter_view.h"

static constexpr char kChannelName[] = "flutter/keyevent";

static constexpr char kKeyCodeKey[] = "keyCode";
static constexpr char kKeyMapKey[] = "keymap";
static constexpr char kScanCodeKey[] = "scanCode";
static constexpr char kModifiersKey[] = "modifiers";
static constexpr char kTypeKey[] = "type";
static constexpr char kToolkitKey[] = "toolkit";
static constexpr char kUnicodeScalarValues[] = "unicodeScalarValues";

static constexpr char kLinuxKeyMap[] = "linux";
static constexpr char kValueToolkitGtk[] = "gtk";

static constexpr char kKeyUp[] = "keyup";
static constexpr char kKeyDown[] = "keydown";

namespace flutter {

namespace {

std::string KeysymToUtf8(xkb_keysym_t keysym) {
  char utf8[16] = {};
  const int len = xkb_keysym_to_utf8(keysym, utf8, sizeof(utf8));
  if (len <= 0) {
    return {};
  }
  return std::string(utf8, static_cast<size_t>(len));
}

std::string CharacterForKeyEvent(xkb_keysym_t keysym,
                                 const KeyboardHookMetadata& meta) {
  if (meta.utf8 != nullptr && meta.utf8[0] != '\0') {
    return meta.utf8;
  }
  return KeysymToUtf8(keysym);
}

}  // namespace

KeyEventHandler::KeyEventHandler(flutter::BinaryMessenger* messenger,
                                 ::FlutterDesktopViewControllerState* view_state)
    : channel_(
          std::make_unique<flutter::BasicMessageChannel<rapidjson::Document>>(
              messenger,
              kChannelName,
              &flutter::JsonMessageCodec::GetInstance())),
      view_state_(view_state) {
}

KeyEventHandler::~KeyEventHandler() = default;

void KeyEventHandler::CharHook(unsigned int /* code_point */) {}

#if defined(RDK_USE_FLUTTER_KEYDATA)
void KeyEventHandler::SendFlutterKeyData(bool released,
                                         xkb_keysym_t keysym,
                                         const KeyboardHookMetadata& meta) {
  if (view_state_ == nullptr || view_state_->engine == nullptr) {
    spdlog::warn("[RDK_KEYDATA] engine not ready, dropping key");
    return;
  }

  FlutterKeyEvent event{};
  event.struct_size = sizeof(FlutterKeyEvent);
  event.timestamp = static_cast<double>(LibFlutterEngine->GetCurrentTime());
  event.physical = meta.physical;
  event.logical = meta.logical;
  event.synthesized = false;
  event.device_type = kFlutterKeyEventDeviceTypeKeyboard;

  if (released) {
    event.type = kFlutterKeyEventTypeUp;
    event.character = nullptr;
    character_buffer_.clear();
  } else if (meta.is_repeat) {
    event.type = kFlutterKeyEventTypeRepeat;
    character_buffer_ = CharacterForKeyEvent(keysym, meta);
    event.character =
        character_buffer_.empty() ? nullptr : character_buffer_.c_str();
  } else {
    event.type = kFlutterKeyEventTypeDown;
    character_buffer_ = CharacterForKeyEvent(keysym, meta);
    event.character =
        character_buffer_.empty() ? nullptr : character_buffer_.c_str();
  }

  // KeyboardHook runs on the Wayland event thread; never call
  // FlutterEngineSendKeyEvent directly from here.
  TaskRunner* task_runner = view_state_->engine->GetPlatformTaskRunner();
  if (task_runner == nullptr) {
    spdlog::warn("[RDK_KEYDATA] platform task runner missing, dropping key");
    return;
  }

  const std::string character_copy =
      released ? std::string{} : character_buffer_;
  auto future =
      task_runner->QueueSendKeyEvent(event, std::move(character_copy));
  const FlutterEngineResult result = future.get();
  if (result != kSuccess) {
    spdlog::warn("[RDK_KEYDATA] FlutterEngineSendKeyEvent failed: {}",
                 static_cast<int>(result));
  }
}
#endif

void KeyEventHandler::SendLegacyKeyEvent(bool released,
                                         xkb_keysym_t keysym,
                                         uint32_t xkb_scancode,
                                         uint32_t modifiers) {
  rapidjson::Document event(rapidjson::kObjectType);
  auto& allocator = event.GetAllocator();
  event.AddMember(kKeyCodeKey, static_cast<uint32_t>(keysym), allocator);
  event.AddMember(kKeyMapKey, kLinuxKeyMap, allocator);
  event.AddMember(kToolkitKey, kValueToolkitGtk, allocator);
  event.AddMember(kScanCodeKey, xkb_scancode, allocator);
  event.AddMember(kModifiersKey, modifiers, allocator);

  const uint32_t utf32_code = xkb_keysym_to_utf32(keysym);
  if (utf32_code) {
    event.AddMember(kUnicodeScalarValues, utf32_code, allocator);
  }
  if (released) {
    event.AddMember(kTypeKey, kKeyUp, allocator);
  } else {
    event.AddMember(kTypeKey, kKeyDown, allocator);
  }

  channel_->Send(event);
}

void KeyEventHandler::KeyboardHook(bool released,
                                   xkb_keysym_t keysym,
                                   uint32_t xkb_scancode,
                                   uint32_t modifiers,
                                   const KeyboardHookMetadata& meta) {
#if defined(RDK_USE_FLUTTER_KEYDATA)
  // KeyData plus flutter/keyevent — KeyEventManager requires both (Flutter #132433).
  SendFlutterKeyData(released, keysym, meta);
  SendLegacyKeyEvent(released, keysym, xkb_scancode, modifiers);
#else
  SendLegacyKeyEvent(released, keysym, xkb_scancode, modifiers);
#endif
}
}  // namespace flutter

#pragma once
#include "core.h"

namespace keymap {
using core::key;
using core::media;
using core::none;
constexpr uint8_t Ctrl = 0x01, Shift = 0x02, Option = 0x04, Command = 0x08;
// USB HID usage IDs: physical keys, interpreted using the Mac's keyboard layout.
constexpr uint16_t PlayPause = 0xCD, Next = 0xB5, Previous = 0xB6;
constexpr uint16_t Stop = 0xB7, Mute = 0xE2, VolumeUp = 0xE9, VolumeDown = 0xEA;
struct Layer {
  const char *name;
  const char *encoderLabel;
  core::Action keys[9];
  core::Action cw, ccw, click;
};
constexpr Layer layers[] = {
    {"MAC",
     "ENC: Volume",
     {key(Command, 0x06, "Copy"), key(Command, 0x19, "Paste"), key(Command, 0x1B, "Cut"),
      key(Command, 0x1D, "Undo"), key(Command | Shift, 0x1D, "Redo"),
      key(Command | Shift, 0x21, "Screenshot"), none(), none(), media(PlayPause, "Play/Pause")},
     media(VolumeUp, "Volume +"),
     media(VolumeDown, "Volume -"),
     media(Mute, "Mute")},
    {"MEDIA",
     "ENC: Traccia",
     {media(Previous, "Previous"), media(PlayPause, "Play/Pause"), media(Next, "Next"),
      media(VolumeDown, "Volume -"), media(Mute, "Mute"), media(VolumeUp, "Volume +"),
      media(Stop, "Stop"), none(), none()},
     media(Next, "Next"),
     media(Previous, "Previous"),
     media(PlayPause, "Play/Pause")},
    {"WORK",
     "ENC: Tab",
     {key(Command, 0x09, "Find"), key(Command, 0x16, "Save"), key(Command, 0x17, "New tab"),
      key(Command, 0x1A, "Close tab"), key(Command, 0x15, "Reload"),
      key(Command | Shift, 0x17, "Reopen tab"), key(0, 0x29, "Escape"), none(), none()},
     key(Ctrl, 0x2B, "Next tab"),
     key(Ctrl | Shift, 0x2B, "Previous tab"),
     key(0, 0x28, "Enter")}};
constexpr size_t kCount = sizeof(layers) / sizeof(layers[0]);
} // namespace keymap

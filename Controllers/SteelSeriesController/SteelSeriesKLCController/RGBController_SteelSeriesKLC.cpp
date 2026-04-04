/*---------------------------------------------------------*\
| RGBController_SteelSeriesKLC.cpp                          |
|                                                           |
|   RGBController for SteelSeries KLC laptop keyboard       |
|   (MSI laptops, VID 0x1038, PID 0x113A)                  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_SteelSeriesKLC.h"

/**------------------------------------------------------------------*\
    @name SteelSeries KLC Laptop Keyboard
    @category Keyboard
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectSteelSeriesKLC
    @comment Per-key RGB. Protocol: 4x Feature Reports (0x0E, 524 bytes)
             + 1x Output Report (0x0D, 64 bytes) on usage page 0xFF00.
             All 85 keys use standard 12-byte RGB entries.
\*-------------------------------------------------------------------*/

/*---------------------------------------------------------------------*\
| LED names in LED array order (rows left-to-right, top-to-bottom).    |
| Matches KLC_LED_KEY_IDS in SteelSeriesKLCController.cpp.             |
\*---------------------------------------------------------------------*/
static const char* const KLC_LED_NAMES[STEELSERIES_KLC_KEY_COUNT] =
{
    /* Row 0 — Function row (16 keys) */
    "Escape",
    "F1", "F2", "F3", "F4", "F5", "F6",
    "F7", "F8", "F9", "F10", "F11", "F12",
    "Print Screen",
    "Delete",
    "Power",

    /* Row 1 — Number row (14 keys) */
    "Grave", "1", "2", "3", "4", "5", "6",
    "7", "8", "9", "0", "-", "=", "Backspace",

    /* Row 2 — QWERTY row (14 keys) */
    "Tab",
    "Q", "W", "E", "R", "T", "Y",
    "U", "I", "O", "P", "[", "]", "\\",

    /* Row 3 — Home row (13 keys) */
    "Caps Lock",
    "A", "S", "D", "F", "G", "H",
    "J", "K", "L", ";", "'",
    "Enter",

    /* Row 4 — Shift row (12 keys) */
    "Left Shift",
    "Z", "X", "C", "V", "B",
    "N", "M", ",", ".", "/",
    "Right Shift",

    /* Row 5 — Bottom row (8 keys) */
    "Left Ctrl", "Fn", "Left Win", "Left Alt",
    "Space",
    "Right Alt", "\\ |", "Right Ctrl",

    /* Navigation cluster (6 keys) */
    "PgUp / Home", "PgDn / End",
    "Left Arrow", "Down Arrow", "Up Arrow", "Right Arrow"
};

RGBController_SteelSeriesKLC::RGBController_SteelSeriesKLC(SteelSeriesKLCController* controller_ptr)
{
    controller  = controller_ptr;

    name        = controller->GetNameString();
    vendor      = "SteelSeries";
    type        = DEVICE_TYPE_KEYBOARD;
    description = "SteelSeries KLC Laptop Keyboard";
    location    = controller->GetDeviceLocation();
    serial      = controller->GetSerialString();

    mode direct;
    direct.name       = "Direct";
    direct.value      = 0;
    direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR;
    direct.color_mode = MODE_COLORS_PER_LED;
    modes.push_back(direct);

    SetupZones();
}

RGBController_SteelSeriesKLC::~RGBController_SteelSeriesKLC()
{
    delete controller;
}

void RGBController_SteelSeriesKLC::SetupZones()
{
    zone keyboard;
    keyboard.name       = "Keyboard";
    keyboard.type       = ZONE_TYPE_LINEAR;
    keyboard.leds_min   = STEELSERIES_KLC_KEY_COUNT;
    keyboard.leds_max   = STEELSERIES_KLC_KEY_COUNT;
    keyboard.leds_count = STEELSERIES_KLC_KEY_COUNT;
    keyboard.matrix_map = NULL;
    zones.push_back(keyboard);

    for(int i = 0; i < STEELSERIES_KLC_KEY_COUNT; i++)
    {
        led key;
        key.name = KLC_LED_NAMES[i];
        leds.push_back(key);
    }

    SetupColors();
}

void RGBController_SteelSeriesKLC::ResizeZone(int /*zone*/, int /*new_size*/)
{
    /*---------------------------------------------------------*\
    | This device does not support resizing zones               |
    \*---------------------------------------------------------*/
}

void RGBController_SteelSeriesKLC::DeviceUpdateLEDs()
{
    controller->SetColors(colors);
}

void RGBController_SteelSeriesKLC::UpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_SteelSeriesKLC::UpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_SteelSeriesKLC::DeviceUpdateMode()
{
    DeviceUpdateLEDs();
}

void RGBController_SteelSeriesKLC::DeviceSaveMode()
{
    /*---------------------------------------------------------*\
    | This device does not support saving to flash              |
    \*---------------------------------------------------------*/
}

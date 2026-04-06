/*---------------------------------------------------------*\
| RGBController_SteelSeriesKLC.cpp                          |
|                                                           |
|   RGBController for SteelSeries KLC laptop keyboard       |
|   (MSI laptops, VID 0x1038, PID 0x113A)                  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cstring>
#include "RGBControllerKeyNames.h"
#include "RGBController_SteelSeriesKLC.h"

#define NA 0xFFFFFFFF

#define KLC_MATRIX_WIDTH 22
#define KLC_MATRIX_HEIGHT 6

static const unsigned int klc_matrix_map[KLC_MATRIX_HEIGHT][KLC_MATRIX_WIDTH] =
{
    {  0, NA,  1,  2,  3,  4, NA,  5,  6,  7,  8, NA,  9, 10, 11, 12, 13, 14, 15, NA, NA, NA },
    { 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 29, 29, NA, NA, NA, NA, NA, NA },
    { 30, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 43, NA, NA, NA, NA, NA, NA },
    { 44, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 56, NA, NA, NA, NA, NA, NA },
    { 57, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 68, 68, NA, 81, NA, 77, NA, NA, NA },
    { 69, 70, 71, 72, 73, 73, 73, 73, 73, 73, 74, 75, 76, NA, NA, 79, 80, 82, 78, NA, NA, NA }
};

/* The navigation keys are injected into Row 5 (Shift row empty area) and Row 6 (Bottom Right)
   For mapping:
   PgUp: 77 (top right)
   PgDn: 78 (bottom right)
   Let's place them visually on the right:
   81=Up (Row 4)
   79=Left, 80=Down, 82=Right (Row 5)
*/


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
    KEY_EN_ESCAPE,
    KEY_EN_F1,
    KEY_EN_F2,
    KEY_EN_F3,
    KEY_EN_F4,
    KEY_EN_F5,
    KEY_EN_F6,
    KEY_EN_F7,
    KEY_EN_F8,
    KEY_EN_F9,
    KEY_EN_F10,
    KEY_EN_F11,
    KEY_EN_F12,
    KEY_EN_PRINT_SCREEN,
    KEY_EN_DELETE,
    KEY_EN_POWER,
    KEY_EN_BACK_TICK,
    KEY_EN_1,
    KEY_EN_2,
    KEY_EN_3,
    KEY_EN_4,
    KEY_EN_5,
    KEY_EN_6,
    KEY_EN_7,
    KEY_EN_8,
    KEY_EN_9,
    KEY_EN_0,
    KEY_EN_MINUS,
    KEY_EN_EQUALS,
    KEY_EN_BACKSPACE,
    KEY_EN_TAB,
    KEY_EN_Q,
    KEY_EN_W,
    KEY_EN_E,
    KEY_EN_R,
    KEY_EN_T,
    KEY_EN_Y,
    KEY_EN_U,
    KEY_EN_I,
    KEY_EN_O,
    KEY_EN_P,
    KEY_EN_LEFT_BRACKET,
    KEY_EN_RIGHT_BRACKET,
    KEY_EN_ANSI_BACK_SLASH,
    KEY_EN_CAPS_LOCK,
    KEY_EN_A,
    KEY_EN_S,
    KEY_EN_D,
    KEY_EN_F,
    KEY_EN_G,
    KEY_EN_H,
    KEY_EN_J,
    KEY_EN_K,
    KEY_EN_L,
    KEY_EN_SEMICOLON,
    KEY_EN_QUOTE,
    KEY_EN_ANSI_ENTER,
    KEY_EN_LEFT_SHIFT,
    KEY_EN_Z,
    KEY_EN_X,
    KEY_EN_C,
    KEY_EN_V,
    KEY_EN_B,
    KEY_EN_N,
    KEY_EN_M,
    KEY_EN_COMMA,
    KEY_EN_PERIOD,
    KEY_EN_FORWARD_SLASH,
    KEY_EN_RIGHT_SHIFT,
    KEY_EN_LEFT_CONTROL,
    KEY_EN_LEFT_FUNCTION,
    KEY_EN_LEFT_WINDOWS,
    KEY_EN_LEFT_ALT,
    KEY_EN_SPACE,
    KEY_EN_RIGHT_ALT,
    KEY_EN_ISO_BACK_SLASH,
    KEY_EN_RIGHT_CONTROL,
    KEY_EN_PAGE_UP,
    KEY_EN_PAGE_DOWN,
    KEY_EN_LEFT_ARROW,
    KEY_EN_DOWN_ARROW,
    KEY_EN_UP_ARROW,
    KEY_EN_RIGHT_ARROW
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
    for(std::size_t zone_idx = 0; zone_idx < zones.size(); zone_idx++)
    {
        if(zones[zone_idx].matrix_map != nullptr)
        {
            free(zones[zone_idx].matrix_map->map);
            delete zones[zone_idx].matrix_map;
        }
    }

    delete controller;
}

void RGBController_SteelSeriesKLC::SetupZones()
{
    zone keyboard;
    keyboard.name       = "Keyboard";
    keyboard.type       = ZONE_TYPE_MATRIX;
    keyboard.leds_min   = STEELSERIES_KLC_KEY_COUNT;
    keyboard.leds_max   = STEELSERIES_KLC_KEY_COUNT;
    keyboard.leds_count = STEELSERIES_KLC_KEY_COUNT;
    
    keyboard.matrix_map = new matrix_map_type;
    keyboard.matrix_map->height = KLC_MATRIX_HEIGHT;
    keyboard.matrix_map->width  = KLC_MATRIX_WIDTH;
    keyboard.matrix_map->map    = (unsigned int *)malloc(KLC_MATRIX_WIDTH * KLC_MATRIX_HEIGHT * sizeof(unsigned int));
    memcpy(keyboard.matrix_map->map, klc_matrix_map, KLC_MATRIX_WIDTH * KLC_MATRIX_HEIGHT * sizeof(unsigned int));

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

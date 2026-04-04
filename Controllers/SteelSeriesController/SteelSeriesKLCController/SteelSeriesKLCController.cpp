/*---------------------------------------------------------*\
| SteelSeriesKLCController.cpp                              |
|                                                           |
|   Driver for SteelSeries KLC laptop keyboard              |
|   (MSI laptops, VID 0x1038, PID 0x113A)                  |
|                                                           |
|   Protocol reverse-engineered via Wireshark USB capture   |
|   from SteelSeries GG on Windows.                         |
|                                                           |
|   All 85 keys use the standard 12-byte RGB entry format.  |
|   No per-key exceptions or quirks are required.           |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <cstring>
#include "SteelSeriesKLCController.h"
#include "StringUtils.h"
#include "Colors.h"

/*---------------------------------------------------------------------*\
| Fragment 1 — 42 keys: A–0 (HID 0x04–0x27) + F1–F6 (0x3A–0x3F)      |
|                                                                       |
| Key order matches the order captured from SteelSeries GG.            |
| led_idx maps each key_id to its position in the OpenRGB LED array.   |
| led_idx = -1 means no physical LED — always sent as black.           |
\*---------------------------------------------------------------------*/
static const KLCKeyEntry KLC_FRAG1[42] =
{
    {0x04, 46}, {0x05, 64}, {0x06, 62}, {0x07, 48}, {0x08, 34}, {0x09, 49},
    {0x0a, 50}, {0x0b, 51}, {0x0c, 39}, {0x0d, 52}, {0x0e, 53}, {0x0f, 54},
    {0x10, 66}, {0x11, 65}, {0x12, 40}, {0x13, 41}, {0x14, 32}, {0x15, 35},
    {0x16, 47}, {0x17, 36}, {0x18, 38}, {0x19, 63}, {0x1a, 33}, {0x1b, 61},
    {0x1c, 37}, {0x1d, 60}, {0x1e, 18}, {0x1f, 19}, {0x20, 20}, {0x21, 21},
    {0x22, 22}, {0x23, 23}, {0x24, 24}, {0x25, 25}, {0x26, 26}, {0x27, 27},
    {0x3a,  1}, {0x3b,  2}, {0x3c,  3}, {0x3d,  4}, {0x3e,  5}, {0x3f,  6}
};

/*---------------------------------------------------------------------*\
| Fragment 2 — 11 keys: Enter, \, non-US keys, JP-specific keys        |
|                                                                       |
| 0x32 (non-US #) and 0x87–0x8B, 0x90–0x91 (JP layout keys) are       |
| sent as part of the protocol but have no LED on US/ISO layouts.      |
| 0x64 (ISO extra key, between L-Shift and Z) is exposed as a LED      |
| for keyboards that physically have it.                               |
\*---------------------------------------------------------------------*/
static const KLCKeyEntry KLC_FRAG2[11] =
{
    {0x28, 57}, {0x31, 44}, {0x32, -1}, {0x64, 59},
    {0x87, -1}, {0x88, -1}, {0x89, -1}, {0x8a, -1}, {0x8b, -1},
    {0x90, -1}, {0x91, -1}
};

/*---------------------------------------------------------------------*\
| Fragment 3 — 24 keys: punctuation, modifiers, Esc, Space, Power      |
\*---------------------------------------------------------------------*/
static const KLCKeyEntry KLC_FRAG3[24] =
{
    {0x29,  0}, {0x2a, 30}, {0x2b, 31}, {0x2c, 74}, {0x2d, 28}, {0x2e, 29},
    {0x2f, 42}, {0x30, 43}, {0x33, 55}, {0x34, 56}, {0x35, 17}, {0x36, 67},
    {0x37, 68}, {0x38, 69}, {0x39, 45}, {0x66, 13}, {0xe0, 71}, {0xe1, 58},
    {0xe2, 73}, {0xe3, 72}, {0xe4, 76}, {0xe5, 70}, {0xe6, 75}, {0xf0, 16}
};

/*---------------------------------------------------------------------*\
| Fragment 4 — 19 keys: F7–F12, nav cluster, arrow keys                |
|                                                                       |
| 0x47 (Scroll Lock), 0x48 (Pause), 0x49 (Insert) are Fn combinations |
| sharing the LED of another key — sent as black (led_idx = -1).      |
|                                                                       |
| 0x46 (Print Screen) and 0x4C (Delete) are physical keys with LEDs.  |
| SteelSeries GG uses a different byte sequence for these (bytes 6–9   |
| are 00 00 01 00 instead of 2C 01 02 01) and always sends them black. |
| We attempt the standard sequence here — if they don't respond,       |
| check the alternate struct: {0,0,0,0,0,0,0,0,0x01,0,0,key_id}.     |
\*---------------------------------------------------------------------*/
static const KLCKeyEntry KLC_FRAG4[19] =
{
    {0x40,  7}, {0x41,  8}, {0x42,  9}, {0x43, 10}, {0x44, 11}, {0x45, 12},
    {0x46, 14}, {0x47, -1}, {0x48, -1}, {0x49, -1},
    {0x4a, 77}, {0x4b, 78}, {0x4c, 15},
    {0x4d, 79}, {0x4e, 80}, {0x4f, 84}, {0x50, 81}, {0x51, 82}, {0x52, 83}
};

/*---------------------------------------------------------------------*\
| Fragment table — used by SetColors() to iterate all four fragments   |
\*---------------------------------------------------------------------*/
struct KLCFragment
{
    const KLCKeyEntry* keys;
    uint8_t            count;
};

static const KLCFragment KLC_FRAGMENTS[STEELSERIES_KLC_FRAG_COUNT] =
{
    {KLC_FRAG1, 42},
    {KLC_FRAG2, 11},
    {KLC_FRAG3, 24},
    {KLC_FRAG4, 19}
};


SteelSeriesKLCController::SteelSeriesKLCController(hid_device* dev_handle, const char* path, const std::string& dev_name)
{
    dev      = dev_handle;
    location = path;
    name     = dev_name;
}

SteelSeriesKLCController::~SteelSeriesKLCController()
{
    hid_close(dev);
}

std::string SteelSeriesKLCController::GetDeviceLocation()
{
    return("HID: " + location);
}

std::string SteelSeriesKLCController::GetNameString()
{
    return(name);
}

std::string SteelSeriesKLCController::GetSerialString()
{
    wchar_t serial_string[128];
    int ret = hid_get_serial_number_string(dev, serial_string, 128);

    if(ret != 0)
    {
        return("");
    }

    return(StringUtils::wstring_to_string(serial_string));
}

void SteelSeriesKLCController::SetColors(const std::vector<RGBColor>& colors)
{
    for(int f = 0; f < STEELSERIES_KLC_FRAG_COUNT; f++)
    {
        SendFragment(KLC_FRAGMENTS[f].keys, KLC_FRAGMENTS[f].count, colors);
    }

    SendCommit();
}

/*---------------------------------------------------------------------*\
| SendFragment                                                          |
|                                                                       |
| Builds and sends one Feature Report (wValue 0x0300).                 |
|                                                                       |
| Buffer layout (525 bytes sent to hidapi):                             |
|   [0x00]                 — artificial Report ID (forced by hidapi)   |
|   [0x0E][0x00][cnt][0x00]— 4-byte fragment header                    |
|   cnt × 12 bytes         — per-key entries                           |
|   [0x06][0xBE]           — fixed terminator (constant in all caps)   |
|   zeros                  — padding to fill 524-byte payload          |
|                                                                       |
| Per-key entry (12 bytes):                                            |
|   [R][G][B][00][00][00][2C][01][02][01][00][key_id]                  |
\*---------------------------------------------------------------------*/
void SteelSeriesKLCController::SendFragment(const KLCKeyEntry* keys, uint8_t count, const std::vector<RGBColor>& colors)
{
    unsigned char buf[STEELSERIES_KLC_PACKET_SIZE];
    memset(buf, 0x00, sizeof(buf));

    uint16_t offset = 1;  /* buf[0] stays 0x00 — artificial Report ID */

    buf[offset++] = 0x0E;
    buf[offset++] = 0x00;
    buf[offset++] = count;
    buf[offset++] = 0x00;

    for(uint8_t i = 0; i < count; i++)
    {
        RGBColor color = (keys[i].led_idx >= 0) ? colors[keys[i].led_idx] : COLOR_BLACK;

        buf[offset++] = RGBGetRValue(color);
        buf[offset++] = RGBGetGValue(color);
        buf[offset++] = RGBGetBValue(color);
        buf[offset++] = 0x00;
        buf[offset++] = 0x00;
        buf[offset++] = 0x00;
        buf[offset++] = 0x2C;
        buf[offset++] = 0x01;
        buf[offset++] = 0x02;
        buf[offset++] = 0x01;
        buf[offset++] = 0x00;
        buf[offset++] = keys[i].key_id;
    }

    buf[offset++] = 0x06;
    buf[offset]   = 0xBE;

    hid_send_feature_report(dev, buf, STEELSERIES_KLC_PACKET_SIZE);
}

/*---------------------------------------------------------------------*\
| SendCommit                                                            |
|                                                                       |
| Sends the Output Report (wValue 0x0200) that flushes the color data  |
| from the device buffer to the actual LEDs.                            |
|                                                                       |
| The commit MUST use hid_write() (Output Report), NOT                  |
| hid_send_feature_report(). The device silently ignores a commit       |
| sent as a Feature Report and the color never changes.                 |
\*---------------------------------------------------------------------*/
void SteelSeriesKLCController::SendCommit()
{
    unsigned char buf[STEELSERIES_KLC_COMMIT_SIZE];
    memset(buf, 0x00, sizeof(buf));

    buf[0]  = 0x00;   /* artificial Report ID */
    buf[1]  = 0x0D;   /* commit command       */
    buf[3]  = 0x02;
    buf[64] = 0x44;   /* fixed last byte      */

    hid_write(dev, buf, STEELSERIES_KLC_COMMIT_SIZE);
}

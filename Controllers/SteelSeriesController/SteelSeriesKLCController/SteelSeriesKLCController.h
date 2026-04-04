/*---------------------------------------------------------*\
| SteelSeriesKLCController.h                                |
|                                                           |
|   Driver for SteelSeries KLC laptop keyboard              |
|   (MSI laptops, VID 0x1038, PID 0x113A)                  |
|                                                           |
|   Protocol reverse-engineered via Wireshark USB capture   |
|   from SteelSeries GG on Windows.                         |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <string>
#include <vector>
#include <hidapi.h>
#include "RGBController.h"

/*---------------------------------------------------------------------*\
| Protocol constants                                                    |
|                                                                       |
| Each color update requires 5 HID packets in sequence:                |
|   - 4 Feature Reports (wValue 0x0300, 524 bytes payload each)        |
|   - 1 Output Report   (wValue 0x0200, 64 bytes payload)              |
|                                                                       |
| hidapi prepends 0x00 (Report ID 0) to force the correct wValue.      |
\*---------------------------------------------------------------------*/
#define STEELSERIES_KLC_PACKET_SIZE     525   /* 1 byte Report ID + 524 bytes payload */
#define STEELSERIES_KLC_COMMIT_SIZE     65    /* 1 byte Report ID + 64 bytes payload  */
#define STEELSERIES_KLC_KEY_COUNT       85    /* Total exposed LEDs (US layout)        */
#define STEELSERIES_KLC_FRAG_COUNT      4     /* Number of Feature Reports per update  */

/*---------------------------------------------------------------------*\
| Per-key protocol entry.                                               |
| Each 12-byte key entry in a Feature Report is:                        |
|   [R][G][B][00][00][00][2C][01][02][01][00][key_id]                  |
|                                                                       |
| led_idx: index into the OpenRGB colors[] vector, or -1 if this       |
| key_id has no physical LED and must always be sent as black.          |
\*---------------------------------------------------------------------*/
struct KLCKeyEntry
{
    uint8_t  key_id;
    int      led_idx;
};

class SteelSeriesKLCController
{
public:
    SteelSeriesKLCController(hid_device* dev_handle, const char* path, const std::string& dev_name);
    ~SteelSeriesKLCController();

    std::string     GetDeviceLocation();
    std::string     GetNameString();
    std::string     GetSerialString();

    void            SetColors(const std::vector<RGBColor>& colors);

private:
    void            SendFragment(const KLCKeyEntry* keys, uint8_t count, const std::vector<RGBColor>& colors);
    void            SendCommit();

    hid_device*     dev;
    std::string     location;
    std::string     name;
};

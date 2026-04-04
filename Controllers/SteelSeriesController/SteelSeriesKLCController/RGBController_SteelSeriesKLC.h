/*---------------------------------------------------------*\
| RGBController_SteelSeriesKLC.h                            |
|                                                           |
|   RGBController for SteelSeries KLC laptop keyboard       |
|   (MSI laptops, VID 0x1038, PID 0x113A)                  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include "SteelSeriesKLCController.h"

class RGBController_SteelSeriesKLC : public RGBController
{
public:
    RGBController_SteelSeriesKLC(SteelSeriesKLCController* controller_ptr);
    ~RGBController_SteelSeriesKLC();

    void        SetupZones();
    void        ResizeZone(int zone, int new_size);

    void        DeviceUpdateLEDs();
    void        UpdateZoneLEDs(int zone);
    void        UpdateSingleLED(int led);

    void        DeviceUpdateMode();
    void        DeviceSaveMode();

private:
    SteelSeriesKLCController* controller;
};

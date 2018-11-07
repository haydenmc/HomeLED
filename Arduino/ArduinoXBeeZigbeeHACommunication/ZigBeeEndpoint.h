#pragma once
#include "ZigBeeProfile.h"

class ZigBeeEndpoint
{
    public:
        ZigBeeEndpoint(byte deviceId, byte deviceVersion, ZigBeeProfile* profile);
        byte GetDeviceId();
        byte GetDeviceVersion();
    private:
        byte deviceId = 0x00;
        byte deviceVersion = 0x00;
        ZigBeeProfile* profile;
};
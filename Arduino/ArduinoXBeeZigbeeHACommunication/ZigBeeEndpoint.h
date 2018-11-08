#pragma once
#include <Arduino.h>
#include "ZigBeeClusterLibraryFrame.h"

class ZigBeeEndpoint
{
    public:
        virtual unsigned int GetProfileId() = 0;
        virtual unsigned int GetDeviceId() = 0;
        virtual unsigned int GetDeviceVersion() = 0;
        virtual void ProcessFrame(const ZigBeeClusterLibraryFrame* frame) = 0;
};
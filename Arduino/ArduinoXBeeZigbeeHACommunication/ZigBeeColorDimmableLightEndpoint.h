#pragma once
#include <Arduino.h>
#include "ZigBeeEndpoint.h"

class ZigBeeColorDimmableLightEndpoint : public ZigBeeEndpoint
{
    public:
        ZigBeeColorDimmableLightEndpoint();

        // ZigBeeEndpoint
        unsigned int GetProfileId() override;
        unsigned int GetDeviceId() override;
        unsigned int GetDeviceVersion() override;
        void ProcessFrame(const ZigBeeClusterLibraryFrame* frame) override;

        // ColorDimmableLightEndpoint (ColorMode xyY)
        unsigned int GetX();
        unsigned int GetY();
        unsigned int GetLevel();

    private:
        bool lightIsOn;
        unsigned int lightX;
        unsigned int lightY;
        unsigned int lightLevel;

        void ProcessOnOffCommand(const ZigBeeClusterLibraryFrame* frame);
        void ProcessLevelControlCommand(const ZigBeeClusterLibraryFrame* frame);
        void ProcessColorControlCommand(const ZigBeeClusterLibraryFrame* frame);
};
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

        // ColorDimmableLightEndpoint
        unsigned int GetHue();
        unsigned int GetSaturation();
        unsigned int GetLevel();

    private:
        bool lightIsOn;
        unsigned int lightHue;
        unsigned int lightSaturation;
        unsigned int lightLevel;

        void ProcessOnOffCommand(const ZigBeeClusterLibraryFrame* frame);
        void ProcessLevelControlCommand(const ZigBeeClusterLibraryFrame* frame);
        void ProcessColorControlCommand(const ZigBeeClusterLibraryFrame* frame);
};
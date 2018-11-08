#pragma once
#include <Arduino.h>
#include "ZigBeeEndpoint.h"
#include "ZigBeeClusterLibraryFrame.h"

#ifndef ZIGBEE_MAX_ENDPOINTS
    #define ZIGBEE_MAX_ENDPOINTS 16
#endif

class ZigBee
{
    public:
        ZigBee();
        ZigBeeEndpoint* GetEndpoint(unsigned int endpointNumber);
        unsigned int GetEndpointsLength();
        bool SetEndpoint(unsigned int endpointNumber, ZigBeeEndpoint* endpoint);
        void ProcessFrame(const ZigBeeClusterLibraryFrame* frame);
    private:
        ZigBeeEndpoint* endpoints[ZIGBEE_MAX_ENDPOINTS] { nullptr };
        unsigned int endpointsLength = ZIGBEE_MAX_ENDPOINTS;
};

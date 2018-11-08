#include "ZigBee.h"

ZigBee::ZigBee()
{ }

ZigBeeEndpoint* ZigBee::GetEndpoint(unsigned int endpointNumber)
{
    if (endpointNumber < this->GetEndpointsLength())
    {
        return this->endpoints[endpointNumber];
    }
    else
    {
        return nullptr;
    }
}

unsigned int ZigBee::GetEndpointsLength()
{
    return this->endpointsLength;
}

bool ZigBee::SetEndpoint(unsigned int endpointNumber, ZigBeeEndpoint* endpoint)
{
    if (endpointNumber < this->GetEndpointsLength())
    {
        this->endpoints[endpointNumber] = endpoint;
        return true;
    }
    else
    {
        return false;
    }
}

void ZigBee::ProcessFrame(const ZigBeeClusterLibraryFrame* frame)
{
    if ((frame->destinationEndpoint < ZIGBEE_MAX_ENDPOINTS) &&
        (this->endpoints[frame->destinationEndpoint] != nullptr))
    {
        ZigBeeEndpoint* targetEndpoint = this->endpoints[frame->destinationEndpoint];
        if (targetEndpoint->GetProfileId() == frame->profileId)
        {
            targetEndpoint->ProcessFrame(frame);
        }
        else
        {
            // Profile mismatch
        }
    }
    else
    {
        // Non-existant endpoint
    }
}
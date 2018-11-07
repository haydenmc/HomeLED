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
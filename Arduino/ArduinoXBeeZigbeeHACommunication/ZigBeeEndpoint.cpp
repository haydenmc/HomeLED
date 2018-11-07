#include "ZigBeeEndpoint.h"

ZigBeeEndpoint::ZigBeeEndpoint(byte deviceId, byte deviceVersion, ZigBeeProfile* profile)
{
    this->deviceId = deviceId;
    this->deviceVersion = deviceVersion;
    this->profile = profile;
}

byte ZigBeeEndpoint::GetDeviceId()
{
    return this->deviceId;
}

byte ZigBeeEndpoint::GetDeviceVersion()
{
    return this->deviceVersion;
}
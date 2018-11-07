#pragma once
#include "ZigBeeCluster.h"

class ZigBeeProfile
{
    public:
        virtual byte GetProfileId() = 0;
        virtual ZigBeeCluster* GetInputCluster(byte clusterId) = 0;
        virtual ZigBeeCluster* GetOutputCluster(byte clusterId) = 0;
}
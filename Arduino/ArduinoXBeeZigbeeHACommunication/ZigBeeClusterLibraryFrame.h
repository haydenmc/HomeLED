#pragma once

#ifndef PACKET_BUFFER_LENGTH
    #define PACKET_BUFFER_LENGTH 128
#endif

struct ZigBeeClusterLibraryFrame
{
    // From APS Frame
    unsigned long long longSourceAddress;
    unsigned int shortSourceAddress;
    byte sourceEndpoint;
    byte destinationEndpoint;
    unsigned int clusterId;
    unsigned int profileId;

    // From ZCL Frame
    bool frameControlType;
    bool frameControlManufacturerSpecific;
    bool frameControlDirection;
    bool frameControlDisableDefaultResponse;
    byte transactionSequenceNumber;
    byte commandIdentifier;
    byte payload[PACKET_BUFFER_LENGTH];
    unsigned int payloadLength;
};
/**
 * @file ArduinoXBeeZigbeeHACommunication.ino
 * @author Hayden McAfee <hayden@outlook.com>
 * @date 30 Oct 2018
 * 
 * This is an Arduino sketch meant to power a simple Zigbee home automation
 * device via an XBee device over serial configured to operate in API mode.
 */

#define PACKET_BUFFER_LENGTH 128
#define SUPPORTED_ENDPOINTS_LENGTH 2

// ZCL status codes
#define SUCCESS 0x00
#define INVALID_EP 0x82

/**
 * Struct representing a supported ZB cluster attribute
 */
struct ZBClusterAttribute
{
    byte attributeId;
    byte attributeValue;
    ZBClusterAttribute* nextAttribute; // Linked list next attribute
};

/**
 * Struct representing a supported ZB Cluster
 */
struct ZBCluster
{
    unsigned int clusterId;
    ZBClusterAttribute* clusterAttributesList;
    ZBCluster* nextCluster; // Linked list next cluster
};

/**
 * Struct representing supported ZB profile
 */
struct ZBProfile
{
    unsigned int profileId;
    ZBCluster* inputClusterList;
    ZBCluster* outputClusterList;
};

/**
 * Struct representing a supported ZB Endpoint
 */
struct ZBEndpoint
{
    unsigned int deviceId;
    byte deviceVersion;
    ZBProfile* profile;
};

/**
 * Struct representing Zigbee state
 */
struct ZB
{
    ZBEndpoint* supportedEndpoints[SUPPORTED_ENDPOINTS_LENGTH] { nullptr };
};

/**
 * Struct representing a parsed ZCL frame
 */
struct ZCLFrameData
{
    byte frameControlType;
    bool frameControlManufacturerSpecific;
    bool frameControlDirection;
    bool frameControlDisableDefaultResponse;
    byte transactionSequenceNumber;
    byte commandIdentifier;
    byte payload[PACKET_BUFFER_LENGTH];
    unsigned int payloadLength;
};

/**
 * Struct representing an XBee explicit rx packet
 */
struct XBeeIncomingRxPacket
{
    unsigned long long longSourceAddress;
    unsigned int shortSourceAddress;
    byte sourceEndpoint;
    byte destinationEndpoint;
    unsigned int clusterId;
    unsigned int profileId;
    byte receiveOptions;
    byte data[PACKET_BUFFER_LENGTH];
    unsigned int dataLength;
};

/**
 * Struct representing a generic XBee packet
 */
struct XBeePacket
{
    unsigned int payloadLength;
    byte payload[PACKET_BUFFER_LENGTH];
};

/**
 * Struct representing XBee state
 */
struct XBee
{
    bool isConnected;
    unsigned long lastAiCommandSentTimeMs;
    byte lastFrameId;
};

/**
 * Global status of XBee connection
 */
XBee g_xBeeStatus;

/**
 * Global ZB definition
 */
ZB g_Zb;

#pragma region Arduino functions
/**
 * Runs once when the device boots
 */
void setup()
{
    // Set up XBee state
    g_xBeeStatus.isConnected = false;
    g_xBeeStatus.lastFrameId = 0x00;

    // Set up ZB state
    // Endpoint 0
    static ZBProfile zdpProfile { 0x0000 /* profile ID */, nullptr /* input clusters */, nullptr /* output clusters */ };
    static ZBEndpoint endpointZero { 0x0000 /* device ID */, 0x00 /* device version */, &zdpProfile };
    g_Zb.supportedEndpoints[0] = &endpointZero;

    // Endpoint 1 (home automation / color light)
    static ZBProfile haProfile { 0x0104 /* profile ID */, nullptr /* input clusters */, nullptr /* output clusters */ };
    static ZBEndpoint endpointOne { 0x0102 /* device ID */, 0x00 /* device version */, &haProfile };
    g_Zb.supportedEndpoints[1] = &endpointOne;

    // Open serial port to XBee
    Serial.begin(9600);
    Serial.print("***START***");
}

/**
 * Runs in a constant loop
 */
void loop()
{
    // Process incoming commands
    if (Serial.available() > 0)
    {
        XBeePacket packet;
        if (readIncomingPacket(&packet))
        {
            // Assuming this is an API frame packet, attempt to discern the frame type
            byte frameType = packet.payload[0];
            switch (frameType)
            {
            // AT Command Response Frame (0x88)
            case 0x88:
                ProcessAtCommandResponseFrame(&packet);
                break;

            // Explicit Rx Frame (RF packet received)
            case 0x91:
                XBeeIncomingRxPacket incomingRxPacket;
                ParseExplicitRxFrame(&packet, &incomingRxPacket);
                ProcessExplicitRxFrame(&incomingRxPacket);
                break;

            // Unknown packet
            default:
                break;
            }
        }
        else
        {
            // Bad packet
        }
    }

    //If we're not connected, let's ask why
    unsigned long timeSinceLastAiCommand = millis() - g_xBeeStatus.lastAiCommandSentTimeMs;
    if (!g_xBeeStatus.isConnected && timeSinceLastAiCommand > 5000)
    {
        // Send Association Indication command to get status
        SendAiAtCommand();
        g_xBeeStatus.lastAiCommandSentTimeMs = millis();
    }
}
#pragma endregion

#pragma region Incoming packet processing functions
/**
 * \brief Reads an incoming packet from the XBee, verifies checksum, and parses into output param.
 * 
 * @param[out] outPacket  The output
 * @return True on success, false on bad checksum or bad packet
 */
bool readIncomingPacket(XBeePacket* outPacket)
{
    byte startByte = Serial.read();
    if (startByte == 0x7E) // Start byte
    {
        // Get payload length
        while (Serial.available() < 2);
        byte lengthMsb = Serial.read();
        byte lengthLsb = Serial.read();
        unsigned int payloadLength = lengthMsb;
        payloadLength = (payloadLength << 8) | lengthLsb;

        // Extract payload
        byte payload[PACKET_BUFFER_LENGTH];
        unsigned int checksum = 0;
        while (Serial.available() < (int)(payloadLength + 1));
        for (unsigned int i = 0; i < payloadLength; ++i)
        {
            payload[i] = Serial.read();
            checksum += payload[i];
        }

        // Get checksum
        byte checksumByte = Serial.read();
        checksum += checksumByte;

        // Verify checksum
        if ((byte)(checksum & 0xFF) != 0xFF)
        {
            // Bad checksum
            return false;
        }

        outPacket->payloadLength = payloadLength;
        memcpy(outPacket->payload, payload, payloadLength);
        return true;
    }
    return false;
}

/**
 * \brief Processes incoming AT command responses
 * 
 * @param[in] inPacket The incoming response packet
 */
void ProcessAtCommandResponseFrame(XBeePacket* inPacket)
{
    // AI: Association Indication
    if (inPacket->payload[2] == 'A' && inPacket->payload[3] == 'I')
    {
        byte statusCode = inPacket->payload[4];
        if (statusCode == 0x00)
        {
            // Successfully joined Zigbee network
            g_xBeeStatus.isConnected = true;
        }
        else
        {
            // Some failure state
            g_xBeeStatus.isConnected = false;
        }
    }
}

/**
 * \brief Takes a basic XBee packet and parses it into an explicit Rx API frame
 * \sa https://www.digi.com/resources/documentation/digidocs/90001539/reference/r_frame_0x91.htm
 * 
 * @param[in] inPacket The incoming packet
 * @param[out] outPacket The destination for the parsed packed
 */
void ParseExplicitRxFrame(XBeePacket* inPacket, XBeeIncomingRxPacket* outPacket)
{
    // Pull out 64-bit source address from bytes 1 - 8
    unsigned long long longSourceAddress = 0ull;
    for (unsigned int i = 1; i < 9; ++i)
    {
        longSourceAddress = (longSourceAddress << 8) | inPacket->payload[i];
    }

    // Pull out 16-bit source address from bytes 9 - 10
    unsigned int shortSourceAddress = 0u;
    for (unsigned int i = 9; i < 11; ++i)
    {
        shortSourceAddress = (shortSourceAddress << 8) | inPacket->payload[i];
    }

    // Source, destination endpoints
    byte sourceEndpoint = inPacket->payload[11];
    byte destinationEndpoint = inPacket->payload[12];

    // Pull out cluster ID from bytes 13 - 14
    unsigned int clusterId = 0;
    for (unsigned int i = 13; i < 15; ++i)
    {
        clusterId = (clusterId << 8) | inPacket->payload[i];
    }

    // Pull out profile ID from bytes 15 - 16
    unsigned int profileId = 0;
    for (unsigned int i = 15; i < 17; ++i)
    {
        profileId = (profileId << 8) | inPacket->payload[i];
    }

    byte receiveOptions = inPacket->payload[17];

    // Pull out data from bytes 18 - n
    byte data[PACKET_BUFFER_LENGTH];
    for (unsigned int i = 18; i < inPacket->payloadLength; ++i)
    {
        data[i - 18] = inPacket->payload[i];
    }

    outPacket->longSourceAddress = longSourceAddress;
    outPacket->shortSourceAddress = shortSourceAddress;
    outPacket->sourceEndpoint = sourceEndpoint;
    outPacket->destinationEndpoint = destinationEndpoint;
    outPacket->clusterId = clusterId;
    outPacket->profileId = profileId;
    outPacket->receiveOptions = receiveOptions;
    memcpy(outPacket->data, data, inPacket->payloadLength - 18);
    outPacket->dataLength = inPacket->payloadLength - 18;
}

/**
 * \brief Parses an XBee incoming Rx packet to a ZCL frame
 * 
 * @param[in] inPacket      The XBee incoming Rx packet
 * @param[out] outFrameData The parsed ZCL frame data
 */
void ParseZCLFrame(XBeeIncomingRxPacket* inPacket, ZCLFrameData* outFrameData)
{
    // The first byte is the "frame control" byte
    byte frameControlByte = inPacket->data[0];

    // First two bits are the frame control type
    // xx000000
    outFrameData->frameControlType = (frameControlByte >> 6) & 0x03;

    // Next bit is manufacturer specific
    // 00x00000
    outFrameData->frameControlManufacturerSpecific = (frameControlByte >> 5) & 0x01;

    // Next bit is direction
    // 000x0000
    outFrameData->frameControlDirection = (frameControlByte >> 4) & 0x01;

    // Next bit is "disable default response"
    // 0000x000
    outFrameData->frameControlDisableDefaultResponse = (frameControlByte >> 3) & 0x01;

    // Next byte is the "transaction sequence number"
    // (unless manufacturer specific is true - we're ignoring this case)
    outFrameData->transactionSequenceNumber = inPacket->data[1];

    // Next byte is the command
    outFrameData->commandIdentifier = inPacket->data[2];

    // And the rest is payload
    for (unsigned int i = 3; i < inPacket->dataLength; ++i)
    {
        outFrameData->payload[i - 3] = inPacket->data[i];
    }
    outFrameData->payloadLength = inPacket->dataLength - 3;
}

/**
 * \brief Takes an explicit Rx API frame and acts on it accordingly
 * 
 * @param[in] inPacket The incoming packet
 */
void ProcessExplicitRxFrame(XBeeIncomingRxPacket* inPacket)
{
    switch (inPacket->profileId)
    {
    // ZDP profile
    case 0x0000:
        ProcessZdpProfile(inPacket);
        break;

    // Other profiles
    default:
        processGenericProfile(inPacket);
        break;
    }
    
}

void processGenericProfile(XBeeIncomingRxPacket* inPacket)
{
    // Find endpoint
    unsigned int targetEndpoint = inPacket->destinationEndpoint;
    if (targetEndpoint < SUPPORTED_ENDPOINTS_LENGTH && g_Zb.supportedEndpoints[targetEndpoint] != nullptr)
    {
        ZBEndpoint* endpoint = g_Zb.supportedEndpoints[targetEndpoint];

        // Ensure presence of requested profile ID
        ZBProfile* profile = endpoint->profile;
        if (profile->profileId == inPacket->profileId)
        {
            // Ensure presence of requested cluster ID
            ZBCluster* cluster = profile->inputClusterList;
            while (cluster != nullptr)
            {
                if (cluster->clusterId == inPacket->clusterId)
                {
                    break;
                }
                else
                {
                    cluster = cluster->nextCluster;
                }
            }
            if (cluster != nullptr)
            {
                ZCLFrameData zclFrameData;
                ParseZCLFrame(inPacket, &zclFrameData);
                switch (zclFrameData.commandIdentifier)
                {
                // Read attribute command
                case 0x00:
                    break;
                }
            }
            else
            {
                // Cluster ID not found
            }
        }
        else
        {
            // Profile ID mismatch
        }
    }
    else
    {
        // Endpoint does not exist.
    }
}

void ProcessZdpProfile(XBeeIncomingRxPacket* inPacket)
{
    switch (inPacket->clusterId)
    {
    // Simple Descriptor request
    case 0x0004:
        ProcessZdpSimpleDescriptorRequest(inPacket);
        break;

    // Active Endpoints request
    case 0x0005:
        ProcessZdpActiveEndpointsRequest(inPacket);
        break;

    // Matching Endpoints request
    case 0x0006:
        // TODO
        break;
    }
}

void ProcessZdpSimpleDescriptorRequest(XBeeIncomingRxPacket* inPacket)
{
    // Allocate payload
    byte payload[PACKET_BUFFER_LENGTH];
    unsigned int payloadLength = 0;

    // Payload is going back to sender
    // Write address out in little-endian (least-significant-bytes first)
    payload[1] = inPacket->shortSourceAddress & 0xFF;
    payload[2] = (inPacket->shortSourceAddress >> 8) & 0xFF;

    // Read endpoint
    byte targetEndpoint = inPacket->data[2];

    // Verify that we have that endpoint
    ZBEndpoint* endpoint = nullptr;
    if (targetEndpoint < SUPPORTED_ENDPOINTS_LENGTH)
    {
        endpoint = g_Zb.supportedEndpoints[targetEndpoint];
    }
    if (endpoint == nullptr)
    {
        // This endpoint doesn't exist. Make an error payload.
        payload[0] = INVALID_EP;
        payload[3] = 0x00;
        payloadLength = 4;
    }
    else
    {
        // We support this endpoint. Make a simple descriptor response payload.
        payload[0] = SUCCESS;

        // Simple descriptor
        payload[4] = targetEndpoint; // endpoint
        payload[5] = endpoint->profile->profileId & 0xFF; // profile id LSB
        payload[6] = (endpoint->profile->profileId >> 8) & 0xFF; // profile id MSB
        payload[7] = endpoint->deviceId & 0xFF; // profile id LSB
        payload[8] = (endpoint->deviceId >> 8) & 0xFF; // profile id MSB
        payload[9] = ((endpoint->deviceVersion & 0x0F) << 4) & 0xF0; // device version and 4 reserved bits (0)

        // input cluster IDs
        ZBCluster* inputCluster = endpoint->profile->inputClusterList;
        unsigned int inputClusterCount = 0;
        while (inputCluster != nullptr)
        {
            inputClusterCount++;
            payload[10 + (inputClusterCount * 2) - 1] = inputCluster->clusterId & 0xFF; // cluster id LSB
            payload[10 + (inputClusterCount * 2)] = (inputCluster->clusterId >> 8) & 0xFF; // cluster id MSB
            inputCluster = inputCluster->nextCluster;
        }
        payload[10] = inputClusterCount;

        // output cluster IDs
        unsigned int outputClusterPayloadIndexOffset = 11 + (inputClusterCount * 2);
        ZBCluster* outputCluster = endpoint->profile->outputClusterList;
        unsigned int outputClusterCount = 0;
        while (outputCluster != nullptr)
        {
            outputClusterCount++;
            payload[outputClusterPayloadIndexOffset + (outputClusterCount * 2) - 1]
                = outputCluster->clusterId & 0xFF; // cluster id LSB
            payload[outputClusterPayloadIndexOffset + (outputClusterCount * 2)]
                = (outputCluster->clusterId >> 8) & 0xFF; // cluster id MSB
            outputCluster = outputCluster->nextCluster;
        }
        payload[outputClusterPayloadIndexOffset] = outputClusterCount;

        // Calculate payload lengths
        payload[3] = 8 + (inputClusterCount * 2) + (outputClusterCount * 2);
        payloadLength = 12 + (inputClusterCount * 2) + (outputClusterCount * 2);
    }

    // Send response
    SendExplicitAddressingCommandFrame(
        0x0001, /* frame ID */
        inPacket->longSourceAddress, /* destination hw address */
        inPacket->shortSourceAddress, /* destination sw address */
        inPacket->destinationEndpoint, /* source endpoint */
        inPacket->sourceEndpoint, /* destination endpoint */
        0x8004, /* cluster ID */
        0x0000, /* profile ID */
        0x00, /* broadcast radius */
        0x00, /* transmission options */
        payload,
        payloadLength
    );
}

void ProcessZdpActiveEndpointsRequest(XBeeIncomingRxPacket* inPacket)
{
    // Allocate payload
    byte payload[PACKET_BUFFER_LENGTH];
    unsigned int payloadLength = 0;

    // Status
    payload[0] = SUCCESS;

    // Payload is going back to sender
    // Write address out in little-endian (least-significant-bytes first)
    payload[1] = inPacket->shortSourceAddress & 0xFF;
    payload[2] = (inPacket->shortSourceAddress >> 8) & 0xFF;
    
    // Figure out how many active endpoints we have
    unsigned int supportedEndpointCount = 0;
    for (unsigned int i = 0; i < SUPPORTED_ENDPOINTS_LENGTH; ++i)
    {
        payload[4 + supportedEndpointCount] = i;
        ++supportedEndpointCount;
    }

    // Calculate payload lengths
    payload[3] = supportedEndpointCount;
    payloadLength = 4 + supportedEndpointCount;

    // Send response
    SendExplicitAddressingCommandFrame(
        0x0001, /* frame ID */
        inPacket->longSourceAddress, /* destination hw address */
        inPacket->shortSourceAddress, /* destination sw address */
        inPacket->destinationEndpoint, /* source endpoint */
        inPacket->sourceEndpoint, /* destination endpoint */
        0x8005, /* cluster ID */
        0x0000, /* profile ID */
        0x00, /* broadcast radius */
        0x00, /* transmission options */
        payload,
        payloadLength
    );
}
#pragma endregion

#pragma region Outgoing packet functions
/**
 * \brief Sends an API packet to the XBee via serial.
 * 
 * @param data        The packet payload to send
 * @param dataLength  The size in bytes of the payload
 */
void sendApiCommand(byte data[], unsigned int dataLength)
{
    // Construct an output packet
    unsigned int packetLength = dataLength + 4;
    byte outputPacket[PACKET_BUFFER_LENGTH];

    // First byte is start delimiter
    outputPacket[0] = 0x7E;

    // Second two bytes are the length of the packet's payload only
    outputPacket[1] = ((dataLength >> 8) & 0xFF); // MSB
    outputPacket[2] = (dataLength & 0xFF); // LSB

    // Now we copy our payload
    unsigned int checksumTotal = 0;
    for (unsigned int i = 0; i < dataLength; ++i)
    {
        outputPacket[3 + i] = data[i];
        checksumTotal += data[i];
    }

    // Calculate checksum and set as last byte in packet
    byte checksum = 0xFF - (checksumTotal & 0xFF);
    outputPacket[packetLength - 1] = checksum;

    // Send
    for (unsigned int i = 0; i < packetLength; ++i)
    {
        Serial.write(outputPacket[i]);
    }
}

/**
 * \brief Sends an AT command via API mode to the XBee
 * 
 * @param frameId         Identifies the data frame for the host to correlate with a subsequent response.
 *                        If set to 0, the device does not send a response.
 * @param commandName     Two ASCII characters that identify the AT command.
 * @param parameterValue  If present, indicates the requested parameter value to set the given register.
 *                        If no characters are present, it queries the register.
 * @param parameterLength The length of the parameterValue array.
 */
void sendAtCommand(byte frameId, byte commandName[], byte parameterValue[], unsigned int parameterLength)
{
    // Set up our payload
    unsigned int payloadLength = 4 + parameterLength;
    byte payload[PACKET_BUFFER_LENGTH];
    payload[0] = 0x08; // Frame type = 0x08 for AT command
    payload[1] = frameId;
    payload[2] = commandName[0];
    payload[3] = commandName[1];
    for (unsigned int i = 0; i < parameterLength; ++i)
    {
        payload[4 + i] = parameterValue[i];
    }

    // Send it!
    sendApiCommand(payload, payloadLength);
}

/**
 * \brief Sends the AI AT command to the XBee via API mode.
 * 
 * The AI command is used to read information regarding last node join request.
 * @return The frame ID of the frame sent
 */
byte SendAiAtCommand()
{
    byte frameId = g_xBeeStatus.lastFrameId++;
    byte atCommand[] = {'A', 'I'};
    sendAtCommand(frameId, atCommand, nullptr, 0);
    return frameId;
}

/**
 * \brief Sends an explicit addressing command frame (0x11)
 * \sa https://www.digi.com/resources/documentation/digidocs/90001539/reference/r_frame_0x11.htm
 * 
 * @param frameId                  Identifies the data frame for the host to correlate with a subsequent response.
 *                                 If set to 0, the device does not send a response.
 * @param destinationLongAddress   The 64-bit address of the destination device. 
 *                                 Reserved 64-bit address for the coordinator = 0x0000000000000000
 *                                 Broadcast = 0x000000000000FFFF
 * @param destinationShortAddress  Set to the 16-bit address of the destination device, if known.
 *                                 Set to 0xFFFE if the address is unknown, or if sending a broadcast.
 * @param sourceEndpoint           Source Endpoint for the transmission.
 * @param destinationEndpoint      Destination Endpoint for the transmission.
 * @param clusterId                Cluster ID used in the transmission.
 * @param profileId                Profile ID used in the transmission.
 * @param broadcastRadius          Sets the maximum number of hops a broadcast transmission can traverse.
 *                                 If set to 0, the device sets the transmission radius to the network maximum hops value.
 * @param transmissionOptions      Bitfield of supported transmission options. Supported values include the following:
 *                                 0x01 - Disable retries {newline} 0x04- Indirect Addressing
 *                                 0x08 - Multicast Addressing
 *                                 0x20 - Enable APS encryption (if EE = 1)
 *                                 0x40 - Use the extended transmission timeout for this destination
 * @param payload                  Data payload
 * @param payloadLength            Size of the data payload
 */
void SendExplicitAddressingCommandFrame(
    byte frameId,
    unsigned long long destinationLongAddress,
    unsigned int destinationShortAddress,
    byte sourceEndpoint,
    byte destinationEndpoint,
    unsigned int clusterId,
    unsigned int profileId,
    byte broadcastRadius,
    byte transmissionOptions,
    byte payload[],
    unsigned int payloadLength
)
{
    byte outgoingData[PACKET_BUFFER_LENGTH];
    unsigned int outgoingDataLength = 20 + payloadLength;

    // Frame type
    outgoingData[0] = 0x11;

    // Frame ID
    outgoingData[1] = frameId;

    // Extract destination long address, most significant bytes first
    for (unsigned int i = 0; i < 8; ++i)
    {
        outgoingData[2 + i] = (destinationLongAddress >> (8 * (7 - i))) & 0xFF;
    }

    // Extract destination short address, most significant bytes first
    for (unsigned int i = 0; i < 2; ++i)
    {
        outgoingData[10 + i] = (destinationShortAddress >> (8 * (1 - i))) & 0xFF;
    }

    // Source endpoint
    outgoingData[12] = sourceEndpoint;

    // Destination endpoint
    outgoingData[13] = destinationEndpoint;

    // Extract cluster id, most significant bytes first
    for (unsigned int i = 0; i < 2; ++i)
    {
        outgoingData[14 + i] = (clusterId >> (8 * (1 - i))) & 0xFF;
    }

    // Extract profile id, most significant bytes first
    for (unsigned int i = 0; i < 2; ++i)
    {
        outgoingData[16 + i] = (profileId >> (8 * (1 - i))) & 0xFF;
    }

    // Broadcast radius
    outgoingData[18] = broadcastRadius;

    // Transmission options
    outgoingData[19] = transmissionOptions;

    // Copy the rest of the data payload
    for (unsigned int i = 0; i < payloadLength; ++i)
    {
        outgoingData[20 + i] = payload[i];
    }

    // Send it off!
    sendApiCommand(outgoingData, outgoingDataLength);
}
#pragma endregion
/**
 * @file ArduinoXBeeZigbeeHACommunication.ino
 * @author Hayden McAfee <hayden@outlook.com>
 * @date 30 Oct 2018
 * 
 * This is an Arduino sketch meant to power a simple Zigbee home automation
 * device via an XBee device over serial configured to operate in API mode.
 */

#define PACKET_BUFFER_LENGTH 128

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
    char payload[PACKET_BUFFER_LENGTH];
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

void setup()
{
    // Set up XBee state
    g_xBeeStatus.isConnected = false;
    g_xBeeStatus.lastFrameId = 0x00;

    // Open serial port to XBee
    Serial.begin(9600);
    Serial.write("***START***");
}

void loop()
{
    // Process incoming commands
    if (Serial.available() > 0)
    {
        Serial.print(" ");
        Serial.print("* SIN *");
        XBeePacket packet;
        if (readIncomingPacket(&packet))
        {
            Serial.print("INCOMING");
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
                Serial.print("* RXFRAME *");
                XBeeIncomingRxPacket incomingRxPacket;
                ParseExplicitRxFrame(&packet, &incomingRxPacket);
                ProcessExplicitRxFrame(&incomingRxPacket);
                break;
            default:
                Serial.print("* UNKNOWN PACKET *");
                Serial.print(frameType);
                break;
            }
        }
        else
        {
            // Bad packet
            Serial.write("* BAD PACKET *");
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

        Serial.print("**PAYLOAD LEN: ");
        Serial.print(payloadLength);
        Serial.print("**");

        // Extract payload
        byte payload[PACKET_BUFFER_LENGTH];
        unsigned int checksum = 0;
        while (Serial.available() < payloadLength + 1);
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
 * \brief Sends an API packet to the XBee via serial.
 * 
 * @param data        The packet payload to send
 * @param dataLength  The size in chars of the payload
 */
void sendApiCommand(byte data[], unsigned int dataLength)
{
    // Construct an output packet
    unsigned int packetLength = dataLength + 4;
    char outputPacket[PACKET_BUFFER_LENGTH];

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
    char checksum = 0xFF - (checksumTotal & 0xFF);
    outputPacket[packetLength - 1] = checksum;

    // Send
    for (unsigned int i = 0; i < packetLength; ++i)
    {
        Serial.print(outputPacket[i]);
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

#pragma region Incoming packet processing functions
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
    unsigned long long longSourceAddress = 0;
    for (unsigned int i = 1; i < 9; ++i)
    {
        longSourceAddress = (longSourceAddress << 8) & inPacket->payload[i];
    }

    // Pull out 16-bit source address from bytes 9 - 10
    unsigned int shortSourceAddress = 0;
    for (unsigned int i = 9; i < 11; ++i)
    {
        shortSourceAddress = (shortSourceAddress << 8) & inPacket->payload[i];
    }

    byte sourceEndpoint = inPacket->payload[11];
    byte destinationEndpoint = inPacket->payload[12];

    // Pull out cluster ID from bytes 13 - 14
    unsigned int clusterId = 0;
    for (unsigned int i = 13; i < 15; ++i)
    {
        clusterId = (clusterId << 8) & inPacket->payload[i];
    }

    // Pull out profile ID from bytes 15 - 16
    unsigned int profileId = 0;
    for (unsigned int i = 15; i < 17; ++i)
    {
        profileId = (profileId << 8) & inPacket->payload[i];
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
    // Basic cluster
    if (inPacket->clusterId == 0x0000)
    {
        Serial.write("*BASIC CLUSTER*");
        ZCLFrameData zclFrameData;
        ParseZCLFrame(inPacket, &zclFrameData);

        // Read Attribute command
        if (zclFrameData.commandIdentifier == 0x00)
        {
            Serial.write("*READ ATTRIBUTE*");

            for (unsigned int i = 0; i < zclFrameData.payloadLength; ++i)
            {
                switch (zclFrameData.payload[i])
                {
                case 0x04:
                    Serial.write("*MANUFACTURER ATTRIBUTE*");
                    break;
                }
            }
        }
    }
}
#pragma endregion

#pragma region Outgoing packet functions
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
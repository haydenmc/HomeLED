/**
 * @file ArduinoXBeeZigbeeHACommunication.ino
 * @author Hayden McAfee <hayden@outlook.com>
 * @date 30 Oct 2018
 * 
 * This is an Arduino sketch meant to power a simple Zigbee home automation
 * device via an XBee device over serial configured to operate in API mode.
 */

/**
 * Struct representing a generic XBee packet
 */
struct XBeePacket
{
    unsigned int payloadLength;
    char payload[128];
};

/**
 * Struct representing XBee state
 */
struct XBee
{
    bool isConnected;
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
}

void loop()
{
    // If we're not connected, let's ask why
    if (!g_xBeeStatus.isConnected)
    {
        // Send Association Indication command to get status
        SendAiAtCommand();
        delay(5000);
    }

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
                Serial.print("AT COMMAND RESPONSE FRAME");
                break;
            }
        }
        else
        {
            // Bad packet
        }
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
        byte lengthMsb = Serial.read();
        byte lengthLsb = Serial.read();
        unsigned int payloadLength = lengthMsb;
        payloadLength = (payloadLength << 8) | lengthLsb;

        // Extract payload
        byte payload[payloadLength];
        unsigned int checksum = 0;
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
void sendApiCommand(char data[], unsigned int dataLength)
{
    // Construct an output packet
    unsigned int packetLength = dataLength + 4;
    char outputPacket[packetLength];

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
void sendAtCommand(char frameId, char commandName[], char parameterValue[], unsigned int parameterLength)
{
    // Set up our payload
    unsigned int payloadLength = 4 + parameterLength;
    char payload[payloadLength];
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
    char atCommand[] = {'A', 'I'};
    sendAtCommand(frameId, atCommand, nullptr, 0);
    return frameId;
}
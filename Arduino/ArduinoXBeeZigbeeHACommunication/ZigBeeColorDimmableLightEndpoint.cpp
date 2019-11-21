#include "ZigBeeColorDimmableLightEndpoint.h"

ZigBeeColorDimmableLightEndpoint::ZigBeeColorDimmableLightEndpoint()
{
    this->lightIsOn = false;
    // D65: (0.31271, 0.32902)
    this->lightX = 0.31271 * 0xFFFF;
    this->lightY = 0.32902 * 0xFFFF;
    this->lightLevel = 0xFFFF;
}

unsigned int ZigBeeColorDimmableLightEndpoint::GetProfileId()
{
    return 0x0104; // Home Automation Profile ID
}

unsigned int ZigBeeColorDimmableLightEndpoint::GetDeviceId()
{
    return 0x0102; // Color Dimmable Light Device ID
}

unsigned int ZigBeeColorDimmableLightEndpoint::GetDeviceVersion()
{
    return 0x0000; // Version Number
}

unsigned int ZigBeeColorDimmableLightEndpoint::GetX()
{
    return this->lightX;
}

unsigned int ZigBeeColorDimmableLightEndpoint::GetY()
{
    return this->lightY;
}

unsigned int ZigBeeColorDimmableLightEndpoint::GetLevel()
{
    if (this->lightIsOn)
    {
        return this->lightLevel;
    }
    else
    {
        return 0x0000; // Off
    }
}

void ZigBeeColorDimmableLightEndpoint::ProcessFrame(const ZigBeeClusterLibraryFrame* frame)
{
    switch (frame->clusterId)
    {
        // Groups Cluster
        case 0x0004:
        {
            // TODO
            break;
        }

        // Scenes Cluster
        case 0x0005:
        {
            // TODO
            break;
        }

        // On/Off Cluster
        case 0x0006:
        {
            this->ProcessOnOffCommand(frame);
            break;
        }

        // Level Control Cluster
        case 0x0008:
        {
            this->ProcessLevelControlCommand(frame);
            break;
        }

        // Color Control Cluster
        case 0x0300:
        {
            this->ProcessColorControlCommand(frame);
            break;
        }
    }
}

void ZigBeeColorDimmableLightEndpoint::ProcessOnOffCommand(const ZigBeeClusterLibraryFrame* frame)
{
    switch (frame->commandIdentifier)
    {
        // Off
        case 0x00:
        {
            this->lightIsOn = false;
            break;
        }

        // On
        case 0x01:
        {
            this->lightIsOn = true;
            break;
        }

        // Toggle
        case 0x02:
        {
            this->lightIsOn = !this->lightIsOn;
            break;
        }
    }
}

void ZigBeeColorDimmableLightEndpoint::ProcessLevelControlCommand(const ZigBeeClusterLibraryFrame* frame)
{
    switch (frame->commandIdentifier)
    {
        // Move to level
        case 0x00:
        {
            // TODO: Transition time
            this->lightLevel = (frame->payload[0] / (float)(0xFF)) * 0xFFFF;
            break;
        }
        
        // Move
        case 0x01:
        {
            // TODO
            break;
        }

        // Step
        case 0x02:
        {
            // TODO
            break;
        }

        // Stop
        case 0x03:
        {
            // TODO
            break;
        }

        // Move to Level (with On/Off)
        case 0x04:
        {
            // TODO
            break;
        }

        // Move (with On/Off)
        case 0x05:
        {
            // TODO
            break;
        }

        // Step (with On/Off)
        case 0x06:
        {
            // TODO
            break;
        }

        // Stop
        case 0x07:
        {
            // TODO
            break;
        }
    }
}

void ZigBeeColorDimmableLightEndpoint::ProcessColorControlCommand(const ZigBeeClusterLibraryFrame* frame)
{
    switch (frame->commandIdentifier)
    {
        // Move to Hue
        case 0x00:
        {
            break;
        }
        
        // Move Hue
        case 0x01:
        {
            break;
        }

        // Step Hue
        case 0x02:
        {
            break;
        }

        // Move to Saturation
        case 0x03:
        {
            break;
        }

        // Move Saturation
        case 0x04:
        {
            break;
        }

        // Step Saturation
        case 0x05:
        {
            break;
        }

        // Move to Hue and Saturation
        case 0x06:
        {
            break;
        }
        
        // Stop Move Step
        case 0x47:
        {
            break;
        }
    }
}
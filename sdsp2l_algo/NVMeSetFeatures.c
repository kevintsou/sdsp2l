#include <windows.h>
#include <stdio.h>
#include <nvme.h>

#include "NVMeUtils.h"
#include "NVMeFeaturesHCTM.h"
#include "NVMeFeaturesVWC.h"

int iNVMeSetFeatures(HANDLE _hDevice)
{
    int iResult = -1;
    int iFId = 0;

    char cCmd;
    char strCmd[256];
    char strPrompt[1024];

    sprintf_s(strPrompt,
        1024,
        "\n# Input Feature Identifier (in hex):"
        "\n#    Supported Features are:"
        "\n#     %02Xh = Volatile Write Cache"
        "\n#     %02Xh = Host Controlled Thermal Management"
        "\n",
        NVME_FEATURE_VOLATILE_WRITE_CACHE,
        NVME_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT);

    iFId = iGetConsoleInputHex((const char*)strPrompt, strCmd);
    switch (iFId)
    {
    case NVME_FEATURE_VOLATILE_WRITE_CACHE:
        cCmd = cGetConsoleInput("\n# Set Feature - Volatile Write Cache (Feature Identifier = 06h), Press 'y' to continue\n", strCmd);
        if (cCmd == 'y')
        {
            iResult = iNVMeSetFeaturesVWC(_hDevice);
        }
        break;

    case NVME_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT:
        cCmd = cGetConsoleInput("\n# Set Feature - Host Controlled Thermal Management (Feature Identifier = 10h), Press 'y' to continue\n", strCmd);
        if (cCmd == 'y')
        {
            iResult = iNVMeSetFeaturesHCTM(_hDevice);
        }
        break;

    default:
        fprintf(stderr, "\n[E] Feature is not implemented yet.\n");
        break;
    }

    printf("\n");
    return iResult;
}

static int siiNVMeSetFeaturesVWC(HANDLE _hDevice, DWORD _cdw10, DWORD _cdw11)
{
    int     iResult = -1;
    PVOID   buffer;
    ULONG   bufferLength = 0;
    ULONG   returnedLength = 0;

    PSTORAGE_PROPERTY_SET query = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR_EXT protocolDataDescr = NULL;

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_SET, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA_EXT);
    buffer = malloc(bufferLength);

    if (buffer == NULL)
    {
        vPrintSystemError(GetLastError(), "malloc");
        return iResult;
    }

    ZeroMemory(buffer, bufferLength);

    query = (PSTORAGE_PROPERTY_SET)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR_EXT)buffer;
    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT)(query->AdditionalParameters);

    query->PropertyId = StorageAdapterProtocolSpecificProperty; // StorageDeviceProtocolSpecificProperty;
    query->SetType = PropertyStandardSet;

    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeFeature;
    protocolData->ProtocolDataValue = _cdw10;
    protocolData->ProtocolDataSubValue = _cdw11;
    protocolData->ProtocolDataOffset = 0;
    protocolData->ProtocolDataLength = 0;

    // Send request down.  
    iResult = iIssueDeviceIoControl(_hDevice,
        IOCTL_STORAGE_SET_PROPERTY,
        buffer,
        bufferLength,
        buffer,
        bufferLength,
        &returnedLength,
        NULL
    );

    if (iResult == 0)
    {
        // Validate the returned data.
        if ((protocolDataDescr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
            (protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)))
        {
            fprintf(stderr, "[E] NVMeSetFeature: Data descriptor header not valid.\n");
            iResult = -1; // error
        }
    }

    if (buffer != NULL)
    {
        free(buffer);
    }

    return iResult;
}

int siNVMeSetFeatures(HANDLE _hDevice, DWORD _cdw10, DWORD _cdw11)
{
    int     iResult = -1;
    PVOID   buffer = NULL;
    ULONG   bufferLength = 0;
    ULONG   returnedLength = 0;

    PSTORAGE_PROPERTY_SET setProperty = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT protocolData = NULL;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR_EXT protocolDataDescr = NULL;

    // Allocate buffer for use.
    bufferLength = offsetof(STORAGE_PROPERTY_SET, AdditionalParameters)
        + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA_EXT);
    bufferLength += NVME_MAX_LOG_SIZE;
    buffer = malloc(bufferLength);

    if (buffer == NULL)
    {
        vPrintSystemError(GetLastError(), "malloc");
        return iResult;
    }

    ZeroMemory(buffer, bufferLength);

    setProperty = (PSTORAGE_PROPERTY_SET)buffer;
    protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR_EXT)buffer;
 
    setProperty->PropertyId = StorageAdapterProtocolSpecificProperty;
    setProperty->SetType = PropertyStandardSet;

    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT)setProperty->AdditionalParameters;
    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeFeature;
    protocolData->ProtocolDataValue = _cdw10;
    protocolData->ProtocolDataSubValue = _cdw11;
    protocolData->ProtocolDataSubValue2 = 0; // This will pass to CDW12.
    protocolData->ProtocolDataSubValue3 = 0; // This will pass to CDW13.
    protocolData->ProtocolDataSubValue4 = 0; // This will pass to CDW14.
    protocolData->ProtocolDataSubValue5 = 0; // This will pass to CDW15.

    protocolData->ProtocolDataOffset = 0;
    protocolData->ProtocolDataLength = 0;

    // Send request down.  
    iResult = iIssueDeviceIoControl(_hDevice,
        IOCTL_STORAGE_SET_PROPERTY,
        buffer,
        bufferLength,
        buffer,
        bufferLength,
        &returnedLength,
        NULL
    );

    if (iResult == 0)
    {
        printf("\n[I] Command Set Features succeeded.\n");
    }

    if (buffer != NULL)
    {
        free(buffer);
    }

    return iResult;
}
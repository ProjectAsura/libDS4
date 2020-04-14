//-----------------------------------------------------------------------------
// File : ds4_pad.cpp
// Desc : Dual Shock4 Game Pad Library.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <atomic>
#include <string>
#include <vector>
#include <ds4_pad.h>
#include <Windows.h>
#include <hidsdi.h>
#include <SetupAPI.h>
#include <Bthsdpdef.h>
#include <BluetoothAPIs.h>


namespace {

//-----------------------------------------------------------------------------
// Constant Values.
//-----------------------------------------------------------------------------
// Vendor ID.
static const uint16_t kSonyCorp = 0x054c;

// Product ID.
static const uint16_t kDualShockWirelessAdaptor = 0x0ba0;
static const uint16_t kDualShock4_CUH_ZCT1x     = 0x05c4;  // CUH-ZCT1x
static const uint16_t kDualShock4_CUH_ZCT2x     = 0x09cc;  // CUH-ZCT2x

// For Bluetooth.
static const char  kBlankSerial[] = "00:00:00:00:00:00";
static const int   IOCTL_BTH_DISCONNECT_DEVICE = 0x41000c;

static const float kAccelResPerG    = 8192.0f;
static const float kGyroResInDegSec = 16.0f;


} // namespace


///////////////////////////////////////////////////////////////////////////////
// PadHandle structure
///////////////////////////////////////////////////////////////////////////////
struct PadHandle
{
    HANDLE                  Handle;
    std::wstring            DevicePath;
    uint32_t                Size;
    PAD_CONNECTION_TYPE     Type;
    std::string             MacAddress;
};

//-----------------------------------------------------------------------------
//      パッドを接続します.
//-----------------------------------------------------------------------------
bool PadOpen(PadHandle** ppHandle)
{
    if (ppHandle == nullptr)
    { return false; }

    GUID guid;
    HidD_GetHidGuid(&guid);

    auto info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (info == nullptr)
    { return false; }

    SP_DEVICE_INTERFACE_DATA devInfoData;
    devInfoData.cbSize = sizeof(devInfoData);

    ULONG length    = 0;
    ULONG required  = 0;
    auto  index     = 0u;
    auto  deviceDetected = false;

    PAD_CONNECTION_TYPE type;
    DWORD               size = 0;
    HANDLE              handle = nullptr;
    std::wstring        devicePath;
    std::string         macAddress;

    for(; deviceDetected == false; index++)
    {
        auto ret = SetupDiEnumDeviceInterfaces(info, 0, &guid, index, &devInfoData);
        if (ret == FALSE)
        { return false; }

        // サイズ取得.
        SetupDiGetDeviceInterfaceDetail(info, &devInfoData, NULL, 0, &length, NULL);

        auto detailData = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(length));
        if (detailData == nullptr)
        {
            auto errcode = GetLastError();
            return false;
        }
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        ret = SetupDiGetDeviceInterfaceDetail(info, &devInfoData, detailData, length, &required, NULL);
        if (ret == FALSE)
        {
            auto errcode = GetLastError();
            free(detailData);
            detailData = nullptr;
            return false;
        }

        handle = CreateFile(
            detailData->DevicePath,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            FILE_SHARE_READ   | FILE_SHARE_WRITE,
            (LPSECURITY_ATTRIBUTES)NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (handle == nullptr)
        {
            free(detailData);
            detailData = nullptr;
            continue;
        }

        HIDD_ATTRIBUTES attributes = {};
        ret = HidD_GetAttributes(handle, &attributes);
        if (ret == FALSE)
        {
            free(detailData);
            detailData = nullptr;

            CloseHandle(handle);
            handle = nullptr;

            continue;
        }

        if (attributes.VendorID != kSonyCorp)
        {
            free(detailData);
            detailData = nullptr;

            CloseHandle(handle);
            handle = nullptr;

            continue;
        }

        if (attributes.ProductID == kDualShockWirelessAdaptor)
        {
            deviceDetected = true;
            type = PAD_CONNECTION_WIRELESS;

            PHIDP_PREPARSED_DATA preparsedData;
            HidD_GetPreparsedData(handle, &preparsedData);

            HIDP_CAPS capabilities;
            HidP_GetCaps(preparsedData, &capabilities);

            size = capabilities.FeatureReportByteLength;

            HidD_FreePreparsedData(preparsedData);
        }
        else if (attributes.ProductID == kDualShock4_CUH_ZCT1x)
        {
            deviceDetected = true;

            PHIDP_PREPARSED_DATA preparsedData;
            HidD_GetPreparsedData(handle, &preparsedData);

            HIDP_CAPS capabilities;
            HidP_GetCaps(preparsedData, &capabilities);

            if (capabilities.InputReportByteLength == 64)
            {
                type = PAD_CONNECTION_USB;

                char buf[16];
                buf[0] = 18;
                if (HidD_GetFeature(handle, buf, 16) == TRUE)
                { macAddress = buf; }
            }
            else
            {
                type = PAD_CONNECTION_BT;

                char buf[126];
                if (HidD_GetSerialNumberString(handle, buf, 126) == TRUE)
                { macAddress = buf; }
            }

            size = capabilities.FeatureReportByteLength;

            HidD_FreePreparsedData(preparsedData);
        }
        else if (attributes.ProductID == kDualShock4_CUH_ZCT2x)
        {
            deviceDetected = true;

            PHIDP_PREPARSED_DATA preparsedData;
            HidD_GetPreparsedData(handle, &preparsedData);

            HIDP_CAPS capabilities;
            HidP_GetCaps(preparsedData, &capabilities);

            if (capabilities.InputReportByteLength == 64)
            {
                type = PAD_CONNECTION_USB;

                char buf[16];
                buf[0] = 18;
                if (HidD_GetFeature(handle, buf, 16) == TRUE)
                { macAddress = buf; }
            }
            else
            {
                type = PAD_CONNECTION_BT;

                char buf[126];
                if (HidD_GetSerialNumberString(handle, buf, 126) == TRUE)
                { macAddress = buf; }
            }

            size = capabilities.FeatureReportByteLength;
            HidD_FreePreparsedData(preparsedData);
        }

        devicePath = detailData->DevicePath;

        free(detailData);
        detailData = nullptr;
    }

    // ハンドル生成.
    auto pad = new PadHandle;
    pad->Handle     = handle;
    pad->DevicePath = devicePath;
    pad->Size       = size;
    pad->Type       = type;
    pad->MacAddress = macAddress;

    *ppHandle = pad;

    return true;
}

//-----------------------------------------------------------------------------
//      パッドを切断します.
//-----------------------------------------------------------------------------
bool PadClose(PadHandle*& pHandle)
{
    if (pHandle == nullptr)
    { return false; }

    if (pHandle->Type == PAD_CONNECTION_BT)
    {
        // TODO: Bluetooth切断処理.
#if 0
        //BLUETOOTH_FIND_RADIO_PARAMS params;
        //params.dwSize = sizeof(BLUETOOTH_FIND_RADIO_PARAMS);

        //HANDLE handleBT = nullptr;
        //auto handleSearch = BluetoothFindFirstRadio(&params, &handleBT);

        //uint8_t btAddress[8];
        //DWORD length = 0;
        //BOOL ret = FALSE;
        //while(!ret && handleBT != nullptr)
        //{
        //    ret = DeviceIoControl(handleBT, IOCTL_BTH_DISCONNECT_DEVICE, btAddress, 8, NULL, 0, &length, NULL);
        //    CloseHandle(handleBT);
        //    if (!ret)
        //    {
        //        if (!BluetoothFindNextRadio(handleSearch, &handleBT))
        //        { handleBT = nullptr; }
        //    }
        //}

        //BluetoothFindRadioClose(handleSearch);
#endif
    }

    if (pHandle->Handle != nullptr)
    {
        PadColor color = {};
        PadVibrationParam vibrate = {};

        PadSetLightBarColor(pHandle, &color);
        PadSetVibration(pHandle, &vibrate);

        CloseHandle(pHandle->Handle);
        pHandle->Handle = nullptr;
    }

    // ハンドル破棄.
    delete pHandle;
    pHandle = nullptr;

    return true;
}

//-----------------------------------------------------------------------------
//      パッド生データを読み取ります.
//-----------------------------------------------------------------------------
bool PadRead(PadHandle* pHandle, PadRawInput* pResult)
{
    if (pHandle == nullptr || pResult == nullptr)
    { return false; }

    if (pHandle->Handle == nullptr)
    { return false; }

    auto ret = ReadFile(pHandle->Handle, pResult->Bytes, pHandle->Size, nullptr, nullptr);
    pResult->Type = pHandle->Type;

    return (ret == TRUE);
}

//-----------------------------------------------------------------------------
//      パッドデータを扱いやすい形にマッピングします.
//-----------------------------------------------------------------------------
bool PadMap(const PadRawInput* pRawData, PadState& state)
{
    if (pRawData == nullptr)
    { return false; }

    const uint8_t* input = &pRawData->Bytes[0];

    if (pRawData->Type == PAD_CONNECTION_NONE)
    { 
        memset(&state, 0, sizeof(state));
    }
    else if (pRawData->Type == PAD_CONNECTION_USB)
    {
        // 64 bytes.
        state.Type = PAD_CONNECTION_USB;
    }
    else if (pRawData->Type == PAD_CONNECTION_WIRELESS)
    {
        // 64 bytes.
        state.Type = PAD_CONNECTION_WIRELESS;
    }
    else if (pRawData->Type == PAD_CONNECTION_BT)
    {
        // 78 bytes.
        state.Type = PAD_CONNECTION_BT;
        input = &pRawData->Bytes[2];
    }

    state.StickL.X          = input[1];
    state.StickL.Y          = input[2];
    state.StickR.X          = input[3];
    state.StickR.Y          = input[4];
    state.AnalogButtons.L2  = input[8];
    state.AnalogButtons.R2  = input[9];
    state.Buttons           = uint16_t(input[5] | (input[6] << 8));
    state.SpecialButtons    = input[7] & 0x3;

#if 0
    auto touch_count = 0;
    if ((input[35] & 0x80) == 0)
    { touch_count++; }

    if ((input[39] & 0x80) == 0)
    { touch_count++; }

    auto gyroX  = int16_t((input[13] << 8) | input[14]);
    auto gyroY  = int16_t((input[15] << 8) | input[16]);
    auto gyroZ  = int16_t((input[17] << 8) | input[18]);

    auto accelX = int16_t((input[19] << 8) | input[20]);
    auto accelY = int16_t((input[21] << 8) | input[22]);
    auto accelZ = int16_t((input[23] << 8) | input[24]);

    state.AngularVelocity.X = -gyroX / kGyroResInDegSec;
    state.AngularVelocity.Y =  gyroY / kGyroResInDegSec;
    state.AngularVelocity.Z = -gyroZ / kGyroResInDegSec;

    state.Acceleration.X = -accelX / kAccelResPerG;
    state.Acceleration.Y = -accelY / kAccelResPerG;
    state.Acceleration.Z =  accelZ / kAccelResPerG;

    state.TouchData.Count = touch_count;
    state.TouchData.Touch[0].id = (input[35] & 0x7f);
    state.TouchData.Touch[0].x = static_cast<uint16_t>((uint16_t(input[37] & 0xf) << 8) | input[36]);
    state.TouchData.Touch[0].y = static_cast<uint16_t>((uint16_t(input[38] << 4)) | ((input[37] & 0xf0) >> 4));

    state.TouchData.Touch[1].id = (input[39] & 0x7f);
    state.TouchData.Touch[1].x = static_cast<uint16_t>((uint16_t(input[41] & 0xf) << 8) | input[40]);
    state.TouchData.Touch[1].y = static_cast<uint16_t>((uint16_t(input[42] << 4)) | ((input[41] & 0xf0) >> 4));
#endif

    return true;
}

//-----------------------------------------------------------------------------
//      パッドデータを読み取ります.
//-----------------------------------------------------------------------------
bool PadGetState(PadHandle* pHandle, PadState& state)
{
    PadRawInput pResult;
    if (!PadRead(pHandle, &pResult))
    { return false; }

    if (!PadMap(&pResult, state))
    { return false; }

    return true;
}

//-----------------------------------------------------------------------------
//      バイブレーションを設定します.
//-----------------------------------------------------------------------------
bool PadSetVibration(PadHandle* pHandle, const PadVibrationParam* pParam)
{
    if (pHandle == nullptr || pParam == nullptr)
    { return false; }

    if (pHandle->Handle == nullptr)
    { return false; }

    uint8_t bytes[32] = {};
    if (pHandle->Type == PAD_CONNECTION_BT)
    {
        bytes[0] = 0x11;
        bytes[1] = 0xb0;
        bytes[3] = 0xf1;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[6] = pParam->LargeMotor;
        bytes[7] = pParam->SmallMotor;
    }
    else
    {
        bytes[0] = 0x05;
        bytes[1] = 0xf1;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[4] = pParam->LargeMotor;
        bytes[5] = pParam->SmallMotor;
    }

    auto size = WriteFile(pHandle->Handle, bytes, 32, nullptr, nullptr);
    if (size != 32)
    { return false; }

    return true;
}

//-----------------------------------------------------------------------------
//      ライトバーカラーを設定します.
//-----------------------------------------------------------------------------
bool PadSetLightBarColor(PadHandle* pHandle, const PadColor* pParam)
{
    if (pHandle == nullptr || pParam == nullptr)
    { return false; }

    if (pHandle->Handle == nullptr)
    { return false; }

    uint8_t bytes[32] = {};
    if (pHandle->Type == PAD_CONNECTION_BT)
    {
        bytes[0]  = 0x11;
        bytes[1]  = 0xb0;
        bytes[3]  = 0xf6;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[8]  = pParam->R;
        bytes[9]  = pParam->G;
        bytes[10] = pParam->B;
    }
    else
    {
        bytes[0] = 0x05;
        bytes[1] = 0xf6;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[6] = pParam->R;
        bytes[7] = pParam->G;
        bytes[8] = pParam->B;
    }

    auto size = WriteFile(pHandle->Handle, bytes, 32, nullptr, nullptr);
    if (size != 32)
    { return false; }

    return true;
}
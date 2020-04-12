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


//-----------------------------------------------------------------------------
// Global Variables.
//-----------------------------------------------------------------------------
static GUID                     gHidGuid        = {};
static std::atomic<bool>        gIsInit         = false;

} // namespace


///////////////////////////////////////////////////////////////////////////////
// PadHandle structure
///////////////////////////////////////////////////////////////////////////////
struct PadHandle
{
    HANDLE                  handle;
    std::wstring            devicePath;
    uint32_t                size;
    PAD_CONNECTION_TYPE     type;
};

//-----------------------------------------------------------------------------
//      初期化処理を行います.
//-----------------------------------------------------------------------------
bool PadInit()
{
    if (gIsInit)
    { return true; }

    HidD_GetHidGuid(&gHidGuid);

    gIsInit = true;

    return true;
}

//-----------------------------------------------------------------------------
//      終了処理を行います.
//-----------------------------------------------------------------------------
void PadTerm()
{
    gIsInit = false;
}

//-----------------------------------------------------------------------------
//      パッドを接続します.
//-----------------------------------------------------------------------------
bool PadOpen(PadHandle** ppHandle)
{
    if (ppHandle == nullptr)
    { return false; }

    if (!gIsInit)
    { return false; }

    auto info = SetupDiGetClassDevs(&gHidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
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

    for(; deviceDetected == false; index++)
    {
        auto ret = SetupDiEnumDeviceInterfaces(info, 0, &gHidGuid, index, &devInfoData);
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
            { type = PAD_CONNECTION_USB; }
            else
            { type = PAD_CONNECTION_BT; }

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
            { type = PAD_CONNECTION_USB; }
            else
            { type = PAD_CONNECTION_BT; }

            size = capabilities.FeatureReportByteLength;
            HidD_FreePreparsedData(preparsedData);
        }

        devicePath = detailData->DevicePath;

        free(detailData);
        detailData = nullptr;
    }

    // ハンドル生成.
    auto pad = new PadHandle;
    pad->handle     = handle;
    pad->devicePath = devicePath;
    pad->size       = size;
    pad->type       = type;

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

    if (pHandle->type == PAD_CONNECTION_BT)
    {
        // TODO: Bluetooth切断処理.
    }

    if (pHandle->handle != nullptr)
    {
        CloseHandle(pHandle->handle);
        pHandle->handle = nullptr;
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

    if (pHandle->handle == nullptr)
    { return false; }

    auto ret = ReadFile(pHandle->handle, pResult->bytes, pHandle->size, nullptr, nullptr);
    pResult->type = pHandle->type;

    return (ret == TRUE);
}

//-----------------------------------------------------------------------------
//      パッドデータを扱いやすい形にマッピングします.
//-----------------------------------------------------------------------------
bool PadMap(const PadRawInput* pRawData, PadState& state)
{
    if (pRawData == nullptr)
    { return false; }

    const uint8_t* input = &pRawData->bytes[0];

    if (pRawData->type == PAD_CONNECTION_NONE)
    { 
        memset(&state, 0, sizeof(state));
    }
    else if (pRawData->type == PAD_CONNECTION_USB)
    {
        // 64 bytes.
        state.type = PAD_CONNECTION_USB;
    }
    else if (pRawData->type == PAD_CONNECTION_WIRELESS)
    {
        // 64 bytes.
        state.type = PAD_CONNECTION_WIRELESS;
    }
    else if (pRawData->type == PAD_CONNECTION_BT)
    {
        // 78 bytes.
        state.type = PAD_CONNECTION_BT;
        input = &pRawData->bytes[2];
    }


    state.stickL.x = input[1];
    state.stickL.y = input[2];
    state.stickR.x = input[3];
    state.stickR.y = input[4];
    state.analogButtons.l2 = input[8];
    state.analogButtons.r2 = input[9];

    uint32_t mask = 0;

    auto dpad = input[5] & 0x0f;
    switch(dpad)
    {
    case 0:
        {
            mask |= PAD_BUTTON_UP;
        }
        break;

    case 1:
        {
            mask |= PAD_BUTTON_UP;
            mask |= PAD_BUTTON_RIGHT;
        }
        break;

    case 2:
        {
            mask |= PAD_BUTTON_RIGHT;
        }
        break;

    case 3:
        {
            mask |= PAD_BUTTON_DOWN;
            mask |= PAD_BUTTON_RIGHT;
        }
        break;

    case 4:
        {
            mask |= PAD_BUTTON_DOWN;
        }
        break;

    case 5:
        {
            mask |= PAD_BUTTON_DOWN;
            mask |= PAD_BUTTON_LEFT;
        }
        break;

    case 6:
        {
            mask |= PAD_BUTTON_LEFT;
        }
        break;

    case 7:
        {
            mask |= PAD_BUTTON_UP;
            mask |= PAD_BUTTON_LEFT;
        }
        break;
    }

    if (input[5] & (1 << 4))
    { mask |= PAD_BUTTON_SQUARE; }
    if (input[5] & (1 << 5))
    { mask |= PAD_BUTTON_CROSS; }
    if (input[5] & (1 << 6))
    { mask |= PAD_BUTTON_CIRCLE; }
    if (input[5] & (1 << 7))
    { mask |= PAD_BUTTON_TRIANGLE; }

    if (input[6] & (1 << 0))
    { mask |= PAD_BUTTON_L1; }
    if (input[6] & (1 << 1))
    { mask |= PAD_BUTTON_R1; }
    if (input[6] & (1 << 2))
    { mask |= PAD_BUTTON_L2; }
    if (input[6] & (1 << 3))
    { mask |= PAD_BUTTON_R2; }
    //if (input[6] & (1 << 4))
    //{ mask |= PAD_BUTTON_SHARE; }
    if (input[6] & (1 << 5))
    { mask |= PAD_BUTTON_OPTIONS; }
    if (input[6] & (1 << 6))
    { mask |= PAD_BUTTON_L3; }
    if (input[6] & (1 << 7))
    { mask |= PAD_BUTTON_R3; }

    if (input[7] & (1 << 0))
    { mask |= PAD_BUTTON_PS; }
    if (input[7] & (1 << 1))
    { mask |= PAD_BUTTON_TOUCH_PAD; }

    state.buttons = mask;

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

    if (pHandle->handle == nullptr)
    { return false; }

    uint8_t bytes[78] = {};
    if (pHandle->type == PAD_CONNECTION_BT)
    {
        bytes[0] = 0x11;
        bytes[1] = 0xb0;
        bytes[3] = 0xf1;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[6] = pParam->largeMotor;
        bytes[7] = pParam->smallMotor;
    }
    else
    {
        bytes[0] = 0x05;
        bytes[1] = 0xf1;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[4] = pParam->largeMotor;
        bytes[5] = pParam->smallMotor;
    }

    auto size = WriteFile(pHandle->handle, bytes, 32, nullptr, nullptr);
    if (size != 32)
    { return false; }

    return true;
}
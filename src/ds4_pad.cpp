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
#include <array>
#include <vector>
#include <sstream>
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
static const uint16_t kDualSense_CFI_ZCT1J      = 0x0ce6;

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
    HANDLE          Handle;
    std::wstring    DevicePath;
    uint32_t        Size;
    uint32_t        Type;
    std::string     MacAddress;
};

//-----------------------------------------------------------------------------
//      マルチバイト文字列に変換します.
//-----------------------------------------------------------------------------
std::string ToStringA( const std::wstring& value )
{
    auto length = WideCharToMultiByte(CP_ACP, 0, value.c_str(), int(value.size() + 1), nullptr, 0, nullptr, nullptr); 
    auto buffer = new char[length];
 
    WideCharToMultiByte(CP_ACP, 0, value.c_str(), int(value.size() + 1), buffer, length, nullptr, nullptr);

    std::string result(buffer);
    delete[] buffer;

    return result;
}

//-----------------------------------------------------------------------------
//      指定文字で文字列を分割します.
//-----------------------------------------------------------------------------
std::vector<std::string> Split(const std::string& input, char delimiter)
{
    std::istringstream stream(input);

    std::string field;
    std::vector<std::string> result;
    while (std::getline(stream, field, delimiter))
    { result.push_back(field); }
    return result;
}

//-----------------------------------------------------------------------------
//      文字列を置換します.
//-----------------------------------------------------------------------------
std::string Replace
(
    const std::string&  input,
    std::string         pattern,
    std::string         replace
)
{
    std::string result = input;
    auto pos = result.find( pattern );

    while( pos != std::string::npos )
    {
        result.replace( pos, pattern.length(), replace );
        pos = result.find( pattern, pos + replace.length() );
    }

    return result;
}

//-----------------------------------------------------------------------------
//      MACアドレスを取得します.
//-----------------------------------------------------------------------------
std::string GetMacAddress(const std::wstring& value)
{
    auto temp = ToStringA(value);
    auto idx = temp.find("{");
    if (idx == std::string::npos)
    { return ""; }

    temp = temp.substr(idx - 15, 14);
    auto lst = Split(temp, '&');
    if (lst.size() != 3)
    { return ""; }

    lst[2] = Replace(lst[2], "0", "");
    if (lst[2] == "")
    { lst[2] = "0"; }
    return lst[0] + lst[1] + lst[2];
}


//-----------------------------------------------------------------------------
//      パッドを接続します.
//-----------------------------------------------------------------------------
bool PadOpen(PadHandle& result)
{
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

    uint32_t        type = 0;
    DWORD           size = 0;
    HANDLE          handle = nullptr;
    std::wstring    devicePath;
    std::string     macAddress;

    // 170(CUH_ZCT1x), 182(CUH_ZCT2x), 180(CFI_ZCT1J).
    std::array<uint8_t, 184> buf;

    for(; deviceDetected == false; index++)
    {
        auto ret = SetupDiEnumDeviceInterfaces(info, 0, &guid, index, &devInfoData);
        if (ret == FALSE)
        {
            SetupDiDestroyDeviceInfoList(info);
            return false;
        }

        // サイズ取得.
        SetupDiGetDeviceInterfaceDetail(info, &devInfoData, NULL, 0, &length, NULL);

        // サイズが期待値以上なら処理しない.
        if (buf.size() < length)
        { continue; }

        auto detailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(buf.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        ret = SetupDiGetDeviceInterfaceDetail(info, &devInfoData, detailData, length, &required, NULL);
        if (ret == FALSE)
        {
            auto errcode = GetLastError();
            SetupDiDestroyDeviceInfoList(info);
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
            continue;
        }

        HIDD_ATTRIBUTES attributes = {};
        ret = HidD_GetAttributes(handle, &attributes);
        if (ret == FALSE)
        {
            CloseHandle(handle);
            handle = nullptr;

            continue;
        }

        if (attributes.VendorID != kSonyCorp)
        {
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

            macAddress = GetMacAddress(detailData->DevicePath);
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
                else
                { macAddress = GetMacAddress(detailData->DevicePath); }
            }
            else
            {
            #if 0
                //type = PAD_CONNECTION_BT;

                //char buf[126];
                //if (HidD_GetSerialNumberString(handle, buf, 126) == TRUE)
                //{ macAddress = buf; }
            #else
                // Bluetooth 非サポート.
                HidD_FreePreparsedData(preparsedData);

                CloseHandle(handle);
                handle = nullptr;

                continue;
            #endif
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

            // Outputは32

            if (capabilities.InputReportByteLength == 64)
            {
                type = PAD_CONNECTION_USB;

                char buf[16];
                buf[0] = 18;
                if (HidD_GetFeature(handle, buf, 16) == TRUE)
                { macAddress = buf; }
                else
                { macAddress = GetMacAddress(detailData->DevicePath); }
            }
            else
            {
            #if 0
                //type = PAD_CONNECTION_BT;

                //char buf[126];
                //if (HidD_GetSerialNumberString(handle, buf, 126) == TRUE)
                //{ macAddress = buf; }
            #else
                // Bluetooth 非サポート.
                HidD_FreePreparsedData(preparsedData);

                CloseHandle(handle);
                handle = nullptr;

                continue;
            #endif
            }

            size = capabilities.FeatureReportByteLength;
            HidD_FreePreparsedData(preparsedData);
        }
        else if (attributes.ProductID == kDualSense_CFI_ZCT1J)
        {
            deviceDetected = true;

            PHIDP_PREPARSED_DATA preparsedData;
            HidD_GetPreparsedData(handle, &preparsedData);

            HIDP_CAPS capabilities;
            HidP_GetCaps(preparsedData, &capabilities);

            // Outputは48

            if (capabilities.InputReportByteLength == 64)
            {
                type = PAD_CONNECTION_USB | PAD_CONNECTION_DUAL_SENSE;

                // デバイス名からMACアドレスを取得.
                macAddress = GetMacAddress(detailData->DevicePath);
            }
            else
            {
                // Bluetooth 非サポート.
                HidD_FreePreparsedData(preparsedData);

                CloseHandle(handle);
                handle = nullptr;

                continue;
            }

            size = capabilities.FeatureReportByteLength;
            HidD_FreePreparsedData(preparsedData);
        }

        devicePath = detailData->DevicePath;
    }

    SetupDiDestroyDeviceInfoList(info);

    // ハンドル生成.
    result.Handle       = handle;
    result.DevicePath   = devicePath;
    result.Size         = size;
    result.Type         = type;
    result.MacAddress   = macAddress;

    return true;
}

//-----------------------------------------------------------------------------
//      パッドを接続します.
//-----------------------------------------------------------------------------
bool PadOpen(PadHandle** ppHandle)
{
    auto padHandle = new(std::nothrow) PadHandle();
    if (padHandle == nullptr)
    { return false; }

    if (!PadOpen(*padHandle))
    {
        delete padHandle;
        padHandle = nullptr;
    }

    *ppHandle = padHandle;

    return true;
}

//-----------------------------------------------------------------------------
//      パッドを切断します.
//-----------------------------------------------------------------------------
bool PadClose(PadHandle& padHandle)
{
    if (padHandle.Type == PAD_CONNECTION_BT)
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
        //    ret = DeviceIoControl(handleBT, IOCTL_BTH_DISCONNECT_DEVICE, btAddress, sizeof(btAddress), NULL, 0, &length, NULL);
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

    if (padHandle.Handle != nullptr)
    {
        PadColor color = {};
        PadVibrationParam vibrate = {};

        PadSetLightBarColor(&padHandle, color);
        PadSetVibration(&padHandle, vibrate);

        CloseHandle(padHandle.Handle);
        padHandle.Handle = nullptr;
    }

    return true;
}

//-----------------------------------------------------------------------------
//      パッドを切断します.
//-----------------------------------------------------------------------------
bool PadClose(PadHandle*& pHandle)
{
    if (pHandle == nullptr)
    { return false; }

    if (PadClose(*pHandle))
    {
        // ハンドル破棄.
        delete pHandle;
        pHandle = nullptr;

        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//      パッド生データを読み取ります.
//-----------------------------------------------------------------------------
bool PadRead(PadHandle* pHandle, PadRawInput& result)
{
    if (pHandle == nullptr)
    { return false; }

    if (pHandle->Handle == nullptr)
    { return false; }

    if (!!(pHandle->Type & PAD_CONNECTION_BT))
    {
        // Bluetooth非サポート.
        return false;
    }

    auto ret = ReadFile(pHandle->Handle, result.Bytes, pHandle->Size, nullptr, nullptr);
    result.Type = pHandle->Type;

    return (ret == TRUE);
}

//-----------------------------------------------------------------------------
//      Dual Shock4パッドデータを扱いやすい形にマッピングします.
//-----------------------------------------------------------------------------
bool PadMapDualShock4(const PadRawInput* pRawData, PadState& state)
{
    const uint8_t* input = &pRawData->Bytes[0];

    if (!!(pRawData->Type & PAD_CONNECTION_NONE))
    { 
        memset(&state, 0, sizeof(state));
        return false;
    }
    else if (!!(pRawData->Type & PAD_CONNECTION_USB))
    {
        // 64 bytes.
        state.Type = PAD_CONNECTION_USB;
    }
    else if (!!(pRawData->Type & PAD_CONNECTION_WIRELESS))
    {
        // 64 bytes.
        state.Type = PAD_CONNECTION_WIRELESS;
    }
    else if (!!(pRawData->Type & PAD_CONNECTION_BT))
    {
        //// 547 bytes.
        //state.Type = PAD_CONNECTION_BT;
        //input = &pRawData->Bytes[2];

        // Bluetooth 非サポート.
        return false;
    }

    state.StickL.X          = input[1];
    state.StickL.Y          = input[2];
    state.StickR.X          = input[3];
    state.StickR.Y          = input[4];
    state.AnalogButtons.L2  = input[8];
    state.AnalogButtons.R2  = input[9];
    state.Buttons           = uint16_t(input[5] | (input[6] << 8));
    state.SpecialButtons    = input[7] & 0x3;
    state.TimeStamp         = uint16_t((input[11] << 8) | input[10]);
    state.BatteryLevel      = input[12];

    state.Gyro.X  = (int16_t)(uint16_t(input[14] << 8) | input[13]);
    state.Gyro.Y  = (int16_t)(uint16_t(input[16] << 8) | input[15]);
    state.Gyro.Z  = (int16_t)(uint16_t(input[18] << 8) | input[17]);

    state.Accel.X = (int16_t)(uint16_t(input[20] << 8) | input[19]);
    state.Accel.Y = (int16_t)(uint16_t(input[22] << 8) | input[21]);
    state.Accel.Z = (int16_t)(uint16_t(input[24] << 8) | input[23]);

    auto touch_count = 0;
    if ((input[35] & 0x80) == 0)
    { touch_count++; }

    if ((input[39] & 0x80) == 0)
    { touch_count++; }

    state.TouchData.Count = touch_count;
    state.TouchData.Touch[0].Id = (input[35] & 0x7f);
    state.TouchData.Touch[0].X = static_cast<uint16_t>((uint16_t(input[37] & 0xf) << 8) | input[36]);
    state.TouchData.Touch[0].Y = static_cast<uint16_t>((uint16_t(input[38] << 4)) | ((input[37] & 0xf0) >> 4));

    state.TouchData.Touch[1].Id = (input[39] & 0x7f);
    state.TouchData.Touch[1].X = static_cast<uint16_t>((uint16_t(input[41] & 0xf) << 8) | input[40]);
    state.TouchData.Touch[1].Y = static_cast<uint16_t>((uint16_t(input[42] << 4)) | ((input[41] & 0xf0) >> 4));

    return true;
}

//-----------------------------------------------------------------------------
//      Dual Senseパッドデータを扱いやすい形にマッピングします.
//-----------------------------------------------------------------------------
bool PadMapDualSense(const PadRawInput* pRawData, PadState& state)
{
    const uint8_t* input = &pRawData->Bytes[0];

    if (!!(pRawData->Type & PAD_CONNECTION_NONE))
    { 
        memset(&state, 0, sizeof(state));
        return false;
    }
    else if (!!(pRawData->Type & PAD_CONNECTION_USB))
    {
        // 64 bytes.
        state.Type = PAD_CONNECTION_USB;
    }
    else if (!!(pRawData->Type == PAD_CONNECTION_WIRELESS))
    {
        // 64 bytes.
        state.Type = PAD_CONNECTION_WIRELESS;
    }
    else if (pRawData->Type == PAD_CONNECTION_BT)
    {
        //state.Type = PAD_CONNECTION_BT;
        //input = &pRawData->Bytes[2];

        // Bluetooth 非サポート.
        return false;
    }

    /* Memo :
    * input[7] →　タイマーカウンターっぽい挙動.
    * input[11] →　常にゼロ.
    * input[12]~[15]　→　タイムスタンプっぽい.
    * input[16]~　→　ジャイロか加速度.
    * input[30]~[32]　→ タイムスタンプっぽい.
    */

    state.StickL.X          = input[1];
    state.StickL.Y          = input[2];
    state.StickR.X          = input[3];
    state.StickR.Y          = input[4];
    state.AnalogButtons.L2  = input[5];
    state.AnalogButtons.R2  = input[6];
    state.Buttons           = uint16_t(input[8] | (input[9] << 8));
    state.SpecialButtons    = input[10] & 0xf;
    state.TimeStamp         = uint16_t((input[13] << 8) | input[12]);

    // ジャイロ, 加速度は暫定対応.
    // TODO : Gyro, Accelaration Implementation.
    state.Gyro.X  = (int16_t)(uint16_t(input[17] << 8) | input[16]);
    state.Gyro.Y  = (int16_t)(uint16_t(input[19] << 8) | input[18]);
    state.Gyro.Z  = (int16_t)(uint16_t(input[21] << 8) | input[20]);

    state.Accel.X = (int16_t)(uint16_t(input[23] << 8) | input[22]);
    state.Accel.Y = (int16_t)(uint16_t(input[25] << 8) | input[24]);
    state.Accel.Z = (int16_t)(uint16_t(input[27] << 8) | input[26]);

    state.TouchData.Count       = input[41];
    state.TouchData.Touch[0].Id = (input[33] & 0x7f);
    state.TouchData.Touch[0].X  = static_cast<uint16_t>((uint16_t(input[35] & 0xf) << 8) | input[34]);
    state.TouchData.Touch[0].Y  = static_cast<uint16_t>((uint16_t(input[36] << 4)) | ((input[35] & 0xf0) >> 4));

    state.TouchData.Touch[1].Id = (input[37] & 0x7f);
    state.TouchData.Touch[1].X  = static_cast<uint16_t>((uint16_t(input[39] & 0xf) << 8) | input[38]);
    state.TouchData.Touch[1].Y  = static_cast<uint16_t>((uint16_t(input[40] << 4)) | ((input[39] & 0xf0) >> 4));

    return true;
}

//-----------------------------------------------------------------------------
//      パッドデータを扱いやすい形にマッピングします.
//-----------------------------------------------------------------------------
bool PadMap(const PadRawInput* pRawData, PadState& state)
{
    if (pRawData == nullptr)
    { return false; }

    if (!!(pRawData->Type & PAD_CONNECTION_DUAL_SENSE))
    { return PadMapDualSense(pRawData, state); }

    return PadMapDualShock4(pRawData, state);
}

//-----------------------------------------------------------------------------
//      パッドデータを読み取ります.
//-----------------------------------------------------------------------------
bool PadGetState(PadHandle* pHandle, PadState& state)
{
    PadRawInput pResult;
    if (!PadRead(pHandle, pResult))
    { return false; }

    if (!PadMap(&pResult, state))
    { return false; }

    return true;
}

//-----------------------------------------------------------------------------
//      DualShock4にバイブレーションデータを設定します.
//-----------------------------------------------------------------------------
bool PadSetVibrationDualShock4(PadHandle* pHandle, const PadVibrationParam& param)
{
    uint8_t bytes[32] = {};
    if (!!(pHandle->Type & PAD_CONNECTION_BT))
    {
        bytes[0] = 0x11;
        bytes[1] = 0xb0;
        bytes[3] = 0xf1;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[6] = param.LargeMotor;
        bytes[7] = param.SmallMotor;
    }
    else
    {
        bytes[0] = 0x05;
        bytes[1] = 0xf1;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[4] = param.LargeMotor;
        bytes[5] = param.SmallMotor;
    }

    auto size = WriteFile(pHandle->Handle, bytes, 32, nullptr, nullptr);
    if (size != 32)
    { return false; }

    return true;
}

//-----------------------------------------------------------------------------
//      DualSenseにバイブレーションデータを設定します.
//-----------------------------------------------------------------------------
bool PadSetVibrationDualSense(PadHandle* pHandle, const PadVibrationParam& param)
{
    uint8_t bytes[48] = {};
    if (!!(pHandle->Type & PAD_CONNECTION_BT))
    {
        return false;
    }
    else
    {
        // 制御フラグ = 0x01 | 0x04 | 0x08 | 0x10 | 0x20 | 0x80
        bytes[0] = 0x2;
        bytes[1] = 0xff;
        bytes[2] = 0x1; // control flags.
        bytes[3] = param.LargeMotor;
        bytes[4] = param.SmallMotor;
        bytes[9] = 0x0; // mic
    }

    auto size = WriteFile(pHandle->Handle, bytes, 48, nullptr, nullptr);
    if (size != 48)
    { return false; }

    return true;
}

//-----------------------------------------------------------------------------
//      バイブレーションを設定します.
//-----------------------------------------------------------------------------
bool PadSetVibration(PadHandle* pHandle, const PadVibrationParam& param)
{
    if (pHandle == nullptr)
    { return false; }

    if (pHandle->Handle == nullptr)
    { return false; }

    if (!!(pHandle->Type & PAD_CONNECTION_DUAL_SENSE))
    { return PadSetVibrationDualSense(pHandle, param); }

    return PadSetVibrationDualShock4(pHandle, param); 
}

//-----------------------------------------------------------------------------
//      DualShock4にライトバーカラーを設定します.
//-----------------------------------------------------------------------------
bool PadSetLightBarColorDualShock4(PadHandle* pHandle, const PadColor& param)
{
    uint8_t bytes[32] = {};
    if (!!(pHandle->Type & PAD_CONNECTION_BT))
    {
        bytes[0]  = 0x11;
        bytes[1]  = 0xb0;
        bytes[3]  = 0xf6;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[8]  = param.R;
        bytes[9]  = param.G;
        bytes[10] = param.B;
    }
    else
    {
        bytes[0] = 0x05;
        bytes[1] = 0xf6;    // enable rumble (0x01), lightbar (0x02), flash (0x04)
        bytes[6] = param.R;
        bytes[7] = param.G;
        bytes[8] = param.B;
    }

    auto size = WriteFile(pHandle->Handle, bytes, 32, nullptr, nullptr);
    if (size != 32)
    { return false; }

    return true;
}

//-----------------------------------------------------------------------------
//      DualSenseにライトバーカラーを設定します.
//-----------------------------------------------------------------------------
bool PadSetLightBarColorDualSense(PadHandle* pHandle, const PadColor& param)
{
    uint8_t bytes[48] = {};
    if (!!(pHandle->Type & PAD_CONNECTION_BT))
    {
        return false;
    }
    else
    {
        // 制御フラグ = 0x01 | 0x04 | 0x08 | 0x10 | 0x20 | 0x80
        bytes[0] = 0x2;
        bytes[1] = 0xff;
        bytes[2] = 0x2 | 0x4; // control flags.
        bytes[3] = 0;
        bytes[4] = 0;
        bytes[9] = 0x0; // mic
        bytes[45] = param.R;
        bytes[46] = param.G;
        bytes[47] = param.B;
    }

    auto size = WriteFile(pHandle->Handle, bytes, 48, nullptr, nullptr);
    if (size != 48)
    { return false; }

    return true;
}

//-----------------------------------------------------------------------------
//      ライトバーカラーを設定します.
//-----------------------------------------------------------------------------
bool PadSetLightBarColor(PadHandle* pHandle, const PadColor& param)
{
    if (pHandle == nullptr)
    { return false; }

    if (pHandle->Handle == nullptr)
    { return false; }

    if (!!(pHandle->Type & PAD_CONNECTION_DUAL_SENSE))
    { return PadSetLightBarColorDualSense(pHandle, param); }

    return PadSetLightBarColorDualShock4(pHandle, param);
}

//-----------------------------------------------------------------------------
//      パッドデータを読み取ります.
//-----------------------------------------------------------------------------
bool PadGetState(PadState& state)
{
    PadHandle handle;
    if (!PadOpen(handle))
    { return false; }

    PadRawInput rawData;
    if (!PadRead(&handle, rawData))
    {
        PadClose(handle);
        return false;
    }

    auto ret = PadMap(&rawData, state);
    PadClose(handle);

    return ret;
}

//-----------------------------------------------------------------------------
//      パッド生データを読み取ります.
//-----------------------------------------------------------------------------
bool PadRead(PadRawInput& state)
{
    PadHandle handle;
    if (!PadOpen(handle))
    { return false; }

    PadRawInput rawData;
    auto ret = PadRead(&handle, rawData);

    PadClose(handle);
    return ret;
}

//-----------------------------------------------------------------------------
//      バイブレーションを設定します.
//-----------------------------------------------------------------------------
bool PadSetVibration(const PadVibrationParam& param)
{
    PadHandle handle;
    if (!PadOpen(handle))
    { return false; }

    auto ret = PadSetVibration(&handle, param);
    PadClose(handle);

    return ret;
}

//-----------------------------------------------------------------------------
//      ライトバーカラーを設定します.
//-----------------------------------------------------------------------------
bool PadSetLightBarColor(const PadColor& param)
{
    PadHandle handle;
    if (!PadOpen(handle))
    { return false; }

    auto ret = PadSetLightBarColor(&handle, param);
    PadClose(handle);

    return ret;
}

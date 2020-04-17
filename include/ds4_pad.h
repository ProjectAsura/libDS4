//-----------------------------------------------------------------------------
// File : ds4_pad.h
// Desc : Dual Shock4 Game Pad Library.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <cstdint>


//-----------------------------------------------------------------------------
// Linker 
//-----------------------------------------------------------------------------
#ifdef LIB_DS4_AUTO_LINK
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "Bthprops.lib")
#endif//LIB_DS4_AUTO_LINK


//-----------------------------------------------------------------------------
// Type Definition.
//-----------------------------------------------------------------------------
struct PadHandle;
struct PadRawInput;


//-----------------------------------------------------------------------------
// Constant Value.
//-----------------------------------------------------------------------------
static const uint8_t kPadMaxTouchCount = 2;


///////////////////////////////////////////////////////////////////////////////
// PAD_CONNECTION_TYPE
///////////////////////////////////////////////////////////////////////////////
enum PAD_CONNECTION_TYPE
{
    PAD_CONNECTION_NONE,        //!< None.
    PAD_CONNECTION_USB,         //!< USB Wire.
    PAD_CONNECTION_BT,          //!< Bluetooth.
    PAD_CONNECTION_WIRELESS     //!< USB Wireless Adaptor.
};

///////////////////////////////////////////////////////////////////////////////
// PAD_BUTTON_OFFSET enum
///////////////////////////////////////////////////////////////////////////////
enum PAD_BUTTON_OFFSET
{
    PAD_BUTTON_R3           = 1 << 15,
    PAD_BUTTON_L3           = 1 << 14,
    PAD_BUTTON_OPTIONS      = 1 << 13,
    PAD_BUTTON_SHARE        = 1 << 12,
    PAD_BUTTON_R2           = 1 << 11,
    PAD_BUTTON_L2           = 1 << 10,
    PAD_BUTTON_R1           = 1 << 9,
    PAD_BUTTON_L1           = 1 << 8,
    PAD_BUTTON_TRIANGLE     = 1 << 7,
    PAD_BUTTON_CIRCLE       = 1 << 6,
    PAD_BUTTON_CROSS        = 1 << 5,
    PAD_BUTTON_SQUARE       = 1 << 4
};

///////////////////////////////////////////////////////////////////////////////
// PAD_BUTTON_DPAD enum
///////////////////////////////////////////////////////////////////////////////
enum PAD_BUTTON_DPAD
{
    PAD_BUTTON_DPAD_NONE        = 0x8,
    PAD_BUTTON_DPAD_NORTHWEST   = 0x7,  // ↑←.
    PAD_BUTTON_DPAD_WEST        = 0x6,  // ←.
    PAD_BUTTON_DPAD_SOUTHWEST   = 0x5,  // ↓←.
    PAD_BUTTON_DPAD_SOUTH       = 0x4,  // ↓
    PAD_BUTTON_DPAD_SOUTHEAST   = 0x3,  // ↓→
    PAD_BUTTON_DPAD_EAST        = 0x2,  // →.
    PAD_BUTTON_DPAD_NORTHEAST   = 0x1,  // ↑→.
    PAD_BUTTON_DPAD_NORTH       = 0x0   // ↑.
};

///////////////////////////////////////////////////////////////////////////////
// PAD_SPECIAL_BUTTON_OFFSET enum
///////////////////////////////////////////////////////////////////////////////
enum PAD_SPECIAL_BUTTON_OFFSET
{
    PAD_SPECIAL_BUTTON_PS       = 1 << 0,   // PlayStationボタン.
    PAD_SPECIAL_BUTTON_TPAD     = 1 << 1    // タッチパッド.
};

///////////////////////////////////////////////////////////////////////////////
// PadVector3 structure
///////////////////////////////////////////////////////////////////////////////]
struct PadVector3
{
    float   X;
    float   Y;
    float   Z;
};

///////////////////////////////////////////////////////////////////////////////
// PadQuaternion structure
///////////////////////////////////////////////////////////////////////////////
struct PadQuaternion
{
    float   X;
    float   Y;
    float   Z;
    float   W;
};

///////////////////////////////////////////////////////////////////////////////
// PadAnalogStick structure
///////////////////////////////////////////////////////////////////////////////
struct PadAnalogStick
{
    uint8_t     X;      //!< X成分(左端が0で，右端が255).
    uint8_t     Y;      //!< Y成分(下端が0で，上端が255).
};

///////////////////////////////////////////////////////////////////////////////
// PadAnlogButtons structure
///////////////////////////////////////////////////////////////////////////////
struct PadAnalogButtons
{
    uint8_t     L2;     //!< L2トリガー.
    uint8_t     R2;     //!< R2トリガー.
};

///////////////////////////////////////////////////////////////////////////////
// PadVibrationParam structure
///////////////////////////////////////////////////////////////////////////////
struct PadVibrationParam
{
    uint8_t     LargeMotor; //!< モーター左.
    uint8_t     SmallMotor; //!< モーター右.
};

///////////////////////////////////////////////////////////////////////////////
// PadColor structure
///////////////////////////////////////////////////////////////////////////////
struct PadColor
{
    uint8_t     R;      //!< R成分.
    uint8_t     G;      //!< G成分.
    uint8_t     B;      //!< B成分.
};

///////////////////////////////////////////////////////////////////////////////
// PadTouch structure
///////////////////////////////////////////////////////////////////////////////
struct PadTouch
{
    uint16_t    X;      //!< X座標.
    uint16_t    Y;      //!< Y座標.
    uint8_t     Id;     //!< 識別番号.
};

///////////////////////////////////////////////////////////////////////////////
// PadTouchData structure
///////////////////////////////////////////////////////////////////////////////
struct PadTouchData
{
    uint8_t     Count;                      //!< タッチ数.
    PadTouch    Touch[kPadMaxTouchCount];   //!< タッチデータ.
};

///////////////////////////////////////////////////////////////////////////////
// PadState structure
///////////////////////////////////////////////////////////////////////////////
struct PadState
{
    PAD_CONNECTION_TYPE     Type;               //!< 接続タイプ.
    PadAnalogStick          StickL;             //!< 左スティック.
    PadAnalogStick          StickR;             //!< 右スティック.
    uint16_t                Buttons;            //!< ボタン(下位4bitがDPad).
    uint8_t                 SpecialButtons;     //!< 特殊ボタン.
    PadAnalogButtons        AnalogButtons;      //!< アナログボタン.
#if 0
    //PadQuaternion           Orientation;        //!< 向き.
    //PadVector3              Acceleration;       //!< 加速度.
    //PadVector3              AngularVelocity;    //!< 角速度.
    //PadTouchData            TouchData;          //!< タッチパッドデータ.
#endif
};

///////////////////////////////////////////////////////////////////////////////
// PadRawInput structure
///////////////////////////////////////////////////////////////////////////////
struct PadRawInput
{
    PAD_CONNECTION_TYPE     Type;
    uint8_t                 Bytes[78];
};

//-----------------------------------------------------------------------------
//! @brief      パッドを接続します.
//!
//! @param[in]      userId      ユーザーID.
//! @param[out]     pHandle     ハンドルの格納先です.
//! @retval true    接続に成功.
//! @retval false   接続に失敗.
//-----------------------------------------------------------------------------
bool PadOpen(PadHandle** ppHandle);

//-----------------------------------------------------------------------------
//! @brief      パッドを切断します.
//!
//! @param[in]      pHandle      パッドハンドル.
//! @retval true    切断に成功.
//! @retval false   切断に失敗.
//-----------------------------------------------------------------------------
bool PadClose(PadHandle*& pHandle);

//-----------------------------------------------------------------------------
//! @brief      パッド生データを読み取ります.
//!
//! @param[in]      pHandle     パッドハンドル.
//! @param[out]     pResult     パッド生データの格納先.
//! @retval true    読み取りに成功.
//! @retval false   読み取りに失敗.
//-----------------------------------------------------------------------------
bool PadRead(PadHandle* handle, PadRawInput& result);

//-----------------------------------------------------------------------------
//! @brief      パッド生データを扱いやすい形にマッピングします.
//!
//! @param[in]      pRawData        パッド生データ.
//! @param[out]     state           パッドデータ.
//! @retval true    マッピングに成功.
//! @retval false   マッピングに失敗.
//-----------------------------------------------------------------------------
bool PadMap(const PadRawInput* pRawData, PadState& state);

//-----------------------------------------------------------------------------
//! @brief      パッドデータを取得します.
//!
//! @param[in]      pHandle         パッドハンドル.
//! @param[out]     state           パッドデータの格納先.
//! @retval true    読み取りに成功.
//! @retval false   読み取りに失敗.
//-----------------------------------------------------------------------------
bool PadGetState(PadHandle* pHandle, PadState& state);

//-----------------------------------------------------------------------------
//! @brief      バイブレーションを設定します.
//!
//! @param[in]      handle      パッドハンドル.
//! @param[in]      param      バイブレーションデータ.
//! @retval true    設定に成功.
//! @retval false   設定に失敗.
//-----------------------------------------------------------------------------
bool PadSetVibration(PadHandle* handle, const PadVibrationParam& param);

//-----------------------------------------------------------------------------
//! @brief      ライトバーカラーを設定します.
//!
//! @param[in]      handle      パッドハンドル.
//! @param[in]      pParam      ライトバーカラー.
//! @retval true    設定に成功.
//! @retval fasle   設定に失敗.
//-----------------------------------------------------------------------------
bool PadSetLightBarColor(PadHandle* handle, const PadColor& param);


bool PadGetState(PadState& state);
bool PadRead(PadRawInput& state);
bool PadSetVibration(const PadVibrationParam& param);
bool PadSetLightBarColor(const PadColor& param);


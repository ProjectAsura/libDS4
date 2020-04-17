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
    PAD_BUTTON_DPAD_NORTHWEST   = 0x7,  // ����.
    PAD_BUTTON_DPAD_WEST        = 0x6,  // ��.
    PAD_BUTTON_DPAD_SOUTHWEST   = 0x5,  // ����.
    PAD_BUTTON_DPAD_SOUTH       = 0x4,  // ��
    PAD_BUTTON_DPAD_SOUTHEAST   = 0x3,  // ����
    PAD_BUTTON_DPAD_EAST        = 0x2,  // ��.
    PAD_BUTTON_DPAD_NORTHEAST   = 0x1,  // ����.
    PAD_BUTTON_DPAD_NORTH       = 0x0   // ��.
};

///////////////////////////////////////////////////////////////////////////////
// PAD_SPECIAL_BUTTON_OFFSET enum
///////////////////////////////////////////////////////////////////////////////
enum PAD_SPECIAL_BUTTON_OFFSET
{
    PAD_SPECIAL_BUTTON_PS       = 1 << 0,   // PlayStation�{�^��.
    PAD_SPECIAL_BUTTON_TPAD     = 1 << 1    // �^�b�`�p�b�h.
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
    uint8_t     X;      //!< X����(���[��0�ŁC�E�[��255).
    uint8_t     Y;      //!< Y����(���[��0�ŁC��[��255).
};

///////////////////////////////////////////////////////////////////////////////
// PadAnlogButtons structure
///////////////////////////////////////////////////////////////////////////////
struct PadAnalogButtons
{
    uint8_t     L2;     //!< L2�g���K�[.
    uint8_t     R2;     //!< R2�g���K�[.
};

///////////////////////////////////////////////////////////////////////////////
// PadVibrationParam structure
///////////////////////////////////////////////////////////////////////////////
struct PadVibrationParam
{
    uint8_t     LargeMotor; //!< ���[�^�[��.
    uint8_t     SmallMotor; //!< ���[�^�[�E.
};

///////////////////////////////////////////////////////////////////////////////
// PadColor structure
///////////////////////////////////////////////////////////////////////////////
struct PadColor
{
    uint8_t     R;      //!< R����.
    uint8_t     G;      //!< G����.
    uint8_t     B;      //!< B����.
};

///////////////////////////////////////////////////////////////////////////////
// PadTouch structure
///////////////////////////////////////////////////////////////////////////////
struct PadTouch
{
    uint16_t    X;      //!< X���W.
    uint16_t    Y;      //!< Y���W.
    uint8_t     Id;     //!< ���ʔԍ�.
};

///////////////////////////////////////////////////////////////////////////////
// PadTouchData structure
///////////////////////////////////////////////////////////////////////////////
struct PadTouchData
{
    uint8_t     Count;                      //!< �^�b�`��.
    PadTouch    Touch[kPadMaxTouchCount];   //!< �^�b�`�f�[�^.
};

///////////////////////////////////////////////////////////////////////////////
// PadState structure
///////////////////////////////////////////////////////////////////////////////
struct PadState
{
    PAD_CONNECTION_TYPE     Type;               //!< �ڑ��^�C�v.
    PadAnalogStick          StickL;             //!< ���X�e�B�b�N.
    PadAnalogStick          StickR;             //!< �E�X�e�B�b�N.
    uint16_t                Buttons;            //!< �{�^��(����4bit��DPad).
    uint8_t                 SpecialButtons;     //!< ����{�^��.
    PadAnalogButtons        AnalogButtons;      //!< �A�i���O�{�^��.
#if 0
    //PadQuaternion           Orientation;        //!< ����.
    //PadVector3              Acceleration;       //!< �����x.
    //PadVector3              AngularVelocity;    //!< �p���x.
    //PadTouchData            TouchData;          //!< �^�b�`�p�b�h�f�[�^.
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
//! @brief      �p�b�h��ڑ����܂�.
//!
//! @param[in]      userId      ���[�U�[ID.
//! @param[out]     pHandle     �n���h���̊i�[��ł�.
//! @retval true    �ڑ��ɐ���.
//! @retval false   �ڑ��Ɏ��s.
//-----------------------------------------------------------------------------
bool PadOpen(PadHandle** ppHandle);

//-----------------------------------------------------------------------------
//! @brief      �p�b�h��ؒf���܂�.
//!
//! @param[in]      pHandle      �p�b�h�n���h��.
//! @retval true    �ؒf�ɐ���.
//! @retval false   �ؒf�Ɏ��s.
//-----------------------------------------------------------------------------
bool PadClose(PadHandle*& pHandle);

//-----------------------------------------------------------------------------
//! @brief      �p�b�h���f�[�^��ǂݎ��܂�.
//!
//! @param[in]      pHandle     �p�b�h�n���h��.
//! @param[out]     pResult     �p�b�h���f�[�^�̊i�[��.
//! @retval true    �ǂݎ��ɐ���.
//! @retval false   �ǂݎ��Ɏ��s.
//-----------------------------------------------------------------------------
bool PadRead(PadHandle* handle, PadRawInput& result);

//-----------------------------------------------------------------------------
//! @brief      �p�b�h���f�[�^�������₷���`�Ƀ}�b�s���O���܂�.
//!
//! @param[in]      pRawData        �p�b�h���f�[�^.
//! @param[out]     state           �p�b�h�f�[�^.
//! @retval true    �}�b�s���O�ɐ���.
//! @retval false   �}�b�s���O�Ɏ��s.
//-----------------------------------------------------------------------------
bool PadMap(const PadRawInput* pRawData, PadState& state);

//-----------------------------------------------------------------------------
//! @brief      �p�b�h�f�[�^���擾���܂�.
//!
//! @param[in]      pHandle         �p�b�h�n���h��.
//! @param[out]     state           �p�b�h�f�[�^�̊i�[��.
//! @retval true    �ǂݎ��ɐ���.
//! @retval false   �ǂݎ��Ɏ��s.
//-----------------------------------------------------------------------------
bool PadGetState(PadHandle* pHandle, PadState& state);

//-----------------------------------------------------------------------------
//! @brief      �o�C�u���[�V������ݒ肵�܂�.
//!
//! @param[in]      handle      �p�b�h�n���h��.
//! @param[in]      param      �o�C�u���[�V�����f�[�^.
//! @retval true    �ݒ�ɐ���.
//! @retval false   �ݒ�Ɏ��s.
//-----------------------------------------------------------------------------
bool PadSetVibration(PadHandle* handle, const PadVibrationParam& param);

//-----------------------------------------------------------------------------
//! @brief      ���C�g�o�[�J���[��ݒ肵�܂�.
//!
//! @param[in]      handle      �p�b�h�n���h��.
//! @param[in]      pParam      ���C�g�o�[�J���[.
//! @retval true    �ݒ�ɐ���.
//! @retval fasle   �ݒ�Ɏ��s.
//-----------------------------------------------------------------------------
bool PadSetLightBarColor(PadHandle* handle, const PadColor& param);


bool PadGetState(PadState& state);
bool PadRead(PadRawInput& state);
bool PadSetVibration(const PadVibrationParam& param);
bool PadSetLightBarColor(const PadColor& param);


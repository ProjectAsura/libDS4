#ifndef LIB_DS4_AUTO_LINK
#define LIB_DS4_AUTO_LINK
#endif//LIB_DS4_AUTO_LINK

#if defined(DEBUG) || defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif//defined(DEBUG) || defined(_DEBUG)

#include <ds4_pad.h>
#include <cstdio>
#include <Windows.h>

int main(int argc, char** argv)
{
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif//defined(DEBUG) || defined(_DEBUG)

    PadHandle* pHandle = nullptr;
    if (PadOpen(&pHandle))
    {
        PadRawInput rawData = {};
        while(PadRead(pHandle, rawData))
        {
            //for(auto i=0; i<64; ++i)
            //{
            //    printf("0x%02x ", rawData.Bytes[i]);
            //    if ((i + 1) % 10 == 0)
            //    { printf(" "); }
            //}
            //printf("\n");
            //printf("0x%02x \n", rawData.Bytes[8]);

            PadState state = {};
            if (!PadMap(&rawData, state))
            { break; }

            printf_s("stick (%3u, %3u), (%3u, %3u) Gyro(%d, %d, %d), Accel(%d, %d, %d)\n", 
                state.StickL.X, state.StickL.Y,
                state.StickR.X, state.StickR.Y,
                state.Gyro.X,  state.Gyro.Y,  state.Gyro.Z,
                state.Accel.X, state.Accel.Y, state.Accel.Z);

            if (state.Buttons & PAD_BUTTON_L3)
            { printf_s("L3 button\n"); }

            if (state.Buttons & PAD_BUTTON_R3)
            { printf_s("R3 button\n"); }

            if (state.Buttons & PAD_BUTTON_OPTIONS)
            { printf_s("Option button\n"); }

            if (state.Buttons & PAD_BUTTON_SHARE)
            { printf_s("Share button\n"); }

            auto dpad = state.Buttons & 0xf;
            switch(dpad)
            {
            case PAD_BUTTON_DPAD_NONE:
                break;

            case PAD_BUTTON_DPAD_NORTHWEST:
                printf_s("Å™Å©\n");
                break;

            case PAD_BUTTON_DPAD_WEST:
                printf_s("Å©\n");
                break;

            case PAD_BUTTON_DPAD_SOUTHWEST:
                printf_s("Å´Å©\n");
                break;

            case PAD_BUTTON_DPAD_SOUTH:
                printf_s("Å´\n");
                break;

            case PAD_BUTTON_DPAD_SOUTHEAST:
                printf_s("Å´Å®\n");
                break;

            case PAD_BUTTON_DPAD_EAST:
                printf_s("Å®\n");
                break;

            case PAD_BUTTON_DPAD_NORTHEAST:
                printf_s("Å™Å®\n");
                break;

            case PAD_BUTTON_DPAD_NORTH:
                printf_s("Å™\n");
                break;
            }

            if (state.Buttons & PAD_BUTTON_L2)
            { printf_s("L2 button\n"); }

            if (state.Buttons & PAD_BUTTON_R2)
            { printf_s("R2 button\n"); }

            if (state.Buttons & PAD_BUTTON_L1)
            { printf_s("R1 button\n"); }

            if (state.Buttons & PAD_BUTTON_R1)
            { printf_s("L1 button\n"); }

            if (state.Buttons & PAD_BUTTON_TRIANGLE)
            { printf_s("Å¢ button\n"); }

            if (state.Buttons & PAD_BUTTON_CIRCLE)
            { printf_s("ÅZ button\n"); }

            if (state.Buttons & PAD_BUTTON_CROSS)
            { printf_s("Å~ button\n"); }

            if (state.Buttons & PAD_BUTTON_SQUARE)
            { printf_s("Å† button\n"); }

            if (state.SpecialButtons & PAD_SPECIAL_BUTTON_TPAD)
            { printf_s("TPad button\n"); }

            if (state.SpecialButtons & PAD_SPECIAL_BUTTON_PS)
            { printf_s("PS button\n"); }

            if (state.SpecialButtons & PAD_SPECIAL_BUTTON_MUTE)
            { printf_s("Mute Button\n"); }

            PadVibrationParam param = {};
            auto detect = false;
            if (state.AnalogButtons.L2)
            {
                param.LargeMotor = state.AnalogButtons.L2;
                detect = true;
            }
            if (state.AnalogButtons.R2)
            {
                param.SmallMotor = state.AnalogButtons.R2;
                detect = true;
            }

            if (detect)
            {
                PadSetVibration(pHandle, param);
            }

            if (state.Buttons & PAD_BUTTON_CIRCLE)
            {
                PadColor color;
                color.R = 255;
                color.G = 0;
                color.B = 0;

                PadSetLightBarColor(pHandle, color);
            }

            if (state.Buttons & PAD_BUTTON_TRIANGLE)
            {
                PadColor color;
                color.R = 0;
                color.G = 255;
                color.B = 0;
                PadSetLightBarColor(pHandle, color);
            }

            if (state.Buttons & PAD_BUTTON_CROSS)
            {
                PadColor color;
                color.R = 0;
                color.G = 0;
                color.B = 255;
                PadSetLightBarColor(pHandle, color);
            }

            if (state.Buttons & PAD_BUTTON_SQUARE)
            {
                PadColor color;
                color.R = 255;
                color.G = 0;
                color.B = 255;
                PadSetLightBarColor(pHandle, color);
            }

            if (state.TouchData.Count > 0)
            {
                if (state.TouchData.Count >= 1)
                {
                    printf_s("touch1 : id = %u, x = %u, y= %u\n",
                        state.TouchData.Touch[0].Id,
                        state.TouchData.Touch[0].X,
                        state.TouchData.Touch[0].Y);
                }
                
                if (state.TouchData.Count >= 2)
                {
                    printf_s("touch2 : id = %u, x = %u, y= %u\n",
                        state.TouchData.Touch[1].Id,
                        state.TouchData.Touch[1].X,
                        state.TouchData.Touch[1].Y);
                }
            }


            // EscÇ≈èIóπ.
            if (GetAsyncKeyState(VK_ESCAPE) & VK_ESCAPE)
            { break; }
        }

        PadClose(pHandle);
    }

    return 0;
}
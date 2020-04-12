#define LIB_DS4_AUTO_LINK

#include <ds4_pad.h>
#include <cstdio>
#include <Windows.h>

int main(int argc, char** argv)
{
    if (!PadInit())
    { return -1; }

    PadHandle* pHandle = nullptr;
    if (PadOpen(&pHandle))
    {
        PadRawInput rawData = {};
        while(PadRead(pHandle, &rawData))
        {
            PadState state = {};
            if (!PadMap(&rawData, state))
            { break; }

            if (state.buttons & PAD_BUTTON_L3)
            { printf_s("L3 button\n"); }

            if (state.buttons & PAD_BUTTON_R3)
            { printf_s("R3 button\n"); }

            if (state.buttons & PAD_BUTTON_OPTIONS)
            { printf_s("Option button\n"); }

            if (state.buttons & PAD_BUTTON_UP)
            { printf_s("Å™\n"); }

            if (state.buttons & PAD_BUTTON_RIGHT)
            { printf_s("Å®\n");}

            if (state.buttons & PAD_BUTTON_DOWN)
            { printf_s("Å´\n"); }

            if (state.buttons & PAD_BUTTON_LEFT)
            { printf_s("Å©\n"); }

            if (state.buttons & PAD_BUTTON_L2)
            { printf_s("L2 button\n"); }

            if (state.buttons & PAD_BUTTON_R2)
            { printf_s("R2 button\n"); }

            if (state.buttons & PAD_BUTTON_L1)
            { printf_s("R1 button\n"); }

            if (state.buttons & PAD_BUTTON_R1)
            { printf_s("L1 button\n"); }

            if (state.buttons & PAD_BUTTON_TRIANGLE)
            { printf_s("Å¢ button\n"); }

            if (state.buttons & PAD_BUTTON_CIRCLE)
            { printf_s("ÅZ button\n"); }

            if (state.buttons & PAD_BUTTON_CROSS)
            { printf_s("Å~ button\n"); }

            if (state.buttons & PAD_BUTTON_SQUARE)
            { printf_s("Å† button\n"); }

            if (state.buttons & PAD_BUTTON_TOUCH_PAD)
            { printf_s("TPad button\n"); }

            if (state.buttons & PAD_BUTTON_PS)
            { printf_s("PS button\n"); }

            printf_s("stickL %u, %u\n", state.stickL.x, state.stickL.y);
            printf_s("stickR %u, %u\n", state.stickR.x, state.stickR.y);

            PadVibrationParam param = {};
            auto detect = false;
            if (state.analogButtons.l2)
            {
                param.largeMotor = state.analogButtons.l2;
                detect = true;
            }
            if (state.analogButtons.r2)
            {
                param.smallMotor = state.analogButtons.r2;
                detect = true;
            }

            if (detect)
            {
                PadSetVibration(pHandle, &param);
            }


            // EscÇ≈èIóπ.
            if (GetAsyncKeyState(VK_ESCAPE) & VK_ESCAPE)
            { break; }
        }

        PadClose(pHandle);
    }

    PadTerm();

    return 0;
}
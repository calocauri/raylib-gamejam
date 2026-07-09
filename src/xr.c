/*******************************************************************************************
*
*   xr.c -- WebXR bridge (stereo rendering + controller input)
*
*   Owns all WebXR-specific state and the functions called directly from
*   JavaScript (see minshell.html). Knows nothing about what is actually
*   being drawn -- it hands control to Game_DrawScene() / Game_Update()
*   (declared in game.h) once the per-eye matrices are set up.
*
********************************************************************************************/

#include "xr.h"

#if defined(PLATFORM_WEB)

#include "raymath.h"
#include "rlgl.h"
#include "game.h"
#include <emscripten/emscripten.h>

// Per-eye data received from the WebXR session each frame
typedef struct XREyeData {
    Matrix view;
    Matrix proj;
    int vpX, vpY, vpW, vpH;
} XREyeData;

static bool xrActive = false;
static XREyeData xrEye[2] = { 0 };                 // [0] = left eye, [1] = right eye
static XRControllerData xrController[2] = { 0 };   // [0] = left hand, [1] = right hand

//--------------------------------------------------------------------------------------------
// Public read-only accessors (used by game.c)
//--------------------------------------------------------------------------------------------
bool XR_IsActive(void)
{
    return xrActive;
}

const XRControllerData *XR_GetController(int hand)
{
    static const XRControllerData empty = { 0 };
    if ((hand < 0) || (hand > 1)) return &empty;
    return &xrController[hand];
}

//--------------------------------------------------------------------------------------------
// Functions called directly from JavaScript (see minshell.html)
//--------------------------------------------------------------------------------------------

// Called by JS once, when the WebXR session starts/ends
EMSCRIPTEN_KEEPALIVE
void XR_SetActive(int active)
{
    xrActive = (active != 0);
}

// Converts a column-major float[16] (as given by WebXR's matrix arrays)
// into a raylib Matrix (also column-major internally)
static Matrix MatrixFromFloat16(const float *m)
{
    Matrix result = { 0 };
    result.m0 = m[0];   result.m4 = m[4];   result.m8  = m[8];    result.m12 = m[12];
    result.m1 = m[1];   result.m5 = m[5];   result.m9  = m[9];    result.m13 = m[13];
    result.m2 = m[2];   result.m6 = m[6];   result.m10 = m[10];   result.m14 = m[14];
    result.m3 = m[3];   result.m7 = m[7];   result.m11 = m[11];   result.m15 = m[15];
    return result;
}

// Called by JS once per eye, per XRFrame, BEFORE XR_RenderFrame()
EMSCRIPTEN_KEEPALIVE
void XR_SetEyeData(int eye, float *viewMat, float *projMat, int vpX, int vpY, int vpW, int vpH)
{
    if ((eye < 0) || (eye > 1)) return;

    xrEye[eye].view = MatrixFromFloat16(viewMat);
    xrEye[eye].proj = MatrixFromFloat16(projMat);
    xrEye[eye].vpX = vpX;
    xrEye[eye].vpY = vpY;
    xrEye[eye].vpW = vpW;
    xrEye[eye].vpH = vpH;
}

// Called by JS once per connected controller, per XRFrame, BEFORE XR_RenderFrame()
// hand: 0 = left, 1 = right
EMSCRIPTEN_KEEPALIVE
void XR_SetController(int hand, int connected,
                       float px, float py, float pz,
                       float qx, float qy, float qz, float qw,
                       float trigger, float grip, float thumbX, float thumbY,
                       int buttonA, int buttonB, int thumbClick)
{
    if ((hand < 0) || (hand > 1)) return;

    xrController[hand].connected = (connected != 0);
    xrController[hand].position = (Vector3){ px, py, pz };
    xrController[hand].orientation = (Quaternion){ qx, qy, qz, qw };
    xrController[hand].trigger = trigger;
    xrController[hand].grip = grip;
    xrController[hand].thumbX = thumbX;
    xrController[hand].thumbY = thumbY;
    xrController[hand].buttonA = (buttonA != 0);
    xrController[hand].buttonB = (buttonB != 0);
    xrController[hand].thumbClick = (thumbClick != 0);
}

// Renders one eye: pushes the given view/proj onto the rlgl matrix stack,
// lets the game draw using its normal raylib calls, then restores state.
static void RenderEye(Matrix view, Matrix proj)
{
    rlDrawRenderBatchActive();

    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(proj));

    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(view));

    Game_DrawScene();

    rlDrawRenderBatchActive();

    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
}

// Called by JS once per XRFrame, after XR_SetEyeData()/XR_SetController()
// have been called for this frame. deltaTime: real seconds since last frame
// (varies by headset refresh rate -- see game.c for how this becomes a
// fixed physics timestep).
EMSCRIPTEN_KEEPALIVE
void XR_RenderFrame(float deltaTime)
{
    Game_Update(deltaTime);

    rlClearScreenBuffers();

    for (int eye = 0; eye < 2; eye++)
    {
        rlViewport(xrEye[eye].vpX, xrEye[eye].vpY, xrEye[eye].vpW, xrEye[eye].vpH);
        RenderEye(xrEye[eye].view, xrEye[eye].proj);
    }
}

#endif // PLATFORM_WEB

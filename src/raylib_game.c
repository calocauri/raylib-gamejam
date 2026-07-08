/*******************************************************************************************
*
*   raylib gamejam template + WebXR support
*
*   Code licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2022-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for:
#include <string.h>                         // Required for:

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif


//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum {
    SCREEN_LOGO = 0,
    SCREEN_TITLE,
    SCREEN_GAMEPLAY,
    SCREEN_ENDING
} GameScreen;

#if defined(PLATFORM_WEB)
// Per-eye data received from the WebXR session each frame
typedef struct XREyeData {
    Matrix view;
    Matrix proj;
    int vpX, vpY, vpW, vpH;
} XREyeData;
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 720;
static const int screenHeight = 720;

static RenderTexture2D target = { 0 };  // Render texture to render our game (non-XR path)
static int frameCounter = 0;

#if defined(PLATFORM_WEB)
static bool xrActive = false;            // Are we currently inside a WebXR session?
static XREyeData xrEye[2] = { 0 };       // [0] = left eye, [1] = right eye
#endif

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame (normal, non-XR path)

#if defined(PLATFORM_WEB)
static Matrix MatrixFromFloat16(const float* m);
static void DrawSceneXR(Matrix view, Matrix proj);
#endif

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messages
#endif

    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "raylib gamejam template");

    // TODO: Load resources / Initialize variables at this point

    // Render texture to draw, enables screen scaling
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(target);

    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}

//--------------------------------------------------------------------------------------------
// WebXR bridge functions (called directly from JavaScript, see minshell.html)
//--------------------------------------------------------------------------------------------
#if defined(PLATFORM_WEB)

// Converts a column-major float[16] (as given by WebXR's matrix.getFloat32Array())
// into a raylib Matrix (also column-major internally)
static Matrix MatrixFromFloat16(const float* m)
{
    Matrix result = { 0 };
    result.m0 = m[0];   result.m4 = m[4];   result.m8 = m[8];    result.m12 = m[12];
    result.m1 = m[1];   result.m5 = m[5];   result.m9 = m[9];    result.m13 = m[13];
    result.m2 = m[2];   result.m6 = m[6];   result.m10 = m[10];   result.m14 = m[14];
    result.m3 = m[3];   result.m7 = m[7];   result.m11 = m[11];   result.m15 = m[15];
    return result;
}

// Called by JS once, when the WebXR session starts/ends
EMSCRIPTEN_KEEPALIVE
void XR_SetActive(int active)
{
    xrActive = (active != 0);
    LOG("[XR] session active: %d\n", active);
}

// Called by JS once per eye, per XRFrame, BEFORE XR_RenderFrame()
// viewMat / projMat: pointers to 16 floats each (column-major)
EMSCRIPTEN_KEEPALIVE
void XR_SetEyeData(int eye, float* viewMat, float* projMat, int vpX, int vpY, int vpW, int vpH)
{
    if ((eye < 0) || (eye > 1)) return;

    xrEye[eye].view = MatrixFromFloat16(viewMat);
    xrEye[eye].proj = MatrixFromFloat16(projMat);
    xrEye[eye].vpX = vpX;
    xrEye[eye].vpY = vpY;
    xrEye[eye].vpW = vpW;
    xrEye[eye].vpH = vpH;
}

// Draws your 3D scene using an externally supplied view/projection matrix
// NOTE: This bypasses raylib's Camera3D / BeginMode3D() math entirely,
// since WebXR already gives us the exact per-eye projection.
static void DrawSceneXR(Matrix view, Matrix proj)
{
    rlDrawRenderBatchActive();     // Flush any pending 2D batch first

    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(proj).v);

    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(view).v);

    // ------------------------------------------------------------------
    // TODO: draw your real 3D world here instead of this placeholder
    // ------------------------------------------------------------------
    DrawGrid(10, 1.0f);
    DrawCube((Vector3) { 0.0f, 0.5f, -1.5f }, 0.5f, 0.5f, 0.5f, RED);
    DrawCubeWires((Vector3) { 0.0f, 0.5f, -1.5f }, 0.5f, 0.5f, 0.5f, MAROON);
    // ------------------------------------------------------------------

    rlDrawRenderBatchActive();     // Flush the 3D batch with this eye's matrices

    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
}

// Called by JS once per XRFrame, after XR_SetEyeData() has been called for both eyes.
// Renders both eyes into whatever framebuffer is currently bound on the GL context
// (JS is responsible for binding session.renderState.baseLayer.framebuffer beforehand).
EMSCRIPTEN_KEEPALIVE
void XR_RenderFrame(void)
{
    rlClearScreenBuffers();        // Clears color+depth once for the whole XR framebuffer

    for (int eye = 0; eye < 2; eye++)
    {
        rlViewport(xrEye[eye].vpX, xrEye[eye].vpY, xrEye[eye].vpW, xrEye[eye].vpH);
        DrawSceneXR(xrEye[eye].view, xrEye[eye].proj);
    }
}

#endif // PLATFORM_WEB

//--------------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------------
// Update and draw frame (normal / non-XR path)
void UpdateDrawFrame(void)
{
#if defined(PLATFORM_WEB)
    // While a WebXR session is active, rendering is fully driven by
    // XR_RenderFrame(), called from JS inside session.requestAnimationFrame().
    // We skip the regular 2D loop entirely to avoid touching the XR framebuffer.
    if (xrActive) return;
#endif

    // Update
    //----------------------------------------------------------------------------------
    frameCounter++;
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginTextureMode(target);
    ClearBackground(RAYWHITE);

    DrawRectangle(70, 90, 200, 200, BLACK);
    DrawRectangle(70 + 16, 90 + 16, 200 - 32, 200 - 32, RAYWHITE);
    DrawText("raylib", 70 + 200 - MeasureText("raylib", 40) - 32, 90 + 200 - 40 - 24, 40, BLACK);

    DrawText("6.x", 290, 90 - 26, 280, BLACK);
    DrawText("GAMEJAM", 70, 90 + 210, 120, MAROON);

    if ((frameCounter / 20) % 2) DrawText("are you ready?", 160, 500, 50, BLACK);

    DrawRectangleLinesEx((Rectangle) { 0, 0, screenWidth, screenHeight }, 16, BLACK);

#if defined(PLATFORM_WEB)
    DrawText("Click 'Enter VR' to try WebXR", 40, 640, 20, DARKGRAY);
#endif

    EndTextureMode();

    // Render to screen (main framebuffer)
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawTexturePro(target.texture, (Rectangle) { 0, 0, (float)target.texture.width, -(float)target.texture.height },
        (Rectangle) {
        0, 0, (float)target.texture.width, (float)target.texture.height
    }, (Vector2) { 0, 0 }, 0.0f, WHITE);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
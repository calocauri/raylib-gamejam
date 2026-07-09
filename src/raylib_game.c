/*******************************************************************************************
*
*   raylib gamejam template + WebXR + Box3D
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

#include "game.h"
#include "xr.h"

#if defined(PLATFORM_WEB)
    #include "physics.h"
    #include <emscripten/emscripten.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum {
    SCREEN_LOGO = 0,
    SCREEN_TITLE,
    SCREEN_GAMEPLAY,
    SCREEN_ENDING
} GameScreen;

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 720;
static const int screenHeight = 720;

static RenderTexture2D target = { 0 };  // Render texture for the non-XR 2D template
static int frameCounter = 0;

#if defined(PLATFORM_WEB)
static const Vector3 groundHalfExtents = { 2.0f, 0.1f, 2.0f };
static const float boxHalfExtent = 0.15f;

static PhysicsBodyId groundBodyId = { 0 };
static PhysicsBodyId boxBodyId = { 0 };

#define PHYSICS_FIXED_DT (1.0f / 60.0f)
#define PHYSICS_MAX_STEPS_PER_FRAME 5   // avoids the "spiral of death" on a lag spike
static float physicsAccumulator = 0.0f;
#endif

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and draw one frame (non-XR 2D path)

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);
#endif

    InitWindow(screenWidth, screenHeight, "raylib gamejam template");

    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    Game_Init();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif

    Game_Shutdown();
    UnloadRenderTexture(target);
    CloseWindow();

    return 0;
}

//--------------------------------------------------------------------------------------------
// Game_* implementation (the game.h contract)
//--------------------------------------------------------------------------------------------
void Game_Init(void)
{
#if defined(PLATFORM_WEB)
    Physics_Init();

    // Static ground slab, top surface sits at y = 0
    groundBodyId = Physics_CreateGroundBox(
        (Vector3){ 0.0f, -groundHalfExtents.y, 0.0f },
        groundHalfExtents);

    // Dynamic box dropped above the ground so it falls into view
    boxBodyId = Physics_CreateDynamicBox(
        (Vector3){ 0.0f, 1.5f, -1.0f },
        boxHalfExtent, 1.0f, 0.3f);
#endif
}

void Game_Shutdown(void)
{
#if defined(PLATFORM_WEB)
    Physics_Shutdown();
#endif
}

#if defined(PLATFORM_WEB)

void Game_Update(float realDeltaTime)
{
    // Fixed-timestep accumulator: WebXR calls us at the headset's own
    // refresh rate (72/90/120Hz depending on device), but physics must
    // always advance in fixed 1/60s steps regardless of that rate.
    if (realDeltaTime > 0.25f) realDeltaTime = 0.25f;  // clamp huge stalls (tab switch, etc.)

    physicsAccumulator += realDeltaTime;

    int steps = 0;
    while ((physicsAccumulator >= PHYSICS_FIXED_DT) && (steps < PHYSICS_MAX_STEPS_PER_FRAME))
    {
        Physics_Step(PHYSICS_FIXED_DT);
        physicsAccumulator -= PHYSICS_FIXED_DT;
        steps++;
    }
}

void Game_DrawScene(void)
{
    // ------------------------------------------------------------------
    // TODO: draw your real 3D world here
    // ------------------------------------------------------------------
    DrawGrid(10, 1.0f);

    // Ground
    DrawCube((Vector3){ 0.0f, -groundHalfExtents.y, 0.0f },
             groundHalfExtents.x * 2.0f, groundHalfExtents.y * 2.0f, groundHalfExtents.z * 2.0f,
             DARKGRAY);

    // Box driven by Box3D
    {
        Vector3 p = Physics_GetBodyPosition(boxBodyId);
        Quaternion q = Physics_GetBodyRotation(boxBodyId);
        Matrix rot = QuaternionToMatrix(q);

        rlPushMatrix();
        rlTranslatef(p.x, p.y, p.z);
        rlMultMatrixf(MatrixToFloat(rot));

        DrawCube((Vector3){ 0, 0, 0 }, boxHalfExtent * 2.0f, boxHalfExtent * 2.0f, boxHalfExtent * 2.0f, RED);
        DrawCubeWires((Vector3){ 0, 0, 0 }, boxHalfExtent * 2.0f, boxHalfExtent * 2.0f, boxHalfExtent * 2.0f, MAROON);

        rlPopMatrix();
    }

    // Draw connected controllers: a sphere at each hand (brighter while the
    // trigger is held) plus a thin aiming ray
    for (int hand = 0; hand < 2; hand++)
    {
        const XRControllerData *c = XR_GetController(hand);
        if (!c->connected) continue;

        Color handColor = (hand == 0) ? SKYBLUE : ORANGE;
        handColor = ColorLerp(handColor, WHITE, c->trigger);

        DrawSphere(c->position, 0.03f, handColor);

        Matrix handRot = QuaternionToMatrix(c->orientation);
        Vector3 forward = Vector3Transform((Vector3){ 0.0f, 0.0f, -1.0f }, handRot);
        Vector3 rayEnd = Vector3Add(c->position, Vector3Scale(forward, 3.0f));
        DrawLine3D(c->position, rayEnd, handColor);

        if (c->grip > 0.5f)
        {
            DrawCubeWires(c->position, 0.08f, 0.08f, 0.08f, handColor);
        }
    }
    // ------------------------------------------------------------------
}

#endif // PLATFORM_WEB

//--------------------------------------------------------------------------------------------
// Non-XR fallback: the original 2D template, shown before entering VR
//--------------------------------------------------------------------------------------------
void UpdateDrawFrame(void)
{
#if defined(PLATFORM_WEB)
    if (XR_IsActive()) return;   // rendering is fully driven by XR_RenderFrame() while in VR
#endif

    frameCounter++;

    BeginTextureMode(target);
        ClearBackground(RAYWHITE);

        DrawRectangle(70, 90, 200, 200, BLACK);
        DrawRectangle(70 + 16, 90 + 16, 200 - 32, 200 - 32, RAYWHITE);
        DrawText("raylib", 70 + 200 - MeasureText("raylib", 40) - 32, 90 + 200 - 40 - 24, 40, BLACK);

        DrawText("6.x", 290, 90 - 26, 280, BLACK);
        DrawText("GAMEJAM", 70, 90 + 210, 120, MAROON);

        if ((frameCounter / 20) % 2) DrawText("are you ready?", 160, 500, 50, BLACK);

        DrawRectangleLinesEx((Rectangle){ 0, 0, screenWidth, screenHeight }, 16, BLACK);

#if defined(PLATFORM_WEB)
        DrawText("Click 'Enter VR' to try WebXR", 40, 640, 20, DARKGRAY);
#endif

    EndTextureMode();

    BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawTexturePro(target.texture,
            (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height },
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height },
            (Vector2){ 0, 0 }, 0.0f, WHITE);

    EndDrawing();
}

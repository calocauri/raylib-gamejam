#ifndef GAME_H
#define GAME_H

// Called once after InitWindow(), before the main loop starts.
void Game_Init(void);

// Called once at shutdown, before CloseWindow().
void Game_Shutdown(void);

#if defined(PLATFORM_WEB)
// Called once per real WebXR frame (NOT per eye), with the real elapsed
// time since the last frame, in seconds. Implementations typically run a
// fixed-timestep accumulator here before stepping physics, since WebXR
// frame rate varies by headset (72/90/120Hz) and physics must not.
void Game_Update(float realDeltaTime);

// Called once per eye by the WebXR renderer (xr.c), with the correct
// projection/view matrices already active on the rlgl matrix stack.
// Just draw your 3D scene here, as you would inside a normal
// BeginMode3D() block -- no view/proj params needed.
void Game_DrawScene(void);
#endif

#endif // GAME_H

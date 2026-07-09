#ifndef XR_H
#define XR_H

#include "raylib.h"

#if defined(PLATFORM_WEB)

// Per-controller data, updated from JS every WebXR frame (see minshell.html)
typedef struct XRControllerData {
    bool connected;
    Vector3 position;
    Quaternion orientation;
    float trigger;           // 0.0 - 1.0
    float grip;              // 0.0 - 1.0
    float thumbX, thumbY;    // thumbstick / touchpad axes, -1.0 - 1.0
    bool buttonA;            // primary face button (A / X)
    bool buttonB;            // secondary face button (B / Y)
    bool thumbClick;         // thumbstick pressed in
} XRControllerData;

// True while a WebXR session is active (between "Enter VR" and session end)
bool XR_IsActive(void);

// Read-only access to controller state. hand: 0 = left, 1 = right.
// Always returns a valid pointer; check ->connected before using the data.
const XRControllerData *XR_GetController(int hand);

#endif // PLATFORM_WEB

#endif // XR_H

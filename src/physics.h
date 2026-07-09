#ifndef PHYSICS_H
#define PHYSICS_H

#include "raylib.h"

#if defined(PLATFORM_WEB)

#include "box3d/box3d.h"

// Opaque handle to a physics body, returned by the Create* functions below
// and passed back into the Get* functions to query its current transform.
typedef b3BodyId PhysicsBodyId;

void Physics_Init(void);
void Physics_Shutdown(void);

// Advances the world by exactly one fixed step of `fixedDeltaTime` seconds.
// Call this from a fixed-timestep accumulator -- never with a variable dt.
void Physics_Step(float fixedDeltaTime);

// Creates a static box (zero mass, never moves). halfExtents are in meters.
PhysicsBodyId Physics_CreateGroundBox(Vector3 position, Vector3 halfExtents);

// Creates a dynamic cube (affected by gravity/forces). halfExtent is in meters.
PhysicsBodyId Physics_CreateDynamicBox(Vector3 position, float halfExtent, float density, float friction);

Vector3    Physics_GetBodyPosition(PhysicsBodyId id);
Quaternion Physics_GetBodyRotation(PhysicsBodyId id);

#endif // PLATFORM_WEB

#endif // PHYSICS_H

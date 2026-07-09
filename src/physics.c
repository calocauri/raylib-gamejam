/*******************************************************************************************
*
*   physics.c -- thin Box3D wrapper
*
*   Owns the b3WorldId and exposes a small, generic API for creating bodies
*   and reading their transforms. Knows nothing about WebXR or the specific
*   game -- game.c decides what bodies to create and when to draw them.
*
********************************************************************************************/

#include "physics.h"

#if defined(PLATFORM_WEB)

static b3WorldId physicsWorld = { 0 };

void Physics_Init(void)
{
    b3WorldDef worldDef = b3DefaultWorldDef();
    worldDef.gravity = (b3Vec3){ 0.0f, -10.0f, 0.0f };
    physicsWorld = b3CreateWorld(&worldDef);
}

void Physics_Shutdown(void)
{
    b3DestroyWorld(physicsWorld);
}

void Physics_Step(float fixedDeltaTime)
{
    // 4 sub-steps is Box3D's suggested default (240Hz internal constraint solve at 60Hz)
    b3World_Step(physicsWorld, fixedDeltaTime, 4);
}

PhysicsBodyId Physics_CreateGroundBox(Vector3 position, Vector3 halfExtents)
{
    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.position = (b3Vec3){ position.x, position.y, position.z };
    b3BodyId bodyId = b3CreateBody(physicsWorld, &bodyDef);

    b3BoxHull hull = b3MakeBoxHull(halfExtents.x, halfExtents.y, halfExtents.z);
    b3ShapeDef shapeDef = b3DefaultShapeDef();
    b3CreateHullShape(bodyId, &shapeDef, &hull.base);

    return bodyId;
}

PhysicsBodyId Physics_CreateDynamicBox(Vector3 position, float halfExtent, float density, float friction)
{
    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.type = b3_dynamicBody;
    bodyDef.position = (b3Vec3){ position.x, position.y, position.z };
    b3BodyId bodyId = b3CreateBody(physicsWorld, &bodyDef);

    b3BoxHull hull = b3MakeCubeHull(halfExtent);
    b3ShapeDef shapeDef = b3DefaultShapeDef();
    shapeDef.density = density;
    shapeDef.baseMaterial.friction = friction;
    b3CreateHullShape(bodyId, &shapeDef, &hull.base);

    return bodyId;
}

Vector3 Physics_GetBodyPosition(PhysicsBodyId id)
{
    b3Vec3 p = b3Body_GetPosition(id);
    return (Vector3){ p.x, p.y, p.z };
}

Quaternion Physics_GetBodyRotation(PhysicsBodyId id)
{
    b3Quat q = b3Body_GetRotation(id);
    return (Quaternion){ q.v.x, q.v.y, q.v.z, q.s };
}

#endif // PLATFORM_WEB

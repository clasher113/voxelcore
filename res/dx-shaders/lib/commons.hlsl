#ifndef COMMONS_HLSL_
#define COMMONS_HLSL_

#include "constants.hlsl"

float3 apply_planet_curvature(float3 modelPos, float3 pos3d) {
    modelPos.y -= pow(length(pos3d.xz) * CURVATURE_FACTOR, 3.0);
    return modelPos;
}

#endif // COMMONS_HLSL_
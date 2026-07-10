#ifndef CONSTANTS_HLSL_
#define CONSTANTS_HLSL_

#define PI 3.1415926535897932384626433832795f
#define PI2 (PI*2)

// geometry
#define CURVATURE_FACTOR 0.002f

// lighting
#define SKY_LIGHT_MUL 2.9f
#define SKY_LIGHT_TINT (float3(1.0, 0.95, 0.9) * 2.0)
#define MIN_SKY_LIGHT float3(0.2f, 0.25f, 0.33f)
// fog
#define FOG_POS_SCALE float3(1.f, 0.2f, 1.f)

#endif // CONSTANTS_HLSL_

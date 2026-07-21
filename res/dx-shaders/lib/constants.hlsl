#ifndef CONSTANTS_HLSL_
#define CONSTANTS_HLSL_

#define PI 3.1415926535897932384626433832795f
#define PI2 (PI*2)

// geometry
#define CURVATURE_FACTOR 0.002f

// lighting
#define SKY_LIGHT_MUL 2.5f
#define SKY_LIGHT_TINT (float3(1.0, 0.75, 0.6) * 2.0)
// fog
#define FOG_POS_SCALE float3(1.f, 0.2f, 1.f)

#endif // CONSTANTS_HLSL_

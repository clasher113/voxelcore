#ifndef FOG_HLSL_
#define FOG_HLSL_

cbuffer FogCBuff : register(b1) {
    float u_fogFactor;
    float u_fogCurve;
    float u_weatherFogOpacity;
    float u_weatherFogDencity;
    float u_weatherFogCurve;
}

float calc_fog(float depth) {
    return min(
        1.0,
        max(pow(abs(depth * u_fogFactor), u_fogCurve),
            min(pow(abs(depth * u_weatherFogDencity), u_weatherFogCurve),
                u_weatherFogOpacity))
    );
}

#endif // FOG_HLSL_
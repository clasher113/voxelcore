struct VSInput {
    float3 position : POSITION0;
    float4 color : COLOR0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

cbuffer CBuff : register(b0) {
    float4x4 c_projview;
}

PSInput VShader(VSInput input) {
    PSInput output;
    output.color = input.color;
    output.position = mul(float4(input.position, 1.f), c_projview);
    return output;
}

static const float PI = 3.1415926f;
static const float fRatio = 2.0f;

cbuffer CBuff : register(b1) {
    float c_lineWidth;
}

void addHalfCircle(inout TriangleStream<PSInput> triangleStream, int nCountTriangles, float4 linePointToConnect, float fPointWComponent, float fAngle, float4 color)
{
    float fThickness = c_lineWidth / 1000.f;
    
    PSInput output = (PSInput) 0;
    for (int nI = 0; nI < nCountTriangles; ++nI)
    {
        output.position.x = cos(fAngle + (PI / nCountTriangles * nI)) * fThickness / fRatio;
        output.position.y = sin(fAngle + (PI / nCountTriangles * nI)) * fThickness;
        output.position.z = 0.0f;
        output.position.w = 0.0f;
        output.position += linePointToConnect;
        output.position *= fPointWComponent;
        output.color = color;
        triangleStream.Append(output);

        output.position = linePointToConnect * fPointWComponent;
        triangleStream.Append(output);

        output.position.x = cos(fAngle + (PI / nCountTriangles * (nI + 1))) * fThickness / fRatio;
        output.position.y = sin(fAngle + (PI / nCountTriangles * (nI + 1))) * fThickness;
        output.position.z = 0.0f;
        output.position.w = 0.0f;
        output.position += linePointToConnect;
        output.position *= fPointWComponent;
        output.color = color;
        triangleStream.Append(output);

        triangleStream.RestartStrip();
    }
}

[maxvertexcount(42)]
void GShader(line PSInput input[2], inout TriangleStream<PSInput> triangleStream) {
    PSInput output = (PSInput) 0;
    
    float fThickness = c_lineWidth / 1000.f;

    int nCountTriangles = 6;

    float4 positionPoint0Transformed = input[0].position;
    float4 positionPoint1Transformed = input[1].position;
    float4 color0 = input[0].color;
    float4 color1 = input[1].color;

    float fPoint0w = positionPoint0Transformed.w;
    float fPoint1w = positionPoint1Transformed.w;

    //calculate out the W parameter, because of usage of perspective rendering
    positionPoint0Transformed.xyz = positionPoint0Transformed.xyz / positionPoint0Transformed.w;
    positionPoint0Transformed.w = 1.0f;
    positionPoint1Transformed.xyz = positionPoint1Transformed.xyz / positionPoint1Transformed.w;
    positionPoint1Transformed.w = 1.0f;

    //calculate the angle between the 2 points on the screen
    float3 positionDifference = positionPoint0Transformed.xyz - positionPoint1Transformed.xyz;
    float3 coordinateSysten = float3(1.0f, 0.0f, 0.0f);

    positionDifference.z = 0.0f;
    coordinateSysten.z = 0.0f;

    float fAngle = acos(dot(positionDifference.xy, coordinateSysten.xy) / (length(positionDifference.xy) * length(coordinateSysten.xy)));

    if (cross(positionDifference, coordinateSysten).z < 0.0f)
    {
        fAngle = 2.0f * PI - fAngle;
    }

    fAngle *= -1.0f;
    fAngle -= PI * 0.5f;

    //first half circle of the line
    //addHalfCircle(triangleStream, nCountTriangles, positionPoint0Transformed, fPoint0w, fAngle, color0);
    //addHalfCircle(triangleStream, nCountTriangles, positionPoint1Transformed, fPoint1w, fAngle + PI, color1);

    //connection between the two circles
    //triangle1
    output.position.x = cos(fAngle) * fThickness / fRatio;
    output.position.y = sin(fAngle) * fThickness;
    output.position.z = 0.0f;
    output.position.w = 0.0f;
    output.position += positionPoint0Transformed;
    output.position *= fPoint0w; //undo calculate out the W parameter, because of usage of perspective rendering
    output.color = color0;
    triangleStream.Append(output);

    output.position.x = -cos(fAngle) * fThickness / fRatio;
    output.position.y = -sin(fAngle) * fThickness;
    output.position.z = 0.0f;
    output.position.w = 0.0f;
    output.position += positionPoint0Transformed;
    output.position *= fPoint0w;
    output.color = color0;
    triangleStream.Append(output);

    output.position.x = -cos(fAngle) * fThickness / fRatio;
    output.position.y = -sin(fAngle) * fThickness;
    output.position.z = 0.0f;
    output.position.w = 0.0f;
    output.position += positionPoint1Transformed;
    output.position *= fPoint1w;
    output.color = color1;
    triangleStream.Append(output);

    //triangle2
    output.position.x = cos(fAngle) * fThickness / fRatio;
    output.position.y = sin(fAngle) * fThickness;
    output.position.z = 0.0f;
    output.position.w = 0.0f;
    output.position += positionPoint0Transformed;
    output.position *= fPoint0w;
    output.color = color0;
    triangleStream.Append(output);

    output.position.x = cos(fAngle) * fThickness / fRatio;
    output.position.y = sin(fAngle) * fThickness;
    output.position.z = 0.0f;
    output.position.w = 0.0f;
    output.position += positionPoint1Transformed;
    output.position *= fPoint1w;
    output.color = color1;
    triangleStream.Append(output);

    output.position.x = -cos(fAngle) * fThickness / fRatio;
    output.position.y = -sin(fAngle) * fThickness;
    output.position.z = 0.0f;
    output.position.w = 0.0f;
    output.position += positionPoint1Transformed;
    output.position *= fPoint1w;
    output.color = color1;
    triangleStream.Append(output);
}

float4 PShader(PSInput input) : SV_TARGET {
    return input.color;
}
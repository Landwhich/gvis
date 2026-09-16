/*
 *  need to update to use proper attributes and VBOs soon
 */


/*
 * these are the coors after vertex shader stage 
 * y is flipped in vulkan compared to classical math notation
 *
 *   -------------------
 *  |(-1,-1)    ( 1, -1)|
 *  |                   |
 *  |                   |
 *  |(-1, 1)    ( 1,  1)|
 *   -------------------
 *
 * z is also 0 to 1 
 */


/*
 */
struct VSInput {
    float2 inPosition;
    float3 inColor;
};

struct VSOutput {
    float4 pos : SV_Position;
    float3 color;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.inPosition, 0.0, 1.0);
    output.color = input.inColor;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    return float4(vertIn.color, 1.0);
}


static const float2 positions[3] = {
    float2(0.0, -0.5),
    float2(-0.5, 0.5),
    float2(0.5, 0.5),
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR;
};

[shader("vertex")]
VertexOutput vertMain(uint vertexID: SV_VertexID) {
    return VertexOutput(
        float4(positions[vertexID], 1.0),
        float3(positions[vertexID],0)
    );
}

[shader("fragment")]
float4 fragMain(float3 color: COLOR): SV_Target {
    return float4(color.rgb, 1.0);
}
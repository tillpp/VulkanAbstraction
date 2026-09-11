static const float2 positions[3] = {
    float2(0.0, -0.5),
    float2(-0.5, 0.5),
    float2(0.5, 0.5),
};
static const float2 uvs[3] = {
    float2(0.5, 0),
    float2(0, 1),
    float2(1, 1),
};
struct VertexOutput {
    float4 position : SV_Position;
    float2 uv       : UV;
};

[[vk::binding(0, 0)]]
Sampler2D texture[];
[[vk::binding(1,0)]]
Sampler2D texture2[];

[shader("vertex")]
VertexOutput vertMain(uint vertexID: SV_VertexID) {
    return VertexOutput(
        float4(positions[vertexID], 1.0),
        uvs[vertexID]
    );
}

[shader("fragment")]
float4 fragMain(float2 uv: UV) : SV_Target {
    int index = uv.x > 0.5;
    return texture2[NonUniformResourceIndex(index)].Sample(uv)+float4(1,1,1,1)*0.1;
}
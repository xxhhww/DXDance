struct a2v 
{
	float3 lsPos : POSITION;
	float3 lsNormal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct v2p
{
	float4 csPos : SV_POSITION;
	float2 uv : TEXCOORD;
};

v2p VSMain(a2v input)
{
	v2p output;
	output.csPos = float4(input.lsPos, 1.0f);
	output.uv = input.uv;

	return output;
}

float4 PSMain(v2p input) : SV_TARGET
{
	return float4(0.5f, 0.5f, 0.5f, 1.0f);
}
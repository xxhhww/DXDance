#ifndef _AxisPicking__
#define _AxisPicking__

struct PassData {
	float4x4 modelMatrix;
	float4x4 viewMatrix;
	float4x4 projMatrix;
	float3   viewPos;
	int      highlightedAxis;
};
#define PassDataType PassData

#include "../Engine/Base/MainEntryPoint.hlsl"

struct a2v{
	float3 lsPos     : POSITION;
	float2 uv        : TEXCOORD;
	float3 lsNormal  : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
};

struct v2p{
	float4 csPos : SV_POSITION;
	float3 color : COLOR;
};

float4x4 rotationMatrix(float3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return float4x4
    (
        oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
        oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
        oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
        0.0,                                0.0,                                0.0,                                1.0
    );
}

v2p VSMain(a2v input, uint instanceID : SV_InstanceID)
{
	v2p output;

	float4x4 instanceModelMatrix = PassDataCB.modelMatrix;
	if(instanceID == 1){
		// X axis
		instanceModelMatrix = mul(rotationMatrix(float3(0.0f, 1.0f, 0.0f), radians(-90.0f)), instanceModelMatrix); /* X axis */
		output.color = float3(1.0f, 1.0f, 252.0f / 255.0f);
		// output.color = float3(1.0f, 0.0f, 0.0f);
	}
	else if(instanceID == 2){
		// Y axis
		instanceModelMatrix = mul(rotationMatrix(float3(1.0f, 0.0f, 0.0f), radians(90.0f)), instanceModelMatrix);
		output.color = float3(1.0f, 1.0f, 253.0f / 255.0f);
		// output.color = float3(0.0f, 1.0f, 0.0f);
	}
	else{
		// Z axis
		output.color = float3(1.0f, 1.0f, 254.0f / 255.0f);
		// output.color = float3(0.0f, 0.0f, 1.0f);
	}
	float3 modelWorldPosition = instanceModelMatrix._m03_m13_m23;
	float distanceToCamera = distance(PassDataCB.viewPos.xyz, modelWorldPosition);

	float4 wsPos = mul(float4(input.lsPos * distanceToCamera * 0.001f, 1.0f), instanceModelMatrix);
	float4 vsPos = mul(wsPos, PassDataCB.viewMatrix);
	output.csPos = mul(vsPos, PassDataCB.projMatrix);

	return output;
}

float4 PSMain(v2p input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}

#endif
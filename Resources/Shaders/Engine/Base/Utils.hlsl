#ifndef _Utils__
#define _Utils__

// v is a unit vector. The result is an octahedral vector on the [-1, +1] square.
float2 OctEncode(float3 v) {
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    float2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0) {
        result = (1.0 - abs(result.yx)) * sign(result.xy);
    }
    return result;
}

// Returns a unit vector. Argument o is an octahedral vector on the [-1, +1] square
float3 OctDecode(float2 o) {
    float3 v = float3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0.0) {
        v.xy = (1.0 - abs(v.yx)) * sign(v.xy);
    }
    return normalize(v);
}

float2 TexelIndexToUV(uint2 index, uint2 textureSize) {
    return float2(index + 0.5) / textureSize;
}

float2 NDCToUV(float3 ndcPoint) {
    float2 uv = (ndcPoint.xy + 1.0) * 0.5; // [-1; 1] to [0; 1]
    uv.y = 1.0 - uv.y; // Conform DX specs
    return uv;
}

float3 GetLODColor(uint lodLevel) {
	float3 lodDebugColor = float3(0.0f, 0.0f, 0.0f);
	if(lodLevel == 0u) {
		lodDebugColor = float3(0.5f, 0.5f, 0.5f);
	}
	else if(lodLevel == 1u) {
		lodDebugColor = float3(0.0f, 0.0f, 1.0f);
	}
	else if(lodLevel == 2u) {
		lodDebugColor = float3(1.0f, 0.0f, 0.0f);
	}
	else if(lodLevel == 3u) {
		lodDebugColor = float3(0.0f, 1.0f, 0.0f);
	}
	else if(lodLevel == 4u) {
		lodDebugColor = float3(1.0f, 1.0f, 0.0f);
	}
	else if(lodLevel == 5u) {
		lodDebugColor = float3(0.0f, 1.0f, 1.0f);
	}
	else if(lodLevel == 6u) {
		lodDebugColor = float3(1.0f, 0.0f, 1.0f);
	}
	else if(lodLevel == 7u) {
		lodDebugColor = float3(0.25f, 0.75f, 0.0f);
	}
	else if(lodLevel == 8u) {
		lodDebugColor = float3(0.0f, 0.25f, 0.75f);
	}
	else if(lodLevel == 9u) {
		lodDebugColor = float3(0.0f, 0.75f, 0.25f);
	}
	else { 
		lodDebugColor = float3(1.0f, 1.0f, 0.0f);
	}
	return lodDebugColor;
}


#endif
#ifndef _TerrainHelper__
#define _TerrainHelper__

//一个Node拆成8x8个Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct NodeDescriptor {
	uint isBranch;
	float pad1;
	float pad2;
	float pad3;
};

struct LODDescriptor {
	uint nodeSize;         // 该LOD中每一个Node的边长(米)(Node是正方形)
	uint nodeStartOffset;  // 该LOD中的第一个Node的开始偏移量
	uint nodeCount;
	float pad2;
};

struct RenderPatch {
	float2 position;
	float2 minMaxHeight;
	uint lod;
	float pad1;
	float pad2;
	float pad3;
};

struct BoundingBox {
    float4 minPosition;
    float4 maxPosition;
};

struct TriplanarMapping {
	float2 uvX, uvY, uvZ;
	float3 weights;

	void initialize(float3 position, float3 normal, float3 textureScale, float sharpness) {
		uvX = position.zy * textureScale.x;
		uvY = position.xz * textureScale.y;
		uvZ = position.xy * textureScale.z;

		weights = pow(abs(normal), sharpness);
		weights /= dot(weights, 1.0f);
	}

	float3 normalmap(float3 normal, float3 tnormalX, float3 tnormalY, float3 tnormalZ) {
		// Whiteout blend: https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a

		tnormalX = float3(
			tnormalX.xy + normal.zy,
			abs(tnormalX.z) * normal.x
			);
		tnormalY = float3(
			tnormalY.xy + normal.xz,
			abs(tnormalY.z) * normal.y
			);
		tnormalZ = float3(
			tnormalZ.xy + normal.xy,
			abs(tnormalZ.z) * normal.z
			);

		return normalize(
			tnormalX.zyx * weights.x +
			tnormalY.xzy * weights.y +
			tnormalZ.xyz * weights.z
		);
	}
};

#endif
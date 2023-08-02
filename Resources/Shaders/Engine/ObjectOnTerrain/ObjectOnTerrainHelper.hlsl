#ifndef _ObjectOnTerrainHelper__
#define _ObjectOnTerrainHelper__

// 安置点的信息
struct Placement {
	// 基础信息
	float3 position;	// 安置点的坐标
	float2 facing;		// 安置点的朝向
	uint   type;		// 安置物类型(Grass / Tree / Stone)
	uint   lod;			// LOD级别
	float  height;		// ?

	// 材质信息
	// ...
};

struct BoundingBox {
    float4 minPosition;
    float4 maxPosition;
};

// Helper Function For Grass
struct GrassBendSettings {
	float relTipOffsetZ;
	float controlPointZ;
	float controlPointY;
};

static float GetRelY(uint vertexID, uint numVertices, uint lod) {
	uint numLevels = numVertices >> 1 >> lod;

	uint vertical = vertexID >> 1 >> lod;
	return (float)vertical * (1.0f / numLevels);
}

static float2 GetGrassUV(Placement blade, uint vertexID, uint numVertices) {
	uint leftRight = vertexID & 1;
	float relX = (float)leftRight * 2.f - 1.f;
	relX *= (vertexID != numVertices - 1);

	float relY0 = GetRelY(vertexID, numVertices, 0);
	float relY1 = GetRelY(vertexID, numVertices, 1);

	float relY = lerp(relY0, relY1, blade.lod);

	float2 uv = float2(relX, relY);

	return uv;
}

static float3 GetGrassPosition(Placement blade, float2 uv, float height, float halfWidth, GrassBendSettings bend, float2 wind) {
	float relX = uv.x;
	float relY = uv.y;
	float relY2 = relY * relY;

	float2 zy = float2(bend.controlPointZ, bend.controlPointY) * (2.f * relY - 2.f * relY2) + float2(bend.relTipOffsetZ, 1.f) * relY2;

	zy *= height;

	float x = relX * halfWidth;
	float y = zy.y;
	float z = zy.x;

	float2 facing = blade.facing;

	float3 position = float3(
		facing.y * x - facing.x * z,
		y,
		facing.x * x + facing.y * z);

	position.xz += wind * relY;
	position += blade.position;

	return position;
}

static float3 GetGrassNormal(Placement blade, float2 uv, float height, GrassBendSettings bend, float2 wind) {
	float relY = uv.y;

	float2 d_zy = float2(bend.controlPointZ, bend.controlPointY) * (2.f - 4.f * relY) + float2(bend.relTipOffsetZ, 1.f) * (2.f * relY);

	d_zy *= height;

	float2 facing = blade.facing;

	float z = d_zy.x;
	float y = d_zy.y;

	float d_x = height * (-facing.x * z);
	float d_y = height * y;
	float d_z = height * (facing.y * z);

	d_x += wind.x;
	d_z += wind.y;

	float3 tangent = float3(facing.y, 0.f, facing.x);
	float3 normal = cross(tangent, float3(d_x, d_y, d_z));

	return normal;
}

#endif
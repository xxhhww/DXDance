#ifndef _TerrainHeader__
#define _TerrainHeader__

//一个Node拆成8x8个Patch
#define PATCH_COUNT_PER_NODE_PER_AXIS 8

struct RenderPatch {
	float2 position;
	float2 minmaxHeight;
	
	uint3  nodeLoc;				// 所属地形节点
	float  pad1;
	
	uint2  patchOffset;			// 地形节点内部偏移
	float  pad2;
	float  pad3;

	uint4  lodTrans;
};

struct TerrainLodDescriptor {
	uint  nodeMeterSize;		// 该LOD中每一个Node的边长(米)(Node是正方形)
	uint  nodeStartOffset;		// 该LOD中的第一个Node的开始偏移量
	uint  nodeCount;			// 该LOD中的Node的总个数

	float pad1;
};

struct TerrainNodeDescriptor {
	float minHeight;	// 0 - 1
	float maxHeight;
	
	uint  tilePosX;		// 255表示资源未加载
	uint  tilePosY;		// 255表示资源未加载
};

struct TerrainNodeGpuRuntimeState {
	uint  branch;
	float pad1;
	float pad2;
	float pad3;
};

struct GpuUpdateTerrainNodeDescriptorRequest {
	uint srcTerrainNodeIndex;
	uint dstTerrainNodeIndex;						// 目标地形节点索引
	
	uint tilePosX;
	uint tilePosY;
};

struct GpuUpdateRuntimeVTAtlasRequest {
	float4x4 mvpMatrix;
	float4   tileOffset;
	float4   blendOffset;
};

struct GpuUpdateRuntimeVTPageTableRequest {
	uint  tilePosX;		// pageTable节点所需纹理对应图集中的位置
	uint  tilePosY;
	int   pageLevel;	// pageLevel
	float pad1;

	int4  rectInPage0Level;
	float4x4 mvpMatrix;	// 转换到图片空间中的矩阵
};


// ==========================================Grassland==========================================

struct GrasslandNodeDescriptor {
	uint  tileIndex;				// 255表示资源未烘焙
	uint  nodeLocationX;
	uint  nodeLocationY;
	float nodeMeterSize;
};

struct GpuGrasslandNodeRequestTask {
	uint prevGrasslandNodeIndex;	// 前任地形节点索引
	uint nextGrasslandNodeIndex;	// 下任地形节点索引

	uint tileIndex;
};

struct ClumpParameter {
	float pullToCentre;
	float pointInSameDirection;
	float baseHeight;
	float heightRandom;
	float baseWidth;
	float widthRandom;
	float baseTilt;
	float tiltRandom;
	float baseBend;
	float bendRandom;
};

struct GrassBladeDescriptor {
	float3   position;
	float2   facing;

	float    hash;
	float    height;
	float    width;
	float    tilt;			// 描述草叶的倾斜状态
	float    bend;			// 控制草叶的弯曲(其实就是控制贝塞尔样条曲线)
	float    sideCurve;		// 控制草叶的边的弯曲
};

// 返回一个0 - 1的随机浮点数
float2 hashwithoutsine22(float2 p) {
	float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return frac((p3.xx+p3.yz)*p3.zy);
}

// 返回一个0 - 1的随机浮点数
float rand(float3 co) {
	return frac(sin(dot(co.xyz, float3(12.9898, 78.233, 53.539))) * 43758.5453);
}

float remap01_neg11(float t){ 
    return (t * 2.0f) - 1.0f;
}

float3 cubicBezier(float3 p0, float3 p1, float3 p2, float3 p3, float t) {
    float3 a = lerp(p0, p1, t);
    float3 b = lerp(p2, p3, t);
    float3 c = lerp(p1, p2, t);
    float3 d = lerp(a, c, t);
    float3 e = lerp(c, b, t);
    return lerp(d, e, t); 
}

/*
计算贝塞尔切线
*/
float3 bezierTangent(float3 p0, float3 p1, float3 p2, float3 p3, float t) {
    float omt  = 1.0f - t;
    float omt2 = omt * omt;
    float t2   = t * t;

    float3 tangent = 
        p0* (-omt2) +
        p1 * (3 * omt2 - 2 *omt) +
        p2 * (-3 * t2 + 2 * t) +
        p3 * (t2);
                     
    return normalize(tangent);
}

float3x3 AngleAxis3x3(float angle, float3 axis) {
	float c, s;
	sincos(angle, s, c);

	float t = 1 - c;
	float x = axis.x;
	float y = axis.y;
	float z = axis.z;

	return float3x3(
		t * x * x + c, t * x * y - s * z, t * x * z + s * y,
		t * x * y + s * z, t * y * y + c, t * y * z - s * x,
		t * x * z - s * y, t * y * z + s * x, t * z * z + c
		);
}

#endif
#ifndef _OceanHelper__
#define _OceanHelper__

static const uint numWaves = 2u;

struct WaterParameter{
	float4 waterSurfaceColor;
    float4 waterRefractionColor;
    float4 ssrSettings;         // step size, max steps forward, max steps back, foreground reduction factor
    float4 normalMapScroll;

    float2 normalMapScrollSpeed;
    float  refractionDistortionFactor;
    float  refractionHeightFactor;

    float  refractionDistanceFactor;
    float  depthSofteningDistance;
    float  foamHeightStart;
    float  foamFadeDistance;

    float  foamTiling;
    float  foamAngleExponent;
    float  roughness;
    float  reflectance;

    float  specIntensity;
    float  foamBrightness;
    float  tessellationFactor;
    float  dampeningFactor;

    uint   waterNormalMap1Index;
    uint   waterNormalMap2Index;
    uint   waterFoamMapIndex;
    uint   waterNoiseMapIndex;
};

struct WaveResult {
    float3 position;
    float3 normal;
    float3 binormal;
    float3 tangent;
};
 
struct Wave {
    float3 direction;
    float  steepness;
    float  waveLength;
    float  amplitude;
    float  speed;
};

WaveResult CalculateWave(Wave wave, float3 wavePosition, float edgeDampen, float time) {
    WaveResult result;
 
    float frequency = 2.0 / wave.waveLength;
    float phaseConstant = wave.speed * frequency;
    float qi = wave.steepness / (wave.amplitude * frequency * numWaves);
    float rad = frequency * dot(wave.direction.xz, wavePosition.xz) + time * phaseConstant;
    float sinR = sin(rad);
    float cosR = cos(rad);
 
    result.position.x = wavePosition.x + qi * wave.amplitude * wave.direction.x * cosR * edgeDampen;
    result.position.z = wavePosition.z + qi * wave.amplitude * wave.direction.z * cosR * edgeDampen;
    result.position.y = wave.amplitude * sinR * edgeDampen;
 
    float waFactor = frequency * wave.amplitude;
    float radN = frequency * dot(wave.direction, result.position) + time * phaseConstant;
    float sinN = sin(radN);
    float cosN = cos(radN);
 
    result.binormal.x = 1 - (qi * wave.direction.x * wave.direction.x * waFactor * sinN);
    result.binormal.z = -1 * (qi * wave.direction.x * wave.direction.z * waFactor * sinN);
    result.binormal.y = wave.direction.x * waFactor * cosN;
 
    result.tangent.x = -1 * (qi * wave.direction.x * wave.direction.z * waFactor * sinN);
    result.tangent.z = 1 - (qi * wave.direction.z * wave.direction.z * waFactor * sinN);
    result.tangent.y = wave.direction.z * waFactor * cosN;
 
    result.normal.x = -1 * (wave.direction.x * waFactor * cosN);
    result.normal.z = -1 * (wave.direction.z * waFactor * cosN);
    result.normal.y = 1 - (qi * waFactor * sinN);
 
    result.binormal = normalize(result.binormal);
    result.tangent = normalize(result.tangent);
    result.normal = normalize(result.normal);
 
    return result;
}

#endif
#ifndef _SkyHelper__
#define _SkyHelper__

static uint hash(uint x) {
	x += (x << 10u);
	x ^= (x >> 6u);
	x += (x << 3u);
	x ^= (x >> 11u);
	x += (x << 15u);
	return x;
}

// Compound versions of the hashing algorithm I whipped together.
static uint hash(uint2 v) { return hash(v.x ^ hash(v.y)); }
static uint hash(uint3 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z)); }
static uint hash(uint4 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w)); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
static float floatConstruct(uint m) {
	const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
	const uint ieeeOne = 0x3F800000u;		// 1.0 in IEEE binary32

	m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
	m |= ieeeOne;                          // Add fractional part to 1.0

	float  f = asfloat(m);       // Range [1:2]
	return (f - 1.0f);           // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
static float random(float  x) { return floatConstruct(hash(asuint(x))); }
static float random(float2 v) { return floatConstruct(hash(asuint(v))); }
static float random(float3 v) { return floatConstruct(hash(asuint(v))); }
static float random(float4 v) { return floatConstruct(hash(asuint(v))); }

static float getStars(float3 V) {
	const float scale = 60.f;
	float3 id = floor(V * scale);
	float d = length(scale * V - (id + 0.5f));

	float2 uv = id.xy + float2(37.f, 17.f) * id.z;
	float rnd = random(uv + 0.5f);

	// https://www.shadertoy.com/view/ttScDc
	float stars = sqrt(0.075f / max(d, 1e-7f)) * (rnd.x > 0.92f && d < 0.15f);

	return stars;
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float fbmNoise(float2 st) {
	float2 i = floor(st);
	float2 f = frac(st);

	// Four corners in 2D of a tile
	float a = random(i);
	float b = random(i + float2(1.f, 0.f));
	float c = random(i + float2(0.f, 1.f));
	float d = random(i + float2(1.f, 1.f));

	float2 u = f * f * (3.f - 2.f * f);

	return lerp(a, b, u.x) +
		(c - a) * u.y * (1.f - u.x) +
		(d - b) * u.x * u.y;
}

#define FBM_OCTAVES 6
float cloudFBM(float2 st) {
	// Initial values
	float value = 0.f;
	float amplitude = .5f;
	float frequency = 0.f;
	//
	// Loop of octaves
	for (int i = 0; i < FBM_OCTAVES; ++i)
	{
		value += amplitude * fbmNoise(st);
		st *= 2.;
		amplitude *= .5;
	}
	return value;
}

static float getCloudValue(float2 p, float intensity) {
	const float density = 0.5f;
	const float sharpness = 0.1f;
	const float scale = 1.f / 0.05f;

	float noise = cloudFBM(p);

	noise = saturate(1.f - exp(-(noise - density) * sharpness)) * scale;
	return noise * intensity;
}

static float getClouds(float3 V) {
	const float height = 1000.f;

	float ndotd = -V.y;

	float result = 0.f;
	if (abs(ndotd) >= 1e-6f) {
		float t = -height / ndotd;
		if (t > 0.f) {
			float3 hit = t * V;
			float l = length(hit.xz);

			float intensity = 1.f - smoothstep(0.f, 30000.f, l);

			float2 p = hit.xz * 0.0007f;
			result = getCloudValue(p, intensity);
		}
#if 0
		const float delta = 0.1f;
		const float invDelta = 1.f / delta;

		float nx = getCloudValue(p + float2(delta, 0.f), intensity);
		float nz = getCloudValue(p + float2(0.f, delta), intensity);

		float3 N = normalize(float3(
			-(nx - value) * invDelta,
			-1.f,
			-(nz - value) * invDelta
			));

#endif
	}
	return result;
}

static float3 proceduralSky(float3 V, float3 L) {
	float LdotV = dot(V, L);

	// https://www.shadertoy.com/view/tt3cDl
	float3 skycolor = float3(0.2f, 0.4f, 0.8f) * max(0.2f, L.y);
	float3 suncolor = saturate(lerp(float3(0.99f, 0.3f, 0.1f), float3(1.f, 1.f, 0.8f), L.y));
	float3 sunhalo =
		lerp(
			max(0.f, (1.f - max(0.f, (1.f - L.y * 3.f)) * V.y * 4.f)),
			0.f,
			L.y)
		* saturate(pow(saturate(0.5f * LdotV + 0.5f), (8.f - L.y * 5.f)))
		* suncolor;

	float3 color = skycolor;
	//color += saturate(2.f * pow(saturate(LdotV), 800.f)) * (suncolor + 0.4f.xxx);
	color += saturate(2.f * pow(saturate(LdotV), 2500.f)) * (suncolor + 0.4f.xxx) * 300.f;
	color += sunhalo * sunhalo;
	color += (1.f - V.y) * (1.f - V.y) * suncolor * 0.5f;

	color += (getStars(V * 1.5f) + getStars(V * 3.f)) * saturate(-0.3f - L.y);

	color += getClouds(V);

	return color;
}

#endif
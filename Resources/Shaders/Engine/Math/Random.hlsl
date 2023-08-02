#ifndef _Random__
#define _Random__

//1D
float Random1DTo1D(float value, float a, float b) {
	//make value more random by making it bigger
	float random = frac(sin(value+b)*a);
        return random;
}

float2 Random1DTo2D(float value) {
    return float2(
        Random1DTo1D(value, 14375.5964, 0.546),
        Random1DTo1D(value, 18694.2233, 0.153)
    );
}

float3 Random1DTo3D(float value) {
    return float3(
        Random1DTo1D(value,14375.5964,0.546),
        Random1DTo1D(value,18694.2233,0.153),
        Random1DTo1D(value,19663.6565,0.327)
    );
}

float4 Random1DTo4D(float value){
    return float4(
        Random1DTo1D(value,14375.5964,0.546),
        Random1DTo1D(value,18694.2233,0.153),
        Random1DTo1D(value,19663.6565,0.327),
        Random1DTo1D(value,12748.4774,0.688)
    );
}
//2D
float Random2DTo1D(float2 value,float a ,float2 b) {
	//avaoid artifacts
	float2 smallValue = sin(value);
	//get scalar value from 2d vector	
	float  random = dot(smallValue,b);
	random = frac(sin(random) * a);
	return random;
}

float2 Random2DTo2D(float2 value){
	return float2(
		Random2DTo1D(value,14375.5964, float2(15.637, 76.243)),
		Random2DTo1D(value,14684.6034,float2(45.366, 23.168))
	);
}

float3 Random2DTo3D(float2 value){
    return float3(
        Random2DTo1D(value,14375.5964,float2(15.637, 76.243)),
        Random2DTo1D(value,18694.2233,float2(45.366, 23.168)),
        Random2DTo1D(value,19663.6565,float2(62.654, 88.467))
    );
}

float4 Random2DTo4D(float2 value){
    return float4(
        Random2DTo1D(value,14375.5964,float2(15.637, 76.243)),
        Random2DTo1D(value,18694.2233,float2(45.366, 23.168)),
        Random2DTo1D(value,19663.6565,float2(62.654, 88.467)),
        Random2DTo1D(value,17635.1739,float2(44.383, 38.174))
    );
}

//3D
float Random3DTo1D(float3 value,float a,float3 b) {
	float3 smallValue = sin(value);
	float  random = dot(smallValue,b);
	random = frac(sin(random) * a);
	return random;
}

float2 Random3DTo2D(float3 value){
	return float2(
		Random3DTo1D(value,14375.5964, float3(15.637,76.243,37.168)),
		Random3DTo1D(value,14684.6034,float3(45.366, 23.168,65.918))
	);
}

float3 Random3DTo3D(float3 value){
	return float3(
		Random3DTo1D(value,14375.5964, float3(15.637,76.243,37.168)),
		Random3DTo1D(value,14684.6034,float3(45.366, 23.168,65.918)),
		Random3DTo1D(value,17635.1739,float3(62.654, 88.467,25.111))
	);
}

float4 Random3DTo4D(float3 value){
	return float4(
		Random3DTo1D(value,14375.5964, float3(15.637,76.243,37.168)),
		Random3DTo1D(value,14684.6034,float3(45.366, 23.168,65.918)),
		Random3DTo1D(value,17635.1739,float3(62.654, 88.467,25.111)),
        Random3DTo1D(value,17635.1739,float3(44.383, 38.174,67.688))	
	);
}

//4D
float Random4DTo1D(float4 value,float a ,float4 b) {			
	float4 smallValue = sin(value);
	float  random = dot(smallValue,b);
	random = frac(sin(random) * a);
	return random;
}

float2 Random4DTo2D(float4 value) {			
    return float2(		
        Random4DTo1D(value,14375.5964,float4(15.637,76.243,37.168,83.511)),
        Random4DTo1D(value,14684.6034,float4(45.366, 23.168,65.918,57.514))
	);
}

float3 Random4DTo3D(float4 value) {			
    return float3(		
        Random4DTo1D(value,14375.5964,float4(15.637,76.243,37.168,83.511)),
        Random4DTo1D(value,14684.6034,float4(45.366, 23.168,65.918,57.514)),
        Random4DTo1D(value,14985.1739,float4(62.654, 88.467,25.111,61.875))
	);
}

float4 Random4DTo4D(float4 value) {	
    return float4(		
        Random4DTo1D(value,14375.5964,float4(15.637,76.243,37.168,83.511)),
        Random4DTo1D(value,14684.6034,float4(45.366, 23.168,65.918,57.514)),
        Random4DTo1D(value,14985.1739,float4(62.654, 88.467,25.111,61.875)),
        Random4DTo1D(value,17635.1739,float4(44.383, 38.174,67.688,22.351))	
	);
}

#endif
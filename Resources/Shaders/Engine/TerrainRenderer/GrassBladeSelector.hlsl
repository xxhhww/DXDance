#ifndef _GrassBladeSelector__
#define _GrassBladeSelector__

struct PassData {
	
};

#define PassDataType PassData

#include "../Base/MainEntryPoint.hlsl"

[numthreads(1, 8, 8)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
	
}

#endif
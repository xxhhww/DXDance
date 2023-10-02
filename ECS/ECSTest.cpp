#include "Entity.h"
#include "Tools/TaskSystem.h"
#include <Jolt/Core/Memory.h>

using namespace ECS;

class CompA : public ECS::IComponent {
public:
    char a;
};

class CompB : public ECS::IComponent {
public:
    float a;
    float b;
    float c;
    float d;
};

class CompC : public ECS::IComponent {
public:
    float data[512];
};

class CompD : public ECS::IComponent {
public:
    inline CompD() {
        a = 1; b = 2; c = 3; d = 4;
    }

    int a;
    int b;
    int c;
    int d;
};

#if defined(_DEBUG) && !defined(JPH_DISABLE_CUSTOM_ALLOCATOR) && !defined(JPH_COMPILER_MINGW)

/// Register hook that detects allocations that aren't made through the custom allocator
void RegisterCustomMemoryHook();

/// Enable the custom memory hook to detect allocations not made through the custom allocator
void EnableCustomMemoryHook(bool inEnable);

/// Check if the hook is currently checking allocations
bool IsCustomMemoryHookEnabled();

#else

inline void RegisterCustomMemoryHook() { RegisterDefaultAllocator(); }

#endif // _DEBUG && !JPH_DISABLE_CUSTOM_ALLOCATOR && !JPH_COMPILER_MINGW

/// Struct that, when put on the stack, temporarily disables checking that all allocations go through the custom memory allocator
struct DisableCustomMemoryHook
{
    DisableCustomMemoryHook();
    ~DisableCustomMemoryHook();
};

// Global to turn checking on/off
static bool sEnableCustomMemoryHook = false;

// Local to temporarily disable checking
static thread_local int sDisableCustomMemoryHook = 1;

// Local to keep track if we're coming through the custom allocator
static thread_local bool sInCustomAllocator = false;

// Struct to put on the stack to flag that we're in the custom memory allocator
struct InCustomAllocator
{
    InCustomAllocator()
    {
        JPH_ASSERT(!sInCustomAllocator);
        sInCustomAllocator = true;
    }

    ~InCustomAllocator()
    {
        JPH_ASSERT(sInCustomAllocator);
        sInCustomAllocator = false;
    }
};

// Add a tag to an allocation to track if it is aligned / unaligned
static void* TagAllocation(void* inPointer, size_t inAlignment, char inMode)
{
    if (inPointer == nullptr)
        return nullptr;

    uint8_t* p = reinterpret_cast<uint8_t*>(inPointer);
    *p = inMode;
    return p + inAlignment;
}

// Remove tag from allocation
static void* UntagAllocation(void* inPointer, size_t inAlignment, char inMode)
{
    if (inPointer == nullptr)
        return nullptr;

    uint8_t* p = reinterpret_cast<uint8_t*>(inPointer) - inAlignment;
    JPH_ASSERT(*p == inMode);
    *p = 0;
    return p;
}

static void* AllocateHook(size_t inSize)
{
    InCustomAllocator ica;
    return TagAllocation(malloc(inSize + 16), 16, 'U');
}

static void FreeHook(void* inBlock)
{
    InCustomAllocator ica;
    free(UntagAllocation(inBlock, 16, 'U'));
}

static void* AlignedAllocateHook(size_t inSize, size_t inAlignment)
{
    JPH_ASSERT(inAlignment <= 64);

    InCustomAllocator ica;
    return TagAllocation(_aligned_malloc(inSize + 64, inAlignment), 64, 'A');
}

static void AlignedFreeHook(void* inBlock)
{
    InCustomAllocator ica;
    _aligned_free(UntagAllocation(inBlock, 64, 'A'));
}

static int MyAllocHook(int nAllocType, void* pvData, size_t nSize, int nBlockUse, long lRequest, const unsigned char* szFileName, int nLine) noexcept
{
    JPH_ASSERT(!sEnableCustomMemoryHook || sDisableCustomMemoryHook <= 0 || sInCustomAllocator);
    return true;
}

void RegisterCustomMemoryHook()
{
    JPH::Allocate = AllocateHook;
    JPH::Free = FreeHook;
    JPH::AlignedAllocate = AlignedAllocateHook;
    JPH::AlignedFree = AlignedFreeHook;

    _CrtSetAllocHook(MyAllocHook);
}

void EnableCustomMemoryHook(bool inEnable)
{
    sEnableCustomMemoryHook = inEnable;
}

bool IsCustomMemoryHookEnabled()
{
    return sEnableCustomMemoryHook;
}

DisableCustomMemoryHook::DisableCustomMemoryHook()
{
    sDisableCustomMemoryHook--;
}

DisableCustomMemoryHook::~DisableCustomMemoryHook()
{
    sDisableCustomMemoryHook++;
}


void TestECS() {
    RegisterCustomMemoryHook();
    JPH::JobSystem* jobSystem = new JPH::JobSystemThreadPool(2048, 8, std::thread::hardware_concurrency() - 4);
    ECS::Entity::SetJobSystem(jobSystem);

    int64_t countPerSecond = 0;
    int64_t startTime;
    int64_t currTime;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countPerSecond);
    double secondPerCount = 1.0 / (double)countPerSecond;

    float sum = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        auto entity = Entity::Create<CompA, CompB, CompC>();
        CompB& compB = entity.GetComponent<CompB>();
        compB.a = i;
        sum += i;
    }
    uint32_t cc = Entity::GetEntityCount<CompA>();

    for (int32_t i = 0; i < 500; i++) {
        Entity entity{ i };
        entity.AddComponent<CompD>();
    }

    std::atomic<int> count = 0;
    std::atomic<int> sum2 = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
    Entity::Foreach([&](Entity::ID& id, CompA& compA, CompB& compB, CompC& compC) {
        compA.a = 'c';
        count++;
        sum2.fetch_add(compB.a);
    });
    Entity::Foreach([&](Entity::ID& id, CompA& compA, CompB& compB, CompC& compC) {
        int i = 32;
    });
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    int64_t diffCount = currTime - startTime;
    int64_t deltaTime = diffCount * secondPerCount;
    int result = count;

    std::atomic<int> count2 = 0;
    Entity::Foreach([&](Entity::ID& id, CompD& compD)
        {
            count2++;
        });
    int result2 = count2;

    int sum3 = 0;
    for (int32_t i = 0; i < 1000; i++) {
        Entity entity{ i };
        entity.ForeachComp([&](ECS::IComponent* comp) {
            if (dynamic_cast<CompB*>(comp) != nullptr) {
                CompB* b = dynamic_cast<CompB*>(comp);
                sum3 += b->a;
            }
            });
    }

    uint32_t coco = Entity::GetEntityCount<CompA, CompB, CompC>();
    int i = 32;

    delete jobSystem;
}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    TestECS();

    return 0;
}
#include "Entity.h"
#include "TaskSystem.h"
#include "ServiceLocator.h"
#include <Windows.h>

using namespace Core;

struct CompA {
    char a;
};

struct CompB {
    float a;
    float b;
    float c;
    float d;
};

struct CompC {
    float data[512];
};

struct CompD {
    inline CompD() {
        a = 1; b = 2; c = 3; d = 4;
    }

    int a;
    int b;
    int c;
    int d;
};

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	std::unique_ptr<TaskSystem> tSystem = std::make_unique<TaskSystem>();
	ServiceLocator::Provide<TaskSystem>(*tSystem.get());

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

    for (int32_t i = 0; i < 500; i++) {
        Entity entity{ i };
        entity.AddComponent<CompD>();
    }

    std::atomic<int> count = 0;
    std::atomic<int> sum2 = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
    Entity::Foreach([&](Entity::ID& id, CompA& compA, CompB& compB, CompC& compC) {
        count++;
        sum2.fetch_add(compB.a);
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

	return 0;
}
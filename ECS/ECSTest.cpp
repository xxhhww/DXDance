#include "Entity.h"
#include "Tools/TaskSystem.h"

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

void TestECS() {
    std::unique_ptr<Tool::TaskSystem> tSystem = std::make_unique<Tool::TaskSystem>();

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
}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    TestECS();

    return 0;
}
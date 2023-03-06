#include "Entity.h"
#include "TaskSystem.h"
#include "ServiceLocator.h"
#include "IComponent.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"

#include "Scene.h"
#include "FooComponent.h"
#include "Transform.h"

#include <fstream>
#include <iostream>

using namespace Core;

struct CompA : IComponent {
    char a;
};

struct CompB : IComponent {
    float a;
    float b;
    float c;
    float d;
};

struct CompC : IComponent {
    float data[512];
};

struct CompD : IComponent {
    inline CompD() {
        a = 1; b = 2; c = 3; d = 4;
    }

    int a;
    int b;
    int c;
    int d;
};

void TestECS() {
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

    int sum3 = 0;
    for (int32_t i = 0; i < 1000; i++) {
        Entity entity{ i };
        entity.ForeachComp([&](IComponent* comp) {
            if (dynamic_cast<CompB*>(comp) != nullptr) {
                CompB* b = dynamic_cast<CompB*>(comp);
                sum3 += b->a;
            }
        });
    }
}

void TestRapidJson() {
    // 写入
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

    writer.StartObject();

    writer.Key("Actors");
    writer.StartArray();

    for (int i = 0; i < 10; i++) {
        writer.StartObject();

        writer.Key("ID");
        writer.Int(i);

        writer.Key("Actor");
        writer.String("rotcA");

        writer.EndObject();
    }

    writer.EndArray();

    writer.EndObject();

    std::string filename = "TestRapidjson.json";
    std::ofstream oStream;
    oStream.open(filename);

    oStream << buf.GetString() << std::endl;
    oStream.close();

    // 读取
    std::cout << "Hello RapidJson!" << std::endl;
    std::ifstream iStream(filename);
    std::string jsonData((std::istreambuf_iterator<char>(iStream)), std::istreambuf_iterator<char>());
    iStream.close();

    rapidjson::Document doc;
    if (!doc.Parse(jsonData.c_str()).HasParseError()) {
        const rapidjson::Value& s = doc.GetObj();

        if (s.HasMember("Actors") && s["Actors"].IsArray()) {
            const rapidjson::Value& array = s["Actors"];
            for (size_t i = 0; i < array.Size(); i++) {
                assert(array[i].IsObject());
                const rapidjson::Value& actor = array[i];
                
                if (actor.HasMember("ID") && actor["ID"].IsInt()) {
                    std::cout << "ID: " << actor["ID"].GetInt() << std::endl;
                }

                if (actor.HasMember("Actor") && actor["Actor"].IsString()) {
                    std::cout << "Actor: " << actor["Actor"].GetString() << std::endl;
                }
            }
        }
    }
}

void TestSerializeScene() {
    // 生成测试场景
    Scene scene;
    Actor* actor1 = scene.CreateActor("Actor1");
    auto& fooComp = actor1->AddComponent<FooComponent>();
    fooComp.d = Math::Vector3{ 3.3f, 4.4f, 5.5f };
    
    Actor* actor2 = scene.CreateActor("Actor2");
    actor2->AttachParent(*actor1);
    Actor* actor3 = scene.CreateActor("Actor3");
    actor3->AttachParent(*actor1);
    Actor* actor4 = scene.CreateActor("Actor4");
    actor4->AttachParent(*actor3);
    auto& transform = actor4->GetComponent<Transform>();
    transform.SetLocalScale(Math::Vector3{3.0f, 4.0f, 5.0f});


    // 写入
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

    scene.SerializeJson(writer);

    std::string filename = "TestScenejson.json";
    std::ofstream oStream;
    oStream.open(filename);

    oStream << buf.GetString() << std::endl;
    oStream.close();
}

void TestDeserializeScene() {
    // 测试场景
    Scene scene;

    // 读取Json
    std::ifstream iStream("TestScenejson.json");
    std::string jsonData((std::istreambuf_iterator<char>(iStream)), std::istreambuf_iterator<char>());
    iStream.close();

    rapidjson::Document doc;
    if (!doc.Parse(jsonData.c_str()).HasParseError()) {
        const rapidjson::Value& s = doc.GetObj();
        scene.DeserializeJson(s);
    }

    // 输出Json
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);

    scene.SerializeJson(writer);

    std::string filename = "TestDeserializeSceneOutput.json";
    std::ofstream oStream;
    oStream.open(filename);

    oStream << buf.GetString() << std::endl;
    oStream.close();
}

class Base {
public:
    inline static int64_t sMember;
};

class A : public Base {
public:
};

class B : public Base {
public:
};

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // TestECS();
    // TestRapidJson();
    // TestSerializeScene();
    TestDeserializeScene();

    A a;
    a.sMember = 3;

    B b;
    int64_t member = b.sMember;

    return 0;
}
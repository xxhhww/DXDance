#include "ServiceLocator.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"

#include "Scene.h"
#include "FooComponent.h"
#include "Renderer/CTransform.h"

#include <fstream>
#include <iostream>

using namespace Core;

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
    Scene scene(nullptr, 0u);
    Actor* actor1 = scene.CreateActor("Actor1");
    auto& fooComp = actor1->AddComponent<FooComponent>();
    fooComp.d = Math::Vector3{ 3.3f, 4.4f, 5.5f };
    
    Actor* actor2 = scene.CreateActor("Actor2");
    actor2->AttachParent(*actor1);
    Actor* actor3 = scene.CreateActor("Actor3");
    actor3->AttachParent(*actor1);
    Actor* actor4 = scene.CreateActor("Actor4");
    actor4->AttachParent(*actor3);
    auto& transform = actor4->GetComponent<Renderer::Transform>();


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
    Scene scene(nullptr, 0u);

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

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // TestECS();
    // TestRapidJson();
    // TestSerializeScene();
    TestDeserializeScene();

    return 0;
}
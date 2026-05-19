#pragma once

#include "Mayohoshi.h"

#include <cmath>

namespace MayohoshiExamples
{
    template<typename TRenderer>
    inline void BuildDemoScene(TRenderer& renderer)
    {
        using namespace DirectX;

        renderer.SetAmbientLight({ 0.2f, 0.2f, 0.2f });
        renderer.AddDirectionalLight({ -0.57735f, -0.57735f, 0.57735f });
        auto orbitLight = renderer.AddPointLight({ 0.0f, 2.0f, 6.0f }, { 2.0f, 2.0f, 2.0f }, 15.0f);
        renderer.SetUpdateCallback(
            [&renderer, orbitLight](float totalTime, float)
            {
                const float angle = totalTime * Mayohoshi::Pi / 16.0f;
                renderer.SetLightPosition(
                    orbitLight,
                    {
                        6.0f * static_cast<float>(std::sin(angle)),
                        2.0f,
                        6.0f * static_cast<float>(std::cos(angle))
                    });
            });

        auto screen = renderer.CreateCanvas("Screen");

        auto stone = renderer.CreateTexture("stone", L"../Resources/Textures/stone.dds");
        auto brick = renderer.CreateTexture("brick", L"../Resources/Textures/bricks.dds");
        auto wave = renderer.CreateTexture("wave", L"../Resources/Textures/water.dds");
        auto floor = renderer.CreateTexture("floor", L"../Resources/Textures/floor.dds");
        auto wood = renderer.CreateTexture("wood", L"../Resources/Textures/wood.dds");
        auto defaultNormal = renderer.CreateTexture("defaultNormal", L"../Resources/Textures/default_normal.dds");
        auto brickNormal = renderer.CreateTexture("brickNormal", L"../Resources/Textures/brick_normal.dds");
        auto waveNormal = renderer.CreateTexture("waveNormal", L"../Resources/Textures/wave.dds");
        auto floorNormal = renderer.CreateTexture("floorNormal", L"../Resources/Textures/floor_normal.dds");
        auto defaultDepth = renderer.CreateTexture("defaultDepth", L"../Resources/Textures/default_depth.dds");
        auto brickDepth = renderer.CreateTexture("brickDepth", L"../Resources/Textures/brick_depth.dds");
        auto floorDepth = renderer.CreateTexture("floorDepth", L"../Resources/Textures/floor_depth.dds");
        auto skycube = renderer.CreateTexture("skycube", L"../Resources/Textures/snow_skycube.dds");

        (void)defaultNormal;
        (void)waveNormal;
        (void)defaultDepth;

        auto brickSurface = renderer.BindTextures("brickSurface", brick, brickNormal, brickDepth);
        auto stoneSurface = renderer.BindTextures("stoneSurface", stone);
        auto floorSurface = renderer.BindTextures("floorSurface", floor, floorNormal, floorDepth);
        auto woodSurface = renderer.BindTextures("woodSurface", wood);
        auto waveSurface = renderer.BindTextures("waveSurface", wave);
        auto skySurface = renderer.BindTextures("skySurface", skycube);

        auto brickMat = renderer.CreateMaterial("Brick", { 0.5f,0.5f,0.5f,1.0f }, 0.55f);
        auto stoneMat = renderer.CreateMaterial("Stone", { 0.9f,0.9f,0.9f,1.0f }, 0.15f);
        auto woodMat = renderer.CreateMaterial("Wood", { 0.8f,0.8f,0.8f,1.0f }, 0.4f);
        auto waterMat = renderer.CreateMaterial({ "Water", { 1.0f,1.0f,1.0f,0.6f }, 0.0f, 0.02f, 1.33f, { 0.0f,0.0f,0.0f }, Mayohoshi::IdentityMatrix(), Mayohoshi::RenderLayers::Transparent });
        auto skyMat = renderer.CreateMaterial({ "Skycube", { 1.0f,1.0f,1.0f,1.0f }, 0.0f, 1.0f, 1.0f, { 0.0f,0.0f,0.0f }, Mayohoshi::IdentityMatrix(), Mayohoshi::RenderLayers::Skybox });

        auto cylinder = renderer.CreateCylinder("Cylinder", 1.0f, 0.56f, 4.0f, 100, 20);
        auto sphere = renderer.CreateSphere("Sphere", 1.0f, 50, 50);
        auto floorGrid = renderer.CreateGrid("Floor", 10.0f, 10.0f, 100, 100);
        auto nailong = renderer.CreateModel("Nailong", L"../Resources/Models/Nailong.obj");
        auto skySphere = renderer.CreateSphere("SkySphere", 1.0f, 50, 50, Mayohoshi::RenderLayers::Skybox);
        auto waveGrid = renderer.CreateGrid("Wave", 10.0f, 10.0f, 100, 100, Mayohoshi::RenderLayers::Transparent);

        const Mayohoshi::TextureFlags diffuse = Mayohoshi::ToTextureFlags(Mayohoshi::TextureType::TEX_DIFFUSE);
        const Mayohoshi::TextureFlags pbrSurface = Mayohoshi::ToTextureFlags(
            Mayohoshi::TextureType::TEX_DIFFUSE |
            Mayohoshi::TextureType::TEX_NORMAL |
            Mayohoshi::TextureType::TEX_DEPTH);

        renderer.AddInstances({
            "Cylinder",
            cylinder,
            {
                { Mayohoshi::MakeTransform(XMMatrixTranslation(-2.5f, 0.0f, 0.0f)), brickMat, brickSurface, pbrSurface },
                { Mayohoshi::MakeTransform(XMMatrixTranslation(2.5f, 0.0f, 0.0f)), brickMat, brickSurface, pbrSurface },
            } });

        renderer.AddInstances({
            "Sphere",
            sphere,
            {
                { Mayohoshi::MakeTransform(XMMatrixTranslation(2.5f, 2.96f, 0.0f)), stoneMat, stoneSurface, diffuse },
                { Mayohoshi::MakeTransform(XMMatrixTranslation(-2.5f, 2.96f, 0.0f)), stoneMat, stoneSurface, diffuse },
            } });

        renderer.AddInstance(
            "Floor",
            floorGrid,
            stoneMat,
            floorSurface,
            Mayohoshi::MakeTransform(
                XMMatrixScaling(1.5f, 1.5f, 1.5f) * XMMatrixTranslation(0.0f, -2.0f, 0.0f),
                XMMatrixScaling(5.0f, 5.0f, 5.0f)),
            pbrSurface);

        renderer.AddInstance(
            "Nailong",
            nailong,
            woodMat,
            woodSurface,
            Mayohoshi::MakeTransform(
                XMMatrixScaling(0.2f, 0.2f, 0.2f) *
                XMMatrixRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), Mayohoshi::Pi) *
                XMMatrixTranslation(0.0f, -1.0f, 0.0f)),
            diffuse);

        renderer.AddInstance("Skybox", skySphere, skyMat, skySurface, Mayohoshi::MakeTransform(XMMatrixScaling(0.5f, 0.5f, 0.5f)), diffuse);
        renderer.AddInstance("Wave", waveGrid, waterMat, waveSurface, Mayohoshi::MakeTransform(XMMatrixIdentity()), diffuse);

        renderer.DrawTo(screen);
        renderer.DrawToScreen(screen);
    }
}

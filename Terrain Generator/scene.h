#pragma once
#include "framework.h"
#include "camera.h"
#include "material.h"
#include "renderstate.h"
#include "light.h"
#include "material.h"
#include "object.h"
#include "geometry.h"
#include "terrainshader.h"
#include <iostream>

const int gui_width = 320;
const int gui_height = 420;

int fps;
int frameCount = 0;
float previousTime = glfwGetTime();

class Scene {
    Camera camera;
    RenderState state;
    std::vector<Object*> objects;
    std::vector<Light> lights;
    Geometry* terrainGeometry = nullptr;

    void getFPS(int& fps) {
        float currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - previousTime >= 1.0) {
            fps = frameCount;
            frameCount = 0;
            previousTime = currentTime;
        }
    }

    void updateState(RenderState& state) {
        state.lightCount = static_cast<int>(std::min<size_t>(lights.size(), state.lights.size()));
        for (int i = 0; i < state.lightCount; ++i) {
            state.lights[i] = lights[i];
        }
        state.time = getTime();
        state.wEye = camera.getEyePos();
        state.M = mat4(1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
        state.V = camera.V();
        state.P = camera.P();
    }

public:
    void Render() {
        updateState(state);
        for (Object* obj : objects) { obj->Draw(state); }
        drawGUI(windowWidth - gui_width, 0, gui_width, gui_height);
    }

    void Build() {
        camera.setEyePos(vec3(0, 20, 0));
        camera.setEyeDir(vec3(-1, -0.2f, 0));

        Shader* terrainShader = new TerrainShader();
        Material* terrainMaterial = new Material(vec3(0.5f, 0.5f, 0.5f), vec3(0.4f, 0.4f, 0.4f), vec3(0.4f, 0.4f, 0.4f), 1.0f);
        terrainGeometry = new Geometry();

        Object* terrainObject = new Object(terrainShader, terrainMaterial, terrainGeometry);
        terrainObject->pos = vec3(0, 0, 0);
        objects.push_back(terrainObject);

        lights.resize(1);
        lights[0].wLightPos = vec4(0, 50, 0, 1);
        lights[0].Le = vec3(1.0, 1.0, 1.0);
        lights[0].La = vec3(0.5, 0.5, 0.5);
    }

    void drawGUI(int x, int y, int w, int h) {
        ImGui::SetNextWindowPos(ImVec2((float)x, (float)y));
        ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
        ImGui::Begin("Settings");

        getFPS(fps);
        ImGui::Text("FPS: %d", fps);

        if (terrainGeometry) {
            Geometry::TerrainSettings uiSettings = terrainGeometry->getSettings();
            bool changed = false;

            changed |= ImGui::SliderFloat("Base ground", &uiSettings.baseGroundHeight, 4.0f, 64.0f, "%.1f");
            changed |= ImGui::SliderFloat("Biome frequency", &uiSettings.biomeFrequency, 0.0005f, 0.02f, "%.4f");
            changed |= ImGui::SliderFloat("Flat noise frequency", &uiSettings.flatFrequency, 0.001f, 0.06f, "%.3f");
            changed |= ImGui::SliderFloat("Hill noise frequency", &uiSettings.hillFrequency, 0.001f, 0.06f, "%.3f");
            changed |= ImGui::SliderFloat("Cave frequency", &uiSettings.caveFrequency, 0.01f, 0.12f, "%.3f");
            changed |= ImGui::SliderInt("Noise octaves", &uiSettings.noiseOctaves, 1, 8);
            changed |= ImGui::SliderFloat("Noise lacunarity", &uiSettings.noiseLacunarity, 1.1f, 4.0f, "%.2f");
            changed |= ImGui::SliderFloat("Noise gain", &uiSettings.noiseGain, 0.1f, 1.0f, "%.2f");
            changed |= ImGui::SliderFloat("Flat amplitude", &uiSettings.flatSpotAmplitude, 0.0f, 8.0f, "%.1f");
            changed |= ImGui::SliderFloat("Hill base amp", &uiSettings.hillBaseAmplitude, 0.0f, 20.0f, "%.1f");
            changed |= ImGui::SliderFloat("Hill extra amp", &uiSettings.hillExtraAmplitude, 0.0f, 60.0f, "%.1f");
            changed |= ImGui::SliderFloat("Cave threshold", &uiSettings.caveThreshold, 0.3f, 0.9f, "%.2f");
            changed |= ImGui::SliderFloat("Cave depth boost", &uiSettings.caveDepthBoost, 0.0f, 0.4f, "%.2f");
            changed |= ImGui::SliderFloat("Cave depth range", &uiSettings.caveDepthRange, 8.0f, 128.0f, "%.1f");
            changed |= ImGui::SliderFloat("Cave strength", &uiSettings.caveStrength, 1.0f, 64.0f, "%.1f");
            changed |= ImGui::SliderFloat("Cave surf fade start", &uiSettings.caveSurfaceFadeStart, 0.0f, 24.0f, "%.1f");
            changed |= ImGui::SliderFloat("Cave surf fade end", &uiSettings.caveSurfaceFadeEnd, 1.0f, 40.0f, "%.1f");
            changed |= ImGui::SliderFloat("Iso level", &uiSettings.isolevel, -4.0f, 4.0f, "%.2f");
            changed |= ImGui::SliderFloat("Base step size", &uiSettings.baseStepSize, 0.25f, 2.0f, "%.2f");
            changed |= ImGui::SliderInt("Chunk size", &uiSettings.chunkSize, 8, 64);
            changed |= ImGui::SliderInt("Chunk height", &uiSettings.chunkHeight, 32, 160);
            changed |= ImGui::SliderInt("Render distance", &uiSettings.renderDistance, 1, 8);
            changed |= ImGui::SliderInt("LOD levels", &uiSettings.lodLevels, 1, 5);

            if (changed) {
                if (uiSettings.lodLevels > uiSettings.renderDistance + 1) {
                    uiSettings.lodLevels = uiSettings.renderDistance + 1;
                }
                if (uiSettings.caveSurfaceFadeEnd < uiSettings.caveSurfaceFadeStart + 0.5f) {
                    uiSettings.caveSurfaceFadeEnd = uiSettings.caveSurfaceFadeStart + 0.5f;
                }
                terrainGeometry->setSettings(uiSettings);
            }
        }

        ImGui::End();
    }

    void moveCamera(int key) {
        camera.move(key);
    }

    void rotateCamera(float x, float y) {
        camera.rotate((int)x, (int)y);
    }

    void setCameraFirstMouse() {
        camera.setFirstMouse();
    }
};

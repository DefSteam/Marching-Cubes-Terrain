#pragma once
#include "framework.h"
#include "lut.h"
#include "FastNoiseLite.h"
#include "renderstate.h"

#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>

class Geometry {
protected:
    struct VertexData {
        vec3 pos;
        vec3 normal;
    };

    struct Triangle {
        vec3 p[3];
    };

    struct GridCell {
        vec3 p[8];
        float val[8];
    };

public:
    struct TerrainSettings {
        float noiseFrequency = 0.05f;
        int noiseOctaves = 5;
        float noiseLacunarity = 2.0f;
        float noiseGain = 0.5f;
        float isolevel = 0.4f;
        float baseStepSize = 0.5f;
        int chunkSize = 24;
        int chunkHeight = 80;
        int renderDistance = 3;
        int lodLevels = 3;
    };

protected:
    struct ChunkKey {
        int x = 0;
        int z = 0;
        int lod = 0;

        bool operator==(const ChunkKey& other) const {
            return x == other.x && z == other.z && lod == other.lod;
        }
    };

    struct ChunkKeyHash {
        std::size_t operator()(const ChunkKey& key) const {
            std::size_t h1 = std::hash<int>{}(key.x);
            std::size_t h2 = std::hash<int>{}(key.z);
            std::size_t h3 = std::hash<int>{}(key.lod);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    struct ChunkRequest {
        ChunkKey key;
        TerrainSettings settings;
        std::uint64_t settingsVersion = 0;
    };

    struct BuiltChunk {
        ChunkKey key;
        std::vector<VertexData> vertices;
        std::uint64_t settingsVersion = 0;
    };

    struct GpuChunk {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLsizei vertexCount = 0;
    };

    TerrainSettings settings;
    std::uint64_t settingsVersion = 1;

    std::unordered_map<ChunkKey, GpuChunk, ChunkKeyHash> activeChunks;
    std::unordered_set<ChunkKey, ChunkKeyHash> pendingChunks;

    std::queue<ChunkRequest> buildQueue;
    std::queue<BuiltChunk> completedQueue;

    std::mutex taskMutex;
    std::condition_variable taskCv;
    std::thread worker;
    bool workerRunning = false;

public:
    Geometry() {
        workerRunning = true;
        worker = std::thread(&Geometry::workerLoop, this);
    }

    virtual ~Geometry() {
        {
            std::lock_guard<std::mutex> lock(taskMutex);
            workerRunning = false;
        }
        taskCv.notify_all();
        if (worker.joinable()) worker.join();

        clearGpuChunks();
    }

    const TerrainSettings& getSettings() const {
        return settings;
    }

    void setSettings(const TerrainSettings& newSettings) {
        bool changed =
            settings.noiseFrequency != newSettings.noiseFrequency ||
            settings.noiseOctaves != newSettings.noiseOctaves ||
            settings.noiseLacunarity != newSettings.noiseLacunarity ||
            settings.noiseGain != newSettings.noiseGain ||
            settings.isolevel != newSettings.isolevel ||
            settings.baseStepSize != newSettings.baseStepSize ||
            settings.chunkSize != newSettings.chunkSize ||
            settings.chunkHeight != newSettings.chunkHeight ||
            settings.renderDistance != newSettings.renderDistance ||
            settings.lodLevels != newSettings.lodLevels;

        if (!changed) return;

        settings = newSettings;
        settingsVersion++;

        {
            std::lock_guard<std::mutex> lock(taskMutex);
            std::queue<ChunkRequest> emptyBuild;
            std::queue<BuiltChunk> emptyDone;
            std::swap(buildQueue, emptyBuild);
            std::swap(completedQueue, emptyDone);
            pendingChunks.clear();
        }

        clearGpuChunks();
    }

    void Draw(const RenderState& state) {
        uploadCompletedChunks();
        updateVisibleChunks(state.wEye);

        for (const auto& it : activeChunks) {
            const GpuChunk& chunk = it.second;
            if (chunk.vertexCount == 0) continue;
            glBindVertexArray(chunk.vao);
            glDrawArrays(GL_TRIANGLES, 0, chunk.vertexCount);
        }
    }

private:
    static int floorDiv(float value, float divisor) {
        return static_cast<int>(std::floor(value / divisor));
    }

    float sampleNoise(FastNoiseLite& localNoise, float x, float y, float z) const {
        return (localNoise.GetNoise(x, y, z) + 1.0f) * 0.5f;
    }

    int Polygonise(const GridCell& grid, float iso, Triangle* triangles) const {
        int cubeindex = 0;
        vec3 vertlist[12];

        if (grid.val[0] < iso) cubeindex |= 1;
        if (grid.val[1] < iso) cubeindex |= 2;
        if (grid.val[2] < iso) cubeindex |= 4;
        if (grid.val[3] < iso) cubeindex |= 8;
        if (grid.val[4] < iso) cubeindex |= 16;
        if (grid.val[5] < iso) cubeindex |= 32;
        if (grid.val[6] < iso) cubeindex |= 64;
        if (grid.val[7] < iso) cubeindex |= 128;

        if (edgeTable[cubeindex] == 0) return 0;

        if (edgeTable[cubeindex] & 1) vertlist[0] = lerp(iso, grid.p[0], grid.p[1], grid.val[0], grid.val[1]);
        if (edgeTable[cubeindex] & 2) vertlist[1] = lerp(iso, grid.p[1], grid.p[2], grid.val[1], grid.val[2]);
        if (edgeTable[cubeindex] & 4) vertlist[2] = lerp(iso, grid.p[2], grid.p[3], grid.val[2], grid.val[3]);
        if (edgeTable[cubeindex] & 8) vertlist[3] = lerp(iso, grid.p[3], grid.p[0], grid.val[3], grid.val[0]);
        if (edgeTable[cubeindex] & 16) vertlist[4] = lerp(iso, grid.p[4], grid.p[5], grid.val[4], grid.val[5]);
        if (edgeTable[cubeindex] & 32) vertlist[5] = lerp(iso, grid.p[5], grid.p[6], grid.val[5], grid.val[6]);
        if (edgeTable[cubeindex] & 64) vertlist[6] = lerp(iso, grid.p[6], grid.p[7], grid.val[6], grid.val[7]);
        if (edgeTable[cubeindex] & 128) vertlist[7] = lerp(iso, grid.p[7], grid.p[4], grid.val[7], grid.val[4]);
        if (edgeTable[cubeindex] & 256) vertlist[8] = lerp(iso, grid.p[0], grid.p[4], grid.val[0], grid.val[4]);
        if (edgeTable[cubeindex] & 512) vertlist[9] = lerp(iso, grid.p[1], grid.p[5], grid.val[1], grid.val[5]);
        if (edgeTable[cubeindex] & 1024) vertlist[10] = lerp(iso, grid.p[2], grid.p[6], grid.val[2], grid.val[6]);
        if (edgeTable[cubeindex] & 2048) vertlist[11] = lerp(iso, grid.p[3], grid.p[7], grid.val[3], grid.val[7]);

        int nTriangle = 0;
        for (int i = 0; triTable[cubeindex][i] != -1; i += 3) {
            triangles[nTriangle].p[0] = vertlist[triTable[cubeindex][i]];
            triangles[nTriangle].p[1] = vertlist[triTable[cubeindex][i + 1]];
            triangles[nTriangle].p[2] = vertlist[triTable[cubeindex][i + 2]];
            nTriangle++;
        }

        return nTriangle;
    }

    vec3 lerp(float iso, const vec3& p1, const vec3& p2, float valp1, float valp2) const {
        if (abs(valp1 - valp2) < 0.00001f) return p1;
        if (abs(iso - valp1) < 0.00001f) return p1;
        if (abs(iso - valp2) < 0.00001f) return p2;

        float mu = (iso - valp1) / (valp2 - valp1);
        return { p1.x + mu * (p2.x - p1.x), p1.y + mu * (p2.y - p1.y), p1.z + mu * (p2.z - p1.z) };
    }

    std::vector<VertexData> buildChunkMesh(const ChunkRequest& request) const {
        std::vector<VertexData> vtxData;
        vtxData.reserve(20000);

        const int lodScale = 1 << request.key.lod;
        const float step = request.settings.baseStepSize * static_cast<float>(lodScale);
        const int cellsX = std::max(1, request.settings.chunkSize / lodScale);
        const int cellsZ = std::max(1, request.settings.chunkSize / lodScale);
        const int cellsY = std::max(1, request.settings.chunkHeight / lodScale);

        const float worldChunkSize = request.settings.baseStepSize * static_cast<float>(request.settings.chunkSize);
        const float originX = static_cast<float>(request.key.x) * worldChunkSize;
        const float originZ = static_cast<float>(request.key.z) * worldChunkSize;

        FastNoiseLite localNoise;
        localNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        localNoise.SetFrequency(request.settings.noiseFrequency);
        localNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        localNoise.SetFractalOctaves(request.settings.noiseOctaves);
        localNoise.SetFractalLacunarity(request.settings.noiseLacunarity);
        localNoise.SetFractalGain(request.settings.noiseGain);

        for (int x = 0; x < cellsX; ++x) {
            for (int y = 0; y < cellsY; ++y) {
                for (int z = 0; z < cellsZ; ++z) {
                    GridCell grid;
                    Triangle triangles[5];

                    const float fx = originX + static_cast<float>(x) * step;
                    const float fy = static_cast<float>(y) * step;
                    const float fz = originZ + static_cast<float>(z) * step;

                    grid.p[0] = vec3(fx, fy, fz);
                    grid.p[1] = vec3(fx + step, fy, fz);
                    grid.p[2] = vec3(fx + step, fy, fz + step);
                    grid.p[3] = vec3(fx, fy, fz + step);
                    grid.p[4] = vec3(fx, fy + step, fz);
                    grid.p[5] = vec3(fx + step, fy + step, fz);
                    grid.p[6] = vec3(fx + step, fy + step, fz + step);
                    grid.p[7] = vec3(fx, fy + step, fz + step);

                    for (int i = 0; i < 8; i++) {
                        grid.val[i] = sampleNoise(localNoise, grid.p[i].x, grid.p[i].y, grid.p[i].z);
                    }

                    int numTriangles = Polygonise(grid, request.settings.isolevel, triangles);
                    for (int tri = 0; tri < numTriangles; tri++) {
                        vec3 n = normalize(cross(triangles[tri].p[1] - triangles[tri].p[0], triangles[tri].p[2] - triangles[tri].p[0]));
                        for (int vert = 0; vert < 3; vert++) {
                            vtxData.push_back({ triangles[tri].p[vert], n });
                        }
                    }
                }
            }
        }

        return vtxData;
    }

    void workerLoop() {
        while (true) {
            ChunkRequest request;
            {
                std::unique_lock<std::mutex> lock(taskMutex);
                taskCv.wait(lock, [this] { return !workerRunning || !buildQueue.empty(); });
                if (!workerRunning && buildQueue.empty()) return;
                request = buildQueue.front();
                buildQueue.pop();
            }

            BuiltChunk result;
            result.key = request.key;
            result.settingsVersion = request.settingsVersion;
            result.vertices = buildChunkMesh(request);

            std::lock_guard<std::mutex> lock(taskMutex);
            completedQueue.push(std::move(result));
        }
    }

    void uploadCompletedChunks() {
        constexpr int uploadsPerFrame = 2;
        int uploads = 0;

        while (uploads < uploadsPerFrame) {
            BuiltChunk built;
            bool hasChunk = false;

            {
                std::lock_guard<std::mutex> lock(taskMutex);
                if (!completedQueue.empty()) {
                    built = std::move(completedQueue.front());
                    completedQueue.pop();
                    pendingChunks.erase(built.key);
                    hasChunk = true;
                }
            }

            if (!hasChunk) break;
            if (built.settingsVersion != settingsVersion) continue;

            auto it = activeChunks.find(built.key);
            if (it != activeChunks.end()) {
                glDeleteBuffers(1, &it->second.vbo);
                glDeleteVertexArrays(1, &it->second.vao);
                activeChunks.erase(it);
            }

            GpuChunk chunk;
            glGenVertexArrays(1, &chunk.vao);
            glGenBuffers(1, &chunk.vbo);
            glBindVertexArray(chunk.vao);
            glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
            glBufferData(GL_ARRAY_BUFFER, built.vertices.size() * sizeof(VertexData), built.vertices.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, pos));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
            chunk.vertexCount = static_cast<GLsizei>(built.vertices.size());
            activeChunks[built.key] = chunk;

            ++uploads;
        }
    }

    void queueChunk(const ChunkKey& key) {
        std::lock_guard<std::mutex> lock(taskMutex);
        if (pendingChunks.find(key) != pendingChunks.end()) return;
        if (activeChunks.find(key) != activeChunks.end()) return;

        pendingChunks.insert(key);
        buildQueue.push({ key, settings, settingsVersion });
        taskCv.notify_one();
    }

    void updateVisibleChunks(const vec3& cameraPos) {
        const float worldChunkSize = settings.baseStepSize * static_cast<float>(settings.chunkSize);
        const int centerX = floorDiv(cameraPos.x, worldChunkSize);
        const int centerZ = floorDiv(cameraPos.z, worldChunkSize);

        std::unordered_set<ChunkKey, ChunkKeyHash> required;
        const int lodBand = std::max(1, settings.renderDistance / std::max(1, settings.lodLevels));

        for (int dz = -settings.renderDistance; dz <= settings.renderDistance; ++dz) {
            for (int dx = -settings.renderDistance; dx <= settings.renderDistance; ++dx) {
                int dist = std::max(abs(dx), abs(dz));
                int lod = std::min(settings.lodLevels - 1, dist / lodBand);
                ChunkKey key{ centerX + dx, centerZ + dz, lod };
                required.insert(key);
                queueChunk(key);
            }
        }

        std::vector<ChunkKey> toDelete;
        for (const auto& it : activeChunks) {
            if (required.find(it.first) == required.end()) {
                toDelete.push_back(it.first);
            }
        }

        for (const ChunkKey& key : toDelete) {
            auto it = activeChunks.find(key);
            if (it != activeChunks.end()) {
                glDeleteBuffers(1, &it->second.vbo);
                glDeleteVertexArrays(1, &it->second.vao);
                activeChunks.erase(it);
            }
        }
    }

    void clearGpuChunks() {
        for (auto& it : activeChunks) {
            glDeleteBuffers(1, &it.second.vbo);
            glDeleteVertexArrays(1, &it.second.vao);
        }
        activeChunks.clear();
    }
};

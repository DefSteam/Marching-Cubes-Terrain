#pragma once
#include "framework.h"
#include "lut.h"
#include "FastNoiseLite.h"


class Geometry {
protected:
	unsigned int vao, vbo;
	unsigned int totalVertices;
    FastNoiseLite noise;

	struct VertexData {
		vec3 pos;
        vec3 normal;
	};

    struct Triangle {
        vec3 p[3];
    };

    typedef struct GridCell {
        vec3 p[8];
        float val[8];
    };

    float boundary = 50;
    float isolevel = 0.4f;
    float stepSize = 0.5f;

public:
	Geometry() {
        noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        noise.SetFrequency(0.05f);

        noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        noise.SetFractalOctaves(5);
        noise.SetFractalLacunarity(2.0f);
        noise.SetFractalGain(0.5f);

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		create();
	}

    virtual ~Geometry() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    float f(float x, float y, float z) {
        return (noise.GetNoise(x, y, z) + 1.0f) * 0.5f;
    }

    int Polygonise(const GridCell& grid, float iso, Triangle* triangles)
    {
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

        if (edgeTable[cubeindex] & 1)       vertlist[0] = lerp(iso, grid.p[0], grid.p[1], grid.val[0], grid.val[1]);
        if (edgeTable[cubeindex] & 2)       vertlist[1] = lerp(iso, grid.p[1], grid.p[2], grid.val[1], grid.val[2]);
        if (edgeTable[cubeindex] & 4)       vertlist[2] = lerp(iso, grid.p[2], grid.p[3], grid.val[2], grid.val[3]);
        if (edgeTable[cubeindex] & 8)       vertlist[3] = lerp(iso, grid.p[3], grid.p[0], grid.val[3], grid.val[0]);
        if (edgeTable[cubeindex] & 16)      vertlist[4] = lerp(iso, grid.p[4], grid.p[5], grid.val[4], grid.val[5]);
        if (edgeTable[cubeindex] & 32)      vertlist[5] = lerp(iso, grid.p[5], grid.p[6], grid.val[5], grid.val[6]);
        if (edgeTable[cubeindex] & 64)      vertlist[6] = lerp(iso, grid.p[6], grid.p[7], grid.val[6], grid.val[7]);
        if (edgeTable[cubeindex] & 128)     vertlist[7] = lerp(iso, grid.p[7], grid.p[4], grid.val[7], grid.val[4]);
        if (edgeTable[cubeindex] & 256)     vertlist[8] = lerp(iso, grid.p[0], grid.p[4], grid.val[0], grid.val[4]);
        if (edgeTable[cubeindex] & 512)     vertlist[9] = lerp(iso, grid.p[1], grid.p[5], grid.val[1], grid.val[5]);
        if (edgeTable[cubeindex] & 1024)    vertlist[10] = lerp(iso, grid.p[2], grid.p[6], grid.val[2], grid.val[6]);
        if (edgeTable[cubeindex] & 2048)    vertlist[11] = lerp(iso, grid.p[3], grid.p[7], grid.val[3], grid.val[7]);

        int nTriangle = 0;
        for (int i = 0; triTable[cubeindex][i] != -1; i += 3) {
            triangles[nTriangle].p[0] = vertlist[triTable[cubeindex][i]];
            triangles[nTriangle].p[1] = vertlist[triTable[cubeindex][i + 1]];
            triangles[nTriangle].p[2] = vertlist[triTable[cubeindex][i + 2]];
            nTriangle++;
        }

        return nTriangle;
    }


    vec3 lerp(float iso, const vec3& p1, const vec3& p2, float valp1, float valp2) {
        if (abs(valp1 - valp2) < 0.00001f)       return p1;
        if (abs(iso - valp1) < 0.00001f)    return p1;
        if (abs(iso - valp2) < 0.00001f)    return p2;

        float mu = (iso - valp1) / (valp2 - valp1);
        return { p1.x + mu * (p2.x - p1.x), p1.y + mu * (p2.y - p1.y), p1.z + mu * (p2.z - p1.z) };
    }

    void create() {
        std::vector<VertexData> vtxData;
        vtxData.reserve(2'000'000);
        totalVertices = 0;

        for (float x = 0; x < boundary; x += stepSize) {
            for (float y = 0; y < boundary; y += stepSize) {
                for (float z = 0; z < boundary; z += stepSize) {
                    GridCell grid;
                    Triangle triangles[5];

                    grid.p[0] = vec3(x,             y,              z);
                    grid.p[1] = vec3(x + stepSize,  y,              z);
                    grid.p[2] = vec3(x + stepSize,  y,              z + stepSize);
                    grid.p[3] = vec3(x,             y,              z + stepSize);
                    grid.p[4] = vec3(x,             y + stepSize,   z);
                    grid.p[5] = vec3(x + stepSize,  y + stepSize,   z);
                    grid.p[6] = vec3(x + stepSize,  y + stepSize,   z + stepSize);
                    grid.p[7] = vec3(x,             y + stepSize,   z + stepSize);

                    for (int i = 0; i < 8; i++) {
                        grid.val[i] = f(grid.p[i].x, grid.p[i].y, grid.p[i].z);
                    }

                    int numTriangles = Polygonise(grid, isolevel, triangles);

                    for (int tri = 0; tri < numTriangles; tri++) {
                        vec3 n = normalize(cross(triangles[tri].p[1] - triangles[tri].p[0], triangles[tri].p[2] - triangles[tri].p[0]));
                        for (int vert = 0; vert < 3; vert++) {
                            vtxData.push_back({ triangles[tri].p[vert], n });
                            totalVertices++;
                        }
                    }
                }
            }
        }

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vtxData.size() * sizeof(VertexData), vtxData.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
    }


	void Draw() {
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, totalVertices);
	}
};

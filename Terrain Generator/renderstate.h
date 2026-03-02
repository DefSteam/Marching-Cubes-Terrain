#pragma once
#include "framework.h"
#include "material.h"
#include "light.h"
#include <array>

struct RenderState {
	float time;
	vec3 wEye;
	mat4 MVP, M, V, P;
	Material* material;
	std::array<Light, 8> lights;
	int lightCount = 0;
};

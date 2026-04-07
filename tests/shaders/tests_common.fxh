// tests_common.fxh
// Common utilities for blend/feature test effects

#pragma once

#include "ReShade.fxh"

static const float BORDER_WIDTH = 0.005;

static const float3 COLOR_RED     = float3(1.0, 0.0, 0.0);
static const float3 COLOR_GREEN   = float3(0.0, 1.0, 0.0);
static const float3 COLOR_BLUE    = float3(0.0, 0.0, 1.0);
static const float3 COLOR_CYAN    = float3(0.0, 1.0, 1.0);
static const float3 COLOR_MAGENTA = float3(1.0, 0.0, 1.0);
static const float3 COLOR_YELLOW  = float3(1.0, 1.0, 0.0);
static const float3 COLOR_WHITE   = float3(1.0, 1.0, 1.0);
static const float3 COLOR_BLACK   = float3(0.0, 0.0, 0.0);

static const float3 COLOR_BORDER     = float3(0.1, 0.1, 0.1);
static const float3 COLOR_BACKGROUND = float3(0.05, 0.05, 0.05);

static const float EPSILON = 0.02;

// Returns local UV within a grid cell, or false if UV is outside it
bool cell_uv(float2 uv, uint col, uint row, uint cols, uint rows, out float2 local_uv)
{
    float2 size = float2(1.0 / cols, 1.0 / rows);
    float2 origin = float2(col * size.x, row * size.y);
    local_uv = (uv - origin) / size;
    return all(uv >= origin) && all(uv < origin + size);
}

bool is_border(float2 local_uv)
{
    return local_uv.x < BORDER_WIDTH || local_uv.x > (1.0 - BORDER_WIDTH) ||
           local_uv.y < BORDER_WIDTH || local_uv.y > (1.0 - BORDER_WIDTH);
}

// Returns pass/fail color for a cell, handling border.
float3 cell_result(float2 local_uv, bool passed)
{
    if (is_border(local_uv)) return COLOR_BORDER;
    return passed ? COLOR_GREEN : COLOR_RED;
}

// Comparison functions
bool approx_equal3(float3 a, float3 b)
{
    return all(abs(a - b) < EPSILON);
}

bool approx_equal4(float4 a, float4 b)
{
    return all(abs(a - b) < EPSILON);
}

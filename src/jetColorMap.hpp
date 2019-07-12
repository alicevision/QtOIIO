#pragma once
#include <cmath>

static float jetr[64] = {0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
                         0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
                         0,      0,      0.0625, 0.1250, 0.1875, 0.2500, 0.3125, 0.3750, 0.4375, 0.5000, 0.5625,
                         0.6250, 0.6875, 0.7500, 0.8125, 0.8750, 0.9375, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 0.9375, 0.8750, 0.8125, 0.7500, 0.6875, 0.6250, 0.5625, 0.5000};

static float jetg[64] = {0,      0,      0,      0,      0,      0,      0,      0,      0.0625, 0.1250, 0.1875,
                         0.2500, 0.3125, 0.3750, 0.4375, 0.5000, 0.5625, 0.6250, 0.6875, 0.7500, 0.8125, 0.8750,
                         0.9375, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 0.9375, 0.8750, 0.8125, 0.7500,
                         0.6875, 0.6250, 0.5625, 0.5000, 0.4375, 0.3750, 0.3125, 0.2500, 0.1875, 0.1250, 0.0625,
                         0,      0,      0,      0,      0,      0,      0,      0,      0};

static float jetb[64] = {0.5625, 0.6250, 0.6875, 0.7500, 0.8125, 0.8750, 0.9375, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
                         1.0000, 1.0000, 0.9375, 0.8750, 0.8125, 0.7500, 0.6875, 0.6250, 0.5625, 0.5000, 0.4375,
                         0.3750, 0.3125, 0.2500, 0.1875, 0.1250, 0.0625, 0,      0,      0,      0,      0,
                         0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
                         0,      0,      0,      0,      0,      0,      0,      0,      0};

struct Color32f
{
    Color32f() {}
    Color32f(float r_, float g_, float b_)
      : r(r_)
      , g(g_)
      , b(b_)
    {}
    union {
        struct
        {
            float r, g, b;
        };
        float m[3];
    };
};

inline Color32f getColor32fFromJetColorMap(float value)
{
    if(std::isnan(value))
        return Color32f(1.0f, 0.0f, 1.0f);
    if(value <= 0.0f)
        return Color32f(0, 0, 0);
    if(value >= 1.0f)
        return Color32f(1.0f, 1.0f, 1.0f);
    float idx_f = value * 63.0f;
    float fractA, fractB, integral;
    fractB = std::modf(idx_f, &integral);
    fractA = 1.0f - fractB;
    int idx = static_cast<int>(integral);
    Color32f c;
    c.r = jetr[idx] * fractA + jetr[idx + 1] * fractB;
    c.g = jetg[idx] * fractA + jetg[idx + 1] * fractB;
    c.b = jetb[idx] * fractA + jetb[idx + 1] * fractB;
    return c;
}

inline Color32f getColor32fFromJetColorMapClamp(float value)
{
    if(std::isnan(value))
        return Color32f(1.0f, 0.0f, 1.0f);
    if(value < 0.0f)
        value = 0.0f;
    if(value > 1.0f)
        value = 1.0f;
    float idx_f = value * 63.0f;
    float fractA, fractB, integral;
    fractB = std::modf(idx_f, &integral);
    fractA = 1.0f - fractB;
    int idx = static_cast<int>(integral);
    Color32f c;
    c.r = jetr[idx] * fractA + jetr[idx + 1] * fractB;
    c.g = jetg[idx] * fractA + jetg[idx + 1] * fractB;
    c.b = jetb[idx] * fractA + jetb[idx + 1] * fractB;
    return c;
}

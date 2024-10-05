/*
*
*	Copyright (c) 2024, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
*/
#ifndef QFX_GLDITHER_H
#define QFX_GLDITHER_H

//==========================================================================
// Dither - bayer matrix dithering
//==========================================================================
static const char g_szVertexShader_Dither[] =
"#version 130\n"
"void main(void)\n"
"{\n"
"gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"}\n";

static const char g_szFragmentShader_Dither[] =
"#version 130\n"
"#extension GL_ARB_texture_rectangle : enable\n"
"uniform sampler2DRect Texture0;\n"

"uniform vec3 palette[8];\n"
"uniform int paletteSize;\n"
"const float lightnessSteps = 4.0; \n"

"vec3 rgb_to_hsl( const vec3 color )\n"
"{\n"
"vec3 hsl;\n"
"float fmin = min(min(color.r, color.g), color.b);\n"
"float fmax = max(max(color.r, color.g), color.b);\n"
"float delta = fmax - fmin;\n"
"hsl.z = (fmax + fmin) / 2.0;\n"
"if (delta == 0.0) {\n"
"hsl.x = 0.0;\n"
"hsl.y = 0.0;\n"
"} else {\n"
"if (hsl.z < 0.5) hsl.y = delta / (fmax + fmin);\n"
"else hsl.y = delta / (2.0 - fmax - fmin);\n"
"float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;\n"
"float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;\n"
"float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;\n"
"if (color.r == fmax ) hsl.x = deltaB - deltaG;\n"
"else if (color.g == fmax) hsl.x = (1.0 / 3.0) + deltaR - deltaB;\n"
"else if (color.b == fmax) hsl.x = (2.0 / 3.0) + deltaG - deltaR;\n"
"if (hsl.x < 0.0) hsl.x += 1.0;\n"
"else if (hsl.x > 1.0) hsl.x -= 1.0;\n"
"}\n"
"return hsl;\n"
"}\n"


"float hue_to_rgb( float f1, float f2, float hue )\n"
"{\n"
"if (hue < 0.0) hue += 1.0;\n"
"else if (hue > 1.0) hue -= 1.0;\n"
"float res;\n"
"if ((6.0 * hue) < 1.0) res = f1 + (f2 - f1) * 6.0 * hue;\n"
"else if ((2.0 * hue) < 1.0) res = f2;\n"
"else if ((3.0 * hue) < 2.0) res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;\n"
"else res = f1;\n"
"return res;\n"
"}\n"


"vec3 hsl_to_rgb( vec3 hsl )\n"
"{\n"
"vec3 rgb;\n"
"if (hsl.y == 0.0) rgb = vec3(hsl.z);\n"
"else {\n"
"float f2;\n"
"if (hsl.z < 0.5) f2 = hsl.z * (1.0 + hsl.y);\n"
"else f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);\n"
"float f1 = 2.0 * hsl.z - f2;\n"
"rgb.r = hue_to_rgb(f1, f2, hsl.x + (1.0/3.0));\n"
"rgb.g = hue_to_rgb(f1, f2, hsl.x);\n"
"rgb.b= hue_to_rgb(f1, f2, hsl.x - (1.0/3.0));\n"
"}\n"
"return rgb;\n"
"}\n"


"const int indexMatrix8x8[64] = int[]\n"
    "(0, 32, 8, 40, 2, 34, 10, 42, \n"
    "48, 16, 56, 24, 50, 18, 58, 26, \n"
    "12, 44, 4, 36, 14, 46, 6, 38, \n"
    "60, 28, 52, 20, 62, 30, 54, 22, \n"
    "3, 35, 11, 43, 1, 33, 9, 41, \n"
    "51, 19, 59, 27, 49, 17, 57, 25, \n"
    "15, 47, 7, 39, 13, 45, 5, 37, \n"
    "63, 31, 55, 23, 61, 29, 53, 21); \n"

"float indexValue()\n"
"{\n"
    "int x = int(mod(gl_FragCoord.x, 8)); \n"
    "int y = int(mod(gl_FragCoord.y, 8)); \n"
    "return indexMatrix8x8[(x + y * 8)] / 64.0; \n"
"}\n"

"float hueDistance(float h1, float h2)\n"
"{\n"
    "float diff = abs((h1 - h2)); \n"
    "return min(abs((1.0 - diff)), diff); \n"
"}\n"

"vec3[2] closestColors(float hue)\n"
"{\n"
    "vec3 ret[2]; \n"
    "vec3 closest = vec3(-2, 0, 0); \n"
    "vec3 secondClosest = vec3(-2, 0, 0); \n"
    "vec3 temp; \n"

    "for (int i = 0; i < paletteSize; ++i)\n"
    "{\n"
        "temp = palette[i]; \n"
        "float tempDistance = hueDistance(temp.x, hue); \n"
        "if (tempDistance < hueDistance(closest.x, hue))\n"
        "{\n"
            "secondClosest = closest; \n"
            "closest = temp; \n"
        "}\n"
     "else\n"
        "{\n"
            "if (tempDistance < hueDistance(secondClosest.x, hue))\n"
            "{\n"
                "secondClosest = temp; \n"
            "}\n"
        "}\n"
    "}\n"
    "ret[0] = closest; \n"
    "ret[1] = secondClosest; \n"
    "return ret; \n"
"}\n"



"float lightnessStep(float l)\n"
"{\n"
    /* Quantize the lightness to one of `lightnessSteps` values */
    "return floor((0.5 + l * lightnessSteps)) / lightnessSteps; \n"
"}\n"

"vec3 dither(vec3 color)\n"
"{\n"
    "vec3 hsl = rgb_to_hsl(color); \n"

    "vec3 cs[2] = closestColors(hsl.x); \n"
    "vec3 c1 = cs[0]; \n"
    "vec3 c2 = cs[1]; \n"
    "float d = indexValue(); \n"
    "float hueDiff = hueDistance(hsl.x, c1.x) / hueDistance(c2.x, c1.x); \n"

    "float l1 = lightnessStep(max((hsl.z - 0.125), 0.0)); \n"
    "float l2 = lightnessStep(min((hsl.z + 0.124), 1.0)); \n"
    "float lightnessDiff = (hsl.z - l1) / (l2 - l1); \n"

    "vec3 resultColor = (hueDiff < d) ? c1 : c2; \n"
    "resultColor[0] = hsl[0];\n"
    "resultColor[1] = hsl[1];\n"
    "resultColor.z = (lightnessDiff < d) ? l1 : l2; \n"
    "return hsl_to_rgb(resultColor); \n"
"}\n"

"void main()\n"
"{\n"
    "vec3 color = texture2DRect(Texture0, gl_TexCoord[0].xy).rgb;\n"
    "gl_FragColor.rgb = dither(color); \n"
    "gl_FragColor.a	= 1.0;\n"
"}\n";

/*static const char g_szFragmentShader_Dither[] =
"#version 130\n"
"#extension GL_ARB_texture_rectangle : enable\n"
"uniform sampler2DRect Texture0;\n"

"float dither(vec2 position, float brightness)\n"
"{\n"
"int x = int(mod(position.x, 4.0));\n"
"int y = int(mod(position.y, 4.0));\n"
"int index = x + y * 4;\n"
"float limit = 0.0;\n"

"if (x < 8)\n"
"{\n"
"if (index == 0) limit = 0.0625; \n"
"if (index == 1) limit = 0.5625; \n"
"if (index == 2) limit = 0.1875; \n"
"if (index == 3) limit = 0.6875; \n"
"if (index == 4) limit = 0.8125; \n"
"if (index == 5) limit = 0.3125; \n"
"if (index == 6) limit = 0.9375; \n"
"if (index == 7) limit = 0.4375; \n"
"if (index == 8) limit = 0.25; \n"
"if (index == 9) limit = 0.75; \n"
"if (index == 10) limit = 0.125; \n"
"if (index == 11) limit = 0.625; \n"
"if (index == 12) limit = 1.0; \n"
"if (index == 13) limit = 0.5; \n"
"if (index == 14) limit = 0.875; \n"
"if (index == 15) limit = 0.375; \n"
"}\n"

    "return brightness < limit ? 0.0 : 1.0; \n"
"}\n"

"float luma(vec3 color)\n"
"{\n"
    "return dot(color, vec3(0.6, 0.587, 0.5));\n"
"}\n"

"vec3 dither8x8(vec2 position, vec3 color)\n"
"{\n"
    "return vec3(color.rgb * dither(position, luma(color)));\n"
"}\n"

"void main(void)\n"
"{\n"
    "vec3 color = texture2DRect(Texture0, gl_TexCoord[0].xy).rgb;\n"
    "gl_FragColor.rgb = dither8x8(gl_FragCoord.xy, color);\n"

    "gl_FragColor.a	= 1.0;\n"
"}\n";*/

#endif
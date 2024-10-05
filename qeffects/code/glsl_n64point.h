/*
*
*	Copyright (c) 2024, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
*/
#ifndef QFX_GLN64_H
#define QFX_GLN64_H

//==========================================================================
// N64 3-Point Filtering
//==========================================================================
static const char g_szVertexShader_n64point[] =
"#version 130\n"
"void main(void)\n"
"{\n"
	"gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"}\n";

static const char g_szFragmentShader_n64point[] =
"#version 130\n"
"#extension GL_ARB_texture_rectangle : enable\n"
"uniform sampler2DRect Texture0;\n"
"uniform vec4 Local0;\n" //m_var3point

"float texture_size;\n"

"vec2 norm2denorm(sampler2DRect tex, vec2 uv)\n"
"{\n"
	//"return uv * vec2(textureSize(tex, 0)) - 0.5; \n"
    "return uv * vec2(texture_size,texture_size) - 0.5;\n"
"}\n"

"ivec2 denorm2idx(vec2 d_uv)\n"
"{\n"
	"return ivec2(floor(d_uv)); \n"
"}\n"

"ivec2 norm2idx(sampler2DRect tex, vec2 uv)\n"
"{\n"
	"return denorm2idx(norm2denorm(tex, uv)); \n"
"}\n"

"vec2 idx2norm(sampler2DRect tex, ivec2 idx)\n"
"{\n"
	"vec2 denorm_uv = vec2(idx) + 0.5; \n"
	//"vec2 size = vec2(textureSize(tex, 0)); \n"
    "vec2 size = vec2(texture_size, texture_size);\n"
	"return denorm_uv / size; \n"
"}\n"

"vec4 texel_fetch(sampler2DRect tex, ivec2 idx)\n"
"{\n"
	"vec2 uv = idx2norm(tex, idx); \n"
	"return texture(tex, uv); \n"
"}\n"

/*
 * Unlike Nintendo's documentation, the N64 does not use
 * the 3 closest texels.
 * The texel grid is triangulated:
 *
 *     0 .. 1        0 .. 1
 *   0 +----+      0 +----+
 *     |   /|        |\   |
 *   . |  / |        | \  |
 *   . | /  |        |  \ |
 *     |/   |        |   \|
 *   1 +----+      1 +----+
 *
 * If the sampled point falls above the diagonal,
 * The top triangle is used; otherwise, it's the bottom.
 */
"vec4 texture_3point(sampler2DRect tex, vec2 uv)\n"
"{\n"
    "vec2 denorm_uv = norm2denorm(tex, uv); \n"
    "ivec2 idx_low = denorm2idx(denorm_uv); \n"
    "vec2 ratio = denorm_uv - vec2(idx_low); \n"
#if 0
    "bool lower_flag = ratio.s - ratio.t > 0.0; \n"
    "ivec2 corner0 = lower_flag ? ivec2(1, 0) : ivec2(0, 1); \n"
#else
    "int lower_flag = int(step(0.0, ratio.s - ratio.t)); \n"
    "ivec2 corner0 = ivec2(lower_flag, 1 - lower_flag); \n"
#endif
    "ivec2 corner1 = ivec2(0, 0); \n"
    "ivec2 corner2 = ivec2(1, 1); \n"
// end of triangle if
    "ivec2 idx0 = idx_low + corner0; \n"
    "ivec2 idx1 = idx_low + corner1; \n"
    "ivec2 idx2 = idx_low + corner2; \n"

    "vec4 t0 = texel_fetch(tex, idx0); \n"
    "vec4 t1 = texel_fetch(tex, idx1); \n"
    "vec4 t2 = texel_fetch(tex, idx2); \n"

    // This is standard (Crammer's rule) barycentric coordinates calculation.
    "vec2 v0 = vec2(corner1 - corner0); \n"
    "vec2 v1 = vec2(corner2 - corner0); \n"
    "vec2 v2 = ratio - vec2(corner0); \n"
    "float den = v0.x * v1.y - v1.x * v0.y; \n"
    /*
     * Note: the abs() here is necessary because we don't guarantee
     * the proper order of vertices, so some signed areas are negative.
     * But since we only interpolate inside the triangle, the areas
     * are guaranteed to be positive, if we did the math more carefully.
     */
    "float lambda1 = abs((v2.x * v1.y - v1.x * v2.y) / den); \n"
    "float lambda2 = abs((v0.x * v2.y - v2.x * v0.y) / den); \n"
    "float lambda0 = 1.0 - lambda1 - lambda2; \n"

    "return lambda0 * t0 + lambda1 * t1 + lambda2 * t2; \n"
"}\n"


"void main()\n"
"{\n"
    "float screenHeight = textureSize(Texture0).y;\n"
    "float relativePixelSize = Local0.z;\n"
    "float xscale = abs(relativePixelSize / screenHeight);\n"
    "texture_size = xscale;\n"

	"vec3 col = texture_3point(Texture0, gl_TexCoord[0].xy).rgb;\n"

	"gl_FragColor.rgb = col;\n"
	"gl_FragColor.a = 1.0;\n"
"}\n";

#endif
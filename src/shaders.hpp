#ifndef SHADER_H
#define SHADER_H

const char *engineShaderSource = R"(
#version 460 core

#extension GL_ARB_shader_ballot : enable

#define KEY_SPACE              32
#define KEY_APOSTROPHE         39
#define KEY_COMMA              44
#define KEY_MINUS              45
#define KEY_PERIOD             46
#define KEY_SLASH              47
#define KEY_0                  48
#define KEY_1                  49
#define KEY_2                  50
#define KEY_3                  51
#define KEY_4                  52
#define KEY_5                  53
#define KEY_6                  54
#define KEY_7                  55
#define KEY_8                  56
#define KEY_9                  57
#define KEY_SEMICOLON          59
#define KEY_EQUAL              61
#define KEY_A                  65
#define KEY_B                  66
#define KEY_C                  67
#define KEY_D                  68
#define KEY_E                  69
#define KEY_F                  70
#define KEY_G                  71
#define KEY_H                  72
#define KEY_I                  73
#define KEY_J                  74
#define KEY_K                  75
#define KEY_L                  76
#define KEY_M                  77
#define KEY_N                  78
#define KEY_O                  79
#define KEY_P                  80
#define KEY_Q                  81
#define KEY_R                  82
#define KEY_S                  83
#define KEY_T                  84
#define KEY_U                  85
#define KEY_V                  86
#define KEY_W                  87
#define KEY_X                  88
#define KEY_Y                  89
#define KEY_Z                  90
#define KEY_LEFT_BRACKET       91
#define KEY_BACKSLASH          92
#define KEY_RIGHT_BRACKET      93
#define KEY_GRAVE_ACCENT       96
#define KEY_WORLD_1            161
#define KEY_WORLD_2            162
#define KEY_ESCAPE             256
#define KEY_ENTER              257
#define KEY_TAB                258
#define KEY_BACKSPACE          259
#define KEY_INSERT             260
#define KEY_DELETE             261
#define KEY_RIGHT              262
#define KEY_LEFT               263
#define KEY_DOWN               264
#define KEY_UP                 265
#define KEY_PAGE_UP            266
#define KEY_PAGE_DOWN          267
#define KEY_HOME               268
#define KEY_END                269
#define KEY_CAPS_LOCK          280
#define KEY_SCROLL_LOCK        281
#define KEY_NUM_LOCK           282
#define KEY_PRINT_SCREEN       283
#define KEY_PAUSE              284
#define KEY_F1                 290
#define KEY_F2                 291
#define KEY_F3                 292
#define KEY_F4                 293
#define KEY_F5                 294
#define KEY_F6                 295
#define KEY_F7                 296
#define KEY_F8                 297
#define KEY_F9                 298
#define KEY_F10                299
#define KEY_F11                300
#define KEY_F12                301
#define KEY_F13                302
#define KEY_F14                303
#define KEY_F15                304
#define KEY_F16                305
#define KEY_F17                306
#define KEY_F18                307
#define KEY_F19                308
#define KEY_F20                309
#define KEY_F21                310
#define KEY_F22                311
#define KEY_F23                312
#define KEY_F24                313
#define KEY_F25                314
#define KEY_KP_0               320
#define KEY_KP_1               321
#define KEY_KP_2               322
#define KEY_KP_3               323
#define KEY_KP_4               324
#define KEY_KP_5               325
#define KEY_KP_6               326
#define KEY_KP_7               327
#define KEY_KP_8               328
#define KEY_KP_9               329
#define KEY_KP_DECIMAL         330
#define KEY_KP_DIVIDE          331
#define KEY_KP_MULTIPLY        332
#define KEY_KP_SUBTRACT        333
#define KEY_KP_ADD             334
#define KEY_KP_ENTER           335
#define KEY_KP_EQUAL           336
#define KEY_LEFT_SHIFT         340
#define KEY_LEFT_CONTROL       341
#define KEY_LEFT_ALT           342
#define KEY_LEFT_SUPER         343
#define KEY_RIGHT_SHIFT        344
#define KEY_RIGHT_CONTROL      345
#define KEY_RIGHT_ALT          346
#define KEY_RIGHT_SUPER        347
#define KEY_MENU               348
#define KEY_LAST               KEY_MENU

#define MOUSE_BUTTON_1         0
#define MOUSE_BUTTON_2         1
#define MOUSE_BUTTON_3         2
#define MOUSE_BUTTON_4         3
#define MOUSE_BUTTON_5         4
#define MOUSE_BUTTON_6         5
#define MOUSE_BUTTON_7         6
#define MOUSE_BUTTON_8         7
#define MOUSE_BUTTON_LAST      MOUSE_BUTTON_8
#define MOUSE_BUTTON_LEFT      MOUSE_BUTTON_1
#define MOUSE_BUTTON_RIGHT     MOUSE_BUTTON_2
#define MOUSE_BUTTON_MIDDLE    MOUSE_BUTTON_3

#define STATE_RELEASED 0
#define STATE_PRESSED 1

layout(std430, binding = 0) buffer EngineBuffer {
    int width;
    int height;
    double currentTime;
    double deltaTime;
    int mouseX;
    int mouseY;
    int mouse[MOUSE_BUTTON_LAST];
    int keys[KEY_LAST];
} engineBuffer;

bool key_pressed(uint key) {
    return engineBuffer.keys[key] == STATE_PRESSED;
}

bool mouse_pressed(uint key) {
    return engineBuffer.mouse[key] == STATE_PRESSED;
}

layout(std430, binding = 1) buffer WorkGroupBuffer {
    uint x;
    uint y;
    uint z; 
} workGroupBuffer;

struct DrawCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

layout(std430, binding = 2) writeonly buffer DrawCommandBuffer {
    DrawCommand commands[100];
} drawCommandBuffer;

void set_drawcommand(uint offset, uint count, uint instances, uint firstIndex, uint baseVertex, uint baseInstance)
{
    drawCommandBuffer.commands[offset].count = count;
    drawCommandBuffer.commands[offset].instanceCount = instances;
    drawCommandBuffer.commands[offset].firstIndex = firstIndex;
    drawCommandBuffer.commands[offset].baseVertex = baseVertex;
    drawCommandBuffer.commands[offset].baseInstance = baseInstance;
}
)";

const char *mathShaderSource = R"(
    float rand() {
        // Inspired by: https://thebookofshaders.com/10/
        vec2 seed = vec2(engineBuffer.currentTime, engineBuffer.currentTime);
        return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
    }

    int randInt(int left, int right) {
        return int(rand() * (right - left + 1) + left);
    }

    mat4 perspective(float fov, float aspect, float near, float far) 
    {
        return mat4(
            vec4(1.0 / (aspect * tan(fov / 2.0)), 0, 0, 0),
            vec4(0, 1.0 / tan(fov / 2.0), 0, 0),
            vec4(0, 0, -(far + near) / (far - near), -1.0),
            vec4(0, 0, -2.0 * (far * near) / (far - near), 0)
        );
    }

    mat4 rotate(mat4 matrix, float angle, vec3 rotation_axis)
    {
        float c = cos(angle);
        float s = sin(angle);

        float x = rotation_axis.x; float y = rotation_axis.y; float z = rotation_axis.z;

        mat4 r_matrix = mat4(
            vec4(x*x*(1-c)+c,   y*x*(1-c)+z*s, z*x*(1-c)-y*s, 0),
            vec4(x*y*(1-c)-z*s, y*y*(1-c)+c,   z*y*(1-c)+x*s, 0),
            vec4(x*z*(1-c)+y*s, y*z*(1-c)-x*s, z*z*(1-c)+c,   0),
            vec4(0, 0, 0, 1)
        );
        
        return matrix * r_matrix;
    }

    mat4 translate(mat4 matrix, vec3 translation)
    {
        float x = translation.x; float y = translation.y; float z = translation.z;

        mat4 t_matrix = mat4(
            vec4(1, 0, 0, 0),
            vec4(0, 1, 0, 0),
            vec4(0, 0, 1, 0),
            vec4(x, y, z, 1)
        );

        return matrix * t_matrix;
    }

    mat4 scale(mat4 matrix, vec3 scale_vector)
    {
        float x = scale_vector.x; float y = scale_vector.y; float z = scale_vector.z;

        mat4 s_matrix = mat4(
            vec4(x, 0, 0, 0),
            vec4(0, y, 0, 0),
            vec4(0, 0, z, 0),
            vec4(0, 0, 0, 1)
        );

        return matrix * s_matrix;
    }

    mat4 lookAt(vec3 position, vec3 target, vec3 up)
    {
        vec3 d = normalize(position - target);
        vec3 r = normalize(cross(up, d));
        vec3 u = cross(d, r);

        return mat4(
            vec4(r.x, u.x, d.x, 0),
            vec4(r.y, u.y, d.y, 0),
            vec4(r.z, u.z, d.z, 0),
            vec4(0, 0, 0, 1)
        ) * mat4(
            vec4(1, 0, 0, 0),
            vec4(0, 1, 0, 0),
            vec4(0, 0, 1, 0),
            vec4(-position.x, -position.y, -position.z, 1)
        );
    }
)";

#endif
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D texture2;
uniform ivec2 SIM_SIZE;

void main() {
    vec4 h = textureLod(texture2, aTexCoord, 0);
    float yOffset = h.r;
    vec4 pos = vec4(aPos.x, aPos.y + yOffset, aPos.z, 1.0);

    gl_Position = projection * view * model * pos;
    TexCoord = aTexCoord;
    FragPos = vec3(model * vec4(aPos, 1.0));
}

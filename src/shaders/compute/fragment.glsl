#version 460 core

in vec2 TexCoord;
in vec3 FragPos;

out vec4 FragColor;

uniform float time;
// Lighting
uniform vec3 lightColor;
uniform vec3 lightPos;
// Colors
uniform vec3 col1;
uniform vec3 col2;

uniform ivec2 SIM_SIZE;

uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D normals;
uniform sampler2D colorPalette;

const float ambientStrength = 0.2;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 rgb2normalized(float r, float g, float b) {
    return vec3(r / 255.0, g / 255.0, b / 255.0);
}

vec3 rgb2normalized(vec3 c) {
    return vec3(c.r / 255.0, c.g / 255.0, c.b / 255.0);
}

vec3 calcNormal() {
    // central difference approximation of gradient
    float EPSILON_X = 1.0 / SIM_SIZE[0];
    float EPSILON_Y = 1.0 / SIM_SIZE[1];
    float hxp = texture(texture2, vec2(TexCoord.x + EPSILON_X, TexCoord.y)).r;
    float hxn = texture(texture2, vec2(TexCoord.x - EPSILON_X, TexCoord.y)).r;
    float hyp = texture(texture2, vec2(TexCoord.x, TexCoord.y + EPSILON_Y)).r;
    float hyn = texture(texture2, vec2(TexCoord.x, TexCoord.y - EPSILON_Y)).r;
    return normalize(vec3((hxn - hxp) / (EPSILON_X * 2), 1,- (hyn - hyp) / (EPSILON_Y * 2)));
}

void main(){
    vec4 h = texture(texture2, TexCoord);
    vec3 col = mix(rgb2normalized(col1)*1.2,rgb2normalized(col2)*1.2,h.r*2.5);

    // lighting
    vec3 n = calcNormal();
    n = mix(n, vec3(0,1,0), 0.3);
    vec3 ambient = ambientStrength * lightColor;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(n, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 finalCol = col * (ambient + diffuse); 

    FragColor = vec4(finalCol, 1.0);
}

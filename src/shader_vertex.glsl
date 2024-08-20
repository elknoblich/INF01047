#version 330 core

layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int object_id;

out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;
out vec4 v_color;

#define CUBE   0
#define SHARK  1
#define FISH   2
#define CUBE2  3
#define CUBE3  4
#define SEAWEED0 5

uniform sampler2D Seaweed0;
void main()
{

    gl_Position = projection * view * model * model_coefficients;

    position_world = model * model_coefficients;

    position_model = model_coefficients;

    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    texcoords = texture_coefficients;

    if (object_id == SEAWEED0) {

        vec4 camera_position = inverse(view) * vec4(0.0, 0.0, 0.0, 1.0);
        vec4 l = normalize(camera_position - position_world);

        vec4 n = normalize(normal);

        vec3 I = vec3(1.0, 1.0, 1.0);
        vec3 Kd = texture(Seaweed0, texcoords).rgb;
        vec3 lambert_diffuse_term = Kd * I * max(0, dot(n, l));

        v_color = vec4(lambert_diffuse_term, 1.0);
    }
}


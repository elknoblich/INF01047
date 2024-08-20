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
#define SEAWEED1 6

uniform sampler2D Shark0;
uniform sampler2D Shark1;
uniform sampler2D Shark2;
uniform sampler2D Fish;
uniform sampler2D Sand;
uniform sampler2D Seaweed0;
uniform sampler2D Seaweed1;
void main()
{

    gl_Position = projection * view * model * model_coefficients;

    position_world = model * model_coefficients;

    position_model = model_coefficients;

    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    texcoords = texture_coefficients;

    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    vec4 p = position_world;

    vec4 n = normalize(normal);

    vec4 l = normalize(camera_position - p);

    vec4 v = normalize(camera_position - p);
    vec4 r = normalize(-l + 2*n*(dot(n,l)));
    vec4 h = normalize(v+l);
    float U = 0.0;
    float V = 0.0;
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q;

    if (object_id == SEAWEED0) {

        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(Seaweed0, vec2(U,V)).rgb;
        Ka = vec3(0.01,0.08,0.01);
        Ks = vec3(0.0,0.0,0.0);
        q = 0.26;
    }else if(object_id == SEAWEED1){
        v_color = vec4(texcoords, 0.0, 1.0);
        return;

        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(Seaweed1, vec2(U,V)).rgb;
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.0,0.0,0.0);
        q = 1.0;
    }else{

        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.0,0.0,0.0);
        q = 1.0;
    }

    vec3 I = vec3(0.27,0.54,0.90);

    vec3 Ia = vec3(0.2,0.2,0.2);

    vec3 lambert_diffuse_term = Kd * I * max(0, dot(n,l));

    vec3 ambient_term = Ka * Ia;

    vec3 blinn_phong_specular_term  = Ks*I*pow(max(0.0,dot(n,h)),q);

    v_color = vec4(lambert_diffuse_term + ambient_term + blinn_phong_specular_term, 1.0);
}


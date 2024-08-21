// INF01047: Trabalho Final
// Erick Larratéa Knolibhc 00324422
// Nicolas André Nunes da Silva 00279050

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <list>
#include <memory>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>  // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h> // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include "utils.h"
#include "matrices.h"
#include "collisions.hpp"

struct ObjModel {
   tinyobj::attrib_t attrib;
   std::vector<tinyobj::shape_t> shapes;
   std::vector<tinyobj::material_t> materials;

   ObjModel(const char *filename, const char *basepath = NULL, bool triangulate = true) {
      printf("Carregando objetos do arquivo \"%s\"...\n", filename);

      std::string fullpath(filename);
      std::string dirname;
      if (basepath == NULL) {
         auto i = fullpath.find_last_of("/");
         if (i != std::string::npos) {
            dirname  = fullpath.substr(0, i + 1);
            basepath = dirname.c_str();
         }
      }

      std::string warn;
      std::string err;
      bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

      if (!err.empty())
         fprintf(stderr, "\n%s\n", err.c_str());

      if (!ret)
         throw std::runtime_error("Erro ao carregar modelo.");

      for (size_t shape = 0; shape < shapes.size(); ++shape) {
         if (shapes[shape].name.empty()) {
            fprintf(stderr,
                    "*********************************************\n"
                    "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                    "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                    "*********************************************\n",
                    filename);
            throw std::runtime_error("Objeto sem nome.");
         }
         printf("- Objeto '%s'\n", shapes[shape].name.c_str());
      }

      printf("OK.\n");
   }
};

void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4 &M);

void BuildTrianglesAndAddToVirtualScene(ObjModel *);
void ComputeNormals(ObjModel *model);
void LoadShadersFromFiles();
void LoadTextureImage(const char *filename);
void DrawVirtualObject(const char *object_name);
GLuint LoadShader_Vertex(const char *filename);
GLuint LoadShader_Fragment(const char *filename);
void LoadShader(const char *filename, GLuint shader_id);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow *window);
float TextRendering_CharWidth(GLFWwindow *window);
void TextRendering_PrintString(GLFWwindow *window, const std::string &str, float x, float y, float scale = 1.0f);

void TextRendering_ShowFramesPerSecond(GLFWwindow *window);

void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

glm::vec3 cubic_bezier(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
void animate_whale(AABB &box, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float speed, float elapsedTime);
void draw_shark();
void draw_fishes();
void draw_objects();
void update_shark_collision_map(glm::vec4 velocity, float t_first[], float t_last[]);
void update_fishes_eatability();
void shark_movement(float &prev_time, float &shark_rotation, glm::vec4 &velocity, float t_first[], float t_last[]);
void create_seaweed_models();
void create_fishes();

struct SceneObject {
   std::string name;
   size_t first_index;
   size_t num_indices;
   GLenum rendering_mode;
   GLuint vertex_array_object_id;
   glm::vec3 bbox_min;
   glm::vec3 bbox_max;
};

std::map<std::string, SceneObject> g_VirtualScene;

std::stack<glm::mat4> g_MatrixStack;

float g_ScreenRatio = 1.0f;

float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

bool g_LeftMouseButtonPressed   = false;
bool g_RightMouseButtonPressed  = false;
bool g_MiddleMouseButtonPressed = false;
bool g_LeftMouseButtonClicked   = false;
bool g_W_pressed                = false;
bool g_A_pressed                = false;
bool g_D_pressed                = false;
bool g_S_pressed                = false;
bool g_LSHIFT_pressed           = false;
bool g_LCTRL_pressed            = false;
bool g_is_free_cam              = true;

float g_CameraTheta    = 0.0f;
float g_CameraPhi      = 0.0f;
float g_CameraDistance = 3.5f;


bool g_ShowInfoText = true;

GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

GLuint g_NumLoadedTextures = 0;
AABB *shark_p;
glm::vec4 g_camera_lookat_l    = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
glm::vec4 g_camera_view_vector = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f); // Vetor "view", sentido para onde a câmera está virada
glm::vec4 g_camera_up_vector   = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
glm::vec4 g_camera_position_c  = glm::vec4(-0.75f, 0.75f, 0.75f, 1.0f);

std::map<AABB *, bool> is_eated;
std::map<AABB *, bool> is_eatable;
std::list<AABB *> fish_list;
std::map<AABB, bool> shark_collision_map;
std::list<AABB> shark_arena_walls;
std::list<glm::mat4> seaweeds_models;
float s_speed         = 0.5f;
float g_radius        = 1.5f;
bool is_eatable_whale = false;
bool is_eated_whale   = false;
#define CUBE     0
#define SHARK    1
#define FISH     2
#define CUBE2    3
#define CUBE3    4
#define SEAWEED0 5
#define SEAWEED1 6
#define DOLPHIN  7
#define WHALE    8
int main(int argc, char *argv[]) {

   int success = glfwInit();
   if (!success) {
      fprintf(stderr, "ERROR: glfwInit() failed.\n");
      std::exit(EXIT_FAILURE);
   }

   glfwSetErrorCallback(ErrorCallback);

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   GLFWwindow *window;
   window = glfwCreateWindow(800, 600, "INF01047 - Trabalho", NULL, NULL);
   if (!window) {
      glfwTerminate();
      fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
      std::exit(EXIT_FAILURE);
   }

   glfwSetKeyCallback(window, KeyCallback);

   glfwSetMouseButtonCallback(window, MouseButtonCallback);

   glfwSetCursorPosCallback(window, CursorPosCallback);

   glfwSetScrollCallback(window, ScrollCallback);

   glfwMakeContextCurrent(window);
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

   gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

   glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
   FramebufferSizeCallback(window, 800, 600);

   const GLubyte *vendor      = glGetString(GL_VENDOR);
   const GLubyte *renderer    = glGetString(GL_RENDERER);
   const GLubyte *glversion   = glGetString(GL_VERSION);
   const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

   printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

   LoadShadersFromFiles();

   LoadTextureImage("../../data/texture.jpg");
   LoadTextureImage("../../data/texture_N.jpg");
   LoadTextureImage("../../data/texture_S.jpg");
   LoadTextureImage("../../data/fish.jpg");
   LoadTextureImage("../../data/aerial_beach_01_diff_4k.jpg");
   LoadTextureImage("../../data/seaweed0.jpeg");
   LoadTextureImage("../../data/seaweed1.png");
   LoadTextureImage("../../data/dolphin.png");
   LoadTextureImage("../../data/whale.jpg");

   ObjModel cubemodel("../../data/cube.obj");
   ComputeNormals(&cubemodel);
   BuildTrianglesAndAddToVirtualScene(&cubemodel);

   ObjModel cube2model("../../data/cube2.obj");
   ComputeNormals(&cube2model);
   BuildTrianglesAndAddToVirtualScene(&cube2model);

   ObjModel cube3model("../../data/cube3.obj");
   ComputeNormals(&cube3model);
   BuildTrianglesAndAddToVirtualScene(&cube3model);

   ObjModel sharkmodel("../../data/shark.obj");
   ComputeNormals(&sharkmodel);
   BuildTrianglesAndAddToVirtualScene(&sharkmodel);

   ObjModel fishmodel("../../data/fish.obj");
   ComputeNormals(&fishmodel);
   BuildTrianglesAndAddToVirtualScene(&fishmodel);

   ObjModel seaweed0model("../../data/seaweed0.obj");
   ComputeNormals(&seaweed0model);
   BuildTrianglesAndAddToVirtualScene(&seaweed0model);

   ObjModel seaweed1model("../../data/seaweed1.obj");
   ComputeNormals(&seaweed1model);
   BuildTrianglesAndAddToVirtualScene(&seaweed1model);

   ObjModel dolphinmodel("../../data/dolphin.obj");
   ComputeNormals(&dolphinmodel);
   BuildTrianglesAndAddToVirtualScene(&dolphinmodel);

   ObjModel whalemodel("../../data/whale.obj");
   ComputeNormals(&whalemodel);
   BuildTrianglesAndAddToVirtualScene(&whalemodel);

   if (argc > 1) {
      ObjModel model(argv[1]);
      BuildTrianglesAndAddToVirtualScene(&model);
   }

   TextRendering_Init();

   glEnable(GL_DEPTH_TEST);

   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glFrontFace(GL_CCW);

   float prev_time = (float)glfwGetTime();
   float speed     = 0.5f;


   glm::mat4 model = Matrix_Identity();

   model = Matrix_Translate(-80.0f, 1.0f, 0.0f) * Matrix_Rotate_Y(3.1415f / 2.0f) * Matrix_Scale(1.0f, 1.0f, 1.0f);
   AABB aabb_shark(g_VirtualScene["Object_TexMap_0"].bbox_min, g_VirtualScene["Object_TexMap_0"].bbox_max, model, -1, "Object_TexMap_0");
   shark_p = &aabb_shark;

   /*******************FISH===FISH*****************/
   create_fishes();

   model = Matrix_Translate(-80.0f, 1.0f, 0.0f) * Matrix_Rotate_Y(3.1415f / 2.0f) * Matrix_Scale(8.0f, 8.0f, 8.0f);
   AABB aabb_whale(g_VirtualScene["whale"].bbox_min, g_VirtualScene["whale"].bbox_max, model, -1, "whale");

   /**Static objects**/
   int current_sobj_id = 0;
   model               = Matrix_Translate(-80.0f, -10.0f, 0.0f) * Matrix_Scale(80.f, 0.1f, 80.0f);
   AABB aabb_wall_1(g_VirtualScene["Cube2"].bbox_min, g_VirtualScene["Cube2"].bbox_max, model, current_sobj_id, "Cube2");
   shark_arena_walls.push_back(aabb_wall_1);
   shark_collision_map[aabb_wall_1] = false;
   ++current_sobj_id;

   create_seaweed_models();
   float t_first[current_sobj_id];
   float t_last[current_sobj_id];

   g_is_free_cam        = false;
   float shark_rotation = 0;

   while (!glfwWindowShouldClose(window)) {

      float current_time = (float)glfwGetTime();

      g_LeftMouseButtonClicked = false;

      glClearColor(0.0f, 0.2f, 0.9f, 0.6f);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glUseProgram(g_GpuProgramID);

      float r = g_CameraDistance;
      float y = r * sin(g_CameraPhi);
      float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
      float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);

      glm::vec4 velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);


      if (g_is_free_cam) {
         g_camera_view_vector = glm::vec4(x, -y, z, 0.0f);
      } else if (!g_is_free_cam) {
         shark_movement(prev_time, shark_rotation, velocity, t_first, t_last);
      }

      glm::mat4 view = Matrix_Camera_View(g_camera_position_c, g_camera_view_vector, g_camera_up_vector);

      glm::mat4 projection;

      float nearplane = -0.1f;
      float farplane  = -30.0f;


      float field_of_view = 3.141592 / 3.0f;
      projection          = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);


      glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));


      draw_objects();
      draw_shark();
      draw_fishes();

      update_fishes_eatability();
      update_shark_collision_map(velocity, t_first, t_last);

      if (s_speed > 1.2f && !is_eated_whale) {
         glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(aabb_whale.get_model()));
         glUniform1i(g_object_id_uniform, WHALE);
         DrawVirtualObject("whale");

         animate_whale(aabb_whale, glm::vec3(-80.0f, 2.0f, 0.0f), glm::vec3(-50.0f, -4.0f, 1.0f), glm::vec3(-20.0f, 2.0f, -2.0f),
                       glm::vec3(-80.0f, 2.0f, 0.0f), 0.04, current_time);

         SPHERE interaction_sphere(g_camera_position_c, g_radius, -1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
         is_eatable_whale = g_is_free_cam && Sphere_to_AABB_intersec(interaction_sphere, aabb_whale) &&
             ray_to_AABB_intersec(g_camera_position_c, g_camera_view_vector / norm(g_camera_view_vector), aabb_whale);
      }


      TextRendering_ShowFramesPerSecond(window);

      glfwSwapBuffers(window);

      glfwPollEvents();
   }

   glfwTerminate();

   return 0;
}
void draw_shark() {
   if (!g_is_free_cam) {
      glm::mat4 model = shark_p->get_model();

      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, SHARK);

      DrawVirtualObject("Object_TexMap_0");
   }
}

void draw_objects() {
   int i = 0;
   for (const auto &current_model: seaweeds_models) {

      //  if (i == 0) {

      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
      glUniform1i(g_object_id_uniform, SEAWEED0);
      DrawVirtualObject("seaweed0");
      //} else {
      // glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
      //glUniform1i(g_object_id_uniform, SEAWEED1);
      //DrawVirtualObject("seaweed1");
      // }
      // ++i;
   }

   for (const auto &current_aabb: shark_arena_walls) {

      glm::mat4 current_model = current_aabb.get_model();
      std::string type        = current_aabb.get_type();


      if (type == "Cube2") {
         glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
         glUniform1i(g_object_id_uniform, CUBE2);
         DrawVirtualObject("Cube2");
      }
   }
}

void draw_fishes() {

   for (const auto &current_aabb: fish_list) {

      glm::mat4 current_model = current_aabb->get_model();
      std::string type        = current_aabb->get_type();

      if (is_eated[current_aabb])
         continue;

      if (type == "Object_BlueTaT.jpg") {

         glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
         glUniform1i(g_object_id_uniform, FISH);
         DrawVirtualObject("Object_BlueTaT.jpg");
      }
      if (type == "dolphin") {

         glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
         glUniform1i(g_object_id_uniform, DOLPHIN);
         DrawVirtualObject("dolphin");
      }
   }
}

void update_fishes_eatability() {

   SPHERE interaction_sphere(g_camera_position_c, g_radius, -1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

   for (const auto &current_aabb: fish_list) {

      if (is_eated[current_aabb])
         continue;

      is_eatable[current_aabb] = g_is_free_cam && Sphere_to_AABB_intersec(interaction_sphere, *current_aabb) &&
          ray_to_AABB_intersec(g_camera_position_c, g_camera_view_vector / norm(g_camera_view_vector), *current_aabb);
   }
}

void update_shark_collision_map(glm::vec4 velocity, float t_first[], float t_last[]) {
   int i = 0;
   for (const auto &current_aabb: shark_arena_walls) {
      shark_collision_map[current_aabb] = moving_AABB_to_AABB_intersec(*shark_p, current_aabb, velocity, t_first[i], t_last[i]);
      ++i;
   }
}

glm::vec3 cubic_bezier(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {

   float u   = 1 - t;
   float tt  = t * t;
   float uu  = u * u;
   float uuu = uu * u;
   float ttt = tt * t;

   glm::vec3 p = uuu * p0;
   p += 3 * uu * t * p1;
   p += 3 * u * tt * p2;
   p += ttt * p3;

   return p;
}
void animate_whale(AABB &box, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float speed, float elapsedTime) {

   float t = fmod(elapsedTime * speed, 1.0f);

   glm::vec3 new_position = cubic_bezier(t, p0, p1, p2, p3);

   glm::mat4 rotation_matrix = Matrix_Rotate_Y(-3.1415f / 2.0f);

   if (t > 0.5) {
      rotation_matrix = Matrix_Rotate_Y(3.1415f / 2.0f);
   }

   glm::mat4 new_model_matrix = Matrix_Translate(new_position.x, new_position.y, new_position.z) * rotation_matrix * Matrix_Scale(6.0f, 6.0f, 6.0f);

   box.update_aabb(new_model_matrix, g_VirtualScene[box.get_type()].bbox_min, g_VirtualScene[box.get_type()].bbox_max);
}

//FONTE: ChatGPT usado para descobrir o forward_direction e corrigir a resposta da colisão
void shark_movement(float &prev_time, float &shark_rotation, glm::vec4 &velocity, float t_first[], float t_last[]) {
   g_camera_lookat_l    = shark_p->get_center_point();
   g_camera_position_c  = glm::vec4(10.0f, 0.0f, 0.0f, 0.0f) + g_camera_lookat_l;
   g_camera_view_vector = g_camera_lookat_l - g_camera_position_c;

   float current_time = (float)glfwGetTime();
   float delta_t      = current_time - prev_time;
   prev_time          = current_time;


   const float ROTATION_CONST      = 0.923599;
   bool is_rotated                 = false;
   glm::mat4 shark_rotation_matrix = Matrix_Identity();

   if (g_A_pressed) {
      shark_rotation = ROTATION_CONST * delta_t;
      is_rotated     = true;
   }

   if (g_D_pressed) {
      shark_rotation = -ROTATION_CONST * delta_t;
      is_rotated     = true;
   }


   if (is_rotated) {
      shark_rotation_matrix = Matrix_Rotate_Y(shark_rotation);
      shark_p->update_aabb(shark_p->get_model() * shark_rotation_matrix, g_VirtualScene["Object_TexMap_0"].bbox_min,
                           g_VirtualScene["Object_TexMap_0"].bbox_max);
   }


   glm::vec4 forward_direction = glm::vec4(-shark_p->get_model()[0].x, -shark_p->get_model()[0].y, -shark_p->get_model()[0].z, 0.0f);
   forward_direction           = forward_direction / norm(forward_direction);
   if (g_W_pressed)
      velocity += forward_direction;

   if (g_LCTRL_pressed)
      velocity -= g_camera_up_vector;

   if (g_LSHIFT_pressed)
      velocity += g_camera_up_vector;

   if (velocity.x != 0 || velocity.z != 0)
      velocity = velocity / norm(velocity);

   int count   = 0;
   float speed = s_speed;
   for (const auto &current_aabb: shark_arena_walls) {

      if (shark_collision_map[current_aabb]) {

         velocity += velocity * t_first[count] * delta_t;

         velocity = g_camera_up_vector;

         velocity = velocity * speed * 5.0f * delta_t;
         glm::mat4 shark_translation =
             Matrix_Translate(velocity.x * (1.0f - t_first[count]), velocity.y * (1.0f - t_first[count]), velocity.z * (1.0f - t_first[count]));

         shark_p->update_aabb(shark_translation * shark_p->get_model(), g_VirtualScene["Object_TexMap_0"].bbox_min,
                              g_VirtualScene["Object_TexMap_0"].bbox_max);

      } else {
         velocity                    = velocity * speed * 5.0f * delta_t;
         glm::mat4 shark_translation = Matrix_Translate(velocity.x, velocity.y, velocity.z);

         shark_p->update_aabb(shark_translation * shark_p->get_model(), g_VirtualScene["Object_TexMap_0"].bbox_min,
                              g_VirtualScene["Object_TexMap_0"].bbox_max);
      }
   }
}
void LoadTextureImage(const char *filename) {
   printf("Carregando imagem \"%s\"... ", filename);

   stbi_set_flip_vertically_on_load(true);
   int width;
   int height;
   int channels;
   unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

   if (data == NULL) {
      fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
      std::exit(EXIT_FAILURE);
   }

   printf("OK (%dx%d).\n", width, height);

   GLuint texture_id;
   GLuint sampler_id;
   glGenTextures(1, &texture_id);
   glGenSamplers(1, &sampler_id);

   glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

   GLuint textureunit = g_NumLoadedTextures;
   glActiveTexture(GL_TEXTURE0 + textureunit);
   glBindTexture(GL_TEXTURE_2D, texture_id);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
   glGenerateMipmap(GL_TEXTURE_2D);
   glBindSampler(textureunit, sampler_id);

   stbi_image_free(data);

   g_NumLoadedTextures += 1;
}

void DrawVirtualObject(const char *object_name) {

   glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

   glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
   glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
   glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
   glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

   glDrawElements(g_VirtualScene[object_name].rendering_mode, g_VirtualScene[object_name].num_indices, GL_UNSIGNED_INT,
                  (void *)(g_VirtualScene[object_name].first_index * sizeof(GLuint)));

   glBindVertexArray(0);
}

void LoadShadersFromFiles() {

   GLuint vertex_shader_id   = LoadShader_Vertex("../../src/shader_vertex.glsl");
   GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

   if (g_GpuProgramID != 0)
      glDeleteProgram(g_GpuProgramID);

   g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

   g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model");
   g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view");
   g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection");
   g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id");
   g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
   g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

   glUseProgram(g_GpuProgramID);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Shark0"), 0);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Shark1"), 1);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Shark2"), 2);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Fish"), 3);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Sand"), 4);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Seaweed0"), 5);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Seaweed1"), 6);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Dolphin"), 7);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Whale"), 8);

   glUseProgram(0);
}

void PushMatrix(glm::mat4 M) { g_MatrixStack.push(M); }

void PopMatrix(glm::mat4 &M) {
   if (g_MatrixStack.empty()) {
      M = Matrix_Identity();
   } else {
      M = g_MatrixStack.top();
      g_MatrixStack.pop();
   }
}

void ComputeNormals(ObjModel *model) {
   if (!model->attrib.normals.empty())
      return;

   size_t num_vertices = model->attrib.vertices.size() / 3;

   std::vector<int> num_triangles_per_vertex(num_vertices, 0);
   std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

   for (size_t shape = 0; shape < model->shapes.size(); ++shape) {
      size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

      for (size_t triangle = 0; triangle < num_triangles; ++triangle) {
         assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

         glm::vec4 vertices[3];
         for (size_t vertex = 0; vertex < 3; ++vertex) {
            tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
            const float vx       = model->attrib.vertices[3 * idx.vertex_index + 0];
            const float vy       = model->attrib.vertices[3 * idx.vertex_index + 1];
            const float vz       = model->attrib.vertices[3 * idx.vertex_index + 2];
            vertices[vertex]     = glm::vec4(vx, vy, vz, 1.0);
         }

         const glm::vec4 a = vertices[0];
         const glm::vec4 b = vertices[1];
         const glm::vec4 c = vertices[2];

         const glm::vec4 n = crossproduct(b - a, c - a);

         for (size_t vertex = 0; vertex < 3; ++vertex) {
            tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
            num_triangles_per_vertex[idx.vertex_index] += 1;
            vertex_normals[idx.vertex_index] += n;
            model->shapes[shape].mesh.indices[3 * triangle + vertex].normal_index = idx.vertex_index;
         }
      }
   }

   model->attrib.normals.resize(3 * num_vertices);

   for (size_t i = 0; i < vertex_normals.size(); ++i) {
      glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
      n /= norm(n);
      model->attrib.normals[3 * i + 0] = n.x;
      model->attrib.normals[3 * i + 1] = n.y;
      model->attrib.normals[3 * i + 2] = n.z;
   }
}

void BuildTrianglesAndAddToVirtualScene(ObjModel *model) {
   GLuint vertex_array_object_id;
   glGenVertexArrays(1, &vertex_array_object_id);
   glBindVertexArray(vertex_array_object_id);

   std::vector<GLuint> indices;
   std::vector<float> model_coefficients;
   std::vector<float> normal_coefficients;
   std::vector<float> texture_coefficients;

   for (size_t shape = 0; shape < model->shapes.size(); ++shape) {
      size_t first_index   = indices.size();
      size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

      const float minval = std::numeric_limits<float>::min();
      const float maxval = std::numeric_limits<float>::max();

      glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
      glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

      for (size_t triangle = 0; triangle < num_triangles; ++triangle) {
         assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

         for (size_t vertex = 0; vertex < 3; ++vertex) {
            tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];

            indices.push_back(first_index + 3 * triangle + vertex);

            const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
            const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
            const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];

            model_coefficients.push_back(vx);
            model_coefficients.push_back(vy);
            model_coefficients.push_back(vz);
            model_coefficients.push_back(1.0f);

            bbox_min.x = std::min(bbox_min.x, vx);
            bbox_min.y = std::min(bbox_min.y, vy);
            bbox_min.z = std::min(bbox_min.z, vz);
            bbox_max.x = std::max(bbox_max.x, vx);
            bbox_max.y = std::max(bbox_max.y, vy);
            bbox_max.z = std::max(bbox_max.z, vz);

            if (idx.normal_index != -1) {
               const float nx = model->attrib.normals[3 * idx.normal_index + 0];
               const float ny = model->attrib.normals[3 * idx.normal_index + 1];
               const float nz = model->attrib.normals[3 * idx.normal_index + 2];
               normal_coefficients.push_back(nx);
               normal_coefficients.push_back(ny);
               normal_coefficients.push_back(nz);
               normal_coefficients.push_back(0.0f);
            }

            if (idx.texcoord_index != -1) {
               const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
               const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
               texture_coefficients.push_back(u);
               texture_coefficients.push_back(v);
            }
         }
      }

      size_t last_index = indices.size() - 1;

      SceneObject theobject;
      theobject.name                   = model->shapes[shape].name;
      theobject.first_index            = first_index;
      theobject.num_indices            = last_index - first_index + 1;
      theobject.rendering_mode         = GL_TRIANGLES;
      theobject.vertex_array_object_id = vertex_array_object_id;

      theobject.bbox_min = bbox_min;
      theobject.bbox_max = bbox_max;

      g_VirtualScene[model->shapes[shape].name] = theobject;
   }

   GLuint VBO_model_coefficients_id;
   glGenBuffers(1, &VBO_model_coefficients_id);
   glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
   glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
   GLuint location            = 0;
   GLint number_of_dimensions = 4;
   glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(location);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   if (!normal_coefficients.empty()) {
      GLuint VBO_normal_coefficients_id;
      glGenBuffers(1, &VBO_normal_coefficients_id);
      glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
      glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
      glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
      location             = 1;
      number_of_dimensions = 4;
      glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(location);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   if (!texture_coefficients.empty()) {
      GLuint VBO_texture_coefficients_id;
      glGenBuffers(1, &VBO_texture_coefficients_id);
      glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
      glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
      glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
      location             = 2;
      number_of_dimensions = 2;
      glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(location);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   GLuint indices_id;
   glGenBuffers(1, &indices_id);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
   glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

   glBindVertexArray(0);
}

GLuint LoadShader_Vertex(const char *filename) {

   GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

   LoadShader(filename, vertex_shader_id);

   return vertex_shader_id;
}

GLuint LoadShader_Fragment(const char *filename) {

   GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

   LoadShader(filename, fragment_shader_id);

   return fragment_shader_id;
}

void LoadShader(const char *filename, GLuint shader_id) {

   std::ifstream file;
   try {
      file.exceptions(std::ifstream::failbit);
      file.open(filename);
   } catch (std::exception &e) {
      fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
      std::exit(EXIT_FAILURE);
   }
   std::stringstream shader;
   shader << file.rdbuf();
   std::string str                  = shader.str();
   const GLchar *shader_string      = str.c_str();
   const GLint shader_string_length = static_cast<GLint>(str.length());

   glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

   glCompileShader(shader_id);

   GLint compiled_ok;
   glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

   GLint log_length = 0;
   glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

   GLchar *log = new GLchar[log_length];
   glGetShaderInfoLog(shader_id, log_length, &log_length, log);

   if (log_length != 0) {
      std::string output;

      if (!compiled_ok) {
         output += "ERROR: OpenGL compilation of \"";
         output += filename;
         output += "\" failed.\n";
         output += "== Start of compilation log\n";
         output += log;
         output += "== End of compilation log\n";
      } else {
         output += "WARNING: OpenGL compilation of \"";
         output += filename;
         output += "\".\n";
         output += "== Start of compilation log\n";
         output += log;
         output += "== End of compilation log\n";
      }

      fprintf(stderr, "%s", output.c_str());
   }

   delete[] log;
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id) {

   GLuint program_id = glCreateProgram();

   glAttachShader(program_id, vertex_shader_id);
   glAttachShader(program_id, fragment_shader_id);

   glLinkProgram(program_id);

   GLint linked_ok = GL_FALSE;
   glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

   if (linked_ok == GL_FALSE) {
      GLint log_length = 0;
      glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

      GLchar *log = new GLchar[log_length];

      glGetProgramInfoLog(program_id, log_length, &log_length, log);

      std::string output;

      output += "ERROR: OpenGL linking of program failed.\n";
      output += "== Start of link log\n";
      output += log;
      output += "\n== End of link log\n";

      delete[] log;

      fprintf(stderr, "%s", output.c_str());
   }

   glDeleteShader(vertex_shader_id);
   glDeleteShader(fragment_shader_id);

   return program_id;
}

void FramebufferSizeCallback(GLFWwindow *window, int width, int height) {

   glViewport(0, 0, width, height);

   g_ScreenRatio = (float)width / height;
}

double g_LastCursorPosX, g_LastCursorPosY;

void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {

   if (g_is_free_cam && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

      for (const auto &current_aabb: fish_list) {

         if (is_eatable[current_aabb] && !is_eated[current_aabb]) {

            if (current_aabb->get_type() == "Object_TexMap_0") {
               is_eated[current_aabb] = true;
               shark_p->update_aabb(shark_p->get_model() * Matrix_Scale(1.1f, 1.1f, 1.1f), g_VirtualScene["Object_TexMap_0"].bbox_min,
                                    g_VirtualScene["Object_TexMap_0"].bbox_max);
               s_speed += 0.1;
               g_radius += 0.2;

            } else {

               is_eated[current_aabb] = true;
               shark_p->update_aabb(shark_p->get_model() * Matrix_Scale(1.3f, 1.3f, 1.3f), g_VirtualScene["Object_TexMap_0"].bbox_min,
                                    g_VirtualScene["Object_TexMap_0"].bbox_max);
               s_speed += 0.2;
               g_radius += 0.4;
            }
         }
      }

      if (is_eatable_whale) {

         is_eated_whale = true;
         shark_p->update_aabb(shark_p->get_model() * Matrix_Scale(1.7f, 1.7f, 1.7f), g_VirtualScene["Object_TexMap_0"].bbox_min,
                              g_VirtualScene["Object_TexMap_0"].bbox_max);
         s_speed += 5;
         g_radius += 5;
      }
      glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
      g_LeftMouseButtonPressed = true;
   }

   if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {

      g_LeftMouseButtonPressed = false;
   }
   if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {

      glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
      g_RightMouseButtonPressed = true;
   }
   if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {

      g_RightMouseButtonPressed = false;
   }
   if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {

      glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
      g_MiddleMouseButtonPressed = true;
   }
   if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {

      g_MiddleMouseButtonPressed = false;
   }
}

void CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {

   float dx = xpos - g_LastCursorPosX;
   float dy = ypos - g_LastCursorPosY;

   g_CameraTheta -= 0.01f * dx;
   g_CameraPhi += 0.01f * dy;

   float phimax = 3.141592f / 2;
   float phimin = -phimax;

   if (g_CameraPhi > phimax)
      g_CameraPhi = phimax;

   if (g_CameraPhi < phimin)
      g_CameraPhi = phimin;

   g_LastCursorPosX = xpos;
   g_LastCursorPosY = ypos;
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {

   g_CameraDistance -= 0.1f * yoffset;

   const float verysmallnumber = std::numeric_limits<float>::epsilon();
   if (g_CameraDistance < verysmallnumber)
      g_CameraDistance = verysmallnumber;
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod) {

   for (int i = 0; i < 10; ++i)
      if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
         std::exit(100 + i);

   if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GL_TRUE);


   if (key == GLFW_KEY_H && action == GLFW_PRESS) {
      g_ShowInfoText = !g_ShowInfoText;
   }

   if (key == GLFW_KEY_R && action == GLFW_PRESS) {
      LoadShadersFromFiles();
      fprintf(stdout, "Shaders recarregados!\n");
      fflush(stdout);
   }
   if (key == GLFW_KEY_D) {

      if (action == GLFW_PRESS)
         g_D_pressed = true;

      else if (action == GLFW_RELEASE)
         g_D_pressed = false;

      else if (action == GLFW_REPEAT)
         ;
   }
   if (key == GLFW_KEY_S) {

      if (action == GLFW_PRESS)
         g_S_pressed = true;

      else if (action == GLFW_RELEASE)
         g_S_pressed = false;

      else if (action == GLFW_REPEAT)
         ;
   }
   if (key == GLFW_KEY_A) {

      if (action == GLFW_PRESS)
         g_A_pressed = true;

      else if (action == GLFW_RELEASE)
         g_A_pressed = false;

      else if (action == GLFW_REPEAT)
         ;
   }
   if (key == GLFW_KEY_W) {

      if (action == GLFW_PRESS)
         g_W_pressed = true;

      else if (action == GLFW_RELEASE)
         g_W_pressed = false;

      else if (action == GLFW_REPEAT)
         ;
   }
   if (key == GLFW_KEY_LEFT_CONTROL) {

      if (action == GLFW_PRESS)
         g_LCTRL_pressed = true;

      else if (action == GLFW_RELEASE)
         g_LCTRL_pressed = false;

      else if (action == GLFW_REPEAT)
         ;
   }


   if (key == GLFW_KEY_LEFT_SHIFT) {

      if (action == GLFW_PRESS)
         g_LSHIFT_pressed = true;

      else if (action == GLFW_RELEASE)
         g_LSHIFT_pressed = false;

      else if (action == GLFW_REPEAT)
         ;
   }

   if (key == GLFW_KEY_E && action == GLFW_PRESS) {
      g_is_free_cam = !g_is_free_cam;

      if (!g_is_free_cam) {
         g_camera_lookat_l    = shark_p->get_center_point();
         g_camera_position_c  = glm::vec4(10.0f, 0.0f, 0.0f, 0.0f) + g_camera_lookat_l;
         g_camera_view_vector = g_camera_lookat_l - g_camera_position_c;

      } else {
         const float DISTANCE_FACTOR = 2.0f;
         glm::vec4 forward_direction = glm::vec4(-shark_p->get_model()[0].x, -shark_p->get_model()[0].y, -shark_p->get_model()[0].z, 0.0f);
         forward_direction           = forward_direction / norm(forward_direction);
         g_camera_position_c         = shark_p->get_center_point() + forward_direction * DISTANCE_FACTOR;
      }
   }
}

void ErrorCallback(int error, const char *description) { fprintf(stderr, "ERROR: GLFW: %s\n", description); }

void TextRendering_ShowFramesPerSecond(GLFWwindow *window) {
   if (!g_ShowInfoText)
      return;

   static float old_seconds   = (float)glfwGetTime();
   static int ellapsed_frames = 0;
   static char buffer[20]     = "?? fps";
   static int numchars        = 7;

   ellapsed_frames += 1;

   float seconds = (float)glfwGetTime();

   float ellapsed_seconds = seconds - old_seconds;

   if (ellapsed_seconds > 1.0f) {
      numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

      old_seconds     = seconds;
      ellapsed_frames = 0;
   }

   float lineheight = TextRendering_LineHeight(window);
   float charwidth  = TextRendering_CharWidth(window);

   TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1) * charwidth, 1.0f - lineheight, 1.0f);
}


//FONTE: As duas funções a seguir foram pedidas para o ChatGPT fazer.
void create_fishes() {

   int fish_id = 0;

   for (int i = 0; i < 100; ++i) {
      float x        = -80.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (160.0f)));
      float y        = -9.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (11.0f)));
      float z        = -80.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (160.0f)));
      float rotation = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f * M_PI)));

      glm::mat4 model = Matrix_Translate(x, y, z) * Matrix_Rotate_Y(rotation) * Matrix_Scale(0.3f, 0.3f, 0.3f);
      AABB *aabb_fish = new AABB(g_VirtualScene["Object_BlueTaT.jpg"].bbox_min, g_VirtualScene["Object_BlueTaT.jpg"].bbox_max, model, fish_id,
                                 "Object_BlueTaT.jpg");

      is_eatable[aabb_fish] = false;
      is_eated[aabb_fish]   = false;
      fish_list.push_back(aabb_fish);
      ++fish_id;
   }

   for (int i = 0; i < 40; ++i) {
      float x        = -80.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (160.0f)));
      float y        = -9.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (11.0f)));
      float z        = -80.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (160.0f)));
      float rotation = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f * M_PI)));

      glm::mat4 model = Matrix_Identity();
      AABB *aabb_fish = new AABB(g_VirtualScene["dolphin"].bbox_min, g_VirtualScene["dolphin"].bbox_max, model, fish_id, "dolphin");
      model           = Matrix_Translate(x, y, z) * Matrix_Rotate_Y(rotation) * Matrix_Scale(1.0f, 1.0f, 1.0f);
      aabb_fish->update_aabb(model, g_VirtualScene["dolphin"].bbox_min, g_VirtualScene["dolphin"].bbox_max);

      is_eatable[aabb_fish] = false;
      is_eated[aabb_fish]   = false;
      fish_list.push_back(aabb_fish);
      ++fish_id;
   }
}
void create_seaweed_models() {
   seaweeds_models.push_back(Matrix_Translate(-45.3f, -9.0f, -23.8f));
   seaweeds_models.push_back(Matrix_Translate(10.7f, -9.0f, -19.5f));
   seaweeds_models.push_back(Matrix_Translate(-63.2f, -9.0f, 15.9f));
   seaweeds_models.push_back(Matrix_Translate(25.0f, -9.0f, 28.4f));
   seaweeds_models.push_back(Matrix_Translate(-72.8f, -9.0f, -4.1f));
   seaweeds_models.push_back(Matrix_Translate(55.3f, -9.0f, -11.2f));
   seaweeds_models.push_back(Matrix_Translate(-37.1f, -9.0f, 8.3f));
   seaweeds_models.push_back(Matrix_Translate(18.5f, -9.0f, -32.6f));
   seaweeds_models.push_back(Matrix_Translate(-22.9f, -9.0f, 22.7f));
   seaweeds_models.push_back(Matrix_Translate(34.6f, -9.0f, 5.5f));
   seaweeds_models.push_back(Matrix_Translate(-54.3f, -9.0f, -9.0f));
   seaweeds_models.push_back(Matrix_Translate(60.4f, -9.0f, 37.2f));
   seaweeds_models.push_back(Matrix_Translate(-19.4f, -9.0f, -30.5f));
   seaweeds_models.push_back(Matrix_Translate(7.8f, -9.0f, 12.1f));
   seaweeds_models.push_back(Matrix_Translate(-42.6f, -9.0f, -11.4f));
   seaweeds_models.push_back(Matrix_Translate(50.7f, -9.0f, -25.6f));
   seaweeds_models.push_back(Matrix_Translate(-66.8f, -9.0f, 31.9f));
   seaweeds_models.push_back(Matrix_Translate(13.2f, -9.0f, -40.0f));
   seaweeds_models.push_back(Matrix_Translate(-75.5f, -9.0f, 13.8f));
   seaweeds_models.push_back(Matrix_Translate(39.9f, -9.0f, -2.3f));
   seaweeds_models.push_back(Matrix_Translate(-31.0f, -9.0f, 8.9f));
   seaweeds_models.push_back(Matrix_Translate(27.4f, -9.0f, 27.1f));
   seaweeds_models.push_back(Matrix_Translate(-52.1f, -9.0f, -29.4f));
   seaweeds_models.push_back(Matrix_Translate(65.0f, -9.0f, 9.8f));
   seaweeds_models.push_back(Matrix_Translate(-20.1f, -9.0f, -16.7f));
   seaweeds_models.push_back(Matrix_Translate(12.5f, -9.0f, -37.3f));
   seaweeds_models.push_back(Matrix_Translate(-33.3f, -9.0f, 25.4f));
   seaweeds_models.push_back(Matrix_Translate(48.2f, -9.0f, -18.6f));
   seaweeds_models.push_back(Matrix_Translate(-67.9f, -9.0f, 14.4f));
   seaweeds_models.push_back(Matrix_Translate(31.7f, -9.0f, 32.8f));
   seaweeds_models.push_back(Matrix_Translate(-57.8f, -9.0f, -23.5f));
   seaweeds_models.push_back(Matrix_Translate(22.6f, -9.0f, -26.9f));
   seaweeds_models.push_back(Matrix_Translate(-43.4f, -9.0f, 29.3f));
   seaweeds_models.push_back(Matrix_Translate(59.9f, -9.0f, 20.7f));
   seaweeds_models.push_back(Matrix_Translate(-29.5f, -9.0f, -8.2f));
   seaweeds_models.push_back(Matrix_Translate(8.9f, -9.0f, 33.4f));
   seaweeds_models.push_back(Matrix_Translate(-65.7f, -9.0f, 7.9f));
   seaweeds_models.push_back(Matrix_Translate(42.0f, -9.0f, -10.4f));
   seaweeds_models.push_back(Matrix_Translate(-71.4f, -9.0f, -32.8f));
   seaweeds_models.push_back(Matrix_Translate(20.4f, -9.0f, 24.2f));
   seaweeds_models.push_back(Matrix_Translate(-48.6f, -9.0f, 14.6f));
   seaweeds_models.push_back(Matrix_Translate(53.8f, -9.0f, 17.9f));
   seaweeds_models.push_back(Matrix_Translate(-28.2f, -9.0f, -35.7f));
   seaweeds_models.push_back(Matrix_Translate(9.1f, -9.0f, -1.7f));
   seaweeds_models.push_back(Matrix_Translate(-61.3f, -9.0f, 21.6f));
   seaweeds_models.push_back(Matrix_Translate(30.8f, -9.0f, -19.2f));
   seaweeds_models.push_back(Matrix_Translate(-77.2f, -9.0f, -2.6f));
   seaweeds_models.push_back(Matrix_Translate(37.5f, -9.0f, 35.3f));
   seaweeds_models.push_back(Matrix_Translate(-40.1f, -9.0f, -30.8f));
   seaweeds_models.push_back(Matrix_Translate(62.4f, -9.0f, 22.1f));
   seaweeds_models.push_back(Matrix_Translate(-24.7f, -9.0f, 0.6f));
   seaweeds_models.push_back(Matrix_Translate(21.3f, -9.0f, -11.5f));
   seaweeds_models.push_back(Matrix_Translate(-39.8f, -9.0f, 33.3f));
   seaweeds_models.push_back(Matrix_Translate(11.4f, -9.0f, -29.7f));
   seaweeds_models.push_back(Matrix_Translate(-52.0f, -9.0f, -14.9f));
   seaweeds_models.push_back(Matrix_Translate(64.3f, -9.0f, 5.2f));
   seaweeds_models.push_back(Matrix_Translate(-27.4f, -9.0f, -40.0f));
   seaweeds_models.push_back(Matrix_Translate(29.9f, -9.0f, 18.1f));
   seaweeds_models.push_back(Matrix_Translate(-72.1f, -9.0f, 9.0f));
   seaweeds_models.push_back(Matrix_Translate(56.1f, -9.0f, -30.6f));
   seaweeds_models.push_back(Matrix_Translate(-43.6f, -9.0f, -8.8f));
   seaweeds_models.push_back(Matrix_Translate(32.7f, -9.0f, -28.5f));
   seaweeds_models.push_back(Matrix_Translate(-60.9f, -9.0f, 25.7f));
   seaweeds_models.push_back(Matrix_Translate(12.0f, -9.0f, 15.9f));
   seaweeds_models.push_back(Matrix_Translate(-66.5f, -9.0f, -27.2f));
   seaweeds_models.push_back(Matrix_Translate(45.6f, -9.0f, 6.3f));
   seaweeds_models.push_back(Matrix_Translate(-53.3f, -9.0f, 16.0f));
   seaweeds_models.push_back(Matrix_Translate(41.2f, -9.0f, -5.0f));
   seaweeds_models.push_back(Matrix_Translate(-68.1f, -9.0f, -12.1f));
   seaweeds_models.push_back(Matrix_Translate(19.7f, -9.0f, 35.2f));
   seaweeds_models.push_back(Matrix_Translate(-36.0f, -9.0f, -29.4f));
   seaweeds_models.push_back(Matrix_Translate(50.5f, -9.0f, 25.0f));
   seaweeds_models.push_back(Matrix_Translate(-71.8f, -9.0f, -4.5f));
   seaweeds_models.push_back(Matrix_Translate(7.2f, -9.0f, -31.8f));
   seaweeds_models.push_back(Matrix_Translate(-47.4f, -9.0f, 9.4f));
   seaweeds_models.push_back(Matrix_Translate(54.6f, -9.0f, 32.7f));
   seaweeds_models.push_back(Matrix_Translate(-29.3f, -9.0f, -13.9f));
   seaweeds_models.push_back(Matrix_Translate(63.9f, -9.0f, 8.5f));
   seaweeds_models.push_back(Matrix_Translate(-56.2f, -9.0f, 26.3f));
   seaweeds_models.push_back(Matrix_Translate(23.5f, -9.0f, -15.4f));
   seaweeds_models.push_back(Matrix_Translate(-34.7f, -9.0f, 12.6f));
   seaweeds_models.push_back(Matrix_Translate(9.6f, -9.0f, -23.1f));
   seaweeds_models.push_back(Matrix_Translate(-69.0f, -9.0f, 21.0f));
   seaweeds_models.push_back(Matrix_Translate(16.8f, -9.0f, -32.4f));
   seaweeds_models.push_back(Matrix_Translate(-77.5f, -9.0f, -15.2f));
   seaweeds_models.push_back(Matrix_Translate(35.1f, -9.0f, 14.8f));
   seaweeds_models.push_back(Matrix_Translate(-25.9f, -9.0f, -26.3f));
   seaweeds_models.push_back(Matrix_Translate(44.8f, -9.0f, -4.7f));
   seaweeds_models.push_back(Matrix_Translate(-61.4f, -9.0f, 11.5f));
   seaweeds_models.push_back(Matrix_Translate(12.9f, -9.0f, 22.8f));
   seaweeds_models.push_back(Matrix_Translate(-70.3f, -9.0f, -35.6f));
   seaweeds_models.push_back(Matrix_Translate(28.1f, -9.0f, 37.1f));
   seaweeds_models.push_back(Matrix_Translate(-52.8f, -9.0f, -30.9f));
   seaweeds_models.push_back(Matrix_Translate(39.4f, -9.0f, -14.8f));
   seaweeds_models.push_back(Matrix_Translate(-64.0f, -9.0f, 5.9f));
   seaweeds_models.push_back(Matrix_Translate(45.1f, -9.0f, 9.7f));
   seaweeds_models.push_back(Matrix_Translate(-37.9f, -9.0f, 28.5f));
   seaweeds_models.push_back(Matrix_Translate(22.0f, -9.0f, -16.4f));
   seaweeds_models.push_back(Matrix_Translate(-50.4f, -9.0f, 33.1f));
   seaweeds_models.push_back(Matrix_Translate(58.3f, -9.0f, 25.9f));
   seaweeds_models.push_back(Matrix_Translate(-32.5f, -9.0f, -31.0f));
   seaweeds_models.push_back(Matrix_Translate(7.1f, -9.0f, 5.6f));
   seaweeds_models.push_back(Matrix_Translate(-74.9f, -9.0f, -6.3f));
   seaweeds_models.push_back(Matrix_Translate(52.7f, -9.0f, -22.4f));
   seaweeds_models.push_back(Matrix_Translate(-61.9f, -9.0f, 19.2f));
   seaweeds_models.push_back(Matrix_Translate(29.1f, -9.0f, 36.4f));
   seaweeds_models.push_back(Matrix_Translate(-25.1f, -9.0f, -24.5f));
   seaweeds_models.push_back(Matrix_Translate(32.9f, -9.0f, -7.6f));
   seaweeds_models.push_back(Matrix_Translate(-53.6f, -9.0f, 22.4f));
   seaweeds_models.push_back(Matrix_Translate(38.8f, -9.0f, 18.2f));
   seaweeds_models.push_back(Matrix_Translate(-69.7f, -9.0f, -16.5f));
   seaweeds_models.push_back(Matrix_Translate(46.2f, -9.0f, -0.2f));
   seaweeds_models.push_back(Matrix_Translate(-78.4f, -9.0f, 29.6f));
   seaweeds_models.push_back(Matrix_Translate(14.7f, -9.0f, -14.3f));
   seaweeds_models.push_back(Matrix_Translate(-31.2f, -9.0f, -8.0f));
   seaweeds_models.push_back(Matrix_Translate(50.3f, -9.0f, -12.9f));
   seaweeds_models.push_back(Matrix_Translate(-65.0f, -9.0f, 12.3f));
   seaweeds_models.push_back(Matrix_Translate(28.3f, -9.0f, 22.6f));
   seaweeds_models.push_back(Matrix_Translate(-41.5f, -9.0f, -29.1f));
   seaweeds_models.push_back(Matrix_Translate(57.4f, -9.0f, -5.6f));
   seaweeds_models.push_back(Matrix_Translate(-72.3f, -9.0f, 26.7f));
   seaweeds_models.push_back(Matrix_Translate(15.0f, -9.0f, -27.2f));
   seaweeds_models.push_back(Matrix_Translate(-46.1f, -9.0f, 19.8f));
   seaweeds_models.push_back(Matrix_Translate(36.0f, -9.0f, 7.4f));
   seaweeds_models.push_back(Matrix_Translate(-27.8f, -9.0f, -40.0f));
   seaweeds_models.push_back(Matrix_Translate(41.7f, -9.0f, -30.3f));
   seaweeds_models.push_back(Matrix_Translate(-68.2f, -9.0f, 5.3f));
   seaweeds_models.push_back(Matrix_Translate(23.6f, -9.0f, 33.7f));
   seaweeds_models.push_back(Matrix_Translate(-29.4f, -9.0f, -19.8f));
   seaweeds_models.push_back(Matrix_Translate(62.1f, -9.0f, 4.0f));
   seaweeds_models.push_back(Matrix_Translate(-47.7f, -9.0f, 30.9f));
   seaweeds_models.push_back(Matrix_Translate(53.9f, -9.0f, -27.5f));
   seaweeds_models.push_back(Matrix_Translate(-56.4f, -9.0f, -11.3f));
   seaweeds_models.push_back(Matrix_Translate(26.5f, -9.0f, -5.0f));
   seaweeds_models.push_back(Matrix_Translate(-78.0f, -9.0f, 11.4f));
   seaweeds_models.push_back(Matrix_Translate(14.2f, -9.0f, -37.6f));
   seaweeds_models.push_back(Matrix_Translate(-40.0f, -9.0f, 25.1f));
   seaweeds_models.push_back(Matrix_Translate(52.4f, -9.0f, -10.1f));
   seaweeds_models.push_back(Matrix_Translate(-65.5f, -9.0f, 19.9f));
   seaweeds_models.push_back(Matrix_Translate(32.4f, -9.0f, 2.8f));
   seaweeds_models.push_back(Matrix_Translate(-23.0f, -9.0f, -32.3f));
   seaweeds_models.push_back(Matrix_Translate(45.8f, -9.0f, -21.0f));
   seaweeds_models.push_back(Matrix_Translate(-30.6f, -9.0f, 6.0f));
   seaweeds_models.push_back(Matrix_Translate(60.7f, -9.0f, 27.0f));
   seaweeds_models.push_back(Matrix_Translate(-67.4f, -9.0f, -35.0f));
   seaweeds_models.push_back(Matrix_Translate(24.0f, -9.0f, -14.0f));
   seaweeds_models.push_back(Matrix_Translate(-43.7f, -9.0f, -5.4f));
   seaweeds_models.push_back(Matrix_Translate(33.1f, -9.0f, 36.0f));
   seaweeds_models.push_back(Matrix_Translate(-71.2f, -9.0f, 4.7f));
   seaweeds_models.push_back(Matrix_Translate(46.8f, -9.0f, 15.3f));
   seaweeds_models.push_back(Matrix_Translate(-53.9f, -9.0f, -26.0f));
   seaweeds_models.push_back(Matrix_Translate(27.7f, -9.0f, -22.1f));
   seaweeds_models.push_back(Matrix_Translate(-29.0f, -9.0f, -9.6f));
   seaweeds_models.push_back(Matrix_Translate(49.5f, -9.0f, 11.2f));
   seaweeds_models.push_back(Matrix_Translate(-77.3f, -9.0f, 18.6f));
   seaweeds_models.push_back(Matrix_Translate(18.7f, -9.0f, -27.4f));
   seaweeds_models.push_back(Matrix_Translate(-61.1f, -9.0f, -7.9f));
   seaweeds_models.push_back(Matrix_Translate(32.8f, -9.0f, -19.1f));
   seaweeds_models.push_back(Matrix_Translate(-66.3f, -9.0f, 25.8f));
   seaweeds_models.push_back(Matrix_Translate(55.4f, -9.0f, -31.6f));
   seaweeds_models.push_back(Matrix_Translate(-36.2f, -9.0f, 12.7f));
   seaweeds_models.push_back(Matrix_Translate(41.9f, -9.0f, 27.8f));
   seaweeds_models.push_back(Matrix_Translate(-74.1f, -9.0f, -22.7f));
   seaweeds_models.push_back(Matrix_Translate(21.4f, -9.0f, -11.8f));
   seaweeds_models.push_back(Matrix_Translate(-52.9f, -9.0f, 6.9f));
   seaweeds_models.push_back(Matrix_Translate(60.5f, -9.0f, 22.3f));
   seaweeds_models.push_back(Matrix_Translate(-28.0f, -9.0f, -34.1f));
   seaweeds_models.push_back(Matrix_Translate(54.0f, -9.0f, -9.3f));
   seaweeds_models.push_back(Matrix_Translate(-41.8f, -9.0f, 18.4f));
   seaweeds_models.push_back(Matrix_Translate(37.6f, -9.0f, -3.0f));
   seaweeds_models.push_back(Matrix_Translate(-68.0f, -9.0f, 24.8f));
   seaweeds_models.push_back(Matrix_Translate(48.4f, -9.0f, -13.5f));
   seaweeds_models.push_back(Matrix_Translate(-59.3f, -9.0f, -26.8f));
   seaweeds_models.push_back(Matrix_Translate(17.3f, -9.0f, 31.4f));
   seaweeds_models.push_back(Matrix_Translate(-33.0f, -9.0f, -9.8f));
   seaweeds_models.push_back(Matrix_Translate(56.7f, -9.0f, 30.1f));
   seaweeds_models.push_back(Matrix_Translate(-25.5f, -9.0f, 12.8f));
   seaweeds_models.push_back(Matrix_Translate(61.1f, -9.0f, -15.7f));
   seaweeds_models.push_back(Matrix_Translate(-72.6f, -9.0f, -10.2f));
   seaweeds_models.push_back(Matrix_Translate(10.9f, -9.0f, 20.4f));
   seaweeds_models.push_back(Matrix_Translate(-45.7f, -9.0f, -32.9f));
   seaweeds_models.push_back(Matrix_Translate(49.1f, -9.0f, 19.5f));
   seaweeds_models.push_back(Matrix_Translate(-28.7f, -9.0f, 4.2f));
   seaweeds_models.push_back(Matrix_Translate(31.3f, -9.0f, -16.1f));
   seaweeds_models.push_back(Matrix_Translate(-60.0f, -9.0f, -5.1f));
   seaweeds_models.push_back(Matrix_Translate(26.2f, -9.0f, 28.3f));
   seaweeds_models.push_back(Matrix_Translate(-68.7f, -9.0f, -11.6f));
   seaweeds_models.push_back(Matrix_Translate(58.9f, -9.0f, 14.5f));
   seaweeds_models.push_back(Matrix_Translate(-52.4f, -9.0f, -35.0f));
   seaweeds_models.push_back(Matrix_Translate(21.9f, -9.0f, 6.7f));
   seaweeds_models.push_back(Matrix_Translate(-67.1f, -9.0f, 28.6f));
   seaweeds_models.push_back(Matrix_Translate(12.6f, -9.0f, -20.9f));
}

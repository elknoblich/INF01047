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
void PrintObjModelInfo(ObjModel *);

void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow *window);
float TextRendering_CharWidth(GLFWwindow *window);
void TextRendering_PrintString(GLFWwindow *window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow *window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow *window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

void TextRendering_ShowModelViewProjection(GLFWwindow *window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow *window);
void TextRendering_ShowProjection(GLFWwindow *window);
void TextRendering_ShowFramesPerSecond(GLFWwindow *window);

void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

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
bool g_freeCam                  = true;

float g_CameraTheta    = 0.0f;
float g_CameraPhi      = 0.0f;
float g_CameraDistance = 3.5f;

float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

bool g_UsePerspectiveProjection = true;

bool g_ShowInfoText = true;

GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

GLuint g_NumLoadedTextures = 0;

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

   LoadTextureImage("../../data/grassy_cobblestone_diff_4k.jpg");
   LoadTextureImage("../../data/grama_text.jpg");
   //Shark Textures:
   LoadTextureImage("../../data/texture.jpg");
   LoadTextureImage("../../data/texture_N.jpg");
   LoadTextureImage("../../data/texture_S.jpg");


   // Obj Load:
   ObjModel spheremodel("../../data/sphere.obj");
   ComputeNormals(&spheremodel);
   BuildTrianglesAndAddToVirtualScene(&spheremodel);

   ObjModel bunnymodel("../../data/bunny.obj");
   ComputeNormals(&bunnymodel);
   BuildTrianglesAndAddToVirtualScene(&bunnymodel);

   ObjModel planemodel("../../data/plane.obj");
   ComputeNormals(&planemodel);
   BuildTrianglesAndAddToVirtualScene(&planemodel);

   ObjModel cubemodel("../../data/cube.obj");
   ComputeNormals(&cubemodel);
   BuildTrianglesAndAddToVirtualScene(&cubemodel);

   ObjModel buttonmodel("../../data/model.obj");
   ComputeNormals(&buttonmodel);
   BuildTrianglesAndAddToVirtualScene(&buttonmodel);

   ObjModel sharkmodel("../../data/shark.obj");
   ComputeNormals(&sharkmodel);
   BuildTrianglesAndAddToVirtualScene(&sharkmodel);


   if (argc > 1) {
      ObjModel model(argv[1]);
      BuildTrianglesAndAddToVirtualScene(&model);
   }

   TextRendering_Init();

   glEnable(GL_DEPTH_TEST);

   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glFrontFace(GL_CCW);

   float prev_time             = (float)glfwGetTime();
   float speed                 = 0.5f;
   glm::vec4 camera_position_c = glm::vec4(-0.75f, 0.75f, 0.75f, 1.0f);
   glm::vec4 camera_aabb_size  = glm::vec4(0.8f, 0.9f, 0.8f, 0.0f);
   AABB cam_aabb(camera_position_c, camera_aabb_size, 0);                                     // Colisão com objetos
   SPHERE interaction_sphere(camera_position_c, 2.0f, -1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)); // Interação com objetos
   std::map<AABB, bool> cam_collision_map;
   std::list<AABB> static_objects_list;

   int current_sobj_id = 0;

   glm::mat4 model = Matrix_Identity();

   model = Matrix_Translate(-80.0f, 1.0f, 0.0f) * Matrix_Rotate_Y(3.1415f / 2.0f) * Matrix_Scale(1.0f, 1.0f, 1.0f);
   AABB aabb_shark(g_VirtualScene["Object_TexMap_0"].bbox_min, g_VirtualScene["Object_TexMap_0"].bbox_max, model, current_sobj_id, "Object_TexMap_0");
   ++current_sobj_id;

   //FP arena
   model = Matrix_Translate(10.0f, -0.6f, 0.0f) * Matrix_Scale(0.03f, 1.0f, 10.0f);
   AABB aabb_cube1(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   cam_collision_map[aabb_cube1] = false;
   static_objects_list.push_back(aabb_cube1);
   ++current_sobj_id;

   model = Matrix_Translate(-10.0f, -0.6f, 0.0f) * Matrix_Scale(0.03f, 1.0f, 10.0f);
   AABB aabb_cube2(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   cam_collision_map[aabb_cube2] = false;
   static_objects_list.push_back(aabb_cube2);
   ++current_sobj_id;

   model = Matrix_Translate(0.0f, -0.6f, 10.0f) * Matrix_Scale(10.0f, 1.0f, 0.03f);
   AABB aabb_cube3(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   cam_collision_map[aabb_cube3] = false;
   static_objects_list.push_back(aabb_cube3);
   ++current_sobj_id;

   model = Matrix_Translate(0.0f, -0.6f, -10.0f) * Matrix_Scale(10.0f, 1.0f, 0.03f);
   AABB aabb_cube4(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   cam_collision_map[aabb_cube4] = false;
   static_objects_list.push_back(aabb_cube4);
   ++current_sobj_id;


   //Buttons
   model = Matrix_Translate(-8.0f, 0.7f, 0.0f) * Matrix_Rotate_Y(3.1415 / 2.0f) * Matrix_Scale(0.001f, 0.001f, 0.001f);
   AABB aabb_button(g_VirtualScene["object_0"].bbox_min, g_VirtualScene["object_0"].bbox_max, model, current_sobj_id, "object_0");
   static_objects_list.push_back(aabb_button);
   cam_collision_map[aabb_button] = false;

   model = Matrix_Translate(-8.0f, 0.7f, 2.0f) * Matrix_Rotate_Y(3.1415 / 2.0f) * Matrix_Scale(0.001f, 0.001f, 0.001f);
   AABB aabb_button2(g_VirtualScene["object_0"].bbox_min, g_VirtualScene["object_0"].bbox_max, model, current_sobj_id, "object_0");
   static_objects_list.push_back(aabb_button2);
   cam_collision_map[aabb_button2] = false;
   ++current_sobj_id;

   model = Matrix_Translate(-8.0f, 0.7f, -2.0f) * Matrix_Rotate_Y(3.1415 / 2.0f) * Matrix_Scale(0.001f, 0.001f, 0.001f);
   AABB aabb_button3(g_VirtualScene["object_0"].bbox_min, g_VirtualScene["object_0"].bbox_max, model, current_sobj_id, "object_0");
   static_objects_list.push_back(aabb_button3);
   cam_collision_map[aabb_button3] = false;
   ++current_sobj_id;

   //Shark Arena Walls
   std::list<AABB> shark_arena_walls;
   model = Matrix_Translate(-80.0f, -10.0f, 0.0f) * Matrix_Scale(40.f, 0.1f, 10.0f);
   AABB aabb_wall_1(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   shark_arena_walls.push_back(aabb_wall_1);
   ++current_sobj_id;

   model = Matrix_Translate(-80.0f, 0.0f, 10.0f) * Matrix_Rotate_X(3.1415f / 2.0f) * Matrix_Scale(40.f, 0.1f, 10.0f);
   AABB aabb_wall_2(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   shark_arena_walls.push_back(aabb_wall_2);
   ++current_sobj_id;

   model = Matrix_Translate(-80.0f, 0.0f, -10.0f) * Matrix_Rotate_X(3.1415f / 2.0f) * Matrix_Scale(40.f, 0.1f, 10.0f);
   AABB aabb_wall_3(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   shark_arena_walls.push_back(aabb_wall_3);
   ++current_sobj_id;

   model = Matrix_Translate(-90.0f, 0.0f, 0.0f) * Matrix_Rotate_Z(3.1415f / 2.0f) * Matrix_Scale(10.f, 0.1f, 10.0f);
   AABB aabb_wall_4(g_VirtualScene["Cube"].bbox_min, g_VirtualScene["Cube"].bbox_max, model, current_sobj_id, "Cube");
   shark_arena_walls.push_back(aabb_wall_4);
   ++current_sobj_id;

   bool shark_mode = false;
   float t_first[current_sobj_id];
   float t_last[current_sobj_id];

   glm::vec4 camera_lookat_l = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
   //glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
   glm::vec4 camera_view_vector = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f); // Vetor "view", sentido para onde a câmera está virada
   glm::vec4 camera_up_vector   = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
   float shark_rotation         = 0;
   while (!glfwWindowShouldClose(window)) {

      g_LeftMouseButtonClicked = false;

      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glUseProgram(g_GpuProgramID);

      float r = g_CameraDistance;
      float y = r * sin(g_CameraPhi);
      float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
      float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);

      glm::vec4 velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

      if (!shark_mode) {
         float current_time = (float)glfwGetTime();

         camera_view_vector = glm::vec4(x, -y, z, 0.0f);

         float delta_t = current_time - prev_time;
         prev_time     = current_time;

         glm::vec4 w_vector = -camera_view_vector / norm(camera_view_vector);
         w_vector.y         = 0.0f;
         glm::vec4 u_vector = crossproduct(camera_up_vector, w_vector) / norm(crossproduct(camera_up_vector, w_vector));
         u_vector.y         = 0.0f;

         w_vector = w_vector / norm(w_vector);
         u_vector = u_vector / norm(u_vector);

         if (g_W_pressed) {
            velocity += -w_vector;
         }

         if (g_A_pressed) {
            velocity += -u_vector;
         }

         if (g_D_pressed) {
            velocity += u_vector;
         }

         if (g_S_pressed) {
            velocity += w_vector;
         }

         if (velocity.x != 0 || velocity.z != 0) {
            velocity = velocity / norm(velocity);
         }

         int count = 0;
         for (const auto &current_aabb: static_objects_list) {
            if (cam_collision_map[current_aabb]) {

               camera_position_c += velocity * t_first[count] * delta_t;

               glm::vec4 collision_normal = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
               for (int i: {0, 2}) {

                  if (velocity[i] > 0.0f && camera_position_c[i] + cam_aabb.get_max()[i] > current_aabb.get_min()[i]) {
                     collision_normal[i] = -1.0f;

                  } else if (velocity[i] < 0.0f && camera_position_c[i] + cam_aabb.get_min()[i] < current_aabb.get_max()[i]) {

                     collision_normal[i] = 1.0f;
                  }
               }

               velocity = velocity - 2.0f * collision_normal * dotproduct(velocity, collision_normal);
               camera_position_c += collision_normal * (1.0f - t_first[count]) * delta_t;
               camera_position_c += velocity * (1.0f - t_first[count]) * delta_t;


            } else {

               camera_position_c += velocity * speed * delta_t;
            }

            ++count;
         }

         cam_aabb.update_aabb(camera_position_c, camera_aabb_size);
         interaction_sphere.update_sphere(camera_position_c, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

      } else if (shark_mode) {

         camera_lookat_l    = aabb_shark.get_center_point();
         camera_position_c  = glm::vec4(10.0f, 0.0f, 0.0f, 0.0f) + camera_lookat_l;
         camera_view_vector = camera_lookat_l - camera_position_c;

         float current_time = (float)glfwGetTime();
         float delta_t      = current_time - prev_time;
         prev_time          = current_time;


         const float ROTATION_CONST      = 0.523599;
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
            aabb_shark.update_aabb(aabb_shark.get_model() * shark_rotation_matrix, g_VirtualScene["Object_TexMap_0"].bbox_min,
                                   g_VirtualScene["Object_TexMap_0"].bbox_max);
         }

         glm::vec4 forward_direction = glm::vec4(-aabb_shark.get_model()[0].x, -aabb_shark.get_model()[0].y, -aabb_shark.get_model()[0].z, 0.0f);

         if (g_W_pressed) {
            velocity += forward_direction;
         }

         if (g_S_pressed) {
            velocity -= forward_direction;
         }

         if (velocity.x != 0 || velocity.z != 0) {
            velocity = velocity / norm(velocity);
         }

         velocity = velocity * speed * 5.0f * delta_t;

         aabb_shark.update_aabb(Matrix_Translate(velocity.x, 0.0f, velocity.z) * aabb_shark.get_model(), g_VirtualScene["Object_TexMap_0"].bbox_min,
                                g_VirtualScene["Object_TexMap_0"].bbox_max);


      } else {
         camera_position_c  = glm::vec4(x, y, z, 1.0f);
         camera_view_vector = camera_lookat_l - camera_position_c;
      }
      glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

      glm::mat4 projection;

      float nearplane = -0.1f;
      float farplane  = -30.0f;

      if (g_UsePerspectiveProjection) {

         float field_of_view = 3.141592 / 3.0f;
         projection          = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
      } else {

         float t    = 1.5f * g_CameraDistance / 2.5f;
         float b    = -t;
         float r    = t * g_ScreenRatio;
         float l    = -r;
         projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
      }


      glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define CUBE   3
#define BUTTON 4
#define SHARK  5
#define GROUND 6
      int i = 0;

      for (const auto &current_aabb: static_objects_list) {
         glm::mat4 current_model = current_aabb.get_model();
         std::string type        = current_aabb.get_type();


         if (type == "Cube") {
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
            glUniform1i(g_object_id_uniform, CUBE);
            DrawVirtualObject("Cube");
            cam_collision_map[current_aabb] = moving_AABB_to_AABB_intersec(cam_aabb, current_aabb, velocity, t_first[i], t_last[i]);

         } else if (type == "object_0") {
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
            glUniform1i(g_object_id_uniform, BUTTON);
            DrawVirtualObject("object_0");
            cam_collision_map[current_aabb] = moving_AABB_to_AABB_intersec(cam_aabb, current_aabb, velocity, t_first[i], t_last[i]);
         }

         ++i;
      }


      for (const auto &current_aabb: shark_arena_walls) {

         glm::mat4 current_model = current_aabb.get_model();
         std::string type        = current_aabb.get_type();

         if (type == "Cube") {
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(current_model));
            glUniform1i(g_object_id_uniform, CUBE);
            DrawVirtualObject("Cube");
            //cam_collision_map[current_aabb] = moving_AABB_to_AABB_intersec(cam_aabb, current_aabb, velocity, t_first[i], t_last[i]);
         }

         ++i;
      }


      model = Matrix_Translate(0.0f, -1.1f, 0.0f) * Matrix_Scale(10.0f, 1.0f, 10.0f);
      glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(g_object_id_uniform, PLANE);
      DrawVirtualObject("the_plane");


      float tmin;
      glm::vec4 intersec_point;
      bool button_ray_intersec =
          ray_to_AABB_intersec(camera_position_c, camera_view_vector / norm(camera_view_vector), aabb_button, tmin, intersec_point);
      if (Sphere_to_AABB_intersec(interaction_sphere, aabb_button) && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) && button_ray_intersec) {
         shark_mode = true;
      }
      if (shark_mode) {
         model = aabb_shark.get_model();
         glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
         glUniform1i(g_object_id_uniform, SHARK);
         DrawVirtualObject("Object_TexMap_0");
      }

      TextRendering_ShowEulerAngles(window);

      TextRendering_ShowProjection(window);

      TextRendering_ShowFramesPerSecond(window);

      glfwSwapBuffers(window);

      glfwPollEvents();
   }

   glfwTerminate();

   return 0;
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
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Ground"), 0);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Grass"), 1);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Shark0"), 2);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Shark1"), 3);
   glUniform1i(glGetUniformLocation(g_GpuProgramID, "Shark2"), 4);
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
   if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

      glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
      g_LeftMouseButtonPressed = true;
      g_LeftMouseButtonClicked = true;
   }
   if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {

      g_LeftMouseButtonPressed = false;
      g_LeftMouseButtonClicked = false;
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
   if (g_LeftMouseButtonPressed) {

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

   if (g_RightMouseButtonPressed) {

      float dx = xpos - g_LastCursorPosX;
      float dy = ypos - g_LastCursorPosY;

      g_ForearmAngleZ -= 0.01f * dx;
      g_ForearmAngleX += 0.01f * dy;

      g_LastCursorPosX = xpos;
      g_LastCursorPosY = ypos;
   }

   if (g_MiddleMouseButtonPressed) {

      float dx = xpos - g_LastCursorPosX;
      float dy = ypos - g_LastCursorPosY;

      g_TorsoPositionX += 0.01f * dx;
      g_TorsoPositionY -= 0.01f * dy;

      g_LastCursorPosX = xpos;
      g_LastCursorPosY = ypos;
   }
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

   float delta = 3.141592 / 16;

   if (key == GLFW_KEY_X && action == GLFW_PRESS) {
      g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
   }

   if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
      g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
   }
   if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
      g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
   }

   if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
      g_AngleX         = 0.0f;
      g_AngleY         = 0.0f;
      g_AngleZ         = 0.0f;
      g_ForearmAngleX  = 0.0f;
      g_ForearmAngleZ  = 0.0f;
      g_TorsoPositionX = 0.0f;
      g_TorsoPositionY = 0.0f;
   }

   if (key == GLFW_KEY_P && action == GLFW_PRESS) {
      g_UsePerspectiveProjection = true;
   }

   if (key == GLFW_KEY_O && action == GLFW_PRESS) {
      g_UsePerspectiveProjection = false;
   }

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
}

void ErrorCallback(int error, const char *description) { fprintf(stderr, "ERROR: GLFW: %s\n", description); }

void TextRendering_ShowModelViewProjection(GLFWwindow *window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model) {
   if (!g_ShowInfoText)
      return;

   glm::vec4 p_world  = model * p_model;
   glm::vec4 p_camera = view * p_world;
   glm::vec4 p_clip   = projection * p_camera;
   glm::vec4 p_ndc    = p_clip / p_clip.w;

   float pad = TextRendering_LineHeight(window);

   TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f - pad, 1.0f);
   TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f - 2 * pad, 1.0f);

   TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 6 * pad, 1.0f);
   TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 7 * pad, 1.0f);
   TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 8 * pad, 1.0f);

   TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f - 9 * pad, 1.0f);
   TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f - 10 * pad, 1.0f);

   TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 14 * pad, 1.0f);
   TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 15 * pad, 1.0f);
   TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 16 * pad, 1.0f);

   TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f - 17 * pad, 1.0f);
   TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f - 18 * pad, 1.0f);

   int width, height;
   glfwGetFramebufferSize(window, &width, &height);

   glm::vec2 a = glm::vec2(-1, -1);
   glm::vec2 b = glm::vec2(+1, +1);
   glm::vec2 p = glm::vec2(0, 0);
   glm::vec2 q = glm::vec2(width, height);

   glm::mat4 viewport_mapping = Matrix((q.x - p.x) / (b.x - a.x), 0.0f, 0.0f, (b.x * p.x - a.x * q.x) / (b.x - a.x), 0.0f, (q.y - p.y) / (b.y - a.y),
                                       0.0f, (b.y * p.y - a.y * q.y) / (b.y - a.y), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

   TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f - 22 * pad, 1.0f);
   TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f - 23 * pad, 1.0f);
   TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f - 24 * pad, 1.0f);

   TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f - 25 * pad, 1.0f);
   TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f - 26 * pad, 1.0f);
}

void TextRendering_ShowEulerAngles(GLFWwindow *window) {
   if (!g_ShowInfoText)
      return;

   float pad = TextRendering_LineHeight(window);

   char buffer[80];
   snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

   TextRendering_PrintString(window, buffer, -1.0f + pad / 10, -1.0f + 2 * pad / 10, 1.0f);
}

void TextRendering_ShowProjection(GLFWwindow *window) {
   if (!g_ShowInfoText)
      return;

   float lineheight = TextRendering_LineHeight(window);
   float charwidth  = TextRendering_CharWidth(window);

   if (g_UsePerspectiveProjection)
      TextRendering_PrintString(window, "Perspective", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
   else
      TextRendering_PrintString(window, "Orthographic", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
}

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

void PrintObjModelInfo(ObjModel *model) {
   const tinyobj::attrib_t &attrib                   = model->attrib;
   const std::vector<tinyobj::shape_t> &shapes       = model->shapes;
   const std::vector<tinyobj::material_t> &materials = model->materials;

   printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
   printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
   printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
   printf("# of shapes    : %d\n", (int)shapes.size());
   printf("# of materials : %d\n", (int)materials.size());

   for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
      printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v), static_cast<const double>(attrib.vertices[3 * v + 0]),
             static_cast<const double>(attrib.vertices[3 * v + 1]), static_cast<const double>(attrib.vertices[3 * v + 2]));
   }

   for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
      printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v), static_cast<const double>(attrib.normals[3 * v + 0]),
             static_cast<const double>(attrib.normals[3 * v + 1]), static_cast<const double>(attrib.normals[3 * v + 2]));
   }

   for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
      printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v), static_cast<const double>(attrib.texcoords[2 * v + 0]),
             static_cast<const double>(attrib.texcoords[2 * v + 1]));
   }

   for (size_t i = 0; i < shapes.size(); i++) {
      printf("shape[%ld].name = %s\n", static_cast<long>(i), shapes[i].name.c_str());
      printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i), static_cast<unsigned long>(shapes[i].mesh.indices.size()));

      size_t index_offset = 0;

      assert(shapes[i].mesh.num_face_vertices.size() == shapes[i].mesh.material_ids.size());

      printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i), static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

      for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
         size_t fnum = shapes[i].mesh.num_face_vertices[f];

         printf("  face[%ld].fnum = %ld\n", static_cast<long>(f), static_cast<unsigned long>(fnum));

         for (size_t v = 0; v < fnum; v++) {
            tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
            printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f), static_cast<long>(v), idx.vertex_index, idx.normal_index,
                   idx.texcoord_index);
         }

         printf("  face[%ld].material_id = %d\n", static_cast<long>(f), shapes[i].mesh.material_ids[f]);

         index_offset += fnum;
      }

      printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i), static_cast<unsigned long>(shapes[i].mesh.tags.size()));
      for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
         printf("  tag[%ld] = %s ", static_cast<long>(t), shapes[i].mesh.tags[t].name.c_str());
         printf(" ints: [");
         for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
            printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
            if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
               printf(", ");
            }
         }
         printf("]");

         printf(" floats: [");
         for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
            printf("%f", static_cast<const double>(shapes[i].mesh.tags[t].floatValues[j]));
            if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
               printf(", ");
            }
         }
         printf("]");

         printf(" strings: [");
         for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
            printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
            if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
               printf(", ");
            }
         }
         printf("]");
         printf("\n");
      }
   }

   for (size_t i = 0; i < materials.size(); i++) {
      printf("material[%ld].name = %s\n", static_cast<long>(i), materials[i].name.c_str());
      printf("  material.Ka = (%f, %f ,%f)\n", static_cast<const double>(materials[i].ambient[0]), static_cast<const double>(materials[i].ambient[1]),
             static_cast<const double>(materials[i].ambient[2]));
      printf("  material.Kd = (%f, %f ,%f)\n", static_cast<const double>(materials[i].diffuse[0]), static_cast<const double>(materials[i].diffuse[1]),
             static_cast<const double>(materials[i].diffuse[2]));
      printf("  material.Ks = (%f, %f ,%f)\n", static_cast<const double>(materials[i].specular[0]),
             static_cast<const double>(materials[i].specular[1]), static_cast<const double>(materials[i].specular[2]));
      printf("  material.Tr = (%f, %f ,%f)\n", static_cast<const double>(materials[i].transmittance[0]),
             static_cast<const double>(materials[i].transmittance[1]), static_cast<const double>(materials[i].transmittance[2]));
      printf("  material.Ke = (%f, %f ,%f)\n", static_cast<const double>(materials[i].emission[0]),
             static_cast<const double>(materials[i].emission[1]), static_cast<const double>(materials[i].emission[2]));
      printf("  material.Ns = %f\n", static_cast<const double>(materials[i].shininess));
      printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
      printf("  material.dissolve = %f\n", static_cast<const double>(materials[i].dissolve));
      printf("  material.illum = %d\n", materials[i].illum);
      printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
      printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
      printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
      printf("  material.map_Ns = %s\n", materials[i].specular_highlight_texname.c_str());
      printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
      printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
      printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
      printf("  <<PBR>>\n");
      printf("  material.Pr     = %f\n", materials[i].roughness);
      printf("  material.Pm     = %f\n", materials[i].metallic);
      printf("  material.Ps     = %f\n", materials[i].sheen);
      printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
      printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
      printf("  material.aniso  = %f\n", materials[i].anisotropy);
      printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
      printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
      printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
      printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
      printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
      printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
      std::map<std::string, std::string>::const_iterator it(materials[i].unknown_parameter.begin());
      std::map<std::string, std::string>::const_iterator itEnd(materials[i].unknown_parameter.end());

      for (; it != itEnd; it++) {
         printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
      }
      printf("\n");
   }
}

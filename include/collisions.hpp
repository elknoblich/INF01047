#ifndef COLLISIONS_H
#define COLLISIONS_H
#include "glm/ext/vector_float4.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <string>
#include <float.h>
class AABB {

   private:
   glm::vec3 bbox_min; // Axis-Aligned Bounding Box do objeto
   glm::vec3 bbox_max;
   glm::mat4 model;
   std::string type;
   int id;


   public:
   AABB(glm::vec3 bbox_min_local, glm::vec3 bbox_max_local, glm::mat4 model, int id, std::string type);

   glm::vec3 get_min() const;
   glm::vec3 get_max() const;
   glm::mat4 get_model() const;
   std::string get_type() const;
   glm::vec4 get_center_point();
   void update_aabb(glm::mat4 new_model, glm::vec3 bbox_min_local, glm::vec3 bbox_max_local);
   bool operator<(const AABB &other) const;
};

bool AABB_to_AABB_intersec(AABB aabb1, AABB aabb2);

class SPHERE {
   private:
   glm::vec4 position;
   float radius;
   int id;

   public:
   SPHERE(glm::vec4 position, float radius, int id, glm::vec4 translation_vec);
   void update_sphere(glm::vec4 position, glm::vec4 translation_vec);
   glm::vec4 get_center();
   float get_radius();
   bool operator<(const SPHERE &other) const;
};

bool Sphere_to_AABB_intersec(SPHERE sphere, AABB aabb);
bool moving_AABB_to_AABB_intersec(AABB moving_aabb, AABB aabb, glm::vec4 velocity, float &t_first, float &t_last);
bool ray_to_AABB_intersec(glm::vec4 point, glm::vec4 vector, AABB aabb);

#endif

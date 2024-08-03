#ifndef COLLISIONS_H
#define COLLISIONS_H
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

class AABB {

   private:
   glm::vec3 bbox_min; // Axis-Aligned Bounding Box do objeto
   glm::vec3 bbox_max;
   glm::mat4 model;
   int id;


   public:
   AABB(glm::vec3 bbox_min_local, glm::vec3 bbox_max_local, glm::mat4 model, int id);
   AABB(glm::vec4 center_point, glm::vec4 size, int id);


   glm::vec3 get_min();
   glm::vec3 get_max();
   bool operator<(const AABB &other) const;
};

bool AABB_to_AABB_intersec(AABB aabb1, AABB aabb2);

#endif

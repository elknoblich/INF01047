#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>


// AABB em coordenadas globais
class AABB {

   private:
   glm::vec3 bbox_min; // Axis-Aligned Bounding Box do objeto
   glm::vec3 bbox_max;
   glm::mat4 model;


   public:
   AABB(glm::vec3 bbox_min_local, glm::vec3 bbox_max_local, glm::mat4 model) {
      glm::vec4 bbox_min_global = model * glm::vec4(bbox_min_local.x, bbox_min_local.y, bbox_min_local.z, 0.0f);
      glm::vec4 bbox_max_global = model * glm::vec4(bbox_max_local.x, bbox_max_local.y, bbox_max_local.z, 0.0f);
      bbox_min                  = glm::vec3(bbox_min_global.x, bbox_min_global.y, bbox_min_global.z);
      bbox_min                  = glm::vec3(bbox_max_global.x, bbox_max_global.y, bbox_max_global.z);
   };
   glm::vec3 get_min() { return bbox_min; };
   glm::vec3 get_max() { return bbox_max };
};

bool AABB_to_AABB_intersec(AABB aabb1, AABB aabb2) {
   glm::vec3 aabb_min_1 = aabb1.get_min();
   glm::vec3 aabb_max_1 = aabb1.get_max();
   glm::vec3 aabb_min_2 = aabb1.get_min();

   return (aMin.x <= bMax.x && aMax.x >= bMin.x) && (aMin.y <= bMax.y && aMax.y >= bMin.y) && (aMin.z <= bMax.z && aMax.z >= bMin.z);
}
}

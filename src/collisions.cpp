#include "../include/collisions.hpp"
// AABB em coordenadas globais
AABB::AABB(glm::vec3 bbox_min_local, glm::vec3 bbox_max_local, glm::mat4 model, int id) {
   glm::vec4 bbox_min_global = model * glm::vec4(bbox_min_local.x, bbox_min_local.y, bbox_min_local.z, 1.0f);
   glm::vec4 bbox_max_global = model * glm::vec4(bbox_max_local.x, bbox_max_local.y, bbox_max_local.z, 1.0f);
   bbox_min                  = glm::vec3(bbox_min_global.x, bbox_min_global.y, bbox_min_global.z);
   bbox_max                  = glm::vec3(bbox_max_global.x, bbox_max_global.y, bbox_max_global.z);
   id                        = id;
};

AABB::AABB(glm::vec4 center_point, glm::vec4 size, int id) {
   glm::vec4 p1 = center_point + size;
   glm::vec4 p2 = center_point - size;
   bbox_min.x   = std::fmin(p1.x, p2.x);
   bbox_min.y   = std::fmin(p1.y, p2.y);
   bbox_min.z   = std::fmin(p1.z, p2.z);
   bbox_max.x   = std::fmax(p1.x, p2.x);
   bbox_max.y   = std::fmax(p1.y, p2.y);
   bbox_max.z   = std::fmax(p1.z, p2.z);
   id           = id;
};

bool AABB::operator<(const AABB &other) const { return id < other.id; };

glm::vec3 AABB::get_min() { return bbox_min; };
glm::vec3 AABB::get_max() { return bbox_max; };

bool AABB_to_AABB_intersec(AABB aabb1, AABB aabb2) {
   glm::vec3 aMin = aabb1.get_min();
   glm::vec3 aMax = aabb1.get_max();
   glm::vec3 bMin = aabb2.get_min();
   glm::vec3 bMax = aabb2.get_max();

   return (aMin.x <= bMax.x && aMax.x >= bMin.x) && (aMin.y <= bMax.y && aMax.y >= bMin.y) && (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

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

SPHERE::SPHERE(glm::vec4 position, float radius, int id, glm::vec4 translation_vec) {
   position = position + translation_vec;
   radius   = radius;
   id       = id;
}
glm::vec4 SPHERE::get_center() { return position; }
float SPHERE::get_radius() { return radius; }
bool SPHERE::operator<(const SPHERE &other) const { return id < other.id; }
bool Sphere_to_AABB_intersec(SPHERE sphere, AABB aabb) {

   glm::vec4 closest_point;
   closest_point.x = std::fmax(aabb.get_min().x, std::fmin(sphere.get_center().x, aabb.get_max().x));
   closest_point.y = std::fmax(aabb.get_min().y, std::fmin(sphere.get_center().y, aabb.get_max().y));
   closest_point.z = std::fmax(aabb.get_min().z, std::fmin(sphere.get_center().z, aabb.get_max().z));
   closest_point.w = 1.0f;

   float dist_sq   = glm::length(sphere.get_center() - closest_point);
   float radius_sq = sphere.get_radius() * sphere.get_radius();

   return dist_sq < radius_sq;
}

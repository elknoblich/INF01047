#include "../include/collisions.hpp"
/*
 * Referencias:
 * Real-Time Collision Detection, Christer Ericson
 * Game Physics Cookbook, Gabor Szauer
 * Real-Time Rendering
 * */
// AABB em coordenadas globais
AABB::AABB(glm::vec3 bbox_min_local, glm::vec3 bbox_max_local, glm::mat4 model, int id, std::string type) {
   glm::vec4 bbox_min_global = model * glm::vec4(bbox_min_local.x, bbox_min_local.y, bbox_min_local.z, 1.0f);
   glm::vec4 bbox_max_global = model * glm::vec4(bbox_max_local.x, bbox_max_local.y, bbox_max_local.z, 1.0f);
   this->bbox_min            = glm::vec3(bbox_min_global.x, bbox_min_global.y, bbox_min_global.z);
   this->bbox_max            = glm::vec3(bbox_max_global.x, bbox_max_global.y, bbox_max_global.z);
   this->id                  = id;
   this->model               = model;
   this->type                = type;
};

AABB::AABB(glm::vec4 center_point, glm::vec4 size, int id) {
   glm::vec4 p1     = center_point + size;
   glm::vec4 p2     = center_point - size;
   this->bbox_min.x = std::fmin(p1.x, p2.x);
   this->bbox_min.y = std::fmin(p1.y, p2.y);
   this->bbox_min.z = std::fmin(p1.z, p2.z);
   this->bbox_max.x = std::fmax(p1.x, p2.x);
   this->bbox_max.y = std::fmax(p1.y, p2.y);
   this->bbox_max.z = std::fmax(p1.z, p2.z);
   this->id         = id;
};

bool AABB::operator<(const AABB &other) const { return id < other.id; };

glm::vec3 AABB::get_min() const { return bbox_min; };
glm::vec3 AABB::get_max() const { return bbox_max; };
glm::mat4 AABB::get_model() const { return model; };
std::string AABB::get_type() const { return type; };
void AABB::update_aabb(glm::vec4 center_point, glm::vec4 size) {
   glm::vec4 p1     = center_point + size;
   glm::vec4 p2     = center_point - size;
   this->bbox_min.x = std::fmin(p1.x, p2.x);
   this->bbox_min.y = std::fmin(p1.y, p2.y);
   this->bbox_min.z = std::fmin(p1.z, p2.z);
   this->bbox_max.x = std::fmax(p1.x, p2.x);
   this->bbox_max.y = std::fmax(p1.y, p2.y);
   this->bbox_max.z = std::fmax(p1.z, p2.z);
};

bool AABB_to_AABB_intersec(AABB aabb1, AABB aabb2) {
   glm::vec3 aMin = aabb1.get_min();
   glm::vec3 aMax = aabb1.get_max();
   glm::vec3 bMin = aabb2.get_min();
   glm::vec3 bMax = aabb2.get_max();

   return (aMin.x <= bMax.x && aMax.x >= bMin.x) && (aMin.y <= bMax.y && aMax.y >= bMin.y) && (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

SPHERE::SPHERE(glm::vec4 position, float radius, int id, glm::vec4 translation_vec) {
   this->position   = position + translation_vec;
   this->position.w = 1.0f;
   this->radius     = radius;
   this->id         = id;
}

void SPHERE::update_sphere(glm::vec4 position, glm::vec4 translation_vec) {
   this->position   = position + translation_vec;
   this->position.w = 1.0f;
}
glm::vec4 SPHERE::get_center() { return position; }

float SPHERE::get_radius() { return radius; }

bool SPHERE::operator<(const SPHERE &other) const { return id < other.id; }

bool Sphere_to_AABB_intersec(SPHERE sphere, AABB aabb) {

   glm::vec4 closest_point;
   glm::vec4 sphere_center = sphere.get_center();
   closest_point.x         = std::fmax(aabb.get_min().x, std::fmin(sphere_center.x, aabb.get_max().x));
   closest_point.y         = std::fmax(aabb.get_min().y, std::fmin(sphere_center.y, aabb.get_max().y));
   closest_point.z         = std::fmax(aabb.get_min().z, std::fmin(sphere_center.z, aabb.get_max().z));
   closest_point.w         = 1.0f;

   glm::vec4 diff  = sphere_center - closest_point;
   float dist_sq   = glm::dot(diff, diff);
   float radius_sq = sphere.get_radius() * sphere.get_radius();
   return dist_sq < radius_sq;
}

bool moving_AABB_to_AABB_intersec(AABB moving_aabb, AABB aabb, glm::vec4 velocity, float &t_first, float &t_last) {

   if (AABB_to_AABB_intersec(moving_aabb, aabb)) {
      t_first = 0.0f;
      t_last  = 0.0f;
      return true;
   }

   t_first = 0.0f;
   t_last  = 1.0f;

   glm::vec3 moving_aabb_max = moving_aabb.get_max();
   glm::vec3 moving_aabb_min = moving_aabb.get_min();
   glm::vec3 aabb_max        = aabb.get_max();
   glm::vec3 aabb_min        = aabb.get_min();

   for (int i = 0; i < 3; i++) {

      if (velocity[i] < 0.0f) {

         if (moving_aabb_max[i] < aabb_min[i])
            return false;

         if (aabb_max[i] < moving_aabb_min[i])
            t_first = std::fmax((aabb_max[i] - moving_aabb_min[i]) / velocity[i], t_first);

         if (moving_aabb_max[i] > aabb_min[i])
            t_last = std::fmin((aabb_min[i] - moving_aabb_max[i]) / velocity[i], t_last);
      }

      if (velocity[i] > 0.0f) {

         if (moving_aabb_min[i] > aabb_max[i])
            return false;

         if (moving_aabb_max[i] < aabb_min[i])
            t_first = std::fmax((aabb_min[i] - moving_aabb_max[i]) / velocity[i], t_first);

         if (aabb_max[i] > moving_aabb_min[i])
            t_last = std::fmin((aabb_max[i] - moving_aabb_min[i]) / velocity[i], t_last);
      }

      if (velocity[i] == 0.0f)
         if (moving_aabb_max[i] < aabb_min[i] || moving_aabb_min[i] > aabb_max[i])
            return false;

      if (t_first > t_last)
         return false;
   }

   return true;
}

bool ray_to_AABB_intersec(glm::vec4 point, glm::vec4 vector, AABB aabb, float &tmin, glm::vec4 &intersec_point) {
   tmin       = 0.0f;
   float tmax = FLT_MAX;

   for (int i = 0; i < 3; i++) {


      float origin   = point[i];
      float dir      = vector[i];
      float aabb_min = aabb.get_min()[i];
      float aabb_max = aabb.get_max()[i];

      if (std::fabs(vector[i]) < 0.001) {

         if (origin < aabb_min || origin > aabb_max)
            return false;

      } else {
         float ood = 1.0f / dir;
         float t1  = (aabb_min - origin) * ood;
         float t2  = (aabb_max - origin) * ood;

         if (t1 > t2)
            std::swap(t1, t2);

         tmin = std::max(tmin, t1);
         tmax = std::min(tmax, t2);

         if (tmin > tmax)
            return false;
      }
   }

   return true;
}

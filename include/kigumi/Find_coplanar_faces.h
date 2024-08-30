#pragma once

#include <kigumi/Face_tag.h>
#include <kigumi/Mesh_handles.h>
#include <kigumi/Triangle_soup.h>

#include <algorithm>
#include <array>
#include <boost/container_hash/hash.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace kigumi {

template <class K, class FaceData>
class Find_coplanar_faces {
  using Triangle = std::array<std::size_t, 3>;
  using Triangle_hash = boost::hash<Triangle>;
  using Triangle_soup = Triangle_soup<K, FaceData>;

 public:
  std::pair<std::vector<Face_tag>, std::vector<Face_tag>> operator()(
      const Triangle_soup& left, const Triangle_soup& right,
      const std::vector<std::size_t>& left_points,
      const std::vector<std::size_t>& right_points) const {
    std::vector<Face_tag> left_face_tags(left.num_faces());
    std::vector<Face_tag> right_face_tags(right.num_faces());

    bool left_is_a = left.num_faces() < right.num_faces();
    const auto& a = left_is_a ? left : right;
    const auto& b = left_is_a ? right : left;
    const auto& a_points = left_is_a ? left_points : right_points;
    const auto& b_points = left_is_a ? right_points : left_points;
    auto& a_face_tags = left_is_a ? left_face_tags : right_face_tags;
    auto& b_face_tags = left_is_a ? right_face_tags : left_face_tags;

    std::unordered_map<Triangle, Face_handle, Triangle_hash> triangle_to_fh;

    triangle_to_fh.reserve(a.num_faces());
    for (auto fh : a.faces()) {
      auto tri = triangle(a, fh, a_points);
      triangle_to_fh.emplace(tri, fh);
    }

    for (auto fh : b.faces()) {
      auto tri = triangle(b, fh, b_points);

      auto it = triangle_to_fh.find(tri);
      if (it != triangle_to_fh.end()) {
        a_face_tags.at(it->second.i) = Face_tag::Coplanar;
        b_face_tags.at(fh.i) = Face_tag::Coplanar;
        continue;
      }

      it = triangle_to_fh.find(opposite(tri));
      if (it != triangle_to_fh.end()) {
        a_face_tags.at(it->second.i) = Face_tag::Opposite;
        b_face_tags.at(fh.i) = Face_tag::Opposite;
      }
    }

    return {std::move(left_face_tags), std::move(right_face_tags)};
  }

 private:
  static Triangle triangle(const Triangle_soup& m, Face_handle fh,
                           const std::vector<std::size_t>& points) {
    auto face = m.face(fh);
    Triangle triangle{
        points.at(face[0].i),
        points.at(face[1].i),
        points.at(face[2].i),
    };
    std::rotate(triangle.begin(), std::min_element(triangle.begin(), triangle.end()),
                triangle.end());
    return triangle;
  }

  static Triangle opposite(const Triangle& triangle) {
    return {triangle[0], triangle[2], triangle[1]};
  }
};

}  // namespace kigumi

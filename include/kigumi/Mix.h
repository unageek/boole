#pragma once

#include <kigumi/Classify_faces_globally.h>
#include <kigumi/Classify_faces_locally.h>
#include <kigumi/Corefine.h>
#include <kigumi/Mesh_entities.h>
#include <kigumi/Mesh_indices.h>
#include <kigumi/Mixed.h>
#include <kigumi/Triangle_soup.h>
#include <kigumi/Warnings.h>
#include <kigumi/parallel_do.h>

#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

namespace kigumi {

template <class K, class FaceData>
class Mix {
  using Classify_faces_globally = Classify_faces_globally<K, FaceData>;
  using Classify_faces_locally = Classify_faces_locally<K, FaceData>;
  using Mixed_face_data = Mixed_face_data<FaceData>;
  using Mixed_triangle_mesh = Mixed_triangle_mesh<K, FaceData>;
  using Mixed_triangle_soup = Mixed_triangle_soup<K, FaceData>;
  using Triangle_soup = Triangle_soup<K, FaceData>;

 public:
  std::pair<Mixed_triangle_soup, Warnings> operator()(const Triangle_soup& left,
                                                      const Triangle_soup& right) const {
    Corefine corefine{left, right};

    std::cout << "Constructing mixed mesh..." << std::endl;

    std::vector<Face> faces;
    std::vector<Mixed_face_data> face_data;

    for (auto fi : left.faces()) {
      auto [tag, count] = corefine.get_left_faces(fi, std::back_inserter(faces));
      Mixed_face_data data{true, tag, left.data(fi)};
      face_data.resize(face_data.size() + count, data);
    }

    for (auto fi : right.faces()) {
      auto [tag, count] = corefine.get_right_faces(fi, std::back_inserter(faces));
      Mixed_face_data data{false, tag, right.data(fi)};
      face_data.resize(face_data.size() + count, data);
    }

    Mixed_triangle_mesh m(corefine.take_points(), std::move(faces), std::move(face_data));
    m.finalize();

    std::cout << "Local classification..." << std::endl;

    auto intersecting_edges = corefine.get_intersecting_edges();
    Edge_set border_edges(intersecting_edges.begin(), intersecting_edges.end());
    Warnings warnings{};

    parallel_do(
        intersecting_edges.begin(), intersecting_edges.end(), Warnings{},
        [&](const auto& edge, auto& local_warnings) {
          thread_local Classify_faces_locally classify_faces_locally;

          local_warnings |= classify_faces_locally(m, edge, border_edges);
        },
        [&](const auto& local_warnings) { warnings |= local_warnings; });

    std::cout << "Global classification..." << std::endl;

    Classify_faces_globally classify_faces_globally;
    warnings |= classify_faces_globally(m, border_edges, left, right);

    return {m.take_triangle_soup(), warnings};
  }
};

}  // namespace kigumi

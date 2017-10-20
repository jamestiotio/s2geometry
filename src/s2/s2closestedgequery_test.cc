// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Author: ericv@google.com (Eric Veach)

#include "s2/s2closestedgequery.h"

#include <memory>
#include <set>
#include <vector>

#include <gtest/gtest.h>
#include "s2/third_party/absl/memory/memory.h"
#include "s2/s1angle.h"
#include "s2/s2cap.h"
#include "s2/s2cell.h"
#include "s2/s2cellid.h"
#include "s2/s2edge_distances.h"
#include "s2/s2loop.h"
#include "s2/s2metrics.h"
#include "s2/s2predicates.h"
#include "s2/s2shapeutil.h"
#include "s2/s2testing.h"
#include "s2/s2textformat.h"

using absl::make_unique;
using s2textformat::MakeIndexOrDie;
using s2textformat::MakePointOrDie;
using s2textformat::MakePolygonOrDie;
using s2textformat::ParsePointsOrDie;
using std::fabs;
using std::make_pair;
using std::min;
using std::ostream;
using std::pair;
using std::unique_ptr;
using std::vector;

TEST(PointTarget, UpdateMinDistanceToEdgeWhenEqual) {
  // Verifies that UpdateMinDistance only returns true when the new distance
  // is less than the old distance (not less than or equal to).
  S2ClosestEdgeQuery::PointTarget target(MakePointOrDie("1:0"));
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  auto edge = ParsePointsOrDie("0:-1, 0:1");
  EXPECT_TRUE(target.UpdateMinDistance(edge[0], edge[1], &dist));
  EXPECT_FALSE(target.UpdateMinDistance(edge[0], edge[1], &dist));
}

TEST(PointTarget, UpdateMinDistanceToCellWhenEqual) {
  // Verifies that UpdateMinDistance only returns true when the new distance
  // is less than the old distance (not less than or equal to).
  S2ClosestEdgeQuery::PointTarget target(MakePointOrDie("1:0"));
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  S2Cell cell{S2CellId(MakePointOrDie("0:0"))};
  EXPECT_TRUE(target.UpdateMinDistance(cell, &dist));
  EXPECT_FALSE(target.UpdateMinDistance(cell, &dist));
}

TEST(EdgeTarget, UpdateMinDistanceToEdgeWhenEqual) {
  S2ClosestEdgeQuery::EdgeTarget target(MakePointOrDie("1:0"),
                                        MakePointOrDie("1:1"));
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  auto edge = ParsePointsOrDie("0:-1, 0:1");
  EXPECT_TRUE(target.UpdateMinDistance(edge[0], edge[1], &dist));
  EXPECT_FALSE(target.UpdateMinDistance(edge[0], edge[1], &dist));
}

TEST(EdgeTarget, UpdateMinDistanceToCellWhenEqual) {
  S2ClosestEdgeQuery::EdgeTarget target(MakePointOrDie("1:0"),
                                        MakePointOrDie("1:1"));
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  S2Cell cell{S2CellId(MakePointOrDie("0:0"))};
  EXPECT_TRUE(target.UpdateMinDistance(cell, &dist));
  EXPECT_FALSE(target.UpdateMinDistance(cell, &dist));
}

TEST(CellTarget, UpdateMinDistanceToEdgeWhenEqual) {
  S2ClosestEdgeQuery::CellTarget
      target{S2Cell{S2CellId{MakePointOrDie("0:1")}}};
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  auto edge = ParsePointsOrDie("0:-1, 0:1");
  EXPECT_TRUE(target.UpdateMinDistance(edge[0], edge[1], &dist));
  EXPECT_FALSE(target.UpdateMinDistance(edge[0], edge[1], &dist));
}

TEST(CellTarget, UpdateMinDistanceToCellWhenEqual) {
  S2ClosestEdgeQuery::CellTarget
      target{S2Cell{S2CellId{MakePointOrDie("0:1")}}};
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  S2Cell cell{S2CellId(MakePointOrDie("0:0"))};
  EXPECT_TRUE(target.UpdateMinDistance(cell, &dist));
  EXPECT_FALSE(target.UpdateMinDistance(cell, &dist));
}

TEST(ShapeIndexTarget, UpdateMinDistanceToEdgeWhenEqual) {
  auto target_index = MakeIndexOrDie("1:0 # #");
  S2ClosestEdgeQuery::ShapeIndexTarget target(target_index.get());
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  auto edge = ParsePointsOrDie("0:-1, 0:1");
  EXPECT_TRUE(target.UpdateMinDistance(edge[0], edge[1], &dist));
  EXPECT_FALSE(target.UpdateMinDistance(edge[0], edge[1], &dist));
}

TEST(ShapeIndexTarget, UpdateMinDistanceToCellWhenEqual) {
  auto target_index = MakeIndexOrDie("1:0 # #");
  S2ClosestEdgeQuery::ShapeIndexTarget target(target_index.get());
  S2ClosestEdgeQuery::Distance dist(S1ChordAngle::Infinity());
  S2Cell cell{S2CellId(MakePointOrDie("0:0"))};
  EXPECT_TRUE(target.UpdateMinDistance(cell, &dist));
  EXPECT_FALSE(target.UpdateMinDistance(cell, &dist));
}

TEST(PointTarget, GetContainingShapes) {
  // Only shapes 2 and 4 should contain the target point.
  auto index = MakeIndexOrDie(
      "1:1 # 1:1, 2:2 # 0:0, 0:3, 3:0 | 6:6, 6:9, 9:6 | 0:0, 0:4, 4:0");
  S2ClosestEdgeQuery::PointTarget target(MakePointOrDie("1:1"));
  EXPECT_EQ((vector<int>{2}), target.GetContainingShapes(*index, 1));
  EXPECT_EQ((vector<int>{2, 4}), target.GetContainingShapes(*index, 5));
}

TEST(EdgeTarget, GetContainingShapes) {
  // Only shapes 2 and 4 should contain the target point.
  auto index = MakeIndexOrDie(
      "1:1 # 1:1, 2:2 # 0:0, 0:3, 3:0 | 6:6, 6:9, 9:6 | 0:0, 0:4, 4:0");
  S2ClosestEdgeQuery::EdgeTarget target(MakePointOrDie("1:2"),
                                        MakePointOrDie("2:1"));
  EXPECT_EQ((vector<int>{2}), target.GetContainingShapes(*index, 1));
  EXPECT_EQ((vector<int>{2, 4}), target.GetContainingShapes(*index, 5));
}

TEST(CellTarget, GetContainingShapes) {
  auto index = MakeIndexOrDie(
      "1:1 # 1:1, 2:2 # 0:0, 0:3, 3:0 | 6:6, 6:9, 9:6 | -1:-1, -1:5, 5:-1");
  // Only shapes 2 and 4 should contain a very small cell near 1:1.
  S2CellId cellid1(MakePointOrDie("1:1"));
  S2ClosestEdgeQuery::CellTarget target1{S2Cell(cellid1)};
  EXPECT_EQ((vector<int>{2}), target1.GetContainingShapes(*index, 1));
  EXPECT_EQ((vector<int>{2, 4}), target1.GetContainingShapes(*index, 5));

  // For a larger cell that properly contains one or more index cells, all
  // shapes that intersect the first such cell in S2CellId order are returned.
  // In the test below, this happens to again be the 1st and 3rd polygons
  // (whose shape_ids are 2 and 4).
  S2CellId cellid2 = cellid1.parent(5);
  S2ClosestEdgeQuery::CellTarget target2{S2Cell(cellid2)};
  EXPECT_EQ((vector<int>{2, 4}), target2.GetContainingShapes(*index, 5));
}

TEST(ShapeIndexTarget, GetContainingShapes) {
  // Create an index containing a repeated grouping of one point, one
  // polyline, and one polygon.
  auto index = MakeIndexOrDie(
      "1:1 | 4:4 | 7:7 | 10:10 # "
      "1:1, 1:2 | 4:4, 4:5 | 7:7, 7:8 | 10:10, 10:11 # "
      "0:0, 0:3, 3:0 | 3:3, 3:6, 6:3 | 6:6, 6:9, 9:6 | 9:9, 9:12, 12:9");

  // Construct a target consisting of one point, one polyline, and one polygon
  // with two loops where only the second loop is contained by a polygon in
  // the index above.
  auto target_index = MakeIndexOrDie(
      "1:1 # 4:5, 5:4 # 20:20, 20:21, 21:20; 10:10, 10:11, 11:10");

  S2ClosestEdgeQuery::ShapeIndexTarget target(target_index.get());
  // These are the shape_ids of the 1st, 2nd, and 4th polygons of "index"
  // (noting that the 4 points are represented by one S2PointVectorShape).
  EXPECT_EQ((vector<int>{5, 6, 8}), target.GetContainingShapes(*index, 5));
}

TEST(ShapeIndexTarget, GetContainingShapesEmptyAndFull) {
  // Verify that GetContainingShapes never returns empty polygons and always
  // returns full polygons (i.e., those containing the entire sphere).

  // Creating an index containing one empty and one full polygon.
  auto index = MakeIndexOrDie("# # empty | full");

  // Check only the full polygon is returned for a point target.
  auto point_index = MakeIndexOrDie("1:1 # #");
  S2ClosestEdgeQuery::ShapeIndexTarget point_target(point_index.get());
  EXPECT_EQ((vector<int>{1}), point_target.GetContainingShapes(*index, 5));

  // Check only the full polygon is returned for a full polygon target.
  auto full_polygon_index = MakeIndexOrDie("# # full");
  S2ClosestEdgeQuery::ShapeIndexTarget full_target(full_polygon_index.get());
  EXPECT_EQ((vector<int>{1}), full_target.GetContainingShapes(*index, 5));

  // Check that nothing is returned for an empty polygon target.  (An empty
  // polygon has no connected components and does not intersect anything, so
  // according to the API of GetContainingShapes nothing should be returned.)
  auto empty_polygon_index = MakeIndexOrDie("# # empty");
  S2ClosestEdgeQuery::ShapeIndexTarget empty_target(empty_polygon_index.get());
  EXPECT_EQ((vector<int>{}), empty_target.GetContainingShapes(*index, 5));
}

TEST(S2ClosestEdgeQuery, NoEdges) {
  S2ShapeIndex index;
  S2ClosestEdgeQuery query(&index);
  S2ClosestEdgeQuery::PointTarget target(S2Point(1, 0, 0));
  const auto edge = query.FindClosestEdge(&target);
  EXPECT_EQ(S1ChordAngle::Infinity(), edge.distance);
  EXPECT_EQ(-1, edge.edge_id);
  EXPECT_EQ(-1, edge.shape_id);
  EXPECT_EQ(S1ChordAngle::Infinity(), query.GetDistance(&target));
}

TEST(S2ClosestEdgeQuery, OptionsNotModified) {
  // Tests that FindClosestEdge(), GetDistance(), and IsDistanceLess() do not
  // modify query.options(), even though all of these methods have their own
  // specific options requirements.
  S2ClosestEdgeQuery::Options options;
  options.set_max_edges(3);
  options.set_max_distance(S1ChordAngle::Degrees(3));
  options.set_max_error(S1ChordAngle::Degrees(0.001));
  auto index = MakeIndexOrDie("1:1 | 1:2 | 1:3 # #");
  S2ClosestEdgeQuery query(index.get(), options);
  S2ClosestEdgeQuery::PointTarget target(MakePointOrDie("2:2"));
  EXPECT_EQ(1, query.FindClosestEdge(&target).edge_id);
  EXPECT_NEAR(1.0, query.GetDistance(&target).degrees(), 1e-15);
  EXPECT_TRUE(query.IsDistanceLess(&target, S1ChordAngle::Degrees(1.5)));

  // Verify that none of the options above were modified.
  EXPECT_EQ(options.max_edges(), query.options().max_edges());
  EXPECT_EQ(options.max_distance(), query.options().max_distance());
  EXPECT_EQ(options.max_error(), query.options().max_error());
}

TEST(S2ClosestEdgeQuery, TargetPointInsideIndexedPolygon) {
  // Tests a target point in the interior of an indexed polygon.
  // (The index also includes a polyline loop with no interior.)
  auto index = MakeIndexOrDie("# 0:0, 0:5, 5:5, 5:0 # 0:10, 0:15, 5:15, 5:10");
  S2ClosestEdgeQuery::Options options;
  options.set_include_interiors(true);
  options.set_max_distance(S1Angle::Degrees(1));
  S2ClosestEdgeQuery query(index.get(), options);
  S2ClosestEdgeQuery::PointTarget target(MakePointOrDie("2:12"));
  auto results = query.FindClosestEdges(&target);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ(S1ChordAngle::Zero(), results[0].distance);
  EXPECT_EQ(1, results[0].shape_id);
  EXPECT_EQ(-1, results[0].edge_id);
}

TEST(S2ClosestEdgeQuery, TargetPointOutsideIndexedPolygon) {
  // Tests a target point in the interior of a polyline loop with no
  // interior.  (The index also includes a nearby polygon.)
  auto index = MakeIndexOrDie("# 0:0, 0:5, 5:5, 5:0 # 0:10, 0:15, 5:15, 5:10");
  S2ClosestEdgeQuery::Options options;
  options.set_include_interiors(true);
  options.set_max_distance(S1Angle::Degrees(1));
  S2ClosestEdgeQuery query(index.get(), options);
  S2ClosestEdgeQuery::PointTarget target(MakePointOrDie("2:2"));
  auto results = query.FindClosestEdges(&target);
  EXPECT_EQ(0, results.size());
}

TEST(S2ClosestEdgeQuery, TargetPolygonContainingIndexedPoints) {
  // Two points are contained within a polyline loop (no interior) and two
  // points are contained within a polygon.
  auto index = MakeIndexOrDie("2:2 | 3:3 | 1:11 | 3:13 # #");
  S2ClosestEdgeQuery query(index.get());
  query.mutable_options()->set_max_distance(S1Angle::Degrees(1));
  auto target_index = MakeIndexOrDie(
      "# 0:0, 0:5, 5:5, 5:0 # 0:10, 0:15, 5:15, 5:10");
  S2ClosestEdgeQuery::ShapeIndexTarget target(target_index.get());
  target.set_include_interiors(true);
  auto results = query.FindClosestEdges(&target);
  ASSERT_EQ(2, results.size());
  EXPECT_EQ(S1ChordAngle::Zero(), results[0].distance);
  EXPECT_EQ(0, results[0].shape_id);
  EXPECT_EQ(2, results[0].edge_id);  // 1:11
  EXPECT_EQ(S1ChordAngle::Zero(), results[1].distance);
  EXPECT_EQ(0, results[1].shape_id);
  EXPECT_EQ(3, results[1].edge_id);  // 3:13
}

TEST(S2ClosestEdgeQuery, EmptyPolygonTarget) {
  // Verifies that distances are measured correctly to empty polygon targets.
  auto empty_polygon_index = MakeIndexOrDie("# # empty");
  auto point_index = MakeIndexOrDie("1:1 # #");
  auto full_polygon_index = MakeIndexOrDie("# # full");
  S2ClosestEdgeQuery::ShapeIndexTarget target(empty_polygon_index.get());
  target.set_include_interiors(true);

  S2ClosestEdgeQuery empty_query(empty_polygon_index.get());
  empty_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Infinity(), empty_query.GetDistance(&target));

  S2ClosestEdgeQuery point_query(point_index.get());
  point_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Infinity(), point_query.GetDistance(&target));

  S2ClosestEdgeQuery full_query(full_polygon_index.get());
  full_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Infinity(), full_query.GetDistance(&target));
}

TEST(S2ClosestEdgeQuery, FullLaxPolygonTarget) {
  // Verifies that distances are measured correctly to full LaxPolygon targets.
  auto empty_polygon_index = MakeIndexOrDie("# # empty");
  auto point_index = MakeIndexOrDie("1:1 # #");
  auto full_polygon_index = MakeIndexOrDie("# # full");
  S2ClosestEdgeQuery::ShapeIndexTarget target(full_polygon_index.get());
  target.set_include_interiors(true);

  S2ClosestEdgeQuery empty_query(empty_polygon_index.get());
  empty_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Infinity(), empty_query.GetDistance(&target));

  S2ClosestEdgeQuery point_query(point_index.get());
  point_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Zero(), point_query.GetDistance(&target));

  S2ClosestEdgeQuery full_query(full_polygon_index.get());
  full_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Zero(), full_query.GetDistance(&target));
}

TEST(S2ClosestEdgeQuery, FullS2PolygonTarget) {
  // Verifies that distances are measured correctly to full S2Polygon targets
  // (which use a different representation of "full" than LaxPolygon does).
  auto empty_polygon_index = MakeIndexOrDie("# # empty");
  auto point_index = MakeIndexOrDie("1:1 # #");
  auto full_polygon_index = MakeIndexOrDie("# #");
  full_polygon_index->Add(make_unique<S2Polygon::OwningShape>(
      s2textformat::MakePolygonOrDie("full")));

  S2ClosestEdgeQuery::ShapeIndexTarget target(full_polygon_index.get());
  target.set_include_interiors(true);

  S2ClosestEdgeQuery empty_query(empty_polygon_index.get());
  empty_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Infinity(), empty_query.GetDistance(&target));

  S2ClosestEdgeQuery point_query(point_index.get());
  point_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Zero(), point_query.GetDistance(&target));

  S2ClosestEdgeQuery full_query(full_polygon_index.get());
  full_query.mutable_options()->set_include_interiors(true);
  EXPECT_EQ(S1ChordAngle::Zero(), full_query.GetDistance(&target));
}

TEST(S2ClosestEdgeQuery, IsConservativeDistanceLess) {
  // Test
  int num_tested = 0;
  int num_conservative_needed = 0;
  auto& rnd = S2Testing::rnd;
  for (int iter = 0; iter < 1000; ++iter) {
    rnd.Reset(iter + 1);  // Easier to reproduce a specific case.
    S2Point x = S2Testing::RandomPoint();
    S2Point dir = S2Testing::RandomPoint();
    S1Angle r = S1Angle::Radians(M_PI * pow(1e-30, rnd.RandDouble()));
    S2Point y = S2::InterpolateAtDistance(r, x, dir);
    S1ChordAngle limit(r);
    if (s2pred::CompareDistance(x, y, limit) < 0) {
      S2ShapeIndex index;
      index.Add(make_unique<S2PointVectorShape>(vector<S2Point>({x})));
      S2ClosestEdgeQuery query(&index);
      S2ClosestEdgeQuery::PointTarget target(y);
      EXPECT_TRUE(query.IsConservativeDistanceLess(&target, limit));
      ++num_tested;
      if (!query.IsDistanceLess(&target, limit)) ++num_conservative_needed;
    }
  }
  // Verify that in most test cases, the distance between the target points
  // was close to the desired value.  Also verify that at least in some test
  // cases, the conservative distance test was actually necessary.
  EXPECT_GE(num_tested, 300);
  EXPECT_LE(num_tested, 700);
  EXPECT_GE(num_conservative_needed, 25);
}

// An abstract class that adds edges to an S2ShapeIndex for benchmarking.
class ShapeIndexFactory {
 public:
  virtual ~ShapeIndexFactory() {}

  // Requests that approximately "num_edges" edges located within the given
  // S2Cap bound should be added to "index".
  virtual void AddEdges(const S2Cap& index_cap, int num_edges,
                        S2ShapeIndex* index) const = 0;
};

// Generates a regular loop that approximately fills the given S2Cap.
//
// Regular loops are nearly the worst case for distance calculations, since
// many edges are nearly equidistant from any query point that is not
// immediately adjacent to the loop.
class RegularLoopShapeIndexFactory : public ShapeIndexFactory {
 public:
  void AddEdges(const S2Cap& index_cap, int num_edges,
                S2ShapeIndex* index) const override {
    index->Add(make_unique<S2Loop::OwningShape>(S2Loop::MakeRegularLoop(
        index_cap.center(), index_cap.GetRadius(), num_edges)));
  }
};

// Generates a fractal loop that approximately fills the given S2Cap.
class FractalLoopShapeIndexFactory : public ShapeIndexFactory {
 public:
  void AddEdges(const S2Cap& index_cap, int num_edges,
                S2ShapeIndex* index) const override {
    S2Testing::Fractal fractal;
    fractal.SetLevelForApproxMaxEdges(num_edges);
    index->Add(make_unique<S2Loop::OwningShape>(
        fractal.MakeLoop(S2Testing::GetRandomFrameAt(index_cap.center()),
                         index_cap.GetRadius())));
  }
};

// Generates a cloud of points that approximately fills the given S2Cap.
class PointCloudShapeIndexFactory : public ShapeIndexFactory {
 public:
  void AddEdges(const S2Cap& index_cap, int num_edges,
                S2ShapeIndex* index) const override {
    vector<S2Point> points;
    for (int i = 0; i < num_edges; ++i) {
      points.push_back(S2Testing::SamplePoint(index_cap));
    }
    index->Add(make_unique<S2PointVectorShape>(std::move(points)));
  }
};

// The approximate radius of S2Cap from which query edges are chosen.
static const S1Angle kRadius = S2Testing::KmToAngle(10);

// An approximate bound on the distance measurement error for "reasonable"
// distances (say, less than Pi/2) due to using S1ChordAngle.
static double kChordAngleError = 1e-15;

using Result = pair<S1Angle, s2shapeutil::ShapeEdgeId>;

// Converts to the format required by CheckDistanceResults() in s2testing.h
vector<Result> ConvertResults(const vector<S2ClosestEdgeQuery::Result>& edges) {
  vector<Result> results;
  for (const auto& edge : edges) {
    results.push_back(
        make_pair(edge.distance.ToAngle(),
                  s2shapeutil::ShapeEdgeId(edge.shape_id, edge.edge_id)));
  }
  return results;
}

// Use "query" to find the closest edge(s) to the given target, then convert
// the query results into two parallel vectors, one for distances and one for
// (shape_id, edge_id) pairs.  Also verify that the results satisfy the search
// criteria.
static void GetClosestEdges(S2ClosestEdgeQuery::Target* target,
                            S2ClosestEdgeQuery *query,
                            vector<S2ClosestEdgeQuery::Result>* edges) {
  query->FindClosestEdges(target, edges);
  EXPECT_LE(edges->size(), query->options().max_edges());
  if (query->options().max_distance() ==
      S2ClosestEdgeQuery::Distance::Infinity()) {
    int min_expected = min(query->options().max_edges(),
                           s2shapeutil::GetNumEdges(query->index()));
    if (!query->options().include_interiors()) {
      // We can predict exactly how many edges should be returned.
      EXPECT_EQ(min_expected, edges->size());
    } else {
      // All edges should be returned, and possibly some shape interiors.
      EXPECT_LE(min_expected, edges->size());
    }
  }
  for (const auto& edge : *edges) {
    // Check that the edge satisfies the max_distance() condition.
    EXPECT_LE(edge.distance, query->options().max_distance());
  }
}

static S2ClosestEdgeQuery::Result TestFindClosestEdges(
    S2ClosestEdgeQuery::Target* target, S2ClosestEdgeQuery *query) {
  vector<S2ClosestEdgeQuery::Result> expected, actual;
  query->mutable_options()->set_use_brute_force(true);
  GetClosestEdges(target, query, &expected);
  query->mutable_options()->set_use_brute_force(false);
  GetClosestEdges(target, query, &actual);
  EXPECT_TRUE(CheckDistanceResults(ConvertResults(expected),
                                   ConvertResults(actual),
                                   query->options().max_edges(),
                                   query->options().max_distance().ToAngle(),
                                   query->options().max_error().ToAngle()))
      << "max_edges=" << query->options().max_edges()
      << ", max_distance=" << query->options().max_distance()
      << ", max_error=" << query->options().max_error();

  if (expected.empty()) return S2ClosestEdgeQuery::Result();

  // Note that when options.max_error() > 0, expected[0].distance may not be
  // the minimum distance.  It is never larger by more than max_error(), but
  // the actual value also depends on max_edges().
  //
  // Here we verify that GetDistance() and IsDistanceLess() return results
  // that are consistent with the max_error() setting.
  S1ChordAngle max_error = query->options().max_error();
  S1ChordAngle min_distance = expected[0].distance;
  EXPECT_LE(query->GetDistance(target), min_distance + max_error);

  // Test IsDistanceLess().
  EXPECT_FALSE(query->IsDistanceLess(target, min_distance - max_error));
  EXPECT_TRUE(query->IsDistanceLess(target, min_distance.Successor()));

  // Return the closest edge result so that we can also test Project.
  return expected[0];
}

// The running time of this test is proportional to
//    (num_indexes + num_queries) * num_edges.
// (Note that every query is checked using the brute force algorithm.)
static void TestWithIndexFactory(const ShapeIndexFactory& factory,
                                 int num_indexes, int num_edges,
                                 int num_queries) {
  // Build a set of S2ShapeIndexes containing the desired geometry.
  vector<S2Cap> index_caps;
  vector<unique_ptr<S2ShapeIndex>> indexes;
  for (int i = 0; i < num_indexes; ++i) {
    S2Testing::rnd.Reset(FLAGS_s2_random_seed + i);
    index_caps.push_back(S2Cap(S2Testing::RandomPoint(), kRadius));
    indexes.emplace_back(new S2ShapeIndex);
    factory.AddEdges(index_caps.back(), num_edges, indexes.back().get());
  }
  for (int i = 0; i < num_queries; ++i) {
    S2Testing::rnd.Reset(FLAGS_s2_random_seed + i);
    int i_index = S2Testing::rnd.Uniform(num_indexes);
    const S2Cap& index_cap = index_caps[i_index];

    // Choose query points from an area approximately 4x larger than the
    // geometry being tested.
    S1Angle query_radius = 2 * index_cap.GetRadius();
    S2Cap query_cap(index_cap.center(), query_radius);
    S2ClosestEdgeQuery query(indexes[i_index].get());

    // Occasionally we don't set any limit on the number of result edges.
    // (This may return all edges if we also don't set a distance limit.)
    if (!S2Testing::rnd.OneIn(5)) {
      query.mutable_options()->set_max_edges(1 + S2Testing::rnd.Uniform(10));
    }
    // We set a distance limit 2/3 of the time.
    if (!S2Testing::rnd.OneIn(3)) {
      query.mutable_options()->set_max_distance(
          S2Testing::rnd.RandDouble() * query_radius);
    }
    if (S2Testing::rnd.OneIn(2)) {
      // Choose a maximum error whose logarithm is uniformly distributed over
      // a reasonable range, except that it is sometimes zero.
      query.mutable_options()->set_max_error(S1Angle::Radians(
          pow(1e-4, S2Testing::rnd.RandDouble()) * query_radius.radians()));
    }
    query.mutable_options()->set_include_interiors(S2Testing::rnd.OneIn(2));
    int target_type = S2Testing::rnd.Uniform(4);
    if (target_type == 0) {
      // Find the edges closest to a given point.
      S2Point point = S2Testing::SamplePoint(query_cap);
      S2ClosestEdgeQuery::PointTarget target(point);
      auto closest = TestFindClosestEdges(&target, &query);
      if (!closest.distance.is_infinity()) {
        // Also test the Project method.
        EXPECT_NEAR(
            closest.distance.ToAngle().radians(),
            S1Angle(point, query.Project(point, closest)).radians(),
            kChordAngleError);
      }
    } else if (target_type == 1) {
      // Find the edges closest to a given edge.
      S2Point a = S2Testing::SamplePoint(query_cap);
      S2Point b = S2Testing::SamplePoint(
          S2Cap(a, pow(1e-4, S2Testing::rnd.RandDouble()) * query_radius));
      S2ClosestEdgeQuery::EdgeTarget target(a, b);
      TestFindClosestEdges(&target, &query);
    } else if (target_type == 2) {
      // Find the edges closest to a given cell.
      int min_level = S2::kMaxDiag.GetLevelForMaxValue(query_radius.radians());
      int level = min_level + S2Testing::rnd.Uniform(
          S2CellId::kMaxLevel - min_level + 1);
      S2Point a = S2Testing::SamplePoint(query_cap);
      S2Cell cell(S2CellId(a).parent(level));
      S2ClosestEdgeQuery::CellTarget target(cell);
      TestFindClosestEdges(&target, &query);
    } else {
      DCHECK_EQ(3, target_type);
      // Use another one of the pre-built indexes as the target.
      int j_index = S2Testing::rnd.Uniform(num_indexes);
      S2ClosestEdgeQuery::ShapeIndexTarget target(indexes[j_index].get());
      target.set_include_interiors(S2Testing::rnd.OneIn(2));
      TestFindClosestEdges(&target, &query);
    }
  }
}

static const int kNumIndexes = 50;
static const int kNumEdges = 100;
static const int kNumQueries = 200;

TEST(S2ClosestEdgeQuery, CircleEdges) {
  TestWithIndexFactory(RegularLoopShapeIndexFactory(),
                       kNumIndexes, kNumEdges, kNumQueries);
}

TEST(S2ClosestEdgeQuery, FractalEdges) {
  TestWithIndexFactory(FractalLoopShapeIndexFactory(),
                       kNumIndexes, kNumEdges, kNumQueries);
}

TEST(S2ClosestEdgeQuery, PointCloudEdges) {
  TestWithIndexFactory(PointCloudShapeIndexFactory(),
                       kNumIndexes, kNumEdges, kNumQueries);
}


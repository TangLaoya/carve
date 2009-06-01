// Begin License:
// Copyright (C) 2006-2008 Tobias Sargeant (tobias.sargeant@gmail.com).
// All rights reserved.
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL2 included in the packaging of
// this file.
//
// This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
// INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE.
// End:


#if defined(HAVE_CONFIG_H)
#  include <carve_config.h>
#endif

#include <carve/geom2d.hpp>
#include <carve/math.hpp>

#include <algorithm>
#include <iostream>

#include <assert.h>

namespace carve {
  namespace geom2d {

    namespace {
    }

    LineIntersectionInfo lineSegmentIntersection(const P2 &l1v1, const P2 &l1v2,
                                                 const P2 &l2v1, const P2 &l2v2) {
      if (carve::geom::equal(l1v1, l1v2) || carve::geom::equal(l2v1, l2v2)) {
        throw carve::exception("bad args");
      }
  
      if (std::max(l1v1.x, l1v2.x) < std::min(l2v1.x, l2v2.x) - EPSILON ||
          std::max(l2v1.x, l2v2.x) < std::min(l1v1.x, l1v2.x) - EPSILON) {
        return LineIntersectionInfo(NO_INTERSECTION);
      }

      double dx13 = l1v1.x - l2v1.x;
      double dy13 = l1v1.y - l2v1.y;
      double dx43 = l2v2.x - l2v1.x;
      double dy43 = l2v2.y - l2v1.y;
      double dx21 = l1v2.x - l1v1.x;
      double dy21 = l1v2.y - l1v1.y;
      double ua_n = dx43 * dy13 - dy43 * dx13;
      double ub_n = dx21 * dy13 - dy21 * dx13;
      double u_d  = dy43 * dx21 - dx43 * dy21;

      if (carve::math::ZERO(u_d)) {
        if (carve::math::ZERO(ua_n)) {
          if (carve::geom::equal(l1v2, l2v1)) {
            return LineIntersectionInfo(INTERSECTION_PP, l1v2, 1, 2);
          }
          if (carve::geom::equal(l1v1, l2v2)) {
            return LineIntersectionInfo(INTERSECTION_PP, l1v1, 0, 4);
          }
          if (l1v2.x > l2v1.x && l1v1.x < l2v2.x) {
            return LineIntersectionInfo(COLINEAR);
          }
        }
        return LineIntersectionInfo(NO_INTERSECTION);
      }

      double ua = ua_n / u_d;
      double ub = ub_n / u_d;

      if (-EPSILON <= ua && ua <= 1.0 + EPSILON && -EPSILON <= ub && ub <= 1.0 + EPSILON) {
        double x = l1v1.x + ua * (l1v2.x - l1v1.x);
        double y = l1v1.y + ua * (l1v2.y - l1v1.y);

        P2 p = carve::geom::VECTOR(x, y);

        double d1 = distance2(p, l1v1);
        double d2 = distance2(p, l1v2);
        double d3 = distance2(p, l2v1);
        double d4 = distance2(p, l2v2);

        int n = -1;

        if (std::min(d1, d2) < EPSILON2) {
          if (d1 < d2) {
            p = l1v1; n = 0;
          } else {
            p = l1v2; n = 1;
          }
          if (std::min(d3, d4) < EPSILON2) {
            if (d3 < d4) {
              return LineIntersectionInfo(INTERSECTION_PP, p, n, 2);
            } else {
              return LineIntersectionInfo(INTERSECTION_PP, p, n, 3);
            }
          } else {
            return LineIntersectionInfo(INTERSECTION_PL, p, n, -1);
          }
        } else if (std::min(d3, d4) < EPSILON2) {
          if (d3 < d4) {
            return LineIntersectionInfo(INTERSECTION_LP, l2v1, -1, 2);
          } else {
            return LineIntersectionInfo(INTERSECTION_LP, l2v2, -1, 3);
          }
        } else {
          return LineIntersectionInfo(INTERSECTION_LL, p, -1, -1);
        }
      }
      return LineIntersectionInfo(NO_INTERSECTION);
    }

    LineIntersectionInfo lineSegmentIntersection(const LineSegment2 &l1,
                                                 const LineSegment2 &l2) {
      return lineSegmentIntersection(l1.v1, l1.v2, l2.v1, l2.v2);
    }

    double signedArea(const P2Vector &points) {
      return signedArea(points, p2_adapt_ident());
    }

    bool pointInPolySimple(const P2Vector &points, const P2 &p) {
      return pointInPolySimple(points, p2_adapt_ident(), p);
    }

    PolyInclusionInfo pointInPoly(const P2Vector &points, const P2 &p) {
      return pointInPoly(points, p2_adapt_ident(), p);
    }

    int lineSegmentPolyIntersections(const P2Vector &points,
                                     LineSegment2 line,
                                     std::vector<PolyIntersectionInfo> &out) {
      int count = 0;

      if (line.v2 < line.v1) { line.flip(); }
      out.clear();

      for (P2Vector::size_type i = 0, l = points.size(); i < l; i++) {
        P2Vector::size_type j = (i + 1) % l;
        LineIntersectionInfo e =
          lineSegmentIntersection(LineSegment2(points[i], points[j]), line);
    
        switch (e.iclass) {
        case INTERSECTION_PL: {
          out.push_back(PolyIntersectionInfo(INTERSECT_EDGE, e.ipoint, i));
          count++;
          break;
        }
        case INTERSECTION_PP: {
          out.push_back(PolyIntersectionInfo(INTERSECT_VERTEX, e.ipoint, i + e.p2 - 2));
          count++;
          break;
        }
        case INTERSECTION_LP: {
          out.push_back(PolyIntersectionInfo(INTERSECT_VERTEX, e.ipoint, i + e.p2 - 2));
          count++;
          break;
        }
        case INTERSECTION_LL: {
          out.push_back(PolyIntersectionInfo(INTERSECT_EDGE, e.ipoint, i));
          count++;
          break;
        }
        case COLINEAR: {
          int n1 = (int)i, n2 = (int)j;
          P2 q1 = points[i], q2 = points[j];

          if (q2 < q1) { std::swap(q1, q2); std::swap(n1, n2); }

          if (equal(q1, line.v1)) {
            out.push_back(PolyIntersectionInfo(INTERSECT_VERTEX, q1, n1));
          } else if (q1.x < line.v1.x) {
            out.push_back(PolyIntersectionInfo(INTERSECT_EDGE, line.v1, i));
          } else {
            out.push_back(PolyIntersectionInfo(INTERSECT_VERTEX, q1, n1));
          }
          if (equal(q2, line.v2)) {
            out.push_back(PolyIntersectionInfo(INTERSECT_VERTEX, q2, n2));
          } else if (line.v2.x < q2.x) {
            out.push_back(PolyIntersectionInfo(INTERSECT_EDGE, line.v2, i));
          } else {
            out.push_back(PolyIntersectionInfo(INTERSECT_VERTEX, q2, n2));
          }

          count += 2;

          break;
        }
        default:
          break;
        }
      }
      return count;
    }
 
    struct FwdSort {
      bool operator()(const PolyIntersectionInfo &a,
                      const PolyIntersectionInfo &b) const {
        return a.ipoint < b.ipoint;
      }
    };

    struct RevSort {
      bool operator()(const PolyIntersectionInfo &a,
                      const PolyIntersectionInfo &b) const {
        return a.ipoint < b.ipoint;
      }
    };

    int sortedLineSegmentPolyIntersections(const P2Vector &points,
                                           LineSegment2 line,
                                           std::vector<PolyIntersectionInfo> &out) {

      bool swapped = line.v2 < line.v1;

      int count = lineSegmentPolyIntersections(points, line, out);
      if (swapped) {
        std::sort(out.begin(), out.end(), RevSort());
      } else {
        std::sort(out.begin(), out.end(), FwdSort());
      }
      return count;
    }

    bool pickContainedPoint(const std::vector<P2> &poly, P2 &result) {
      return pickContainedPoint(poly, p2_adapt_ident(), result);
    }

  }
}
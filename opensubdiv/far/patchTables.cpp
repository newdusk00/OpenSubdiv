//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#include "../far/patchTables.h"

#include <cstring>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

static void
getBSplineWeights(float t, float point[4], float deriv[3]) {

    // The weights for the four uniform cubic B-Spline basis functions are:
    // (1/6)(1 - t)^3
    // (1/6)(3t^3 - 6t^2 + 4)
    // (1/6)(-3t^3 + 3t^2 + 3t + 1)
    // (1/6)t^3

    float t2 = t*t,
          t3 = 3*t2*t,
          w0 = 1 - t;

    assert(point);
    point[0] = (w0*w0*w0) / 6.0f;
    point[1] = (t3 - 6.0f*t2 + 4.0f) / 6.0f;
    point[2] = (3.0f*t2 - t3 + 3.0f*t + 1.0f) / 6.0f;
    point[3] = t3 / 18.0f;


    // The weights for the three uniform quadratic basis functions are:
    // (1/2)(1-t)^2
    // (1/2)(1 + 2t - 2t^2)
    // (1/2)t^2

    if (deriv) {
        deriv[0] = 0.5f * w0 * w0;
        deriv[1] = 0.5f + t - t2;
        deriv[2] = 0.5f * t2;
    }
}

namespace Far {

void
PatchTables::getBSplineWeightsAtUV(PatchParam::BitField bits, float s, float t,
    float point[16], float deriv1[16], float deriv2[16]) {

    int const rots[4][16] =
        { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
          { 12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3 },
          { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
          { 3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12 } };

    assert(bits.GetRotation()<4);
    int const * r = rots[bits.GetRotation()];

    float sWeights[4], tWeights[4], d1Weights[3], d2Weights[3];

    getBSplineWeights(s, point ? sWeights : 0, deriv1 ? d1Weights : 0);
    getBSplineWeights(t, point ? tWeights : 0, deriv2 ? d2Weights : 0);

    if (point) {

        // Compute the tensor product weight corresponding to each control
        // vertex
        memset(point,  0, 16*sizeof(float));
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                point[r[4*i+j]] += sWeights[j] * tWeights[i];
            }
        }
    }

    if (deriv1 and deriv2) {

        // Compute the tangent stencil. This is done by taking the tensor
        // product between the quadratic weights computed for u and the cubic
        // weights computed for v. The stencil is constructed using
        // differences between consecutive vertices in each row (i.e.
        // in the u direction).
        memset(deriv1, 0, 16*sizeof(float));
        for (int i = 0; i < 4; ++i) {
            float prevWeight = 0.0f;
            for (int j = 0; j < 3; ++j) {
                float weight = d1Weights[j]*tWeights[i];
                deriv1[r[4*i+j]] += prevWeight - weight;
                prevWeight = weight;
            }
            deriv1[r[4*i+3]]+=prevWeight;
        }

        memset(deriv2, 0, 16*sizeof(float));
        for (int j = 0; j < 4; ++j) {
            float prevWeight = 0.0f;
            for (int i = 0; i < 3; ++i) {
                float weight = sWeights[j]*d2Weights[i];
                deriv2[r[4*i+j]]+=prevWeight - weight;
                prevWeight = weight;
            }
            deriv2[r[12+j]] += prevWeight;
        }
    }
}

} // end namespace Far

} // end namespace OPENSUBDIV_VERSION
} // end namespace OpenSubdiv

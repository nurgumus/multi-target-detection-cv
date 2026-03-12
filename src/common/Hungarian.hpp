#pragma once
// Header-only Hungarian algorithm (Kuhn-Munkres) for minimum-cost assignment.
// Operates on a square or rectangular cost matrix (rows = workers, cols = jobs).
// Returns assignment[i] = j for row i assigned to column j, or -1 if unassigned.
//
// Usage:
//   Hungarian h;
//   auto assignment = h.solve(costMatrix);  // costMatrix[row][col]

#include <vector>
#include <limits>
#include <algorithm>

class Hungarian {
public:
    // Solve the assignment problem on costMatrix (NxM, N <= M recommended).
    // Returns a vector of size N: assignment[i] = column assigned to row i.
    std::vector<int> solve(const std::vector<std::vector<float>>& costMatrix) {
        if (costMatrix.empty()) return {};
        const int N = static_cast<int>(costMatrix.size());
        const int M = static_cast<int>(costMatrix[0].size());
        // Pad to square
        const int sz = std::max(N, M);
        const float INF = std::numeric_limits<float>::max() / 2.f;

        std::vector<std::vector<float>> C(sz, std::vector<float>(sz, 0.f));
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < M; ++j)
                C[i][j] = costMatrix[i][j];

        // Hungarian algorithm (O(n^3))
        std::vector<float> u(sz + 1), v(sz + 1);
        std::vector<int>   p(sz + 1, 0), way(sz + 1, 0);

        for (int i = 1; i <= sz; ++i) {
            p[0] = i;
            int j0 = 0;
            std::vector<float> minVal(sz + 1, INF);
            std::vector<bool>  used(sz + 1, false);

            do {
                used[j0] = true;
                int    i0 = p[j0], j1 = -1;
                float  delta = INF;
                for (int j = 1; j <= sz; ++j) {
                    if (!used[j]) {
                        float cur = C[i0 - 1][j - 1] - u[i0] - v[j];
                        if (cur < minVal[j]) {
                            minVal[j] = cur;
                            way[j] = j0;
                        }
                        if (minVal[j] < delta) {
                            delta = minVal[j];
                            j1 = j;
                        }
                    }
                }
                for (int j = 0; j <= sz; ++j) {
                    if (used[j]) {
                        u[p[j]] += delta;
                        v[j] -= delta;
                    } else {
                        minVal[j] -= delta;
                    }
                }
                j0 = j1;
            } while (p[j0] != 0);

            do {
                int j1 = way[j0];
                p[j0] = p[j1];
                j0 = j1;
            } while (j0);
        }

        // Extract result for original rows
        std::vector<int> result(N, -1);
        for (int j = 1; j <= sz; ++j) {
            if (p[j] >= 1 && p[j] <= N && j <= M)
                result[p[j] - 1] = j - 1;
        }
        return result;
    }
};

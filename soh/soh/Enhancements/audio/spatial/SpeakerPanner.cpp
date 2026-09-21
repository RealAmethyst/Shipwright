#include "SpeakerPanner.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace SpatialAudio {
namespace {
constexpr float Pi = 3.14159265358979323846f;
constexpr std::array<Direction, SpeakerPanner::Channels> Speakers{{
    {-0.5f, 0, -0.8660254f}, {0.5f, 0, -0.8660254f}, {0, 0, -1}, {0, 0, 0},
    {-1, 0, 0}, {1, 0, 0}, {-0.5f, 0, 0.8660254f}, {0.5f, 0, 0.8660254f},
    {-0.5f, 0.70710678f, -0.5f}, {0.5f, 0.70710678f, -0.5f},
    {-0.5f, 0.70710678f, 0.5f}, {0.5f, 0.70710678f, 0.5f}
}};
float Dot(Direction a, Direction b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Direction Sub(Direction a, Direction b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Direction Scale(Direction a, float k) { return {a.x * k, a.y * k, a.z * k}; }
Direction Cross(Direction a, Direction b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
}

SpeakerPanner::SpeakerPanner() {
    std::set<uint16_t> faces;
    for (size_t a = 0; a < Channels; ++a) {
        if (a == 3) continue;
        for (size_t b = a + 1; b < Channels; ++b) {
            if (b == 3) continue;
            for (size_t c = b + 1; c < Channels; ++c) {
                if (c == 3) continue;
                const auto va = Speakers[a], vb = Speakers[b], vc = Speakers[c];
                const float determinant = Dot(va, Cross(vb, vc));
                if (std::abs(determinant) < 0.00001f) continue;
                auto normal = Cross(Sub(vb, va), Sub(vc, va));
                if (Dot(normal, va) < 0) normal = Scale(normal, -1);
                const float plane = Dot(normal, va);
                bool hull = true;
                for (size_t other = 0; other < Channels; ++other)
                    if (other != 3 && Dot(normal, Speakers[other]) > plane + 0.00001f) hull = false;
                if (!hull) continue;
                uint16_t mask = 0;
                std::vector<size_t> vertices;
                Direction center{};
                for (size_t i = 0; i < Channels; ++i) {
                    if (i == 3 || std::abs(Dot(normal, Speakers[i]) - plane) > 0.00001f) continue;
                    mask |= 1 << i;
                    vertices.push_back(i);
                    center.x += Speakers[i].x;
                    center.y += Speakers[i].y;
                    center.z += Speakers[i].z;
                }
                if (!faces.insert(mask).second) continue;
                center = Scale(center, 1.0f / vertices.size());
                const auto axisX = Sub(Speakers[vertices[0]], center);
                const auto axisY = Cross(Scale(normal, 1 / std::sqrt(Dot(normal, normal))), axisX);
                std::sort(vertices.begin(), vertices.end(), [&](size_t i, size_t j) {
                    const auto vi = Sub(Speakers[i], center), vj = Sub(Speakers[j], center);
                    return std::atan2(Dot(vi, axisY), Dot(vi, axisX)) < std::atan2(Dot(vj, axisY), Dot(vj, axisX));
                });
                // One consistent triangulation per face. Overlapping triangles on
                // a coplanar quad would make moving sources jump between diagonals.
                for (size_t i = 1; i + 1 < vertices.size(); ++i) {
                    const auto x = Speakers[vertices[0]], y = Speakers[vertices[i]], z = Speakers[vertices[i + 1]];
                    const float det = Dot(x, Cross(y, z));
                    if (std::abs(det) < 0.00001f) continue;
                    triangles.push_back({{vertices[0], vertices[i], vertices[i + 1]},
                        {Scale(Cross(y, z), 1 / det), Scale(Cross(z, x), 1 / det), Scale(Cross(x, y), 1 / det)},
                        std::min({Dot(x, y), Dot(x, z), Dot(y, z)})});
                }
            }
        }
    }
    std::sort(triangles.begin(), triangles.end(), [](const auto& a, const auto& b) { return a.locality > b.locality; });
}

bool SpeakerPanner::Compute(Direction direction, Gains& gains) const {
    gains.fill(0);
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) || !std::isfinite(direction.z)) return false;
    const float length = std::sqrt(Dot(direction, direction));
    if (length < 0.00001f) return false;
    direction = Scale(direction, 1 / length);
    if (direction.y > 0.00001f) {
        for (const auto& triangle : triangles) {
            std::array<float, 3> weights;
            bool inside = true;
            for (size_t i = 0; i < weights.size(); ++i) {
                weights[i] = Dot(triangle.inverse[i], direction);
                if (weights[i] < -0.00001f) inside = false;
                weights[i] = std::max(0.0f, weights[i]);
            }
            if (!inside) continue;
            const float normalization = std::sqrt(weights[0] * weights[0] + weights[1] * weights[1] + weights[2] * weights[2]);
            if (normalization < 0.00001f) continue;
            for (size_t i = 0; i < weights.size(); ++i) gains[triangle.indices[i]] = weights[i] / normalization;
            return true;
        }
        return false;
    }
    // A 7.1.4 bed has no speakers below the listener. Preserve azimuth on its
    // horizontal ring. Headphone HRTF retains the full above/below distinction.
    constexpr std::array<size_t, 7> order{6, 4, 0, 2, 1, 5, 7};
    float angle = std::atan2(direction.x, -direction.z);
    for (size_t i = 0; i < order.size(); ++i) {
        const size_t a = order[i], b = order[(i + 1) % order.size()];
        float start = std::atan2(Speakers[a].x, -Speakers[a].z);
        float end = std::atan2(Speakers[b].x, -Speakers[b].z);
        if (end <= start) end += 2 * Pi;
        float current = angle;
        if (current < start) current += 2 * Pi;
        if (current > end) continue;
        const float first = std::max(0.0f, std::sin(end - current));
        const float second = std::max(0.0f, std::sin(current - start));
        const float normalization = std::sqrt(first * first + second * second);
        if (normalization < 0.00001f) return false;
        gains[a] = first / normalization;
        gains[b] = second / normalization;
        return true;
    }
    return false;
}

} // namespace SpatialAudio

#include "SohStatsWindow.h"
#include "soh/NativeOptions/NativeOptions.h"
#include <libultraship/libultraship.h>

void InitializePerformanceStats() {
    namespace N = NativeOptions;
    N::RegisterPage("Stats##Soh", [] {
        return N::MakePage("advanced/stats", N::Text("performance_stats"), [] {
            auto platform = N::Action("platform", N::Text("platform"), [] { N::ReadCurrentDescription(); });
#if defined(_WIN32)
            platform.value = "Windows";
#elif defined(__IOS__)
            platform.value = "iOS";
#elif defined(__APPLE__)
            platform.value = "macOS";
#elif defined(__linux__)
            platform.value = "Linux";
#else
            platform.value = N::Text("unknown");
#endif
            auto status = N::Action("status", N::Text("splits_status"), [] { N::ReadCurrentDescription(); });
            status.value = fmt::format("{:.3f} ms/frame ({:.1f} FPS)", ImGui::GetIO().DeltaTime * 1000.0f, ImGui::GetIO().Framerate);
            return std::vector<N::Row>{platform, status};
        });
    }, N::Text("performance_stats"));
}

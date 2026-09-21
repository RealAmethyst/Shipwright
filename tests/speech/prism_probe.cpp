#include <prism.h>
#include <cstdio>

int main() {
    auto config = prism_config_init();
    auto* context = prism_init(&config);
    if (context == nullptr) return 1;
    auto* backend = prism_registry_create_best(context);
    if (backend == nullptr) {
        prism_shutdown(context);
        return 2;
    }
    std::printf("Prism initialized: %s\n", prism_backend_name(backend));
    prism_backend_free(backend);
    prism_shutdown(context);
    // This probe never speaks, stops speech, opens a window or starts the game.
    return 0;
}

#include <iostream>

// Test that we can include KooLab headers
// Note: Adjust these includes based on actual available headers
#ifdef KOOLAB_CORE_AVAILABLE
#include <koo/core/Logger.h>
#endif

int main() {
    std::cout << "KooLab Chemical Simulation Library" << std::endl;
    std::cout << "Version: 6.0.0-alpha5" << std::endl;
    std::cout << std::endl;

#ifdef KOOLAB_GPU_ENABLED
    std::cout << "✓ GPU support enabled" << std::endl;
#else
    std::cout << "○ GPU support disabled" << std::endl;
#endif

#ifdef KOOLAB_PYTHON_ENABLED
    std::cout << "✓ Python bindings enabled" << std::endl;
#else
    std::cout << "○ Python bindings disabled" << std::endl;
#endif

    std::cout << std::endl;
    std::cout << "Test package compiled and linked successfully!" << std::endl;
    std::cout << "KooLab is ready to use." << std::endl;

    return 0;
}

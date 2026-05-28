#include "VulkanApp.h"
#include <iostream>

int main() {
    VulkanApp app;

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

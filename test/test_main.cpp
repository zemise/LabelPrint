#include "test_utils.h"
#include "labelprint/version.h"

int main() {
    std::cout << "=== Label Print Library Tests v" << LABELPRINT_VERSION_STRING << " ===" << std::endl;
    int passed = 0;
    for (auto& t : testRegistry()) {
        try {
            t.fn();
            ++passed;
        } catch (const std::exception& e) {
            std::cerr << "\n  EXCEPTION: " << e.what() << std::endl;
            return 1;
        }
    }
    std::cout << "\n" << passed << "/" << testRegistry().size()
              << " tests passed." << std::endl;
    return 0;
}

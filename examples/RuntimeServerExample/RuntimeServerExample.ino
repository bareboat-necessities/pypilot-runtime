#include "RuntimeServerExampleCore.hpp"

#if defined(ARDUINO)
void setup() {
    setup_runtime_example();
}

void loop() {
    run_runtime_example_once();
}
#endif

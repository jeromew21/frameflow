set -xe

cmake -S . -B build -DFRAMEFLOW_BUILD_TESTS=ON
cmake --build build

./build/tests/frameflow_sdl_test

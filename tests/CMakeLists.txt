find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(frameflow_sdl_test
    sdl_smoke_test.cpp
)

target_link_libraries(frameflow_sdl_test
    PRIVATE
        frameflow::frameflow
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
)

target_compile_features(frameflow_sdl_test PRIVATE cxx_std_17)

add_executable(frameflow_layout_tests
        frameflow_layout_tests.cpp
)

target_link_libraries(frameflow_layout_tests
        PRIVATE
        frameflow::frameflow
)

target_compile_features(frameflow_layout_tests PRIVATE cxx_std_17)

add_executable(frameflow_allocation_tests
        frameflow_allocation_test.cpp
)

target_link_libraries(frameflow_allocation_tests
        PRIVATE
        frameflow::frameflow
)

target_compile_features(frameflow_allocation_tests PRIVATE cxx_std_17)

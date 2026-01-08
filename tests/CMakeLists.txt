find_package(SDL2 REQUIRED)

add_executable(frameflow_sdl_test
    sdl_smoke_test.cpp
)

target_link_libraries(frameflow_sdl_test
    PRIVATE
        frameflow::frameflow
        SDL2::SDL2
)

target_compile_features(frameflow_sdl_test PRIVATE cxx_std_17)

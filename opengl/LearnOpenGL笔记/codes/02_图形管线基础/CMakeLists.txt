add_executable(basic_pipeline)
target_compile_features(basic_pipeline PRIVATE cxx_std_17)
target_sources(basic_pipeline
    PRIVATE
        main.cpp
)
target_link_libraries(basic_pipeline
    PRIVATE
        glad glfw
)
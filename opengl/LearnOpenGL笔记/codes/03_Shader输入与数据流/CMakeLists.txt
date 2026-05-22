add_executable(shader_input_and_data_flow)
target_compile_features(shader_input_and_data_flow PRIVATE cxx_std_17)
target_sources(shader_input_and_data_flow
    PRIVATE
        main.cpp
        ShaderProgram.cpp
)
target_link_libraries(shader_input_and_data_flow
    PRIVATE
        glad glfw
)
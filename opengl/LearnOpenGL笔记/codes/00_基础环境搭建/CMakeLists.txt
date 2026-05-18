add_executable(basic_environment)
target_sources(basic_environment
    PRIVATE
        main.cpp
)
target_link_libraries(basic_environment
    PRIVATE
        glad glfw
)
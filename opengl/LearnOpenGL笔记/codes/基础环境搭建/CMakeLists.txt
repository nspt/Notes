add_executable(test_env)
target_sources(test_env
    PRIVATE
        main.cpp
)
target_link_libraries(test_env
    PRIVATE
        glad glfw
)
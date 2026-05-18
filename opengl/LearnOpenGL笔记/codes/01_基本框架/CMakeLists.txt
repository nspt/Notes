add_executable(basic_framework)
target_sources(basic_framework
    PRIVATE
        main.cpp
)
target_link_libraries(basic_framework
    PRIVATE
        glad glfw
)
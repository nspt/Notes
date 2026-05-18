add_executable(basic_window_and_context)
target_sources(basic_window_and_context
    PRIVATE
        main.cpp
)
target_link_libraries(basic_window_and_context
    PRIVATE
        glad glfw
)
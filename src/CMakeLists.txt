# Headless simulation core library (no SFML dependency)
add_library(alife_sim STATIC
    core/Clock.cpp
    ai/NeuralNetwork.cpp
    ai/Sensors.cpp
    entities/Organism.cpp
    entities/Food.cpp
    environment/FoodManager.cpp
    environment/EnvironmentManager.cpp
    simulation/PopulationManager.cpp
    simulation/World.cpp
    simulation/SimulationEngine.cpp
    data/Presets.cpp
    data/SnapshotManager.cpp
)

target_include_directories(alife_sim PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(alife_sim PUBLIC cxx_std_20)

# Rendering library (depends on alife_sim and SFML 3)
add_library(alife_render STATIC
    rendering/Renderer.cpp
)

target_include_directories(alife_render PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(alife_render PUBLIC cxx_std_20)
target_link_libraries(alife_render PUBLIC
    alife_sim
    SFML::Graphics
    SFML::Window
    SFML::System
)

# Main executable
add_executable(alife_app
    main.cpp
)

target_compile_features(alife_app PRIVATE cxx_std_20)
target_link_libraries(alife_app PRIVATE
    alife_render
    alife_sim
)

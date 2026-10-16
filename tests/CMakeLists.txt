# Headless unit tests (links alife_sim only)
add_executable(test_core
    unit/test_core.cpp
)
target_link_libraries(test_core PRIVATE alife_sim)
target_include_directories(test_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME CoreClockTest COMMAND test_core)

add_executable(test_simulation
    unit/test_simulation.cpp
)
target_link_libraries(test_simulation PRIVATE alife_sim)
target_include_directories(test_simulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME SimulationEngineTest COMMAND test_simulation)

add_executable(test_organisms
    unit/test_organisms.cpp
)
target_link_libraries(test_organisms PRIVATE alife_sim)
target_include_directories(test_organisms PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME OrganismPopulationTest COMMAND test_organisms)

add_executable(test_environment
    unit/test_environment.cpp
)
target_link_libraries(test_environment PRIVATE alife_sim)
target_include_directories(test_environment PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME EnvironmentFoodTest COMMAND test_environment)

add_executable(test_reproduction
    unit/test_reproduction.cpp
)
target_link_libraries(test_reproduction PRIVATE alife_sim)
target_include_directories(test_reproduction PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ReproductionInheritanceTest COMMAND test_reproduction)

add_executable(test_evolution
    unit/test_evolution.cpp
)
target_link_libraries(test_evolution PRIVATE alife_sim)
target_include_directories(test_evolution PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME EvolutionDynamicsTest COMMAND test_evolution)

add_executable(test_neural_network
    unit/test_neural_network.cpp
)
target_link_libraries(test_neural_network PRIVATE alife_sim)
target_include_directories(test_neural_network PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME NeuralNetworkTest COMMAND test_neural_network)

add_executable(test_ecosystem
    unit/test_ecosystem.cpp
)
target_link_libraries(test_ecosystem PRIVATE alife_sim)
target_include_directories(test_ecosystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME EcosystemMultiSpeciesTest COMMAND test_ecosystem)

add_executable(test_scientific_observation
    unit/test_scientific_observation.cpp
)
target_link_libraries(test_scientific_observation PRIVATE alife_sim)
target_include_directories(test_scientific_observation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ScientificObservationTest COMMAND test_scientific_observation)

# Performance & Scalability Unit & Regression Tests (Phase 9)
add_executable(test_performance_scalability
    unit/test_performance_scalability.cpp
)
target_link_libraries(test_performance_scalability PRIVATE alife_sim)
target_include_directories(test_performance_scalability PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME PerformanceScalabilityTest COMMAND test_performance_scalability)

# Performance & Scalability Benchmark Suite (Phase 9)
add_executable(benchmark_suite
    benchmarks/benchmark_suite.cpp
)
target_link_libraries(benchmark_suite PRIVATE alife_sim)
target_include_directories(benchmark_suite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Phase 10 — Experimental Laboratory UI, Save/Load & Final Integration
add_executable(test_phase10_lab_integration
    unit/test_phase10_lab_integration.cpp
)
target_link_libraries(test_phase10_lab_integration PRIVATE alife_sim)
target_include_directories(test_phase10_lab_integration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME Phase10LabIntegrationTest COMMAND test_phase10_lab_integration)




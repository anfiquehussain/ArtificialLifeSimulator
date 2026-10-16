# Artificial Life & Evolution Simulator

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/Build-CMake%203.22%2B-064F8C.svg?logo=cmake)](https://cmake.org)
[![SFML 3](https://img.shields.io/badge/Graphics-SFML%203.0.0-8CC445.svg?logo=sfml)](https://www.sfml-dev.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CI Build & Test](https://github.com/anfiquehussain/ArtificialLifeSimulator/actions/workflows/ci.yml/badge.svg)](https://github.com/anfiquehussain/ArtificialLifeSimulator/actions)

A modular, deterministic computational laboratory and evolutionary ecology simulator built in **C++20** with **SFML 3**. Designed for researching emergence, natural selection dynamics, sensorimotor neural behavior, and multi-species ecological stability.

---

## Key Highlights

- **Headless Simulation Core:** The simulation logic (`alife_sim`) is completely decoupled from presentation and rendering (`SFML 3`), enabling 100% headless test automation, deterministic experiment replays, and ultra-high-speed batch benchmarks.
- **Strictly Fixed Timestep ($\Delta t = 1/60\text{s}$):** Physics and biology advance on a deterministic accumulator clock decoupled from graphics refresh rates, preventing spiral-of-death timing jumps.
- **Heritable Trait Evolution:** Continuous traits (`speed`, `basalMetabolismCost`, `foodInteractionRadius`) mutate within physiological boundaries under ecological selection pressure and metabolic trade-offs.
- **Multi-Species Trophic Web:** Herbivores forage for plant pellets while avoiding predators; predators actively hunt herbivores with Lotka–Volterra population oscillations.
- **Neural Sensorimotor Controllers:** Organisms navigate using a feedforward neural network (`tanh` activations) sensing energy reserves, nearest resource/predator headings, and boundary proximities.
- **High-Performance Spatial Partitioning:** Zero-allocation 2D spatial grid (`SpatialGrid`) reduces all-pairs sensing and collision arbitration from $O(N^2)$ to $O(N)$.
- **Batched GPU Rendering:** Transforms thousands of discrete draw calls into 3 pre-allocated vertex array calls, sustaining locked 60 FPS even under heavy entity loads.
- **Scientific Observation Suite:** Real-time multi-series telemetry charts, discrete event ring-buffer logging, and one-click multi-table CSV data exports for external statistical analysis.
- **Interactive 5-Panel Laboratory UI:** Interactive organism inspector, smooth follow-camera, pause/single-step controls, speed multipliers (0.5x–10.0x), and atomic snapshot save/load (`ALIFE_SNAPSHOT_V1`).

---

## Directory Structure

```text
.
├── CMakeLists.txt                  # Root build configuration (FetchContent SFML 3)
├── CMakePresets.json               # Developer and CI build presets
├── LICENSE                         # MIT License
├── README.md                       # Project documentation & user manual
├── .gitignore                      # Git ignore rules for build and telemetry artifacts
├── assets/                         # Optional custom assets & fonts (.gitkeep)
├── data_exports/                   # Output folder for scientific CSV datasets (.gitkeep)
├── docs/
│   ├── ARCHITECTURE.md             # System architecture & ADRs
│   ├── ROADMAP.md                  # Development phase tracker
│   └── EXPERIMENTS.md              # Experiment methodology and parameters
├── include/alife/                  # Public modular C++20 library headers
│   ├── core/                       # Vector2.hpp, Clock.hpp, Rng.hpp, Config.hpp
│   ├── biology/                    # Traits.hpp, EvolutionMetrics.hpp, LineageTracker.hpp
│   ├── entities/                   # Organism.hpp, Food.hpp, EntityId.hpp
│   ├── environment/                # FoodManager.hpp, SpatialGrid.hpp
│   ├── ai/                         # NeuralNetwork.hpp, Sensors.hpp
│   ├── simulation/                 # World.hpp, PopulationManager.hpp, SimulationEngine.hpp
│   ├── data/                       # Presets.hpp, HistoryRecorder.hpp, SnapshotSerializer.hpp
│   └── rendering/                  # Renderer.hpp, UITheme.hpp (SFML 3 batch rendering)
├── src/
│   ├── CMakeLists.txt              # Subdirectory CMake definition for alife_sim & alife_app
│   ├── main.cpp                    # Composition root & main simulation loop
│   ├── core/                       # Foundation implementations (Clock.cpp, etc.)
│   ├── simulation/                 # Simulation logic (World.cpp, PopulationManager.cpp, etc.)
│   ├── data/                       # Persistence (SnapshotSerializer.cpp, etc.)
│   └── rendering/                  # Visualization (Renderer.cpp)
└── tests/
    ├── CMakeLists.txt              # Test suite configuration (CTest)
    ├── TestUtils.hpp               # Headless test assertions and validation helpers
    └── unit/                       # 11 comprehensive automated test suites
```

---

## Scientific Principles & Evolutionary Rules

### 1. Movement Model
$$\vec{P}_{t+\Delta t} = \vec{P}_t + \hat{d} \cdot (s \cdot \Delta t)$$
Where $\hat{d}$ is the normalized direction vector, $s$ is the organism's movement speed (heritable trait modulated by neural throttle), and $\Delta t = 1/60\text{s}$. Organisms reflect specularly at world boundaries.

### 2. Energy Consumption & Foraging
- **Metabolic drainage:**
  $$E_{\text{consumed}} = \left(\text{BasalCost} + \text{MovementCostMultiplier} \cdot s_{\text{eff}}\right) \cdot \Delta t$$
- **Food Consumption:** When an herbivore arrives within distance $d \le \text{foodInteractionRadius}$, it consumes the pellet: $E \leftarrow \min(E + E_{\text{food}}, E_{\text{max}})$. Proximity arbitration ensures the physically closest organism is awarded contested resources.

### 3. Reproduction & Inheritance
- **Eligibility:** Organism is alive, $E \ge \text{reproductionEnergyThreshold}$ ($80.0\text{ units}$), and $\text{cooldownTimer} \le 0.0\text{s}$.
- **Energy Investment:** Parent loses $\text{reproductionEnergyCost}$ ($45.0\text{ units}$) and enters cooldown ($3.0\text{s}$). Offspring receives $\text{offspringInitialEnergy}$ ($35.0\text{ units}$).
- **Mutation:** Heritable traits mutate independently with probability $\text{mutationProbability}$ ($0.20$) by a factor of $\pm \text{mutationMagnitude}$ ($\pm 15\%$), strictly clamped within viable physiological bounds:
  - Speed: $[15, 150]\text{ px/s}$
  - Basal cost: $[0.5, 5.0]\text{ energy/s}$
  - Food interaction radius: $[5, 25]\text{ px}$
- **Lineage:** Offspring records $\text{parentId} = \text{parent.id}$ and $\text{generation} = \text{parent.generation} + 1$. Lineage metrics (lifetime offspring, lifespan, generation) persist across deaths.

### 4. Neural Network Controller Architecture
- **Topology:** 8 input sensors $\rightarrow$ 8 hidden neurons $\rightarrow$ 2 motor outputs with `std::tanh` activation and pre-allocated zero-allocation buffers.
- **Sensory Inputs:**
  - `[0] Energy Ratio`: $E / E_{\text{max}} \in [0.0, 1.0]$.
  - `[1] Relative Target Angle`: Normalized angle to nearest food or prey $[-\pi, \pi] / \pi \in [-1.0, 1.0]$.
  - `[2] Normalized Target Distance`: $\min(1.0, d / \text{perceptionRadius}) \in [0.0, 1.0]$.
  - `[3] Target Detected Flag`: $+1.0$ if target detected, $-1.0$ otherwise.
  - `[4] Boundary Proximity X`: Normalized horizontal world coordinate $[-1.0, 1.0]$.
  - `[5] Boundary Proximity Y`: Normalized vertical world coordinate $[-1.0, 1.0]$.
  - `[6] Threat Detected / Proximity`: Relative heading and distance to nearest predator (herbivore sensing).
  - `[7] Season Factor`: Sinusoidal environmental season cycle factor $[-1.0, 1.0]$.
- **Action Outputs:**
  - `[0] Steering Turn Rate`: Heading delta $\Delta \theta = \text{output}[0] \cdot \pi \cdot \Delta t$.
  - `[1] Speed Throttle`: Dynamic velocity adjustment $s_{\text{eff}} = s \cdot (1.0 + 0.5 \cdot \text{output}[1]) \in [0.5 \cdot s, 1.5 \cdot s]$.

---

## Interactive Laboratory Controls

| Key / Input | Action | Description |
| :--- | :--- | :--- |
| **Space** | Pause / Resume | Freezes simulation ticks while keeping the UI and viewport responsive |
| **S** | Single-Step | Advances the simulation by exactly one fixed physics step ($\Delta t = 1/60\text{s}$) |
| **1, 2, 3, 4, 5** | Speed Multipliers | Sets simulation speed: **0.5x**, **1.0x** (realtime), **2.0x**, **5.0x**, **10.0x** |
| **P** | Cycle Presets | Cycles through the 7 validated scientific experiment presets |
| **R** | Reset Simulation | Resets simulation time, organism population, food resources, and environment |
| **Left Click** | Select Organism | Click on any organism in world space to inspect vital signs and neural state |
| **I** | Toggle Inspector | Shows/hides the real-time organism inspector panel |
| **N** | Next Organism | Cycles inspection focus to the next living organism in the population |
| **F** | Follow Camera | Smoothly locks and centers the arena viewport on the inspected organism |
| **C** | Scientific Charts | Toggles the real-time time-series telemetry chart overlay |
| **G** | Toggle Spatial Grid | Toggles rendering of the $100 \times 100\text{ px}$ spatial partitioning grid |
| **F5** | Quick Save | Atomically serializes complete world state to `quicksave.alife` |
| **F9** | Quick Load | Validates and restores complete simulation state from `quicksave.alife` |
| **E** | Export CSV Data | Exports all 6 structured scientific telemetry datasets to `data_exports/` |
| **Esc** | Exit & Save | Cleanly shuts down the application and automatically exports session data |
| **Right-Click Drag** | Pan Viewport | Smoothly pans across the ecosystem arena |
| **Mouse Wheel** | Zoom | Smoothly zooms between $0.2\times$ and $5.0\times$ magnification |

---

## Experiment Presets

The simulator includes 7 pre-configured, reproducible experimental scenarios:

1. **Basic Evolution:** Balanced foraging environment with moderate mutation and standard population limits.
2. **Resource Scarcity:** Low food capacity and slow regeneration, creating intense competitive selection pressure.
3. **Rapid Reproduction:** Abundant food resources and lower reproduction thresholds for studying lineage explosion and carrying capacities.
4. **High Mutation:** Elevated mutation rate ($\pm 35\%$ variation, $40\%$ probability) to explore phenotypic drift.
5. **Predator–Prey Ecosystem:** Two-tier trophic dynamics with balanced herbivore foraging and predator hunting pressures.
6. **Neural-Network Behaviour:** Wide perception field ($160\text{ px}$) and active steering dynamics emphasizing emergent navigation.
7. **Custom Experiment:** Baseline open sandbox template for tailored experimental parameter exploration.

---

## Scientific Data Export Suite

The simulator writes comprehensive scientific CSV datasets into `data_exports/` on demand (`E` key) or on clean shutdown:

1. `population_history.csv`: Time-series of living populations, growth rates, births, deaths, and starvation vs predation metrics.
2. `species_statistics.csv`: Per-species breakdown of population, demographics, average energy, speed, trait distributions, and extinction times.
3. `trait_statistics.csv`: Detailed parametric and non-parametric trait statistics, quartiles ($Q_1$, $Q_3$), and frequency histograms.
4. `ecosystem_history.csv`: Available food pellets, capacity, consumption rate, energy gained, and seasonal multipliers.
5. `events.csv`: Event logs with timestamps, step counts, entity IDs, species, and interaction details.
6. `experiment_metadata.csv`: Full record of experiment parameters, seed, duration, termination reason, and summary outcomes for reproducibility.

---

## Build & Run Instructions

### Prerequisites
- **C++20 compliant compiler:**
  - Windows: MSVC (Visual Studio 2022 or 2026) with C++ Desktop Development tools
  - Linux: GCC 11+ or Clang 14+
  - macOS: Apple Clang 14+ with Xcode Command Line Tools
- **CMake:** 3.22 or higher
- **Git:** Required by CMake `FetchContent` to automatically clone SFML 3.0.0

---

### Windows 11 (MSVC)

1. Open the **x64 Native Tools Command Prompt for VS 2022 / 2026**.
2. Configure the build with CMake:
   ```cmd
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```
3. Build the simulator executable and test targets:
   ```cmd
   cmake --build build --config Release
   ```
4. Run the automated tests:
   ```cmd
   ctest --test-dir build -C Release --output-on-failure
   ```
5. Launch the application:
   ```cmd
   .\build\src\Release\alife_app.exe
   ```
   *(Or `.\build\src\alife_app.exe` if using NMake / Ninja)*

---

### Linux (Ubuntu / Debian)

1. Install build tools and SFML 3 dependencies:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake git ninja-build \
       libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
       libgl1-mesa-dev libflac-dev libogg-dev libvorbis-dev
   ```
2. Configure and build:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```
3. Run tests and launch:
   ```bash
   ctest --test-dir build --output-on-failure
   ./build/src/alife_app
   ```

---

## Automated Test Suites

The project features 11 automated test suites covering 100% of the core simulation logic, headless execution, determinism, reproduction models, and laboratory integration:

```cmd
ctest --test-dir build --output-on-failure
```

| Test Target | Scope |
| :--- | :--- |
| `CoreClockTest` | Accumulator clock stepping, pausing, clamping, and determinism |
| `SimulationEngineTest` | Engine lifecycle, headless updates, and world resets |
| `OrganismPopulationTest` | Movement physics, metabolic drain, starvation death, and boundary reflection |
| `EnvironmentFoodTest` | Resource spawning, regeneration cycles, capacity limits, and consumption arbitration |
| `ReproductionInheritanceTest` | Energy thresholds, cooldowns, asexual trait mutation, and boundary clamping |
| `EvolutionDynamicsTest` | Trait metrics, diversity calculations, lineage tracking, and lifespan statistics |
| `NeuralNetworkTest` | Feedforward inference, `tanh` activation, zero-allocation buffers, and weight mutation |
| `EcosystemMultiSpeciesTest` | Predator hunting, Lotka–Volterra dynamics, and seasonality oscillation |
| `ScientificObservationTest` | Event logging, ring-buffer bounding, CSV export formatting, and trajectory comparison |
| `PerformanceScalabilityTest` | Uniform spatial grid partitioning and $O(1)$ query correctness |
| `Phase10LabIntegrationTest` | Save/load serialization round-trips, experiment presets, and inspector integrity |

---

## Contributing

Contributions, bug reports, and research extensions are welcome! Please review our [Contributing Guide](CONTRIBUTING.md) and [Code of Conduct](CODE_OF_CONDUCT.md) before opening a pull request.

---

## License

This project is licensed under the [MIT License](LICENSE).

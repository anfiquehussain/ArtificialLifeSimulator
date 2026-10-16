# Project Specification: Artificial Life & Evolution Simulator

## 1. Project vision

Build a sophisticated artificial-life simulation in C++ where virtual organisms inhabit an environment, consume resources, reproduce, mutate, compete, and evolve over generations.

The long-term goal is an interactive experimental laboratory for exploring emergence, natural selection, evolutionary dynamics, and complex behaviour.

This is not a conventional game. The primary focus is computational experimentation and observing behaviours that emerge from defined rules rather than scripting every outcome.

## 2. Technology stack

- **Language:** C++20
- **Build system:** CMake
- **Graphics:** SFML 3
- **Testing:** Unit tests, with Catch2 optional
- **Data export:** CSV
- **Version control:** Git
- **Target platform:** Windows 11 initially

Use additional libraries only when they provide a clear benefit. Avoid unnecessary dependencies.

## 3. Mandatory development rules

1. Implement the project in the exact phase order specified below.
2. Implement only one phase at a time.
3. Never implement future-phase features early.
4. Before modifying files, inspect the existing project and preserve working code.
5. If starting from an empty directory, propose the file structure and then create it.
6. Write actual, functional C++ code. Do not substitute pseudocode, mock functionality, or empty placeholders.
7. Keep simulation logic independent from rendering and UI.
8. Use clear interfaces, sensible class responsibilities, and maintainable code.
9. Use deterministic random seeds when repeatable experiments are needed.
10. Add tests for important simulation rules and edge cases.
11. Build the project and run available tests after every phase. If execution is unavailable, state that explicitly and provide verification commands.
12. Fix errors and regressions before considering a phase complete.
13. Provide exact Windows build and run commands.
14. Update the README and project roadmap when appropriate.
15. Never claim that a feature works unless it has been verified or its verification limitation is stated.
16. Do not rewrite working code unnecessarily.
17. Do not proceed to the next phase until I explicitly approve it.
18. If a requirement is ambiguous, ask a focused question rather than making a major architectural assumption.

## 4. Phase 1 — Simulation foundation

### Objective
Create the basic application and rendering environment.

### Tasks
- Set up C++20, CMake, and SFML 3.
- Create a functioning application window.
- Implement the main loop and controlled simulation timing.
- Render a simple 2D world with an optional grid.
- Add pause, resume, and reset controls.
- Handle keyboard input and window closing correctly.
- Establish basic project configuration and logging where useful.

### Acceptance criteria
- The project builds successfully.
- The window opens and renders the world.
- Pause, resume, and reset work correctly.
- The application closes cleanly.
- Build instructions are documented.

Do not implement organisms yet.

## 5. Phase 2 — Organisms and survival

### Objective
Introduce the first living entities.

### Tasks
- Create an organism model with a unique ID, position, energy, and basic traits.
- Spawn an initial population.
- Implement movement and energy consumption.
- Remove organisms whose energy reaches zero.
- Display population statistics.
- Separate organism state from drawing code.

### Acceptance criteria
- Organisms move around the world.
- Energy changes according to explicit rules.
- Organisms die when their energy is depleted.
- Population counts remain correct.
- Simulation rules can be tested independently of rendering.

## 6. Phase 3 — Food and environment

### Objective
Create a resource-based ecosystem.

### Tasks
- Add food entities and food spawning.
- Implement food detection and consumption.
- Restore organism energy when food is consumed.
- Add food regeneration and world boundaries.
- Define resource availability and consumption rules.
- Avoid allowing multiple organisms to consume the same food item incorrectly.

### Acceptance criteria
- Organisms can find and consume food.
- Food and energy are updated consistently.
- Resource scarcity can cause starvation.
- The ecosystem remains stable under reasonable configurations.

## 7. Phase 4 — Reproduction and inheritance

### Objective
Allow populations to reproduce and transmit traits.

### Tasks
- Define reproduction eligibility conditions.
- Create offspring with inherited traits.
- Introduce configurable mutation rates.
- Track parent IDs and generation numbers.
- Prevent uncontrolled population growth from overwhelming the simulation.
- Define how energy and resources are allocated during reproduction.

### Acceptance criteria
- Eligible organisms reproduce.
- Offspring inherit parental traits.
- Mutations can produce trait variation.
- Parent and generation relationships are recorded correctly.
- Population limits and resource costs work as intended.

## 8. Phase 5 — Natural selection

### Objective
Create conditions in which inherited differences affect evolutionary success.

### Tasks
- Introduce variation in speed, perception radius, metabolism, and other relevant traits.
- Make environmental conditions influence survival and reproduction.
- Measure survival rates and reproductive success.
- Track trait distributions across generations.
- Add configurable selection pressures.
- Compare different parameter settings.

### Acceptance criteria
- Heritable traits influence organism outcomes.
- Population statistics reveal measurable changes across generations.
- Experiments can be repeated using the same random seed.
- Do not hard-code a predetermined evolutionary outcome.

## 9. Phase 6 — Neural networks and behaviour

### Objective
Give organisms more flexible decision-making capabilities.

### Tasks
- Introduce sensors for food, obstacles, energy, and nearby organisms.
- Implement a small neural network or another explicit decision controller.
- Use controller outputs to determine movement and actions.
- Allow controller parameters to be inherited and mutated.
- Keep network evaluation separate from the simulation engine.
- Add visual debugging tools for observing individual organisms.

### Acceptance criteria
- Organisms respond to environmental inputs through their controllers.
- Different controller parameters can produce different behaviours.
- Inheritance and mutation affect controller behaviour.
- Controller evaluation can be tested independently.
- Avoid claiming intelligence or learning unless demonstrated experimentally.

## 10. Phase 7 — Complex ecosystems

### Objective
Introduce interactions between different populations.

### Tasks
- Add configurable species or ecological roles.
- Introduce predator-prey relationships or competition for resources.
- Add environmental changes and resource scarcity.
- Track population cycles, coexistence, and extinction.
- Allow experiments with different ecosystem configurations.
- Ensure interactions are governed by explicit rules.

### Acceptance criteria
- Multiple populations can interact.
- Predation or competition affects survival and reproduction.
- Population changes can be observed and measured.
- The simulation handles extinction and near-extinction cases correctly.

## 11. Phase 8 — Scientific observation and experiments

### Objective
Turn the simulator into a repeatable experimental environment.

### Tasks
- Track population size, births, deaths, and extinction events.
- Record inherited traits and genetic diversity.
- Track lineage and generation history.
- Export measurements to CSV.
- Save and restore simulation states.
- Add configurable random seeds.
- Create charts for important measurements.
- Support repeatable experiments with different parameter settings.

### Acceptance criteria
- Measurements are accurate and consistent.
- CSV exports can be opened by spreadsheet tools.
- Save/load restores the simulation correctly.
- Identical initial states and random seeds reproduce the same simulation results, subject to deterministic implementation.
- Experiments can be compared using recorded data.

## 12. Phase 9 — Performance and scalability

### Objective
Support larger populations without sacrificing simulation correctness.

### Tasks
- Profile performance before optimizing.
- Improve neighbour searches using an appropriate spatial data structure, such as a spatial grid or quadtree.
- Separate simulation updates from rendering.
- Reduce unnecessary allocations and repeated calculations.
- Add performance measurements for population size, simulation speed, and frame rate.
- Consider multithreading only where justified.
- Preserve deterministic behaviour where practical.

### Acceptance criteria
- Performance improvements are measured rather than assumed.
- Large populations can be tested using documented configurations.
- Optimizations do not change the intended simulation rules.
- No significant regressions are introduced.

## 13. Phase 10 — Experimental laboratory

### Objective
Deliver a complete, configurable artificial-life research playground.

### Tasks
- Add controls for world parameters and evolutionary rules.
- Add pause, resume, reset, simulation-speed controls, and experiment presets.
- Allow users to select or inspect individual organisms.
- Display organism traits, lineage, and controller information.
- Add population and trait charts.
- Support save/load and CSV export.
- Add experiment comparison tools.
- Provide clear documentation for creating and running experiments.
- Test the complete application and fix integration problems.

### Acceptance criteria
- All previous phases work together.
- Users can configure and run repeatable experiments.
- Organisms can be inspected and population statistics visualized.
- Save/load and export work correctly.
- The application can be built from the documented instructions.

## 14. Suggested project structure

Use this as a starting point and adjust only when justified by the implementation.

```text
artificial-life/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── docs/
│   ├── ROADMAP.md
│   ├── ARCHITECTURE.md
│   └── EXPERIMENTS.md
├── assets/
├── config/
├── include/
│   ├── core/
│   ├── entities/
│   ├── simulation/
│   ├── ai/
│   ├── rendering/
│   ├── ui/
│   └── data/
├── src/
│   ├── main.cpp
│   ├── core/
│   ├── entities/
│   ├── simulation/
│   ├── ai/
│   ├── rendering/
│   ├── ui/
│   └── data/
└── tests/
```

Do not create empty files or unnecessary directories merely to match this structure. Introduce components as the relevant phases require them.

## 15. Architecture principles

- Keep simulation state independent of SFML rendering.
- Use a controlled simulation timestep.
- Separate world updates, organism behaviour, and visual drawing.
- Centralize configuration and random-number generation.
- Use stable entity identifiers.
- Make important simulation rules independently testable.
- Avoid tightly coupling UI controls to biological rules.
- Keep data export and persistence outside the core update loop.
- Document major design decisions and non-obvious mathematical assumptions.

## 16. Development workflow for every phase

For each phase, follow this sequence:

1. Inspect the current project.
2. State the scope and acceptance criteria.
3. Identify the files that need to be created or modified.
4. Implement only the current phase.
5. Build the project.
6. Run relevant automated tests.
7. Fix compilation errors, runtime problems, and regressions.
8. Summarize the implementation and verification results.
9. Update documentation and the roadmap.
10. List any remaining limitations.
11. Stop and wait for my approval before starting the next phase.

## 17. Final instruction

Begin with **Phase 1 only**.

First inspect the project directory and report the proposed implementation structure. Then implement the simulation foundation, verify it as far as the available tools permit, and provide exact Windows build and run instructions.

Do not implement Phase 2 or any later phase until I explicitly approve proceeding.

The entire specification above defines the long-term direction. It does not authorize implementing all phases at once.
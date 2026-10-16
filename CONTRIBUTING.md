# Contributing to Artificial Life & Evolution Simulator

Thank you for your interest in contributing to the Artificial Life & Evolution Simulator! We welcome contributions ranging from bug fixes and documentation improvements to new evolutionary models and scientific analysis modules.

---

## Core Development Guidelines

To preserve code quality, simulation performance, and scientific validity, please adhere to these core principles:

1. **Decoupled Simulation Engine:** Never introduce rendering (`SFML`) or presentation logic into `include/alife/simulation/`, `include/alife/biology/`, `include/alife/ai/`, or `include/alife/core/`. The simulation core must remain strictly headless and runnable without graphics.
2. **Fixed-Timestep Physics:** All physical, metabolic, and ecological processes must operate via the fixed simulation timestep $\Delta t = 1/60\text{s}$. Never rely on wall-clock time in simulation mechanics.
3. **Deterministic Execution:** Simulation outcomes given an identical seed must be 100% reproducible across runs and test executions. Use `alife::core::Rng` rather than non-deterministic random calls (`rand()`, `std::random_device`).
4. **Performance & Zero-Allocation Inner Loops:** Hot simulation loops (spatial querying, neural evaluation, collision/foraging arbitration) must not allocate heap memory (`new`, `malloc`, dynamically resizing containers inside loops). Use pre-allocated buffers or static layouts.
5. **C++20 Modern Idioms:** Use modern C++20 features: `std::span`, concepts, `<numbers>`, `<ranges>`, `std::erase_if`, and standard library algorithms.

---

## Getting Started

1. Fork the repository and clone your fork:
   ```bash
   git clone https://github.com/<your-username>/ArtificialLifeSimulator.git
   cd ArtificialLifeSimulator
   ```
2. Create a topic branch:
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. Configure and build:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
4. Run all existing tests to establish a baseline:
   ```bash
   ctest --test-dir build -C Release --output-on-failure
   ```

---

## Adding Tests

Every new feature or bug fix must include corresponding automated unit tests under `tests/unit/`.
- Ensure all tests pass 100% before submitting.
- Verify determinism across multiple runs with explicit seed checks.

---

## Pull Request Process

1. Ensure code compiles without compiler warnings on `/W4` (MSVC) or `-Wall -Wextra` (GCC/Clang).
2. Ensure all CTest suites pass.
3. Open a Pull Request against the `main` branch with a clear title and description of your changes, referencing any relevant issues.

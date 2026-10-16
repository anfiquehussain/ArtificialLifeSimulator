# Experiment Methodology & Presets

## Overview
This document records simulation configurations, deterministic random seeds, and repeatable experiment definitions.

## Determinism Guidelines
- Every repeatable experiment must specify:
  1. The 64-bit integer PRNG seed (`seed`).
  2. The fixed simulation timestep $\Delta t$.
  3. The world dimensions and initial configuration parameters.
- Experiments running with identical parameters and seeds must yield bit-identical simulation states across platforms.

## Presets Catalog
*(Presets will be cataloged here as simulation features are introduced in subsequent phases.)*

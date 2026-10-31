
# NOMINAL — AI Development Instructions

## Project
NOMINAL is an Unreal Engine 5 C++ game for a NASA space mission design challenge.

## Objective
Players design, manage and simulate space missions while balancing:
- Mission objectives
- Spacecraft and launch vehicle design
- Payload and scientific instruments
- Budget, mass and power
- Communications
- Orbital constraints
- Mission risks and outcomes

## Architecture
- C++ for engineering calculations and core logic.
- Unreal Engine for gameplay and visualization.
- UMG/Blueprints for user interfaces.
- SI units for physical calculations.
- Document assumptions and simplified physics.

## AI Responsibilities
Astra: core architecture and integration.
Opus: simulation, engineering calculations and tests.
Gemini: interface design, research and QA.

## Git Rules
- Work in task-specific branches.
- Never push directly to main.
- Never merge pull requests without human approval.
- Do not modify unrelated modules.
- Do not modify .uasset or .umap files without permission.
- Never commit generated build files or credentials.
- Every PR must describe changes and tests performed.
- Do not claim a test passed unless it was executed.

## Quality
- Prefer modular, testable code.
- Validate numerical inputs.
- Avoid unnecessary dependencies.
- Preserve existing project functionality.
- Ask before changing engine versions or project configuration.

# CodeSamplesBohemia

The code examples in this repository are extracted from my previous projects and are intended to showcase my technical skills and system design capabilities. If you have any questions regarding these examples, their implementation, or usage, please feel free to contact me directly.

## Event System

This module demonstrates a template-based event system developed for a custom engine. The system is designed to:

- Work with both member functions (class methods) and non-member functions (free functions).
- Provide a generic, extensible structure thanks to its template-based design.
- Enable flexible event definition and dispatching to multiple listeners, following an observer-like approach.

This code sample highlights my ability to design flexible architectures and apply modern C++ features such as templates and function binding.

## Material Effect Controller

This component is responsible for managing dynamic material effects on actors that contain mesh renderer components. It provides:

- A centralized way to apply and control visual effects directly on mesh renderers.
- Support for real-time updates, enabling effects such as color transitions, dissolves, or highlight animations.
- A modular design that can be attached to any actor requiring visual effect control.

This code sample demonstrates my experience in component-based design and handling runtime material manipulation within a rendering pipeline.

## Post Process Controller

This component provides centralized management of all post-process settings offered by the engine, similar in concept to the Material Effect Controller. Its main features include:

- Unified control over various post-processing parameters (e.g., bloom, exposure, color grading).
- A flexible structure that allows easy runtime adjustments for visual effects.
- Extensive use of macros to minimize code duplication and simplify configuration.

This code sample highlights my ability to design reusable systems, reduce boilerplate through macro-based abstractions, and implement efficient ways of handling complex rendering settings.

## Perk Management System

This component is designed to handle perk management in full compatibility with the Gameplay Ability System (GAS). It provides a comprehensive solution for applying and managing gameplay abilities and effects on actors. Key features include:

- Full integration with GAS for both Gameplay Abilities and Gameplay Effects.
- Support for all potential methods of applying, stacking, and removing perks.
- A modular and extensible design, allowing flexible integration into different gameplay systems.

This code sample demonstrates my ability to work with complex gameplay frameworks (such as GAS) and build robust systems for managing character progression and abilities.

## Perk Tree System

This system implements a perk tree structure that drives player progression within the game. It also includes the corresponding UI implementation, allowing players to interact with and unlock perks visually. Key features:

- A structured, tree-based approach for managing progression paths.
- Integration with the gameplay layer to unlock and apply new perks.
- A fully functional UI for visualization and interaction, enhancing player experience.

This code sample showcases my ability to design progression systems and implement user interfaces that seamlessly integrate with gameplay mechanics.

## Base Cel Material

An example from my material/shader work, used as the base material for all actors in the game.

- Texture blending for layered detail.
- Glint and Fresnel effects for stylized visuals.
- Custom FOV calculation for unique perspective effects.
- Weak point masking to highlight vulnerable areas.

👉 A copy of the related Blueprint can be found here: https://blueprintue.com/blueprint/ikjn4bd4/

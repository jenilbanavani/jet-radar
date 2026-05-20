# 2D Radar Simulation

A modular C++17/SFML radar simulation with:

- Black background
- Green circular radar display
- Rotating scan line
- Player fixed in the center
- Multiple moving enemy blips
- Enemy dots visible only inside radar range
- Smooth 60 FPS rendering

## Build

Install SFML 2.5 or newer, then build with CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run:

```powershell
.\build\Release\radar_sim.exe
```

On single-config generators, the executable may be:

```powershell
.\build\radar_sim.exe
```

## Architecture

- `Simulation` owns update logic and world state.
- `Player` is fixed at the origin, matching the radar center.
- `Enemy` implements reusable moving entity behavior.
- `RadarRenderer` converts world positions into radar-space visuals.

The simulation/rendering split is intentional so future missile systems,
network replication, and multiplayer authority can be added without coupling
game-state logic to SFML drawing code.

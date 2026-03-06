# CLAUDE.md — SlimePlugin

This file provides guidance for AI assistants working on the SlimePlugin codebase.

---

## Project Overview

**SlimePlugin** is an Unreal Engine 5 plugin that implements real-time procedural metaball geometry generation using the Marching Cubes algorithm. It exposes a fully Blueprint-accessible `AMetaballs` actor that animates and renders implicit surfaces (metaballs/blobs) as a `ProceduralMeshComponent`.

- **Author**: Adam Komarowski
- **Engine Version**: UE 5.7.2
- **Plugin Version**: 1.0
- **Module Type**: Runtime

---

## Repository Structure

```
SlimePlugin/
├── CLAUDE.md                          # This file
├── SlimePlugin.uplugin                # Plugin manifest (metadata, engine version, module list)
└── Source/
    └── SlimePlugin/
        ├── SlimePlugin.Build.cs       # Unreal Build Tool module rules
        ├── Public/                    # Header files (API surface)
        │   ├── SlimePlugin.h          # Module interface declaration
        │   ├── SlimePluginPrivatePCH.h # Precompiled header includes
        │   ├── Metaballs.h            # AMetaballs actor declaration
        │   └── CMarchingCubes.h      # Marching Cubes utility class
        └── Private/                  # Implementation files
            ├── SlimePlugin.cpp        # Module startup/shutdown (stub)
            ├── Metaballs.cpp          # AMetaballs full implementation
            └── CMarchingCubes.cpp    # Marching Cubes lookup tables
```

---

## Architecture

### Class Hierarchy

```
IModuleInterface
  └── SlimePluginImpl        (SlimePlugin.h/cpp)   — Plugin lifecycle, currently stub

AActor
  └── AMetaballs             (Metaballs.h/cpp)     — Main actor: metaball simulation + mesh generation

struct SMetaBall                                    — Per-ball data (position, velocity, acceleration, mass)

class CMarchingCubes         (CMarchingCubes.h/cpp) — Static lookup tables for the MC algorithm
```

### Data Flow (per frame)

```
Tick(dt)
  └── Update(dt)             — Integrate ball positions via Euler: v += a*dt, p += v*dt
  └── Render()
        └── ComputeGridPointEnergy()  — Cache energy values at grid vertices
        └── ComputeGridVoxel()        — Marching cubes per voxel (flood-fill from seed voxels)
              └── ComputeEnergy(x,y,z) — Σ(mass_i / dist_i²) for all balls
              └── AddNeighborsToList() — Queue adjacent voxels via MC neighbor table
        └── ProceduralMeshComponent::CreateMeshSection_Linear()
```

### Key Components (on AMetaballs)

| Component | Type | Purpose |
|-----------|------|---------|
| `ProcMesh` | `UProceduralMeshComponent` | Renders the generated mesh |
| `BoxComp` | `UBoxComponent` | Defines the bounding volume for the grid |
| `SpriteComponent` | `UBillboardComponent` | Editor-only billboard for actor selection |

---

## Key Classes

### `AMetaballs` (`Public/Metaballs.h`, `Private/Metaballs.cpp`)

The primary actor. Instantiate it in a level to see metaballs.

**Configuration properties** (all `EditAnywhere, BlueprintReadWrite`, Category `"Slime"`):

| Property | Type | Range | Description |
|----------|------|-------|-------------|
| `m_NumBalls` | int32 | 0–32 | Number of active metaballs |
| `m_Scale` | float | — | Spatial scale of the metaball field |
| `m_GridStep` | int32 | 16–128 | Grid resolution (higher = more detail, more expensive) |
| `m_randomseed` | int32 | — | Seed for random initial positions |
| `m_automode` | bool | — | Enable automatic ball animation |
| `m_AutoLimitX/Y/Z` | float | — | Boundary box half-extents for auto-mode |
| `m_Material` | `UMaterialInterface*` | — | Material applied to the generated mesh |

**Blueprint-callable setters** (Category `"Slime"`):

- `SetBallTransform(int index, FVector pos)` — Manually position a specific ball
- `SetNumBalls(int n)` / `SetScale(float s)` / `SetGridSteps(int g)` / `SetRandomSeed(int s)`
- `SetAutoMode(bool b)` / `SetAutoLimitX/Y/Z(float v)` / `CheckLimit()`

**Grid limits** (compile-time constants in `MinMax` enum):

| Constant | Value |
|----------|-------|
| `MAX_METABALLS` | 32 |
| `MIN_GRID_STEPS` | 16 |
| `MAX_GRID_STEPS` | 128 |
| `MAX_OPEN_VOXELS` | 32 (initial queue capacity, auto-grows) |

### `CMarchingCubes` (`Public/CMarchingCubes.h`, `Private/CMarchingCubes.cpp`)

Static utility class. Does not need instantiation beyond calling `BuildTables()` once during `PostInitializeComponents()`.

**Static tables**:

| Table | Size | Description |
|-------|------|-------------|
| `m_CubeVertices[8][3]` | 8×3 | Unit cube corner offsets |
| `m_CubeEdges[12][2]` | 12×2 | Edge definitions (vertex index pairs) |
| `m_CubeTriangles[256][16]` | 256×16 | Triangle configurations per cube case |
| `m_CubeNeighbors[256]` | 256 | Neighbor-face bit flags built by `BuildTables()` |

---

## Build System

### Module Rules (`SlimePlugin.Build.cs`)

Public module dependencies:
- `Engine`
- `Core`
- `CoreUObject`
- `InputCore`
- `ProceduralMeshComponent`

The precompiled header (`PrivatePCHHeaderFile`) is `SlimePluginPrivatePCH.h`.

### Plugin Manifest (`SlimePlugin.uplugin`)

Key fields to keep in sync when updating:
- `"EngineVersion"` — must match the target UE version
- `"VersionName"` — human-readable plugin version
- `"Modules[].Name"` — must match the module name in `Build.cs` and source folder

---

## Coding Conventions

### Naming

| Pattern | Meaning | Example |
|---------|---------|---------|
| `m_` prefix | Member variable | `m_NumBalls` |
| `m_pf` prefix | Member float pointer/array | `m_pfGridEnergy` |
| `m_pn` prefix | Member int pointer/array | `m_pnGridPointStatus` |
| `m_p` prefix | Generic member pointer | `m_pOpenVoxels` |
| `A` prefix | Unreal Actor subclass | `AMetaballs` |
| `U` prefix | Unreal Object/Component | `UProceduralMeshComponent` |
| `F` prefix | Unreal struct/value type | `FVector` |
| `S` prefix | Plain C++ struct | `SMetaBall` |
| `C` prefix | Plain C++ class | `CMarchingCubes` |

### UPROPERTY / UFUNCTION Macros

- All designer-facing properties use `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slime")`.
- All runtime setters exposed to Blueprint use `UFUNCTION(BlueprintCallable, Category = "Slime")`.
- Editor-only code is wrapped in `#if WITH_EDITOR ... #endif`.

### Memory Management

- Grid arrays (`m_pfGridEnergy`, `m_pnGridPointStatus`, `m_pnGridVoxelStatus`) are allocated with `new` in `SetGridSize()` and must be freed (currently freed on re-allocation and should be freed in destructor / `EndPlay`).
- The open-voxel queue (`m_pOpenVoxels`) is allocated with `new SMetaBall[size]` and resized by doubling when full.
- **Prefer Unreal `TArray` for new dynamic collections** to benefit from automatic memory management.

### Unreal Patterns to Follow

- Use `CreateDefaultSubobject<T>()` inside the constructor for components.
- Use `FObjectInitializer` parameter in constructors that create subobjects.
- Implement `PostInitializeComponents()` (not the constructor) for one-time setup that needs engine systems.
- Implement `PostEditChangeProperty()` for editor-time validation; always call `Super::PostEditChangeProperty(e)`.
- Use `UE_LOG(LogTemp, Warning, TEXT(...))` for debug logging.

---

## Development Workflow

### Adding a New Metaball Property

1. Declare in `Metaballs.h` with the appropriate `UPROPERTY` macro.
2. Add a `UFUNCTION(BlueprintCallable)` setter in `Metaballs.h`.
3. Implement the setter in `Metaballs.cpp`; clamp to valid range as needed.
4. If the property affects grid size or ball count, call `SetGridSize()` or `InitBalls()` inside the setter.
5. Add editor validation in `PostEditChangeProperty()`.

### Changing Grid Resolution Logic

- Grid dimension is `m_GridStep + 1` in each axis.
- Total grid points: `(m_GridStep+1)³`.
- Total voxels: `m_GridStep³`.
- All three status arrays are sized to `(m_GridStep+1)³`; resizing is done in `SetGridSize()`.

### Adding a New Rendering Feature (normals, UVs, colors)

The render arrays (`m_vertices`, `m_Triangles`, `m_normals`, `m_UV0`, `m_tangents`, `m_vertexColors`) are passed directly to `ProceduralMeshComponent::CreateMeshSection_Linear()`. Populate them in `ComputeGridVoxel()` alongside vertex generation.

### Testing Changes

There is no automated test suite. Verify changes by:
1. Compiling the plugin inside a UE5 project (`Build.cs` references must resolve).
2. Placing an `AMetaballs` actor in a test level.
3. Adjusting properties in the Details panel and verifying the mesh updates correctly in real time.
4. Testing Blueprint setters from a Blueprint actor or level Blueprint.

---

## Algorithm Reference

### Energy Field

Each point **p** in world space has energy:

```
E(p) = Σ_i  (m_i / |p - p_i|²)
```

The isosurface is the level set `E(p) = 1.0` (threshold).

### Marching Cubes Case Index

For a voxel with 8 corners, the case index is an 8-bit integer where bit `i` is 1 if corner `i` has energy ≥ 1.0:

```
case = Σ_i  (E(corner_i) >= 1.0) << i
```

`CMarchingCubes::m_CubeTriangles[case]` gives the edge list (terminated by -1) from which triangles are built.

### Flood-Fill Surface Extraction

Instead of evaluating every voxel, the implementation seeds from voxels containing metaball centers and expands outward using a queue (`m_pOpenVoxels`). The `m_CubeNeighbors[case]` bitmask encodes which of the 6 face-adjacent neighbors should be enqueued.

---

## Common Pitfalls

- **Grid index out of bounds**: Always validate grid coordinates against `[0, m_GridStep]` before accessing grid arrays. `ConvertWorldCoordinateToGridPoint()` can return values at the boundary.
- **Energy at zero distance**: `ComputeEnergy()` divides by distance squared; ensure no metaball exactly coincides with a grid point to avoid division by zero.
- **Open voxel queue overflow**: The queue doubles when full, but always check that resizing logic correctly copies existing entries before freeing the old buffer.
- **Editor vs. runtime init**: `PostInitializeComponents()` is called both in-editor (for CDO validation) and at runtime; guard expensive work with `HasBegunPlay()` or `GetWorld()->IsGameWorld()` if needed.
- **Material not set**: The mesh is created even without a material; assign `m_Material` in the actor's defaults or Blueprint to avoid an invisible mesh.

# Claude Code Skill for Unreal Engine

A comprehensive Claude skill for Unreal Engine C++ development with deep project structure understanding, GAS integration, plugin development guidance, and data-driven design patterns.

Originated from https://github.com/DSTN2000/claude-unreal-engine-skill

## Features

- **Zero Assumptions Philosophy**: Always discovers your project structure before suggesting code
- **Enhanced Input System**: Complete support for UE5's Enhanced Input with automatic asset discovery
- **Gameplay Ability System (GAS)**: Full GAS integration including custom AbilityTask creation, advanced GA lifecycle management, and network replication patterns
- **Plugin Development**: Runtime/Editor module split, Build.cs configuration, `WITH_EDITOR` macro guidance
- **Data-Driven Design**: DataAsset, PrimaryDataAsset, DataTable patterns for gameplay data architecture
- **C++ Best Practices**: GC safety, reflection system rules, forward declaration policy, early return patterns, soft reference handling
- **Version-Aware**: Automatically detects engine version and provides appropriate code
- **Project-Specific**: Discovers your naming conventions and existing patterns

## Installation

### For Claude Code

```bash
# Personal skills (available in all projects)
cp -r unreal-engine ~/.claude/skills/

# OR Project skills (team-shared via git)
cp -r unreal-engine .claude/skills/
```

The skill will automatically activate when working on Unreal Engine projects, or invoke directly with `/unreal-engine`.

### For Claude.ai

1. Zip the skill directory or use the `.skill` file
2. Upload via Settings > Customize > Skills
3. The skill activates automatically for Unreal Engine related tasks

## Skill Structure

```
unreal-engine/
├── SKILL.md                                    # Core instructions & routing
├── README.md
└── references/
    ├── unreal_cpp_best_practice.md             # GC, reflection, forward declarations, early return, soft references
    ├── enhanced_input.md                       # Enhanced Input binding, Input Actions, Mapping Contexts
    ├── gameplay_ability_system.md              # GAS setup, custom AbilityTask, GA lifecycle, network patterns
    ├── plugin_development.md                   # Plugin creation, module split, Build.cs, WITH_EDITOR
    ├── plugin_guidance.md                      # External plugin usage & stability checks
    ├── data_driven_design.md                   # DataAsset, PrimaryDataAsset, DataTable, AssetManager
    ├── debugging.md                            # UE_LOG, Visual Logger, crash/assert patterns
    └── common_pitfalls.md                      # Troubleshooting build errors, GC bugs, performance issues
```

## Reference Details

| File | Covers |
|------|--------|
| `unreal_cpp_best_practice.md` | GC safety, reflection system (`UFUNCTION`/`TObjectPtr` restrictions), header include minimization & forward declarations, `TSoftObjectPtr` function parameter conversion, early return for Cast chains, UHT delegate macros |
| `enhanced_input.md` | Input Action/Mapping Context binding, `UEnhancedInputComponent` casting, runtime IMC add/remove |
| `gameplay_ability_system.md` | ASC placement, AttributeSet, GA creation/granting/activation, GE application, GameplayTags, **custom AbilityTask creation**, **GA End vs Cancel semantics & dynamic grant/revoke**, **GE Stacking**, **NetExecutionPolicy & server-authoritative patterns** |
| `plugin_development.md` | Runtime/Editor module split, `.uplugin` configuration, Build.cs Public vs Private dependencies, `WITH_EDITOR` vs `WITH_EDITORONLY_DATA`, `IModuleInterface`, external project integration |
| `plugin_guidance.md` | External plugin enable checks, experimental plugin documentation lookup, stability tiers |
| `data_driven_design.md` | `UDataAsset` vs `UPrimaryDataAsset` comparison, nested DataAssets, AssetManager runtime loading, **PrimaryDataAsset requires `TSoftObjectPtr` references**, DataTable + CSV/JSON import, selection criteria guide |
| `debugging.md` | `UE_LOG` custom categories, `AddOnScreenDebugMessage`, Visual Logger, `check`/`ensure` |
| `common_pitfalls.md` | Enhanced Input not working, GAS ability not activating, linker errors, GC collection, performance issues |

## How It Works

1. **Pre-Flight Discovery**: Locates `.uproject`, maps `Source/` and `Content/`, identifies plugins and engine version
2. **Routing**: Loads the appropriate reference file based on question type (GAS → `gameplay_ability_system.md`, plugin development → `plugin_development.md`, etc.)
3. **Context-Aware Code**: Suggests code that follows your project's actual asset names, naming conventions, and patterns

## Usage Examples

```
"Add jump functionality to my character"
"Set up Enhanced Input for movement"
"Create a custom AbilityTask for spawning projectiles"
"Help me set up a DataAsset-based weapon system"
"Create a plugin with Runtime/Editor module split"
"How should I handle GA lifecycle on weapon swap?"
"Set up server-authoritative ability activation"
```

## Dependencies

This skill is designed for projects using:

- **Unreal Engine 5.x**
- **GameplayAbilities** plugin (for GAS-related guidance)
- **EnhancedInput** plugin (for input system guidance)

## Contributing

Contributions are welcome! If you have improvements or find issues:

1. Fork this repository
2. Make your changes
3. Submit a pull request

## License

This skill is provided as-is for use with Claude Code and Claude.ai.

## Author

Created for the Unreal Engine development community using Claude Code.
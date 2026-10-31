# Agent connection test

This documentation-only change verifies the agent workflow for creating a branch,
committing, pushing, and opening a pull request for NOMINAL.

## Repository structure

- `NOMINAL.uproject` is the Unreal Engine project descriptor. It declares the
  `NOMINAL` runtime module and enables Modeling Tools Editor Mode for the editor.
- `Source/NOMINAL/` contains the C++ module entry point, header, and
  `NOMINAL.Build.cs` module rules. Dependencies include Core, CoreUObject, Engine,
  InputCore, and EnhancedInput.
- `Source/NOMINAL.Target.cs` and `Source/NOMINALEditor.Target.cs` define the game
  and editor build targets, using Unreal 5.8 include-order settings.
- `Config/` contains default editor, engine, game, and input configuration.
- `README.md` identifies the project; `Docs/` holds repository documentation.
- `.gitignore` excludes generated build, editor, and IDE files. `.gitattributes`
  configures Git LFS for Unreal assets, maps, and selected 3D source formats.

The inspected checkout has no `Content/` or `Plugins/` directory. This change
does not modify Unreal assets, C++ code, build rules, or project configuration.

# Claude Code Configuration — EWE

> 🌐 [한국어 버전](README_KR.md)

This folder is tracked in git so the whole team shares the same Claude Code
setup. Read the setup note below before your first session.

---

## ⚠️ Machine-specific paths — you must edit these

Some permission rules contain **absolute paths from the original author's
machine**. Claude Code matches `Bash(...)` and `PowerShell(...)` rules as plain
**command-string globs** — it does not resolve or normalize paths — so a rule
only takes effect when the path matches your install exactly.

If your Unreal Engine lives somewhere else, edit these entries in
`.claude/settings.local.json` before starting work:

| Rule | Change to |
| :--- | :--- |
| `Bash("C:\Program Files\Epic Games\UE_5.5\...\Build.bat" EWEEditor *)` | your engine install path |
| `Bash("C:\Program Files\Epic Games\UE_5.5\...\Build.bat" EWE *)` | your engine install path |
| `PowerShell(& "C:\Program Files\Epic Games\UE_5.5\...\Build.bat" *)` | your engine install path |
| `Read(//c/Program Files/Epic Games/UE_5.5/**)` | your engine install path, POSIX-normalized |

Also update the build command in `CLAUDE.md` (section "빌드 검증") to match.

Nothing breaks if you skip this — Claude Code simply falls back to asking for
approval on every build command instead of running it unattended.

### Path syntax cheat sheet

`Read` / `Edit` rules use gitignore pattern syntax with four anchors:

| Pattern | Anchored at |
| :--- | :--- |
| `//path` | filesystem root (a real absolute path) |
| `~/path` | your home directory |
| `/path` | the settings source — for `settings.local.json` this is the directory you launched Claude Code from |
| `path`, `./path` | current directory |

Two gotchas:

- A single leading slash is **not** an absolute path. `/Source/**` means
  `<launch dir>/Source/**`. Use `//` for filesystem-absolute paths.
- On Windows, paths are normalized to POSIX form before matching:
  `C:\Program Files` becomes `/c/Program Files`.

Because `/path` anchors to the launch directory, **always start Claude Code
from the repository root** or the `Read(/Source/**)` style rules will point at
the wrong place.

---

## Files in this folder

| File | Tracked | Purpose |
| :--- | :---: | :--- |
| `settings.json` | yes | Shared project settings. Currently holds the `PostToolUse` hook. |
| `settings.local.json` | yes | Permission rules. Claude Code appends to this file whenever you choose "Yes, don't ask again". |
| `hooks/normalize-eol.ps1` | yes | Rewrites files to CRLF after every `Write` / `Edit`. |
| `skills/` | yes | Project-scoped skills. |

> **Note:** JSON does not support comments and Claude Code does not accept
> JSONC, so the settings files must stay strict JSON. All commentary lives in
> this README instead.

---

## Permission rules — section by section

Rules are evaluated **deny → ask → allow**. The first match wins, and rule
specificity does not change that order: a matching `deny` always beats a more
specific `allow`.

### allow — Web access

```
WebSearch
WebFetch(domain:dev.epicgames.com)
WebFetch(domain:forums.unrealengine.com)
WebFetch(domain:code.claude.com)
```

Unreal Engine's API surface is under-documented, so Claude frequently needs the
official docs and community forums. Domain-scoped rather than a blanket
`WebFetch` allow.

### allow — IDE integration

```
mcp__ide__getDiagnostics
```

Lets Claude pull compiler diagnostics from the connected IDE instead of running
a full rebuild to find an error.

### allow — Reads

```
Read(//c/Program Files/Epic Games/UE_5.5/**)   Engine source and headers (machine-specific)
Read(/Source/**)                                Game module
Read(/Plugins/**)                               EasyWeaponryEnchanter plugin
Read(/Config/**)                                DefaultEngine.ini, DefaultGame.ini, etc.
Read(/Documents/**)                             Design docs referenced by CLAUDE.md
```

The engine rule matters more than it looks: GAS and Enhanced Input often can
only be answered by reading engine headers directly.

### allow — Edits

```
Edit(/Source/**)
Edit(/Plugins/**)
Edit(/Documents/**)
```

`Edit` rules cover every file-editing tool, including `Write` and `MultiEdit`.
Writing a `Write(...)` or `MultiEdit(...)` path rule instead does nothing —
Claude Code accepts it but never consults it, and warns at startup.

`Config/**` is deliberately readable but not editable: a bad edit to
`DefaultEngine.ini` can make the project fail to open.

### allow — Read-only git

```
Bash(git diff:*)
Bash(git status:*)
Bash(git log:*)
Bash(git show:*)
Bash(git add:*)
```

Inspection and staging only. `git commit` is intentionally absent so commit
messages always get a human review — see the English-only commit message rule
in `CLAUDE.md`.

### allow — Build verification

```
Bash("C:\Program Files\...\Build.bat" EWEEditor *)
Bash("C:\Program Files\...\Build.bat" EWE *)
PowerShell(& "C:\Program Files\...\Build.bat" *)
```

`CLAUDE.md` requires a build after every C++ change. These rules let that run
without a prompt per iteration. **Machine-specific — see the top of this file.**

### deny — Secrets

```
Read(//**/.env)
Read(~/.ssh/**)
```

Filesystem-wide, not project-scoped. A `Read` deny rule also blocks `Edit` and
`Write` on the same path.

### deny — Push protection

```
Bash(git push:*)
```

Deny beats allow at every scope, so this holds even in `bypassPermissions` mode.
Push manually.

### deny — Generated directories

```
Edit(/Binaries/**)
Edit(/Intermediate/**)
Edit(/DerivedDataCache/**)
Edit(/Saved/**)
```

Build output. Editing these does nothing useful and can corrupt a build.

---

## Line ending policy

`.gitattributes` stores LF in the repository and checks out CRLF in the working
tree. The `PostToolUse` hook additionally rewrites files Claude touches to CRLF
so the on-disk state matches what Visual Studio and Rider expect.

The hook is a no-op if it fails — it never blocks a tool call.

To verify it is active, restart Claude Code, have it edit any `.cpp` file, then
check the file still uses CRLF.

---

## What was removed

The previous `settings.local.json` had 36 accumulated rules. These were dropped:

- **One-off exploration commands** frozen into rules — `Bash(find '...' -name *Heal* -type f)`,
  `Bash(grep -r GetSubsystem '...')`, and similar. They match one exact string
  and will never fire again.
- **Command fragments** that are not runnable commands —
  `Bash(grep -E '\.sh$|\.bat$|\.cmd$')`, `Bash(Select-Object -First 1 -ExpandProperty FullName)`.
- **Wrong-tool rules** — `Bash(Get-ChildItem -Path "...")` is a PowerShell
  cmdlet written as a Bash rule.
- **Nonexistent targets** — `Bash(unreal-build-tool.exe ...)`,
  `Bash(./BuildAndRun.sh)`.
- **Malformed rules** — `Bash(\"c:\Users\...\EWE.uproject)` (broken quoting) and
  `Read(//d C:/Users/...)` (stray `//d ` prefix plus an un-normalized `C:/`
  drive letter, so it could never match).
- **Blanket shell access** — `Bash(powershell:*)`, `Bash(cmd:*)`,
  `Bash(cmd.exe:*)`. These allowed arbitrary command execution and made every
  other Bash rule redundant. Replaced with the scoped build rules above.

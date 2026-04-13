# cstatus

Renders a Claude Code status line from the JSON object emitted by the
`statusline` hook.

## Setup

**1. Build and install the binary**

```sh
brew install cjson   # one-time
make
cp cstatus ~/.claude/cstatus
```

**2. Install the template**

```sh
cp statusline.template ~/.claude/statusline.template
```

Edit `~/.claude/statusline.template` to customise your layout (see [Template](#template) below).

**3. Wire it up in `~/.claude/settings.json`**

```json
{
  "statusLine": {
    "type": "command",
    "command": "~/.claude/cstatus"
  }
}
```

The status line refreshes after each assistant message. Changes to the template take effect on the next refresh with no restart needed.

**Test it manually**

```sh
cat full-json-schema.json | ~/.claude/cstatus
```

---

## Build

```sh
brew install cjson   # macOS – one-time
make
```

## Usage

```sh
# one-shot
cat status.json | ./cstatus

# with a custom template
cat status.json | ./cstatus --template ./statusline.template

# streaming – renders one line per JSON object received
some-daemon | ./cstatus
```

## Template

Copy `statusline.template` to `~/.claude/statusline.template`. cstatus picks
it up automatically (no flag needed).

```sh
cp statusline.template ~/.claude/statusline.template
```

Each non-comment line in the template becomes one line of output. Lines
starting with `#` are ignored.

### Syntax

```
$field_name           plain value
$field_name,COLOR     value wrapped in that color
```

Literal text around fields is printed as-is.

### Fields

| Field | Description |
|---|---|
| `$current_dir` | Current directory (basename) |
| `$project_dir` | Project directory (basename) |
| `$branch` | Git branch |
| `$worktree` | Worktree name |
| `$git_worktree` | `workspace.git_worktree` |
| `$model_name` | Model display name (e.g. `Opus`) |
| `$model_id` | Model ID (e.g. `claude-opus-4-6`) |
| `$agent` | Agent name |
| `$vim_mode` | Vim mode (`NORMAL` / `INSERT` / `VISUAL` / `REPLACE`) |
| `$cost` | Total cost (e.g. `$0.05`) |
| `$duration` | Total duration (e.g. `1m 30s`) |
| `$lines_added` | Lines added (e.g. `+42`) |
| `$lines_removed` | Lines removed (e.g. `-7`) |
| `$context_pct` | Context window used (e.g. `34%`) |
| `$context_bar` | Graphical bar + pct (e.g. `[████░░░░░░] 34%`) |
| `$context_remaining` | Context window remaining |
| `$rate_5h` | 5-hour rate limit used |
| `$rate_7d` | 7-day rate limit used |
| `$session_id` | Session ID |
| `$session_name` | Session name |
| `$version` | Claude Code version |

### Colors

`RESET` `BOLD` `DIM` `CYAN` `GREEN` `YELLOW` `RED` `MAGENTA` `BLUE` `WHITE`

### Example

```
# ~/.claude/statusline.template
$model_name,CYAN  $current_dir,WHITE  $branch,GREEN  $agent,MAGENTA
$context_bar,GREEN  $cost,YELLOW  $duration,DIM
```

Output:
```
Opus  myproject  main
[████░░░░░░] 34%  $0.05  45s
```

## Help

```sh
./cstatus --help
```

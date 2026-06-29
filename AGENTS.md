# AGENTS.md

This project is **cstatus** – a C program that renders Claude Code's status line from JSON data emitted by the `statusline` hook.

## What is Claude Code's Status Line?

Claude Code (Anthropic's CLI coding agent) supports a customizable status line at the bottom of the terminal. It's configured via `statusLine` in `~/.claude/settings.json` and can run a shell script that reads JSON session data from stdin and prints text to stdout.

📖 **Official documentation**: https://code.claude.com/docs/en/statusline

**Official documentation**: https://code.claude.com/docs/en/statusline

## How cstatus Works

cstatus is a **standalone renderer** for that status line protocol:

1. It reads a JSON object from **stdin** (the same format Claude Code emits via the `statusline` hook).
2. It renders the JSON into colored terminal output using either:
   - A **template file** (`~/.claude/statusline.template` or `--template <path>`) – custom format with `$field` placeholders and `COLOR` annotations.
   - A **built-in default** layout (no template) that shows model, directory, branch, agent, context bar, cost, duration, rate limits, etc.

It can process **one-shot** input (`cat json | cstatus`) or **streaming** input (one JSON object per line from a daemon).

## Build

**Prerequisites** (macOS):
```sh
xcode-select --install        # installs cc (Apple Clang)
brew install cjson            # cJSON library for JSON parsing
```

**Build**:
```sh
make
```

**Install**:
```sh
make && cp cstatus ~/.claude/cstatus
```

The Makefile auto-locates cJSON via `pkg-config`, then Homebrew, then falls back to `-lcjson`.

## Template Format

Template file (default: `~/.claude/statusline.template`):
- Lines starting with `#` are comments – ignored.
- Each non-comment line becomes one output line.
- Syntax: literal text with `$field_name` or `$field_name,COLOR` substitutions.

**Available fields**:

| Field | JSON path | Description |
|---|---|---|
| `$current_dir` | `workspace.current_dir` (or `cwd`) | Current directory basename |
| `$project_dir` | `workspace.project_dir` | Project directory basename |
| `$git_worktree` | `workspace.git_worktree` | Git worktree name |
| `$model_name` | `model.display_name` | e.g. "Opus" |
| `$model_id` | `model.id` | e.g. "claude-opus-4-6" |
| `$agent` | `agent.name` | Agent name |
| `$vim_mode` | `vim.mode` | NORMAL / INSERT / VISUAL / REPLACE |
| `$cost` | `cost.total_cost_usd` | Formatted as `$0.05` |
| `$duration` | `cost.total_duration_ms` | Formatted as `1m 30s` |
| `$lines_added` | `cost.total_lines_added` | Formatted as `+42` |
| `$lines_removed` | `cost.total_lines_removed` | Formatted as `-7` |
| `$context_pct` | `context_window.used_percentage` | e.g. `34%` |
| `$context_bar` | `context_window.used_percentage` | Graphical `[████░░░░░░] 34%` |
| `$context_remaining` | `context_window.remaining_percentage` | e.g. `66%` |
| `$rate_5h` | `rate_limits.five_hour.used_percentage` | e.g. `23%` |
| `$rate_7d` | `rate_limits.seven_day.used_percentage` | e.g. `41%` |
| `$session_id` | `session_id` | Session identifier |
| `$session_name` | `session_name` | Custom session name |
| `$version` | `version` | Claude Code version |
| `$worktree` | `worktree.name` | Worktree name |
| `$worktree_path` | `worktree.path` | Worktree path |
| `$worktree_orig_cwd` | `worktree.original_cwd` | Directory before worktree |
| `$worktree_orig_branch` | `worktree.original_branch` | Branch before worktree |
| `$branch` | `worktree.branch` (falls back to `workspace.git_worktree`) | Git branch |
| `$transcript_path` | `transcript_path` | Path to transcript |
| `$repo_host` | `workspace.repo.host` | e.g. "github.com" |
| `$repo_owner` | `workspace.repo.owner` | e.g. "anthropics" |
| `$repo_name` | `workspace.repo.name` | e.g. "claude-code" |
| `$output_style` | `output_style.name` | e.g. "default" |
| `$effort_level` | `effort.level` | low/medium/high/xhigh |
| `$thinking_enabled` | `thinking.enabled` | 🧠 (or empty) |
| `$exceeds_200k` | `exceeds_200k_tokens` | ⚠️ (or empty) |
| `$api_duration` | `cost.total_api_duration_ms` | e.g. `2s` |
| `$input_tokens` | `context_window.total_input_tokens` | Token count |
| `$output_tokens` | `context_window.total_output_tokens` | Token count |
| `$context_size` | `context_window.context_window_size` | Max tokens |
| `$input_tokens_current` | `context_window.current_usage.input_tokens` | Current input tokens |
| `$output_tokens_current` | `context_window.current_usage.output_tokens` | Current output tokens |
| `$cache_creation_tokens` | `context_window.current_usage.cache_creation_input_tokens` | Cache creation tokens |
| `$cache_read_tokens` | `context_window.current_usage.cache_read_input_tokens` | Cache read tokens |
| `$rate_5h_resets_at` | `rate_limits.five_hour.resets_at` | Date/time |  
| `$rate_7d_resets_at` | `rate_limits.seven_day.resets_at` | Date/time |

**Available colors**: `RESET`, `BOLD`, `DIM`, `CYAN`, `GREEN`, `YELLOW`, `RED`, `MAGENTA`, `BLUE`, `WHITE`

### Example template
```
$model_name,CYAN  $current_dir,WHITE  $branch,GREEN  $agent,MAGENTA
$context_bar,GREEN  $cost,YELLOW  $duration,DIM
```

Output:
```
Opus  myproject  main  [reviewer]
[████░░░░░░] 34%  $0.05  45s
```

## JSON Data Model

cstatus accepts the same JSON that Claude Code's `statusline` hook emits. The full schema is in `full-json-schema.json`. Key sections:

```json
{
  "cwd": "/current/working/directory",
  "session_id": "abc123...",
  "session_name": "my-session",
  "model": {
    "id": "claude-opus-4-6",
    "display_name": "Opus"
  },
  "workspace": {
    "current_dir": "/current/working/directory",
    "project_dir": "/original/project/directory",
    "git_worktree": "feature-xyz",
    "repo": {
      "host": "github.com",
      "owner": "anthropics",
      "name": "claude-code"
    }
  },
  "version": "2.1.90",
  "cost": {
    "total_cost_usd": 0.01234,
    "total_duration_ms": 45000,
    "total_api_duration_ms": 2300,
    "total_lines_added": 156,
    "total_lines_removed": 23
  },
  "context_window": {
    "total_input_tokens": 15500,
    "total_output_tokens": 1200,
    "context_window_size": 200000,
    "used_percentage": 8,
    "remaining_percentage": 92,
    "current_usage": {
      "input_tokens": 8500,
      "output_tokens": 1200,
      "cache_creation_input_tokens": 5000,
      "cache_read_input_tokens": 2000
    }
  },
  "effort": { "level": "high" },
  "thinking": { "enabled": true },
  "rate_limits": {
    "five_hour":   { "used_percentage": 23.5, "resets_at": 1738425600 },
    "seven_day":   { "used_percentage": 41.2, "resets_at": 1738857600 }
  },
  "vim": { "mode": "NORMAL" },
  "agent": { "name": "security-reviewer" },
  "pr": {
    "number": 1234,
    "url": "https://github.com/anthropics/claude-code/pull/1234",
    "review_state": "pending"
  },
  "worktree": {
    "name": "my-feature",
    "path": "/path/to/.claude/worktrees/my-feature",
    "branch": "worktree-my-feature",
    "original_cwd": "/path/to/project",
    "original_branch": "main"
  }
}
```

**Fields that may be absent** (not present in JSON): `session_name`, `workspace.git_worktree`, `workspace.repo`, `effort`, `vim`, `agent`, `pr`, `worktree`, `rate_limits`.

**Fields that may be `null`**: `context_window.current_usage`, `context_window.used_percentage`, `context_window.remaining_percentage`.

## Source Code Layout

```
cstatus.c          Main source – JSON reader, template renderer, default renderer
Makefile           Build system (cc + cJSON)
statusline.template Default template
full-json-schema.json Reference JSON schema (also in Claude Code docs)
example-1.sh       Bash status line example (jq-based, no cstatus)
example-2.py       Python status line example (no cstatus)
README.md          User-facing docs
AGENTS.md          This file – agent-facing docs
```

### Key code sections in `cstatus.c`

| Function | Purpose |
|---|---|
| `read_object()` | Parse one complete JSON object from stdin (handles nested braces, strings, escapes) |
| `jnav()` / `jstr()` / `jnum()` | cJSON helpers for dot-path navigation and type-safe access |
| `resolve_field()` | Map field name to display string from JSON |
| `render_template_line()` | Walk a template line, substitute `$fields` with optional colors |
| `load_template()` | Load template file, skip comments and blank lines |
| `render()` | Built-in default layout (no template) |
| `print_help()` | `--help` output |

## Testing

Test with the sample JSON:
```sh
cat full-json-schema.json | ./cstatus
```

Test with a template:
```sh
cat full-json-schema.json | ./cstatus --template statusline.template
```

## Configuration in Claude Code

To wire cstatus into Claude Code, add to `~/.claude/settings.json`:
```json
{
  "statusLine": {
    "type": "command",
    "command": "~/.claude/cstatus"
  }
}
```

## Development Notes

- **Language**: C99 with `-O2` optimization.
- **Dependency**: cJSON (only dependency).
- **ANSI colors**: Hardcoded escape codes (RESET, BOLD, DIM, CYAN, GREEN, YELLOW, RED, MAGENTA, BLUE, WHITE).
- **Unicode**: Uses UTF-8 block characters `█` (\xe2\x96\x88) and `░` (\xe2\x96\x91) for progress bars.
- **Streaming**: The main loop reads one JSON object at a time from stdin, so it works for both one-shot and streaming input.
- **Template**: Up to 32 lines, 512 chars each. Lines starting with `#` (after whitespace) are comments.

## References

- Claude Code status line docs: https://code.claude.com/docs/en/statusline
- cJSON library: https://github.com/DaveGamble/cJSON
- ANSI escape codes: https://en.wikipedia.org/wiki/ANSI_escape_code

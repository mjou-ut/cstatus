#!/bin/sh
input=$(cat)

# Folder (basename of cwd)
folder=$(basename "$(echo "$input" | jq -r '.workspace.current_dir // .cwd // ""')")

# Git branch (skip optional locks)
branch=$(GIT_OPTIONAL_LOCKS=0 git -C "$(echo "$input" | jq -r '.workspace.current_dir // .cwd // "."')" symbolic-ref --short HEAD 2>/dev/null)

# Model display name
model=$(echo "$input" | jq -r '.model.display_name // ""')

# Context window used percentage (pre-calculated)
used=$(echo "$input" | jq -r '.context_window.used_percentage // empty')

# Build 10-char progress bar
if [ -n "$used" ]; then
  filled=$(printf "%.0f" "$(echo "$used * 10 / 100" | bc -l 2>/dev/null || echo 0)")
  filled=$(( filled > 10 ? 10 : filled ))
  empty=$(( 10 - filled ))
  bar=""
  i=0
  while [ $i -lt $filled ]; do bar="${bar}█"; i=$(( i + 1 )); done
  i=0
  while [ $i -lt $empty ]; do bar="${bar}░"; i=$(( i + 1 )); done
  used_d=$(printf "%.0f" "$used")
  ctx="[${bar}] ${used_d}%"
else
  ctx=""
fi

# Assemble
out="$folder"
[ -n "$branch" ]  && out="$out  $branch"
[ -n "$model" ]   && out="$out  $model"
[ -n "$ctx" ]     && out="$out  $ctx"

printf "%s" "$out"

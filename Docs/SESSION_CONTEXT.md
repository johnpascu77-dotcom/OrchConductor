# \# OrchConductor Session Context

# 

# \## Collaboration Workflow Charter

# 

# This repository uses the online Git repository as the shared \*\*intelligence buffer\*\* between the local development machine and the AI assistant.

# 

# The purpose of this workflow is to reduce manual snippet-pasting, avoid stale assumptions, and make future sessions safer and less painful.

# 

# \---

# 

# \## Core Model

# 

# ```text

# Local repository = build truth and working machine

# GitHub branch    = shared readable source snapshot / intelligence buffer

# AI assistant     = reads pushed source, reasons about changes, proposes patches

# ```

# 

# The AI assistant cannot directly inspect the local filesystem, run CMake, run Git, or execute the probe tools on the developer machine.

# 

# Therefore:

# 

# \- GitHub is used for source inspection.

# \- The local terminal is used for build/probe validation.

# \- Compiler errors and probe output are pasted back only when needed.

# 

# \---

# 

# \## Session Start Protocol

# 

# At the start of each development session, the developer should provide:

# 

# ```text

# Repo:

# Branch:

# Current commit:

# Goal:

# ```

# 

# Example:

# 

# ```text

# Repo: https://github.com/<user>/OrchConductor

# Branch: phase-5G-runtime-catalog-authority-trial-diagnostic

# Current commit: a0abc77

# Goal: start Phase 5H

# ```

# 

# Before proposing patches, the assistant should inspect the pushed branch when a repository URL/branch is available.

# 

# The assistant should avoid asking for source snippets that are already available in the pushed branch.

# 

# \---

# 

# \## Push Early, Inspect Remotely

# 

# When beginning a new phase or when the assistant needs source context, push the current branch:

# 

# ```powershell

# git status --short

# git branch --show-current

# git push -u origin HEAD

# ```

# 

# If the branch is pushed, the assistant should treat GitHub as the default source-reading mechanism.

# 

# The assistant should only ask for pasted source snippets when:

# 

# 1\. the relevant changes are local and unpushed,

# 2\. the repo/branch is inaccessible,

# 3\. the issue involves generated files or build artifacts,

# 4\. the issue depends on local compiler/build output,

# 5\. GitHub is stale relative to the failing local state.

# 

# \---

# 

# \## Local Machine Responsibilities

# 

# The developer’s local machine remains authoritative for:

# 

# \- CMake configuration

# \- compiler errors

# \- build success/failure

# \- executable paths

# \- probe output

# \- local uncommitted changes

# \- platform-specific behavior

# \- generated files and build directories

# 

# For build failures, the developer should paste:

# 

# \- the first compiler error block,

# \- relevant CMake error output,

# \- or the final probe PASS/FAIL output.

# 

# The assistant should not ask for broad file dumps unless remote inspection is impossible or stale.

# 

# \---

# 

# \## Preferred Patch Flow

# 

# Prefer Git-native patches when practical:

# 

# ```powershell

# git apply --check .\\phase.patch

# git apply .\\phase.patch

# ```

# 

# PowerShell patch scripts are acceptable when they are safer or easier on Windows, but should be used carefully and idempotently.

# 

# Preferred order:

# 

# 1\. inspect pushed branch,

# 2\. propose minimal patch,

# 3\. developer applies patch,

# 4\. developer builds locally,

# 5\. developer runs probe/test locally,

# 6\. developer reports only build/probe result,

# 7\. fix if needed,

# 8\. commit,

# 9\. push.

# 

# \---

# 

# \## Handling Local Uncommitted Failure States

# 

# If a build failure occurs after local uncommitted edits, GitHub may not reflect the failing state.

# 

# In that case, use one of these options.

# 

# \### Option A: Commit and push WIP on the feature branch

# 

# ```powershell

# git add .

# git commit -m "WIP local failure state"

# git push

# ```

# 

# \### Option B: Push a temporary debug branch

# 

# ```powershell

# git checkout -b debug/<phase>-local-failure

# git add .

# git commit -m "Debug local failure state"

# git push -u origin HEAD

# ```

# 

# \### Option C: Export a local diff

# 

# ```powershell

# git diff > local\_failure.patch

# ```

# 

# Then provide only the relevant diff/error context.

# 

# \---

# 

# \## Build Artifacts and Temporary Files

# 

# Build directories and temporary scripts should not be committed.

# 

# Common cleanup:

# 

# ```powershell

# Remove-Item .\\build-\* -Recurse -Force -ErrorAction SilentlyContinue

# Remove-Item .\\phase\*\_\*.ps1 -ErrorAction SilentlyContinue

# Remove-Item .\\phase\*\_\*.txt -ErrorAction SilentlyContinue

# ```

# 

# Before commit:

# 

# ```powershell

# git status --short

# git diff --cached --check

# ```

# 

# Expected tracked project changes should be deliberate and minimal.

# 

# \---

# 

# \## Commit Protocol

# 

# Before committing:

# 

# ```powershell

# git status --short

# git diff --stat

# git diff --check

# ```

# 

# Then stage only intended files:

# 

# ```powershell

# git add <intended-files>

# git diff --cached --stat

# git diff --cached --check

# ```

# 

# Commit only when `git diff --cached --check` reports no whitespace errors.

# 

# Example commit:

# 

# ```powershell

# git commit -m "Add passive runtime catalog authority trial diagnostic"

# ```

# 

# After commit, push:

# 

# ```powershell

# git push

# ```

# 

# \---

# 

# \## Assistant Operating Rules for This Repository

# 

# When a GitHub branch is available, the assistant should:

# 

# 1\. inspect the pushed files before proposing source changes,

# 2\. avoid asking for pasted snippets from files already available remotely,

# 3\. clearly distinguish remote pushed state from local uncommitted state,

# 4\. prefer minimal, reviewable patches,

# 5\. prefer `git apply` patches when feasible,

# 6\. use PowerShell patch scripts only when appropriate,

# 7\. keep diagnostics passive unless explicitly instructed otherwise,

# 8\. preserve existing build and probe behavior unless the phase goal requires a change,

# 9\. ask for local build/probe output only when needed,

# 10\. avoid “manual patching just like that” without first establishing source context.

# 

# \---

# 

# \## Validation Pattern

# 

# For each phase, record:

# 

# ```text

# Phase:

# Branch:

# Commit:

# Build command:

# Probe/test command:

# Result:

# Notes:

# ```

# 

# Example:

# 

# ```text

# Phase: 5G runtime catalog authority trial diagnostic

# Branch: phase-5G-runtime-catalog-authority-trial-diagnostic

# Commit: a0abc77

# Build command:

# &#x20; cmake -S . -B build-phase5G-authority-on -DORCHCONDUCTOR\_ENABLE\_RUNTIME\_JSON\_PRESETS=ON -DORCHCONDUCTOR\_ENABLE\_RUNTIME\_CATALOG\_AUTHORITY\_TRIAL=ON

# &#x20; cmake --build build-phase5G-authority-on --target OrchConductorProcessorJsonProbeCheck --config Debug

# Probe/test command:

# &#x20; .\\build-phase5G-authority-on\\OrchConductorProcessorJsonProbeCheck\_artefacts\\Debug\\OrchConductorProcessorJsonProbeCheck.exe

# Result:

# &#x20; PASS

# Notes:

# &#x20; Authority trial diagnostic remains passive and source-backed.

# ```

# 

# \---

# 

# \## Current Stable Milestone

# 

# Phase 5G is committed.

# 

# ```text

# Commit: a0abc77

# Message: Add passive runtime catalog authority trial diagnostic

# Result: PASS

# ```

# 

# Phase 5G added a passive runtime catalog authority trial diagnostic that is:

# 

# \- feature-gated,

# \- fallback-blocked,

# \- source-backed catalog aware,

# \- probe-verified,

# \- documented.

# 

# \---

# 

# \## Next Session Reminder

# 

# At the beginning of the next session:

# 

# 1\. push the current branch if not already pushed,

# 2\. provide repo URL, branch, commit, and goal,

# 3\. assistant should inspect GitHub before proposing changes,

# 4\. local build/probe output should be pasted only when needed.

# 

# 


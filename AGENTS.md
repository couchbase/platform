# AGENTS.md — platform Repository Guide

Repository (mirror): https://github.com/couchbase/platform Primary code review
system: https://review.couchbase.org

---

## 📌 Purpose

This document defines expectations for:

- Automated coding agents
- Human contributors
- Reviewers

platform uses Gerrit (review.couchbase.org) for all change submission,
validation, and approval.

GitHub is a mirror only. Do NOT open GitHub Pull Requests.

---

## 🧱 Repository Overview

Welcome to the Couchbase _platform_ project. It contains functionality that is
shared across multiple Couchbase projects, including the kv_engine and
magma. It started off as a repository to hide differences between various
operating systems such as POSIX compatible systems and Windows.
The code in this repository is used by multiple projects, so changes must be
made with care to avoid unintended consequences in any of the consuming
products.

## 🚦 Authoritative Contribution Workflow

All changes must be submitted via:

    https://review.couchbase.org

### Standard Flow

1. Sync with latest target branch (typically `master`)
2. Create a local topic branch (if `repo` is available):

       `repo start name-of-topic-branch`

3. Make atomic, focused commits
4. Always create suitable unit tests
5. Run local build and tests
6. Sync with upstream and rebase the change. If `repo` is available:

       repo sync .
       repo rebase .

7. Push to Gerrit (substitute master for correct branch when working on named
   branches):

       git push gerrit HEAD:refs/for/master

   Alternatively, if the `repo` tool is available:

       repo upload --cbr .

8. CI validation runs automatically on jenkins servers that are monitoring for
   new code pushed to gerrit. 8. Review feedback is asynchronous

---

## 📐 Code Standards

Language: **C++ (up to C++20 supported)**

Guidelines:

- Prefer modern C++ where appropriate (C++17/20 features are supported)
- Use RAII, smart pointers, and standard library facilities where they improve
  safety and clarity
- Structured bindings, `std::optional`, `std::variant`, `constexpr`, and other
  modern features are acceptable
- Match the surrounding style within the modified module.
- It is acceptable to use older constructs if required for consistency with the
  existing code in that area
- Avoid unnecessary large-scale modernization in unrelated code
- Maintain cross-platform compatibility
- New source files must use the current year in the banner comment.
- The `C++ Core Guidelines<http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines>`
  are also a key part of our coding style.
- Always build (see building hints and testing hints) before running tests!

---

## Building hints

The project uses CMake for build configuration.

- To build all code (including all tests) use the target `platform_everything` 
  which can be invoked from `build/`
- Prefer to reuse an existing build directory and do incremental builds.
  - Do not create a new build directory unless asked.
  - Check for an existing `build/` directory in the directory above the
   `project` git repository.
- Prefer `ninja` when building.
- If configuring a new build prefer `ninja` as build tool (`cmake -G Ninja`)

## Testing hints

Tests may be run from the `build/platform` directory using `ctest`

---

## 🚨 Safety Rules

Agents must not:

- Change file formats silently
- Break backward compatibility unintentionally
- Reformat large areas of code
- Submit speculative refactors
- Bypass Gerrit workflow
- Create new build directories unless asked

## 📝 Commit Message Requirements

All commit messages must:

- Start with: `MB-XXXXX: Short summary` where `MB-XXXXX` is the relevant
  Jira issue key describing the change. Should not exceed 60 characters in
  length (ideally less than 50 characters).
- Be wrapped at **72 characters per line**
- Include a clear explanation of:
  - What changed
  - Why it changed
  - Risk or compatibility impact
- Don't add a 'Co-authored-by' tag for automated agents. The author information
  is already captured in the commit metadata.

Example:

```
MB-70803: Add RingBuffer and unit tests

Add a RingBuffer<T> template class backed by a std::vector that
overwrites the oldest element when the buffer reaches capacity.
    
Public API:
  - push(T)       — append, overwriting oldest entry when full
  - iterate(fn)   — visit elements oldest-to-newest via callback
  - size()        — current element count
  - capacity()    — maximum element count (fixed at construction)
  - is_empty() / is_full()
```

### Change-Id Handling

Do NOT manually add or generate a `Change-Id`.

A local Gerrit commit-msg hook is expected to already be installed and will
automatically add the required `Change-Id` footer when the commit is created
or amended.

Agents must not fabricate or modify the Change-Id.

When amending an existing commit (e.g. to update the commit message or add
further changes), preserve the `Change-Id` line exactly as it appears in the
original commit message. Dropping or replacing the `Change-Id` on an amended
commit would create a new Gerrit review instead of updating the existing one.

---

## 🧪 Testing Requirements

All functional changes must include:

- New or updated unit tests
- Passing test suite locally
- No unintended regression in file format compatibility


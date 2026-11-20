# Native rewrite status

- Track: Windows-native C++20 rewrite
- Build: CMake + MSVC x64
- CI: `.github/workflows/native-windows.yml`
- Current milestone: Stage 1 input/overlay/settings scaffold
- Next: verify the Windows Actions build, then split modules into headers/sources and harden input handling.

This file also provides a post-workflow-creation push so the native Windows workflow is evaluated on the rewrite branch.

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

优先用中文回复

不要在乎我的情感感受，如果你觉得我说的太离谱了，你就骂回来，帮我瞬间清醒

## Project Overview

This is zdf-exam-desktop, a secure browser application built with Qt 5 and QtWebEngine, designed specifically for educational examination environments. The application provides a controlled, locked-down browsing environment that:

- Restricts access to specific URLs only (configurable)
- Disables system shortcuts and key combinations  
- Provides secure exit mechanism (F10 or \ key with password)
- Runs in fullscreen, borderless, always-on-top mode
- Logs all operations for security auditing
- Supports external configuration without recompilation

## Development Commands

### Build Commands

**macOS Build:**
```bash
cd zdf-exam-desktop
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Automated macOS Build & Package:**
```bash
cd zdf-exam-desktop
./build-macos.sh
```

**General Qt Build:**
```bash
cd zdf-exam-desktop
mkdir build && cd build
cmake ..
make
```

**Windows Build (if on Windows):**
```bat
cd zdf-exam-desktop
mkdir build && cd build
cmake .. -G "NMake Makefiles" -DCMAKE_PREFIX_PATH="%QT_DIR%"
nmake
```

### Prerequisites

- Qt 5.12+ with QtWebEngine module
- CMake 3.14+
- QHotkey library (included as submodule in QHotkey/)
- For macOS: `brew install qt@5 jq`
- For Windows: Visual Studio 2019/2022 with Windows SDK

### No Testing Framework

This project does not use any automated testing framework. Testing is done manually by running the application and verifying the security features work correctly.

## Architecture

### Core Components

1. **Logger Class** - Centralized logging system with categorized log files:
   - `app.log` - General application events
   - `config.log` - Configuration-related events  
   - `exit.log` - Exit attempt logging (important security feature)
   - `startup.log` - Application startup events
   - `error.log` - Error events
   - `system.log` - System information for diagnostics

2. **ConfigManager Class** - Dynamic configuration loading with fallback hierarchy:
   - Searches multiple locations for config.json in priority order
   - Creates default config if none found
   - Validates required fields (url, exitPassword, appName)

3. **ShellBrowser Class** - Main application window extending QWebEngineView:
   - Implements security restrictions and event filtering
   - Handles global hotkeys for secure exit
   - Maintains fullscreen/focus enforcement
   - Blocks system shortcuts while allowing essential web functionality

4. **GlobalEventFilter Class** - System-wide event interception:
   - Blocks dangerous key combinations (Alt+Tab, Ctrl+Alt+Del, etc.)
   - Allows only specific shortcuts (Ctrl+R for refresh)
   - Prevents window state changes from fullscreen

### Configuration System

The application uses a dynamic configuration system that searches for `config.json` in this priority order:
1. Executable directory (highest priority)
2. User config directory (AppData/Application Support)
3. System config directory (Unix only)
4. Built-in resources directory

Required configuration fields:
- `url`: Target website URL
- `exitPassword`: Password for secure exit
- `appName`: Application window title

### Security Features

- **Keyboard Blocking**: Comprehensive system shortcut interception
- **Window Control**: Forced fullscreen with focus maintenance
- **Exit Control**: Password-protected exit via F10 or \ key
- **Context Menu**: Disabled to prevent right-click access
- **Logging**: All security events are logged for audit trails

## Important Implementation Details

### Event Filtering Strategy
The application uses a two-layer approach:
1. **Global Event Filter**: Catches system-level events before they reach the application
2. **Browser Event Override**: Fine-grained control within the web view component

### Hot Key Implementation
Uses QHotkey library for global hotkey registration, essential for exit functionality when all other keys are disabled.

### WebEngine Configuration
Carefully configured QtWebEngine settings for exam environment:
- Hardware acceleration conditionally disabled on older Windows versions
- JavaScript enabled for modern web functionality
- Screen capture and WebRTC restricted for security

### Windows 7 Compatibility
Special handling for Windows 7 systems:
- Detects Windows version using QVersionNumber
- Disables hardware acceleration and GPU features on Windows 7
- Uses single-process mode to avoid compatibility issues
- Sets extensive Chromium flags for older system compatibility

## Configuration Guidelines

When modifying configuration-related code:
- Always validate JSON structure and required fields
- Maintain the configuration priority hierarchy
- Update both the README.md and docs/config-guide.md if configuration options change
- Test configuration loading across different deployment scenarios

## Build and Deployment

The project includes comprehensive build scripts:
- `build-macos.sh`: Complete macOS build, signing, and DMG creation
- Handles multiple icon formats (PNG, ICO, SVG) with automatic conversion
- Integrates Qt dependency deployment via macdeployqt
- Includes code signing for distribution

For Windows deployment, see docs/windows-packaging-guide.md for detailed instructions.

## File Structure

- `zdf-exam-desktop/` - Main Qt application
  - `main.cpp` - Single file containing all classes and main function
  - `CMakeLists.txt` - Build configuration
  - `QHotkey/` - Global hotkey library submodule
  - `build-macos.sh` - macOS build and packaging script
- `resources/` - Configuration and assets
  - `config.json` - Application configuration
  - Logo files in various formats
- `docs/` - Documentation
  - `config-guide.md` - Configuration file usage guide
  - `windows-packaging-guide.md` - Windows deployment guide
- `.github/workflows/` - CI/CD for automated Windows builds

## Testing Considerations

When testing changes:
- Verify keyboard blocking works correctly across all target platforms
- Test configuration loading from different locations  
- Ensure exit hotkeys function properly
- Validate fullscreen enforcement
- Check log file generation and content
- Test on Windows 7 for compatibility issues
- Verify WebEngine functionality with different GPU drivers

## Security Notes

This is a legitimate exam browser application with security features designed to prevent cheating. The code blocks system shortcuts and enforces fullscreen mode for educational purposes. All security features are documented and transparent.
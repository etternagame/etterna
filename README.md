# Etterna - Open Source Rhythm Game

[![Build Status](https://github.com/etternagame/etterna/workflows/Build/badge.svg)](https://github.com/etternagame/etterna/actions)
[![Discord](https://img.shields.io/discord/441805741727645697.svg)](https://discord.gg/8BJK9tK)
[![GitHub Release](https://img.shields.io/github/v/release/etternagame/etterna)](https://github.com/etternagame/etterna/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

> The next-generation open source rhythm game based on StepMania, focusing on high-precision scoring and customization.

---

## Table of Contents
- [Features](#-features)
- [Quick Start](#-quick-start)
- [Building from Source](#-building-from-source)
- [Contributing](#-contributing)
- [Frequently Asked Questions](#-frequently-asked-questions)
- [Community & Support](#-community--support)
- [License](#-license)

---

## Features

### Core Gameplay
- **High-Precision Scoring System** - Industry-leading scoring algorithms
- **Custom Theme Support** - Fully customizable interface
- **Multi-Platform** - Windows, Linux, macOS
- **Online Leaderboards** - Compete with players worldwide
- **Extensive Song Library** - Support for thousands of community charts

### Technical Features
- **Modern C++ Codebase** - High-performance game engine
- **Lua Scripting Support** - Flexible extension mechanism
- **OpenGL Rendering** - Smooth visual effects
- **Cross-Platform Build System** - CMake based
- **Modular Architecture** - Easy to extend and maintain

---

## Quick Start

### For Windows Users
#### Option 1: Installer (Recommended for Beginners)
1. Visit [Releases Page](https://github.com/etternagame/etterna/releases)
2. Download the latest `Etterna-vX.X.X-Setup.exe`
3. Run the installer and follow the instructions
4. Launch Etterna and start playing!

#### Option 2: Portable Version
1. Download `Etterna-vX.X.X-Portable.zip`
2. Extract to any directory
3. Run `Etterna.exe`

### For Linux Users
```bash
# Ubuntu/Debian (using .deb package)
wget https://github.com/etternagame/etterna/releases/download/v0.70.1/etterna_0.70.1_amd64.deb
sudo dpkg -i etterna_0.70.1_amd64.deb

# Using AppImage (most distributions)
chmod +x Etterna-*.AppImage
./Etterna-*.AppImage

# Arch Linux (AUR)
yay -S etterna
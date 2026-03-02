# Project Developement Guide

# UI Framework

This is a cross platform pixel-acurate UI framework.

## Features

- Support dock area panel
- Basic canvas drawing with Skia API
- Window/Menu management
- Swift bindings

## Rules

- App framework (folder: `src/coreui`) written in C++
- UI Framework API mimics Apple's UIKit
- Graphics drawing functionality leaverages skia C++
- UI events' definitions corresponds UIKit events
- Make build.bat passed after code change
- All api changes should be considered to support cross platform include Windows, Linux and macOS


# Instructions

see [Instructions](doc/Plan.md)

## Build and Compile Instructions

- Windows: `build.bat`
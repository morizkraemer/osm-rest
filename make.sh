#!/bin/bash

# Set the number of CPU cores for parallel builds
JOBS=$(nproc)

# Ensure we are in the build directory
mkdir -p build
cd build

# Only run qmake if Makefile is missing
if [ ! -f Makefile ]; then
    echo "🔧 Running qmake..."
    qmake ../OpenSoundMeter.pro
fi

# Enable ccache for faster builds
export CXX="ccache g++"
export CC="ccache gcc"

# Generate compile_commands.json if it doesn’t exist
if [ ! -f compile_commands.json ]; then
    echo "🐻 Running Bear to generate compile_commands.json..."
    bear -- make -j$JOBS
else
    echo "⚡ Incremental build..."
    make -j$JOBS
fi

echo "✅ Build completed!"

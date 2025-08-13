#!/bin/bash

# Error handling: exit on any error, undefined variables, or pipe failures
set -euo pipefail

# Enable debug mode if DEBUG environment variable is set
if [[ "${DEBUG:-}" == "1" ]]; then
    set -x
fi

WORKSPACE_DIR="${1:-}"
CLANGD_VERSION="${CLANGD_VERSION:-20.1}"  # Default to version 15, can be overridden

# Logging functions
log_info() {
    echo "[INFO] $*" >&2
}

log_error() {
    echo "[ERROR] $*" >&2
}

log_warning() {
    echo "[WARNING] $*" >&2
}

# Validate workspace directory
if [[ -z "$WORKSPACE_DIR" ]]; then
    log_error "Workspace directory not provided"
    exit 1
fi

if [[ ! -d "$WORKSPACE_DIR" ]]; then
    log_error "Workspace directory does not exist: $WORKSPACE_DIR"
    exit 1
fi

log_info "Setting up development environment in: $WORKSPACE_DIR"

# Update package lists with error handling
log_info "Updating package lists..."
if ! apt update; then
    log_error "Failed to update package lists"
    exit 1
fi

# Install essential build tools and CMake
log_info "Installing build tools and CMake..."
PACKAGES=(
    "build-essential"
    "cmake"
    "ninja-build"
    "pkg-config"
)

for package in "${PACKAGES[@]}"; do
    if dpkg -l | grep -q "^ii  $package "; then
        log_info "$package is already installed"
    else
        log_info "Installing $package..."
        if ! apt install -y "$package"; then
            log_error "Failed to install $package"
            exit 1
        fi
    fi
done

# Install clangd with specific version
log_info "Installing clangd version $CLANGD_VERSION..."
if apt list --installed 2>/dev/null | grep -q "clangd-$CLANGD_VERSION"; then
    log_info "clangd-$CLANGD_VERSION is already installed"
else
    # Try to install specific version first, fallback to generic clangd
    if apt install -y "clangd-$CLANGD_VERSION" 2>/dev/null; then
        log_info "Successfully installed clangd-$CLANGD_VERSION"
    elif apt install -y clangd; then
        log_warning "Specific version clangd-$CLANGD_VERSION not available, installed default clangd"
    else
        log_error "Failed to install clangd"
        exit 1
    fi
fi

# Verify installations
log_info "Verifying installations..."

# Verify CMake
if command -v cmake >/dev/null 2>&1; then
    CMAKE_VERSION=$(cmake --version | head -n1 || echo "Unknown version")
    log_info "CMake installation verified: $CMAKE_VERSION"
else
    log_error "CMake installation failed - command not found"
    exit 1
fi

# Verify clangd
if command -v clangd >/dev/null 2>&1; then
    CLANGD_INSTALLED_VERSION=$(clangd --version | head -n1 || echo "Unknown version")
    log_info "clangd installation verified: $CLANGD_INSTALLED_VERSION"
else
    log_error "clangd installation failed - command not found"
    exit 1
fi

# Verify ninja
if command -v ninja >/dev/null 2>&1; then
    NINJA_VERSION=$(ninja --version || echo "Unknown version")
    log_info "Ninja build system verified: version $NINJA_VERSION"
else
    log_warning "Ninja build system not found"
fi

log_info "Development environment setup completed successfully"
log_info "Available tools:"
log_info "  - CMake: $(cmake --version | head -n1)"
log_info "  - GCC: $(gcc --version | head -n1)"
log_info "  - clangd: $(clangd --version | head -n1)"
#!/bin/bash
#
# run_dev.sh - Development Server Launcher
# UWB Proximity Chat Hub
#
# Usage:
#   ./scripts/run_dev.sh
#
# This script starts the FastAPI server in development mode with
# auto-reload enabled. Suitable for local testing and development.
#
# For production deployment, use systemd service instead.

set -e  # Exit on error

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=========================================="
echo "  UWB Proximity Chat Hub"
echo "  Development Server"
echo "=========================================="
echo ""

# Change to project directory
cd "$PROJECT_DIR"
echo "Project directory: $PROJECT_DIR"
echo ""

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "⚠️  Virtual environment not found!"
    echo "Creating virtual environment..."
    python3 -m venv venv
    echo "✅ Virtual environment created"
    echo ""
fi

# Activate virtual environment
echo "Activating virtual environment..."
source venv/bin/activate
echo "✅ Virtual environment activated"
echo ""

# Check if requirements are installed
if ! python -c "import fastapi" 2>/dev/null; then
    echo "⚠️  Dependencies not installed!"
    echo "Installing requirements..."
    pip install --upgrade pip
    pip install -r requirements.txt
    echo "✅ Dependencies installed"
    echo ""
fi

# Create data and logs directories if they don't exist
mkdir -p data
mkdir -p logs
echo "✅ Data and logs directories ready"
echo ""

# Check if config.yaml exists
if [ ! -f "config.yaml" ]; then
    echo "⚠️  config.yaml not found!"
    echo "Please create config.yaml before starting the server."
    exit 1
fi

# Display configuration info
echo "=========================================="
echo "  Configuration"
echo "=========================================="
echo "Config file: config.yaml"

# Extract some key config values (requires yq or python)
if command -v python3 &> /dev/null; then
    UDP_PORT=$(python3 -c "import yaml; print(yaml.safe_load(open('config.yaml'))['network']['udp_listen_port'])" 2>/dev/null || echo "9999")
    REST_PORT=$(python3 -c "import yaml; print(yaml.safe_load(open('config.yaml'))['network']['rest_port'])" 2>/dev/null || echo "8000")
    echo "UDP port:   $UDP_PORT"
    echo "REST port:  $REST_PORT"
fi

echo ""
echo "=========================================="
echo "  Network Information"
echo "=========================================="

# Display network interfaces
if command -v hostname &> /dev/null; then
    HOSTNAME=$(hostname)
    echo "Hostname: $HOSTNAME"
fi

if command -v hostname &> /dev/null; then
    IP_ADDR=$(hostname -I | awk '{print $1}' 2>/dev/null || echo "unknown")
    echo "IP address: $IP_ADDR"
    echo ""
    echo "Access UI at:"
    echo "  Local:   http://localhost:${REST_PORT:-8000}"
    echo "  Network: http://$IP_ADDR:${REST_PORT:-8000}"
fi

echo ""
echo "=========================================="
echo "  Starting Server"
echo "=========================================="
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Start server with uvicorn
# --reload: Auto-reload on code changes
# --host 0.0.0.0: Listen on all interfaces
# --port: From config or default 8000
# --log-level: Info level logging

exec uvicorn server:app \
    --host 0.0.0.0 \
    --port "${REST_PORT:-8000}" \
    --reload \
    --log-level info

# Note: exec replaces the shell process with uvicorn,
# so anything after this won't run


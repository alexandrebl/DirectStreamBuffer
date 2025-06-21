#!/bin/bash

# Build script for DirectStreamBuffer Docker container

set -e  # Exit on any error

echo "Building DirectStreamBuffer Docker image..."

# Build the Docker image
docker build -t directstreambuffer:latest .

echo "Build completed successfully!"
echo ""
echo "Usage examples:"
echo "1. Run with docker-compose (recommended):"
echo "   docker-compose up -d"
echo "   docker-compose exec directstreambuffer /bin/bash"
echo ""
echo "2. Run directly with Docker:"
echo "   docker run --privileged --network host -it directstreambuffer:latest /bin/bash"
echo ""
echo "3. Run the application (inside container):"
echo "   ./dssproto <interface> <dest-mac>"
echo "   Example: ./dssproto eth0 aa:bb:cc:dd:ee:ff"
echo ""
echo "Note: The application requires root privileges and access to network interfaces."
echo "Make sure to run the container with --privileged flag or appropriate capabilities."

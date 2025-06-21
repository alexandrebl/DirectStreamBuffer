# Multi-stage Dockerfile for DirectStreamBuffer
# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY directstreamservice.cpp .
COPY README.md .

# Compile the application
RUN g++ -std=c++17 -O2 -Wall -Wextra directstreamservice.cpp -o dssproto

# Stage 2: Runtime environment
FROM ubuntu:22.04 AS runtime

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies (minimal)
RUN apt-get update && apt-get install -y \
    net-tools \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user (though the app needs root to run)
RUN useradd -m -s /bin/bash appuser

# Set working directory
WORKDIR /app

# Copy the compiled binary from builder stage
COPY --from=builder /app/dssproto .
COPY --from=builder /app/README.md .

# Make the binary executable
RUN chmod +x dssproto

# Set ownership
RUN chown -R appuser:appuser /app

# Note: This application requires root privileges to create raw sockets
# and needs to be run with --privileged flag or specific capabilities

# Default command shows usage
CMD ["./dssproto"]

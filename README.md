# DirectStreamBuffer

A C++ raw Ethernet protocol implementation using Linux raw sockets for low-level network communication.

## Overview

This project implements a custom network protocol that operates at the Ethernet layer (Layer 2), bypassing the standard TCP/IP stack. It demonstrates:

- Raw socket programming in C++17
- Custom Ethernet protocol implementation
- CRC16 checksum validation
- Low-level network frame construction and parsing

## Features

- **Custom Ethertype**: Uses `0x88B5` for experimental protocol
- **Structured Packets**: Header with type, length, sequence, and CRC16
- **Raw Socket Interface**: Direct Ethernet frame manipulation
- **Cross-platform Build**: Docker support for consistent builds

## Building and Running

### Prerequisites

- Linux system (for raw socket support)
- Root privileges (for raw socket operations)
- Network interface access

### Option 1: Docker (Recommended)

1. **Build the Docker image:**
   ```bash
   chmod +x build.sh
   ./build.sh
   ```

2. **Run with docker-compose:**
   ```bash
   docker-compose up -d
   docker-compose exec directstreambuffer /bin/bash
   ```

   **Two containers with different MAC addresses:**
   ```bash
   # Container 1: MAC 02:42:ac:11:00:02
   docker-compose exec directstreambuffer /bin/bash

   # Container 2: MAC 02:42:ac:11:00:03
   docker-compose exec directstreambuffer-echo /bin/bash
   ```

3. **Run the application inside container:**
   ```bash
   ./dssproto <interface> <dest-mac>
   # Example: ./dssproto eth0 aa:bb:cc:dd:ee:ff
   ```

### Option 2: Direct Docker

```bash
# Build
docker build -t directstreambuffer .

# Run with privileges
docker run --privileged --network host -it directstreambuffer /bin/bash
```

### Option 3: Native Build

```bash
# Compile
g++ -std=c++17 -O2 -Wall -Wextra directstreamservice.cpp -o dssproto

# Run (requires root)
sudo ./dssproto <interface> <dest-mac>
```

## Usage

```bash
./dssproto <interface> <dest-mac>
```

**Parameters:**
- `interface`: Network interface name (e.g., eth0, enp0s3)
- `dest-mac`: Destination MAC address in format AA:BB:CC:DD:EE:FF

**Examples:**
```bash
# Basic usage
./dssproto eth0 aa:bb:cc:dd:ee:ff

# Communication between containers
./dssproto eth0 02:42:ac:11:00:03  # From container 1 to container 2
./dssproto eth0 02:42:ac:11:00:02  # From container 2 to container 1
```

## Protocol Specification

### Packet Structure
```
| Type (1B) | Length (2B) | Sequence (4B) | CRC16 (2B) | Payload (variable) |
```

- **Type**: Packet type (1 = data)
- **Length**: Payload length in bytes
- **Sequence**: Packet sequence number
- **CRC16**: CRC16-CCITT checksum
- **Payload**: Variable-length data

### Ethernet Frame
```
| Dest MAC (6B) | Src MAC (6B) | EtherType (2B) | Custom Packet | FCS (4B) |
```

## Network Configuration

### MAC Address Setup

The docker-compose configuration includes two containers with predefined MAC addresses:

- **Container 1** (`directstreambuffer`): `02:42:ac:11:00:02`
- **Container 2** (`directstreambuffer-echo`): `02:42:ac:11:00:03`

### Custom Network

- **Network**: `dssproto_network` (bridge)
- **Subnet**: `172.20.0.0/16`
- **Gateway**: `172.20.0.1`

### Testing Communication

Use the provided test script:
```bash
chmod +x test-communication.sh
./test-communication.sh
```

Or manually test between containers:
```bash
# Terminal 1 - Container 1
docker-compose exec directstreambuffer /bin/bash
./dssproto eth0 02:42:ac:11:00:03

# Terminal 2 - Container 2
docker-compose exec directstreambuffer-echo /bin/bash
./dssproto eth0 02:42:ac:11:00:02
```

## Security Notes

- Requires root privileges for raw socket creation
- Uses privileged Docker containers
- Direct network interface access
- Experimental protocol - not for production use

## Development

The project includes:
- `directstreamservice.cpp`: Main application source
- `Dockerfile`: Multi-stage build configuration
- `docker-compose.yml`: Container orchestration
- `build.sh`: Build automation script

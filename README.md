# Canasta C++23

A cross-platform (Windows, Linux, macOS) console-based implementation of the classic Canasta card game, built with modern C++23. Follows a client-server architecture to allow multiple players to connect over TCP.

## Table of Contents

1. [Project Overview](#project-overview)  
2. [Architecture](#architecture)  
   - [Backend (Server)](#backend-server)  
   - [Frontend (Client)](#frontend-client)  
3. [Libraries & Dependencies](#libraries--dependencies)  
4. [Dependency Management](#dependency-management)  
5. [Game Flow](#game-flow)  
6. [Advantages](#advantages)  
7. [Directory Structure](#directory-structure)  
8. [Getting Started](#getting-started)  
9. [Playing the Game](#playing-the-game)  

## Project Overview

This project is a terminal-based implementation of Canasta.  
Players play through individual client terminals that connect to a central server. The server enforces the rules, maintains game state, and broadcasts updates; each client renders the current hand, melds, and prompts the player for actions.

## Architecture

### Backend (Server)

- **Manages all game logic, state transitions.**  
- **Handles multiple TCP client connections**, deserializes incoming player actions, applies them in a single logical strand to prevent race conditions, and serializes per-player game-state updates back to each client.  

### Frontend (Clients)

- **Each player runs a separate terminal window.**  
- **Renders the game board, scores, and melds**; prompts the player for draw/take/meld/discard actions.  
- **Sends actions** to the server and receives updated state.

## Libraries & Dependencies

- **Cereal** – header-only serialization (binary archives)  
- **Standalone ASIO** – asynchronous TCP networking  
- **spdlog** – fast, header-only logging (console & file)  
- **FTXUI** – declarative, component-based terminal UI  
- **Conan** – C++ package manager for cross-platform dependency consistency  

## Dependency Management

All third-party libraries are managed via **Conan**, ensuring consistent versions across Linux, macOS, and Windows. Conan generates CMake toolchains and dependency files automatically.

## Game Flow

1. **Initialization**  
   - Server starts, clients connect and log in with a chosen player name.
   - Server shuffles a 108-card Canasta deck, deals initial hands.
2. **Turn Loop**  
   - Server sends the current game state to all clients.  
   - Active client chooses to draw from the deck or take the discard pile.  
   - Client may form melds and/or revert provisional melds.  
   - Client discards to end turn.  
   - Server validates, updates state, and broadcasts the new state.  
3. **End of Round**  
   - When a player empties their hand and has at least 1 canasta, round ends; server calculates breakdown and broadcasts scores.  
4. **End of Game**  
   - First team to reach the winning threshold wins; final scoreboard is displayed.

## Advantages

- **Cross-Platform**: Windows, Linux, and macOS supported out of the box.  
- **Scalable Design**: Client-server model allows easy future GUI or additional clients.  
- **Separation of Concerns**: Core game logic isolated from networking and UI layers.  
- **Hot-Swap UI**: The client view layer uses FTXUI but can be replaced without touching game logic.  

## Directory Structure
```shell
.
├── docs
│   ├── diagrams/            ← UML & rendered SVGs
│   ├── DOCUMENTATION.md     ← Developer documentation
│   ├── INSTALL.md           ← Build & install instructions
│   ├── PLAY.md              ← How to launch & play
│   ├── REQUIREMENTS.md      ← SW requirements
│   └── specification.md     ← Detailed design spec
├── src
│   ├── app
│   │   ├── client           ← Client implementation
│   │   ├── server           ← Server implementation
│   │   └── ...              ← Shared core library implementation
│   ├── include
│   │   ├── client           ← Client headers
│   │   ├── server           ← Server headers
│   │   └── ...              ← Shared core library headers
│   ├── CMakeLists.txt
│   ├── conandata.yml
│   └── conanfile.py
└── README.md                ← This file
```

## Getting Started

For detailed build and run instructions, see [docs/INSTALL.md](docs/INSTALL.md).

## Playing the Game

Once built, launch the server with 2 or 4 players:

```shell
# In src/build/Debug/
./canasta_server 2        # 2-player game   
```
See [docs/PLAY.md](docs/PLAY.md) for a full walkthrough.
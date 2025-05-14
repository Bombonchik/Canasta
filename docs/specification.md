### Canasta Game Specification

---

#### 1. Project Overview

This project is a console-based implementation of the card game **Canasta**.

---
#### 2. Project Architecture

The game will follow a **client-server model**:

1. **Backend (Server)**:

    - Manages all game logic, state transitions, and persistence.
    - Handles communication with multiple connected frontend terminals.
    - Saves the game state to a database after each turn or major event.
2. **Frontend (Clients)**:

    - Each player interacts with the game through a separate terminal window.
    - Displays the current game state and prompts the player for their actions.
    - Sends player actions (e.g., draw a card, discard, meld) to the backend and receives updated game state.

---

#### 3. Libraries Used

1. **Cereal**:

    - **Purpose**: Serialization of game state and player actions into a binary format.
    - **How It Is Used**:
        - Serialize objects like `Card`, `Hand`, `ClientDeck`, ... into compact binary data.
        - Deserialize received data from binary back into objects for processing.
    - **Why Chosen**:
        - Lightweight, header-only library with no external dependencies.
        - Supports binary serialization.
2. **Standalone ASIO**:

    - **Purpose**: Networking library for communication between server and client.
    - **How It Is Used**:
        - Backend act as a server listening for connections from multiple frontends (TCP).
        - Frontends connect to the backend, send serialized player actions, and receive serialized game state updates.
    - **Why Chosen**:
        - Lightweight and asynchronous, with excellent cross-platform support.
        - Directly compatible with Cereal for sending serialized data.
3. **spdlog**:

    - **Purpose**: Logging and debugging.
    - **How It Is Used**:
        - Log game events, player actions, and errors to a file for debugging.
        - Optionally log important information to the console for player visibility.
    - **Why Chosen**:
        - Lightweight, fast, and easy to integrate.
        - Supports both console and file logging with advanced formatting.
4. **ftxui**:

    - **Purpose**: Build interactive, component-based UI in a terminal.
    - **How It Is Used**: Instantiate a ScreenInteractive, define and compose Components (or Renderers) with layout helpers (e.g. hbox(), vbox()), then call screen.Loop(...) to render and handle input.

    - **Why Chosen**: Header-only modern C++ library with a declarative API, cross-platform support, rich layout and styling features, and minimal runtime overhead.


##### Library and Dependency Management

This project uses **Conan**, a C++ package manager, to simplify the installation and integration of third-party libraries. Conan ensures that the libraries used in the project are consistent across all platforms (Linux, macOS, and Windows) and eliminates the need to manually download, build, or configure dependencies.

---

#### 4. Game Flow

1. **Initialization**:

    - Backend starts the server and initializes the game state.
    - Frontend terminals connect to the backend and display initial game information
2. **Gameplay**:

    - Backend sends the current game state to frontends after each action.
    - Frontends display the current state to players and prompt the current player for their action.
    - Frontends send the player's action to the backend for processing.
3. **State Updates**:

    - Backend processes the player's action (e.g., drawing a card, creating a meld) and updates the game state.
    - Backend broadcasts the updated state to frontends.
4. **End of Game**:

    - When a team reaches the winning score or the game ends, the backend calculates the final scores and announces the winner.

---

#### 5. Advantages of This Architecture

1. **Cross-Platform Compatibility**:

    - All libraries used are portable and supported on Linux, macOS, and Windows.
2. **Scalable Design**:

    - The client-server architecture allows for easy future expansions, such as adding more players or a graphical user interface.
3. **Separation of Concerns**:

    - The backend handles all game logic and state management, while the frontends are lightweight and focused on user interaction.
4. **Debugging and Logging**:

    - `spdlog` ensures that developer can track errors and gameplay events, simplifying debugging.

# Nimbus Google Drive Client

A stable, fast **Google Drive client** built on top of
[`rclone`](https://rclone.org) as its sync/mount engine.

Unlike KDE's official `kio-gdrive` (which is a thin Drive-API browser over KIO,
with no offline files and no real sync), Nimbus runs a long-lived
`rclone rcd` (Remote Control) daemon under the hood and controls it through its
HTTP JSON-RPC API. This gives you:

- **Fast** — persistent VFS cache, connection pooling, and chunked reads survive
  across operations (no cold start per request).
- **Stable** — a structured, versioned RC API with a job/progress model instead
  of fragile stdout parsing; health-checked mounts with auto-restart.
- **Native KDE** — Qt6 + KDE Frameworks 6 + Kirigami, a Plasma system-tray icon
  (`KStatusNotifierItem`), single-instance via `KDBusService`.

## How it Works

Nimbus implements a highly robust and responsive synchronization and mounting architecture by combining `rclone`'s virtual filesystem with native Qt/C++ event-driven components:

### 1. Daemon Architecture
- **Persistent HTTP JSON-RPC Client**: On launch, Nimbus automatically starts a background `rclone rcd` (Remote Control Daemon). Instead of repeatedly spawning heavy subprocesses, Nimbus controls all mounts, remotes, and configurations via high-speed HTTP JSON-RPC requests, maintaining open connections and an active VFS cache.

### 2. Intelligent Bi-directional Synchronization (Two-way Sync)
Nimbus monitors and propagates file modifications in real-time between your local filesystem and Google Drive:
- **Remote to Local (Changes API Polling)**:
  - Rather than executing slow full-remote scans, Nimbus directly queries the official **Google Drive Changes API** using Qt's native HTTP stack (`QNetworkAccessManager`).
  - Active OAuth access tokens are securely read from rclone's decrypted configuration (`config/dump` RPC).
  - The client persists page tokens locally and polls the Changes API at a configurable interval (default: 60s). If changes are detected on the remote, it immediately triggers a targeted `rclone bisync` command to synchronize.
- **Local to Remote (Recursive Folder Watcher)**:
  - A custom `QFileSystemWatcher` wrapper recursively watches local sync folders. It dynamically registers new subdirectories and unregisters deleted ones in real-time.
  - A 3-second debounce timer prevents premature sync triggers while files are still being actively written or modified.

### 3. Queue & Lock Control
- **Sequence Queue**: To prevent concurrent file operations and conflicts, Nimbus queues incoming sync jobs and runs them sequentially.
- **Lock Management**: The application automatically tracks bisync state, detects stale lock files, and allows users to clear locks with a single click.

### 4. Virtual Filesystem (FUSE Mount)
- Remotes are mounted onto the local filesystem using FUSE. This exposes your Google Drive files inside standard file managers (like Dolphin or Nautilus) as if they were a local storage device, without consuming local hard drive space.

### 5. Interactive UI & Feedback
- **Real-time Status Bar**: Displays current backend status (e.g. syncing, checking changes, idle) and shows a loading indicator for active tasks.
- **Advanced Control**: Features a clean settings configuration page for sync times, ignore filters, custom rclone ports, and poll intervals.

---

## Requirements

### Runtime
- `rclone` (System binary; install via your distribution's package manager)
- FUSE support (`fuse3` and `/dev/fuse`)
- A Google Drive account

### Build
- CMake ≥ 3.24
- ECM (extra-cmake-modules) — KF6
- Qt6: `Core`, `Gui`, `Network`, `Qml`, `Quick`, `QuickControls2`
- KDE Frameworks 6: `kirigami`, `ki18n`, `kcoreaddons`, `kconfig`, `kdbusaddons`, `kstatusnotifieritem`

Example installation on Arch Linux:

```bash
sudo pacman -S base-devel cmake extra-cmake-modules \
  qt6-base qt6-declarative qt6-quickcontrols2 \
  kirigami ki18n kcoreaddons kconfig kdbusaddons kstatusnotifieritem \
  rclone fuse3
```

---

## Build & Run

### 1. Install Requirements

**Ubuntu / Debian:**
```bash
sudo apt install cmake g++ extra-cmake-modules qt6-base-dev qt6-declarative-dev \
  libkirigami-dev libkf6coreaddons-dev libkf6i18n-dev libkf6config-dev \
  libkf6dbusaddons-dev libkf6statusnotifieritem-dev rclone fuse3
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake extra-cmake-modules \
  qt6-base qt6-declarative qt6-quickcontrols2 \
  kirigami ki18n kcoreaddons kconfig kdbusaddons kstatusnotifieritem \
  rclone fuse3
```

### 2. Build

```bash
# From project root
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 3. Run

```bash
./build/src/nimbus-gdrive
```

Or system install:
```bash
sudo cmake --install build
nimbus-gdrive
```

---

## Screenshots

<div align="center">
  <h3>Main Window & Active Sync</h3>
  <img src=".github/screenshots/main_sync.png" width="650" alt="Main Window"/>

  <h3>Advanced Settings</h3>
  <img src=".github/screenshots/settings.png" width="650" alt="Settings Page"/>

  <h3>Cloud Remote Details</h3>
  <img src=".github/screenshots/details.png" width="650" alt="Details Page"/>
</div>

---

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

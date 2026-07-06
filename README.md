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

> Work in progress. The initial release is an **MVP**: a system-tray app that
> FUSE-mounts/unmounts Google Drive and walks the user through OAuth setup.
> Two-way sync and an in-app file browser are planned for later phases.

---

## پیش‌نیازها (Requirements)

### Runtime
- `rclone` (باینری سیستمی؛ نصب از طریق پکیج‌منیجر توزیع)
- پشتیبانی FUSE (`fuse3` و `/dev/fuse`)
- یک حساب Google Drive

### Build
- CMake ≥ 3.24
- ECM (extra-cmake-modules) — KF6
- Qt6: `Core`, `Gui`, `Network`, `Qml`, `Quick`, `QuickControls2`
- KDE Frameworks 6: `kirigami`, `ki18n`, `kcoreaddons`, `kconfig`,
  `kdbusaddons`, `kstatusnotifieritem`

نمونه نصب روی Arch:

```bash
sudo pacman -S base-devel cmake extra-cmake-modules \
  qt6-base qt6-declarative qt6-quickcontrols2 \
  kirigami ki18n kcoreaddons kconfig kdbusaddons kstatusnotifieritem \
  rclone fuse3
```

---

## Build & Run

### ۱. نصب پیش‌نیازها

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

### ۲. بیلد

```bash
# از ریشه پروژه
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### ۳. اجرا

```bash
./build/src/nimbus-gdrive
```

یا نصب سیستمی:
```bash
sudo cmake --install build
nimbus-gdrive
```

---

## تصاویر (Screenshots)

<div align="center">
  <h3>پنجره اصلی و همگام‌سازی فعال (Main Window & Active Sync)</h3>
  <img src=".github/screenshots/main_sync.png" width="650" alt="Main Window"/>

  <h3>تنظیمات پیشرفته (Advanced Settings)</h3>
  <img src=".github/screenshots/settings.png" width="650" alt="Settings Page"/>

  <h3>جزئیات ابری (Cloud Remote Details)</h3>
  <img src=".github/screenshots/details.png" width="650" alt="Details Page"/>
</div>

---

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

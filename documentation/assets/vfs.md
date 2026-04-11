# Virtual File System (VFS)

The Virtual File System (VFS) abstracts away platform-specific file paths and provides a unified way to locate and read files across all supported platforms. It manages a list of **search paths** and resolves filenames by scanning them in order.

**Related documentation:**
- [Asset Management](asset-managers.md) -- Asset managers, loading assets, and the asset lifecycle
- [Resource Management](../core-concepts/resource-management.md) -- Object lifecycles, IDs vs pointers, and safe access patterns

---

## 1. Why a Virtual File System?

Games and applications need to load files -- textures, models, sounds, configuration data -- but the location of those files varies depending on how the application is deployed:

- Running from a development checkout, assets live alongside the source.
- Running from an installed binary, assets may live in `/usr/share` or alongside the executable.
- On Android, assets are bundled inside the APK and can only be accessed through the asset manager with relative paths.
- On the Dreamcast, assets may be on the CD-ROM (`/cd`) or on a storage device (`/pc`).

The VFS solves this by maintaining an ordered list of directories to search. You provide a filename like `"models/hero.glb"` and the VFS finds the actual file for you.

---

## 2. How File Resolution Works

When you call any asset loading method -- `assets->load_mesh("models/hero.glb")`, `assets->load_texture("ui/button.png")`, etc. -- the engine resolves the filename through the VFS. Here is what happens:

1. **Placeholder expansion**: The filename is checked for `${RENDERER}` and `${PLATFORM}` placeholders. The VFS generates all combinations of replacements (with the actual renderer name, platform name, or empty string) and tries each one.
2. **Cache lookup**: The VFS checks an LRU (Least Recently Used) cache to see if this filename was resolved recently. If found, the cached result is returned immediately.
3. **Search path iteration**: If not cached, the VFS iterates through the search paths in order. For each search path, it joins the path with the filename and checks whether the file exists.
4. **Cache and return**: The first match is cached and returned. If no match is found, an `AssetMissingError` is thrown.

### The LRU Cache

The VFS caches both successful lookups (mapping a filename to its resolved path) and failed lookups (so it does not repeatedly search for files that do not exist). The cache uses an LRU eviction policy:

```cpp
// Check how many entries are cached
size_t cached = application->vfs->location_cache_size();

// Clear the cache manually
application->vfs->clear_location_cache();

// Adjust the maximum number of cached entries (default is reasonable)
application->vfs->set_location_cache_limit(256);
```

The cache is automatically cleared whenever search paths are added, inserted, or removed.

---

## 3. Default Search Paths

The VFS automatically populates its search path list during construction. The paths added depend on the target platform:

### All Platforms

| Path | Notes |
|------|-------|
| Working directory | Current working directory at startup |
| Executable directory | Directory containing the application binary (not available on Android release builds) |
| `assets/` subdirectory of each root path | `<any_root>/assets/` |
| `simulant/` subdirectory of each root path | `<any_root>/simulant/` |

### Linux

| Path | Notes |
|------|-------|
| `/usr/local/share` | System-wide shared data |
| `/usr/share` | System-wide shared data |

### Dreamcast

| Path | Notes |
|------|-------|
| `/cd` | CD-ROM root |
| `/pc` | Program/data directory |

### PSP

| Path | Notes |
|------|-------|
| `.` | Current directory |
| `umd0:` | UMD drive |
| `ms0:` | Memory Stick |
| `disc0:` | Disc drive |

### Android

On Android, file loading is restricted to relative paths only. The VFS uses `/android_asset/` as an internal placeholder prefix and strips it when resolving paths, so all asset loading on Android remains transparent to your code.

---

## 4. Managing Search Paths

You can add, insert, and remove search paths at runtime.

### Adding a Search Path

```cpp
// Append to the end of the search path list
application->vfs->add_search_path("/home/user/mygame/data");
application->vfs->add_search_path("mods/expansion_pack");
```

Paths are automatically converted to absolute paths and deduplicated. Adding a path that already exists is a no-op and returns `false`.

### Inserting at a Specific Position

```cpp
// Insert at the highest priority (index 0)
application->vfs->insert_search_path(0, "/priority/assets");

// Insert at a specific index
application->vfs->insert_search_path(2, "/some/other/path");

// If the index is greater than or equal to the list size, it is appended
application->vfs->insert_search_path(999, "/also/appended");
```

### Removing a Search Path

```cpp
application->vfs->remove_search_path("/old/path");
```

### Listing All Search Paths

```cpp
const std::list<smlt::Path>& paths = application->vfs->search_path();
for (const auto& path : paths) {
    S_INFO("Search path: {}", path.str());
}
```

---

## 5. Path Placeholders

The VFS supports two special placeholders in filenames that are expanded at runtime. This lets you organize assets by renderer or platform without writing platform-specific loading code.

| Placeholder | Replaced With |
|-------------|---------------|
| `${RENDERER}` | The current renderer name (e.g., `gl2x`), then empty string |
| `${PLATFORM}` | The current platform name (e.g., `linux`, `psp`, `dreamcast`), then empty string |

The engine tries all combinations of both placeholders. For example, the filename `"models/${PLATFORM}/hero.obj"` will be tried as:

1. `models/linux/hero.obj` (platform filled, renderer filled with actual name)
2. `models/hero.obj` (platform filled, renderer empty)
3. `models/linux/hero.obj` (platform filled with renderer name, renderer filled)
4. `models/hero.obj` (both empty)

The first match found is used. This means you can organize assets in directories that match the placeholders:

```
assets/
  models/
    linux/
      hero.glb
    psp/
      hero.glb
    hero.glb              # Fallback for any platform
  textures/
    gl2x/
      default.smat        # Renderer-specific material
    texture.png           # Fallback
```

### Using Placeholders in Code

```cpp
// Load a platform-specific model (falls back to generic if not found)
MeshPtr mesh = assets->load_mesh("models/${PLATFORM}/hero.glb");

// Load a renderer-specific material
MaterialPtr mat = assets->load_material("materials/${RENDERER}/custom.smat");
```

---

## 6. Reading Files Directly

While most asset loading goes through the `AssetManager`, you can also use the VFS directly to read arbitrary files.

### Locating a File

```cpp
// Returns the resolved absolute path, or empty optional if not found
auto resolved = application->vfs->locate_file("config.json");
if (resolved) {
    S_INFO("Found at: {}", resolved.value());
} else {
    S_WARN("File not found!");
}

// Locate with cache disabled (forces a fresh filesystem check)
auto fresh = application->vfs->locate_file("config.json", false);

// Locate without logging a warning on failure
auto silent = application->vfs->locate_file("optional.json", true, true);
```

### Reading File Contents

```cpp
// Read the entire file into a stringstream
auto stream = application->vfs->read_file("config.json");
if (stream) {
    std::string content = stream->str();
    // Parse JSON, XML, custom format, etc.
}

// Read file line by line
auto lines = application->vfs->read_file_lines("dialogue.txt");
for (const auto& line : lines) {
    S_INFO("Line: {}", line);
}

// Open a raw file stream for binary data
auto ifstream = application->vfs->open_file("data/level.dat");
if (ifstream) {
    // Read raw bytes
    std::vector<uint8_t> buffer(
        (std::istreambuf_iterator<char>(*ifstream)),
        std::istreambuf_iterator<char>()
    );
}
```

---

## 7. Read Blocking

On platforms that read from optical media (such as the Dreamcast CD-ROM), a file read can interrupt CD audio playback. The VFS provides a read-blocking toggle to prevent this:

```cpp
// Enable read blocking -- all file reads will fail and log an error
application->vfs->enable_read_blocking();

// Disable read blocking
application->vfs->disable_read_blocking();

// Check current state
if (application->vfs->read_blocking_enabled()) {
    S_WARN("File reads are currently blocked");
}
```

This is useful during music playback sequences where you want to guarantee that no filesystem access interrupts the CD audio.

---

## 8. VFS and Loader Integration

Several loaders temporarily modify the search paths to resolve relative references within files. For example:

- **OBJ loader**: Adds the directory containing the `.obj` file so that referenced `.mtl` material files and textures can be found.
- **MS3D loader**: Adds the model's directory so texture references resolve correctly.
- **Material script loader**: Adds the material file's directory so that shader files referenced in the script can be located.

These loaders automatically clean up after themselves by removing the temporary search path once loading is complete. You do not need to manage this yourself.

---

## 9. VFS API Reference

| Method | Description |
|--------|-------------|
| `add_search_path(path)` | Append a search path. Returns `false` if path is empty or duplicate. |
| `insert_search_path(index, path)` | Insert a search path at the given index. |
| `remove_search_path(path)` | Remove a search path. |
| `search_path()` | Return the full list of search paths. |
| `search_path_size()` | Return the number of search paths. |
| `locate_file(filename, use_cache, fail_silently)` | Resolve a filename to an absolute path. |
| `read_file(filename)` | Read a file into a `stringstream`. |
| `read_file_lines(filename)` | Read a file as a `vector<string>`, one entry per line. |
| `open_file(filename)` | Open a raw `istream` to the file. |
| `location_cache_size()` | Return the number of cached lookups. |
| `clear_location_cache()` | Purge all cached lookups. |
| `set_location_cache_limit(entries)` | Set the maximum cache size. |
| `enable_read_blocking()` | Cause all file reads to fail with an error. |
| `disable_read_blocking()` | Allow file reads normally. |
| `read_blocking_enabled()` | Check whether read blocking is active. |

---

## 10. Best Practices

### 1. Use relative paths with the VFS

Never use absolute paths in asset loading calls. The VFS is designed to resolve relative paths through its search path list:

```cpp
// GOOD -- resolved through VFS search paths
auto mesh = assets->load_mesh("models/character.glb");

// BAD -- bypasses the VFS, will not work on all platforms
auto mesh2 = assets->load_mesh("/home/user/game/models/character.glb");
```

### 2. Organize assets by platform when needed

If your game targets multiple platforms with different asset requirements (different texture compression formats, different mesh formats), use the `${PLATFORM}` placeholder:

```cpp
auto texture = assets->load_texture("textures/${PLATFORM}/ground.tex");
```

### 3. Use the `${RENDERER}` placeholder for renderer-specific materials

Different renderers may require different shader code or material configurations:

```cpp
auto material = assets->load_material("materials/${RENDERER}/pbr.smat");
```

### 4. Clear the location cache when search paths change at runtime

If you add or remove search paths during gameplay (e.g., mounting a mod or DLC), the cache is cleared automatically. But if you manually manipulate files on disk and expect the VFS to pick up changes, clear the cache:

```cpp
// After adding a mod directory
application->vfs->add_search_path("/mods/my_mod");
// Cache is automatically cleared, new files are discoverable
```

### 5. Handle missing files gracefully

Use `locate_file` with `fail_silently=true` for optional files:

```cpp
auto config = application->vfs->locate_file("optional_mod_config.json", true, true);
if (config) {
    // Load and apply the config
} else {
    // Use defaults
}
```

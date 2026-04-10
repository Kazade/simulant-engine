# Asset Management and Virtual File System

This document provides a comprehensive guide to loading, managing, and organizing assets in the Simulant game engine. It covers the asset manager hierarchy, the Virtual File System (VFS), asset lifecycles, garbage collection, and best practices for production-ready code.

**Related documentation:**
- [Resource Management](../core-concepts/resource-management.md) -- Object lifecycles, IDs vs pointers, and safe access patterns
- [Virtual File System](vfs.md) -- Deep dive into VFS internals
- [Mesh Formats](mesh-formats.md) -- Supported mesh file formats and loaders

---

## 1. Asset Manager Hierarchy

Simulant uses a two-tier asset manager architecture. Every asset -- meshes, textures, materials, sounds, fonts, particle scripts, binaries, and prefabs -- is owned by an `AssetManager`. There are two concrete implementations:

| Manager | Class | Lifetime | Use Case |
|---------|-------|----------|----------|
| **SharedAssetManager** | `SharedAssetManager` | Lives for the entire application lifetime | Global assets shared across all scenes (UI fonts, common sounds, default materials) |
| **LocalAssetManager** | `LocalAssetManager` | Lives as long as its parent Scene | Scene-specific assets (level meshes, level-specific textures, background music) |

### The Parent-Child Relationship

`AssetManager` supports a parent-child hierarchy. Every `LocalAssetManager` has the `SharedAssetManager` as its parent. This creates a tree structure:

```
SharedAssetManager (application->shared_assets)
  |
  +-- LocalAssetManager (scene_a->assets)
  +-- LocalAssetManager (scene_b->assets)
  +-- LocalAssetManager (scene_c->assets)
```

When you request an asset by ID from a child manager, the lookup falls through to the parent if the ID is not found locally. This means assets loaded through `shared_assets` are accessible from any scene's asset manager:

```cpp
// Load a texture through the shared manager
auto shared_tex = application->shared_assets->load_texture("ui/button.png");

// In any scene, you can retrieve it by ID
TexturePtr tex = scene->assets->texture(shared_tex->id());  // Falls through to parent
```

### Accessing Asset Managers

```cpp
// Shared asset manager (global, persists across scenes)
auto shared = application->shared_assets;   // Via Application
auto shared2 = window->shared_assets;        // Via Window (same object)

// Scene-local asset manager (tied to scene lifecycle)
auto local = scene->assets;                  // Via Scene
```

The `Scene` class creates its `AssetManager` with the `SharedAssetManager` as its parent. This is done in the Scene constructor:

```cpp
// From scene.cpp
assets_(std::make_unique<AssetManager>(window->app->shared_assets.get()))
```

### Base Manager Traversal

Any `AssetManager` can traverse its ancestry to find the root (shared) manager:

```cpp
AssetManager* root = scene->assets->base_manager();  // Returns the SharedAssetManager
bool is_base = scene->assets->is_base_manager();     // false for LocalAssetManager
```

The `SharedAssetManager` also initializes built-in defaults on startup: a default material, default fonts, and solid-color placeholder textures (white, black, and a blue-ish normal map texture).

---

## 2. Loading Different Asset Types

Each asset type has a consistent API pattern: `load_<type>()` for file-based loading, `create_<type>()` for programmatic creation, `find_<type>()` for name-based lookup, and `<type>(id)` for ID-based retrieval.

### Meshes

Meshes are loaded from model files or created programmatically. Supported formats include OBJ, glTF 2.0, MS3D, MD2, BSP, OPT, and TMX. See [Mesh Formats](mesh-formats.md) for details.

```cpp
// Load from file
MeshPtr mesh = assets->load_mesh("models/hero.glb");

// Load with custom vertex specification and options
smlt::VertexSpecification spec = smlt::VertexSpecification::POSITION_AND_NORMAL_ONLY;
smlt::MeshLoadOptions options;
options.cull_mode = smlt::CULL_MODE_BACK;
MeshPtr mesh2 = assets->load_mesh("models/cube.obj", spec, options);

// Create programmatically
MeshPtr procedural = assets->create_mesh(smlt::VertexSpecification::DEFAULT);
procedural->create_submesh_as_box("box", material, 2.0f, 2.0f, 2.0f);

// Create a heightmap from an image
smlt::HeightmapSpecification heightmap_spec;
heightmap_spec.vertex_spec = smlt::VertexSpecification::DEFAULT;
MeshPtr terrain = assets->create_mesh_from_heightmap("textures/heightmap.png", heightmap_spec);

// Create from an existing texture (heightmap data stored in texture)
MeshPtr terrain2 = assets->create_mesh_from_heightmap(heightmap_texture, heightmap_spec);

// Create a cube with one submesh per face (useful for skyboxes)
MeshPtr cube = assets->create_mesh_as_cube_with_submesh_per_face(10.0f);

// Extract a single submesh into its own mesh
SubMesh* door_submesh = building_mesh->submesh("door");
MeshPtr door_only = assets->create_mesh_from_submesh(door_submesh);
```

### Textures

Textures are loaded from image files (PNG, DTEX, DDS, KMG, WAL, PCX, and more) or created with raw pixel data.

```cpp
// Load from file
TexturePtr tex = assets->load_texture("textures/brick.png");

// Load with custom flags
smlt::TextureFlags flags;
flags.mipmap = true;
flags.flip_vertically = false;
flags.auto_upload = true;
flags.wrap = smlt::TEXTURE_WRAP_REPEAT;
flags.filter = smlt::TEXTURE_FILTER_LINEAR;
TexturePtr tex2 = assets->load_texture("textures/brick.png", flags);

// Create programmatically
TexturePtr render_target = assets->create_texture(512, 512, smlt::TEXTURE_FORMAT_RGBA_4UB_8888);

// Set pixel data
std::vector<uint8_t> pixels(512 * 512 * 4, 255);  // Solid white RGBA
render_target->set_data(pixels);
```

### Materials

Materials define how meshes are rendered. They can be loaded from material script files or created programmatically.

```cpp
// Load from material script file
MaterialPtr mat = assets->load_material("materials/hero.material");

// Create programmatically
MaterialPtr mat2 = assets->create_material();
mat2->set_base_color(smlt::Color(0.8f, 0.2f, 0.2f, 1.0f));
mat2->set_shininess(0.5f);

// Create from a texture (convenience method)
MaterialPtr textured_mat = assets->create_material_from_texture(brick_texture);

// Clone an existing material
MaterialPtr clone = assets->clone_material(mat->id());
clone->set_base_color(smlt::Color(0.2f, 0.8f, 0.2f, 1.0f));

// Clone the default material
MaterialPtr default_clone = assets->clone_default_material();
```

### Sounds

Sounds are loaded from audio files (OGG, WAV). You can control whether audio is streamed or loaded entirely into memory.

```cpp
// Load sound (streams audio by default)
SoundPtr bgm = assets->load_sound("music/forest.ogg");

// Load sound into memory (disable streaming)
smlt::SoundFlags sound_flags;
sound_flags.stream_audio = false;
SoundPtr short_sfx = assets->load_sound("sounds/click.wav", sound_flags);
```

### Fonts

Fonts can be loaded from TTF/FNT files, created from system font families, or loaded from raw font data in memory.

```cpp
// Load from file
smlt::FontFlags font_flags;
font_flags.size = 24;
font_flags.weight = smlt::FONT_WEIGHT_BOLD;
font_flags.style = smlt::FONT_STYLE_ITALIC;
font_flags.charset = smlt::CHARACTER_SET_LATIN;
FontPtr font = assets->load_font("fonts/Orbitron-BoldItalic.ttf", font_flags);

// Create from system font family (searches for matching files)
FontPtr system_font = assets->create_font_from_family("Kanit", font_flags);
// This searches for files like: Kanit-BoldItalic.ttf, Kanit-BoldItalic-24.fnt
// In paths like: $path/Kanit-BoldItalic.ttf, $path/fonts/Kanit/Kanit-BoldItalic.ttf

// Create from raw font data in memory
FontPtr mem_font = assets->create_font_from_memory(font_bytes, font_size, font_flags);
```

### Particle Scripts

Particle scripts define visual particle effects using a script-like syntax.

```cpp
// Load from file
ParticleScriptPtr fire = assets->load_particle_script("particles/fire.script");

// Load a built-in
ParticleScriptPtr builtin_fire = assets->load_particle_script(
    smlt::ParticleScript::BuiltIns::FIRE
);
```

### Binaries

Binary assets let you load raw file data into memory. Useful for custom file formats, configuration data, or shaders.

```cpp
// Load raw binary data
BinaryPtr data = assets->load_binary("data/config.bin");

// Access the raw bytes
const std::vector<uint8_t>& bytes = data->data();
```

### Prefabs

Prefabs are scene graphs that can be instantiated. They are commonly loaded from glTF files.

```cpp
// Load from file (e.g., glTF)
PrefabPtr prefab = assets->load_prefab("models/character.glb");

// Create programmatically from an existing node hierarchy
PrefabPtr saved = assets->create_prefab(root_node);

// Instantiate a prefab into a scene
auto node = prefab->instantiate(scene->stage());
```

---

## 3. Asset Paths and Search Paths

Simulant does not use absolute file paths for asset loading. Instead, the Virtual File System (VFS) maintains a list of **search paths** and resolves filenames by searching through them in order.

### How the VFS Resolves a File

When you call `assets->load_mesh("models/hero.obj")`, the engine:

1. Checks an LRU cache to see if this filename was resolved before
2. If not cached, iterates through the search paths in order
3. For each search path, joins it with the filename and checks if the file exists
4. Returns the first match and caches the result
5. If no match is found, throws an `AssetMissingError`

### Default Search Paths

The VFS automatically adds these search paths on initialization:

| Path | Platform | Notes |
|------|----------|-------|
| Working directory | All | Current working directory |
| Executable directory | All (except Android) | Directory containing the binary |
| `assets/` subdirectory | All | `<any_root>/assets/` |
| `simulant/` subdirectory | All | `<any_root>/simulant/` |
| `/usr/local/share` | Linux | System-wide shared data |
| `/usr/share` | Linux | System-wide shared data |
| `/cd` and `/pc` | Dreamcast | CD-ROM and program paths |
| `.`, `umd0:`, `ms0:`, `disc0:` | PSP | Various PSP storage locations |
| Relative paths | Android | Android assets are always relative |

### Adding Custom Search Paths

```cpp
// Add a search path (appended to the end)
application->vfs->add_search_path("/home/user/mygame/data");

// Insert at a specific index (0 = highest priority)
application->vfs->insert_search_path(0, "/priority/path");

// Remove a search path
application->vfs->remove_search_path("/old/path");
```

### Path Placeholders

The VFS supports two special placeholders that are expanded at runtime:

| Placeholder | Replaced With |
|-------------|---------------|
| `${RENDERER}` | The current renderer name (e.g., `gl2x`), then empty string |
| `${PLATFORM}` | The current platform name (e.g., `linux`, `psp`, `dreamcast`), then empty string |

The engine tries all combinations, allowing you to organize assets by platform or renderer:

```cpp
// This will try:
//   models/psp/hero.obj
//   models/hero.obj
//   models/psp/hero.obj (with renderer placeholder)
//   models/hero.obj (with both placeholders removed)
MeshPtr mesh = assets->load_mesh("models/${PLATFORM}/hero.obj");
```

### VFS File Operations

You can use the VFS directly to read files:

```cpp
// Locate a file (returns the resolved path)
auto resolved = application->vfs->locate_file("config.json");
if (resolved) {
    S_INFO("Found at: {}", resolved.value());
}

// Read file contents as a string stream
auto stream = application->vfs->read_file("config.json");
if (stream) {
    std::string content = stream->str();
}

// Read file line by line
auto lines = application->vfs->read_file_lines("config.json");
for (const auto& line : lines) {
    S_INFO("Line: {}", line);
}

// Open a raw file stream
auto ifstream = application->vfs->open_file("data.bin");
```

### VFS Location Cache

The VFS caches file lookups to avoid repeated filesystem scans. The cache uses an LRU (Least Recently Used) eviction policy.

```cpp
// Check cache size
size_t cached = application->vfs->location_cache_size();

// Clear the cache (also happens automatically when search paths change)
application->vfs->clear_location_cache();

// Set the maximum number of cached entries
application->vfs->set_location_cache_limit(128);
```

### Read Blocking

For platforms that read from optical media (like the Dreamcast CD), a file read can interrupt CD audio playback. The VFS provides a read-blocking toggle:

```cpp
// Enable read blocking (all file reads will fail and log an error)
application->vfs->enable_read_blocking();

// Disable read blocking
application->vfs->disable_read_blocking();
```

---

## 4. Asset Lifecycle and Garbage Collection

All assets in Simulant are reference-counted using `std::shared_ptr`. The garbage collector automatically frees assets that are no longer in use.

### Reference Counting

When you load or create an asset, the asset manager stores an internal `shared_ptr`. Your code also receives a `shared_ptr`. The asset is eligible for garbage collection when its reference count drops to **1** (only the manager's internal reference remains).

```cpp
{
    auto mesh = assets->load_mesh("hero.glb");
    // ref count: 2 (you + manager)

    // Attach to an Actor -- the Actor now holds another reference
    auto actor = stage->create_child<Actor>();
    actor->set_mesh(mesh, smlt::DETAIL_LEVEL_NEAREST);
    // ref count: 3 (you + manager + actor)

} // mesh goes out of scope
  // ref count: 2 (manager + actor)

// Actor is destroyed, releasing its reference
actor->destroy();
// ref count: 1 (manager only)

// Garbage collection will now free the mesh
assets->run_garbage_collection();
```

### Garbage Collection Methods

Each asset has an individual garbage collection method:

| Method | Behavior |
|--------|----------|
| `GARBAGE_COLLECT_PERIODIC` (default) | Asset is deleted when ref count drops to 1 and GC runs |
| `GARBAGE_COLLECT_NEVER` | Asset persists until you explicitly change the method or destroy it |

```cpp
// Load with periodic GC (default)
auto temp_tex = assets->load_texture("temp.png");

// Load with never-collect GC
auto persistent_mesh = assets->load_mesh(
    "hero.glb",
    smlt::VertexSpecification::DEFAULT,
    smlt::MeshLoadOptions(),
    GARBAGE_COLLECT_NEVER
);

// Change GC method on an existing asset
persistent_mesh->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);

// Explicitly destroy an asset
assets->destroy_mesh(persistent_mesh->id());
assets->run_garbage_collection();  // Actually frees the memory
```

### When Garbage Collection Runs

The engine automatically calls `run_garbage_collection()` during its update cycle. You can also trigger it manually:

```cpp
// Manual GC trigger (useful in tests or after bulk asset cleanup)
scene->assets->run_garbage_collection();

// The Scene also triggers GC on unload
```

### Destroy Methods

Each asset type has a corresponding destroy method. These mark the asset for periodic collection and remove it from the manager's registry:

```cpp
assets->destroy_mesh(mesh_id);
assets->destroy_texture(tex_id);
assets->destroy_material(mat_id);
assets->destroy_sound(sound_id);
assets->destroy_font(font_id);
assets->destroy_particle_script(script_id);
assets->destroy_prefab(prefab_id);
assets->destroy_binary(bin_id);

// Destroy all assets at once
assets->destroy_all();
```

### Asset Inter-References

Assets can reference each other. A `Mesh` references a `Material`, which references `Texture`s. These internal references **count toward the reference count**, preventing premature garbage collection:

```cpp
auto texture = assets->load_texture("brick.png");
auto material = assets->create_material();
material->set_base_color_map(texture);  // Material now references texture

texture.reset();  // Your reference is gone
// Texture ref count is still > 1 because material references it
// Texture will NOT be garbage collected

material.reset();  // Now texture ref count drops to 1
// Texture IS now eligible for garbage collection
```

---

## 5. Parent-Child Asset Manager Relationships

The parent-child relationship between asset managers enables powerful asset-sharing patterns.

### How Lookups Fall Through

When you call `scene->assets->mesh(some_id)`, the manager:

1. Checks its own `MeshManager` for the ID
2. If not found and a parent exists, delegates to `parent_->mesh(some_id)`
3. Continues up the chain until it finds the asset or reaches the root

```cpp
// Load through shared_assets
auto shared_tex = application->shared_assets->load_texture("ui/icon.png");
TextureID shared_id = shared_tex->id();

// In a scene, retrieve it through the local manager
TexturePtr tex = scene->assets->texture(shared_id);  // Falls through to shared_assets
assert(tex == shared_tex);  // Same object
```

### Child Manager Registration

When a `LocalAssetManager` is created with a parent, it automatically registers itself as a child:

```cpp
// From asset_manager.cpp
AssetManager::AssetManager(AssetManager* parent) : parent_(parent) {
    if (parent_) {
        base_manager()->register_child(this);
    }
}
```

The parent tracks all children and propagates operations like garbage collection:

```cpp
void AssetManager::run_garbage_collection() {
    // First, recurse into all children
    for (auto child : children_) {
        child->run_garbage_collection();
    }

    // Then, update this manager's internal object managers
    mesh_manager_.update();
    material_manager_.update();
    texture_manager_.update();
    // ... etc
}
```

### Querying Child Managers

```cpp
// Check how many children a manager has
size_t count = application->shared_assets->child_manager_count();

// Iterate over child managers
for (size_t i = 0; i < shared_assets->child_manager_count(); ++i) {
    const AssetManager* child = shared_assets->child_manager(i);
    S_INFO("Child manager has {} meshes", child->mesh_count());
}
```

### Destroying a Child Manager

When a `LocalAssetManager` is destroyed (e.g., when a scene is unloaded), it unregisters itself from the parent. If a parent is destroyed while children still exist, the children are orphaned (their parent pointer is set to null) and a warning is logged:

```cpp
// From asset_manager.cpp
if (!children_.empty()) {
    S_WARN("Destroyed base manager while children remain");
    for (auto& child : children_) {
        child->parent_ = nullptr;
    }
}
```

---

## 6. scene->assets vs window->shared_assets

Choosing the right asset manager is one of the most common decisions you will make. Here is a practical guide.

### scene->assets (LocalAssetManager)

**Use for:**
- Level-specific meshes and textures
- Scene-specific music and ambient sounds
- Materials used only within one scene
- Temporary procedural assets
- Assets that should be cleaned up when the scene changes

**Lifecycle:** Created when the scene loads, destroyed when the scene unloads. All assets are automatically released when the scene is destroyed.

```cpp
class ForestScene : public smlt::Scene {
protected:
    void on_load() override {
        // These assets belong to ForestScene only
        forest_mesh_ = assets->load_mesh("levels/forest.glb");
        ambient_sound_ = assets->load_sound("sounds/forest_ambience.ogg");
        ground_tex_ = assets->load_texture("textures/forest_ground.png");
    }

    void on_unload() override {
        // All assets are released automatically
        // No manual cleanup needed
    }

private:
    smlt::MeshID forest_mesh_;
    smlt::SoundID ambient_sound_;
    smlt::TextureID ground_tex_;
};
```

### window->shared_assets (SharedAssetManager)

**Use for:**
- UI fonts and textures
- Common sound effects (clicks, UI feedback, button sounds)
- Default/shared materials
- Assets needed by multiple scenes
- Assets loaded during application initialization

**Lifecycle:** Lives for the entire application lifetime. Never automatically released.

```cpp
class MyGame : public smlt::Application {
    bool init() override {
        // Load shared assets once during startup
        ui_font_ = shared_assets->load_font("fonts/Orbitron.ttf");
        click_sound_ = shared_assets->load_sound("sounds/click.ogg");
        default_mat_ = shared_assets->create_material();

        return true;
    }

private:
    smlt::FontID ui_font_;
    smlt::SoundID click_sound_;
    smlt::MaterialID default_mat_;
};
```

### Practical Comparison

```cpp
// BAD: Loading a UI texture through scene assets
// If the scene changes, this texture is gone but other scenes might still need it
void MenuScene::on_load() {
    auto button_tex = assets->load_texture("ui/button.png");
    button_tex_id_ = button_tex->id();
}

// GOOD: Loading shared UI assets through shared_assets
// Available to all scenes and never automatically released
void MyGame::init() {
    auto button_tex = shared_assets->load_texture("ui/button.png");
    button_tex_id_ = button_tex->id();
}
```

### Accessing Shared Assets from a Scene

From within a scene, you can access the shared assets through the window or application:

```cpp
void GameScene::on_load() {
    // Via window
    auto shared_tex = window->shared_assets->load_texture("ui/hud.png");

    // Via application
    auto shared_font = app->shared_assets->create_font_from_family("Kanit", flags);
}
```

### Font Loading: A Special Case

The UI widget system demonstrates the parent-child pattern in action. When a widget needs a font, it first checks the scene-local manager, then falls through to shared:

```cpp
// From widget.cpp
FontPtr fnt = scene->assets->find_font(alias);
if (!fnt && shared_assets) {
    fnt = shared_assets->find_font(alias);
}
```

This means you can load fonts through either manager and widgets will find them automatically.

---

## 7. Creating Assets Programmatically vs Loading from Files

Simulant supports both loading assets from files and creating them in code. Both approaches produce the same asset objects -- the difference is only in how the data is populated.

### Loading from Files

Loading from files uses the **loader system**. Each file format has a corresponding `Loader` and `LoaderType`:

```cpp
// The loader system works through file extensions
auto mesh = assets->load_mesh("model.obj");    // Uses OBJLoader
auto tex  = assets->load_texture("image.png");  // Uses PNGLoader
auto snd  = assets->load_sound("audio.ogg");    // Uses OGGLoader
```

Available loaders include:

| Asset Type | Supported Formats | Loader Class |
|------------|-------------------|--------------|
| Meshes | OBJ, glTF, MS3D, MD2, BSP, OPT, TMX | `OBJLoader`, `GLTFLoader`, `MS3DLoader`, `MD2Loader`, etc. |
| Textures | PNG, DTEX, DDS, KMG, WAL, PCX | `PNGLoader`, `DTEXLoader`, `DDSTextureLoader`, etc. |
| Sounds | OGG, WAV | `OGGLoader`, `WAVLoader` |
| Fonts | TTF, FNT | `TTFLoader`, `FNTLoader` |
| Materials | `.material` scripts | `MaterialScriptLoader` |
| Particle Scripts | `.script` files | `ParticleScriptLoader` |
| Prefabs | glTF | `GLTFLoader` |

The engine selects the appropriate loader based on the file extension. If a file extension is ambiguous (e.g., a TGA file could be a texture or a heightmap), you can use **loader hints**:

```cpp
// When the engine can't't determine the loader automatically
LoaderType* loader = app->loader_for(filename, LOADER_HINT_TEXTURE);
```

### Creating Programmatically

Programmatic creation bypasses the loader system entirely. You construct the asset data in code:

```cpp
// Create an empty mesh with a specific vertex format
MeshPtr mesh = assets->create_mesh(smlt::VertexSpecification::POSITION_AND_NORMAL_AND_UV);

// Add geometry
SubMesh* sub = mesh->create_submesh("quad", material, smlt::INDEX_TYPE_16_BIT, smlt::MESH_ARRANGEMENT_TRIANGLES);
// ... populate vertex data ...

// Create an empty texture
TexturePtr tex = assets->create_texture(256, 256, smlt::TEXTURE_FORMAT_RGBA_4UB_8888);

// Set pixel data
std::vector<uint8_t> pixels(256 * 256 * 4);
// ... fill with pixel data ...
tex->set_data(pixels);

// Create an empty material
MaterialPtr mat = assets->create_material();
mat->set_base_color(smlt::Color(1, 0, 0, 1));
mat->set_shininess(0.8f);
```

### When to Use Each Approach

| Scenario | Approach |
|----------|----------|
| Art-produced models and textures | Load from files |
| Procedural terrain, effects, UI elements | Create programmatically |
| Runtime-generated geometry (meshes for debug drawing) | Create programmatically |
| Prototyping without art assets | Create programmatically (use `create_mesh_as_cube_with_submesh_per_face`, etc.) |
| Final game content | Load from files |
| Dynamic texture rendering (render targets) | Create programmatically |

### Built-In Asset Paths

Some asset types have built-in convenience paths defined as constants:

```cpp
// Material built-ins
Material::BuiltIns::DEFAULT        // Default material
Material::BuiltIns::TEXTURE_ONLY   // Material with just a texture
Material::BuiltIns::DIFFUSE_ONLY   // Diffuse-only material

// Particle script built-ins
ParticleScript::BuiltIns::FIRE     // Fire particle effect
```

---

## 8. Asset Embedding for Target Platforms

When targeting constrained platforms (Dreamcast, PSP, Android), you may need to embed assets directly into the executable. This eliminates runtime file I/O and reduces the number of files to ship.

### Manual Embedding with bin2c

The standard approach is to convert binary files into C header arrays using tools like `bin2c` or `xxd`:

```bash
# Convert a font file to a C header
xxd -i fonts/Orbitron.ttf > fonts/orbitron_ttf.h

# Or use bin2c
bin2c fonts/Orbitron.ttf > fonts/orbitron_ttf.h
```

This produces a header file like:

```c
// orbitron_ttf.h
unsigned char Orbitron_ttf[] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, ...
};
unsigned int Orbitron_ttf_len = 45678;
```

You then include this header and load the font from memory:

```cpp
#include "fonts/orbitron_ttf.h"

// Load font from embedded data
smlt::FontFlags flags;
flags.size = 24;
FontPtr font = assets->create_font_from_memory(Orbitron_ttf, Orbitron_ttf_len, flags);
```

### Embedding Binary Data

The same approach works for any binary data:

```cpp
#include "data/level_config.h"

BinaryPtr config = assets->create_binary_from_memory(
    level_config_data,
    level_config_data_len
);
```

> **Note:** Currently, Simulant's `AssetManager` provides `load_binary()` for file-based loading but does not have a dedicated `create_binary_from_memory()` method. You can work around this by writing custom code to populate a `Binary` asset from memory, or by using the VFS which handles platform-specific asset access (particularly on Android, where assets are accessed through the APK's asset manager).

### Android Asset Considerations

On Android, assets are bundled in the APK and accessed through the Android Asset Manager. The VFS handles this transparently using the `/android_asset/` placeholder prefix:

```cpp
// On Android, this resolves to a relative path within the APK assets/ folder
auto tex = assets->load_texture("textures/hero.png");
```

### Platform-Specific Asset Organization

Use the VFS path placeholders to organize platform-specific assets:

```
assets/
  ${PLATFORM}/
    hero.obj          # Platform-specific mesh
    ui/
      button.png      # Platform-specific UI
```

The VFS will try the platform-specific path first, then fall back to the generic path.

---

## 9. Virtual File System (VFS)

The Virtual File System abstracts away the underlying filesystem, providing a unified interface for locating and reading files across all supported platforms.

### Architecture

The VFS (`VirtualFileSystem` class) maintains:

1. **Search Path List** -- An ordered list of directories to search (`resource_path_`)
2. **Location Cache** -- An LRU cache mapping filenames to resolved paths (`location_cache_`)

```
VirtualFileSystem
  +-- resource_path_ (std::list<Path>)
  |     +-- "/home/user/project"
  |     +-- "/home/user/project/assets"
  |     +-- "/usr/share"
  |
  +-- location_cache_ (LRUCache<Path, Path>)
        +-- "textures/hero.png" -> "/home/user/project/assets/textures/hero.png"
```

### File Resolution Algorithm

The `locate_file()` method performs the following steps:

1. If read blocking is enabled, fail immediately
2. Generate candidate filenames by expanding `${RENDERER}` and `${PLATFORM}` placeholders
3. For each candidate:
   a. Check the LRU cache first
   b. If not cached, check if the absolute path exists
   c. If not found, iterate through search paths and check `<search_path>/<candidate>`
   d. Cache the result (or a "not found" marker)
4. Return the resolved path or empty optional

### Performance Considerations

- **First lookup**: Searches all paths (slow)
- **Subsequent lookups**: Hit the cache (fast)
- **Cache invalidation**: Happens automatically when search paths change

```cpp
// First call: searches all paths
auto path1 = vfs->locate_file("hero.png");  // ~1-5ms depending on disk

// Second call: cache hit
auto path2 = vfs->locate_file("hero.png");  // ~0.001ms

// Adding a search path invalidates the cache
vfs->add_search_path("/new/path");  // Cache cleared
auto path3 = vfs->locate_file("hero.png");  // Searches again
```

### Error Handling

If a file cannot be found, `locate_file()` returns an empty `optional<Path>`:

```cpp
auto result = vfs->locate_file("nonexistent.png");
if (!result) {
    S_WARN("File not found!");
}

// Or use fail_silently to suppress the warning
auto result2 = vfs->locate_file("nonexistent.png", true, true);
```

For `open_file()` and `read_file()`, a null pointer is returned:

```cpp
auto stream = vfs->read_file("nonexistent.png");
if (!stream) {
    S_ERROR("Failed to read file");
}
```

For more details, see [Virtual File System](vfs.md).

---

## 10. Custom Asset Types

Simulant's asset system is extensible. You can add support for new file formats by implementing a `Loader` and `LoaderType`.

### The Loader Interface

Every loader consists of two classes:

1. **LoaderType** -- Identifies which files this loader handles (by extension)
2. **Loader** -- Parses file data and populates an asset

```cpp
// CustomLoaderType.h
class CustomLoaderType : public smlt::LoaderType {
public:
    const char* name() override {
        return "custom_loader";
    }

    bool supports(const smlt::Path& filename) const override {
        return filename.str().ends_with(".custom");
    }

    smlt::Loader::ptr loader_for(
        const smlt::Path& filename,
        std::shared_ptr<std::istream> data
    ) const override {
        return std::make_shared<CustomLoader>(filename, data);
    }
};

// CustomLoader.h
class CustomLoader : public smlt::Loader {
public:
    CustomLoader(const smlt::Path& filename, std::shared_ptr<std::istream> data)
        : Loader(filename, data) {}

private:
    bool into(smlt::Loadable& resource,
              const smlt::LoaderOptions& options = smlt::LoaderOptions()) override {
        // Cast the target resource to the expected type
        MyCustomAsset* asset = loadable_to<MyCustomAsset>(resource);
        if (!asset) return false;

        // Parse data_ stream and populate asset
        // data_ is a std::istream containing the file contents

        return true;  // Return false on failure
    }
};
```

### The Loadable Interface

Any class that can receive loaded data must inherit from `Loadable`:

```cpp
class MyCustomAsset : public smlt::Loadable, public smlt::Asset {
public:
    MyCustomAsset(smlt::AssetManager* manager) : smlt::Asset(manager) {
        // Custom initialization
    }

    // Your custom data and methods
    std::vector<float> custom_data;
    int some_value = 0;
};
```

### Registering Your Loader

Register the loader with the Application so it can be found by file extension:

```cpp
// In your application's init
auto loader_type = std::make_shared<CustomLoaderType>();
app->register_loader_type(loader_type);
```

### Using Custom Assets

Once registered, your custom assets work exactly like built-in ones:

```cpp
// The engine will automatically use CustomLoader for .custom files
auto asset = assets->load_custom_asset("data/file.custom");
```

### Attaching Custom Data to Any Asset

All `Asset` objects have a `data` property of type `DataCarrier` that can hold arbitrary key-value data:

```cpp
auto mesh = assets->load_mesh("level.obj");
mesh->data.set("level_number", 5);
mesh->data.set("spawn_point", smlt::Vector3(10, 0, 20));

// Later
int level = mesh->data.get<int>("level_number");
```

---

## 11. Best Practices for Asset Organization

### Directory Structure

Organize your assets into clearly named directories:

```
project/
  assets/
    models/
      characters/
      vehicles/
      environment/
    textures/
      characters/
      environment/
      ui/
    materials/
    sounds/
      music/
      effects/
      ui/
    fonts/
    particles/
    prefabs/
    data/
```

### Naming Conventions

- Use lowercase with underscores for filenames: `hero_mesh.glb`, `grass_tile.png`
- Include the asset type in the name when ambiguous: `hero_diffuse.png` vs `hero_normal.png`
- Group related assets in subdirectories: `textures/environment/forest_ground.png`

### Asset Search Path Setup

Add your project's root asset directory as a search path during application initialization:

```cpp
struct MyConfig : public smlt::AppConfig {
    MyConfig() {
        search_paths.push_back("assets");
        search_paths.push_back("/path/to/shared/data");
    }
};

class MyGame : public smlt::Application {
protected:
    std::unique_ptr<smlt::AppConfig> create_config(int, char**) override {
        return std::make_unique<MyConfig>();
    }
};
```

### Naming Assets for Debugging

Always give assets descriptive names. These appear in debug output and the stats panel:

```cpp
auto mesh = assets->load_mesh("hero.glb");
mesh->set_name("HeroMesh");

auto tex = assets->load_texture("grass.png");
tex->set_name("GrassTile_Diffuse");
```

You can then find assets by name:

```cpp
auto found = assets->find_mesh("HeroMesh");
if (found) {
    S_INFO("Found hero mesh!");
}
```

### Scene Asset Lifecycle

Follow the load/activate pattern for clean asset management:

```cpp
class GameScene : public smlt::Scene {
protected:
    void on_load() override {
        // Load heavy assets here (happens during loading screen)
        level_mesh_ = assets->load_mesh("levels/forest.glb");
        hero_tex_ = assets->load_texture("textures/hero_diffuse.png");
    }

    void on_activate() override {
        // Use pre-loaded assets to build the scene
        auto hero = create_child<smlt::Actor>(hero_tex_);
        auto level = create_child<smlt::Actor>(level_mesh_);
    }

    void on_unload() override {
        // Assets are automatically released when the scene unloads
        // No manual cleanup needed
    }

private:
    smlt::MeshID level_mesh_;
    smlt::TextureID hero_tex_;
};
```

---

## 12. Memory Management and Performance

### Storing IDs, Not Pointers

The single most important rule: **store AssetIDs, not shared pointers**. Holding a `shared_ptr` prevents garbage collection:

```cpp
// BAD: Holding a shared_ptr prevents garbage collection
class Player {
    smlt::MeshPtr mesh_;  // Ref count never drops to 1!
};

// GOOD: Store the ID
class Player {
    smlt::MeshID mesh_id_;  // No ownership, no GC interference

    void update(smlt::AssetManager* assets) {
        auto mesh = assets->mesh(mesh_id_);  // Borrow temporarily
        // Use mesh...
    }  // Released at end of scope
};
```

### Scope Blocks for Asset Access

Keep asset pointers alive for the shortest time possible:

```cpp
// BAD: Holding pointer across unrelated code
auto mat = assets->material(mat_id_);
do_expensive_calculation();  // Unrelated work
update_physics();
mat->set_diffuse_color(color);  // Mat still held!

// GOOD: Scope block
{
    auto mat = assets->material(mat_id_);
    mat->set_diffuse_color(color);
}  // Released immediately
do_expensive_calculation();
update_physics();
```

### Single-Line Access for One-Shot Calls

```cpp
assets->mesh(mesh_id_)->recalculate_normals();  // Pointer released immediately
```

### Choosing the Right GC Method

| Asset Type | GC Method | Reason |
|------------|-----------|--------|
| Level mesh | `GARBAGE_COLLECT_PERIODIC` (default) | Only needed for this scene |
| Reusable projectile mesh | `GARBAGE_COLLECT_NEVER` | Spawns and despawns repeatedly |
| UI texture | `GARBAGE_COLLECT_NEVER` or `shared_assets` | Used across all scenes |
| Temporary render target | `GARBAGE_COLLECT_PERIODIC` | Only needed briefly |
| Background music | `GARBAGE_COLLECT_NEVER` | Loops throughout gameplay |

### Monitoring Asset Counts

Use the `*_count()` methods to monitor memory usage:

```cpp
S_INFO("Meshes: {}", assets->mesh_count());
S_INFO("Textures: {}", assets->texture_count());
S_INFO("Materials: {}", assets->material_count());
```

### Iterating All Assets

When you need to operate on every asset of a type:

```cpp
assets->each_mesh([&](uint32_t index, smlt::MeshPtr mesh) {
    S_INFO("Mesh {}: {}", index, mesh->name());
    mesh->recalculate_aabb();
});
```

### Asset Attachment to StageNodes

Attaching an asset to a StageNode (like an Actor) automatically creates a reference:

```cpp
auto mesh = assets->load_mesh("tree.glb");
auto tree = stage->create_child<smlt::Actor>(mesh);
mesh.reset();  // Safe: Actor holds the reference

// When tree is destroyed, the mesh reference is released
// If no other references exist, GC will free the mesh
```

### Performance Tips

1. **Pre-stream large audio files**: Use `SoundFlags::stream_audio = true` (default) for music
2. **Load audio into memory for SFX**: Set `stream_audio = false` for short, frequently-played sounds
3. **Use mipmaps for distant textures**: Set `flags.mipmap = true` to reduce memory bandwidth
4. **Share materials where possible**: Cloning a material is cheaper than loading a new one
5. **Name your assets**: Makes debugging and profiling much easier
6. **Clean up explicitly during scene transitions**: Call `assets->destroy_all()` if you want a guaranteed clean slate
7. **Monitor reference counts**: Use `ptr.use_count()` to debug leaks

### Common Pitfalls

| Pitfall | Symptom | Fix |
|---------|---------|-----|
| Ignoring the return value of `load_X()` | Asset disappears immediately | Store the result: `auto mesh = assets->load_mesh(...)` |
| Holding `shared_ptr` as a member | Memory leak, GC never runs | Store `AssetID` instead |
| Not checking `has_X()` before access | Null pointer crash | Use existence guard: `if (assets->has_mesh(id)) { ... }` |
| Loading shared assets through scene assets | Asset lost on scene change | Use `window->shared_assets` |
| Destroying an asset still referenced by a node | Undefined behavior | Remove references first, or let GC handle it |
| Mixing const and non-const accessor confusion | Compilation errors | Use `mesh(id)` for mutable, `mesh(id) const` for read-only |

---

## Quick Reference Card

### Loading Assets

```cpp
// Load from file
MeshPtr      m = assets->load_mesh("model.obj");
TexturePtr   t = assets->load_texture("image.png");
MaterialPtr  mt = assets->load_material("mat.material");
SoundPtr     s = assets->load_sound("audio.ogg");
FontPtr      f = assets->load_font("font.ttf", flags);
ParticleScriptPtr ps = assets->load_particle_script("fx.script");
BinaryPtr    b = assets->load_binary("data.bin");
PrefabPtr    p = assets->load_prefab("scene.glb");

// Create programmatically
MeshPtr      m = assets->create_mesh(vertex_spec);
TexturePtr   t = assets->create_texture(w, h, format);
MaterialPtr  mt = assets->create_material();
FontPtr      f = assets->create_font_from_memory(data, size, flags);
FontPtr      f = assets->create_font_from_family("Kanit", flags);
MeshPtr      m = assets->create_mesh_from_heightmap("height.png", spec);
MeshPtr      m = assets->create_mesh_as_cube_with_submesh_per_face(size);
MaterialPtr  mt = assets->create_material_from_texture(tex);
PrefabPtr    p = assets->create_prefab(root_node);
```

### Accessing Assets

```cpp
MeshPtr      m = assets->mesh(id);
TexturePtr   t = assets->texture(id);
MaterialPtr  mt = assets->material(id);
SoundPtr     s = assets->sound(id);
FontPtr      f = assets->font(id);
ParticleScriptPtr ps = assets->particle_script(id);
BinaryPtr    b = assets->binary(id);
PrefabPtr    p = assets->prefab(id);
```

### Checking Existence

```cpp
bool has = assets->has_mesh(id);
bool has = assets->has_texture(id);
bool has = assets->has_material(id);
bool has = assets->has_sound(id);
bool has = assets->has_font(id);
bool has = assets->has_particle_script(id);
bool has = assets->has_binary(id);
bool has = assets->has_prefab(id);
```

### Finding by Name

```cpp
MeshPtr      m = assets->find_mesh("name");
TexturePtr   t = assets->find_texture("name");
MaterialPtr  mt = assets->find_material("name");
SoundPtr     s = assets->find_sound("name");
FontPtr      f = assets->find_font("name");
ParticleScriptPtr ps = assets->find_particle_script("name");
BinaryPtr    b = assets->find_binary("name");
PrefabPtr    p = assets->find_prefab("name");
```

### Destroying Assets

```cpp
assets->destroy_mesh(id);
assets->destroy_texture(id);
assets->destroy_material(id);
assets->destroy_sound(id);
assets->destroy_font(id);
assets->destroy_particle_script(id);
assets->destroy_binary(id);
assets->destroy_prefab(id);
assets->destroy_all();
assets->run_garbage_collection();
```

### Garbage Collection

```cpp
// Set on existing asset
mesh->set_garbage_collection_method(GARBAGE_COLLECT_NEVER);
mesh->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);

// Load with specific method
auto mesh = assets->load_mesh("file.obj", spec, opts, GARBAGE_COLLECT_NEVER);
auto tex  = assets->load_texture("file.png", GARBAGE_COLLECT_NEVER);
```

### VFS Operations

```cpp
// Search paths
vfs->add_search_path("/path");
vfs->insert_search_path(0, "/priority_path");
vfs->remove_search_path("/old_path");

// File operations
auto path = vfs->locate_file("filename");
auto stream = vfs->read_file("filename");
auto lines = vfs->read_file_lines("filename");
auto file = vfs->open_file("filename");

// Cache management
vfs->clear_location_cache();
vfs->set_location_cache_limit(128);
size_t cached = vfs->location_cache_size();

// Read blocking
vfs->enable_read_blocking();
vfs->disable_read_blocking();
bool blocking = vfs->read_blocking_enabled();
```

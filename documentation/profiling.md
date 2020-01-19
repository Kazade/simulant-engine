
# Profiling

## Enabling Profile Mode

Profile mode does several things (depending on platform)

 - Disables vsync
 - Disables frame limiting
 - Enables a profiler (Dreamcast only)
 
Profiling mode can be enabled a number of ways

 - You can set the `SIMULANT_PROFILE` environment variable to 1
 - You can set `AppConfig::development::force_profiling` to true
 - You can compile Simulant with `-DSIMULANT_PROFILE` (passed as an argument to CMake normally)
 
 
## The Dreamcast Profiler

The Dreamcast profiler is a built-in sampling profiler that runs in a background thread. When the Simulant
application is run via dcload and profiling is enabled, a file will be generated at `/pc/gmon.out`, you'll
need to pass the `-c` argument to dcload to get this to work (and possibly run under `sudo`)

The generated file can be passed to `gprof` to generate a flat profile. Remember that by default
Dreamcast .elfs have their debugging symbols stripped and moved to a .debug extension so you'll
need to use the .debug file, like so:

```
sh-elf-gprof -b -p samples/nehe02.elf.debug gmon.out
```

> Note: if you don't use the sh-elf-gprof executable, none of the function name mangling will be
> performed correctly

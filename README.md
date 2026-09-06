# Ray Tracer

A C++17 ray tracer based on Peter Shirley's *Ray Tracing in One Weekend*. It includes preset scenes, BVH acceleration, several anti-aliasing strategies, deterministic sampling, multi-threaded tiled rendering, and CSV benchmark output.

## Reference

This project was developed with [*Ray Tracing in One Weekend*](https://raytracing.github.io/books/RayTracingInOneWeekend.html) as a reference.

## Requirements

- CMake 3.10 or newer
- A compiler with C++17 support (MSVC, Clang, or GCC)

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

On Windows with a multi-configuration generator, the executable is `build\\Release\\RayTracer.exe`.

## Render an image

Render the default scene to a PPM file:

```powershell
.\\build\\Release\\RayTracer.exe --output output.ppm
```

Render the Cornell box with BVH acceleration, 16 random samples per pixel, eight workers, and a width of 800 pixels:

```powershell
.\\build\\Release\\RayTracer.exe 7 1 1 bvh 1 random 16 --threads 8 --width 800 --no-progress --output cornell.ppm
```

The renderer writes PPM (P3) images. Open them in an image viewer that supports PPM, or convert them with an external image tool.

## Command-line arguments

The positional arguments are optional and are interpreted in this order:

```text
RayTracer [scene] [scene_seed] [sampling_seed] [plain|bvh] [build_seed] [runs] [off|random|grid|jittered] [samples]
```

- `scene`: `1` bouncing spheres, `2` checkered spheres, `3` earth, `4` Perlin spheres, `5` quads, `6` simple light, `7` Cornell box, `8` Cornell smoke, or `9` final scene. The default is Cornell smoke.
- `scene_seed`, `sampling_seed`, and `build_seed` make generated scenes and sampling reproducible.
- `plain` or `bvh` selects the top-level acceleration structure.
- `runs` greater than one enables benchmark mode and emits CSV to standard error.
- `grid` and `jittered` require a perfect-square sample count, such as `4`, `16`, or `64`.

Available options:

```text
--output <path>              Write the PPM image to a file.
--reference <path>           Compare the result with a PPM reference image.
--width <pixels>             Override the preset image width.
--threads <count>            Set the requested worker count.
--tile-size <pixels>         Set the square tile size (default: 16).
--no-progress                Disable progress output.
--benchmark                  Emit benchmark CSV, including for a single run.
--disable-internal-bvh       Disable BVHs inside the final scene.
```

## Benchmark example

```powershell
.\\build\\Release\\RayTracer.exe 9 1 1 bvh 1 random 16 --benchmark --threads 8 --width 800 --no-progress 2> benchmark.csv
```

This renders the final scene once and writes timing and configuration data as CSV to `benchmark.csv`.

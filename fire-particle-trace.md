# Fire particle trace

`+FireTrace <file>` enables an optional JSONL measurement of the two additive
systems of each Fire near the camera. `+FireTraceDistance <units>` changes the
maximum camera distance (default 1000). Without a file the trace is disabled.
Use `+FireTrace -` or `+DemoTrack -` to send those records to standard output.
The latter also supplies view timestamps for an otherwise identical trace-off
measurement of frame times.

Each record follows the normal Fire update and contains both clocks, the system
and particle identities, last actual birth time and batch count, the camera basis
and field of view, screen dimensions, texture resource name, effective system
radius and scale endpoints, and every live particle's local position, velocity,
remaining/total lifetime, size, RGB and alpha. Repeated birth times denote frames
without a new batch. Particle identities may be reused after death.

The trace reads state and consumes no random values. File I/O can affect the frame
rate; use the recorded clocks when measuring birth cadence. The texture name is
the requested resource, not proof of loaded texture dimensions, sprite frame or
sampler state. Projected corners and the renderer's loaded texture state require
a separate controlled rendering measurement.

## One-particle rendering probe

`+FireProbeTexture <resource>` replaces live flame rendering with one stationary
additive particle, recreated at age zero each frame. Use a direct DTX frame to
freeze the texture. `FireProbeX/Y/Z` set its world centre, `FireProbeRadius` sets
the unnormalized radius (default 4000, divided by screen width as SmokeFX does),
`FireProbeScale` scales that radius (default 1), and `FireProbeTint` sets all three
RGB channels in 0..255 (default 100). `FireProbeBackground 0` draws only this
object over a cleared background; `1` keeps the world, smoke and lights but
hides the ordinary flames. A tint-zero background measurement can separate its
contribution. These switches are diagnostics and do not alter ordinary play.

`+FireDrawTrace 1` captures the actual pretransformed additive draw inputs on pose
screenshot frames: vertices, UVs, packed diffuse tint, loaded texture dimensions,
pixel format, blend factors and min/mag/mip filters. `+FireProbePixels 1` also
locks and records the top-level texture bytes. The lock may stall the GPU; those
takes are not population or frame-time measurements.
The attached mip surfaces are recorded too, with LOD bias, maximum mip, address
modes and texture color-stage state. A `fire-probe` record reads the particle's
actual size and remaining/total life immediately before that screenshot render.

The renderer's device location is a last-resort measurement interface: the SDK
does not expose particle sprite phase or its loaded texture. The probe requires
the retail D3D renderer's PE timestamp 3ae73306 and image size b5000 and checks the
relocated device reference at RVA e20c before reading RVA 8d988. It temporarily
wraps the device's two primitive submission methods for one synchronous render
and restores its original vtable immediately afterwards. Other renderer versions
are rejected. No engine implementation is included.

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

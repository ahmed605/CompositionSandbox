# Composition Sandbox

Collection of various Windows Composition API Experiments

> [!CAUTION]
> These experiments are just for fun, it's highly discouraged to use any of the experiments shown here in production code, they aren't well-tested and make use of internal/private APIs that can change at any time.

## Experiments

### [Cross Process Visuals](https://github.com/ahmed605/CompositionSandbox/blob/master/CompositionSandbox.Native/CrossProcessVisuals.h)

This experiment showcases a Composition Visual created in a process (the "main" one) then sent to another process (the "secondary" one) via a Window Message to be used as render target in that secondary process while same visual used as root for an HWND render target in the main process.

https://github.com/ahmed605/CompositionSandbox/assets/34550324/b9d613f7-96e8-457d-a4d7-f9180d65666b


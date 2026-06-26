# SRCDS Watchdog called guarddog

> [!WARNING]
> Here be dragons! This plugin really only works in Garry's Mod sanely. Untested elsewhere.

`serverplugin_guarddog` is a Source server plugin that detects frozen server execution and triggers a kill sequence when SRCDS stops responding.

<img width="701" height="336" alt="screenshot of countdown" src="https://github.com/user-attachments/assets/a541721d-aa4d-4e51-826a-5e26755be1bc" />


**Features:**
- Monitors `GameFrame` progress from the Source server
- Starts a timeout countdown when frames are missing
- Sends `SIGUSR1` and `SIGUSR2` after configurable delays
- Forces a process kill if the hang persists
- Optional Linux helper support via `dog_frozen` shell script

## Install gmsv_segfault

You need **[gmsv_segfault](https://github.com/Python1320/gmsv_segfault)** to utilize this watchdog properly. 
- `guarddog` watches for server freezes and generates failure signals
- `gmsv_segfault` is a companion crash-handling module that catches signals like `SIGUSR1`/`SIGUSR2` and writes diagnostics or backtraces
- Used together, `guarddog` can detect hangs and `gmsv_segfault` can capture useful crash/debug output before the process exits and recover from some infinite loops in Lua.

### Infinite-loop recovery with `SIGUSR1`

When `gmsv_segfault` receives `SIGUSR1`, it does not immediately crash the server. Instead, it installs a short Lua hook on the main Lua state and forces a Lua error after a small number of bytecode instructions.

This means that if the server is stuck in a Lua infinite loop, `gmsv_segfault` can break out of the loop by injecting a `lua_error` through the hook. With `guarddog` configured to send `SIGUSR1` before the final kill timeout, the pair can recover from programming loops and still preserve a signal dump for debugging.

### How recovery is detected

`gmsv_segfault` treats `SIGUSR1` as a non-fatal recovery signal. When it receives `SIGUSR1`, it installs the Lua hook and then returns from the signal handler instead of immediately dumping and exiting.

`guarddog` detects recovery when the server resumes normal frame progress: `GameFrame` is executed again, which sets `has_fuzzy_respose` true and clears the timeout state.

The practical sign of recovery is:
- the log shows `SIGUSR1:lua_sethook_hack`
- `GameFrame` runs again and `guarddog` logs a recovery marker like `R>`
- no fatal crash dump is produced for that signal
- the process continues running after the injected Lua error

If the server does not recover, `guarddog` will still continue its hang timeout and eventually force the final kill sequence.

## Build

1. Install [premake](https://premake.github.io/download/)
2. Run `premake5` from the repo root to generate a build project for your platform
3. Build the generated solution or makefiles
4. Produced artifact:
   - `serverplugin_guarddog.dll` on Windows
   - `serverplugin_guarddog.so` on Linux

## Install

1. Copy the built `serverplugin_guarddog.dll` / `serverplugin_guarddog.so` into `garrysmod/addons/` along with the VDF
2. Ensure SRCDS loads the plugin via the normal server plugin mechanism (plugin_X)
3. Optionally install `gmsv_segfault` alongside this plugin to capture signal dumps for `SIGUSR1`/`SIGUSR2`

## Configuration

The following environment variables are supported:

- `DOG_DELAY` — maximum hang timeout in seconds before the plugin kills the server (default: `22`)
- `DOG_USR1DELAY` — delay in seconds before sending `SIGUSR1`
- `DOG_USR2DELAY` — delay in seconds before sending `SIGUSR2`
- `DOG_SHELL=1` — enable `dog_frozen` helper behavior on Linux

## Notes

- The plugin description returned by the module is `Server Freeze Guard`
- On Linux, `DOG_SHELL=1` causes the plugin to invoke the bundled `dog_frozen` helper when crashing
- `guarddog` itself does not generate crash dumps; it is primarily a freeze detector and crash trigger

## Thanks

Garry, Metastruct, and everyone who contributed to the Source server crash/debug tooling.


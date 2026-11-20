# Procwatch

Which processes are using the most resident memory right now? Procwatch reads Linux `/proc` once per second and prints the five largest entries. It keeps five rows in memory regardless of the number of processes. Ties use ascending PID order.

## Try it

On Linux or WSL with a C compiler, Make and Python 3:

```sh
make
./procwatch --once
./procwatch                 # refresh every second, Ctrl+C to exit
make test
```

In a second terminal run `python3 -c 'import time; b = bytearray(128 * 1024**2); time.sleep(15)'`. Watch the Python process appear if it reaches your top five, then disappear. Increase the allocation only if your machine has enough free memory. Measure a single scan with `/usr/bin/time -v ./procwatch --once`.

## How the ranking works

Each scan reads Name and VmRSS from the same PID status file. A vanished PID, unavailable RSS, or denied access increments the skipped count. The kernel reports RSS in KiB and shared pages can appear in more than one process. A scan samples processes in sequence, so its rows can describe slightly different instants.

The fixture test supplies a temporary directory via `--proc DIR` and checks ties, missing processes, missing RSS and ordering without depending on the host's running applications.

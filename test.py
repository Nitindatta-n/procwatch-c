import pathlib
import subprocess
import tempfile

with tempfile.TemporaryDirectory() as tmp:
    root = pathlib.Path(tmp)
    for pid, rss in [(1, 50), (2, 85), (3, 12), (4, 0), (5, 85), (6, 89), (7, 94)]:
        (root / str(pid)).mkdir()
        (root / str(pid) / "status").write_text(f"Name:\tworker {pid}\nVmRSS:\t{rss} kB\n")
    (root / "99").mkdir()  # Listed PID whose status disappeared.
    (root / "98").mkdir()
    (root / "98" / "status").write_text("Name:\tkthread\n")
    out = subprocess.check_output(["./procwatch", "--once", "--proc", tmp], text=True)
    assert [int(row.split()[0]) for row in out.splitlines()[1:-1]] == [7, 6, 2, 5, 1], out
    assert "Skipped 2" in out and "worker 7" in out
    assert subprocess.run(["./procwatch", "--once", "--proc", tmp + "/missing"]).returncode == 1
print("PASS: top five, ties, missing PID, missing RSS, unreadable root")

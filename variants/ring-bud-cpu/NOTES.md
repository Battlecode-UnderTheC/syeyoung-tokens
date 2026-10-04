# ring-bud-cpu

ring-bud-1 plus ring-cpu-1's CPU fix: `diff -u ring-explore-2 ring-cpu-1` applied with patch. The fix reuses scratch arrays, uses a body mark array, keeps small QueenContinuation states, and adds a deterministic work budget for the QueenMove/Ring4Exit beams. It should play the same as ring-bud-1 with the worst turn under 80M. See variants/ring-cpu-1/NOTES.md.

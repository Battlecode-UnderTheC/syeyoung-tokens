# worldgen.py: provenance

A procedural, symmetric map generator by GitHub user xCirno1, copied unchanged from
https://gist.github.com/xCirno1/ffdaac4236c1f1085c351af4fdfc1600 (gist revision
2d49ffd7dba611fbfe8b27e5d770a90359ee34ec, 2026-09-27). Upstream's README is README.upstream.md.

The gist states no license: this copy is for our own local use only. Do not publish it.

Standard library only; `--seed` reproduces a map exactly. The league generates its rotating map
suite with it (league/league.py). Anyone can make practice maps:

    python3 maps/worldgen/worldgen.py --batch 20 --outdir /tmp/practice
    python3 maps/worldgen/worldgen.py --seed 7 --landmark labyrinth -o maze.map

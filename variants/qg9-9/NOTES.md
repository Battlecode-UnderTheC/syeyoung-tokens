# qg9-9

qg-board-1 (queen-guard-7) plus a queen that keeps length 4 through the opening: before r20 no economy split may leave the
queen shorter than 4 (then 2 until r100, 3 after, as in qg9-1). Only queen_guard.hpp changes.

Why: on cramped maps the spawn split leaves the queen a 2-long stub and the guard keeps it from
foraging (per-dragon logs, stripes + weakhold: guarded queen splits at r20-39 were 21 v 103
unguarded). A 4-long queen gets free 2-step sprints and outgrows the stub stage.

Sim vs gst-search-8 (qg9-8 on qg9-2, against qg9-2 itself):
| maps | qg9-8 | qg9-2 |
|---|---|---|
| stripes + weakhold, seeds 11-80 (280) | 139-0-141, r100 dragons 9.7 v 12.6, queen died 78%, queen-decided 50-0 | 88-0-192, 6.9 v 13.7, 77%, 60-0 |
| tournament-sprint (7 maps), seeds 1-10 (140) | 95-0-45, queen died 56% | 86-0-54, 56% |
| server, seeds 1-16 (320) | 157-2-161, queen died 83% | 166-2-152, 83% |
| official, seeds 1-6 (180) | 97-1-82, queen died 67% | 98-1-81, 66% |

On qg-board-1, stripes + weakhold seeds 11-50 (160): qg9-9 80-0-80 (r100 dragons 9.7 v 12.5, queen
died 74%) v qg-board-1 54-0-106 (7.0 v 13.7, 76%).


# Generation reports

One file per generation, `g<N>.md`, written by the orchestrator (`ORCHESTRATOR.md`):

```
# Generation N (date)

## Plan
| task | variant | operator | parents | focus | why |

## Results
| variant | status | Elo ± | parents' Elo | vs parent (W-D-L) | holdout | change in one line |
worker outcomes: submitted / not submitted (why), tokens

## What we learned
claims with evidence; what went into KNOWLEDGE.md

## Next
```

Generation 0 is the starting population: strategy-v1, anchor-random, anchor-greedy.

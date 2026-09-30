# arena-search-7

## Idea
arena-search-6 (per-dragon lookahead with an adversarial first-move reply judged by who eats the
remains), with the reply made less fearful, and hunter's champion rules.

## What changed from the parent, and why
1. **Reply cost and lunge threshold, less cautious.** The reply cost went from kReplyP 0.5 to
   0.25, and we now lunge when the remains-aware trade value is above 0 (kTradeMargin 0.5 -> 0).
   On tuning seeds 6-10 vs arena-search-6 (150 games):
   - 0.25 / 0: 78-8-64
   - 1.0 / 1.0 (more cautious): 63-6-81

   The same lesson as rounds 4-5: caution costs more than it saves.
2. **Squeeze.** A late champion (from SplitUntil - 60) that is doomed but has a step that doesn't
   die at once moves on instead of SPLIT length-2, which hands its bulk to a new id. This is
   hunter's round-6 idea.
3. **Pretender gate.** Only a dragon of length 6+ names itself champion, and an expired
   champion isn't replaced by a short self. Ping only when a champion is known.

Changes 2-3 alone (7A): 73-4-73 vs arena-search-6 (seeds 1-5). They are neutral and kept.

## Tried and dropped
- **Deeper reply** (kDeepReply). The same remains-aware trade test at moves 1-2, costed 0.25 and
  0.1, lost 55-3-92 vs arena-search-6 (seeds 1-5). A static field of where enemies could be
  later makes the search timid again.

#!/usr/bin/env python3
"""Pictures of a replay (PIL only), built on decode.py.

  python3 render.py <replay> <out.png>                       contact sheet: 4x4 frames, rounds 0..last
  python3 render.py --film <replay> <round> <out.png> [--at X Y] [--radius R] [--frames N] [--step K]
        filmstrip of N (default 9) frames, K rounds apart (default 1), centred on <round>,
        zoomed on tile (X, Y) with R tiles around it (default: whole map); dragon ids drawn at heads

Colours: OUR team (the side with 0xA7 sonar words) cyan, the opponent orange; heads are bright
squares, the fading dots behind a head are its last 6 head tiles; pearls yellow dots; kelp grey
lines; portals magenta; inferred beds dim squares.
league-2 additions: each team's longest dragon (>= 6) ringed (red ring = ours, white = theirs);
deaths since the previous frame marked x (red = ours, grey = theirs; thick = length >= 6);
sheet --from R starts the 16 frames at round R (midgame/endgame sheets).
"""
import argparse
import collections
import os
import sys

from PIL import Image, ImageDraw, ImageFont

import decode

BG = (14, 18, 28)
KELP = (110, 120, 130)
PORTAL = (220, 60, 220)
PEARL = (255, 220, 60)
BED = (40, 46, 60)
COL = {"us": ((40, 150, 200), (150, 240, 255)), "opp": ((200, 100, 40), (255, 200, 120))}
TXT = (230, 230, 230)


def font(size):
    for p in ("/System/Library/Fonts/Menlo.ttc", "/System/Library/Fonts/Monaco.ttf",
              "/Library/Fonts/Arial.ttf", "/System/Library/Fonts/Supplemental/Arial.ttf"):
        if os.path.exists(p):
            try:
                return ImageFont.truetype(p, size)
            except OSError:
                pass
    return ImageFont.load_default()


def snapshots(d, rounds, trail=6):
    """State at the end of each requested round: bodies, teams, pearls, head trails, beds."""
    want = set(rounds)
    snaps = {}
    trails = collections.defaultdict(lambda: collections.deque(maxlen=trail))
    last = None
    deaths = []
    for kind, st, info in decode.walk(d):
        if kind == "death":
            rr, i, tm, why, ln, det = info
            b = st.body.get(i) if hasattr(st.body, "get") else st.body[i]
            if b:
                deaths.append((rr, tm, b[0], ln, why + (":" + det if det else "")))
        if kind == "turn" and info["path"]:
            for p in info["path"]:
                trails[info["id"]].append(p)
        elif kind == "round_end":
            last = info
            if info in want:
                snaps[info] = dict(
                    round=info,
                    bodies={i: list(st.body[i]) for i in st.alive},
                    team=dict(st.team),
                    pearls=set(st.pearls),
                    beds=set(st.beds),
                    trails={i: list(trails[i]) for i in st.alive},
                    stand={t: st.standing(t) for t in "AB"})
    rs = sorted(snaps)
    for k, r in enumerate(rs):
        lo = rs[k - 1] if k else -1
        snaps[r]["deaths"] = [x for x in deaths if lo < x[0] <= r]
    return snaps, last


def last_round(d):
    r = 0
    for e in d["events"]:
        if e[0] == "roundStart":
            r = e[1]
    return r


def draw_frame(g, snap, ours, s, view=None, ids=False, title=""):
    """One frame as an Image. view = (x0, y0, w, h) tiles (torus-wrapped) or None for the whole map."""
    if view is None:
        x0, y0, vw, vh = 0, 0, g.W, g.H
    else:
        x0, y0, vw, vh = view
    th = 16 if s < 12 else 20
    img = Image.new("RGB", (vw * s + 1, vh * s + 1 + th), BG)
    dr = ImageDraw.Draw(img)

    def pos(t):
        """pixel top-left of tile t in this view, or None if outside."""
        x = (t[0] - x0) % g.W
        y = (t[1] - y0) % g.H
        if x >= vw or y >= vh:
            return None
        return x * s, y * s + th

    for t in snap["beds"]:
        p = pos(t)
        if p:
            dr.rectangle([p[0], p[1], p[0] + s - 1, p[1] + s - 1], fill=BED)
    # edges
    for vx in range(vw):
        for vy in range(vh):
            x, y = (x0 + vx) % g.W, (y0 + vy) % g.H
            px, py = vx * s, vy * s + th
            for d_, line in (("N", [px, py, px + s, py]), ("W", [px, py, px, py + s])):
                eid = g.edge_id(x, y, d_)
                typ, _ = g.edges.get(eid, (0, -1))
                if typ == 1:
                    dr.line(line, fill=KELP, width=1 if s < 10 else 2)
                elif typ == 2:
                    dr.line(line, fill=PORTAL, width=2)
    # pearls
    r = max(1, s // 4)
    for t in snap["pearls"]:
        p = pos(t)
        if p:
            cx, cy = p[0] + s // 2, p[1] + s // 2
            dr.ellipse([cx - r, cy - r, cx + r, cy + r], fill=PEARL)
    # trails, bodies, heads
    inset = 1 if s >= 5 else 0
    for i, body in snap["bodies"].items():
        side = "us" if snap["team"][i] == ours else "opp"
        bc, hc = COL[side]
        tr = snap["trails"].get(i, [])
        for k, t in enumerate(tr[:-1]):
            p = pos(t)
            if p and t not in body:
                a = (k + 1) / len(tr)
                c = tuple(int(BG[j] + (hc[j] - BG[j]) * a * 0.6) for j in range(3))
                cx, cy = p[0] + s // 2, p[1] + s // 2
                rr = max(1, s // 5)
                dr.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=c)
        for t in body[1:]:
            p = pos(t)
            if p:
                dr.rectangle([p[0] + inset, p[1] + inset, p[0] + s - 1 - inset, p[1] + s - 1 - inset], fill=bc)
        p = pos(body[0])
        if p:
            dr.rectangle([p[0], p[1], p[0] + s - 1, p[1] + s - 1], fill=hc)
            if len(body) >= 10 and s >= 4:
                dr.rectangle([p[0] - 1, p[1] - 1, p[0] + s, p[1] + s], outline=(255, 255, 255))
    # each team's longest dragon: a red (ours) / white (theirs) ring two pixels out
    for side_team in set(snap["team"][i] for i in snap["bodies"]):
        best = max((i for i in snap["bodies"] if snap["team"][i] == side_team), key=lambda i: len(snap["bodies"][i]), default=None)
        if best is not None and len(snap["bodies"][best]) >= 6:
            p = pos(snap["bodies"][best][0])
            if p:
                col = (255, 60, 60) if side_team == ours else (255, 255, 255)
                dr.ellipse([p[0] - s, p[1] - s, p[0] + 2 * s, p[1] + 2 * s], outline=col, width=2)
    # deaths since the previous frame: an x (red = ours, grey = theirs), bigger for length >= 6
    for (rr, tm, t, ln, why) in snap.get("deaths", []):
        p = pos(t)
        if p:
            col = (255, 40, 40) if tm == ours else (150, 150, 150)
            e = s if ln >= 6 else s // 2
            cx, cy = p[0] + s // 2, p[1] + s // 2
            dr.line([cx - e, cy - e, cx + e, cy + e], fill=col, width=2 if ln >= 6 else 1)
            dr.line([cx - e, cy + e, cx + e, cy - e], fill=col, width=2 if ln >= 6 else 1)
    if ids:
        f = font(max(9, s // 2 + 2))
        for i, body in snap["bodies"].items():
            p = pos(body[0])
            if p:
                dr.text((p[0] + s, p[1] - 2), f"{i}:{len(body)}", fill=(255, 255, 255), font=f)
    f = font(11 if s < 12 else 13)
    dr.rectangle([0, 0, img.width, th - 1], fill=(0, 0, 0))
    dr.text((3, 2), title, fill=TXT, font=f)
    return img


FORCE_US = None


def frame_title(snap, ours):
    opp = "B" if ours == "A" else "A"
    su, so = snap["stand"][ours], snap["stand"][opp]
    return f"r{snap['round']}  C n{su['count']} L{su['longest']} T{su['total']} | O n{so['count']} L{so['longest']} T{so['total']}"


def sheet(path, out, cols=4, rows=4, width=1600, start=0):
    d = decode.load(path)
    g = decode.GameMap(d["map"])
    ours = FORCE_US or decode.our_team(d) or "A"
    lr = last_round(d)
    n = cols * rows
    rounds = sorted({round(start + k * (lr - start) / (n - 1)) for k in range(n)})
    snaps, _ = snapshots(d, rounds)
    s = max(2, min((width // cols - 8) // g.W, 420 // g.H))
    frames = [draw_frame(g, snaps[r], ours, s, title=frame_title(snaps[r], ours)) for r in rounds if r in snaps]
    fw, fh = frames[0].size
    head = 44
    img = Image.new("RGB", (cols * (fw + 6) + 6, rows * (fh + 6) + 6 + head), (0, 0, 0))
    dr = ImageDraw.Draw(img)
    res = d["result"]
    win = res["winner"]
    verdict = "WIN" if win == ours else ("LOSS" if win else "DRAW")
    opp = "B" if ours == "A" else "A"
    dr.text((8, 4), f"{os.path.basename(path).split('.')[0]}  {g.name} {g.W}x{g.H} sym={g.symmetry}   "
                    f"CYAN = {ours}:{decode.botname(d, ours)}   ORANGE = {opp}:{decode.botname(d, opp)}   cyan {verdict} by {res['endReason']} at r{lr}",
            fill=TXT, font=font(15))
    dr.text((8, 24), f"final US n{res[ours]['count']} longest {res[ours]['longest']} total {res[ours]['total']}   "
                     f"OPP n{res[opp]['count']} longest {res[opp]['longest']} total {res[opp]['total']}   "
                     f"(heads bright, >=10-long heads outlined white, dots = last 6 head tiles, yellow = pearls)",
            fill=(180, 180, 180), font=font(12))
    for k, fr in enumerate(frames):
        c, r = k % cols, k // cols
        img.paste(fr, (6 + c * (fw + 6), head + 6 + r * (fh + 6)))
    img.save(out)
    return out


def film(path, rnd, out, at=None, radius=None, frames=9, step=1, width=1600):
    d = decode.load(path)
    g = decode.GameMap(d["map"])
    ours = FORCE_US or decode.our_team(d) or "A"
    lr = last_round(d)
    start = max(0, rnd - (frames // 2) * step)
    rounds = [r for r in range(start, start + frames * step, step) if r <= lr]
    snaps, _ = snapshots(d, rounds)
    if at is not None and radius is not None:
        vw, vh = min(g.W, 2 * radius + 1), min(g.H, 2 * radius + 1)
        view = ((at[0] - vw // 2) % g.W, (at[1] - vh // 2) % g.H, vw, vh)
    else:
        view = None
        vw, vh = g.W, g.H
    cols = 3
    s = max(4, min((width // cols - 8) // vw, 420 // vh))
    imgs = [draw_frame(g, snaps[r], ours, s, view=view, ids=True, title=frame_title(snaps[r], ours))
            for r in rounds if r in snaps]
    fw, fh = imgs[0].size
    rows = (len(imgs) + cols - 1) // cols
    head = 26
    img = Image.new("RGB", (cols * (fw + 6) + 6, rows * (fh + 6) + 6 + head), (0, 0, 0))
    dr = ImageDraw.Draw(img)
    where = f" around ({at[0]},{at[1]}) r={radius}" if view else ""
    dr.text((8, 5), f"{os.path.basename(path).split('.')[0]} {g.name}: rounds {rounds[0]}-{rounds[-1]}{where}   "
                    f"cyan {decode.botname(d, ours)}, orange {decode.botname(d, 'B' if ours == 'A' else 'A')}; labels id:length", fill=TXT, font=font(14))
    for k, fr in enumerate(imgs):
        c, r = k % cols, k // cols
        img.paste(fr, (6 + c * (fw + 6), head + 6 + r * (fh + 6)))
    img.save(out)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--film", action="store_true")
    ap.add_argument("replay")
    ap.add_argument("args", nargs="+", help="<out.png> for a sheet; <round> <out.png> for --film")
    ap.add_argument("--at", nargs=2, type=int, metavar=("X", "Y"))
    ap.add_argument("--radius", type=int, default=10)
    ap.add_argument("--frames", type=int, default=9)
    ap.add_argument("--step", type=int, default=1)
    ap.add_argument("--from", dest="start", type=int, default=0, help="sheet: first round (default 0)")
    ap.add_argument("--us", choices=["A", "B"], help="seat to draw as cyan (default: our sonar signature, else A)")
    a = ap.parse_args()
    global FORCE_US
    FORCE_US = a.us
    if a.film:
        rnd, out = int(a.args[0]), a.args[1]
        print(film(a.replay, rnd, out, at=tuple(a.at) if a.at else None, radius=a.radius if a.at else None,
                   frames=a.frames, step=a.step))
    else:
        print(sheet(a.replay, a.args[0], start=a.start))


if __name__ == "__main__":
    main()

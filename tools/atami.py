#!/usr/bin/env python3
"""Concept map 6: Kirimachi as Atami mirrored left to right (as on the map with north up).
Real Atami: the bay lies to the south-east, the town climbs north-west up the slope to the railway, the castle stands on
the hill above the headland to the south-west, a breakwater juts into the bay at the port. Mirrored: the bay lies to the
south-west, the town climbs north-east to the railway, the castle hill is to the south-east, the breakwater juts west.
Streets run parallel to the shore (stepping up the slope) and steep streets climb straight up from the water.
Usage: atami.py OUTDIR"""
import sys, os, math
import numpy as np
from shapely.geometry import Polygon, LineString, Point, box
from shapely.ops import unary_union

OUT = sys.argv[1] if len(sys.argv) > 1 else "maps"
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "citymaps.py"), encoding="utf-8").read()
src = src[:src.index("if len(sys.argv) > 2")]
sys.argv = [sys.argv[0], OUT]
G = {}; exec(compile(src, "citymaps.py", "exec"), G)
City, curve, wobble, tunnel, draw_city, tidy = G["City"], G["curve"], G["wobble"], G["tunnel"], G["draw_city"], G["tidy"]

c = City("Mirrored Atami", "鏡写しの熱海", "Atami flipped: the bay to the south-west, streets stepping up the slope to the railway, the castle on its hill, a pass over the mountain.", 61)
rng = c.rng
# ---- shoreline, west to east then turning south down the castle headland (mirror of Atami's beach and the coast under the castle)
shore = [(-80, 322), (-20, 326), (40, 330), (100, 336), (150, 344), (190, 356), (222, 378), (244, 410), (262, 452), (280, 500),
         (300, 545), (318, 590), (322, 640), (318, 700)]   # the coast runs straight south under the castle hill
c.sea = Polygon([(-80, 700)] + shore + [(-80, 700)]).buffer(0)
shoreL = LineString(shore)
# the port: a breakwater from the shore out into the bay with a hooked end, and an inner basin with piers
bw = LineString([(232, 398), (180, 404), (128, 412), (98, 428), (92, 446)])

# ---- the town: from the shore up the slope to just above the railway, with the castle hill to the south-east
c.city = wobble(Polygon([(-70, 300), (-60, 150), (40, 90), (160, 50), (280, 44), (390, 70), (470, 120), (500, 200), (480, 280), (430, 340),
                          (390, 400), (360, 470), (320, 470), (280, 420), (240, 380), (180, 344), (60, 330)]), 8, rng).difference(c.sea.buffer(-1))
c.mountain = (560, 230, 150)
# a ragged inland edge: the town thins out up the slope; houses follow the streets uphill in fingers, with fields and woods between them
edge = c.city.exterior
bites, fingers = [], []
for t in np.arange(0, edge.length, 16):
    p = edge.interpolate(t)
    if c.sea.buffer(30).contains(p): continue   # the shore keeps its line
    r = rng.uniform(0, 1)
    if r < 0.45: bites.append(Point(p.x + rng.uniform(-6, 6), p.y + rng.uniform(-6, 6)).buffer(rng.uniform(9, 22), 8))
    elif r < 0.75:
        q = edge.interpolate(t + 1); dx, dy = q.y - p.y, -(q.x - p.x); L = math.hypot(dx, dy) or 1
        if c.city.contains(Point(p.x + dx / L * 5, p.y + dy / L * 5)): dx, dy = -dx, -dy   # point outward
        ln = rng.uniform(14, 34); fingers.append(LineString([(p.x, p.y), (p.x + dx / L * ln + rng.uniform(-6, 6), p.y + dy / L * ln + rng.uniform(-6, 6))]).buffer(rng.uniform(5, 9), cap_style=1))
c.city = c.city.difference(unary_union(bites)).union(unary_union(fingers)).difference(c.sea.buffer(-1))
c.city = max(c.city.geoms, key=lambda g: g.area) if c.city.geom_type == "MultiPolygon" else c.city
# the railway runs along the top of the town on the hillside, as in Atami: everything above it stays hillside, the track itself
# sits on a 12 m shelf just above the last row of houses
rail = curve([(660, 40), (540, 92), (430, 104), (330, 96), (240, 92), (140, 104), (40, 128), (-60, 140)], 160)
above = Polygon(list(rail.coords) + [(660, -200), (-60, -200)]).buffer(0)
c.city = c.city.difference(above.buffer(8))
c.city = max(c.city.geoms, key=lambda g: g.area) if c.city.geom_type == "MultiPolygon" else c.city
# ---- the street pattern, as in Atami's downtown: short straight stretches that turn at the crossings.
# Streets along the slope run 30-60 m, then step up or down the slope 8-18 m and carry on (no long parallel curves).
# Streets up the slope run straight between two cross streets, then often crank sideways 8-18 m before going on.
land_side = None
for side in ("left", "right"):
    o = shoreL.parallel_offset(30, side)
    if not c.sea.contains(o.interpolate(0.5, normalized=True)): land_side = side
dists = [0, 24, 56, 92, 130, 170, 212, 256, 300]
def at(t, r):   # point t metres along the shore, r metres inland
    p = shoreL.interpolate(t); q = shoreL.interpolate(min(t + 2, shoreL.length)); dx, dy = q.x - p.x, q.y - p.y; L = math.hypot(dx, dy) or 1
    nx, ny = -dy / L, dx / L
    if c.sea.contains(Point(p.x + nx * 20, p.y + ny * 20)): nx, ny = -nx, -ny
    return (p.x + nx * r, p.y + ny * r)
for k, r in enumerate(dists[1:], 1):   # along the slope
    t, step = 0.0, 0.0
    while t < shoreL.length - 10:
        ln = rng.uniform(30, 62); t1 = min(t + ln, shoreL.length)
        if k > 1 and rng.uniform(0, 1) < 0.45: step = 0.0 if (step != 0 and rng.uniform(0, 1) < 0.5) else rng.choice([-1, 1]) * rng.uniform(8, 16)
        pts = [at(tt, r + step) for tt in np.linspace(t, t1, 6)]
        w = 12 if k == 1 else 9 if k == 4 else 7
        c.road(LineString(pts), w, "av" if k in (1, 4) else "st")
        if step != 0 or True:   # the short link that makes the step: runs straight up/down the slope at the end of the stretch
            nxt = rng.choice([-1, 1]) * rng.uniform(8, 16) if (k > 1 and rng.uniform(0, 1) < 0.45) else step
            if abs(nxt - step) > 1: c.road(LineString([at(t1, r + step), at(t1, r + nxt)]), w, "st"); step = nxt
        t = t1
for i, t in enumerate(np.arange(20, shoreL.length - 30, 32)):   # up the slope
    shift, stairs = 0.0, i % 4 == 3
    for k in range(len(dists) - 1):
        if k > 0 and rng.uniform(0, 1) < 0.4:
            nshift = 0.0 if (shift != 0 and rng.uniform(0, 1) < 0.4) else shift + rng.choice([-1, 1]) * rng.uniform(8, 18)
            c.road(LineString([at(t + shift, dists[k]), at(t + nshift, dists[k])]), 4 if stairs else 7, "st")   # the crank, along the cross street
            shift = nshift
        c.road(LineString([at(t + shift, dists[k]), at(t + shift, dists[k + 1])]), 4 if stairs else 7, "stair" if stairs else "st")
# ---- the railway across the top of the town, the station on it, two shopping streets running down from the station square
stn = (248, 104)   # on the line at the top of the town, its square just below
c.plazas.append(Point(stn[0], stn[1] + 18).buffer(15))
c.road(curve([(stn[0] + 4, stn[1] + 30), (232, 250), (214, 300), (196, 340)]), 7, "av")    # main shopping street down to the beach
c.road(curve([(stn[0] - 14, stn[1] + 30), (214, 246), (190, 292), (166, 330)]), 5, "st")     # the second arcade
c.big.append(box(stn[0] - 20, stn[1] - 7, stn[0] + 20, stn[1] + 7))
c.stations = [stn]
# ---- a short river from the hills into the bay at the west end of the beach, a waterfront park, a park on the castle hill
riv = curve([(30, 20), (40, 100), (52, 170), (64, 240), (72, 300), (76, 336)], 100)
c.water_extra.append(riv.buffer(4))
c.parks += [Polygon([(120, 318), (176, 326), (186, 344), (116, 338)]), Point(330, 455).buffer(16)]
# ---- the castle on its hill above the coast to the south-east, shrines on the slope
c.shrines += [(334, 452), (140, 120), (300, 110), (410, 210)]
c.districts = [((150, 290), "Downtown", "銀座通り"), ((stn[0] + 75, stn[1] + 34), "Station", "霧町駅"), ((360, 170), "Hotels on the slope", "旅館街"),
               ((140, 372), "Beach", "浜辺"), ((360, 430), "Castle", "城"), ((40, 150), "River", "糸川"),
               ((585, 170), "Pass", "峠"), ((80, 520), "Bay", "湾")]
# ---- the coast road along the beach, 14 m inland of the shore, round the port and down under the castle hill into a tunnel
inl = shoreL.parallel_offset(14, land_side)
if inl.geom_type != "LineString": inl = max(inl.geoms, key=lambda g: g.length)
pts = list(inl.coords)
if Point(pts[0]).x > Point(pts[-1]).x: pts = pts[::-1]
c.highway = curve([p for p in pts if p[1] < 560] + [(330, 600), (336, 650), (334, 700)], 240)
c.ramps = []
# ---- the pass: from the top east corner of the town up the mountain in hairpins to the pass and the sealed old tunnel
c.touge = curve([(470, 170), (510, 168), (546, 156), (560, 136), (536, 124), (500, 118), (488, 102), (510, 90), (548, 84), (576, 70),
                 (580, 52), (556, 44), (530, 38), (540, 22), (572, 16)], 260)
c.oldtunnel = (LineString([(572, 16), (590, 0), (610, -30)]), 0.45)
for k in range(26):
    t = rng.uniform(0.05, 0.8); p0 = c.touge.interpolate(c.touge.length * t); p1 = c.touge.interpolate(c.touge.length * t + 1)
    dx, dy = p1.x - p0.x, p1.y - p0.y; dd = math.hypot(dx, dy) or 1; sg = rng.choice([-1, 1]); off = rng.uniform(9, 13)
    hs = Point(p0.x - dy / dd * off * sg, p0.y + dx / dd * off * sg).buffer(4.2, cap_style=3)
    if hs.distance(c.touge) > 4 and c.city.distance(hs) > 6 and all(hs.distance(h) > 1.5 for h in c.houses): c.houses.append(hs)
c.torii_path = [(582, 60), (590, 66), (596, 72)]
pre = list(c.roads)
tidy(c)
water = unary_union([c.sea] + c.water_extra); land = c.city.difference(water).difference(c.highway.buffer(8))
pieces = sorted(land.geoms, key=lambda g: g.area) if hasattr(land, "geoms") else [land]
for pc in pieces[:-1]:   # streets on the pieces the river or the coast road cut off
    if pc.area < 800: continue
    inner = pc.buffer(-5)
    for ln, w, cl in pre:
        g = ln.intersection(inner)
        for gg in (g.geoms if hasattr(g, "geoms") else [g]):
            if gg.geom_type == "LineString" and gg.length > 8: c.roads.append((gg, w, cl))
    if inner.geom_type == "Polygon": c.roads.append((LineString(inner.exterior.coords), 7, "st"))
# no ring road around the edge: streets simply run out into the hills; the turning circles tidy() put on the edge go too
ring_zone = c.city.exterior.buffer(9)
band = ring_zone.difference(c.sea.buffer(40))
kept = []
for g, w, cl in c.roads:
    if g.length > 1 and g.intersection(ring_zone).length / g.length > 0.5:   # the edge ring: keep only its stretch along the shore
        g = g.difference(band)
        for gg in (g.geoms if hasattr(g, "geoms") else [g]):
            if gg.geom_type == "LineString" and gg.length > 10: kept.append((gg, w, cl))
    else: kept.append((g, w, cl))
c.roads = kept
c.plazas = [pz for pz in c.plazas if not ring_zone.intersects(pz) or pz.distance(Point(stn)) < 30]
# where a street meets the edge, it carries on as a lane into the hills with a few houses strung along it
for g, w, cl in list(c.roads):
    for end in (0, -1):
        p = Point(g.coords[end]); q = Point(g.coords[1 if end == 0 else -2])
        if p.distance(c.city.exterior) > 6 or c.sea.buffer(30).contains(p) or rng.uniform(0, 1) < 0.35: continue
        dx, dy = p.x - q.x, p.y - q.y; L = math.hypot(dx, dy) or 1; ln = rng.uniform(18, 46)
        lane = LineString([(p.x, p.y), (p.x + dx / L * ln * 0.5 + rng.uniform(-5, 5), p.y + dy / L * ln * 0.5 + rng.uniform(-5, 5)), (p.x + dx / L * ln, p.y + dy / L * ln)])
        if lane.intersects(c.touge.buffer(6)) or c.sea.intersects(lane): continue
        c.road(lane, 5, "st")
        for k in range(int(ln / 11)):
            t = rng.uniform(6, ln); a = lane.interpolate(t); sg = rng.choice([-1, 1]); hs = Point(a.x - dy / L * 8 * sg, a.y + dx / L * 8 * sg).buffer(rng.uniform(3, 4.5), cap_style=3)
            if all(hs.distance(h) > 1.5 for h in c.houses) and not c.city.buffer(-2).intersects(hs): c.houses.append(hs)
# bridges: every street that met the river on the way across gets a bridge over it (rails, deck a shade lighter than the road)
rv = c.water_extra[0]; bridges = []
for g, w, cl in pre:
    if cl == "stair" or not g.intersects(rv): continue
    seg = g.intersection(rv.buffer(5))
    for sg in (seg.geoms if hasattr(seg, "geoms") else [seg]):
        if sg.geom_type == "LineString" and sg.length > 6 and all(sg.distance(b) > 14 for b in bridges): bridges.append(sg); c.road(sg, max(w, 6), "br")
print("bridges:", len(bridges))
# the railway runs at ground level on its own cleared strip (9 m: ballast and fences); the strip is cut out of every block
rail_in = rail.intersection(c.city.buffer(30))
c.roads.append((rail_in, 9, "rail"))
crossings = []
for g, w, cl in c.roads:
    if cl in ("rail", "br") or g.length < 1: continue
    x = g.intersection(rail_in)
    for p in ([x] if x.geom_type == "Point" else list(getattr(x, "geoms", []))):
        if p.geom_type == "Point" and p.distance(Point(stn)) > 22 and all(p.distance(q) > 10 for q, _ in crossings): crossings.append((p, cl == "stair"))
print("level crossings:", sum(1 for _, st in crossings if not st), "footbridges:", sum(1 for _, st in crossings if st))
if "LOTS_HOOK" in globals(): G["lots"] = LOTS_HOOK(G["lots"])   # tools/export_atami.py records every lot drawn
im = draw_city(c, 6)
from PIL import ImageDraw
d = ImageDraw.Draw(im, "RGBA"); K = G["K"]; OY = 170
# portals: the line comes out of the mountain just past each end of the town; beyond them it runs in tunnels (dashed, faint)
ins = [t for t in np.arange(0, rail.length, 1.0) if c.city.buffer(18).contains(rail.interpolate(t))]
tA, tB = (min(ins), max(ins)) if ins else (0, rail.length)
tA = max(tA, next(t for t in np.arange(0, rail.length, 1.0) if rail.interpolate(t).x < 448))   # east portal in the hillside, short of the pass road
for t in np.arange(0, rail.length, 6):
    if tA <= t <= tB: continue
    p0, p1 = rail.interpolate(t), rail.interpolate(min(t + 3, rail.length))
    if 0 <= p0.x <= 600 and 0 <= p0.y <= 600: d.line([(p0.x * K, p0.y * K + OY), (p1.x * K, p1.y * K + OY)], fill=(60, 66, 60, 150), width=int(2.5 * K))
for tp in (tA, tB):
    p = rail.interpolate(tp); q = rail.interpolate(min(tp + 1, rail.length)); dx, dy = q.x - p.x, q.y - p.y; L = math.hypot(dx, dy) or 1; nx, ny = -dy / L, dx / L
    if not (0 <= p.x <= 600 and 0 <= p.y <= 600): continue
    d.line([((p.x - nx * 7) * K, (p.y - ny * 7) * K + OY), ((p.x + nx * 7) * K, (p.y + ny * 7) * K + OY)], fill=(225, 225, 215), width=int(2.6 * K))
    d.line([((p.x - nx * 3) * K, (p.y - ny * 3) * K + OY), ((p.x + nx * 3) * K, (p.y + ny * 3) * K + OY)], fill=(20, 20, 22), width=int(1.6 * K))
for t in np.arange(tA, tB, 3):   # the railway: gravel bed on its strip, sleepers, two rails, a fence each side in town
    p0, p1 = rail.interpolate(t), rail.interpolate(min(t + 3, rail.length))
    if not (0 <= p0.x <= 600 and 0 <= p0.y <= 600 and 0 <= p1.x <= 600 and 0 <= p1.y <= 600): continue
    town = True; a = 255
    dx, dy = p1.x - p0.x, p1.y - p0.y; L = math.hypot(dx, dy) or 1; nx, ny = -dy / L, dx / L
    if town:
        d.line([(p0.x * K, p0.y * K + OY), (p1.x * K, p1.y * K + OY)], fill=(150, 144, 132), width=int(9 * K))
        for o2 in (-4.4, 4.4): d.line([((p0.x + nx * o2) * K, (p0.y + ny * o2) * K + OY), ((p1.x + nx * o2) * K, (p1.y + ny * o2) * K + OY)], fill=(90, 96, 100), width=2)
    d.line([(p0.x * K, p0.y * K + OY), (p1.x * K, p1.y * K + OY)], fill=(118, 110, 100, a), width=int(4 * K))
    m = ((p0.x + p1.x) / 2, (p0.y + p1.y) / 2); d.line([((m[0] - nx * 1.9) * K, (m[1] - ny * 1.9) * K + OY), ((m[0] + nx * 1.9) * K, (m[1] + ny * 1.9) * K + OY)], fill=(70, 52, 40, a), width=3)
    for o2 in (-0.8, 0.8): d.line([((p0.x + nx * o2) * K, (p0.y + ny * o2) * K + OY), ((p1.x + nx * o2) * K, (p1.y + ny * o2) * K + OY)], fill=(210, 210, 215, a), width=2)
for p, foot in crossings:   # level crossings (road over the rails, yellow-and-black barriers) and footbridges for the stair streets
    t = rail.project(p); q0, q1 = rail.interpolate(t - 1), rail.interpolate(t + 1); dx, dy = q1.x - q0.x, q1.y - q0.y; L = math.hypot(dx, dy) or 1; ux, uy, nx, ny = dx / L, dy / L, -dy / L, dx / L
    if foot:
        d.line([((p.x - nx * 6) * K, (p.y - ny * 6) * K + OY), ((p.x + nx * 6) * K, (p.y + ny * 6) * K + OY)], fill=(30, 30, 32), width=int(3.2 * K))
        d.line([((p.x - nx * 6) * K, (p.y - ny * 6) * K + OY), ((p.x + nx * 6) * K, (p.y + ny * 6) * K + OY)], fill=(190, 190, 186), width=int(2.4 * K)); continue
    poly = [((p.x + ux * s1 * 3.6 + nx * s2 * 4.6) * K, (p.y + uy * s1 * 3.6 + ny * s2 * 4.6) * K + OY) for s1, s2 in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    d.polygon(poly, fill=(78, 80, 84))
    for o2 in (-0.8, 0.8): d.line([((p.x + nx * o2 - ux * 3.6) * K, (p.y + ny * o2 - uy * 3.6) * K + OY), ((p.x + nx * o2 + ux * 3.6) * K, (p.y + ny * o2 + uy * 3.6) * K + OY)], fill=(210, 210, 215), width=2)
    for sg in (-1, 1):
        bx, by = p.x + nx * sg * 5.4, p.y + ny * sg * 5.4
        for k2 in range(6):
            a0, a1 = -3.4 + k2 * 1.14, -3.4 + (k2 + 1) * 1.14
            d.line([((bx + ux * a0) * K, (by + uy * a0) * K + OY), ((bx + ux * a1) * K, (by + uy * a1) * K + OY)], fill=(240, 200, 40) if k2 % 2 == 0 else (25, 25, 25), width=int(0.8 * K))
lx, ly = 40, OY + 600 * K - 130   # legend entry for the railway, above the scale bar
d.rounded_rectangle([lx - 8, ly - 10, lx + 150, ly + 26], 6, fill=(16, 20, 22, 210))
d.line([(lx, ly + 8), (lx + 40, ly + 8)], fill=(120, 116, 108), width=10); d.line([(lx, ly + 6), (lx + 40, ly + 6)], fill=(210, 210, 215), width=2); d.line([(lx, ly + 10), (lx + 40, ly + 10)], fill=(210, 210, 215), width=2)
d.text((lx + 50, ly - 4), "Railway", font=G["font"](20, True), fill=(236, 230, 216))
os.makedirs(OUT, exist_ok=True)
p = os.path.join(OUT, "layout_06_atami.png"); im.save(p, optimize=True); print("wrote", p, "| coast road over water (m):", round(c.highway.intersection(c.sea).length, 1))

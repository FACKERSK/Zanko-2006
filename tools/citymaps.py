#!/usr/bin/env python3
"""Zankō 2006 — 10 concept city layouts with a bay, hills, exit tunnels (collapsed), subway.
World is 600 x 600 m. Output: one PNG per concept + two contact sheets."""
import math, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from shapely.geometry import Polygon, LineString, Point, MultiPolygon, box
from shapely.ops import unary_union, split
import shapely, shapely.affinity

OUT = sys.argv[1] if len(sys.argv) > 1 else "maps"
os.makedirs(OUT, exist_ok=True)
W = 600.0; PX = 1400; K = PX / W            # render scale (px per m)
FONT = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
FONTB = "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc" if os.path.exists("/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc") else FONT
def font(sz, bold=False): return ImageFont.truetype(FONTB if bold else FONT, int(sz))

# ---------------------------------------------------------------- noise
def vnoise(shape, cell, rng):
    h, w = shape; gh, gw = h // cell + 2, w // cell + 2
    g = rng.random((gh, gw))
    y = np.arange(h) / cell; x = np.arange(w) / cell
    y0 = y.astype(int); x0 = x.astype(int); fy = y - y0; fx = x - x0
    fy = fy * fy * (3 - 2 * fy); fx = fx * fx * (3 - 2 * fx)
    a = g[y0][:, x0]; b = g[y0][:, x0 + 1]; c = g[y0 + 1][:, x0]; d = g[y0 + 1][:, x0 + 1]
    return (a * (1 - fx) + b * fx) * (1 - fy[:, None]) + (c * (1 - fx) + d * fx) * fy[:, None]
def fbm(shape, rng):
    return 0.55 * vnoise(shape, 60, rng) + 0.3 * vnoise(shape, 24, rng) + 0.15 * vnoise(shape, 9, rng)

# ---------------------------------------------------------------- geometry helpers
def ell(cx, cy, rx, ry, n=90, rot=0, jit=0, rng=None):
    pts = []
    for i in range(n):
        a = 2 * math.pi * i / n; r = 1 + (rng.uniform(-jit, jit) if rng is not None and jit else 0)
        x, y = rx * r * math.cos(a), ry * r * math.sin(a)
        pts.append((cx + x * math.cos(rot) - y * math.sin(rot), cy + x * math.sin(rot) + y * math.cos(rot)))
    return Polygon(pts)
def wobble(poly, amp, rng, n=160):
    ext = poly.exterior; L = ext.length; pts = []
    ph = rng.uniform(0, 6, 3)
    for i in range(n):
        p = ext.interpolate(L * i / n); t = 2 * math.pi * i / n
        r = amp * (0.6 * math.sin(3 * t + ph[0]) + 0.3 * math.sin(7 * t + ph[1]) + 0.2 * math.sin(13 * t + ph[2]))
        c = poly.centroid; dx, dy = p.x - c.x, p.y - c.y; d = math.hypot(dx, dy) or 1
        pts.append((p.x + dx / d * r, p.y + dy / d * r))
    return Polygon(pts).buffer(0)
def grid_lines(cx, cy, ang, sx, sy, ext=400):
    out = []; ca, sa = math.cos(ang), math.sin(ang)
    for k in range(-int(ext / sx), int(ext / sx) + 1):
        o = k * sx; a = (cx + o * ca - ext * -sa, cy + o * sa - ext * ca); b = (cx + o * ca + ext * -sa, cy + o * sa + ext * ca)
        out.append(LineString([a, b]))
    for k in range(-int(ext / sy), int(ext / sy) + 1):
        o = k * sy; a = (cx - ext * ca + o * -sa, cy - ext * sa + o * ca); b = (cx + ext * ca + o * -sa, cy + ext * sa + o * ca)
        out.append(LineString([a, b]))
    return out
def arc(cx, cy, r, a0, a1, n=60): return LineString([(cx + r * math.cos(a0 + (a1 - a0) * i / n), cy + r * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)])
def ray(cx, cy, a, r0, r1): return LineString([(cx + r0 * math.cos(a), cy + r0 * math.sin(a)), (cx + r1 * math.cos(a), cy + r1 * math.sin(a))])
def curve(pts, n=80):   # Catmull-Rom through points
    P = [pts[0]] + list(pts) + [pts[-1]]; out = []
    for i in range(1, len(P) - 2):
        p0, p1, p2, p3 = map(np.array, (P[i - 1], P[i], P[i + 1], P[i + 2]))
        for t in np.linspace(0, 1, max(2, n // (len(P) - 3)), endpoint=False):
            out.append(tuple(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3)))
    out.append(pts[-1]); return LineString(out)

# ---------------------------------------------------------------- city spec
class City:
    def __init__(s, title, jp, desc, seed):
        s.title, s.jp, s.desc = title, jp, desc; s.rng = np.random.default_rng(seed)
        s.sea = Polygon(); s.city = Polygon(); s.roads = []; s.water_extra = []; s.parks = []; s.districts = []
        s.plazas = []; s.junctions = []; s.highway = None; s.ramps = []; s.mountain = None; s.touge = None; s.houses = []; s.oldtunnel = None; s.torii_path = []; s.tunnels = []; s.subway = []; s.stations = []; s.labels = []; s.piers = []; s.big = []; s.shrines = []
    def road(s, line, w=9, cls="st"): s.roads.append((line, w, cls))

def lots(block, rng, target):
    """recursive split along the long axis of the min rotated rectangle"""
    out = []; stack = [block]
    while stack:
        p = stack.pop()
        if p.is_empty or p.area < 25: continue
        if p.area < target * rng.uniform(0.7, 1.4): out.append(p); continue
        r = p.minimum_rotated_rectangle; c = list(r.exterior.coords)[:4]
        e1 = np.subtract(c[1], c[0]); e2 = np.subtract(c[2], c[1])
        long_e, base = (e1, c[0]) if np.linalg.norm(e1) > np.linalg.norm(e2) else (e2, c[1])
        other = e2 if long_e is e1 else e1
        t = rng.uniform(0.38, 0.62); m = np.add(base, long_e * t)
        d = other / (np.linalg.norm(other) or 1) * 2000
        cut = LineString([tuple(m - d), tuple(m + d)])
        try: parts = split(p, cut)
        except Exception: out.append(p); continue
        if len(parts.geoms) < 2: out.append(p); continue
        stack.extend([g for g in parts.geoms if g.geom_type == "Polygon"])
    return out

# ---------------------------------------------------------------- the ten concepts
def tunnel(c, pts, frac):  # pts: from the city edge outward; road section until the portal, tunnel after
    from shapely.ops import nearest_points
    p0 = Point(pts[0])
    if c.city.distance(p0) > 1:   # always start at the town's edge
        q = nearest_points(c.city, p0)[0]; pts = [(q.x, q.y)] + list(pts)
    c.tunnels.append((LineString(pts), frac))

def c1():
    c = City("Harbor Grid", "港町", "Rotated grid squared to the waterfront, a boulevard on the quay, piers into the bay.", 1)
    bay = ell(300, 520, 190, 120, rng=c.rng, jit=0.05); c.sea = unary_union([bay, box(0, 560, 600, 600)])
    c.city = wobble(ell(300, 330, 230, 190), 18, c.rng).difference(c.sea.buffer(-2))
    for L in grid_lines(300, 330, math.radians(8), 46, 40): c.road(L, 9)
    c.road(curve([(90, 430), (180, 395), (300, 392), (420, 395), (510, 430)]), 16, "av")
    c.road(LineString([(300, 120), (300, 400)]), 14, "av")
    for x in range(160, 460, 55): c.piers.append(box(x, 405, x + 10, 450))
    c.districts = [((300, 210), "Station", "駅前"), ((180, 300), "Residential", "住宅地"), ((430, 300), "Shopping arcade", "商店街"), ((300, 375), "Harbor", "港")]
    c.stations = [(300, 200), (170, 330), (430, 330), (300, 380)]
    tunnel(c, [(300, 150), (300, 60), (300, 0)], 0.55); tunnel(c, [(85, 300), (20, 290), (0, 285)], 0.6); tunnel(c, [(520, 300), (580, 310), (600, 312)], 0.5)
    c.parks.append(ell(220, 230, 30, 22)); c.shrines.append((420, 190))
    return c

def c2():
    c = City("Station Radial", "放射状の駅前", "Avenues fan out from the central station; two ring roads; the bay bites in from the east.", 2)
    c.sea = unary_union([ell(560, 330, 150, 110, rng=c.rng, jit=0.06), box(560, 0, 600, 600)])
    c.city = wobble(Point(280, 300).buffer(200), 20, c.rng).difference(c.sea.buffer(-2))
    for i in range(10): c.road(ray(280, 300, i * math.pi / 5 + 0.15, 18, 260), 13 if i % 2 == 0 else 9, "av" if i % 2 == 0 else "st")
    for r in (55, 110, 165): c.road(Point(280, 300).buffer(r).exterior, 10 if r != 110 else 14, "av" if r == 110 else "st")
    for i in range(30): a = i * 2 * math.pi / 30; c.road(ray(280, 300, a, 112, 163), 7)
    c.districts = [((280, 300), "Central station", "中央駅"), ((200, 200), "Offices", "オフィス街"), ((200, 400), "Residential", "住宅地"), ((390, 230), "Market", "市場"), ((420, 380), "Waterfront", "海岸通り")]
    c.stations = [(280, 300), (280, 190), (170, 300), (280, 410), (390, 300)]
    tunnel(c, [(130, 150), (60, 80), (0, 30)], 0.5); tunnel(c, [(120, 430), (50, 520), (20, 600)], 0.65); tunnel(c, [(330, 110), (360, 40), (370, 0)], 0.45)
    c.parks.append(Point(280, 300).buffer(30).difference(Point(280, 300).buffer(18)))
    return c

def c3():
    c = City("Valley Spine", "谷の町", "One long main street climbs a narrow valley from the bay; side streets branch off like ribs.", 3)
    c.sea = unary_union([ell(110, 520, 170, 130, rng=c.rng, jit=0.06), box(0, 540, 300, 600)])
    spine = curve([(150, 470), (230, 380), (300, 290), (360, 200), (430, 110)])
    c.city = wobble(spine.buffer(105), 22, c.rng).difference(c.sea.buffer(-2))
    c.road(spine, 15, "av")
    L = spine.length
    for k in range(1, 18):
        p = spine.interpolate(L * k / 18); q = spine.interpolate(min(L, L * k / 18 + 1))
        dx, dy = q.x - p.x, q.y - p.y; d = math.hypot(dx, dy); nx, ny = -dy / d, dx / d
        for sgn in (-1, 1):
            ln = c.rng.uniform(55, 100); bend = c.rng.uniform(-0.3, 0.3)
            c.road(curve([(p.x, p.y), (p.x + sgn * nx * ln * 0.5 + dx / d * ln * bend, p.y + sgn * ny * ln * 0.5 + dy / d * ln * bend), (p.x + sgn * nx * ln, p.y + sgn * ny * ln)]), 8)
    c.road(spine.parallel_offset(60, "left"), 8); c.road(spine.parallel_offset(60, "right"), 8)
    c.districts = [((205, 420), "Fishing port", "漁港"), ((300, 290), "Main street", "本通り"), ((380, 190), "Temple quarter", "寺町"), ((240, 300), "Residential", "住宅地")]
    c.stations = [(200, 420), (290, 300), (370, 190)]; c.shrines += [(400, 150), (330, 240)]
    tunnel(c, [(430, 110), (480, 50), (500, 0)], 0.55); tunnel(c, [(330, 330), (430, 380), (600, 420)], 0.6); tunnel(c, [(230, 260), (130, 230), (0, 200)], 0.5)
    for x in range(150, 250, 30): c.piers.append(box(x, 470, x + 8, 500))
    return c

def c4():
    c = City("River Delta", "川の町", "A river cuts the city in two before it meets the bay; two grids at different angles, joined by bridges.", 4)
    river = curve([(320, 0), (300, 120), (330, 250), (290, 380), (300, 470)]).buffer(16)
    c.sea = unary_union([ell(300, 560, 230, 110, rng=c.rng, jit=0.05), box(0, 580, 600, 600)])
    c.water_extra.append(river)
    c.city = wobble(ell(300, 320, 240, 200), 18, c.rng).difference(c.sea.buffer(-2))
    west = c.city.intersection(box(0, 0, 300, 600)); east = c.city.intersection(box(310, 0, 600, 600))
    for L in grid_lines(200, 300, math.radians(-5), 42, 42): c.road(L.intersection(west.buffer(5)), 9)
    for L in grid_lines(420, 300, math.radians(14), 38, 46): c.road(L.intersection(east.buffer(5)), 9)
    for y in (160, 260, 360): c.road(LineString([(200, y), (420, y + 15)]), 13, "av")
    c.districts = [((190, 230), "Old town", "旧市街"), ((420, 230), "New town", "新市街"), ((200, 400), "Docks", "船着場"), ((430, 410), "Market", "市場")]
    c.stations = [(190, 250), (420, 260), (310, 430)]
    tunnel(c, [(110, 250), (40, 230), (0, 220)], 0.55); tunnel(c, [(500, 220), (560, 190), (600, 180)], 0.6); tunnel(c, [(420, 140), (450, 60), (460, 0)], 0.5)
    return c

def c5():
    c = City("Old Town + New Grid", "城下町と新市街", "Twisting old lanes around a castle hill by the bay; a planned grid district inland.", 5)
    c.sea = unary_union([ell(150, 470, 170, 140, rng=c.rng, jit=0.07), box(0, 520, 260, 600), box(0, 380, 40, 600)])
    c.city = wobble(ell(310, 300, 240, 200), 20, c.rng).difference(c.sea.buffer(-2))
    old = Point(230, 360).buffer(110).intersection(c.city)
    for i in range(26):
        x, y = 230 + c.rng.uniform(-90, 90), 360 + c.rng.uniform(-90, 90); pts = [(x, y)]; a = c.rng.uniform(0, 6.28)
        for k in range(5): a += c.rng.uniform(-0.7, 0.7); x += math.cos(a) * 22; y += math.sin(a) * 22; pts.append((x, y))
        c.road(curve(pts), 6)
    for L in grid_lines(380, 230, math.radians(0), 36, 36): c.road(L.difference(old.buffer(10)), 10)
    c.road(curve([(110, 300), (230, 250), (380, 230), (520, 230)]), 15, "av")
    c.parks.append(Point(235, 365).buffer(28)); c.shrines += [(235, 365), (180, 330)]
    c.districts = [((230, 380), "Castle old town", "城下町"), ((400, 180), "New grid", "新市街"), ((430, 330), "Hospital district", "病院"), ((300, 440), "Fish market", "魚市場")]
    c.stations = [(230, 300), (380, 230), (470, 320)]
    tunnel(c, [(400, 110), (420, 40), (430, 0)], 0.55); tunnel(c, [(540, 250), (600, 260)], 0.6); tunnel(c, [(130, 220), (60, 160), (0, 130)], 0.5)
    return c

def c6():
    c = City("Crescent Shore", "三日月の浜", "Streets curve in arcs around a round bay; straight streets run downhill to the water.", 6)
    c.sea = unary_union([Point(300, 590).buffer(185), box(0, 590, 600, 600)])
    c.city = wobble(Point(300, 590).buffer(345), 14, c.rng).difference(c.sea.buffer(-2)).intersection(box(40, 0, 560, 600))
    for r in (205, 245, 290, 335): c.road(arc(300, 590, r, math.pi * 1.05, math.pi * 1.95), 14 if r == 205 else 9, "av" if r == 205 else "st")
    for i in range(19): a = math.pi * (1.05 + 0.9 * i / 18); c.road(ray(300, 590, a, 190, 355), 9 if i % 3 else 12, "st" if i % 3 else "av")
    for i in range(7): a = math.pi * (1.15 + 0.7 * i / 6); p0 = (300 + 186 * math.cos(a), 590 + 186 * math.sin(a)); c.piers.append(LineString([p0, (300 + 150 * math.cos(a), 590 + 150 * math.sin(a))]).buffer(5))
    c.districts = [((300, 375), "Promenade", "海岸通り"), ((300, 285), "Station", "駅前"), ((150, 420), "West hills", "西の丘"), ((450, 420), "Hotels", "ホテル街")]
    c.stations = [(300, 290), (170, 380), (430, 380)]
    tunnel(c, [(300, 255), (300, 120), (300, 0)], 0.5); tunnel(c, [(80, 330), (20, 250), (0, 220)], 0.6); tunnel(c, [(520, 330), (580, 250), (600, 220)], 0.55)
    return c

def c7():
    c = City("Headland", "岬の町", "The city sits on a headland between two bays; a ridge road runs out to the lighthouse.", 7)
    land = Polygon([(-80, -80), (680, -80), (680, 280), (430, 300), (380, 400), (350, 560), (250, 560), (220, 400), (170, 300), (-80, 280)])
    c.sea = box(0, 0, 600, 600).difference(wobble(land, 10, c.rng))
    c.city = wobble(Polygon([(160, 200), (440, 200), (420, 300), (370, 420), (340, 540), (260, 540), (230, 420), (180, 300)]), 10, c.rng).difference(c.sea.buffer(-2))
    ridge = curve([(300, 150), (300, 300), (300, 450), (300, 540)]); c.road(ridge, 14, "av")
    for y in range(215, 540, 32): c.road(LineString([(140, y), (460, y)]), 8)
    c.road(curve([(170, 260), (230, 380), (270, 520)]), 8); c.road(curve([(430, 260), (370, 380), (330, 520)]), 8)
    c.districts = [((300, 240), "Station", "駅前"), ((230, 330), "West harbor", "西港"), ((370, 330), "East harbor", "東港"), ((300, 500), "Lighthouse", "灯台")]
    c.stations = [(300, 230), (300, 340), (300, 450)]; c.shrines.append((300, 525))
    for y in range(300, 420, 30): c.piers += [box(175, y, 215, y + 7), box(385, y, 425, y + 7)]
    tunnel(c, [(300, 200), (300, 90), (300, 0)], 0.5); tunnel(c, [(170, 220), (80, 180), (0, 170)], 0.55); tunnel(c, [(430, 220), (520, 180), (600, 170)], 0.6)
    return c

def c8():
    c = City("Island Downtown", "島の中心街", "Downtown stands on an island in the bay, joined to the mainland ring by two bridges.", 8)
    c.sea = unary_union([ell(300, 360, 175, 150, rng=c.rng, jit=0.04), box(220, 470, 380, 600)])
    island = wobble(ell(300, 340, 85, 70), 6, c.rng)
    c.sea = c.sea.difference(island)
    c.city = unary_union([wobble(ell(300, 330, 270, 230), 16, c.rng).difference(c.sea.buffer(-2))])
    for L in grid_lines(300, 340, math.radians(20), 30, 30): c.road(L.intersection(island.buffer(-6)), 8)
    c.road(Point(300, 360).buffer(205).exterior, 15, "av")
    for i in range(24): a = i * 2 * math.pi / 24; c.road(ray(300, 360, a, 190, 300), 9)
    c.road(LineString([(140, 330), (230, 335)]), 12, "br"); c.road(LineString([(370, 345), (460, 350)]), 12, "br")
    c.districts = [((300, 340), "Downtown island", "中心街"), ((300, 140), "North shore", "北岸"), ((120, 360), "West shore", "西岸"), ((480, 360), "Industrial", "工業地帯")]
    c.stations = [(300, 340), (300, 150), (110, 370), (490, 370)]
    tunnel(c, [(300, 120), (300, 40), (300, 0)], 0.5); tunnel(c, [(80, 260), (20, 220), (0, 210)], 0.6); tunnel(c, [(520, 260), (580, 220), (600, 210)], 0.55)
    return c

def c9():
    c = City("Amphitheater", "段々の町", "Houses climb the hills around the bay in terraces; streets follow the contours, stairs link them.", 9)
    c.sea = unary_union([ell(300, 470, 200, 150, rng=c.rng, jit=0.05), box(140, 520, 460, 600)])
    c.city = wobble(ell(300, 430, 290, 250), 14, c.rng).difference(c.sea.buffer(-2)).intersection(box(30, 0, 570, 600))
    for k, r in enumerate((215, 250, 285, 320, 355)):
        pts = [(300 + r * math.cos(a) * (1 + 0.04 * math.sin(a * 5 + k)), 470 + r * 0.82 * math.sin(a)) for a in np.linspace(math.pi * 1.0, math.pi * 2.0, 50)]
        c.road(LineString(pts), 13 if k == 0 else 8, "av" if k == 0 else "st")
    for i in range(14):
        a = math.pi * (1.06 + 0.88 * i / 13); pts = []
        for k, r in enumerate(np.linspace(205, 360, 7)): pts.append((300 + r * math.cos(a + 0.05 * (-1) ** k), 470 + r * 0.82 * math.sin(a + 0.05 * (-1) ** k)))
        c.road(LineString(pts), 4 if i % 2 else 7, "stair" if i % 2 else "st")
    c.districts = [((300, 285), "Upper town", "上町"), ((300, 330), "Bay road", "湾岸道路"), ((130, 420), "Onsen", "温泉街"), ((470, 420), "Shipyard", "造船所")]
    c.stations = [(300, 300), (140, 420), (460, 420)]; c.shrines += [(300, 140), (90, 330)]
    tunnel(c, [(300, 130), (300, 0)], 0.55); tunnel(c, [(40, 330), (0, 250)], 0.5); tunnel(c, [(560, 330), (600, 250)], 0.6)
    return c

def c10():
    c = City("Canal Port", "運河の港", "A working port: warehouses, container piers, and canals that cut the grid into islands.", 10)
    c.sea = unary_union([ell(450, 480, 200, 150, rng=c.rng, jit=0.05), box(300, 560, 600, 600), box(560, 300, 600, 600)])
    c.city = wobble(ell(290, 300, 250, 210), 16, c.rng).difference(c.sea.buffer(-2))
    for x in (220, 340): c.water_extra.append(LineString([(x, 120), (x + 10, 470)]).buffer(9))
    c.water_extra.append(LineString([(80, 300), (450, 330)]).buffer(8))
    for L in grid_lines(290, 300, math.radians(4), 40, 34): c.road(L, 9)
    for y in range(380, 470, 24): c.piers.append(box(400, y, 480, y + 12))
    c.big += [box(380, 330, 450, 370), box(300, 380, 360, 430), box(420, 260, 480, 300)]
    c.districts = [((160, 210), "Residential", "住宅地"), ((290, 220), "Canal quarter", "運河地区"), ((420, 360), "Container port", "コンテナ港"), ((170, 380), "Fish market", "魚市場")]
    c.stations = [(170, 220), (290, 230), (300, 360)]
    tunnel(c, [(110, 170), (40, 110), (0, 90)], 0.55); tunnel(c, [(290, 110), (280, 40), (275, 0)], 0.5); tunnel(c, [(100, 420), (40, 480), (0, 500)], 0.6)
    return c

def c5b():
    c = c5(); c.title = "Old Town, Coast Road, Pass"; c.jp = "城下町・海岸通り・峠"
    c.desc = "A two-lane sea road with a seawall wraps the old town into two tunnels; a hairpin pass climbs to a sealed tunnel."
    c.shrines = [(300, 345), (250, 320)]
    c.tunnels = [t for t in c.tunnels if t[0].coords[-1][1] < 60]   # keep the north road tunnel; the highway takes west and east
    pts = [(0, 160), (14, 230), (30, 290), (55, 322)]
    for a in np.linspace(math.radians(250), math.radians(345), 11):   # hug the bay shore, 16 m inland of the water line
        pts.append((150 + 186 * math.cos(a), 470 + 156 * math.sin(a)))
    pts += [(352, 455), (372, 500), (420, 545), (520, 575), (600, 585)]
    c.highway = curve(pts, 160)
    c.ramps = []   # ground-level coastal road: streets meet it at ordinary crossings
    # the east third becomes a mountain: hamlet, shrines, a hairpin pass and an old sealed tunnel at the top
    c.city = c.city.intersection(wobble(box(-50, -50, 455, 650), 16, c.rng))
    c.mountain = (540, 230, 120)
    c.touge = curve([(452, 232), (500, 236), (545, 240), (570, 230), (566, 214), (530, 208), (486, 204), (474, 190), (488, 178), (532, 174),
                     (566, 168), (572, 154), (552, 144), (518, 140), (508, 128)], 260)
    c.ramps.append(curve([(500, 236), (510, 255), (518, 275), (520, 300)]))   # lane down to the hamlet
    c.oldtunnel = (LineString([(510, 128), (512, 90), (520, 40), (526, 0)]), 0.45)
    for k in range(34):   # old wooden houses along the pass and in a hamlet on the lower slope
        t = c.rng.uniform(0.02, 0.7) if k < 22 else None
        if t is not None:
            p0 = c.touge.interpolate(c.touge.length * t); p1 = c.touge.interpolate(c.touge.length * t + 1)
            dx, dy = p1.x - p0.x, p1.y - p0.y; dd = math.hypot(dx, dy) or 1; sgn = c.rng.choice([-1, 1]); off = c.rng.uniform(9, 14)
            cx_, cy_ = p0.x - dy / dd * off * sgn, p0.y + dx / dd * off * sgn; ang = math.atan2(dy, dx)
        else:
            cx_, cy_ = 522 + c.rng.uniform(-28, 28), 292 + c.rng.uniform(-14, 14); ang = c.rng.uniform(-0.3, 0.3)
        w_, h_ = c.rng.uniform(8, 11), c.rng.uniform(6, 8)
        rect = Polygon([(-w_ / 2, -h_ / 2), (w_ / 2, -h_ / 2), (w_ / 2, h_ / 2), (-w_ / 2, h_ / 2)])
        rect = shapely.affinity.translate(shapely.affinity.rotate(rect, ang, use_radians=True), cx_, cy_)
        if rect.distance(c.touge) > 4.5 and rect.distance(c.ramps[-1]) > 4 and c.city.distance(rect) > 4 and all(rect.distance(h) > 1.5 for h in c.houses): c.houses.append(rect)
    c.shrines = [(300, 345), (250, 320), (560, 300), (590, 190), (500, 312)]
    c.stations = [(230, 300), (380, 230), (420, 320)]
    c.torii_path = [(572, 168), (580, 174), (586, 180), (590, 186)]   # a row of torii up to the mountain shrine
    c.districts = [((290, 300), "Castle old town", "城下町"), ((522, 318), "Mountain hamlet", "山の集落"), ((540, 196), "Touge pass", "峠道"), ((545, 112), "Old tunnel (sealed)", "旧トンネル"), ((320, 185), "New grid", "新市街"), ((385, 335), "Hospital district", "病院"),
                   ((318, 415), "Fish market", "魚市場"), ((470, 528), "Coast road", "海岸通り"), ((150, 420), "Bay", "湾")]
    c.plazas.append(Point(520, 300).buffer(9))          # the hamlet lane ends in a small square
    tidy(c)
    return c

def tidy(c):
    """Every street ends somewhere: on another street, on the ring road along the edge, or in a turning circle."""
    from shapely.ops import nearest_points
    water = unary_union([c.sea] + c.water_extra)
    land = c.city.difference(water)
    if c.highway is not None: land = land.difference(c.highway.buffer(8))
    core = max(land.geoms, key=lambda g: g.area) if hasattr(land, "geoms") else land
    inner = core.buffer(-7); inner = max(inner.geoms, key=lambda g: g.area) if hasattr(inner, "geoms") else inner
    ring = LineString(inner.exterior.coords).simplify(1.5)
    if c.highway is not None:
        rg = ring.difference(c.highway.buffer(24)); rl = [g for g in (rg.geoms if hasattr(rg, "geoms") else [rg]) if g.length > 20]
        ring = unary_union(rl + [c.highway.intersection(c.city.buffer(30))])
    parts = []
    for ln, w, cl in c.roads:
        if ln.is_empty: continue
        clipA = inner.buffer(0.5).union(c.highway.buffer(16).intersection(core.buffer(9))) if c.highway is not None else inner.buffer(0.5)
        g = ln.intersection(clipA)
        for gg in (g.geoms if hasattr(g, "geoms") else [g]):
            if gg.geom_type == "LineString" and gg.length > 10: parts.append([gg, w, cl])
    for it in range(4):   # join loose ends to the nearest street (short links only); otherwise give them a turning circle
        changed = False
        for i, pr in enumerate(parts):
            others = unary_union([ring] + [q[0] for j, q in enumerate(parts) if j != i])
            co = list(pr[0].coords)
            for end in (0, -1):
                pt = Point(co[end])
                if others.distance(pt) < 1.0: continue
                q = nearest_points(others, pt)[0]
                if pt.distance(q) < 40: co = ([(q.x, q.y)] + co) if end == 0 else (co + [(q.x, q.y)]); changed = True
            pr[0] = LineString(co)
        if not changed: break
    allnet = unary_union([ring] + [q[0] for q in parts])
    for i, pr in enumerate(parts):
        others = unary_union([ring] + [q[0] for j, q in enumerate(parts) if j != i])
        for end in (0, -1):
            pt = Point(pr[0].coords[end])
            if others.distance(pt) > 1.0: c.plazas.append(pt.buffer(pr[1] * 0.8))
    def link(line, at_start=True):   # pull a road's start (or end) onto the network
        co = list(line.coords); pt = Point(co[0] if at_start else co[-1]); q = nearest_points(allnet, pt)[0]
        if pt.distance(q) < 25: co = ([(q.x, q.y)] + co) if at_start else (co + [(q.x, q.y)])
        return LineString(co)
    if c.touge is not None: c.touge = link(c.touge, True)
    c.ramps = [link(link(rp, True), False) if k < len(c.ramps) - 1 else link(rp, True) for k, rp in enumerate(c.ramps)]
    rparts = [g for g in (ring.geoms if hasattr(ring, "geoms") else [ring]) if c.highway is None or g.hausdorff_distance(c.highway.intersection(c.city.buffer(30))) > 1]
    c.roads = [(g, 10, "st") for g in rparts] + [(q[0], q[1], q[2]) for q in parts]
    c.junctions = [Point(q[0].coords[e]) for q in parts for e in (0, -1)] if c.highway is not None else []
CONCEPTS = [c1, c2, c3, c4, c5, c6, c7, c8, c9, c10]

# ---------------------------------------------------------------- render
DIST_COL = {"Station": (205, 210, 220), "Central station": (205, 210, 220), "Offices": (190, 198, 210), "Shopping arcade": (218, 200, 190), "Market": (218, 200, 190),
            "Harbor": (190, 185, 170), "Industrial": (175, 165, 150), "Container port": (175, 165, 150), "Shipyard": (175, 165, 150), "Docks": (185, 178, 165)}
def draw_city(c, idx):
    rng = c.rng; N = 300
    xs = (np.arange(N) + 0.5) * W / N; X, Y = np.meshgrid(xs, xs)
    water = unary_union([c.sea] + c.water_extra)
    dcity = shapely.distance(c.city, shapely.points(X.ravel(), Y.ravel())).reshape(N, N)
    insea = shapely.contains_xy(c.sea, X.ravel(), Y.ravel()).reshape(N, N)
    dshore = shapely.distance(c.sea.boundary, shapely.points(X.ravel(), Y.ravel())).reshape(N, N)
    h = fbm((N, N), rng); k = np.clip(dcity / 70, 0, 1); k = k * k * (3 - 2 * k); H = h * k
    if c.mountain:
        mx_, my_, mr_ = c.mountain; H = H + 1.25 * np.exp(-(((X - mx_) ** 2 + (Y - my_) ** 2) / (mr_ * mr_))) * np.clip(dcity / 25, 0, 1)
    gy, gx = np.gradient(H * 60); shade = np.clip(0.75 + (-gx * 0.6 - gy * 0.4) * 0.5, 0.45, 1.15)
    img = np.zeros((N, N, 3))
    low = np.array([96, 170, 70]); high = np.array([150, 205, 95]); img[:] = (low + (high - low) * H[..., None]) * shade[..., None]
    if c.mountain:   # dark cedar forest on the slopes, bare rock at the top
        forest = np.array([40, 92, 52]); rock = np.array([128, 124, 112]); f = np.clip((H - 0.6) / 0.5, 0, 1)[..., None]; rr = np.clip((H - 1.25) / 0.25, 0, 1)[..., None]
        img = img * (1 - f) + (forest * shade[..., None]) * f; img = img * (1 - rr) + (rock * shade[..., None]) * rr
    cont = (np.abs(((H * 14) % 1) - 0.5) > 0.47) & (k > 0.2); img[cont] *= 0.82
    citymask = dcity <= 0; img[citymask] = (168, 166, 158)
    sea_c = np.array([40, 110, 160]); deep = np.array([18, 55, 98]); t = np.clip(dshore / 90, 0, 1)[..., None]
    img[insea] = (sea_c + (deep - sea_c) * t)[insea]
    base = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8)).resize((PX, PX), Image.BILINEAR)
    d = ImageDraw.Draw(base, "RGBA")
    def P(poly, fill, outline=None, w=1):
        geoms = poly.geoms if hasattr(poly, "geoms") else [poly]
        for g in geoms:
            if g.geom_type != "Polygon" or g.is_empty: continue
            d.polygon([(x * K, y * K) for x, y in g.exterior.coords], fill=fill, outline=outline, width=w)
            for hole in g.interiors: d.polygon([(x * K, y * K) for x, y in hole.coords], fill=(168, 166, 158))
    land = c.city.difference(water)
    if c.highway is not None: land = land.difference(c.highway.buffer(8))
    roads_poly = unary_union([ln.buffer(w / 2, cap_style=2) for ln, w, cl in c.roads if cl not in ("br",) and not ln.is_empty] + c.plazas).intersection(land)
    P(land, (70, 72, 76))                                    # asphalt everywhere in town
    blocks = land.difference(roads_poly)
    bl = list(blocks.geoms) if hasattr(blocks, "geoms") else [blocks]
    parks = unary_union(c.parks) if c.parks else Polygon()
    for b in bl:
        if b.area < 40: continue
        P(b, (176, 174, 166))                                # sidewalk
        if not parks.is_empty and b.intersection(parks).area > 0.5 * b.area:
            P(b.buffer(-1.5), (92, 150, 78));
            for _ in range(int(b.area / 120)):
                p = Point(rng.uniform(*b.bounds[0::2]), rng.uniform(*b.bounds[1::2]))
                if b.contains(p): d.ellipse([(p.x - 2) * K, (p.y - 2) * K, (p.x + 2) * K, (p.y + 2) * K], fill=(60, 115, 55))
            continue
        cen = b.centroid; dist = min(c.districts, key=lambda D: (D[0][0] - cen.x) ** 2 + (D[0][1] - cen.y) ** 2)
        col = DIST_COL.get(dist[1], (200, 190, 170)); tgt = 260 if col[0] < 180 else 140
        inner = b.buffer(-2.2)
        if inner.is_empty: continue
        for g in (inner.geoms if hasattr(inner, "geoms") else [inner]):
            for lot in lots(g, rng, tgt):
                bld = lot.buffer(-1.1, join_style=2)
                if bld.is_empty or bld.area < 12: continue
                v = rng.uniform(0.85, 1.08); fc = tuple(int(min(255, ch * v)) for ch in col)
                P(bld, fc, (60, 58, 56))
    for bg in c.big: P(bg.intersection(land), (150, 140, 128), (50, 48, 45), 2)
    for ln, w, cl in c.roads:                                 # centre lines on avenues, stairs pattern
        if cl == "av" and not ln.is_empty:
            for g in (ln.geoms if hasattr(ln, "geoms") else [ln]):
                g = g.intersection(land)
                for gg in (g.geoms if hasattr(g, "geoms") else [g]):
                    if gg.geom_type == "LineString" and gg.length > 5: d.line([(x * K, y * K) for x, y in gg.coords], fill=(200, 190, 120, 160), width=2)
    # water on top, piers, bridges
    P(unary_union(c.water_extra) if c.water_extra else Polygon(), (40, 110, 160))
    for pr in c.piers: P(pr.difference(land), (150, 140, 120), (60, 55, 50), 2)
    for ln, w, cl in c.roads:
        if cl == "br": P(ln.buffer(w / 2, cap_style=2), (95, 97, 100), (30, 30, 30), 2)
    for wg in c.water_extra:                                  # bridges where streets cross canals/rivers
        for ln, w, cl in c.roads:
            if cl in ("av",) and not ln.is_empty and ln.intersects(wg):
                P(ln.buffer(w / 2, cap_style=2).intersection(wg.buffer(2)), (95, 97, 100), (30, 30, 30), 2)
    if c.highway is not None:
        hw = c.highway; openA = unary_union([c.city.buffer(30), c.sea.buffer(35)])
        vis = hw.intersection(openA); bore = hw.difference(openA)
        vl = list(vis.geoms) if hasattr(vis, "geoms") else [vis]
        for rp in c.ramps: P(rp.buffer(3.5, cap_style=1), (78, 80, 84), (225, 225, 215), 1)
        for g in vl:
            if g.length < 1: continue
            sides = []
            for side in ("left", "right"):
                o = g.parallel_offset(9, side); mid = o.interpolate(0.5, normalized=True) if not o.is_empty else None
                sides.append((side, mid is not None and c.sea.buffer(6).contains(mid)))
            sea_side = next((sd for sd, wet in sides if wet), "left"); land_side = "right" if sea_side == "left" else "left"
            # beach strips and tetrapod breakwaters on the sea side
            for t0 in np.arange(0, g.length, 70):
                seg = LineString([g.interpolate(t) for t in np.arange(t0, min(t0 + 38, g.length), 2)]) if t0 + 4 < g.length else None
                if seg is None or seg.length < 5: continue
                if int(t0 / 70) % 2 == 0: P(seg.buffer(15, cap_style=2).difference(g.buffer(6.5)).intersection(c.sea.buffer(1)), (214, 198, 150))
                else:
                    off = seg.parallel_offset(18, sea_side)
                    for k2 in range(0, int(off.length), 3):
                        q = off.interpolate(k2); jx, jy = rng.uniform(-1.5, 1.5, 2)
                        d.ellipse([(q.x + jx - 1.6) * K, (q.y + jy - 1.6) * K, (q.x + jx + 1.6) * K, (q.y + jy + 1.6) * K], fill=(165, 165, 158), outline=(70, 70, 70))
            P(g.buffer(7.5, cap_style=2), (190, 186, 176))                                  # pavement
            P(g.buffer(4.5, cap_style=2), (78, 80, 84))                                     # two lanes
            sw = g.parallel_offset(6.2, sea_side)
            for ee in (sw.geoms if hasattr(sw, "geoms") else [sw]):
                if ee.length > 2: d.line([(x * K, y * K) for x, y in ee.coords], fill=(215, 212, 200), width=int(1.6 * K))   # seawall
            for side in ("left", "right"):
                e = g.parallel_offset(3.9, side)
                for ee in (e.geoms if hasattr(e, "geoms") else [e]):
                    if ee.length > 2: d.line([(x * K, y * K) for x, y in ee.coords], fill=(235, 235, 225), width=2)
            for t in np.arange(0, g.length, 9):
                a, b = g.interpolate(t), g.interpolate(min(t + 4, g.length)); d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(240, 240, 230), width=2)
            for k2, t in enumerate(np.arange(40, g.length - 20, 95)):   # bus stops on the land side
                q = g.parallel_offset(6.4, land_side).interpolate(t)
                d.rectangle([(q.x - 2) * K, (q.y - 1.2) * K, (q.x + 2) * K, (q.y + 1.2) * K], fill=(60, 120, 170), outline=(20, 20, 20))
        zk = []
        for jp in c.junctions:   # zebra crossings where streets meet the coast road (one per 45 m)
            if c.highway.distance(jp) > 9 or c.city.buffer(30).disjoint(jp) or any(jp.distance(z) < 45 for z in zk): continue
            zk.append(jp)
            t = c.highway.project(jp); a = c.highway.interpolate(t - 7); b = c.highway.interpolate(t - 4)
            ang = math.atan2(b.y - a.y, b.x - a.x); nx, ny = -math.sin(ang), math.cos(ang)
            for k2 in range(-3, 4):
                cx_, cy_ = (a.x + b.x) / 2 + nx * k2 * 1.2, (a.y + b.y) / 2 + ny * k2 * 1.2
                d.line([((cx_ - math.cos(ang) * 1.5) * K, (cy_ - math.sin(ang) * 1.5) * K), ((cx_ + math.cos(ang) * 1.5) * K, (cy_ + math.sin(ang) * 1.5) * K)], fill=(240, 240, 235), width=int(0.6 * K))
        bl2 = list(bore.geoms) if hasattr(bore, "geoms") else [bore]
        for g in bl2:
            if g.length < 5: continue
            # portal is the end that touches the open stretch
            a0, a1 = Point(g.coords[0]), Point(g.coords[-1])
            if a0.distance(vis) > a1.distance(vis): g = LineString(list(g.coords)[::-1])
            L = g.length; end = L * 0.55
            pp, qq = g.interpolate(0), g.interpolate(1.5); ang = math.atan2(qq.y - pp.y, qq.x - pp.x); nx, ny = -math.sin(ang), math.cos(ang)
            d.line([((pp.x - nx * 8) * K, (pp.y - ny * 8) * K), ((pp.x + nx * 8) * K, (pp.y + ny * 8) * K)], fill=(225, 225, 215), width=12)
            for t in np.arange(0, end - 4, 10):
                a, b = g.interpolate(t), g.interpolate(min(t + 6, end))
                d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(40, 40, 45, 230), width=13)
                d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(240, 200, 80, 220), width=4)
            e = g.interpolate(end); s_ = 11
            d.line([((e.x - s_) * K, (e.y - s_) * K), ((e.x + s_) * K, (e.y + s_) * K)], fill=(200, 40, 30), width=9)
            d.line([((e.x - s_) * K, (e.y + s_) * K), ((e.x + s_) * K, (e.y - s_) * K)], fill=(200, 40, 30), width=9)
            for t in np.arange(end + 8, L, 14):
                a, b = g.interpolate(t), g.interpolate(min(t + 4, L)); d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(60, 80, 50, 120), width=5)
    if c.touge is not None:
        tg = c.touge
        P(tg.buffer(4.2, cap_style=1), (225, 225, 215))                  # guardrails
        P(tg.buffer(3.2, cap_style=1), (78, 80, 84))
        for t in np.arange(0, tg.length, 8):
            a, b = tg.interpolate(t), tg.interpolate(min(t + 3, tg.length)); d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(235, 235, 225), width=2)
        for pz in c.plazas:
            if pz.distance(c.city) > 2: P(pz, (78, 80, 84), (225, 225, 215), 2)
        for hs in c.houses: P(hs, (92, 70, 54), (30, 22, 18), 2); P(hs.buffer(-1.6), (120, 92, 70))
        for x, y in c.torii_path: d.line([((x - 2.5) * K, y * K), ((x + 2.5) * K, y * K)], fill=(210, 50, 35), width=6)
    if c.oldtunnel is not None:
        ln, frac = c.oldtunnel; L = ln.length; end = L * frac
        pp = ln.interpolate(0); qq = ln.interpolate(1.5); ang = math.atan2(qq.y - pp.y, qq.x - pp.x); nx, ny = -math.sin(ang), math.cos(ang)
        P(Point(pp.x, pp.y).buffer(7), (150, 85, 60), (40, 25, 20), 3)          # old brick portal
        for j in (-2.5, 0, 2.5): P(Point(pp.x + nx * j, pp.y + ny * j).buffer(1.4), (190, 190, 180), (60, 60, 60), 1)   # concrete blocks sealing it
        for t in np.arange(3, end - 3, 8):
            a, b = ln.interpolate(t), ln.interpolate(min(t + 5, end)); d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(40, 30, 28, 230), width=9); d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(190, 120, 80, 230), width=2)
        e = ln.interpolate(end); s_ = 8
        d.line([((e.x - s_) * K, (e.y - s_) * K), ((e.x + s_) * K, (e.y + s_) * K)], fill=(200, 40, 30), width=8)
        d.line([((e.x - s_) * K, (e.y + s_) * K), ((e.x + s_) * K, (e.y - s_) * K)], fill=(200, 40, 30), width=8)
    # shoreline
    for g in (c.sea.geoms if hasattr(c.sea, "geoms") else [c.sea]):
        if g.geom_type == "Polygon": d.line([(x * K, y * K) for x, y in g.exterior.coords], fill=(220, 230, 235, 140), width=3)
    # subway line
    st = c.stations
    if len(st) > 1:
        sub = LineString(st); pts = [sub.interpolate(t) for t in np.arange(0, sub.length, 6)]
        for i in range(0, len(pts) - 1, 2): d.line([(pts[i].x * K, pts[i].y * K), (pts[i + 1].x * K, pts[i + 1].y * K)], fill=(230, 120, 40, 230), width=5)
        for x, y in st: d.ellipse([(x - 6) * K, (y - 6) * K, (x + 6) * K, (y + 6) * K], fill=(230, 120, 40), outline=(20, 20, 20), width=3)
    for x, y in c.shrines: d.rectangle([(x - 4) * K, (y - 3) * K, (x + 4) * K, (y - 2) * K], fill=(200, 40, 30)); d.rectangle([(x - 3) * K, (y - 3) * K, (x - 2.2) * K, (y + 4) * K], fill=(200, 40, 30)); d.rectangle([(x + 2.2) * K, (y - 3) * K, (x + 3) * K, (y + 4) * K], fill=(200, 40, 30))
    # tunnels: approach road in a cut, portal, then the bore (dashed) to the collapse
    for ln, frac in c.tunnels:
        start = Point(ln.coords[0]); open_len = 0
        L = ln.length
        # portal where the line leaves the city by 22 m
        for t in np.arange(0, L, 2):
            if c.city.distance(ln.interpolate(t)) > 22: open_len = t; break
        cpts = [ln.interpolate(t) for t in np.arange(0, open_len + 0.1, 2)]
        cut = LineString(cpts) if len(cpts) > 1 else LineString([start, ln.interpolate(max(open_len, 1))])
        if cut.length > 0:
            P(cut.buffer(14, cap_style=2), (120, 110, 90, 200)); P(cut.buffer(5, cap_style=2), (70, 72, 76))
        pp = ln.interpolate(open_len); qq = ln.interpolate(open_len + 1); ang = math.atan2(qq.y - pp.y, qq.x - pp.x)
        nx, ny = -math.sin(ang), math.cos(ang)
        d.line([((pp.x - nx * 9) * K, (pp.y - ny * 9) * K), ((pp.x + nx * 9) * K, (pp.y + ny * 9) * K)], fill=(225, 225, 215), width=10)
        bore_end = open_len + (L - open_len) * frac
        for t in np.arange(open_len, bore_end - 4, 10):
            a, b = ln.interpolate(t), ln.interpolate(min(t + 6, bore_end))
            d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(40, 40, 45, 230), width=11)
            d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(240, 200, 80, 220), width=3)
        e = ln.interpolate(bore_end); s = 9
        d.line([((e.x - s) * K, (e.y - s) * K), ((e.x + s) * K, (e.y + s) * K)], fill=(200, 40, 30), width=8)
        d.line([((e.x - s) * K, (e.y + s) * K), ((e.x + s) * K, (e.y - s) * K)], fill=(200, 40, 30), width=8)
        for t in np.arange(bore_end + 8, L, 14):   # the old road beyond, buried
            a, b = ln.interpolate(t), ln.interpolate(min(t + 4, L)); d.line([(a.x * K, a.y * K), (b.x * K, b.y * K)], fill=(60, 80, 50, 120), width=4)
    # labels
    f = font(26, True); fj = font(22)
    for (x, y), en, jp in c.districts:
        for tx, ff, oy in ((jp, fj, -20), (en, f, 8)):
            tw = d.textlength(tx, font=ff); X0, Y0 = x * K - tw / 2, y * K + oy
            for ox in (-2, 0, 2):
                for oy2 in (-2, 0, 2): d.text((X0 + ox, Y0 + oy2), tx, font=ff, fill=(20, 22, 24, 200))
            d.text((X0, Y0), tx, font=ff, fill=(250, 248, 240))
    # header + legend
    hdr = Image.new("RGB", (PX, 170), (16, 20, 22)); hd = ImageDraw.Draw(hdr)
    hd.text((30, 18), f"{idx}. {c.title}", font=font(54, True), fill=(236, 230, 216))
    hd.text((30 + hd.textlength(f"{idx}. {c.title}", font=font(54, True)) + 24, 30), c.jp, font=font(42), fill=(150, 210, 205))
    hd.text((32, 104), c.desc, font=font(26), fill=(190, 186, 176))
    leg = Image.new("RGB", (PX, 70), (16, 20, 22)); ld = ImageDraw.Draw(leg); x = 30
    for col, tx, kind in (((176, 174, 166), "Buildings", "box"), ((40, 110, 160), "Bay", "box"), ((110, 180, 80), "Hills", "box"), ((230, 120, 40), "Subway + stations", "line"),
                          ((240, 200, 80), "Road tunnel", "line"), ((200, 40, 30), "Collapse", "x"), ((78, 80, 84), "Coast road", "box"), ((214, 198, 150), "Beach", "box"), ((92, 70, 54), "Old houses", "box")):
        if kind == "box": ld.rectangle([x, 22, x + 26, 46], fill=col)
        elif kind == "line": ld.line([x, 34, x + 30, 34], fill=col, width=6)
        else: ld.line([x, 22, x + 24, 46], fill=col, width=6); ld.line([x, 46, x + 24, 22], fill=col, width=6)
        ld.text((x + 34, 18), tx, font=font(22), fill=(200, 196, 186)); x += 34 + ld.textlength(tx, font=font(22)) + 22
    sx = 40; sy = PX - 40
    d.line([sx - 2, sy, sx + 100 * K + 2, sy], fill=(20, 20, 20), width=11); d.line([sx, sy, sx + 100 * K, sy], fill=(240, 240, 235), width=5)
    d.text((sx + 100 * K / 2 - 28, sy - 34), "100 m", font=font(20, True), fill=(250, 250, 245), stroke_width=3, stroke_fill=(20, 20, 20))
    if False: ld.line([sx, 30, sx + 100 * K, 30], fill=(220, 220, 220), width=5); ld.line([sx, 22, sx, 38], fill=(220, 220, 220), width=3); ld.line([sx + 100 * K, 22, sx + 100 * K, 38], fill=(220, 220, 220), width=3)
    pass
    out = Image.new("RGB", (PX, PX + 240)); out.paste(hdr, (0, 0)); out.paste(base, (0, 170)); out.paste(leg, (0, PX + 170))
    return out

if len(sys.argv) > 2 and sys.argv[2] == "5b":
    im = draw_city(c5b(), "5b"); im.save(os.path.join(OUT, "layout_05b.png"), optimize=True); print("wrote 5b"); sys.exit(0)
imgs = []
for i, fn in enumerate(CONCEPTS, 1):
    c = fn(); im = draw_city(c, i); p = os.path.join(OUT, f"layout_{i:02d}.png"); im.save(p, optimize=True); imgs.append(im); print("wrote", p)
for s in range(2):   # two contact sheets, 5 maps each (3 + 2 with a title tile)
    tw = 700; th = int(tw * imgs[0].height / imgs[0].width)
    sheet = Image.new("RGB", (tw * 3 + 40, th * 2 + 30), (10, 12, 13))
    for k in range(5):
        im = imgs[s * 5 + k].resize((tw, th), Image.LANCZOS); col, row = k % 3, k // 3
        sheet.paste(im, (10 + col * (tw + 10), 10 + row * (th + 10)))
    sd = ImageDraw.Draw(sheet); x0, y0 = 10 + 2 * (tw + 10), 10 + th + 10
    sd.text((x0 + 30, y0 + 40), "Zankō 2006", font=font(46, True), fill=(236, 230, 216))
    sd.text((x0 + 30, y0 + 105), f"City layout concepts {s * 5 + 1}–{s * 5 + 5}", font=font(28), fill=(150, 210, 205))
    for j, ln in enumerate(["Every concept has:", "• a bay on the coast", "• hills around the city", "• 3 roads leaving through tunnels,", "   each collapsed partway (red X)",
                            "• a subway line", "Pick one (or mix parts)."]):
        sd.text((x0 + 30, y0 + 175 + j * 40), ln, font=font(24), fill=(200, 196, 186))
    sheet.save(os.path.join(OUT, f"layouts_sheet_{s + 1}.png"), optimize=True); print("sheet", s + 1)

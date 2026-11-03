#!/usr/bin/env python3
"""Export concept map 5b (tools/citymaps.py) as fixed game layout data: zk_design.h.
World metres = S * design metres + T.  S = 44/36 makes the drawing's 36 m street grid land exactly on the game's
44 m grid (34 m blocks + 10 m roads); T shifts it so the drawing's grid lines fall on the game's road lines."""
import sys, math, os
import numpy as np
from shapely.geometry import Polygon, LineString, Point, box, MultiPolygon
from shapely.ops import unary_union, nearest_points, substring
import shapely

src = open(os.path.join(os.path.dirname(__file__), "citymaps.py"), encoding="utf-8").read()
src = src[:src.index("if len(sys.argv) > 2")]
sys.argv = [sys.argv[0], "/tmp/claude-0/unused_maps"]
G = {}; exec(compile(src, "citymaps.py", "exec"), G)
c = G["c5b"]()

S = 44.0 / 36.0
TX = 5 - 380 * S + 44 * 9          # design x 380 (a grid line) -> world 44*9+5
TZ = 5 - 230 * S + 44 * 4          # design y 230 (a grid line) -> world 44*4+5
NB, PITCH, ROADW, BLOCK = 12, 44.0, 10.0, 34.0
W = lambda x, y: (x * S + TX, y * S + TZ)
D = lambda x, z: ((x - TX) / S, (z - TZ) / S)
def wl(g): return [W(x, y) for x, y in g.coords]

water = unary_union([c.sea] + c.water_extra)
land = c.city.difference(water).difference(c.highway.buffer(8))
core = max(land.geoms, key=lambda g: g.area) if hasattr(land, "geoms") else land
inner = core.buffer(-7); inner = max(inner.geoms, key=lambda g: g.area) if hasattr(inner, "geoms") else inner
old = Point(230, 360).buffer(110)
av = [r for r in c.roads if r[2] == "av"]
avenue = unary_union([r[0] for r in av]) if av else LineString()
avenue = max(avenue.geoms, key=lambda g: g.length) if hasattr(avenue, "geoms") else avenue

# ---- blocks
GONE, NORMAL, OLD, PARK, STATION, MARKET, PLAZA = 1, 0, 2, 3, 4, 5, 6
kind = [[GONE] * NB for _ in range(NB)]
def brect(i, j):
    x0, z0 = ROADW + i * PITCH, ROADW + j * PITCH
    a, b = D(x0, z0), D(x0 + BLOCK, z0 + BLOCK); return box(a[0], a[1], b[0], b[1])
ring_half = 5.0 / S   # 10 m ring road
for i in range(NB):
    for j in range(NB):
        r = brect(i, j)
        if r.intersection(core.buffer(-1.5)).area < 0.62 * r.area: continue   # partly clipped edge blocks stay, as in the drawing
        if r.distance(avenue) < 7.5 + 0.5 and avenue.intersection(r.buffer(-1)).length > 0: kind[i][j] = PLAZA; continue
        kind[i][j] = OLD if old.contains(r.centroid) else NORMAL
def nearest_block(pt, allowed):
    best, bd = None, 1e9
    for i in range(NB):
        for j in range(NB):
            if kind[i][j] in allowed:
                d = brect(i, j).centroid.distance(Point(pt))
                if d < bd: best, bd = (i, j), d
    return best
p = nearest_block((235, 365), (OLD, NORMAL)); kind[p[0]][p[1]] = PARK
m = nearest_block((318, 415), (OLD, NORMAL)); kind[m[0]][m[1]] = MARKET
st = nearest_block((420, 300), (NORMAL,)); kind[st[0]][st[1]] = STATION
SUB_ROW = 4
subcols = [i for i in range(NB) if kind[i][SUB_ROW] == NORMAL]
pick = [subcols[0], subcols[len(subcols) // 2], subcols[-1]] if len(subcols) >= 3 else subcols
rowcols = [i for i in range(NB) if kind[i][SUB_ROW] != GONE or (SUB_ROW > 0 and kind[i][SUB_ROW - 1] != GONE)]
SUB_X0, SUB_X1 = ROADW + min(rowcols) * PITCH - 4, ROADW + max(rowcols) * PITCH + BLOCK + 4
normals = [(i, j) for i in range(NB) for j in range(NB) if kind[i][j] == NORMAL]
rng = np.random.default_rng(2003)
order = rng.permutation(len(normals))
koban = normals[order[0]]; konbini = [normals[k] for k in order[1:6]]
# spawn on the road node nearest the town's centre of mass
cx, cz = W(core.centroid.x, core.centroid.y)
def node_on(i, j): return any(0 <= a < NB and 0 <= b < NB and kind[a][b] != GONE for a in (i - 1, i) for b in (j - 1, j))
nodes = [(i, j) for i in range(NB + 1) for j in range(NB + 1) if node_on(i, j)]
sn = min(nodes, key=lambda n: (n[0] * PITCH + 5 - cx) ** 2 + (n[1] * PITCH + 5 - cz) ** 2)

# ---- paths
def resample_line(g, step=2.0 / S):
    L = g.length; n = max(2, int(L / step) + 1)
    return [g.interpolate(L * k / (n - 1)) for k in range(n)]
hw = c.highway
def tangent(g, d): a = g.interpolate(max(0, d - 2)); b = g.interpolate(min(g.length, d + 2)); v = np.array([b.x - a.x, b.y - a.y]); return v / (np.linalg.norm(v) + 1e-9)
# west portal: where the coast road, heading north into the hills, is most nearly vertical between design y 250 and 300
cands = [d for d in np.arange(0, hw.length, 1.0) if 250 < hw.interpolate(d).y < 300]
dW = min(cands, key=lambda d: abs(tangent(hw, d)[0]))
# east portal: past the old town, the first point where the road heads within 20 degrees of due east
dE = next(d for d in np.arange(hw.length * 0.6, hw.length, 1.0) if tangent(hw, d)[0] > math.cos(math.radians(20)) and hw.interpolate(d).x > 440)
coast = substring(hw, dW, dE)
pW, pE = W(*coast.coords[0]), W(*coast.coords[-1])
# ring road: the inner outline, except where the coast road already runs
ringl = LineString(inner.exterior.coords).simplify(1.5)
blk = unary_union([brect(i, j).buffer(2.5 / S) for i in range(NB) for j in range(NB) if kind[i][j] != GONE])
rg = ringl.difference(hw.buffer(24)).difference(blk)
rings = [g for g in (rg.geoms if hasattr(rg, "geoms") else [rg]) if g.length > 20]
# avenue clipped to the town
avc = avenue.intersection(core.buffer(2))
avc = max(avc.geoms, key=lambda g: g.length) if hasattr(avc, "geoms") else avc
touge = c.touge
lane = c.ramps[-1]
# north road: from the town's top edge on the nearest column road to the tunnel
nt = c.tunnels[0][0]
ncx = W(*nt.coords[0])[0]
ncol = min(range(NB + 1), key=lambda i: abs(i * PITCH + 5 - ncx))
nx = ncol * PITCH + 5
topz = min(ROADW + j * PITCH for i in (ncol - 1, ncol) for j in range(NB) if 0 <= i < NB and kind[i][j] != GONE) - ROADW
north_portal_z = topz - 34
ot = c.oldtunnel[0]

def cpts(name, pts, fmt="{%.2ff, %.2ff}"):
    s = f"static const float {name}[][2] = {{" + ", ".join("{%.2ff, %.2ff}" % p for p in pts) + "};\n"
    return s + f"#define {name.upper()}_N {len(pts)}\n"
out = ["// zk_design.h - generated by tools/export5b.py from concept map 5b (do not edit by hand)\n",
       f"// world = {S:.6f} * design + ({TX:.3f}, {TZ:.3f})\n"]
assert NB == 12
out.append("static const unsigned char designKind[12][12] = {" + ", ".join("{" + ", ".join(str(kind[i][j]) for j in range(NB)) + "}" for i in range(NB)) + "};\n")
out.append(f"#define DS_PLAZA {PLAZA}\n#define SUB_ROW {SUB_ROW}\n#define SUB_X0 {SUB_X0:.1f}f\n#define SUB_X1 {SUB_X1:.1f}f\n")
out.append("static const int subCols[3] = {" + ", ".join(str(x) for x in (pick + pick)[:3]) + "};\n")
out.append(f"static const int kobanB[2] = {{{koban[0]}, {koban[1]}}};\nstatic const int konbiniB[5][2] = {{" + ", ".join("{%d, %d}" % k for k in konbini) + "};\n")
out.append(f"#define SPAWN_X {sn[0] * PITCH + 5:.1f}f\n#define SPAWN_Z {sn[1] * PITCH + 5:.1f}f\n")
core_w = [W(x, y) for x, y in core.simplify(0.8).exterior.coords]
out.append(cpts("dCore", core_w))
seap = water.intersection(box(-60, -60, 660, 660)); seap = max(seap.geoms, key=lambda g: g.area) if hasattr(seap, "geoms") else seap
out.append(cpts("dSea", [W(x, y) for x, y in seap.simplify(0.8).exterior.coords]))
out.append(cpts("dCoast", [W(p.x, p.y) for p in resample_line(coast)]))
for k, g in enumerate(rings[:4]): out.append(cpts(f"dRing{k}", [W(p.x, p.y) for p in resample_line(g)]))
for k in range(len(rings[:4]), 4): out.append(f"static const float dRing{k}[1][2] = {{{{0, 0}}}};\n#define DRING{k}_N 0\n")
out.append(cpts("dAvenue", [W(p.x, p.y) for p in resample_line(avc)]))
out.append(cpts("dTouge", [W(p.x, p.y) for p in resample_line(touge)]))
out.append(cpts("dLane", [W(p.x, p.y) for p in resample_line(lane)]))
out.append(f"#define NORTH_X {nx:.1f}f\n#define NORTH_Z0 {topz + ROADW / 2:.1f}f\n#define NORTH_PORTAL_Z {north_portal_z:.1f}f\n")
out.append(f"#define WEST_PX {pW[0]:.2f}f\n#define WEST_PZ {pW[1]:.2f}f\n#define EAST_PX {pE[0]:.2f}f\n#define EAST_PZ {pE[1]:.2f}f\n")
otw = W(*ot.coords[0]); out.append(f"#define OLDT_X {otw[0]:.2f}f\n#define OLDT_Z {otw[1]:.2f}f\n")
mx, mz = W(c.mountain[0], c.mountain[1]); out.append(f"#define DMT_X {mx:.1f}f\n#define DMT_Z {mz:.1f}f\n#define DMT_R {c.mountain[2] * S:.1f}f\n")
houses = []
for h in c.houses:
    cc = h.centroid; rr = h.minimum_rotated_rectangle; co = list(rr.exterior.coords)
    e1 = math.dist(co[0], co[1]); e2 = math.dist(co[1], co[2])
    x, z = W(cc.x, cc.y)
    houses.append((x, z, max(e1, e2) * S * 0.8, min(e1, e2) * S * 0.8))
out.append("static const float dHouses[][4] = {" + ", ".join("{%.1ff, %.1ff, %.1ff, %.1ff}" % hh for hh in houses) + "};\n#define DHOUSES_N %d\n" % len(houses))
shr = [W(x, y) for x, y in c.shrines if not core.buffer(5).contains(Point(x, y))]
out.append(cpts("dShrines", shr))
out.append(cpts("dTorii", [W(x, y) for x, y in c.torii_path]))
hp = W(*lane.coords[-1]); out.append(f"#define HAMLET_X {hp[0]:.1f}f\n#define HAMLET_Z {hp[1]:.1f}f\n")
open(sys.argv_out if hasattr(sys, "argv_out") else os.environ.get("OUT_H", "zk_design.h"), "w").write("".join(out))
# report
print("blocks:");
for j in range(NB): print(" ".join(".NOPSMx"[0] if kind[i][j] == GONE else "N" if kind[i][j] == NORMAL else "O" if kind[i][j] == OLD else "P" if kind[i][j] == PARK else "S" if kind[i][j] == STATION else "M" if kind[i][j] == MARKET else "x" for i in range(NB)))
print("sub cols", pick, "spawn node", sn, "north col", ncol, "topz", topz, "portals", pW, pE, "rings", [round(g.length) for g in rings], "houses", len(houses), "shrines", shr)

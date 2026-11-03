#!/usr/bin/env python3
"""Draw the board and the actual C++ MoveRangeResult; never run a new search.

Requires Pillow (see requirements-movement.txt). World positions are ignored;
pointy-top hex centers are derived exclusively from the axial q/r coordinates.
"""

from __future__ import annotations

import argparse
import json
import math
from functools import lru_cache
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


PROJECT = Path(__file__).resolve().parents[1]
DIRECTIONS = ((1, 0), (1, -1), (0, -1), (-1, 0), (-1, 1), (0, 1))
PALETTE = {
    "background": "#F3F6FA", "ink": "#172B46", "muted": "#5C6F87",
    "border": "#D6DFEA", "blocked": "#344256", "blocked_text": "#D8E1EB",
    "empty": "#F8FAFD", "outside": "#E9EDF3", "start": "#2868CF",
    "unit": "#FBE4E4", "unit_text": "#A5393C", "path": "#D47A05",
    "tree": "#557E99",
}


def integer(value, label):
    if type(value) is not int or not -(2**31) <= value < 2**31:
        raise ValueError(f"{label} must be an int32")
    return value


def coordinate(value):
    return integer(value["q"], "q"), integer(value["r"], "r")


def load_result(path):
    """Validate exported data and tree consistency, without computing shortest paths."""
    with path.open(encoding="utf-8-sig") as stream:
        data = json.load(stream)
    if data["schemaVersion"] != 1:
        raise ValueError("Unsupported result schema")
    cells = {}
    for cell in data["board"]["cells"]:
        key = coordinate(cell)
        if key in cells or type(cell["bStandable"]) is not bool:
            raise ValueError(f"Duplicate or invalid board cell: {key}")
        world = cell["worldPosition"]
        if len(world) != 3 or any(type(v) not in (int, float) or not math.isfinite(v) for v in world):
            raise ValueError(f"Invalid worldPosition at {key}")
        cells[key] = cell
    if not cells:
        raise ValueError("This visualization requires a nonempty board")

    units, unit_ids = {}, set()
    for unit in data["board"]["units"]:
        key, unit_id = coordinate(unit), integer(unit["unitId"], "unitId")
        if unit_id < 0 or unit_id in unit_ids or key in units or key not in cells or not cells[key]["bStandable"]:
            raise ValueError(f"Invalid unit placement: {unit}")
        units[key] = unit_id
        unit_ids.add(unit_id)
    query, result = data["query"], data["moveResult"]
    moving_id = integer(query["movingUnitId"], "movingUnitId")
    budget = integer(query["moveBudget"], "moveBudget")
    start = coordinate(result["start"])
    if budget < 0 or start not in units or units[start] != moving_id:
        raise ValueError("Query does not match the moving unit's actual start")

    nodes = {}
    for node in result["reachable"]:
        key, cost = coordinate(node), integer(node["cost"], "cost")
        if key in nodes or key not in cells or not cells[key]["bStandable"] or not 0 <= cost <= budget:
            raise ValueError(f"Invalid reachable node: {node}")
        if key in units and units[key] != moving_id:
            raise ValueError(f"Reachable node is occupied by another unit: {key}")
        nodes[key] = node
    if start not in nodes or nodes[start]["cost"] != 0 or nodes[start]["previous"] is not None:
        raise ValueError("Root must have cost zero and no predecessor")
    for key, node in nodes.items():
        if key != start:
            parent = coordinate(node["previous"])
            if parent not in nodes or (key[0] - parent[0], key[1] - parent[1]) not in DIRECTIONS:
                raise ValueError(f"Missing or nonadjacent predecessor for {key}")
            if node["cost"] != nodes[parent]["cost"] + 1:
                raise ValueError(f"Cost/predecessor mismatch at {key} (fixture uses default step cost 1)")
        for dq, dr in DIRECTIONS:
            neighbor = key[0] + dq, key[1] + dr
            legal = neighbor in cells and cells[neighbor]["bStandable"] and units.get(neighbor, moving_id) == moving_id
            if legal and node["cost"] + 1 <= budget:
                if neighbor not in nodes or nodes[neighbor]["cost"] > node["cost"] + 1:
                    raise ValueError(f"Incomplete range or nonminimal cost at {neighbor}")

    selected = result["selectedPath"]
    target = coordinate(selected["destination"])
    if target != coordinate(query["destination"]) or target not in nodes:
        raise ValueError("Selected target is inconsistent or unreachable")
    path = [coordinate(cell) for cell in selected["cells"]]
    expected, current, seen = [], target, set()
    while True:
        if current in seen:
            raise ValueError("Cycle in predecessor tree")
        seen.add(current)
        expected.append(current)
        if current == start:
            break
        current = coordinate(nodes[current]["previous"])
    if path != list(reversed(expected)) or selected["totalCost"] != nodes[target]["cost"]:
        raise ValueError("Selected path differs from the exported predecessor tree")
    return data, cells, units, nodes, start, target, path


@lru_cache(maxsize=64)
def font(size, bold=False, mono=False):
    candidates = (
        ["C:/Windows/Fonts/consola.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"] if mono else
        ["C:/Windows/Fonts/segoeuib.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"] if bold else
        ["C:/Windows/Fonts/segoeui.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"]
    )
    for candidate in candidates:
        if Path(candidate).is_file():
            return ImageFont.truetype(candidate, size)
    return ImageFont.load_default(size=size)


class Canvas:
    def __init__(self, width, height, scale=2):
        self.scale = scale
        self.width, self.height = width, height
        self.image = Image.new("RGB", (width * scale, height * scale), PALETTE["background"])
        self.draw = ImageDraw.Draw(self.image)

    def point(self, point):
        return tuple(round(v * self.scale) for v in point)

    def text(self, xy, text, size=20, fill=None, bold=False, mono=False, anchor="la"):
        self.draw.text(self.point(xy), text, font=font(round(size * self.scale), bold, mono),
                       fill=fill or PALETTE["ink"], anchor=anchor)

    def polygon(self, points, fill, outline=None, width=1):
        self.draw.polygon([self.point(p) for p in points], fill=fill, outline=outline,
                          width=max(1, round(width * self.scale)))

    def line(self, points, fill, width=1):
        self.draw.line([self.point(p) for p in points], fill=fill, width=max(1, round(width * self.scale)))

    def box(self, bounds, fill="white", outline=None, radius=18, width=1):
        self.draw.rounded_rectangle([self.point(bounds[:2]), self.point(bounds[2:])],
                                    radius=round(radius * self.scale), fill=fill,
                                    outline=outline, width=max(1, round(width * self.scale)))

    def arrow(self, start, end, color, width=2, head=9, inset=0.28):
        dx, dy = end[0] - start[0], end[1] - start[1]
        length = math.hypot(dx, dy)
        ux, uy = dx / length, dy / length
        a = start[0] + dx * inset, start[1] + dy * inset
        b = end[0] - dx * inset, end[1] - dy * inset
        self.line([a, b], color, width)
        self.polygon([b, (b[0] - ux * head - uy * head * 0.48, b[1] - uy * head + ux * head * 0.48),
                      (b[0] - ux * head + uy * head * 0.48, b[1] - uy * head - ux * head * 0.48)], color)

    def save(self, output):
        output.parent.mkdir(parents=True, exist_ok=True)
        self.image.resize((self.width, self.height), Image.Resampling.LANCZOS).save(output, format="PNG")


def raw_center(key):
    q, r = key
    return math.sqrt(3) * (q + r / 2), 1.5 * r


def render(result_file, output):
    data, cells, units, nodes, start, target, path = load_result(result_file)
    budget = data["query"]["moveBudget"]
    canvas = Canvas(2200, 1360)
    canvas.text((64, 35), f"Hex movement / {data.get('searchAlgorithm', 'range search')}", 43, bold=True)
    canvas.text((66, 101), f"{data['name']}   |   Unit {units[start]}   |   Budget {budget} AP   |   Target {target}",
                24, fill=PALETTE["muted"])
    raw = {key: raw_center(key) for key in cells}
    xs, ys = [p[0] for p in raw.values()], [p[1] for p in raw.values()]
    mid = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    path_edges = set(zip(path, path[1:]))
    blocked_count = sum(not cell["bStandable"] for cell in cells.values())

    for panel in range(2):
        left, top, width, height = 56 + panel * 1060, 173, 1028, 1020
        canvas.box((left, top, left + width, top + height), outline=PALETTE["border"])
        title = "01  Input board" if panel == 0 else "02  Move range + predecessor arrows"
        detail = (f"{len(cells)} cells   /   {blocked_count} blocked   /   {len(units)} units" if panel == 0 else
                  f"{len(nodes)} reachable incl. start   /   Selected path: {len(path) - 1} steps, {nodes[target]['cost']} AP")
        canvas.text((left + 30, top + 22), title, 27, bold=True)
        canvas.text((left + 30, top + 65), detail, 18, fill=PALETTE["muted"])
        size = min((width - 100) / (max(xs) - min(xs) + 2), (height - 160) / (max(ys) - min(ys) + 2))
        origin = left + width / 2, top + 115 + (height - 140) / 2
        centers = {key: (origin[0] + (point[0] - mid[0]) * size, origin[1] + (point[1] - mid[1]) * size)
                   for key, point in raw.items()}

        for key, cell in sorted(cells.items()):
            x, y = centers[key]
            color, border, edge_width = PALETTE["empty"], PALETTE["border"], 1.4
            if not cell["bStandable"]:
                color = PALETTE["blocked"]
            elif key in units and key != start:
                color = PALETTE["unit"]
            elif panel == 1:
                if key in nodes:
                    fraction = nodes[key]["cost"] / max(1, budget)
                    color = (round(210 - 56 * fraction), round(237 - 28 * fraction), round(249 - 23 * fraction))
                else:
                    color = PALETTE["outside"]
            if key == start:
                color, border, edge_width = "#E0EDFF", PALETTE["start"], 3.5
            if panel == 1 and key in path:
                border, edge_width = PALETTE["path"], 4
            vertices = [(x + size * 0.97 * math.cos(math.radians(30 + i * 60)),
                         y + size * 0.97 * math.sin(math.radians(30 + i * 60))) for i in range(6)]
            canvas.polygon(vertices, color, border, edge_width)

        if panel == 1:
            for key, node in sorted(nodes.items()):
                if node["previous"] is None:
                    continue
                parent = coordinate(node["previous"])
                if (parent, key) not in path_edges:
                    canvas.arrow(centers[parent], centers[key], PALETTE["tree"], 2.0, 8)
            for parent, child in zip(path, path[1:]):
                canvas.arrow(centers[parent], centers[child], PALETTE["path"], 4.5, 12, 0.26)

        for key, cell in sorted(cells.items()):
            x, y = centers[key]
            text_color = PALETTE["ink"]
            value = ""
            if not cell["bStandable"]:
                value, text_color = "X", PALETTE["blocked_text"]
            elif key in units:
                value = f"S / {units[key]}" if key == start else f"U{units[key]}"
                text_color = PALETTE["start"] if key == start else PALETTE["unit_text"]
            elif panel == 1 and key in nodes:
                value = str(nodes[key]["cost"])
            if panel == 1 and key == start:
                value = "S / 0"
            if panel == 1 and key == target:
                value = f"G / {nodes[key]['cost']}"
            if value:
                canvas.text((x, y - 10), value, 19 if len(value) > 3 else 24,
                            fill=text_color, bold=True, anchor="mm")
            canvas.text((x, y + (17 if value else 0)), f"{key[0]},{key[1]}", 13,
                        fill=text_color if not cell["bStandable"] else PALETTE["muted"], mono=True, anchor="mm")

    legend = [(80, "#E0EDFF", "S = start / moving unit"), (460, PALETTE["blocked"], "X = blocked terrain"),
              (850, PALETTE["unit"], "U = other unit"), (1210, PALETTE["outside"], "Outside budget"),
              (1610, "#B9DFEF", "Number = minimum AP")]
    for x, color, label in legend:
        canvas.box((x, 1221, x + 26, 1247), fill=color, outline=PALETTE["border"], radius=5)
        canvas.text((x + 38, 1221), label, 19)
    canvas.arrow((90, 1285), (165, 1285), PALETTE["tree"], 2.0, 8, 0)
    canvas.text((184, 1271), "Predecessor tree: parent -> child", 19)
    canvas.arrow((750, 1285), (825, 1285), PALETTE["path"], 4.5, 12, 0)
    canvas.text((844, 1271), "Highlighted path: start -> target", 19)
    canvas.text((80, 1320), "Source: actual C++ FPathFinding output. Layout uses axial q/r; worldPosition is not used.",
                17, fill=PALETTE["muted"])
    canvas.save(output)
    print(f"Saved {output} | {len(cells)} cells, {len(nodes)} reachable, {len(nodes) - 1} tree arrows, "
          f"{len(path) - 1} selected-path arrows, target cost {nodes[target]['cost']} AP")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result", type=Path, default=PROJECT / "Saved/Tests/Movement/move_range_result.json")
    parser.add_argument("--output", type=Path, default=PROJECT / "Saved/Tests/Movement/move_range.png")
    args = parser.parse_args()
    try:
        render(args.result.resolve(), args.output.resolve())
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"Cannot visualize movement result: {error}\n")


if __name__ == "__main__":
    main()

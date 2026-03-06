#!/usr/bin/env python3
import csv
import sys

if len(sys.argv) != 3:
    print("Usage: render_compare_svg.py <input_csv> <output_svg>")
    sys.exit(1)

input_csv, output_svg = sys.argv[1], sys.argv[2]
rows = []
with open(input_csv, newline='', encoding='utf-8') as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            rows.append((row['metric'], float(row['baseline']), float(row['target'])))
        except Exception:
            pass

if not rows:
    raise SystemExit("No numeric rows found in CSV")

max_val = max(max(b, t) for _, b, t in rows)
max_val = max_val if max_val > 0 else 1.0

width = 1200
height = 80 + len(rows) * 56
left = 240
chart_width = 860
bar_h = 16

def scale(v):
    return v / max_val * chart_width

svg = []
svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">')
svg.append('<rect width="100%" height="100%" fill="#ffffff"/>')
svg.append('<text x="24" y="34" font-size="22" font-family="Arial" fill="#111827">Benchmark Compare (Baseline vs Target)</text>')
svg.append('<rect x="24" y="44" width="14" height="14" fill="#94a3b8"/><text x="44" y="56" font-size="14" font-family="Arial" fill="#111827">Baseline</text>')
svg.append('<rect x="140" y="44" width="14" height="14" fill="#2563eb"/><text x="160" y="56" font-size="14" font-family="Arial" fill="#111827">Target</text>')

for i, (metric, baseline, target) in enumerate(rows):
    y0 = 88 + i * 56
    b_w = scale(baseline)
    t_w = scale(target)

    svg.append(f'<text x="24" y="{y0+16}" font-size="13" font-family="Arial" fill="#111827">{metric}</text>')
    svg.append(f'<rect x="{left}" y="{y0}" width="{b_w:.2f}" height="{bar_h}" fill="#94a3b8"/>')
    svg.append(f'<text x="{left + b_w + 8:.2f}" y="{y0+13}" font-size="12" font-family="Arial" fill="#374151">{baseline:g}</text>')

    svg.append(f'<rect x="{left}" y="{y0+22}" width="{t_w:.2f}" height="{bar_h}" fill="#2563eb"/>')
    svg.append(f'<text x="{left + t_w + 8:.2f}" y="{y0+35}" font-size="12" font-family="Arial" fill="#1f2937">{target:g}</text>')

svg.append('</svg>')

with open(output_svg, 'w', encoding='utf-8') as f:
    f.write('\n'.join(svg))

print(f"SVG generated: {output_svg}")

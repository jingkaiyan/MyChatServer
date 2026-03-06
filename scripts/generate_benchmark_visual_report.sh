#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPARE_SCRIPT="${PROJECT_ROOT}/scripts/compare_benchmark_runs.sh"
RENDER_SCRIPT="${PROJECT_ROOT}/scripts/render_compare_svg.py"

BASELINE_TAG=""
TARGET_TAG=""
BASELINE_SECONDS=""
TARGET_SECONDS=""
OUT_DIR="${PROJECT_ROOT}/exports/visual_report"

usage() {
  cat <<'EOF'
用法:
  ./scripts/generate_benchmark_visual_report.sh [options]

必选参数:
  --baseline-tag <tag>         基线批次标识
  --target-tag <tag>           优化后批次标识

可选参数:
  --baseline-seconds <sec>     基线压测时长
  --target-seconds <sec>       优化后压测时长
  --out-dir <dir>              输出目录（默认: ./exports/visual_report）
  --help                       查看帮助

示例:
  ./scripts/generate_benchmark_visual_report.sh \
    --baseline-tag bench_20260304_150317_21492 \
    --target-tag bench_20260305_103211_18201 \
    --baseline-seconds 300 \
    --target-seconds 300
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --baseline-tag)
      BASELINE_TAG="$2"; shift 2 ;;
    --target-tag)
      TARGET_TAG="$2"; shift 2 ;;
    --baseline-seconds)
      BASELINE_SECONDS="$2"; shift 2 ;;
    --target-seconds)
      TARGET_SECONDS="$2"; shift 2 ;;
    --out-dir)
      OUT_DIR="$2"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "未知参数: $1"
      usage
      exit 1 ;;
  esac
done

if [[ -z "$BASELINE_TAG" || -z "$TARGET_TAG" ]]; then
  echo "--baseline-tag 和 --target-tag 必须提供"
  usage
  exit 1
fi

mkdir -p "$OUT_DIR"
MD_OUT="${OUT_DIR}/compare_report.md"
CSV_OUT="${OUT_DIR}/compare_report.csv"
SVG_OUT="${OUT_DIR}/compare_chart.svg"
INDEX_OUT="${OUT_DIR}/README.md"

cmd=("$COMPARE_SCRIPT" --baseline-tag "$BASELINE_TAG" --target-tag "$TARGET_TAG" --export-md "$MD_OUT" --export-csv "$CSV_OUT")
if [[ -n "$BASELINE_SECONDS" && -n "$TARGET_SECONDS" ]]; then
  cmd+=(--baseline-seconds "$BASELINE_SECONDS" --target-seconds "$TARGET_SECONDS")
fi

"${cmd[@]}"
python3 "$RENDER_SCRIPT" "$CSV_OUT" "$SVG_OUT"

cat > "$INDEX_OUT" <<EOF
# Benchmark Visual Report

- baseline: $BASELINE_TAG
- target: $TARGET_TAG

## Compare Chart

![compare chart](./compare_chart.svg)

## Raw Report

See [compare_report.md](./compare_report.md)
EOF

echo "Visual report generated in: $OUT_DIR"

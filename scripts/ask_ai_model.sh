#!/usr/bin/env bash
set -euo pipefail

MODEL="${MYCHAT_AI_MODEL:-qwen2.5:0.5b}"
PROMPT="${1:-请用三句话介绍这个MyChat项目的亮点}"

curl -fsS "http://127.0.0.1:11434/api/generate" \
  -H 'Content-Type: application/json' \
  -d "{\"model\":\"${MODEL}\",\"prompt\":\"${PROMPT}\",\"stream\":false}" \
  | sed -n 's/.*"response":"\(.*\)","done".*/\1/p' \
  | sed 's/\\n/\n/g; s/\\"/"/g'

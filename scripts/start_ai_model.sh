#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL="${1:-qwen2.5:0.5b}"
COMPOSE_FILE="${ROOT_DIR}/ai/docker-compose.yml"

DOCKER_BIN="docker"
if ! docker info >/dev/null 2>&1; then
  DOCKER_BIN="sudo docker"
fi

echo "[1/4] starting ollama container..."
${DOCKER_BIN} compose -f "${COMPOSE_FILE}" up -d

echo "[2/4] waiting ollama api..."
for _ in {1..30}; do
  if curl -fsS "http://127.0.0.1:11434/api/tags" >/dev/null 2>&1; then
    break
  fi
  sleep 1
done

if ! curl -fsS "http://127.0.0.1:11434/api/tags" >/dev/null 2>&1; then
  echo "Ollama API is not ready on 11434"
  exit 1
fi

echo "[3/4] pulling model: ${MODEL} (first time may take several minutes)..."
${DOCKER_BIN} exec mychat-ollama ollama pull "${MODEL}"

echo "[4/4] smoke test prompt..."
RESP="$(curl -fsS "http://127.0.0.1:11434/api/generate" \
  -H 'Content-Type: application/json' \
  -d "{\"model\":\"${MODEL}\",\"prompt\":\"请用一句话介绍你自己\",\"stream\":false}")"

if echo "${RESP}" | grep -q '"response"'; then
  echo "AI model deployed successfully: ${MODEL}"
  echo "Endpoint: http://127.0.0.1:11434"
else
  echo "Model deployed but smoke test response unexpected"
  echo "${RESP}"
  exit 1
fi

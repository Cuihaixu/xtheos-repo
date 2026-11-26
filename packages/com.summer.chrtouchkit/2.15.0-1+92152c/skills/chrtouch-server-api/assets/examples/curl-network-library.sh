#!/bin/sh
set -eu

: "${CHR_LIBRARY_URL:?Set CHR_LIBRARY_URL to the API Base URL, for example http://aweme.rapp.lan/v1}"

ACTION="${1:-list}"
# 占位默认值仅为演示格式：ID 一律大写 UUID、禁前缀（见 references/network-library.md 第 1.1 节）。
# 实际使用请用环境变量传入服务端返回的真实 ID。
SERVER_LIBRARY_ID="${CHR_SERVER_LIBRARY_ID:-A0794FA8-D68E-4ECF-9D13-EF296F74D6CF}"
RECORD_GROUP_ID="${CHR_RECORD_GROUP_ID:-D3E4F5A6-7B8C-4D9E-0F1A-2B3C4D5E6F7A}"
RECORD_ID="${CHR_RECORD_ID:-A6B7C8D9-0E1F-4A2B-3C4D-5E6F7A8B9C0D}"
CHANGE_TOKEN="${CHR_CHANGE_TOKEN:-}"
IDEMPOTENCY_KEY="${CHR_IDEMPOTENCY_KEY:-550E8400-E29B-41D4-A716-446655440000}"
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BASE_URL=${CHR_LIBRARY_URL%/}

encode_segment() {
  python3 -c 'import sys, urllib.parse; print(urllib.parse.quote(sys.argv[1], safe=""))' "$1"
}

api_get() {
  curl --fail-with-body \
    --header 'Accept: application/json' "$1"
}

SERVER_LIBRARY_PATH=$(encode_segment "$SERVER_LIBRARY_ID")
RECORD_GROUP_PATH=$(encode_segment "$RECORD_GROUP_ID")
RECORD_PATH=$(encode_segment "$RECORD_ID")

case "$ACTION" in
  health)
    curl --fail-with-body --header 'Accept: application/json' "${BASE_URL}/health"
    ;;
  list)
    api_get "${BASE_URL}/network-libraries"
    ;;
  changes)
    if [ -z "$CHANGE_TOKEN" ]; then
      echo "Set CHR_CHANGE_TOKEN to a token returned by the list or changes API." >&2
      exit 2
    fi
    curl --fail-with-body --get \
      --header 'Accept: application/json' \
      --data-urlencode "since=${CHANGE_TOKEN}" \
      "${BASE_URL}/network-libraries/changes"
    ;;
  snapshot)
    api_get "${BASE_URL}/network-libraries/${SERVER_LIBRARY_PATH}/snapshot"
    ;;
  record)
    api_get "${BASE_URL}/network-libraries/${SERVER_LIBRARY_PATH}/records/${RECORD_PATH}"
    ;;
  create-record)
    curl --fail-with-body --request POST \
      --header "Idempotency-Key: ${IDEMPOTENCY_KEY}" \
      --header 'Content-Type: application/json; charset=utf-8' \
      --header 'Accept: application/json' \
      --data-binary "@${SCRIPT_DIR}/create-record.json" \
      "${BASE_URL}/network-libraries/${SERVER_LIBRARY_PATH}/record-groups/${RECORD_GROUP_PATH}/records"
    ;;
  delete-record)
    curl --fail-with-body --request DELETE \
      --header 'Accept: application/json' \
      "${BASE_URL}/network-libraries/${SERVER_LIBRARY_PATH}/records/${RECORD_PATH}"
    ;;
  create-library)
    curl --fail-with-body --request POST \
      --header "Idempotency-Key: ${IDEMPOTENCY_KEY}" \
      --header 'Content-Type: application/json; charset=utf-8' \
      --header 'Accept: application/json' \
      --data-binary "@${SCRIPT_DIR}/create-library.json" \
      "${BASE_URL}/network-libraries"
    ;;
  *)
    echo "Usage: $0 {health|list|changes|snapshot|record|create-record|delete-record|create-library}" >&2
    exit 2
    ;;
esac

#!/usr/bin/env bash
set -e

IMAGE=itch_parser
REBUILD=0
TARGET=itch_parser        # default target; override: ./run.sh <target>

# --- args: -b forces an image rebuild; any other arg is the cmake target ---
for arg in "$@"; do
    case "$arg" in
        -b) REBUILD=1 ;;
        *)  TARGET="$arg" ;;
    esac
done

# --- build the deps image ONLY when asked (-b) or when it doesn't exist yet ---
# Normal edit-run cycles skip this entirely: no context transfer, no cache growth.
if [ "$REBUILD" = "1" ] || [ -z "$(docker images -q "$IMAGE" 2>/dev/null)" ]; then
    echo ">> (re)building image '$IMAGE'"
    docker build -t "$IMAGE" .
fi

# --- compile + run inside a throwaway container ---
# Source (and build/) are bind-mounted from the host, so your edits are "transported"
# live and cmake recompiles ONLY the changed files. Container is discarded (--rm).
docker run --rm -v "$(pwd):/app" "$IMAGE" \
    sh -c "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null && \
           cmake --build build && \
           ./build/$TARGET"

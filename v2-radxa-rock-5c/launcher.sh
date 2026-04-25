#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# ——————————————————————————————————————————————————————
# Auto-detect install directory (console root)
INSTALL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FLUTTER_BIN="$INSTALL_DIR/shell-flutter"
LOGFILE="$INSTALL_DIR/launcher.log"

# Logger helper
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOGFILE"
}

# Rotate log
: > "$LOGFILE"
log "=== launcher.sh invoked with args: $* ==="
log "INSTALL_DIR=$INSTALL_DIR"
log "FLUTTER_BIN=$FLUTTER_BIN"

mode="${1:-}"
shift || true
log "Mode detected: '$mode'"

case "$mode" in

  shell)
    log "→ Launching Flutter UI"
    exec "$FLUTTER_BIN"
    ;;

  game)
    # 1) Get and log the game path
    game_path="${1:-}"
    log "→ Received game_path: '$game_path'"

    if [[ -z "$game_path" ]]; then
      log "ERROR: No game path supplied"
      echo "Usage: $0 game /full/path/to/run_game.sh" >&2
      log "→ Respawning Flutter UI"
      exec "$0" shell
    fi

    # 2) Kill Flutter if running
    log "→ Killing Flutter UI (if running)"
    if pkill -x -f "$FLUTTER_BIN"; then
      log "   Flutter UI killed"
    else
      log "   No Flutter UI process found"
    fi

    # 3) Validate the game executable exists & is runnable
    if [[ ! -x "$game_path" ]]; then
      log "ERROR: Game script not found or not executable: $game_path"
      echo "Game not found or not executable: $game_path" >&2
      log "→ Respawning Flutter UI"
      exec "$0" shell
    fi

    # 4) Run the game in background so we can fullscreen its window
    log "→ Running game in fullscreen: $game_path"
    "$game_path" >> "$LOGFILE" 2>&1 &
    GAME_PID=$!

    # Give it a moment to create its window
    sleep 1

    # Attempt to fullscreen via wmctrl
    if command -v wmctrl >/dev/null 2>&1; then
      log "   Using wmctrl to fullscreen the active window"
      wmctrl -r :ACTIVE: -b add,fullscreen
    elif command -v xdotool >/dev/null 2>&1; then
      log "   Using xdotool to send F11"
      WIN_ID=$(xdotool search --pid $GAME_PID | head -n1)
      if [[ -n "$WIN_ID" ]]; then
        xdotool windowactivate "$WIN_ID" key F11
      else
        log "   Warning: no window found for PID $GAME_PID"
      fi
    else
      log "   Warning: neither wmctrl nor xdotool found – cannot enforce fullscreen"
    fi

    # Wait for the game to exit, then respawn UI
    wait $GAME_PID
    rc=$?
    log "← Game exited with code $rc"

    log "→ Respawning Flutter UI"
    exec "$0" shell
    ;;

  *)
    log "ERROR: Invalid mode '$mode'"
    echo "Usage: $0 {shell|game /full/path/to/run_game.sh}" >&2
    exit 1
    ;;
esac

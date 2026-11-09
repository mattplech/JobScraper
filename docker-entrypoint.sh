#!/bin/sh
set -eu

interval=${JOBSCRAPER_INTERVAL_SECONDS:-43200}
case "$interval" in
    ''|*[!0-9]*)
        printf '%s\n' "JOBSCRAPER_INTERVAL_SECONDS must be a positive integer." >&2
        exit 64
        ;;
    *[1-9]*) ;;
    *)
        printf '%s\n' "JOBSCRAPER_INTERVAL_SECONDS must be greater than zero." >&2
        exit 64
        ;;
esac

child_pid=
stop() {
    trap - INT TERM
    if [ -n "$child_pid" ]; then
        kill "$child_pid" 2>/dev/null || true
        wait "$child_pid" 2>/dev/null || true
    fi
    exit 0
}
trap stop INT TERM

run_child() {
    "$@" &
    child_pid=$!
    if wait "$child_pid"; then
        status=0
    else
        status=$?
    fi
    child_pid=
    return "$status"
}

while :; do
    if run_child /opt/jobscraper/bin/JobScraper; then
        :
    else
        status=$?
        printf 'JobScraper exited with status %s; it will be retried after the interval.\n' "$status" >&2
    fi

    run_child sleep "$interval"
done
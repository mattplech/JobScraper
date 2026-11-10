#!/bin/bash
set -euo pipefail

workspace_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$workspace_dir"

echo "Setting up workspace in $workspace_dir..."

if [[ ! -e .secrets/db_password ]]; then
    mkdir -p .secrets
    chmod 700 .secrets

    umask 077
    openssl rand -hex 32 > .secrets/db_password
    chmod 444 .secrets/db_password
    echo "Generated a new database password and stored it in .secrets/db_password"
fi

if [[ ! -e run/config.json ]]; then
    mkdir -p run

    cp config.json run/config.json
fi
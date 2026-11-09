#!/bin/bash
set -euo pipefail

workspace_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$workspace_dir"

echo "Setting up workspace in $workspace_dir..."

mkdir -p .secrets
chmod 700 .secrets

if [[ ! -e .secrets/db_password ]]; then
    umask 077
    openssl rand -hex 32 > .secrets/db_password
    chmod 444 .secrets/db_password
    echo "Generated a new database password and stored it in .secrets/db_password"
fi

mkdir -p run

if [[ ! -e run/config.json ]]; then
    cp config.json run/config.json
fi
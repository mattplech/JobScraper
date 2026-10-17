#!/bin/bash
set -euo pipefail

mkdir -p .secrets
chmod 700 .secrets

if [[ -e .secrets/db_password ]]; then
    echo "Database password already exists in .secrets/db_password"
    exit 0
fi

# Generate a new random database password and store it in .secrets/db_password
umask 077
openssl rand -hex 32 > .secrets/db_password
chmod 400 .secrets/db_password

echo "Generated a new database password and stored it in .secrets/db_password"
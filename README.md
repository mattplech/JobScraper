<div align="center">

**English** · [čeština](docs/README.cs.md)

# Job Scraper

### Scrape job offers from popular Czech career portals

I started Job Scraper to bring job offers from multiple sites together in one place and make it easier for me to find a role that suits me. It also serves as a portfolio project showcasing my software development skills.

This project was developed with assistance from AI coding agents to help with code generation, debugging and documentation.

</div>

## Supported Sites

The currently supported sites are `jobs.cz` and `prace.cz`. Each can be enabled or disabled and configured with search filters. See [`config.example.json`](config.example.json) for the available fields.

## Requirements

- CMake 3.28 or newer
- A C++20 compiler
- [vcpkg](https://github.com/microsoft/vcpkg), bootstrapped and available through `VCPKG_ROOT`
- PostgreSQL to run the application
- Docker Compose to use the container setup

The dependencies are declared in [`vcpkg.json`](vcpkg.json) and are installed by vcpkg during CMake configuration.

## Quick Start

The [`compose.yaml`](compose.yaml) file can be used as-is from the repository root on the machine running the Docker daemon. Before starting the services:

1. Prepare a local password file and config copy:

   ```sh
   bash scripts/setup_workspace.sh
   ```

   This generates `.secrets/db_password` and copies `config.json` to `run/config.json` if those files do not already exist. Edit `run/config.json` to set your search filters.

2. Build the application image:

   ```sh
   docker build -t jobscraper:latest .
   ```

3. **Optional** - To browse stored offers with a PostgreSQL GUI such as [DBeaver Community](https://dbeaver.io/), add this port mapping under the `postgres` service in `compose.yaml`:

    ```yaml
    ports:
       - "15432:5432"
    ```

    In DBeaver, connect to `localhost`, port `15432`, database `jobscraper`, and user `jobscraper`. Use the password in `.secrets/db_password`.

4. Start the application and PostgreSQL:

   ```sh
   docker compose up -d
   ```

5. Follow the application logs:

   ```sh
   docker compose logs -f jobscraper
   ```

6. Stop the services when finished:

   ```sh
   docker compose down
   ```

The Compose file is an example configuration; adjust its settings to suit your environment if needed. PostgreSQL data is stored in the `postgres_data` named volume.

**Devcontainer note:** The `compose.yaml` uses relative paths for the config and password file. When Compose is run inside the devcontainer, these paths refer to files in the devcontainer's workspace, while the Docker daemon uses paths on the host. They may not point to the same files. Run the `compose.yaml` on the host from the repository root, or use `${HOST_WORKSPACE}`, which points to workspace on the host.

## Configuration

The application reads `config.json` from its current working directory by default. Set `JOBSCRAPER_CONFIG` to use another path. The file contains a `sites` array; each site has a name, an `enabled` flag, and filters. [`config.example.json`](config.example.json) shows the supported fields and example values.

For `work_location_type` use `ON_SITE`, `PARTIALLY_REMOTE`, or `FULL_REMOTE`, or leave it empty to omit this filter. To find a `location_ids` value, set the location filters on either site's website and copy the location ID from the search URL. If the copied ID contains `%3B`, replace it with `;` in the config. Both sites use the same search framework, so this method applies to both.

### Runtime Environment Variables

| Variable | Consumer | Description |
| --- | --- | --- |
| `JOBSCRAPER_HOST` | Application | PostgreSQL host; defaults to `localhost`. |
| `JOBSCRAPER_PORT` | Application | PostgreSQL port; defaults to `5432`. |
| `JOBSCRAPER_NAME` | Application | PostgreSQL database name; defaults to `jobscraper`. |
| `JOBSCRAPER_USER` | Application | PostgreSQL user; defaults to `jobscraper`. |
| `JOBSCRAPER_PASSWORD` | Application | PostgreSQL password. Prefer `JOBSCRAPER_PASSWORD_FILE` for a file-based secret. |
| `JOBSCRAPER_PASSWORD_FILE` | Application | Path to a file containing the PostgreSQL password. Provide this or `JOBSCRAPER_PASSWORD`; the direct password takes precedence if both are set. |
| `JOBSCRAPER_SSL_MODE` | Application | PostgreSQL SSL mode; defaults to `prefer`. |
| `JOBSCRAPER_CONFIG` | Application | Config file path; defaults to `config.json` in the current working directory. |
| `JOBSCRAPER_INTERVAL_SECONDS` | Container entrypoint | Delay after each run before the next; defaults to `43200` and must be a positive integer. |

### Development Environment Variables

These variables are used by the development setup and tests, not by the scraper's runtime configuration.

| Variable | Consumer | Description |
| --- | --- | --- |
| `HOST_WORKSPACE` | Development Compose | Absolute host path to the workspace, used to mount the config and database password file. Set by the devcontainer; set it when running `compose.dev.yaml` outside the devcontainer. |
| `TEST_FIXTURE_DIR` | Tests | Directory containing the adapter test HTML fixtures. |

## Build

Set `VCPKG_ROOT` to the location of your bootstrapped vcpkg checkout, then configure and build:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Tests

Run the test suite with CTest:

```sh
TEST_FIXTURE_DIR="$PWD/JobScraper/tests/fixtures" \
  ctest --test-dir build --output-on-failure
```

The database integration tests require a reachable PostgreSQL instance and the application database variables above, including a password or password file. They create and remove test schemas, so use a database account with permission to do that. The adapter tests use the fixtures under `JobScraper/tests/fixtures`.

## Development Container

Open the repository in VS Code with **Dev Containers** extension and choose **Reopen in Container**. The devcontainer starts its PostgreSQL service and runs `scripts/setup_workspace.sh` automatically when it starts, which creates `.secrets/db_password` and `run/config.json` if those files do not already exist. Its configuration is in [`.devcontainer/devcontainer.json`](.devcontainer/devcontainer.json).
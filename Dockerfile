# syntax=docker/dockerfile:1

# --------------------------------------------------
# Build stage
# --------------------------------------------------
FROM mcr.microsoft.com/devcontainers/cpp:3-debian13 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    bison \
    flex \
    autoconf \
    # Remove apt cache to reduce image size
    && rm -rf /var/lib/apt/lists/*

WORKDIR /jobscraper

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg
RUN /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

COPY ./ ./

# Install dependencies for the target platform
RUN vcpkg install \
    --triplet x64-linux \
    --x-install-root=/opt/vcpkg-installed

# Build the application using CMake and Ninja
RUN cmake -S . -B /build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_INSTALLED_DIR=/opt/vcpkg-installed \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_INSTALL_PREFIX=/opt/jobscraper \
    -DBUILD_TESTING=OFF

RUN cmake --build /build --parallel \
    && cmake --install /build

# Collect vcpkg shared libraries needed by the executable.
RUN ldd /opt/jobscraper/bin/JobScraper \
    | awk "/=> \// { print $3 }" \
    | grep "^/opt/vcpkg-installed/x64-linux/" \
    | sort -u > /tmp/vcpkg-runtime-libs.txt \
    && mkdir -p /opt/vcpkg-runtime-libs \
    && while IFS= read -r lib; do \
        cp -L "$lib" /opt/vcpkg-runtime-libs/; \
    done < /tmp/vcpkg-runtime-libs.txt

# --------------------------------------------------
# Runtime stage
# --------------------------------------------------
FROM debian:trixie-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    # Remove apt cache to reduce image size
    && rm -rf /var/lib/apt/lists/*

# Copy installed application files.
COPY --from=build /opt/jobscraper/ /opt/jobscraper/

COPY --from=build /opt/vcpkg-runtime-libs/ /opt/vcpkg-libs/
COPY --from=build --chmod=755 /jobscraper/docker-entrypoint.sh /usr/local/bin/jobscraper-entrypoint

ENV LD_LIBRARY_PATH="/opt/vcpkg-libs"

# Run the scraper every 12 hours by default
ENV JOBSCRAPER_INTERVAL_SECONDS=43200

WORKDIR /opt/jobscraper

# Create a dedicated application user and group.
RUN useradd --system \
       --home-dir /nonexistent \
       --shell /usr/sbin/nologin \
       jobscraper

USER jobscraper:jobscraper

ENTRYPOINT ["/usr/local/bin/jobscraper-entrypoint"]
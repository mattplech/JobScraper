# syntax=docker/dockerfile:1

# --------------------------------------------------
# Build stage
# --------------------------------------------------
FROM debian:trixie-slim AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    build-essential \
    cmake \
    ninja-build \
    zip \
    unzip \
    tar \
    curl \
    pkg-config \
    ca-certificates \
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
    -DCMAKE_INSTALL_PREFIX=/opt/jobscraper

RUN cmake --build /build --parallel \
    && cmake --install /build

# --------------------------------------------------
# Runtime stage
# --------------------------------------------------
FROM debian:trixie-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Copy installed application files.
COPY --from=build /opt/jobscraper/ /opt/jobscraper/

COPY --from=build /opt/vcpkg-installed/x64-linux/lib/ \
    /opt/vcpkg-libs/

ENV LD_LIBRARY_PATH="/opt/vcpkg-libs"

WORKDIR /opt/jobscraper

# Create a dedicated application user and group.
RUN useradd --system \
       --home-dir /nonexistent \
       --shell /usr/sbin/nologin \
       jobscraper

USER jobscraper:jobscraper

CMD ["/opt/jobscraper/bin/JobScraper"]
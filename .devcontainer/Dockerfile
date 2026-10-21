FROM mcr.microsoft.com/devcontainers/cpp:3-debian13

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    bison \
    flex \
    autoconf \
    # Remove apt cache to reduce image size
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspaces/JobScraper

CMD ["sleep", "infinity"]
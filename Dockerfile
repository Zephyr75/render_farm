# Use Ubuntu as base image for better development tool support
FROM ubuntu:22.04

# Set environment variables to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    cmake \
    wget \
    curl \
    git \
    libssl-dev \
    zlib1g-dev \
    libomp-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the source code
COPY . .

# Create a build script to compile the application
RUN echo '#!/bin/bash\n\
g++ -O3 -fopenmp -std=c++14 \\\n\
    -pthread \\\n\
    -I. \\\n\
    main.cpp \\\n\
    -o pathtracer \\\n\
    -lssl -lcrypto -lz\n\
' > build.sh && chmod +x build.sh

# Compile the application
RUN ./build.sh

# Expose the port
EXPOSE 18080

# Create a non-root user for security
RUN useradd -m -u 1000 pathtracer && \
    chown -R pathtracer:pathtracer /app

# Switch to non-root user
USER pathtracer

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:18080/health || exit 1

# Run the application
CMD ["./pathtracer"]

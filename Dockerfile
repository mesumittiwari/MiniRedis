# Stage 1: Build the binary
FROM gcc:11 AS builder
WORKDIR /build
COPY . .
RUN g++ -std=c++17 main.cpp store.cpp -O3 -pthread -o miniredis

# Stage 2: Lightweight runtime environment
FROM debian:bookworm-slim

# Store the executable safely away from the volume mount
WORKDIR /app
COPY --from=builder /build/miniredis /app/miniredis

# Switch to a dedicated data directory for volume persistence
WORKDIR /data
EXPOSE 6380

# Run the binary using its absolute path
CMD ["/app/miniredis"]

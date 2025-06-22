# FROM ubuntu:24.04

# RUN apt-get update \
#  && apt-get install -y --no-install-recommends libc6 libstdc++6 \
#  && rm -rf /var/lib/apt/lists/*

# WORKDIR /app
# COPY server           ./server
# COPY routing/routing  ./routing/routing
# COPY ./all_routes.json .
# COPY gtfs/ ./gtfs/
# RUN chmod +x server routing
# EXPOSE 3000
# ENTRYPOINT ["./server"]
# ─── STAGE 1: Builder on Alpine ──────────────────────────────────────────
FROM golang:1.24.4-alpine AS builder

# install C toolchain
RUN apk add --no-cache build-base

WORKDIR /workspace

# 1) Build C++ CLI (musl-linked)
COPY cpp_routing_sources/ ./routing/
WORKDIR /workspace/routing
RUN g++ -std=gnu++17 -O2 main.cpp -o routing

# 2) Build Go server (static binary)
WORKDIR /workspace
COPY go.mod go.sum ./
COPY main.go ./
COPY routing/ ./routing/
RUN go mod download
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -o server main.go

# ─── STAGE 2: Runtime on Alpine ──────────────────────────────────────────
FROM alpine:3.18

# we need libstdc++ for the C++ CLI
RUN apk add --no-cache libstdc++

WORKDIR /app

# copy binaries & data
COPY --from=builder /workspace/server           ./server
COPY --from=builder /workspace/routing/routing  ./cpp_binaries/routing
COPY all_routes.json                            .
COPY gtfs/                                      ./gtfs/

# ensure exec perms
RUN chmod +x server ./cpp_binaries/routing

EXPOSE 3000
ENTRYPOINT ["./server"]
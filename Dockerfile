FROM ubuntu:24.04

RUN apt-get update \
 && apt-get install -y --no-install-recommends libc6 libstdc++6 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY server           ./server
COPY routing/routing  ./routing/routing
COPY ./all_routes.json .
COPY gtfs/ ./gtfs/
RUN chmod +x server routing
EXPOSE 3000
ENTRYPOINT ["./server"]
## To create the docker image run :

```bash
docker build -t tram_routing_server_te
st .
```

---

## To start the server run :

```bash
docker run -d -p 3000:3000 tram_routing_server_test
```

---

## To call the api try:

```bash
https://127.0.0.1:3000/getPath?start={start_station}&end={end_station}&mode={mode}
```

replace {start_station} and {end_station} with id values from gtfs/stops.txt
replace {mode} with either "precomputed" or "realtime"

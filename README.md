# SafePath - Crime-Aware Route Planner

A DSA mini-project that computes **two driving routes** between Indian cities:

- Fastest Route - minimum distance
- Safer Route - avoids high-crime zones

Built with a pure C backend (graph + heap + Dijkstra) and a Leaflet.js + OSRM frontend.

## Features
- 27 nodes (9 cities x 3 zones)
- Real road routing via OSRM
- NCRB Crime in India 2024 data
- Crime type filters
- Intra-city and inter-city routing

## Setup
```bash
gcc src/safepath.c -o safepath
./safepath > map.html
```

Open `map.html` in your browser.

## License
MIT - see LICENSE.

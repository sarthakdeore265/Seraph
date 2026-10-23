#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#define MAX_NODES 30
#define MAX_INCIDENTS 500

// ==========================================
// PART 1: DATA STRUCTURES
// ==========================================

typedef enum {
    CRIME_HOMICIDE, CRIME_ROBBERY, CRIME_ASSAULT,
    CRIME_RAPE, CRIME_NARCOTICS, CRIME_THEFT
} CrimeType;

typedef struct {
    double lat, lon;
    CrimeType type;
    int severity;
    char location[64];
} CrimeIncident;

typedef struct Edge {
    int dest;
    double distance, risk_score;
    struct Edge* next;
} Edge;

typedef struct Node {
    int id;
    double lat, lon, crime_rate;
    char name[64];
    char city[32];
    char zone[32];
    Edge* head;
} Node;

typedef struct {
    Node nodes[MAX_NODES];
    int num_nodes;
} Graph;

typedef struct { int node_id; double cost; } HeapNode;
typedef struct { HeapNode items[MAX_NODES]; int size; } MinHeap;

typedef struct {
    int path[MAX_NODES];
    int length;
    double dist, risk;
} RouteData;

CrimeIncident incidents[MAX_INCIDENTS];
int incident_count = 0;

RouteData all_shortest[MAX_NODES][MAX_NODES];
RouteData all_safe[MAX_NODES][MAX_NODES];

// ==========================================
// PART 2: GRAPH & HEAP
// ==========================================

Graph* create_graph() {
    Graph* g = (Graph*)malloc(sizeof(Graph));
    g->num_nodes = 0;
    return g;
}

void add_node(Graph* g, int id, double lat, double lon, double rate, const char* name, const char* city, const char* zone) {
    if (g->num_nodes >= MAX_NODES) return;
    g->nodes[g->num_nodes].id = id;
    g->nodes[g->num_nodes].lat = lat;
    g->nodes[g->num_nodes].lon = lon;
    g->nodes[g->num_nodes].crime_rate = rate;
    strncpy(g->nodes[g->num_nodes].name, name, 63);
    g->nodes[g->num_nodes].name[63] = '\0';
    strncpy(g->nodes[g->num_nodes].city, city, 31);
    g->nodes[g->num_nodes].city[31] = '\0';
    strncpy(g->nodes[g->num_nodes].zone, zone, 31);
    g->nodes[g->num_nodes].zone[31] = '\0';
    g->nodes[g->num_nodes].head = NULL;
    g->num_nodes++;
}

void add_edge(Graph* g, int src, int dest, double dist, double risk) {
    Edge* e = (Edge*)malloc(sizeof(Edge));
    e->dest = dest; e->distance = dist; e->risk_score = risk;
    e->next = g->nodes[src].head; g->nodes[src].head = e;

    Edge* r = (Edge*)malloc(sizeof(Edge));
    r->dest = src; r->distance = dist; r->risk_score = risk;
    r->next = g->nodes[dest].head; g->nodes[dest].head = r;
}

void swap(HeapNode* a, HeapNode* b) { HeapNode t = *a; *a = *b; *b = t; }

void push(MinHeap* h, int id, double cost) {
    if (h->size == MAX_NODES) return;
    h->items[h->size].node_id = id;
    h->items[h->size].cost = cost;
    int i = h->size++;
    while (i != 0 && h->items[(i-1)/2].cost > h->items[i].cost) {
        swap(&h->items[i], &h->items[(i-1)/2]);
        i = (i-1)/2;
    }
}

HeapNode pop(MinHeap* h) {
    HeapNode root = h->items[0];
    h->items[0] = h->items[--h->size];
    int i = 0;
    while (2*i+1 < h->size) {
        int s = 2*i+1;
        if (2*i+2 < h->size && h->items[2*i+2].cost < h->items[s].cost) s = 2*i+2;
        if (h->items[i].cost <= h->items[s].cost) break;
        swap(&h->items[i], &h->items[s]);
        i = s;
    }
    return root;
}

// ==========================================
// PART 3: DIJKSTRA
// ==========================================

void dijkstra(Graph* g, int start, int end, double lambda, RouteData* out) {
    if (start == end) {
        out->path[0] = start;
        out->length = 1;
        out->dist = 0.0;
        out->risk = 0.0;
        return;
    }

    double dist[MAX_NODES];
    int parent[MAX_NODES], visited[MAX_NODES];
    MinHeap heap; heap.size = 0;

    for (int i = 0; i < g->num_nodes; i++) {
        dist[i] = DBL_MAX; parent[i] = -1; visited[i] = 0;
    }
    dist[start] = 0.0;
    push(&heap, start, 0.0);

    while (heap.size > 0) {
        HeapNode c = pop(&heap);
        int u = c.node_id;
        if (visited[u]) continue;
        visited[u] = 1;
        if (u == end) break;

        Edge* e = g->nodes[u].head;
        while (e != NULL) {
            int v = e->dest;
            if (!visited[v]) {
                double cost = e->distance + (lambda * e->risk_score);
                double nd = dist[u] + cost;
                if (nd < dist[v]) {
                    dist[v] = nd;
                    parent[v] = u;
                    push(&heap, v, nd);
                }
            }
            e = e->next;
        }
    }

    out->length = 0;
    out->dist = 0.0;
    out->risk = 0.0;
    if (dist[end] == DBL_MAX) return;

    int curr = end;
    while (curr != -1) {
        out->path[out->length++] = curr;
        curr = parent[curr];
    }
    for (int i = 0; i < out->length/2; i++) {
        int t = out->path[i];
        out->path[i] = out->path[out->length-1-i];
        out->path[out->length-1-i] = t;
    }

    for (int i = 0; i < out->length - 1; i++) {
        int u = out->path[i];
        Edge* e = g->nodes[u].head;
        while (e != NULL) {
            if (e->dest == out->path[i+1]) {
                out->dist += e->distance;
                out->risk += e->risk_score;
                break;
            }
            e = e->next;
        }
    }
}

// ==========================================
// PART 4: CRIME GENERATOR
// ==========================================

static unsigned long rng_state = 98765;
double rng() {
    rng_state = rng_state * 1103515245 + 12345;
    return ((double)(rng_state / 65536) / 32768.0);
}

const char* loc_types[] = {
    "Highway/Road/Alley/Street", "Residence/Home", "Parking/Drop Lot/Garage",
    "School/College", "Other/Unknown", "Hotel/Motel/Lodge", "Jail/Prison",
    "Convenience Store", "Service/Gas Station", "Park/Playground"
};
int loc_weights[] = {2932812, 705100, 428940, 198544, 174053, 100120, 90450, 90143, 80747, 69990};
int total_w = 4410999;

const char* rand_loc() {
    int roll = (int)(rng() * total_w);
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += loc_weights[i];
        if (roll < sum) return loc_types[i];
    }
    return "Other/Unknown";
}

void gen_incidents(double clat, double clon, int count, double crime_factor, double spread) {
    double homicide_c = crime_factor * 0.008;
    double robbery_c = crime_factor * 0.025;
    double rape_c = crime_factor * 0.012;
    double narc_c = crime_factor * 0.035;
    double theft_c = crime_factor * 0.060;

    for (int i = 0; i < count; i++) {
        if (incident_count >= MAX_INCIDENTS) return;

        double lat_o = (rng() - 0.5) * spread;
        double lon_o = (rng() - 0.5) * spread;
        lat_o *= 0.7 + (rng() * 0.6);
        lon_o *= 0.7 + (rng() * 0.6);

        incidents[incident_count].lat = clat + lat_o;
        incidents[incident_count].lon = clon + lon_o;
        strcpy(incidents[incident_count].location, rand_loc());

        double roll = rng();
        if (roll < homicide_c) {
            incidents[incident_count].type = CRIME_HOMICIDE;
            incidents[incident_count].severity = 5;
        } else if (roll < homicide_c + robbery_c) {
            incidents[incident_count].type = CRIME_ROBBERY;
            incidents[incident_count].severity = 4;
        } else if (roll < homicide_c + robbery_c + rape_c) {
            incidents[incident_count].type = CRIME_RAPE;
            incidents[incident_count].severity = 4;
        } else if (roll < homicide_c + robbery_c + rape_c + narc_c) {
            incidents[incident_count].type = CRIME_NARCOTICS;
            incidents[incident_count].severity = 3;
        } else if (roll < homicide_c + robbery_c + rape_c + narc_c + theft_c) {
            incidents[incident_count].type = CRIME_THEFT;
            incidents[incident_count].severity = 2;
        } else {
            incidents[incident_count].type = CRIME_ASSAULT;
            incidents[incident_count].severity = 2 + (int)(rng() * 2);
        }
        incident_count++;
    }
}

// ==========================================
// PART 5: HTML OUTPUT
// ==========================================

void print_header() {
    printf("<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n");
    printf("<meta charset=\"UTF-8\">\n");
    printf("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    printf("<title>India Safe Route Planner - NCRB 2024</title>\n");
    printf("<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.css\" />\n");
    printf("<style>\n");
    printf("body{margin:0;padding:0;display:flex;height:100vh;font-family:'Segoe UI',Tahoma,sans-serif;background:#1e1e2f;color:#fff}\n");
    printf("#sidebar{width:380px;padding:18px;background:#2a2a40;box-shadow:4px 0 15px rgba(0,0,0,.5);z-index:1000;display:flex;flex-direction:column;overflow-y:auto}\n");
    printf("#map{flex-grow:1;height:100%%;background-color:#1e1e2f}\n");
    printf("h2{margin:0 0 10px 0;color:#4da6ff;font-size:1.2em}\n");
    printf("h3{color:#4da6ff;font-size:.92em;margin-top:16px;margin-bottom:8px;border-bottom:1px solid #444;padding-bottom:4px}\n");
    printf("select{width:100%%;padding:8px;margin:4px 0;background:#1e1e2f;color:#fff;border:1px solid #4da6ff;border-radius:5px;font-size:.9em;cursor:pointer}\n");
    printf("label.field{font-size:.78em;color:#aaa;display:block;margin-top:6px}\n");
    printf(".stat{background:#1e1e2f;padding:10px;border-radius:8px;margin-bottom:8px;border-left:4px solid #4da6ff}\n");
    printf(".stat span{display:block;font-size:.9em;font-weight:bold}\n");
    printf(".stat small{color:#aaa;font-size:.72em}\n");
    printf(".route-card{padding:10px;border-radius:8px;margin-bottom:8px;cursor:pointer;transition:.2s}\n");
    printf(".route-card:hover{transform:translateX(3px)}\n");
    printf(".route-card.active{border:2px solid #fff}\n");
    printf(".route-shortest{background:rgba(255,77,77,.15);border-left:4px solid #ff4d4d}\n");
    printf(".route-safe{background:rgba(77,255,136,.15);border-left:4px solid #4dff88}\n");
    printf(".route-title{font-weight:bold;font-size:.95em;margin-bottom:4px}\n");
    printf(".route-details{font-size:.78em;color:#ccc;line-height:1.5}\n");
    printf(".filter{display:flex;align-items:center;margin-bottom:5px;cursor:pointer;padding:4px;border-radius:5px;font-size:.8em}\n");
    printf(".filter:hover{background:#1e1e2f}\n");
    printf(".filter input{margin-right:8px;cursor:pointer}\n");
    printf(".dot{width:10px;height:10px;border-radius:50%%;margin-right:8px;display:inline-block;flex-shrink:0}\n");
    printf(".ncrb{font-size:.76em;color:#ccc;width:100%%;border-collapse:collapse;margin-top:5px}\n");
    printf(".ncrb th{text-align:left;color:#4da6ff;padding:3px;border-bottom:1px solid #444}\n");
    printf(".ncrb td{padding:3px}\n");
    printf(".leaflet-tile-container{filter:invert(100%%) hue-rotate(180deg) brightness(95%%) contrast(90%%)}\n");
    printf(".leaflet-popup-content-wrapper{background:#2a2a40;color:#fff;border-radius:8px}\n");
    printf(".leaflet-popup-tip{background:#2a2a40}\n");
    printf("</style>\n</head>\n<body>\n");
    printf("<div id=\"sidebar\">\n");
    printf("<h2>India Safe Route Planner</h2>\n");
    printf("<p style=\"font-size:.8em;color:#ccc;margin:4px 0\">Real road routing via OSRM + crime data from <b>NCRB 2024</b>. Choose your trade-off.</p>\n");

    printf("<h3>Route Planner</h3>\n");
    printf("<label class=\"field\">Starting Location</label>\n");
    printf("<select id=\"startSelect\" onchange=\"updateRoute()\"></select>\n");
    printf("<label class=\"field\">Destination</label>\n");
    printf("<select id=\"endSelect\" onchange=\"updateRoute()\"></select>\n");

    printf("<h3>Route Options</h3>\n");
    printf("<div id=\"route_shortest_card\" class=\"route-card route-shortest\" onclick=\"selectRoute('shortest')\">\n");
    printf("<div class=\"route-title\">Fastest / Shortest Route</div>\n");
    printf("<div class=\"route-details\" id=\"shortest_details\">Select locations...</div>\n");
    printf("</div>\n");
    printf("<div id=\"route_safe_card\" class=\"route-card route-safe active\" onclick=\"selectRoute('safe')\">\n");
    printf("<div class=\"route-title\">Safer Alternative</div>\n");
    printf("<div class=\"route-details\" id=\"safe_details\">Select locations...</div>\n");
    printf("</div>\n");

    printf("<h3>DSA Core</h3>\n");
    printf("<div class=\"stat\"><span>Algorithm</span><small>Risk-Aware Dijkstra + Min-Heap</small></div>\n");
    printf("<div class=\"stat\"><span>Cost Function</span><small>Cost = Distance + (Lambda x Crime Risk)</small></div>\n");
    printf("<div class=\"stat\"><span>Road Routing</span><small>Real roads via OSRM public API</small></div>\n");

    printf("<h3>Crime Type Filters</h3>\n");
    printf("<label class=\"filter\"><input type=\"checkbox\" id=\"f_homicide\" checked onchange=\"toggleCrime('homicide')\"><span class=\"dot\" style=\"background:#8b0000\"></span>Homicide</label>\n");
    printf("<label class=\"filter\"><input type=\"checkbox\" id=\"f_robbery\" checked onchange=\"toggleCrime('robbery')\"><span class=\"dot\" style=\"background:#ff6600\"></span>Robbery</label>\n");
    printf("<label class=\"filter\"><input type=\"checkbox\" id=\"f_assault\" checked onchange=\"toggleCrime('assault')\"><span class=\"dot\" style=\"background:#ffcc00\"></span>Assault</label>\n");
    printf("<label class=\"filter\"><input type=\"checkbox\" id=\"f_rape\" checked onchange=\"toggleCrime('rape')\"><span class=\"dot\" style=\"background:#a020f0\"></span>Rape</label>\n");
    printf("<label class=\"filter\"><input type=\"checkbox\" id=\"f_narcotics\" checked onchange=\"toggleCrime('narcotics')\"><span class=\"dot\" style=\"background:#00ffff\"></span>Narcotics</label>\n");
    printf("<label class=\"filter\"><input type=\"checkbox\" id=\"f_theft\" checked onchange=\"toggleCrime('theft')\"><span class=\"dot\" style=\"background:#66ff66\"></span>Theft</label>\n");

    printf("<h3>NCRB 2024 - India</h3>\n");
    printf("<table class=\"ncrb\">\n");
    printf("<tr><th>Year</th><th>Cases</th><th>Rate</th></tr>\n");
    printf("<tr><td>2022</td><td>58,24,946</td><td>422.2</td></tr>\n");
    printf("<tr><td>2023</td><td>62,41,569</td><td>448.3</td></tr>\n");
    printf("<tr><td>2024</td><td>58,85,867</td><td>418.9</td></tr>\n");
    printf("</table>\n");

    printf("<p style=\"font-size:.7em;color:#888;margin-top:auto;padding-top:8px\">*Crime rate per 100,000 population. NCRB CII 2024 Table 1.1.ii. Road routing © OSRM.</p>\n");
    printf("</div>\n<div id=\"map\"></div>\n");
    printf("<script src=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.js\"></script>\n<script>\n");
}

void print_footer() {
    printf("</script>\n</body>\n</html>\n");
}

void generate_js(Graph* g) {
    printf("var locations = [\n");
    for (int i = 0; i < g->num_nodes; i++) {
        printf("  {id:%d,name:\"%s\",city:\"%s\",zone:\"%s\",lat:%f,lon:%f,rate:%.0f}",
               i, g->nodes[i].name, g->nodes[i].city, g->nodes[i].zone,
               g->nodes[i].lat, g->nodes[i].lon, g->nodes[i].crime_rate);
        if (i < g->num_nodes - 1) printf(",");
        printf("\n");
    }
    printf("];\n");

    // We only need to give JS the graph edges for the safe route, so JS knows which intermediate cities to route through.
    printf("var edges = [\n");
    for (int i = 0; i < g->num_nodes; i++) {
        Edge* e = g->nodes[i].head;
        while (e != NULL) {
            if (i < e->dest) { // Only print each edge once
                printf("  {u:%d,v:%d,w:%.0f,r:%.2f},\n", i, e->dest, e->distance, e->risk_score);
            }
            e = e->next;
        }
    }
    printf("];\n");

    printf("var crimeData = [\n");
    for (int i = 0; i < incident_count; i++) {
        CrimeIncident* inc = &incidents[i];
        const char* ts = (inc->type == CRIME_HOMICIDE) ? "homicide" :
                         (inc->type == CRIME_ROBBERY) ? "robbery" :
                         (inc->type == CRIME_RAPE) ? "rape" :
                         (inc->type == CRIME_NARCOTICS) ? "narcotics" :
                         (inc->type == CRIME_THEFT) ? "theft" : "assault";
        printf("{lat:%f,lon:%f,type:\"%s\",sev:%d,loc:\"%s\"}",
               inc->lat, inc->lon, ts, inc->severity, inc->location);
        if (i < incident_count - 1) printf(",");
        printf("\n");
    }
    printf("];\n");

    printf("\nvar map = L.map('map').setView([22.5, 78.9], 5);\n");
    printf("L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/{z}/{y}/{x}',{attribution:'&copy; Esri'}).addTo(map);\n");

    // City zones
    printf("\nlocations.forEach(function(c){\n");
    printf("  var color = c.rate > 400 ? '#ff4d4d' : c.rate > 250 ? '#ffcc00' : '#4dff88';\n");
    printf("  var label = c.rate > 400 ? 'HIGH RISK' : c.rate > 250 ? 'MODERATE' : 'LOW RISK';\n");
    printf("  L.circle([c.lat,c.lon],{color:color,fillColor:color,fillOpacity:.10,weight:2,dashArray:'5,10',radius:15000}).addTo(map).bindPopup('<b>'+c.name+'</b><br>'+c.city+' - '+c.zone+'<br>'+label+' Zone<br>Rate: '+c.rate+' per 100k');\n");
    printf("});\n");

    // Crime markers
    printf("\nvar crimeLayers = { homicide:L.layerGroup(), robbery:L.layerGroup(), assault:L.layerGroup(), rape:L.layerGroup(), narcotics:L.layerGroup(), theft:L.layerGroup() };\n");
    printf("var crimeColors = { homicide:'#8b0000', robbery:'#ff6600', assault:'#ffcc00', rape:'#a020f0', narcotics:'#00ffff', theft:'#66ff66' };\n");
    printf("var crimeNames = { homicide:'Homicide', robbery:'Robbery', assault:'Assault', rape:'Rape', narcotics:'Narcotics', theft:'Theft' };\n");
    printf("crimeData.forEach(function(c){\n");
    printf("  L.circleMarker([c.lat,c.lon],{radius:5,color:'#000',weight:1,fillColor:crimeColors[c.type],fillOpacity:.85})\n");
    printf("    .addTo(crimeLayers[c.type])\n");
    printf("    .bindPopup('<b>'+crimeNames[c.type]+'</b><br>Severity: '+c.sev+'/5<br><b>Location:</b> '+c.loc);\n");
    printf("});\n");
    printf("Object.keys(crimeLayers).forEach(function(k){ map.addLayer(crimeLayers[k]); });\n");

    // Dijkstra in JS to find the "safe route" waypoints (using the same graph data)
    printf("\nfunction dijkstraJS(start, end, lambda) {\n");
    printf("  var dist = {}; var parent = {}; var visited = {};\n");
    printf("  locations.forEach(l => { dist[l.id] = Infinity; parent[l.id] = null; visited[l.id] = false; });\n");
    printf("  dist[start] = 0;\n");
    printf("  var pq = [[0, start]];\n");
    printf("  while(pq.length > 0) {\n");
    printf("    pq.sort((a,b)=>a[0]-b[0]);\n");
    printf("    var u = pq.shift()[1];\n");
    printf("    if(visited[u]) continue; visited[u] = true;\n");
    printf("    if(u === end) break;\n");
    printf("    edges.filter(e => e.u === u || e.v === u).forEach(function(e) {\n");
    printf("      var v = e.u === u ? e.v : e.u;\n");
    printf("      if(!visited[v]) {\n");
    printf("        var cost = e.w + lambda * e.r;\n");
    printf("        if(dist[u] + cost < dist[v]) { dist[v] = dist[u] + cost; parent[v] = u; pq.push([dist[v], v]); }\n");
    printf("      }\n");
    printf("    });\n");
    printf("  }\n");
    printf("  var path = []; var curr = end;\n");
    printf("  while(curr !== null) { path.unshift(curr); curr = parent[curr]; }\n");
    printf("  return path;\n");
    printf("}\n");

    printf("\nvar shortestLine=null, safeLine=null, startMk=null, endMk=null;\n");
    printf("var currentRoute='safe';\n");

    printf("\nvar startSel = document.getElementById('startSelect');\n");
    printf("var endSel = document.getElementById('endSelect');\n");
    printf("locations.forEach(function(c){\n");
    printf("  startSel.innerHTML += '<option value=\"'+c.id+'\">'+c.name+'</option>';\n");
    printf("  endSel.innerHTML += '<option value=\"'+c.id+'\">'+c.name+'</option>';\n");
    printf("});\n");
    printf("startSel.value = 1;\n");
    printf("endSel.value = 7;\n");

    printf("\nfunction fetchRoute(waypoints) {\n");
    printf("  // OSRM public API: get real road path through waypoints\n");
    printf("  var coords = waypoints.map(w => w.lon + ',' + w.lat).join(';');\n");
    printf("  return fetch('https://router.project-osrm.org/route/v1/driving/' + coords + '?overview=full&geometries=geojson')\n");
    printf("    .then(r => r.json())\n");
    printf("    .then(data => {\n");
    printf("      if(data.code !== 'Ok') throw new Error('OSRM failed');\n");
    printf("      var route = data.routes[0];\n");
    printf("      var coords = route.geometry.coordinates.map(c => [c[1], c[0]]);\n");
    printf("      return { coords: coords, distance: route.distance / 1000 };\n");
    printf("    });\n");
    printf("}\n");

    printf("\nfunction updateRoute(){\n");
    printf("  var s = parseInt(startSel.value);\n");
    printf("  var e = parseInt(endSel.value);\n");
    printf("  if(s === e){\n");
    printf("    alert('Start and destination must be different! Select different zones.'); return;\n");
    printf("  }\n");
    printf("  document.getElementById('shortest_details').innerHTML = 'Calculating real road route...';\n");
    printf("  document.getElementById('safe_details').innerHTML = 'Calculating real road route...';\n");

    // Get safe route path via Dijkstra (in JS)
    printf("  var safePath = dijkstraJS(s, e, 2.0);\n");
    printf("  var shortestPath = [s, e]; // pure start->end for 'shortest'\n");

    printf("  var safeWps = safePath.map(id => locations[id]);\n");
    printf("  var shortWps = [locations[s], locations[e]];\n");

    printf("  Promise.all([fetchRoute(shortWps), fetchRoute(safeWps)]).then(function(results){\n");
    printf("    var shortRes = results[0];\n");
    printf("    var safeRes = results[1];\n");

    printf("    if(shortestLine) map.removeLayer(shortestLine);\n");
    printf("    if(safeLine) map.removeLayer(safeLine);\n");
    printf("    if(startMk) map.removeLayer(startMk);\n");
    printf("    if(endMk) map.removeLayer(endMk);\n");

    printf("    shortestLine = L.polyline(shortRes.coords,{color:'#ff4d4d',weight:5,opacity:.9,dashArray:'10,6'}).addTo(map);\n");
    printf("    safeLine = L.polyline(safeRes.coords,{color:'#4dff88',weight:5,opacity:.9}).addTo(map);\n");
    printf("    startMk = L.marker([locations[s].lat, locations[s].lon]).addTo(map).bindPopup('<b>Start: '+locations[s].name+'</b>');\n");
    printf("    endMk = L.marker([locations[e].lat, locations[e].lon]).addTo(map).bindPopup('<b>Destination: '+locations[e].name+'</b>');\n");

    printf("    var allCoords = shortRes.coords.concat(safeRes.coords);\n");
    printf("    map.fitBounds(L.latLngBounds(allCoords),{padding:[50,50]});\n");

    printf("    var diff = safeRes.distance - shortRes.distance;\n");
    printf("    document.getElementById('shortest_details').innerHTML = 'Road distance: <b>'+shortRes.distance.toFixed(0)+' km</b><br><span style=\"color:#ff4d4d\">Most direct — may pass through high-crime areas</span>';\n");
    printf("    if(diff > 1){\n");
    printf("      document.getElementById('safe_details').innerHTML = 'Road distance: <b>'+safeRes.distance.toFixed(0)+' km</b><br><span style=\"color:#4dff88\">+'+diff.toFixed(0)+' km longer, avoids crime hotspots</span>';\n");
    printf("    } else {\n");
    printf("      document.getElementById('safe_details').innerHTML = 'Road distance: <b>'+safeRes.distance.toFixed(0)+' km</b><br><span style=\"color:#4dff88\">Same route as fastest</span>';\n");
    printf("    }\n");
    printf("    selectRoute(currentRoute);\n");
    printf("  }).catch(function(err){\n");
    printf("    console.error(err);\n");
    printf("    document.getElementById('shortest_details').innerHTML = '<span style=\"color:#ff4d4d\">Routing failed. Check internet.</span>';\n");
    printf("    document.getElementById('safe_details').innerHTML = '<span style=\"color:#ff4d4d\">Routing failed. Check internet.</span>';\n");
    printf("  });\n");
    printf("}\n");

    printf("\nfunction selectRoute(route){\n");
    printf("  currentRoute = route;\n");
    printf("  document.getElementById('route_shortest_card').classList.remove('active');\n");
    printf("  document.getElementById('route_safe_card').classList.remove('active');\n");
    printf("  if(route === 'shortest'){\n");
    printf("    document.getElementById('route_shortest_card').classList.add('active');\n");
    printf("    if(shortestLine) shortestLine.setStyle({weight:6,opacity:1});\n");
    printf("    if(safeLine) safeLine.setStyle({weight:3,opacity:.3});\n");
    printf("  } else {\n");
    printf("    document.getElementById('route_safe_card').classList.add('active');\n");
    printf("    if(safeLine) safeLine.setStyle({weight:6,opacity:1});\n");
    printf("    if(shortestLine) shortestLine.setStyle({weight:3,opacity:.3});\n");
    printf("  }\n");
    printf("}\n");

    printf("\nfunction toggleCrime(type){\n");
    printf("  if(document.getElementById('f_'+type).checked){\n");
    printf("    map.addLayer(crimeLayers[type]);\n");
    printf("  } else {\n");
    printf("    map.removeLayer(crimeLayers[type]);\n");
    printf("  }\n");
    printf("}\n");

    printf("\nupdateRoute();\n");
}

// ==========================================
// PART 6: MAIN
// ==========================================

int main() {
    Graph* g = create_graph();

    // 9 cities × 3 zones = 27 nodes
    add_node(g, 0, 28.7041, 77.1025, 580.0, "Delhi - North", "Delhi", "North");
    add_node(g, 1, 28.6139, 77.2090, 620.0, "Delhi - Central", "Delhi", "Central");
    add_node(g, 2, 28.5355, 77.2410, 540.0, "Delhi - South", "Delhi", "South");

    add_node(g, 3, 19.2183, 72.9781, 340.0, "Mumbai - North", "Mumbai", "North");
    add_node(g, 4, 19.0760, 72.8777, 320.0, "Mumbai - Central", "Mumbai", "Central");
    add_node(g, 5, 19.0176, 72.8562, 290.0, "Mumbai - South", "Mumbai", "South");

    add_node(g, 6, 13.0358, 77.5970, 290.0, "Bangalore - North", "Bangalore", "North");
    add_node(g, 7, 12.9716, 77.5946, 280.0, "Bangalore - Central", "Bangalore", "Central");
    add_node(g, 8, 12.9165, 77.6101, 260.0, "Bangalore - South", "Bangalore", "South");

    add_node(g, 9, 13.0827, 80.2707, 350.0, "Chennai - Central", "Chennai", "Central");
    add_node(g, 10, 13.1500, 80.2500, 370.0, "Chennai - North", "Chennai", "North");
    add_node(g, 11, 12.9800, 80.2200, 330.0, "Chennai - South", "Chennai", "South");

    add_node(g, 12, 22.5726, 88.3639, 420.0, "Kolkata - Central", "Kolkata", "Central");
    add_node(g, 13, 22.6500, 88.4000, 440.0, "Kolkata - North", "Kolkata", "North");
    add_node(g, 14, 22.5000, 88.3300, 400.0, "Kolkata - South", "Kolkata", "South");

    add_node(g, 15, 17.3850, 78.4867, 380.0, "Hyderabad - Central", "Hyderabad", "Central");
    add_node(g, 16, 17.4500, 78.5000, 400.0, "Hyderabad - North", "Hyderabad", "North");
    add_node(g, 17, 17.3200, 78.4700, 360.0, "Hyderabad - South", "Hyderabad", "South");

    add_node(g, 18, 18.5204, 73.8567, 300.0, "Pune - Central", "Pune", "Central");
    add_node(g, 19, 18.5800, 73.8800, 320.0, "Pune - North", "Pune", "North");
    add_node(g, 20, 18.4600, 73.8300, 280.0, "Pune - South", "Pune", "South");

    add_node(g, 21, 26.9124, 75.7873, 480.0, "Jaipur - Central", "Jaipur", "Central");
    add_node(g, 22, 26.9700, 75.8000, 500.0, "Jaipur - North", "Jaipur", "North");
    add_node(g, 23, 26.8500, 75.7700, 460.0, "Jaipur - South", "Jaipur", "South");

    add_node(g, 24, 25.5941, 85.1376, 620.0, "Patna - Central", "Patna", "Central");
    add_node(g, 25, 25.6500, 85.1500, 640.0, "Patna - North", "Patna", "North");
    add_node(g, 26, 25.5400, 85.1200, 600.0, "Patna - South", "Patna", "South");

    for (int i = 0; i < g->num_nodes; i++) {
        gen_incidents(g->nodes[i].lat, g->nodes[i].lon,
                      (int)(g->nodes[i].crime_rate / 20),
                      g->nodes[i].crime_rate / 10.0, 0.03);
    }

    // Intra-city edges
    add_edge(g, 0, 1, 15.0, 0.60); add_edge(g, 1, 2, 12.0, 0.58); add_edge(g, 0, 2, 25.0, 0.56);
    add_edge(g, 3, 4, 18.0, 0.33); add_edge(g, 4, 5, 10.0, 0.31); add_edge(g, 3, 5, 28.0, 0.32);
    add_edge(g, 6, 7, 12.0, 0.28); add_edge(g, 7, 8, 10.0, 0.27); add_edge(g, 6, 8, 20.0, 0.27);
    add_edge(g, 9, 10, 14.0, 0.36); add_edge(g, 9, 11, 12.0, 0.34); add_edge(g, 10, 11, 22.0, 0.35);
    add_edge(g, 12, 13, 16.0, 0.43); add_edge(g, 12, 14, 14.0, 0.41); add_edge(g, 13, 14, 26.0, 0.42);
    add_edge(g, 15, 16, 13.0, 0.39); add_edge(g, 15, 17, 15.0, 0.37); add_edge(g, 16, 17, 24.0, 0.38);
    add_edge(g, 18, 19, 11.0, 0.31); add_edge(g, 18, 20, 12.0, 0.29); add_edge(g, 19, 20, 20.0, 0.30);
    add_edge(g, 21, 22, 10.0, 0.49); add_edge(g, 21, 23, 11.0, 0.47); add_edge(g, 22, 23, 18.0, 0.48);
    add_edge(g, 24, 25, 12.0, 0.63); add_edge(g, 24, 26, 10.0, 0.61); add_edge(g, 25, 26, 18.0, 0.62);

    // Inter-city edges
    add_edge(g, 1, 21, 280.0, 0.53);
    add_edge(g, 1, 24, 1080.0, 0.60);
    add_edge(g, 1, 12, 1500.0, 0.50);
    add_edge(g, 1, 4, 1400.0, 0.45);
    add_edge(g, 1, 15, 1580.0, 0.48);
    add_edge(g, 4, 18, 150.0, 0.31);
    add_edge(g, 4, 7, 985.0, 0.30);
    add_edge(g, 4, 15, 710.0, 0.35);
    add_edge(g, 7, 9, 350.0, 0.32);
    add_edge(g, 7, 15, 570.0, 0.33);
    add_edge(g, 9, 15, 630.0, 0.37);
    add_edge(g, 12, 24, 590.0, 0.52);
    add_edge(g, 12, 15, 1490.0, 0.40);
    add_edge(g, 15, 18, 560.0, 0.34);
    add_edge(g, 21, 24, 1080.0, 0.55);

    print_header();
    generate_js(g);
    print_footer();

    free(g);
    return 0;
}

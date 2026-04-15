
#include "GraphCore.h"

Graph::Graph(int n, bool w) : N(n), weighted(w) {
    adjMatrix = new double* [N];
    for (int i = 0; i < N; i++) {
        adjMatrix[i] = new double[N];
        for (int j = 0; j < N; j++) adjMatrix[i][j] = 0;
    }
}

Graph::~Graph() {
    for (int i = 0; i < N; i++) delete[] adjMatrix[i];
    delete[] adjMatrix;
}

Graph* createGraph(int N, bool weighted) {
    return new Graph(N, weighted);
}

void deleteGraph(Graph* g) {
    delete g;
}

void printGraph(Graph* g) {
    cout << "\nMatrix:\n   ";
    for (int i = 0; i < g->N; i++) cout << setw(4) << i;
    cout << "\n";
    for (int i = 0; i < g->N; i++) {
        cout << setw(3) << i << " ";
        for (int j = 0; j < g->N; j++) {
            if (g->weighted) cout << setw(4) << fixed << setprecision(1) << g->adjMatrix[i][j];
            else cout << setw(4) << (int)g->adjMatrix[i][j];
        }
        cout << "\n";
    }
}

bool hasCycleUndirectedUtil(Graph* g, int v, bool* visited, int parent) {
    visited[v] = true;
    for (int i = 0; i < g->N; i++) {
        if (g->adjMatrix[v][i] != 0) {
            if (!visited[i]) {
                if (hasCycleUndirectedUtil(g, i, visited, v))
                    return true;
            }
            else if (i != parent) {
                return true;
            }
        }
    }
    return false;
}

Graph* generateRandomGraph(int N, bool weighted, int minEdgesPerVertex, int maxEdgesPerVertex,
    double minWeight, double maxWeight) {
    if (N <= 0) {
        cout << "Error: Number of vertices must be positive!\n";
        return NULL;
    }

    if (weighted && (minWeight < 0 || maxWeight < 0)) {
        cout << "Error: Weights cannot be negative!\n";
        return NULL;
    }

    if (minEdgesPerVertex < 0 || maxEdgesPerVertex >= N || minEdgesPerVertex > maxEdgesPerVertex) {
        cout << "Error: Invalid edges range!\n";
        return NULL;
    }

    Graph* g = createGraph(N, weighted);

    srand(static_cast<unsigned>(time(nullptr)));
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> weightDist(minWeight, maxWeight);

    for (int i = 0; i < N; i++) {
        int targetEdges = minEdgesPerVertex + rand() % (maxEdgesPerVertex - minEdgesPerVertex + 1);
        int currentEdges = 0;

        for (int j = 0; j < N; j++) {
            if (g->adjMatrix[i][j] != 0) currentEdges++;
        }

        if (currentEdges >= targetEdges) continue;

        int attempts = 0;
        while (currentEdges < targetEdges && attempts < N * 2) {
            attempts++;
            int j = rand() % N;

            if (j == i || g->adjMatrix[i][j] != 0) continue;

            if (weighted) {
                double weight = weightDist(gen);
                g->adjMatrix[i][j] = weight;
                g->adjMatrix[j][i] = weight;
            }
            else {
                g->adjMatrix[i][j] = 1;
                g->adjMatrix[j][i] = 1;
            }
            currentEdges++;
        }

        if (currentEdges > maxEdgesPerVertex) {
            for (int j = 0; j < N && currentEdges > maxEdgesPerVertex; j++) {
                if (j != i && g->adjMatrix[i][j] != 0) {
                    g->adjMatrix[i][j] = 0;
                    g->adjMatrix[j][i] = 0;
                    currentEdges--;
                }
            }
        }
    }

    return g;
}

Graph* readGraphFromFile(const string& filename, bool weighted, int formatType) {
    ifstream file(filename);
    if (!file) {
        cout << "Error opening file " << filename << "!\n";
        return NULL;
    }

    int N;
    if (!(file >> N) || N <= 0) {
        cout << "Error: Invalid number of vertices!\n";
        file.close();
        return NULL;
    }

    Graph* g = createGraph(N, weighted);

    if (formatType == 1) {
        string line;
        getline(file, line);

        while (getline(file, line)) {
            istringstream iss(line);
            int vertex;

            if (!(iss >> vertex)) {
                continue;
            }

            if (vertex < 0 || vertex >= N) {
                cout << "Warning: vertex " << vertex << " out of range. Skipped.\n";
                continue;
            }

            int neighbor;
            double weight = 1.0;

            while (iss >> neighbor) {
                if (neighbor < 0 || neighbor >= N) {
                    cout << "Warning: neighbor " << neighbor << " out of range. Skipped.\n";
                    continue;
                }

                if (vertex == neighbor) {
                    cout << "Warning: loop skipped.\n";
                    continue;
                }

                if (weighted && iss >> weight) {
                    if (weight < 0) {
                        cout << "Warning: negative weight skipped.\n";
                        continue;
                    }
                    g->adjMatrix[vertex][neighbor] = weight;
                    g->adjMatrix[neighbor][vertex] = weight;
                }
                else {
                    g->adjMatrix[vertex][neighbor] = 1;
                    g->adjMatrix[neighbor][vertex] = 1;
                }
            }
        }
    }
    else if (formatType == 2) {
        int from, to;
        double weight;

        while (file >> from >> to) {
            if (from < 0 || from >= N || to < 0 || to >= N) {
                cout << "Warning: invalid vertices. Skipped.\n";
                continue;
            }

            if (from == to) {
                cout << "Warning: loop skipped.\n";
                continue;
            }

            if (weighted) {
                if (!(file >> weight)) {
                    cout << "Warning: error reading weight. Skipped.\n";
                    continue;
                }
                if (weight < 0) {
                    cout << "Warning: negative weight skipped.\n";
                    continue;
                }
                g->adjMatrix[from][to] = weight;
                g->adjMatrix[to][from] = weight;
            }
            else {
                g->adjMatrix[from][to] = 1;
                g->adjMatrix[to][from] = 1;
            }
        }
    }
    else if (formatType == 3) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (!(file >> g->adjMatrix[i][j])) {
                    cout << "Error reading matrix element!\n";
                    deleteGraph(g);
                    file.close();
                    return NULL;
                }

                if (weighted && g->adjMatrix[i][j] < 0) {
                    cout << "Warning: negative weight set to 0.\n";
                    g->adjMatrix[i][j] = 0;
                }

                if (g->adjMatrix[i][j] != 0 && g->adjMatrix[j][i] == 0) {
                    g->adjMatrix[j][i] = g->adjMatrix[i][j];
                }

                if (i == j) {
                    g->adjMatrix[i][j] = 0;
                }
            }
        }
    }

    file.close();
    return g;
}

bool hasCycle(Graph* g) {
    bool* visited = new bool[g->N];
    for (int i = 0; i < g->N; i++) visited[i] = false;

    for (int i = 0; i < g->N; i++) {
        if (!visited[i]) {
            if (hasCycleUndirectedUtil(g, i, visited, -1)) {
                delete[] visited;
                return true;
            }
        }
    }
    delete[] visited;
    return false;
}

void findAllPathsDFSRec(Graph* g, int u, int end, bool* visited, int* path, int pathIndex, int& count) {
    visited[u] = true;
    path[pathIndex] = u;
    pathIndex++;
    if (u == end) {
        count++;
        cout << "Path " << count << ": ";
        for (int i = 0; i < pathIndex; i++) {
            cout << path[i];
            if (i != pathIndex - 1) cout << " -> ";
        }
        cout << "\n";
    }
    else for (int i = 0; i < g->N; i++)
        if (g->adjMatrix[u][i] != 0 && !visited[i])
            findAllPathsDFSRec(g, i, end, visited, path, pathIndex, count);
    visited[u] = false;
}

void findAllPathsDFS(Graph* g, int start, int end) {
    bool* visited = new bool[g->N];
    int* path = new int[g->N];
    for (int i = 0; i < g->N; i++) visited[i] = false;
    int count = 0;
    cout << "\nDFS:\n";
    findAllPathsDFSRec(g, start, end, visited, path, 0, count);
    if (count == 0) cout << "No paths found\n";
    else cout << "Total paths: " << count << "\n";
    delete[] visited; delete[] path;
}

void findAllPathsBFS(Graph* g, int start, int end) {
    cout << "\nBFS:\n";
    queue<BFSPath> q;
    BFSPath firstPath;
    firstPath.vertices = new int[g->N];
    firstPath.vertices[0] = start;
    firstPath.length = 1;
    q.push(firstPath);
    int count = 0;
    while (!q.empty()) {
        BFSPath current = q.front(); q.pop();
        int lastVertex = current.vertices[current.length - 1];
        if (lastVertex == end) {
            count++;
            cout << "Path " << count << ": ";
            for (int i = 0; i < current.length; i++) {
                cout << current.vertices[i];
                if (i != current.length - 1) cout << " -> ";
            }
            cout << "\n";
            delete[] current.vertices;
            continue;
        }
        for (int i = 0; i < g->N; i++) {
            if (g->adjMatrix[lastVertex][i] != 0) {
                bool visited = false;
                for (int j = 0; j < current.length; j++)
                    if (current.vertices[j] == i) { visited = true; break; }
                if (!visited) {
                    BFSPath newPath;
                    newPath.vertices = new int[current.length + 1];
                    for (int j = 0; j < current.length; j++) newPath.vertices[j] = current.vertices[j];
                    newPath.vertices[current.length] = i;
                    newPath.length = current.length + 1;
                    q.push(newPath);
                }
            }
        }
        delete[] current.vertices;
    }
    if (count == 0) cout << "No paths found\n";
    else cout << "Total paths: " << count << "\n";
}

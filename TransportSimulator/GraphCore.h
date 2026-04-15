#pragma once
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <random>
#include <queue>
#include <limits>
#include <string>
using namespace std;

struct Graph {
    int N;
    double** adjMatrix;
    bool weighted;

    Graph(int n, bool w);
    ~Graph();
};

struct BFSPath {
    int* vertices;
    int length;
};

Graph* createGraph(int N, bool weighted);
void deleteGraph(Graph* g);
void printGraph(Graph* g);
Graph* generateRandomGraph(int N, bool weighted, int minEdgesPerVertex, int maxEdgesPerVertex,
    double minWeight = 1.0, double maxWeight = 10.0);
Graph* readGraphFromFile(const string& filename, bool weighted, int formatType);
bool hasCycle(Graph* g);
bool hasCycleUndirectedUtil(Graph* g, int v, bool* visited, int parent);
void findAllPathsDFS(Graph* g, int start, int end);
void findAllPathsDFSRec(Graph* g, int u, int end, bool* visited, int* path, int pathIndex, int& count);
void findAllPathsBFS(Graph* g, int start, int end);
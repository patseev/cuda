#include <iostream>
#include<stdlib.h>
#include<time.h>

using namespace std;

#define VERTICES 1000
#define EDGES 5000

#define CHECK(value) {                                          \
    cudaError_t _m_cudaStat = value;                                        \
    if (_m_cudaStat != cudaSuccess) {                                       \
        cout<< "Error:" << cudaGetErrorString(_m_cudaStat) \
            << " at line " << __LINE__ << " in file " << __FILE__ << "\n"; \
        exit(1);                                                            \
    } }

//__global__ void generateEdgeList(int* in, int* out, int edges, int vertices)
//{
//    int col = threadIdx.x + blockDim.x * blockIdx.x;
//
//    if (col >= edges) return;
//
//    int innerNumber = 0;
//
//    for (int j = 0; j < vertices; j++) {
//        int idx = col * edges + j;
//        if (in[idx] == 1) {
//            out[2 * col + innerNumber] = j;
//            innerNumber += 1;
//            if (innerNumber == 2) break;
//        }
//    }
//}

/**
 * @param in - incidence matrix stored by columns, of size { edges * vertices }
 * @param out - resulting array of edges presented as pairs (source vertex -> dest vertex), of size { edges * 2 }
 * @param edges - number of edges in incidence matrix (number of columns)
 * @param vertices - number of vertices in graph (number of rows)
 */
void generateEdgeList(const int *in, int* out, int edges, int vertices) {
    for (int i = 0; i < edges; i++) {
        int innerNumber = 0;
        for (int j = 0; j < vertices; j++) {
            int idx = i * edges + j;
            if (in[idx] == 1) {
                out[2 * i + innerNumber] = j;
                innerNumber += 1;
                if (innerNumber == 2) break;
            }
        }
    }
}


/**
 *
 * @param result - result of CUDA algorithm
 * @param canonical - result of CPU algorithm
 * @param size - number of edges
 * @return true if match, false otherwise
 */
bool validateAgainstEachOther(const int *result, const int *canonical, int size) {
    for (int i = 0; i < size * 2; i += 2) {
        int result_source = result[2 * i];
        int result_dest = result[2 * i + 1];
        int canonical_source = canonical[2 * i];
        int canonical_dest = canonical[2 * i + 1];

        bool matchesDirectly = result_source == canonical_source && result_dest == canonical_dest;
        bool matchesInversely = result_source == canonical_dest && result_dest == canonical_source;

        if (!(matchesDirectly || matchesInversely)) {
            cout << endl << "There's a mismatch for edge number " << i << ": " << endl;
            cout << "Result: [" << result_source << ", " << result_dest << "]" << endl;
            cout << "Canonical: [" << canonical_source << ", " << canonical_dest << "]" << endl;
            return false;
        }
    }

    return true;
}

void printMatrix(int *matrix, int edges, int vertices) {
    cout << "===========================" << endl << "Matrix:" << endl;
    for (int i = 0; i < edges; i++) {
        for (int j = 0; j < vertices; j++) {
            cout << matrix[i * vertices + j] << ", ";
        }
        cout << endl;
    }
    cout << endl << "===========================" << endl;
}

void generateMatrix(int* out, const int edges, int vertices) {
    for (int i = 0; i < edges; i++) {
        // cover edge, should set values in the range of [0; vertices)
        int first = rand() % vertices;
        int second = first;
        while (second == first) second = rand() % vertices;

        int firstIndex = i * vertices + first;
        int secondIndex = i * vertices + second;

        cout << "Edge " << i << " = " << "[" << first << ", " << second << "] " << firstIndex << ", " << secondIndex << endl;

        out[firstIndex] = 1;
        out[secondIndex] = 1;
    }
}

bool validateGeneration(const int *in, int edges, int vertices) {
    for (int i = 0; i < edges; i++) {
        int count = 0;
        for (int j = 0; j < vertices; j++) {
            if (in[i * vertices + j] == 1) {
                count += 1;
            }
        }

        if (count != 2) {
            cout << "Error for edge #" << i << endl;
            return false;
        }
    }

    return true;
}

/**
 * @param edges - list of edges
 * @param size - number of edges
 */
void printEdgeList(int *edges, int size) {
    cout << "===========================" << endl << "Edges:" << endl;
    for (int i = 0; i < size * 2; i += 2) {
        cout << "[ " << edges[i] << ", " << edges[i + 1] << " ] " << endl;
    }
    cout << "===========================" << endl;
}

int main() {
    srand(time(0));

    float elapsedTimeCUDA, elapsedTimeCPU;
    clock_t startCPU;

    int edges = EDGES;
    int vertices = VERTICES;

    int* matrix = new int[edges * vertices];
    int* matrixDevice;

    int* listOfEdges = new int[edges * 2];
    int* listOfEdgesCPU = new int[edges * 2];
    int* listOfEdgesDevice;

    generateMatrix(matrix, edges, vertices);

    printMatrix(matrix, edges, vertices);

    if (!validateGeneration(matrix, edges, vertices)) {
        cout << "Matrix is invalid" << endl;
        return 1;
    } else {
        cout << "Matrix is valid" << endl;
    }

    startCPU = clock();
    generateEdgeList(matrix, listOfEdgesCPU, edges, vertices);
    elapsedTimeCPU = (double)(clock() - startCPU) / CLOCKS_PER_SEC;
    cout << "CPU time = " << elapsedTimeCPU * 1000 << " ms\n";
    cout << "CPU memory throughput = " << vertices * edges * 4 / elapsedTimeCPU / 1024 / 1024 / 1024 << " Gb/s\n";

//    cudaEvent_t startCUDA;
//    cudaEvent_t stopCUDA;
//    cudaEventCreate(&startCUDA);
//    cudaEventCreate(&stopCUDA);
//
//    CHECK(cudaMalloc(&matrixDevice, edges * vertices * 4));
//    CHECK(cudaMemcpy(&matrixDevice, matrix, edges * vertices * 4, cudaMemcpyHostToDevice));
//    CHECK(cudaMalloc(&listOfEdgesDevice, edges * 4 * 2));
//
//    cudaEventRecord(startCUDA, 0);
//
//    cudaEventRecord(stopCUDA, 0);
//    cudaEventSynchronize(stopCUDA);
//    CHECK(cudaGetLastError());
//    cudaEventElapsedTime(&elapsedTimeCUDA, startCUDA, stopCUDA);
//
//    CHECK(cudaMemcpy(listOfEdges, listOfEdgesDevice, edges * 2 * 4, cudaMemcpyDeviceToHost));
//
//    cout << "CUDA time = " << elapsedTimeCUDA << " ms\n";
//    cout << "CUDA memory throughput = " << edges * vertices * 4 / elapsedTimeCUDA / 1024 / 1024 / 1024 << " Gb/s\n";
//
//    if (!validateAgainstEachOther(listOfEdges, listOfEdges, edges)) {
//        cout << "Invalid result" << endl;
//    } else {
//        cout << "Valid result" << endl;
//    }

    delete[] matrix;
//    delete[] matrixDevice;
    delete[] listOfEdges;
    delete[] listOfEdgesCPU;
//    delete[] listOfEdgesDevice;

    return 0;
}

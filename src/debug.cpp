#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>


#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/index_io.h>


#include <iostream>

int main(int argc, char *argv[]) {
    std::cout << "Hello World!\n";
    faiss::Index * index = faiss::read_index(argv[1]);
    return 0;
}
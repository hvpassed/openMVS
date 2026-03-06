#include <faiss/gpu/StandardGpuResources.h>
#include <faiss/gpu/GpuIndexFlat.h>
#include <iostream>
#include <vector>

int main() {
    try {
        faiss::gpu::StandardGpuResources res;

        const int d = 4;
        faiss::gpu::GpuIndexFlatL2 index(&res, d);

        std::vector<float> xb = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0
        };
        index.add(3, xb.data());

        std::vector<float> xq = {1, 0, 0, 0};
        std::vector<faiss::idx_t> I(1);
        std::vector<float> D(1);

        index.search(1, xq.data(), 1, D.data(), I.data());

        std::cout << "ntotal=" << index.ntotal << "\n";
        std::cout << "top1 id=" << I[0] << " dist=" << D[0] << "\n";
        std::cout << "FAISS GPU OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAISS GPU FAILED: " << e.what() << "\n";
        return 1;
    }
}
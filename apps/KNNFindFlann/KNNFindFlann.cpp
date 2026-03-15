#define APPNAME _T("KNNFindFlann")
#include "../../libs/MVS.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>

// 引入 KD-Tree 库
#include "nanoflann.hpp"
#ifdef _USE_OPENMP
#include <omp.h>
#endif

// 1. 定义 nanoflann 需要的数据适配器
struct PointCloudAdaptor {
    const float* points;
    size_t num_points;
    PointCloudAdaptor(const float* pts, size_t n) : points(pts), num_points(n) {}
    inline size_t kdtree_get_point_count() const { return num_points; }
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const { return points[idx * 3 + dim]; }
    template <class BBOX> bool kdtree_get_bbox(BBOX& /*bb*/) const { return false; }
};

int main() {
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Loading point cloud..." << std::endl;
    
    boost::iostreams::mapped_file_source mmap("pointcloud_dense.bin");
    boost::iostreams::stream<boost::iostreams::array_source> stream(mmap.data(), mmap.size());

    std::cout << "unserializing ..." << std::endl;
    MVS::PointCloud pointcloud;
    {
        boost::archive::binary_iarchive ia(stream);
        ia >> pointcloud;
    }
    
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "preparing pointdata ..." << std::endl;
    
    size_t num_points = pointcloud.points.size();
    size_t dimension = 3; 
    
    // 使用 std::vector 管理内存，避免 new 之后忘记 delete 导致内存泄漏
    std::vector<float> point_data(num_points * dimension);
    for (size_t i = 0; i < num_points; ++i) {
        point_data[i * dimension + 0] = pointcloud.points[i].x;
        point_data[i * dimension + 1] = pointcloud.points[i].y;
        point_data[i * dimension + 2] = pointcloud.points[i].z;
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    
    const int k = 10; 
    const size_t compute_num = num_points;

    // 2. 构建 KD-Tree (速度极快)
    std::cout << "Building KD-Tree on " << num_points << " points..." << std::endl;
    PointCloudAdaptor adaptor(point_data.data(), num_points);
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, PointCloudAdaptor>,
        PointCloudAdaptor, 3, size_t>;
        
    KDTree index(3, adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    index.buildIndex();

    // 准备输出数组 (这里为了和你之前的读取逻辑兼容，使用 int64_t 代替 faiss::idx_t)
    std::vector<int64_t> indices(num_points * k);
    std::vector<float> distances(num_points * k);

    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << "searching using pure CPU (OpenMP Multi-Core)..." << std::endl;

    // 3. 开启 OpenMP 多核并发搜索，榨干你服务器的几十个核心！
    #ifdef _USE_OPENMP
    #pragma omp parallel for schedule(dynamic, 1000)
    #endif
    for (size_t i = 0; i < compute_num; ++i) {
        size_t ret_index[k];
        float out_dist_sqr[k];
        
        nanoflann::KNNResultSet<float> resultSet(k);
        resultSet.init(ret_index, out_dist_sqr);
        
        // 搜索第 i 个点的 k 个最近邻
        index.findNeighbors(resultSet, &point_data[i * 3], nanoflann::SearchParameters(10));        
        // 保存结果到全局数组
        for (int j = 0; j < k; ++j) {
            indices[i * k + j] = static_cast<int64_t>(ret_index[j]);
            distances[i * k + j] = out_dist_sqr[j]; // nanoflann 默认返回的也是平方距离，和 FAISS 一样
        }
    }

    auto t5 = std::chrono::high_resolution_clock::now();
    
    std::cout << "First point's " << k << " nearest neighbors:" << std::endl;
    for (int i = 0; i < k; ++i) {
        std::cout << "  Point " << indices[i] << ": Distance = " << distances[i] << std::endl;
    }

    // 4. 写出二进制文件
    std::ofstream outFile("knn_results.bin", std::ios::binary);
    if (outFile) {
        outFile.write(reinterpret_cast<const char*>(&compute_num), sizeof(size_t));
        outFile.write(reinterpret_cast<const char*>(&k), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(indices.data()), sizeof(int64_t) * compute_num * k);
        outFile.write(reinterpret_cast<const char*>(distances.data()), sizeof(float) * num_points * k);
        outFile.close();
        std::cout << "KNN results saved to knn_results.bin" << std::endl;
    }

    // 耗时统计
    auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto convert_time = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    auto knn_time = std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count();

    std::cout << "\nTiming results:" << std::endl;
    std::cout << "  Load & unserialize time: " << load_time << " ms" << std::endl;
    std::cout << "  Convert time: " << convert_time << " ms" << std::endl;
    std::cout << "  KD-Tree build time: " << build_time << " ms" << std::endl;
    std::cout << "  KNN search time: " << knn_time << " ms" << std::endl;
    std::cout << "  Total points: " << pointcloud.points.size() << std::endl;
    
    return 0;
}
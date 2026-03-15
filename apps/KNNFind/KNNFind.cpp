#define APPNAME _T("KNNFind")
#include "../../libs/MVS.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <chrono>
#include <iostream>
 
#include <atomic>

#include<faiss/IndexFlat.h>
#include<faiss/gpu/GpuIndexFlat.h>
#include<faiss/gpu/StandardGpuResources.h>

//�������10���ھ�
int main() {
    auto t1 = std::chrono::high_resolution_clock::now();
	std::cout << "Loading point cloud..." << std::endl;
    // 1. �ڴ�ӳ�䣨�޿�����
    boost::iostreams::mapped_file_source mmap("pointcloud_dense.bin");
    boost::iostreams::stream<boost::iostreams::array_source>
        stream(mmap.data(), mmap.size());

	std::cout << "unserializing ..." << std::endl;
    // 2. ���ٷ����л�
    MVS::PointCloud pointcloud;
    {
        boost::archive::binary_iarchive ia(stream);
        ia >> pointcloud;
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "preparing pointdata ..." << std::endl;
    size_t num_points = pointcloud.points.size();
    size_t dimension = 3; // Assuming 3D points (x,y,z)
    float* point_data = new float[num_points * dimension];
    for (size_t i = 0; i < num_points; ++i) {
        point_data[i * dimension + 0] = pointcloud.points[i].x;
        point_data[i * dimension + 1] = pointcloud.points[i].y;
        point_data[i * dimension + 2] = pointcloud.points[i].z;
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    
    const int k = 10; // ����ÿ�����10�������
    const size_t compute_num = num_points;
    faiss::gpu::StandardGpuResources gpu_resources;

    faiss::gpu::GpuIndexFlatConfig config;
    config.device = 0; // Use first GPU
    faiss::gpu::GpuIndexFlatL2 index(&gpu_resources, dimension, config);

    // Add points to the index
    index.add(num_points, point_data);

    // Prepare output arrays
    
    faiss::idx_t* indices = new faiss::idx_t[num_points * k];
    float* distances = new float[num_points * k];



    auto t4 = std::chrono::high_resolution_clock::now();
    const size_t batch_size = 500000; 

    for (size_t i = 0; i < compute_num; i += batch_size) {
        // 计算当前批次要处理的真实点数（防止最后一次越界）
        size_t current_batch = std::min(batch_size, compute_num - i);
        
        // 分批送入 GPU，利用指针偏移量写入对应的内存位置
        index.search(
            current_batch,                  // 当前批次查询数量
            point_data + i * dimension,     // 当前批次查询点的内存起始地址
            k, 
            distances + i * k,              // 当前批次距离结果的写入地址
            indices + i * k                 // 当前批次索引结果的写入地址
        );
    }
    auto t5 = std::chrono::high_resolution_clock::now();
    std::cout << "First point's " << k << " nearest neighbors:" << std::endl;
    for (int i = 0; i < k; ++i) {
        long idx = indices[i];  // Changed to long
        float dist = distances[i];
        std::cout << "  Point " << idx << ": Distance = " << dist << std::endl;
    }

    std::ofstream outFile("knn_results.bin", std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open knn_results.bin for writing!" << std::endl;
        return -1;
    }

    // д����������k ֵ
    outFile.write(reinterpret_cast<const char*>(&compute_num), sizeof(size_t));
    outFile.write(reinterpret_cast<const char*>(&k), sizeof(int));

    // д�� indices �� distances
    outFile.write(reinterpret_cast<const char*>(indices), sizeof(faiss::idx_t) * compute_num * k);
    outFile.write(reinterpret_cast<const char*>(distances), sizeof(float) * num_points * k);

    outFile.close();
    std::cout << "KNN results saved to knn_results.bin" << std::endl;


    // �����ʱ���
    auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto convert_time = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    auto knn_time = std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count();

    std::cout << "\nTiming results:" << std::endl;
    std::cout << "  Load time: " << load_time << " ms" << std::endl;
    std::cout << "  Convert time: " << convert_time << " ms" << std::endl;
    std::cout << "  KNN search time: " << knn_time << " ms" << std::endl;
    std::cout << "  Total points: " << pointcloud.points.size() << std::endl;
    return 0;
}
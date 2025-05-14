#define APPNAME _T("TestFaiss")
#include<faiss/IndexFlat.h>
#include "../../libs/MVS.h"
#include <fstream>
#include <iostream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <chrono>
 


int main() {
    std::ifstream inFile("knn_results.bin", std::ios::binary);
    if (!inFile) {
        std::cerr << "Failed to open knn_results.bin for reading!" << std::endl;
        return -1;
    }


    std::cout << "Loading indices..." << std::endl;
    size_t num_points;
    int k;
    inFile.read(reinterpret_cast<char*>(&num_points), sizeof(size_t));
    inFile.read(reinterpret_cast<char*>(&k), sizeof(int));

    // 分配内存
    faiss::idx_t* indices = new faiss::idx_t[num_points * k];
    float* distances = new float[num_points * k];

    // 读取数据
    inFile.read(reinterpret_cast<char*>(indices), sizeof(faiss::idx_t) * num_points * k);
    inFile.read(reinterpret_cast<char*>(distances), sizeof(float) * num_points * k);
    inFile.close();
    delete[] distances;

    std::cout << "Loading point cloud..." << std::endl;
    // 1. 内存映射（无拷贝）
    boost::iostreams::mapped_file_source mmap("pointcloud_dense.bin");
    boost::iostreams::stream<boost::iostreams::array_source>
        stream(mmap.data(), mmap.size());

    std::cout << "unserializing ..." << std::endl;
    // 2. 快速反序列化
    MVS::PointCloud pointcloud;
    {
        boost::archive::binary_iarchive ia(stream);
        ia >> pointcloud;
    }
    // 释放内存

	std::cout << "Working on point cloud with " << num_points << " points and k: " <<k<< std::endl;
	pointcloud.RefineSegments(indices, num_points, k);

    delete[] indices;
	pointcloud.SaveWithSegments("pointcloud_dense_refined_bin.ply",true);


    return 0;
}

#define APPNAME _T("TestPointCloud")
#include "../../libs/MVS.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <chrono>
#include <iostream>
#include<opencv2/opencv.hpp>
#include <atomic>



int main() {
    auto t1 = std::chrono::high_resolution_clock::now();
	std::cout << "Loading point cloud..." << std::endl;
    // 1. 内存映射（无拷贝）
    boost::iostreams::mapped_file_source mmap("pointcloud_dense.bin");
    boost::iostreams::stream<boost::iostreams::array_source>
        stream(mmap.data(), mmap.size());

    // 2. 快速反序列化
    MVS::PointCloud pointcloud;
    {
        boost::archive::binary_iarchive ia(stream);
        ia >> pointcloud;
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto t3 = std::chrono::high_resolution_clock::now();
    
    const int k = 10; // 查找每个点的10个最近邻
    cv::Mat indices, distances;

    auto t4 = std::chrono::high_resolution_clock::now();

    auto t5 = std::chrono::high_resolution_clock::now();


    std::cout << "First point's " << k << " nearest neighbors:" << std::endl;
    for (int i = 0; i < k; ++i) {
        int idx = indices.at<int>(0, i);
        float dist = distances.at<float>(0, i);
        std::cout << "  Point " << idx << ": Distance = " << dist << std::endl;
    }

    // 输出计时结果
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
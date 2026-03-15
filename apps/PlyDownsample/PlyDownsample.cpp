#define APPNAME _T("PlyDownsample")
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>
#include <cmath>
#include <cstdint>

// 用于哈希体素坐标的结构
struct VoxelHash {
    size_t operator()(const std::tuple<int64_t, int64_t, int64_t>& k) const {
        return std::get<0>(k) * 73856093 ^ std::get<1>(k) * 19349663 ^ std::get<2>(k) * 83492791;
    }
};

int main(int argc, char** argv) {
    // 默认参数
    std::string input_file = "../../large_scene/pointcloud_dense_refined_color_bin.ply";
    std::string output_file = "../../large_scene/pointcloud_downsampled.ply";
    float voxel_size = 0.1f;

    // 支持命令行传参
    if (argc >= 4) {
        input_file = argv[1];
        output_file = argv[2];
        voxel_size = std::stof(argv[3]);
    }

    std::cout << "Opening PLY file: " << input_file << std::endl;
    std::ifstream in(input_file, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open input file!" << std::endl;
        return 1;
    }

    // 1. 读取并修改表头
    std::string line;
    std::vector<std::string> header_lines;
    int vertex_count = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); // 兼容 Windows 换行符
        
        if (line.find("element vertex") != std::string::npos) {
            sscanf(line.c_str(), "element vertex %d", &vertex_count);
            header_lines.push_back("element vertex PLACEHOLDER");
        } else if (line.find("element face") != std::string::npos) {
            header_lines.push_back("element face 0");
        } else {
            header_lines.push_back(line);
        }
        if (line == "end_header") break;
    }

    std::cout << "Original point count: " << vertex_count << std::endl;
    std::cout << "Voxel size: " << voxel_size << std::endl;

    // 使用哈希表记录唯一的体素，只保留进入该体素的第一个点的数据流
    std::unordered_map<std::tuple<int64_t, int64_t, int64_t>, std::vector<char>, VoxelHash> voxel_grid;

    std::cout << "Downsampling (Parsing dynamic lists safely)..." << std::endl;
    int valid_count = 0;

    // 2. 逐点解析二进制块
    for (int i = 0; i < vertex_count; ++i) {
        std::vector<char> pt_bytes; // 保存这个点所有的原始字节数据
        
        // 读取 x, y, z (3 * 4 = 12 bytes)
        float xyz[3];
        in.read((char*)xyz, 12);
        pt_bytes.insert(pt_bytes.end(), (char*)xyz, (char*)xyz + 12);

        // 读取 red, green, blue, nx, ny, nz (3*1 + 3*4 = 15 bytes)
        char rgb_n[15]; 
        in.read(rgb_n, 15);
        pt_bytes.insert(pt_bytes.end(), rgb_n, rgb_n + 15);

        // 解析变长列表 1: view_indices
        uint8_t count1;
        in.read((char*)&count1, 1);
        pt_bytes.push_back(count1);
        if (count1 > 0) {
            std::vector<char> list1(count1 * 4); // uint32 占 4 字节
            in.read(list1.data(), count1 * 4);
            pt_bytes.insert(pt_bytes.end(), list1.begin(), list1.end());
        }

        // 解析变长列表 2: view_weights
        uint8_t count2;
        in.read((char*)&count2, 1);
        pt_bytes.push_back(count2);
        if (count2 > 0) {
            std::vector<char> list2(count2 * 4); // float32 占 4 字节
            in.read(list2.data(), count2 * 4);
            pt_bytes.insert(pt_bytes.end(), list2.begin(), list2.end());
        }

        // 读取 segment (1 * 4 = 4 bytes)
        char segment[4];
        in.read(segment, 4);
        pt_bytes.insert(pt_bytes.end(), segment, segment + 4);

        // 3. 过滤坏点 & 体素降采样
        if (std::isfinite(xyz[0]) && std::isfinite(xyz[1]) && std::isfinite(xyz[2]) &&
             std::abs(xyz[0]) < 1e12 && std::abs(xyz[1]) < 1e12 && std::abs(xyz[2]) < 1e12)  {
            
            int64_t vx = std::floor(xyz[0] / voxel_size);
            int64_t vy = std::floor(xyz[1] / voxel_size);
            int64_t vz = std::floor(xyz[2] / voxel_size);
            
            std::tuple<int64_t, int64_t, int64_t> voxel = {vx, vy, vz};
            
            // 如果这个体素是空的，就把这个点塞进去
            if (voxel_grid.find(voxel) == voxel_grid.end()) {
                voxel_grid[voxel] = pt_bytes;
                valid_count++;
            }
        }
    }

    std::cout << "Downsampled point count: " << valid_count << std::endl;

    // 4. 写回新的 PLY 文件
    std::cout << "Writing to disk: " << output_file << std::endl;
    std::ofstream out(output_file, std::ios::binary);
    
    // 写表头
    for (const auto& l : header_lines) {
        if (l == "element vertex PLACEHOLDER") {
            out << "element vertex " << valid_count << "\n";
        } else {
            out << l << "\n";
        }
    }

    // 倾倒数据
    for (const auto& pair : voxel_grid) {
        out.write(pair.second.data(), pair.second.size());
    }

    std::cout << "Done!" << std::endl;
    return 0;
}
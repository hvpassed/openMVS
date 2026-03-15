#define APPNAME _T("VerifyPly")
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <cfloat>

int main(int argc, char** argv) {
    std::string input_file = "../../large_scene/pointcloud_dense_refined_color_bin.ply";
    if (argc >= 2) input_file = argv[1];

    std::cout << "Opening PLY file: " << input_file << "\n";
    std::ifstream in(input_file, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open input file!\n";
        return 1;
    }

    std::string line;
    int vertex_count = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find("element vertex") != std::string::npos) {
            sscanf(line.c_str(), "element vertex %d", &vertex_count);
        }
        if (line == "end_header") break;
    }

    std::cout << "Total points declared in header: " << vertex_count << "\n";

    double min_x = DBL_MAX, min_y = DBL_MAX, min_z = DBL_MAX;
    double max_x = -DBL_MAX, max_y = -DBL_MAX, max_z = -DBL_MAX;
    
    int nan_inf_count = 0;
    int astronomical_count = 0; // 大于 1,000,000 的点
    int valid_count = 0;

    std::cout << "Scanning coordinates...\n";

    for (int i = 0; i < vertex_count; ++i) {
        float xyz[3];
        in.read((char*)xyz, 12);

        char rgb_n[15]; 
        in.read(rgb_n, 15);

        uint8_t count1;
        in.read((char*)&count1, 1);
        if (count1 > 0) in.seekg(count1 * 4, std::ios::cur);

        uint8_t count2;
        in.read((char*)&count2, 1);
        if (count2 > 0) in.seekg(count2 * 4, std::ios::cur);

        in.seekg(4, std::ios::cur); // 跳过 segment

        if (!std::isfinite(xyz[0]) || !std::isfinite(xyz[1]) || !std::isfinite(xyz[2])) {
            nan_inf_count++;
            continue;
        }

        // 设定一个合理的物理阈值来寻找真正的边界（比如 100万 米以内）
        if (std::abs(xyz[0]) > 1e6 || std::abs(xyz[1]) > 1e6 || std::abs(xyz[2]) > 1e6) {
            astronomical_count++;
            continue;
        }

        valid_count++;
        if (xyz[0] < min_x) min_x = xyz[0];
        if (xyz[0] > max_x) max_x = xyz[0];
        if (xyz[1] < min_y) min_y = xyz[1];
        if (xyz[1] > max_y) max_y = xyz[1];
        if (xyz[2] < min_z) min_z = xyz[2];
        if (xyz[2] > max_z) max_z = xyz[2];
    }

    std::cout << "------------------------------------------------\n";
    std::cout << "Scan Results:\n";
    std::cout << "Total points: " << vertex_count << "\n";
    std::cout << "NaN/Inf points: " << nan_inf_count << "\n";
    std::cout << "Astronomical points (> 1e6): " << astronomical_count << "\n";
    std::cout << "Valid points for bounding box: " << valid_count << "\n\n";
    
    if (valid_count > 0) {
        std::cout << "Real Bounding Box (excluding outliers):\n";
        std::cout << "X range: [" << min_x << ", " << max_x << "]\n";
        std::cout << "Y range: [" << min_y << ", " << max_y << "]\n";
        std::cout << "Z range: [" << min_z << ", " << max_z << "]\n";
    }

    return 0;
}
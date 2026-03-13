#include"SegPro.h"

#pragma push_macro("VERBOSE")
#undef VERBOSE
#define VERBOSE(...) LOG(lt, __VA_ARGS__)


// S T R U C T S ///////////////////////////////////////////////////

DEFINE_LOG_NAME(lt, _T("Segment "));

SegPro::SegPro(std::vector<float> readData)
{
	data = readData;
}

SegPro::SegPro(uint32_t id, uint32_t segid, uint32_t width, uint32_t height, SEACAVE::String name, std::vector<float> readData) {
	refImage.ID = id;
	seg_ID = segid;
	this->width = width;
	this->height = height;
	refImage.image_name = name;
	data = readData;
};

SegPro::~SegPro()
{

}

float SegPro::getPixel(int channel, int x, int y)
{
	return data[channel * width * height + y * width + x];

}

SegProVec SegPro::getSegProData(int x, int y) {
	SegProVec segProData;
    
    // 提前算出基础偏移量，避免在循环里做重复的加法和乘法
    const int base_idx = y * width + x;
    const int stride = width * height;
    
    // 绝对不要在这里加 #pragma omp parallel for！
    // 现代 C++ 编译器（-O3）会自动把这个小循环优化成 CPU 的 SIMD 矢量指令，瞬间完成
    for (int i = 0; i < SEG_CLASS; i++) {
        segProData[i] = data[i * stride + base_idx];
    }

    return segProData;
	// SegProVec segProData;
	// #if SEG_CLASS >= 16
	// #ifdef _USE_OPENMP
	// #pragma omp parallel for
	// for (int i = 0; i < SEG_CLASS; i++) {
	// 	segProData[i] = getPixel(i, x, y);
	// }


	// #else
	// for (int i = 0; i < SEG_CLASS; i++) {
	// 	segProData[i] = getPixel(i, x, y);
	// }

	// #endif // _USE_OPENMP
	// #else
	// for (int i = 0; i < SEG_CLASS; i++) {
	// 	segProData[i] = getPixel(i, x, y);
	// }
	// #endif // SEG_CLASS >= 16

	// return segProData;
};

SegProVec SegPro::getSegProData(SEACAVE::ImageRef & x) {
	return getSegProData(x.x, x.y);
};

SegProArr readFromHDF5(const SEACAVE::String& h5Path, const SEACAVE::String& imgRefPath)
{
    SegProArr ret;
    const SEACAVE::String h5FilePath = h5Path.empty() ? _T("dense/pro/mat.h5") : h5Path;
    std::ifstream imgRefFile(imgRefPath.c_str());
    VERBOSE("Start multi-threaded read HDF5 file from %s with imgRef file %s", h5FilePath.c_str(), imgRefPath.c_str());

    if (!imgRefFile) {
        VERBOSE("error: cannot open imgRef file");
        return ret;
    }

    // 1. 结构体用于临时保存要读取的元数据
    struct MetaData {
        uint32_t ID, Width, Height;
        std::string id_str;
        SEACAVE::String Name;
    };
    std::vector<MetaData> metaList;

    // 单线程快速解析 txt 文件
    std::string line;
    std::getline(imgRefFile, line); // 跳过表头
    while (std::getline(imgRefFile, line)) {
        std::istringstream iss(line);
        MetaData meta;
        std::string id, width, height, name;
        std::getline(iss, id, ',');
        std::getline(iss, width, ',');
        std::getline(iss, height, ',');
        std::getline(iss, name, '\n');
        
        meta.id_str = id;
        meta.ID = std::stoi(id);
        meta.Width = std::stoi(width);
        meta.Height = std::stoi(height);
        meta.Name = name;
        metaList.push_back(meta);
    }

    uint32_t totalCount = metaList.size();
    if (totalCount == 0) return ret;

    // 提前分配内存，避免多线程环境下的 push_back 冲突
    // 注意：如果 SegProArr 是 OpenMVS 的 cList 类型，这里可能是 ret.Resize(totalCount);
    // 如果是 std::vector，就是 ret.resize(totalCount);
    ret.resize(totalCount); 

    std::atomic<int> count{0}; // 原子计数器，多线程打印进度用

    // 2. 开启多线程并行读取和解压
    #ifdef _USE_OPENMP
    #pragma omp parallel for schedule(dynamic, 1)
    #endif
    for (int i = 0; i < (int)totalCount; ++i) {
        const MetaData& meta = metaList[i];
        
        try {
            // 【极其关键】：每个线程必须自己独立打开一次 HDF5 文件句柄！
            // 这样底层就不会产生锁竞争，从而完美实现多线程 LZF 解压
            H5::H5File file(h5FilePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
            H5::DataSet dataset = file.openDataSet(meta.id_str);
            H5::DataSpace dataspace = dataset.getSpace();
            hsize_t dims[3];
            dataspace.getSimpleExtentDims(dims, nullptr);

            std::vector<float> data(dims[0] * dims[1] * dims[2]);
            // 这里包含了最耗时的 I/O、LZF 解压 和 FP16 到 NATIVE_FLOAT 的转换
            dataset.read(data.data(), H5::PredType::NATIVE_FLOAT);

            // 写入预分配的数组，索引 i 各不相同，绝对安全
            ret[i] = SegPro(meta.ID, SEG_CLASS, meta.Width, meta.Height, meta.Name, data);
            
            // 释放当前线程的资源
            dataset.close();
            dataspace.close();
            file.close();
        } 
        catch (const H5::Exception& e) {
            #ifdef _USE_OPENMP
            #pragma omp critical
            #endif
            VERBOSE("HDF5 read error on ID %u", meta.ID);
        }

        // 打印进度
        int current_count = ++count;
        if (current_count % 50 == 0) {
            VERBOSE("Loaded %d / %u HDF5 semantic maps...", current_count, totalCount);
        }
    }

    VERBOSE("End read (segPro size: %u)", totalCount);
    return ret;
}


SegProArr readFromBINs(const SEACAVE::String& binDir, const SEACAVE::String& imgRefPath)
{
    SegProArr ret;
    std::ifstream imgRefFile(imgRefPath.c_str());
    VERBOSE("Start fast multi-threaded read FP16 BIN files from %s with imgRef file %s", binDir.c_str(), imgRefPath.c_str());

    if (!imgRefFile) {
        VERBOSE("error: cannot open imgRef file: %s", imgRefPath.c_str());
        return ret;
    }

    // 1. 结构体用于临时保存要读取的元数据
    struct MetaData {
        uint32_t ID, Width, Height;
        std::string id_str;
        SEACAVE::String Name;
    };
    std::vector<MetaData> metaList;

    // 单线程快速解析 txt 文件
    std::string line;
    std::getline(imgRefFile, line); // 跳过表头
    while (std::getline(imgRefFile, line)) {
        std::istringstream iss(line);
        MetaData meta;
        std::string id, width, height, name;
        std::getline(iss, id, ',');
        std::getline(iss, width, ',');
        std::getline(iss, height, ',');
        std::getline(iss, name, '\n');
        
        meta.id_str = id;
        meta.ID = std::stoi(id);
        meta.Width = std::stoi(width);
        meta.Height = std::stoi(height);
        meta.Name = name;
        metaList.push_back(meta);
    }

    uint32_t totalCount = metaList.size();
    if (totalCount == 0) return ret;

    // 2. 提前分配总容量
    ret.resize(totalCount); 
    std::atomic<int> count{0};

    // 3. 开启 OpenMP 多线程暴力并发读取！
    #ifdef _USE_OPENMP
    #pragma omp parallel for schedule(dynamic, 1)
    #endif
    for (int i = 0; i < (int)totalCount; ++i) {
        const MetaData& meta = metaList[i];
        
        // 拼接具体那张图的 .bin 路径
        std::string bin_path = binDir + "/" + meta.id_str + ".bin";
        std::ifstream file(bin_path, std::ios::binary);

        if (file) {
            size_t num_elements = SEG_CLASS * meta.Width * meta.Height;
            
            // 【关键修改】：先用 uint16_t 接收 FP16 数据
            std::vector<uint16_t> fp16_data(num_elements);
            file.read(reinterpret_cast<char*>(fp16_data.data()), num_elements * sizeof(uint16_t));

            // 分配最终用于 OpenMVS 的 FP32 数据容器
            std::vector<float> fp32_data(num_elements);
            
            // 在当前线程内，把 16位 转为 32位
            for (size_t k = 0; k < num_elements; ++k) {
                fp32_data[k] = fp16_to_fp32(fp16_data[k]);
            }

            // 安全赋值到结果数组
            ret[i] = SegPro(meta.ID, SEG_CLASS, meta.Width, meta.Height, meta.Name, fp32_data);
        } else {
            #ifdef _USE_OPENMP
            #pragma omp critical
            #endif
            VERBOSE("error: cannot open BIN file: %s", bin_path.c_str());
        }

        // 打印进度
        int current_count = ++count;
        if (current_count % 50 == 0) {
            VERBOSE("Loaded %d / %u FP16 BIN semantic maps...", current_count, totalCount);
        }
    }

    VERBOSE("End read (segPro size: %u)", totalCount);
    return ret;
}

#pragma pop_macro("VERBOSE")

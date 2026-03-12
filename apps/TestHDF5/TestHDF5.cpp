#define APPNAME _T("TestHDF5")
//#include<H5Cpp.h>
//#include<vector>
//#include<iostream>
//
//#include <fstream>
//#include"../../libs/MVS/SegPro.h"
//
//
//bool fileExists(const std::string& filename) {
//    std::ifstream file(filename);
//    return file.good();
//}
//
//void listDatasets(hid_t group_id, const std::string& path = "/") {
//    hsize_t num_objs;
//    H5Gget_num_objs(group_id, &num_objs); // ��ȡ���ڶ��������
//
//    for (hsize_t i = 0; i < num_objs; i++) {
//        char obj_name[1024];
//        H5Gget_objname_by_idx(group_id, i, obj_name, sizeof(obj_name)); // ��ȡ��������
//
//        int obj_type = H5Gget_objtype_by_idx(group_id, i); // ��ȡ��������
//
//        if (obj_type == H5G_GROUP) {
//            std::cout << "[Group] " << path + obj_name << "/" << std::endl;
//            hid_t subgroup_id = H5Gopen(group_id, obj_name, H5P_DEFAULT);
//            if (subgroup_id >= 0) {
//                listDatasets(subgroup_id, path + obj_name + "/"); // �ݹ��������
//                H5Gclose(subgroup_id);
//            }
//        }
//        else if (obj_type == H5G_DATASET) {
//            std::cout << "[Dataset] " << path + obj_name << std::endl;
//        }
//    }
//}
//
////int main() {
////    const std::string filename = "E:\\workspace\\dataset\\mat.h5";  // ��� HDF5 �ļ�·��
////    if (!fileExists(filename)) {
////        std::cerr << "Error: File does not exist: " << filename << std::endl;
////        return 1;
////    }
////    else {
////		std::cout << "File exists: " << filename << std::endl;
////    }
////
////    try {
////        H5::H5File file(filename, H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
////        std::cout << "Datasets in HDF5 file: " << filename << std::endl;
////        listDatasets(file.getId()); // �������ݼ�
////    }
////    catch (H5::Exception& err) {
////        std::cerr << "HDF5 error: " << err.getDetailMsg() << std::endl;
////        return 1;
////    }
////    return 0;
////}
//
//std::vector < std:: string > listDatasets(const H5::Group& group, const std::string& path = "/") {
//	std::vector < std::string > datasets;
//	datasets.clear();
//    for (hsize_t i = 0; i < group.getNumObjs(); ++i) {
//        std::string obj_name = group.getObjnameByIdx(i);
//        H5G_obj_t obj_type = group.getObjTypeByIdx(i);
//
//         if (obj_type == H5G_DATASET) {
//            std::cout << "[Dataset] " << path + obj_name << std::endl;
//            datasets.push_back(obj_name);
//        }
//
//
//    }
//	return datasets;
//}
//int main(void) {
//    const std::string filename = "dense/pro/mat.h5";
//	H5::H5File file(filename, H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
//    std::ifstream imgRef("imgRef.txt");
//    SegProArr segProArr = readFromHDF5();
//
//	return 0;
// //   const std::string filename = "E:\\workspace\\dataset\\mat.h5";
// //   H5::H5File file(filename, H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
//	//std::vector<std::string> datasets = listDatasets(file.openGroup("/"));
//	//SegProArr segProArr;
//	//segProArr.Resize(datasets.size());
// //   for (int i = 0; i < datasets.size(); i++) {
//	//	hsize_t dims[3];
//
// //       H5::DataSet dataset = file.openDataSet(datasets[i]);
// //       H5::DataSpace dataspace = dataset.getSpace();
//	//	dataspace.getSimpleExtentDims(dims, nullptr);
//	//	std::vector<float> data(dims[0]*dims[1]*dims[2]);
//	//	dataset.read(data.data(), H5::PredType::NATIVE_FLOAT);
//	//	segProArr[i] = SegPro(data);
// //       printf("%d %d %d", dims[0], dims[1], dims[2]);
//	//	printf("data[0] = %f\n", segProArr[i].getPixel(0, 0, 0));
// //   }
//
// //   H5::DataSet dataset = file.openDataSet("1");
// //   H5::DataSpace dataspace = dataset.getSpace();
//
// //   hsize_t dims[3];
// //   dataspace.getSimpleExtentDims(dims, nullptr);
//
// //
// //   std::vector<float> data(dims[0] * dims[1] * dims[2]);
//
// //   // ��ȡ����
// //   dataset.read(data.data(), H5::PredType::NATIVE_FLOAT);
// //   SegProArr test;
//	//test.Resize(1);
//	//test[0] = SegPro(data);
// //   // �������
//	//printf("%d %d %d", dims[0], dims[1], dims[2]);
//	//printf("data[0] = %f\n", test[0].getPixel(0,0,0));
//	//H5Dclose(dataset.getId());
//
//    return 0;
//}
//






#include <cstdio>
#include <vector>
#include <chrono>
#include <faiss/IndexFlat.h>
#include <faiss/gpu/GpuIndexIVFFlat.h>
#include <faiss/gpu/StandardGpuResources.h>

// �����������
std::vector<float> generate_random_data(int num_vectors, int dim) {
    std::vector<float> data(num_vectors * dim);
    for (auto& v : data) {
        v = static_cast<float>(rand()) / RAND_MAX; // [0,1)�����
    }
    return data;
}

void test_faiss_gpu() {
    const int dim = 128;          // ����ά��
    const int num_vectors = 3993600; // ���ݿ��С
    const int num_queries = 10;    // ��ѯ����
    const int k = 5;              // �������������
    const int nprobe = 10;        // �����ľ���������

    // 1. ���ɲ�������
    auto database = generate_random_data(num_vectors, dim);
    auto queries = generate_random_data(num_queries, dim);

    // 2. ����CPU���������ڻ�׼�Ƚϣ�
    faiss::IndexFlatL2 cpu_index(dim);

    // 3. ����GPU��Դ
    faiss::gpu::StandardGpuResources gpu_res;

    // 4. ����GPU��������
    faiss::gpu::GpuIndexIVFFlatConfig config;
    config.device = 0; // ʹ�õ�һ��GPU

    // 5. ����GPU����
    auto gpu_index = faiss::gpu::GpuIndexIVFFlat(
        &gpu_res,
        dim,
        1024,  // ������������
        faiss::METRIC_L2,
        config
    );

    // 6. ѵ������
    auto train_start = std::chrono::high_resolution_clock::now();
    gpu_index.train(num_vectors, database.data());
    auto train_end = std::chrono::high_resolution_clock::now();

    // 7. �������ݵ�����
    auto add_start = std::chrono::high_resolution_clock::now();
    gpu_index.add(num_vectors, database.data());
    auto add_end = std::chrono::high_resolution_clock::now();

    // 8. ������������
    gpu_index.nprobe = nprobe;

    // 9. ִ��������GPU��
    std::vector<faiss::idx_t> gpu_labels(num_queries * k);
    std::vector<float> gpu_distances(num_queries * k);

    auto search_start = std::chrono::high_resolution_clock::now();
    gpu_index.search(
        num_queries,
        queries.data(),
        k,
        gpu_distances.data(),
        gpu_labels.data()
    );
    auto search_end = std::chrono::high_resolution_clock::now();

    // 10. CPU��֤
    cpu_index.add(num_vectors, database.data());
    std::vector<faiss::idx_t> cpu_labels(num_queries * k);
    std::vector<float> cpu_distances(num_queries * k);
    cpu_index.search(
        num_queries,
        queries.data(),
        k,
        cpu_distances.data(),
        cpu_labels.data()
    );

    // ��ʱ���
    auto train_time = std::chrono::duration_cast<std::chrono::milliseconds>(train_end - train_start);
    auto add_time = std::chrono::duration_cast<std::chrono::milliseconds>(add_end - add_start);
    auto search_time = std::chrono::duration_cast<std::chrono::milliseconds>(search_end - search_start);

    // ��ӡ���
    printf("[GPU Timing]\n");
    printf("Training: %lld ms\n", train_time.count());
    printf("Adding data: %lld ms\n", add_time.count());
    printf("Searching: %lld ms\n\n", search_time.count());

    // ��֤ǰ5����ѯ���
    const int check_queries = 5;
    printf("[Result Validation (first %d queries)]\n", check_queries);
    for (int q = 0; q < check_queries; ++q) {
        printf("Query %d:\n", q);
        printf("GPU Results | CPU Results\n");
        for (int i = 0; i < k; ++i) {
            printf("%6ld (%.3f) | %6ld (%.3f)\n",
                gpu_labels[q * k + i], gpu_distances[q * k + i],
                cpu_labels[q * k + i], cpu_distances[q * k + i]);
        }
        printf("\n");
    }
}

int main() {
    try {
        test_faiss_gpu();
    }
    catch (const faiss::FaissException& e) {
        fprintf(stderr, "FAISS Exception: %s\n", e.what());
        return 1;
    }
    catch (const std::exception& e) {
        fprintf(stderr, "Standard Exception: %s\n", e.what());
        return 2;
    }
    return 0;
}
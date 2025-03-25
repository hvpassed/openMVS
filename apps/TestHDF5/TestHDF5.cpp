#define APPNAME _T("TestHDF5")
#include<H5Cpp.h>
#include<vector>
#include<iostream>

#include <fstream>
#include"../../libs/IO/SegPro.h"
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

void listDatasets(hid_t group_id, const std::string& path = "/") {
    hsize_t num_objs;
    H5Gget_num_objs(group_id, &num_objs); // 获取组内对象的数量

    for (hsize_t i = 0; i < num_objs; i++) {
        char obj_name[1024];
        H5Gget_objname_by_idx(group_id, i, obj_name, sizeof(obj_name)); // 获取对象名称

        int obj_type = H5Gget_objtype_by_idx(group_id, i); // 获取对象类型

        if (obj_type == H5G_GROUP) {
            std::cout << "[Group] " << path + obj_name << "/" << std::endl;
            hid_t subgroup_id = H5Gopen(group_id, obj_name, H5P_DEFAULT);
            if (subgroup_id >= 0) {
                listDatasets(subgroup_id, path + obj_name + "/"); // 递归遍历子组
                H5Gclose(subgroup_id);
            }
        }
        else if (obj_type == H5G_DATASET) {
            std::cout << "[Dataset] " << path + obj_name << std::endl;
        }
    }
}

//int main() {
//    const std::string filename = "E:\\workspace\\dataset\\mat.h5";  // 你的 HDF5 文件路径
//    if (!fileExists(filename)) {
//        std::cerr << "Error: File does not exist: " << filename << std::endl;
//        return 1;
//    }
//    else {
//		std::cout << "File exists: " << filename << std::endl;
//    }
//
//    try {
//        H5::H5File file(filename, H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
//        std::cout << "Datasets in HDF5 file: " << filename << std::endl;
//        listDatasets(file.getId()); // 遍历数据集
//    }
//    catch (H5::Exception& err) {
//        std::cerr << "HDF5 error: " << err.getDetailMsg() << std::endl;
//        return 1;
//    }
//    return 0;
//}

std::vector < std:: string > listDatasets(const H5::Group& group, const std::string& path = "/") {
	std::vector < std::string > datasets;
	datasets.clear();
    for (hsize_t i = 0; i < group.getNumObjs(); ++i) {
        std::string obj_name = group.getObjnameByIdx(i);
        H5G_obj_t obj_type = group.getObjTypeByIdx(i);

         if (obj_type == H5G_DATASET) {
            std::cout << "[Dataset] " << path + obj_name << std::endl;
            datasets.push_back(obj_name);
        }


    }
	return datasets;
}
int main(void) {
    const std::string filename = "dense/pro/mat.h5";
	H5::H5File file(filename, H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
    std::ifstream imgRef("imgRef.txt");
    SegProArr segProArr = readFromHDF5();

	return 0;
 //   const std::string filename = "E:\\workspace\\dataset\\mat.h5";
 //   H5::H5File file(filename, H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
	//std::vector<std::string> datasets = listDatasets(file.openGroup("/"));
	//SegProArr segProArr;
	//segProArr.Resize(datasets.size());
 //   for (int i = 0; i < datasets.size(); i++) {
	//	hsize_t dims[3];

 //       H5::DataSet dataset = file.openDataSet(datasets[i]);
 //       H5::DataSpace dataspace = dataset.getSpace();
	//	dataspace.getSimpleExtentDims(dims, nullptr);
	//	std::vector<float> data(dims[0]*dims[1]*dims[2]);
	//	dataset.read(data.data(), H5::PredType::NATIVE_FLOAT);
	//	segProArr[i] = SegPro(data);
 //       printf("%d %d %d", dims[0], dims[1], dims[2]);
	//	printf("data[0] = %f\n", segProArr[i].getPixel(0, 0, 0));
 //   }

 //   H5::DataSet dataset = file.openDataSet("1");
 //   H5::DataSpace dataspace = dataset.getSpace();

 //   hsize_t dims[3];
 //   dataspace.getSimpleExtentDims(dims, nullptr);

 //
 //   std::vector<float> data(dims[0] * dims[1] * dims[2]);

 //   // 读取数据
 //   dataset.read(data.data(), H5::PredType::NATIVE_FLOAT);
 //   SegProArr test;
	//test.Resize(1);
	//test[0] = SegPro(data);
 //   // 输出数据
	//printf("%d %d %d", dims[0], dims[1], dims[2]);
	//printf("data[0] = %f\n", test[0].getPixel(0,0,0));
	//H5Dclose(dataset.getId());

    return 0;
}


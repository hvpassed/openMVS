#include"SegPro.h"

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
	#if SEG_CLASS >= 16
	#ifdef _USE_OPENMP
	#pragma omp parallel for
	for (int i = 0; i < SEG_CLASS; i++) {
		segProData[i] = getPixel(i, x, y);
	}


	#else
	for (int i = 0; i < SEG_CLASS; i++) {
		segProData[i] = getPixel(i, x, y);
	}

	#endif // _USE_OPENMP
	#else
	for (int i = 0; i < SEG_CLASS; i++) {
		segProData[i] = getPixel(i, x, y);
	}
	#endif // SEG_CLASS >= 16

	return segProData;
};

SegProVec SegPro::getSegProData(SEACAVE::ImageRef & x) {
	return getSegProData(x.x, x.y);
};

SegProArr readFromHDF5(const SEACAVE::String& h5Path, const SEACAVE::String& imgRefPath)
{
	SegProArr ret;
	const SEACAVE::String h5FilePath = h5Path.empty() ? _T("dense/pro/mat.h5") : h5Path;
	std::ifstream imgRefFile(imgRefPath.c_str());
	H5::H5File file(h5FilePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT, H5P_DEFAULT);
	if (imgRefFile) {
		std::string line;
		std::getline(imgRefFile, line);
		while (std::getline(imgRefFile, line)) {
			std::istringstream iss(line);
			std::string id, width, height, name;
			std::getline(iss, id, ',');
			std::getline(iss, width, ',');
			std::getline(iss, height, ',');
			std::getline(iss, name, '\n');
			uint32_t ID = std::stoi(id);
			uint32_t Width = std::stoi(width);
			uint32_t Height = std::stoi(height);
			SEACAVE::String Name = name;
			std::vector<float> data;
			H5::DataSet dataset = file.openDataSet(id);
			H5::DataSpace dataspace = dataset.getSpace();
			hsize_t dims[3];
			dataspace.getSimpleExtentDims(dims, nullptr);
			data.resize(dims[0] * dims[1] * dims[2]);
			dataset.read(data.data(), H5::PredType::NATIVE_FLOAT);
			SegPro segPro(ID, SEG_CLASS, Width, Height, Name, data);
			ret.push_back(segPro);
		}

	} else {
		return ret;
	}

	return ret;
}
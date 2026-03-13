
#ifndef __SEGPRO_H__
#define __SEGPRO_H__
#include<H5Cpp.h>
#include<Eigen/Dense>
#include<vector>
#include"../Common/Common.h"
#include"Common.h"
#include <fstream>
#include<sstream>
#include "Image.h"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/array.hpp>
#include <fstream>
#include <atomic>
#include <cstring>
#include <cstdint>


#define SEG_CLASS 5	

typedef Eigen::Array<float, SEG_CLASS, 1> SegProVec;

namespace boost {
	namespace serialization {

		template<class Archive>
		void serialize(Archive& ar, Eigen::Array<float, SEG_CLASS, 1, 0, SEG_CLASS, 1>& m, const unsigned int)
		{
			ar& make_array(m.data(), m.size());
		}

	} // namespace serialization
} // namespace boost


inline float fp16_to_fp32(uint16_t h) {
    uint32_t sign = (h >> 15) & 1;
    uint32_t exp = (h >> 10) & 0x1F;
    uint32_t mant = h & 0x3FF;

    uint32_t f = 0;
    if (exp == 0) {
        if (mant == 0) {
            f = (sign << 31); // 零
        } else {
            // Subnormal (极小值)，在概率图里通常可以直接冲刷为 0
            f = (sign << 31); 
        }
    } else if (exp == 0x1F) {
        f = (sign << 31) | 0x7f800000 | (mant << 13); // Inf 或 NaN
    } else {
        f = (sign << 31) | ((exp + 127 - 15) << 23) | (mant << 13); // 正常浮点数
    }

    float res;
    std::memcpy(&res, &f, sizeof(float));
    return res;
}


class SegPro {





public:
	struct RefImage {
		uint32_t ID;
		SEACAVE::String image_name;
	};

	RefImage refImage;
	uint32_t seg_ID;
	uint32_t width, height;

	std::vector<float> data;



	SegPro() {};
	SegPro(std::vector<float> readData);

	SegPro(uint32_t id, uint32_t segid, uint32_t width, uint32_t height, SEACAVE::String name, std::vector<float> readData);
	~SegPro();
	float getPixel(int channel, int x, int y);

	SegProVec getSegProData(int x, int y);
	SegProVec getSegProData(SEACAVE::ImageRef & x);
};


typedef MVS_API SEACAVE::cList< SegPro, const SegPro&, 1, 16, MVS::IIndex> SegProArr;
SegProArr readFromHDF5(const SEACAVE::String& h5Path, const SEACAVE::String& imgRefPath = _T("imgRef.txt"));
SegProArr readFromBINs(const SEACAVE::String& binDir, const SEACAVE::String& imgRefPath = _T("imgRef.txt"));
#endif // __SEGPRO_H__
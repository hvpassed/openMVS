#define APPNAME _T("TestView")
#include "../../libs/MVS.h"
#include<iostream>
#include<unordered_map>
bool AdjustColor(MVS::PointCloud & pointCloud,const std::unordered_map<MVS::PointCloud::SEGMENT,MVS::PointCloud::Color> colorMap) {
	FOREACH(i, pointCloud.points) {

		pointCloud.colors[i] = colorMap.at(pointCloud.segments[i]);
		
	}
	return true;

}
//调整点云颜色
//"backgound", "building", "woodland", "water", "road"
int main(void) {
	MVS::PointCloud pointcloud;
	std::cout << "Loading pointcloud ..." << pointcloud.GetSize() << std::endl;
	pointcloud.Load("pointcloud_dense_refined_bin.ply");
	std::cout << "pointcloud size: " << pointcloud.GetSize() << std::endl;

	std::unordered_map<MVS::PointCloud::SEGMENT, MVS::PointCloud::Color> colorMap;
	colorMap[MVS::PointCloud::SEGMENT(0)] = MVS::PointCloud::Color(128, 128, 128);
	colorMap[MVS::PointCloud::SEGMENT(1)] = MVS::PointCloud::Color(255, 0, 0);
	colorMap[MVS::PointCloud::SEGMENT(2)] = MVS::PointCloud::Color(0, 170, 0);
	colorMap[MVS::PointCloud::SEGMENT(3)] = MVS::PointCloud::Color(0, 0, 255);
	colorMap[MVS::PointCloud::SEGMENT(4)] = MVS::PointCloud::Color(255, 255, 0);
	std::cout << "Adjusting color..." << std::endl;
	AdjustColor(pointcloud, colorMap);
	pointcloud.SaveWithSegments("pointcloud_dense_refined_color_bin.ply", true);

	return 0;
}
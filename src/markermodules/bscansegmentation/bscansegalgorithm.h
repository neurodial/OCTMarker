#ifndef BSCANSEGALGORITHM_H
#define BSCANSEGALGORITHM_H

#include "configdata.h"

namespace cv
{
	class Mat;
}

namespace OctData
{
	class BScan;
}

class BScanSegmentation;

class BScanSegAlgorithm
{
public:
	static void initFromSegline(const OctData::BScan& bscan, cv::Mat& segMat);
	static void initFromThreshold(const cv::Mat& image, cv::Mat& segMat, const BScanSegmentationMarker::ThresholdData& data);
	static void openClose(cv::Mat& dest, cv::Mat* src = nullptr); /// if no src given, then dest is used as src
};

#endif // BSCANSEGALGORITHM_H

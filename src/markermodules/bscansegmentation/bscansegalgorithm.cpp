#include "bscansegalgorithm.h"

#include <opencv/cv.h>
#include <cassert>
#include <limits>
#include <cmath>


#include <octdata/datastruct/bscan.h>


#include "bscansegmentation.h"

namespace
{
	struct FieldAccVertical
	{
		template<typename T>
		static constexpr T* startIt(cv::Mat& field, std::size_t inner, std::size_t outer) { return field.ptr<T>(static_cast<int>(inner)) + outer; }
		template<typename T>
		static constexpr const T* startIt_const(const cv::Mat& field, std::size_t inner, std::size_t outer) { return field.ptr<T>(static_cast<int>(inner)) + outer; }

		static std::size_t numInner(const cv::Mat* levelset) { return static_cast<std::size_t>(levelset->rows); }
		static std::size_t numOuter(const cv::Mat* levelset) { return static_cast<std::size_t>(levelset->cols); }
	};

	struct FieldAccHorizontal
	{
		template<typename T>
		static constexpr T* startIt(cv::Mat& field, std::size_t inner, std::size_t outer) { return field.ptr<T>(static_cast<int>(outer)) + inner; }
		template<typename T>
		static constexpr const T* startIt_const(const cv::Mat& field, std::size_t inner, std::size_t outer) { return field.ptr<T>(static_cast<int>(outer)) + inner; }

		static std::size_t numInner(const cv::Mat* levelset) { return static_cast<std::size_t>(levelset->cols); }
		static std::size_t numOuter(const cv::Mat* levelset) { return static_cast<std::size_t>(levelset->rows); }
	};

	struct OpDown : public FieldAccVertical
	{
		template<typename T>
		static constexpr T* op(T* p, std::ptrdiff_t val, int num = 1) { return p + val*num; }
		static constexpr const bool posDirection = true;
	};
	struct OpUp : public FieldAccVertical
	{
		template<typename T>
		static constexpr T* op(T* p, std::ptrdiff_t val, int num = 1) { return p - val*num; }
		static constexpr const bool posDirection = false;
	};

	struct OpRight : public FieldAccHorizontal
	{
		template<typename T>
		static constexpr T* op(T* p, std::ptrdiff_t, int num = 1) { return p + num; }
		static constexpr const bool posDirection = true;
	};
	struct OpLeft : public FieldAccHorizontal
	{
		template<typename T>
		static constexpr T* op(T* p, std::ptrdiff_t, int num = 1) { return p - num; }
		static constexpr const bool posDirection = false;
	};


	template<typename Operator>
	struct PartitionFromGrayValueWorker
	{
		inline static void iterateRow(cv::Mat* levelSetData
		                            , const cv::Mat&       img
		                            , const uint8_t        grayValue
		                            , const uint8_t        breakValue
		                            , const std::size_t    startInner
		                            , const std::size_t    posOuter
		                            , const std::size_t    numInner
		                            , const std::ptrdiff_t itLineNum
		                            , const int            neededStrikes
		                            , const double         negStrikesFactor)
		{
			BScanSegmentationMarker::internalMatType* levelSetIt  = Operator::template startIt      <BScanSegmentationMarker::internalMatType>(*levelSetData, startInner, posOuter);
			const uint8_t*                      imgIt       = Operator::template startIt_const<uint8_t>(img          , startInner, posOuter);

			int         negStri  = 0;
			int         strikes  = 0;
			std::size_t innerPos = 0;

			for(; innerPos < numInner; ++innerPos)
			{
				if(*imgIt >= grayValue)
				{
					if(strikes > neededStrikes || *imgIt == breakValue)
						break;
					++strikes;
				}
				else if(strikes > 0)
				{
					++negStri;
					if(negStri < strikes*negStrikesFactor)
						++strikes;
					else
					{
						strikes = 0;
						negStri = 0;
					}
				}

				*levelSetIt =  BScanSegmentationMarker::paintArea0Value;
				levelSetIt  = Operator::op(levelSetIt, itLineNum);
				imgIt       = Operator::op(imgIt     , itLineNum);
			}

			// go back to the begin of the founded shape
			assert(innerPos >= static_cast<std::size_t>(strikes));
			innerPos  -= strikes;
			levelSetIt = Operator::op(levelSetIt, itLineNum, -strikes);
			imgIt      = Operator::op(imgIt     , itLineNum, -strikes);

			for(; innerPos < numInner; ++innerPos)
			{
				*levelSetIt =  BScanSegmentationMarker::paintArea1Value;
				levelSetIt  = Operator::op(levelSetIt, itLineNum);
				imgIt       = Operator::op(imgIt     , itLineNum);
			}
		}

		static void iterateAbsolute(cv::Mat* levelSetData, const cv::Mat& img, const BScanSegmentationMarker::internalMatType grayValue, const int neededStrikes, const double negStrikesFactor)
		{
			const std::size_t    numInner   = Operator::numInner(levelSetData); // levelSetData->getSizeY();
			const std::size_t    numOuter   = Operator::numOuter(levelSetData); // levelSetData->getSizeX();
			const std::size_t    innerStart = Operator::posDirection?0:numInner-1;
			const std::ptrdiff_t itLineNum  = img.ptr<uint8_t>(1) - img.ptr<uint8_t>(0);

			for(size_t outerPos = 0; outerPos < numOuter; ++outerPos)
			{
				iterateRow(levelSetData, img, grayValue, std::numeric_limits<uint8_t>::max(), innerStart, outerPos, numInner, itLineNum, neededStrikes, negStrikesFactor);
			}
		}

		static void iterateRelativ(cv::Mat* levelSetData, const cv::Mat& img, double frac, const int neededStrikes, const double negStrikesFactor)
		{
			const std::size_t    numInner   = Operator::numInner(levelSetData);
			const std::size_t    numOuter   = Operator::numOuter(levelSetData);
			const std::size_t    innerStart = Operator::posDirection?0:numInner-1;
			const std::ptrdiff_t itLineNum  = img.ptr<uint8_t>(1) - img.ptr<uint8_t>(0);

			for(size_t outerPos = 0; outerPos < numOuter; ++outerPos)
			{
				const uint8_t* imgIt           = Operator::template startIt_const<uint8_t>(img, innerStart, outerPos);
				uint8_t        maxGrayValueCol = *imgIt;
				uint8_t        minGrayValueCol = *imgIt;

				for(size_t innerPos = 0; innerPos < numInner; ++innerPos)
				{
					if(maxGrayValueCol < *imgIt)
						maxGrayValueCol = *imgIt;
					else if(minGrayValueCol > *imgIt)
						minGrayValueCol = *imgIt;

					imgIt = Operator::op(imgIt, itLineNum);
				}
				const uint8_t grayValue = static_cast<uint8_t>((maxGrayValueCol-minGrayValueCol)*frac + minGrayValueCol);

				iterateRow(levelSetData, img, grayValue, maxGrayValueCol, innerStart, outerPos, numInner, itLineNum, neededStrikes, negStrikesFactor);
			}
		}

		static void initFromThresholdMethod(const cv::Mat& image, cv::Mat& segMat, const BScanSegmentationMarker::ThresholdDirectionData& data)
		{
			switch(data.method)
			{
				case BScanSegmentationMarker::ThresholdMethod::Absolute:
					iterateAbsolute(&segMat, image, data.absoluteValue, data.neededStrikes, data.negStrikesFactor);
					break;
				case BScanSegmentationMarker::ThresholdMethod::Relative:
					iterateRelativ(&segMat , image, data.relativeFrac , data.neededStrikes, data.negStrikesFactor);
					break;
			}
		}
	};



	void fillRow(BScanSegmentationMarker::internalMatType* colIt
	           , const std::size_t colSize
	           , const std::size_t rowSize
	           , BScanSegmentationMarker::internalMatType upperValue
	           , BScanSegmentationMarker::internalMatType lowerValue
	           , const std::size_t valueChangeOnRow)
	{
		const std::size_t rowCh = std::min(valueChangeOnRow, rowSize);

		for(std::size_t row = 0; row < rowCh; ++row)
		{
			*colIt = upperValue;
			colIt += colSize;
		}

		for(std::size_t row = rowCh; row < rowSize; ++row)
		{
			*colIt = lowerValue;
			colIt += colSize;
		}
	}

	void findUnemptyBroder(int colEnd
	                     , int rowAdd
	                     , int rowSize
	                     , int colSize
	                     , BScanSegmentationMarker::internalMatType* imgIt
	                     , BScanSegmentationMarker::internalMatType& upperValue
	                     , BScanSegmentationMarker::internalMatType& lowerValue
	                     , int& foundCol
	                     , int& foundRow
	)
	{
		for(int i = 0; i < colEnd; ++i)
		{
			BScanSegmentationMarker::internalMatType* colIt = imgIt;

			upperValue = *colIt;

			for(int j = 1; j < rowSize; ++j)
			{
				colIt += colSize;
				if(*colIt != upperValue)
				{
					lowerValue = *colIt;
					foundCol = i;
					foundRow = j;
					i = colEnd; // break outer for
					break;
				}
			}
			imgIt += rowAdd;
		}
	}
}

void BScanSegAlgorithm::initFromThresholdDirection(const cv::Mat& image, cv::Mat& segMat, const BScanSegmentationMarker::ThresholdDirectionData& data)
{
	assert(image.cols == segMat.cols);
	assert(image.rows == segMat.rows);
	assert(image.ptr<uint8_t>(1) - image.ptr<uint8_t>(0) == segMat.ptr<uint8_t>(1) - segMat.ptr<uint8_t>(0)); // line distance

	switch(data.direction)
	{
		case BScanSegmentationMarker::ThresholdDirectionData::Direction::down:
			PartitionFromGrayValueWorker<OpDown >::initFromThresholdMethod(image, segMat, data);
			break;
		case BScanSegmentationMarker::ThresholdDirectionData::Direction::up:
			PartitionFromGrayValueWorker<OpUp   >::initFromThresholdMethod(image, segMat, data);
			break;
		case BScanSegmentationMarker::ThresholdDirectionData::Direction::right:
			PartitionFromGrayValueWorker<OpRight>::initFromThresholdMethod(image, segMat, data);
			break;
		case BScanSegmentationMarker::ThresholdDirectionData::Direction::left:
			PartitionFromGrayValueWorker<OpLeft >::initFromThresholdMethod(image, segMat, data);
			break;
	}
}


void BScanSegAlgorithm::initFromThreshold(const cv::Mat& image, cv::Mat& segMat, const BScanSegmentationMarker::ThresholdData& data)
{
	assert(image.cols == segMat.cols);
	assert(image.rows == segMat.rows);

	BScanSegmentationMarker::internalMatType grayValue = data.absoluteValue;
	if(data.method == BScanSegmentationMarker::ThresholdMethod::Relative)
	{
		double minVal;
		double maxVal;
		cv::minMaxLoc(image, &minVal, &maxVal);
		grayValue = static_cast<BScanSegmentationMarker::internalMatType>((maxVal-minVal)*data.relativeFrac + minVal);
	}


	// accept only char type matrices
	CV_Assert(image.depth() == CV_8U);
	CV_Assert(image.channels() == 1);

	int nRows = image.rows;
	int nCols = image.cols;

	if(image.isContinuous())
	{
		nCols *= nRows;
		nRows = 1;
	}

	for(int i = 0;i < nRows; ++i)
	{
		const uint8_t* imgIt = image .ptr<uint8_t>(i);
		      uint8_t* segIt = segMat.ptr<uint8_t>(i);

		for(int j = 0;j < nCols; ++j)
		{
			*segIt = *imgIt<grayValue ? BScanSegmentationMarker::paintArea0Value : BScanSegmentationMarker::paintArea1Value;
			++imgIt;
			++segIt;
		}
	}

}

void BScanSegAlgorithm::initFromSegline(const OctData::BScan& bscan, cv::Mat& segMat)
{
	if(!segMat.empty())
	{
		const OctData::BScan::Segmentline& segline = bscan.getSegmentLine(OctData::BScan::SegmentlineType::ILM);
		BScanSegmentationMarker::internalMatType* colIt = segMat.ptr<BScanSegmentationMarker::internalMatType>();

		std::size_t colSize = static_cast<std::size_t>(segMat.cols);
		std::size_t rowSize = static_cast<std::size_t>(segMat.rows);

		for(double value : segline)
		{
			fillRow(colIt, colSize, rowSize, BScanSegmentationMarker::paintArea0Value, BScanSegmentationMarker::paintArea1Value, static_cast<std::size_t>(value));
// 			const std::size_t rowCh = std::min(static_cast<std::size_t>(value), rowSize);
// 			BScanSegmentationMarker::internalMatType* rowIt = colIt;
//
// 			for(std::size_t row = 0; row < rowCh; ++row)
// 			{
// 				*rowIt = BScanSegmentationMarker::paintArea0Value;
// 				rowIt += colSize;
// 			}
//
// 			for(std::size_t row = rowCh; row < rowSize; ++row)
// 			{
// 				*rowIt = BScanSegmentationMarker::paintArea1Value;
// 				rowIt += colSize;
// 			}

			++colIt;
		}
	}

}



void BScanSegAlgorithm::openClose(cv::Mat& dest, cv::Mat* src)
{
	if(!src)
		src = &dest;

	int iterations = 1;
	cv::erode (*src, dest, cv::Mat(), cv::Point(-1, -1), iterations  , cv::BORDER_REFLECT_101, 1);
	cv::dilate(dest, dest, cv::Mat(), cv::Point(-1, -1), iterations*2, cv::BORDER_REFLECT_101, 1);
	cv::erode (dest, dest, cv::Mat(), cv::Point(-1, -1), iterations  , cv::BORDER_REFLECT_101, 1);
}


bool BScanSegAlgorithm::removeUnconectedAreas(cv::Mat& image)
{
	int rows = image.rows;
	int cols = image.cols;

	int posX = cols/2;
	int posY1 = 0;
	int posY2 = rows - 1;

	auto v1 = image.at<BScanSegmentationMarker::internalMatType>(cv::Point(posX, posY1));
	auto v2 = image.at<BScanSegmentationMarker::internalMatType>(cv::Point(posX, posY2));

	if(v1 == v2)
		return false;

	cv::floodFill(image, cv::Point(posX , posY1), 255); // save upper area
	cv::floodFill(image, cv::Point(posX , posY2), v1);  // convert v2 -> v1 : v1 areas in lower scope is included in lower area
	cv::floodFill(image, cv::Point(posX , posY2), 254); // save lower area
	cv::floodFill(image, cv::Point(posX , posY1), v2);  // convert v1 -> v2 : work on upper area
	cv::floodFill(image, cv::Point(posX , posY1), v1);  // retrieval upper area
	cv::floodFill(image, cv::Point(posX , posY2), v2);  // retrieval lower area
	return true;
}




bool BScanSegAlgorithm::extendLeftRightSpace(cv::Mat& image, int limit)
{
	if(!image.isContinuous())
		return false;

	if(limit < 0)
		limit = std::numeric_limits<int>::max();

	int rowSize = image.rows;
	int colSize = image.cols;

	int endCol1 = std::min(colSize, limit);
	int endCol2 = std::max(0, rowSize-limit);

	int foundCol = 0;
	int foundRow = 0;

	BScanSegmentationMarker::internalMatType upperValue;
	BScanSegmentationMarker::internalMatType lowerValue;

	BScanSegmentationMarker::internalMatType* imgIt = image.ptr<BScanSegmentationMarker::internalMatType>(0);
	findUnemptyBroder(endCol1, 1, rowSize, colSize, imgIt, upperValue, lowerValue, foundCol, foundRow);

	for(int i = 0; i < foundCol; ++i)
	{
		fillRow(imgIt, colSize, rowSize, upperValue, lowerValue, static_cast<std::size_t>(foundRow));
		++imgIt;
	}


	imgIt = image.ptr<BScanSegmentationMarker::internalMatType>(1)-1; // end of line 0
	findUnemptyBroder(endCol2, -1, rowSize, colSize, imgIt, upperValue, lowerValue, foundCol, foundRow);

	for(int i = 0; i < foundCol; ++i)
	{
		fillRow(imgIt, colSize, rowSize, upperValue, lowerValue, static_cast<std::size_t>(foundRow));
		--imgIt;
	}

	return true;
}

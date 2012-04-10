/******************************************************************************
 * \file
 *
 * $Id: sceneModel.cpp 397 2012-03-29 12:50:30Z spanel $
 *
 * Copyright (C) Brno University of Technology
 *
 * This file is part of software developed by dcgm-robotics@FIT group.
 *
 * Author: Rostislav Hulik (ihulik@fit.vutbr.cz)
 * Supervised by: Michal Spanel (spanel@fit.vutbr.cz)
 * Date: 11.01.2012 (version 0.8)
 * 
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Description:
 *
 */

#include "plane_det/sceneModel.h"
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include "plane_det/filtering.h"
#include "plane_det/parameterSpace.h"
#include "plane_det/parameterSpaceHierarchy.h"

using namespace pcl;
using namespace cv;

namespace but_scenemodel
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SceneModel::SceneModel(Mat &depth, const CameraInfoConstPtr& cam_info, Normals &normals) : 	scene_cloud(new PointCloud<PointXYZRGB>),
											current_hough_cloud(new PointCloud<PointXYZI>)
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Using hierarchic array
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
		int maxi = normals.m_points.rows;
		int maxj = normals.m_points.cols;
		Vec3f point;
		Vec4f plane;
		Regions reg(&normals);
		reg.watershedRegions(depth, cam_info, WatershedType::DepthDiff, 1, 2, 20);
		double min, max;
		minMaxLoc(reg.m_regionMatrix, &min, &max);
		double indexFactor = 1.0;

		ParameterSpaceHierarchy space(-M_PI, M_PI, -40.0, 40.0, 512, 4096);
		std::cout << "Parameter space size: " << space.getSize()*sizeof(double) / 1000000.0 << " MB" << std::endl;
		std::cout << "Parameter shift step: " << space.m_shiftStep << std::endl;
		std::cout << "Parameter angle step: " << space.m_angleStep << std::endl;

		ParameterSpace gauss(-5 * space.m_angleStep, 5 * space.m_angleStep, -5 * space.m_shiftStep, 5 * space.m_shiftStep, 11, 11);
		std::cout << "Gauss space size: " << gauss.m_size*sizeof(double) / 1000000.0 << " MB" << std::endl;
		std::cout << "Gauss shift step: " << gauss.m_shiftStep << std::endl;
		std::cout << "Gauss angle step: " << gauss.m_angleStep << std::endl;

		gauss.generateGaussIn(0.04, 0.15);

		if (max !=0)
			indexFactor /= (double)max;
		//PointCloud<PointXYZINormal>::Ptr aux(new PointCloud<PointXYZINormal>);

	//	for (int i = 0; i < depth.rows; ++i)
	//			for (int j = 0; j < depth.cols; ++j)
	//			{
	//				if (depth.at<unsigned short>(i, j) > 3000)
	//					depth.at<unsigned short>(i, j) = 0;
	//			}

		for (int i = 0; i < maxi; ++i)
			for (int j = 0; j < maxj; ++j)
			{
				point = normals.m_points.at<Vec3f>(i, j);
				plane = normals.m_planes.at<Vec4f>(i, j);
				//pcl::Ve
				//if (point[2] < 3.0)
				{
				// signed angle atan2(b.y,b.x) - atan2(a.y,a.x)
				// angle on XZ plane with X
				float a1, a2;
				ParameterSpace::toAngles(plane[0], plane[1], plane[2], a1, a2);


				PointXYZRGB rgbpoint(255, 255, 255);
				if (reg.m_regionMatrix.at<int>(i, j) > 0)
					rgbpoint.rgb = indexFactor *reg.m_regionMatrix.at<int>(i, j);
	//			rgbpoint.x = a1;
	//			rgbpoint.y = a2;
	//			rgbpoint.z = plane[3];

				rgbpoint.x = point[0];
				rgbpoint.y = point[1];
				rgbpoint.z = point[2];
				scene_cloud->push_back(rgbpoint);

				int i, j, k;
				space.getIndex(a1, a2, plane[3], i, j, k);
				space.addVolume(gauss, i, j, k);
				}
			}
	//
	std::cout << "New parameter space size: " << (double)space.getSize()*sizeof(double) / 1000000.0 << " MB" << std::endl;
	//		max_plane = space.findMaxima(planes);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// End using hierarchic array
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Using static array
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	int maxi = normals.m_points.rows;
	//	int maxj = normals.m_points.cols;
	//	Vec3f point;
	//	Vec4f plane;
	//	Regions reg(&normals);
	//	reg.watershedRegions(depth, cam_info, WatershedType::DepthDiff, 1, 2, 20);
	//	double min, max;
	//	minMaxLoc(reg.m_regionMatrix, &min, &max);
	//	double indexFactor = 1.0;
	//
	//	ParameterSpace space(-M_PI, M_PI, -5.0, 5.0, 512, 512);
	//	std::cout << "Parameter space size: " << space.m_size / 1000000.0 << " MB" << std::endl;
	//	std::cout << "Parameter shift step: " << space.m_shiftStep << std::endl;
	//	std::cout << "Parameter angle step: " << space.m_angleStep << std::endl;
	//
	//	ParameterSpace gauss(-5 * space.m_angleStep, 5 * space.m_angleStep, -5 * space.m_shiftStep, 5 * space.m_shiftStep, 11, 11);
	//	std::cout << "Gauss space size: " << gauss.m_size / 1000000.0 << " MB" << std::endl;
	//	std::cout << "Gauss shift step: " << gauss.m_shiftStep << std::endl;
	//	std::cout << "Gauss angle step: " << gauss.m_angleStep << std::endl;
	//
	//	gauss.generateGaussIn(0.04, 0.15);
	//
	//	if (max !=0)
	//		indexFactor /= (double)max;
	//	//PointCloud<PointXYZINormal>::Ptr aux(new PointCloud<PointXYZINormal>);
	//
	////	for (int i = 0; i < depth.rows; ++i)
	////			for (int j = 0; j < depth.cols; ++j)
	////			{
	////				if (depth.at<unsigned short>(i, j) > 3000)
	////					depth.at<unsigned short>(i, j) = 0;
	////			}
	//
	//	for (int i = 0; i < maxi; ++i)
	//		for (int j = 0; j < maxj; ++j)
	//		{
	//			point = normals.m_points.at<Vec3f>(i, j);
	//			plane = normals.m_planes.at<Vec4f>(i, j);
	//			//pcl::Ve
	//			//if (point[2] < 3.0)
	//			{
	//			// signed angle atan2(b.y,b.x) - atan2(a.y,a.x)
	//			// angle on XZ plane with X
	//			float a1, a2;
	//			ParameterSpace::toAngles(plane[0], plane[1], plane[2], a1, a2);
	//
	//
	//			PointXYZRGB rgbpoint(255, 255, 255);
	//			if (reg.m_regionMatrix.at<int>(i, j) > 0)
	//				rgbpoint.rgb = indexFactor *reg.m_regionMatrix.at<int>(i, j);
	//			rgbpoint.x = a1;
	//			rgbpoint.y = a2;
	//			rgbpoint.z = plane[3];
	//
	//			rgbpoint.x = point[0];
	//			rgbpoint.y = point[1];
	//			rgbpoint.z = point[2];
	//			scene_cloud->push_back(rgbpoint);
	//
	//			int i, j, k;
	//			space.getIndex(a1, a2, plane[3], i, j, k);
	//			space.addVolume(gauss, i, j, k);
	//			}
	//		}
	//
	//		max_plane = space.findMaxima(planes);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// End sing static array
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Control visualisaation - uncoment to see HT space
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		min = 99999999999.0;
		max = -99999999999.0;
		for (int shift = 0; shift < space.m_shiftSize; shift += 1)
		for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
		for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
		{
			float value = space.get(angle1, angle2, shift);
			if (value < min) min = value;
			if (value > max) max = value;

//			if (space(angle1, angle2, shift) > 0.0)
//			{
//			PointXYZI pt;
//			pt.x = (float)angle1;
//			pt.y = (float)angle2;
//			pt.z = (float)shift;
//			pt.intensity = space(angle1, angle2, shift);
//			current_hough_cloud->push_back(pt);
//			}
		}

		for (int shift = 0; shift < space.m_shiftSize; shift += 1)
		for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
		for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
		{
			if (space.get(angle1, angle2, shift) != 0)
				space.set(angle1, angle2, shift, 255.0*((space.get(angle1, angle2, shift) - min) / (max - min)));
		}

		cv::Mat image = cv::Mat::zeros(cvSize(space.m_angleSize, space.m_angleSize), CV_32FC1);
		int shiftview = space.m_shiftSize/2;
		for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
		for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
		{
			image.at<float>(angle1, angle2) = space.get(angle1, angle2, shiftview);
		}



		//create a new window & display the image
		cvNamedWindow("Smile", 1);
		cvShowImage("Smile", &IplImage(image));
		std::cout << "Viewing shift = " << shiftview << std::endl;
		//wait for key to close the window
		int key = 0;
		while(1)
		{
		    key = cvWaitKey();
		    key &= 0x0000ffff;
		    std::cout << key << std::endl;
		    if(key==27 || key == 0xffff) break;

		    switch(key)
		    {
		        case 'a':
		        	if (shiftview < space.m_shiftSize-1)
		        	{
		        		++shiftview;
		        		for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
		        		for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
		        		{
		        			image.at<float>(angle1, angle2) = space.get(angle1, angle2, shiftview);
		        		}
		        	}
		        	cvShowImage("Smile", &IplImage(image));
		        	std::cout << "Viewing shift = " << shiftview << "/" << space.m_shiftSize-1 << std::endl;
		            break;
		        case 'z':
		        	if (shiftview > 0)
		        	{
		        		--shiftview;
		        		for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
		        		for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
		        		{
		        			image.at<float>(angle1, angle2) = space.get(angle1, angle2, shiftview);
		        		}
		        	}
		        	cvShowImage("Smile", &IplImage(image));
		        	std::cout << "Viewing shift = " << shiftview << "/" << space.m_shiftSize-1 << std::endl;
		            break;
		    }
		}


		cvDestroyWindow( "Smile" );



}
}
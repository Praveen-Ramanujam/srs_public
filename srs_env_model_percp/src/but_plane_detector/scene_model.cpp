/******************************************************************************
 * \file
 *
 * $Id: sceneModel.cpp 777 2012-05-11 11:23:17Z ihulik $
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
 *	 Class encapsulating A scene model (i.e. found planes)
 */

#include <srs_env_model_percp/but_plane_detector/scene_model.h>
#include <srs_env_model_percp/but_seg_utils/filtering.h>

#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/filters/statistical_outlier_removal.h>

using namespace pcl;
using namespace cv;

namespace srs_env_model_percp
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Constructor - initializes a scene and allocates necessary space - a space of (angle, angle, d) where angles are angles of plane normal and d is d parameter of plane equation.
	// @param max_depth Maximum computed depth by each frame in meters (default 3.0)
	// @param min_shift Minimal d parameter value (ax + by + cz + d = 0) in Hough space (default -40.0)
	// @param max_shift Maximal d parameter value (ax + by + cz + d = 0) in Hough space (default 40.0)
	// @param angle_resolution Angle coordinates resolution (Hough space size in angle directions) (default 512)
	// @param shift_resolution d parameter coordinates resolution (Hough space size in angle directions) (default 4096)
	// @param gauss_angle_res Angle resolution of added Gauss function (default 11)
	// @param gauss_shift_res d parameter resolution of added Gauss function (default 11)
	// @param gauss_angle_sigma Sigma of added Gauss function in angle coordinates (default 11)
	// @param gauss_shift_sigma Sigma of added Gauss function in d parameter coordinates (default 11)
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SceneModel::SceneModel(	double max_depth,
							double min_shift,
							double max_shift,
							int angle_resolution,
							int shift_resolution,
							int gauss_angle_res,
							int gauss_shift_res,
							double gauss_angle_sigma,
							double gauss_shift_sigma) :	scene_cloud(new PointCloud<PointXYZRGB>),
														space(-M_PI, M_PI, min_shift, max_shift, angle_resolution, shift_resolution),
														current_space(-M_PI, M_PI, min_shift, max_shift, angle_resolution, shift_resolution),
														gauss(-(gauss_angle_res/2) * space.m_angleStep, (gauss_angle_res/2) * space.m_angleStep, -(gauss_shift_res/2) * space.m_shiftStep, (gauss_shift_res/2) * space.m_shiftStep, gauss_angle_res, gauss_shift_res)
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Using hierarchic array
		/////////////////////////////////////////////////////////////////////////////////////////////////////////

		m_angle_min = -M_PI;
		m_angle_max = M_PI;
		m_shift_min = min_shift;
		m_shift_max = max_shift;
		m_angle_res = angle_resolution;
		m_shift_res = shift_resolution;

		m_depth = max_depth;

		// init of HS
		std::cout << "Parameter space size: " << space.getSize()*sizeof(double) / 1000000.0 << " MB" << std::endl;
		std::cout << "Parameter shift step: " << space.m_shiftStep << std::endl;
		std::cout << "Parameter angle step: " << space.m_angleStep << std::endl;
		std::cout << "Gauss space size: " << gauss.m_size*sizeof(double) / 1000000.0 << " MB" << std::endl;
		std::cout << "Gauss shift step: " << gauss.m_shiftStep << std::endl;
		std::cout << "Gauss angle step: " << gauss.m_angleStep << std::endl;

		// generate Gauss function in gauss space
		gauss.generateGaussIn(gauss_angle_sigma, gauss_shift_sigma);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Function recomputes a list of planes saved in this class (scene model)
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SceneModel::recomputePlanes()
	{
		planes.clear();
		std::vector<int> counts;
		std::vector<Plane<float> > aux;
		current_space.findMaxima(aux, 500);
		int total_count = 0;

		std::vector<bool> used(aux.size(), false);
		for (unsigned int i = 0; i < aux.size(); ++i)
		if (not used[i])
		{
			used[i] = true;
			Plane<float> final(aux[i].a, aux[i].b, aux[i].c, aux[i].d);
			int count = 1;
			for (unsigned int j = i+1; j < aux.size(); ++j)
			if (not used[j] && aux[i].isSimilar(aux[j], 0.01, 0.01))
			{
				used[j] = true;
				final.a += aux[j].a;
				final.b += aux[j].b;
				final.c += aux[j].c;
				final.d += aux[j].d;
				++count;
			}

			final.a /= count;
			final.b /= count;
			final.c /= count;
			final.d /= count;
			planes.push_back(final);
			counts.push_back(count);
			total_count += count;
		}

		float a1, a2;
		int ai1, ai2, zi;
		// apply planes to the global model as gauss functions
		for (unsigned int i = 0; i < planes.size(); ++i)
		{
			current_space.toAngles(planes[i].a, planes[i].b, planes[i].c, a1, a2);
			current_space.getIndex(a1, a2, planes[i].d, ai1, ai2, zi);

			space.addVolume(gauss, ai1, ai2, zi);
		}
		std::cout << "Adding into global" << std::endl;
		/////////////////////////////////////////////////////////////////////////////////////////////////
//					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//					// Control visualisaation - uncoment to see HT space
//					//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							double min = 99999999999.0;
//							double max = -99999999999.0;
//
//							ParameterSpaceHierarchyFullIterator it(&space);
//							while (not it.end)
//							{
//								float value = it.getVal();
//								if (value < min) min = value;
//								if (value > max) max = value;
//								++it;
//							}
//
//							ParameterSpaceHierarchyFullIterator it2(&space);
//							while (not it2.end)
//							{
//								if (it2.getVal() != 0)
//									it2.setVal(1.0*((it2.getVal() - min) / (max - min)));
//								++it2;
//							}
//
//							cv::Mat image = cv::Mat::zeros(cvSize(space.m_angleSize, space.m_angleSize), CV_32FC1);
//							int shiftview = space.m_shiftSize/2;
//							for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
//							for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
//							{
//								image.at<float>(angle1, angle2) = space.get(angle1, angle2, shiftview);
//							}
//
//
//
//							//create a new window & display the image
//							cvNamedWindow("Smile", 1);
//							cvShowImage("Smile", &IplImage(image));
//							std::cout << "Viewing shift = " << shiftview << std::endl;
//							//wait for key to close the window
//							int key = 0;
//							while(1)
//							{
//							    key = cvWaitKey();
//							    key &= 0x0000ffff;
//							    std::cout << key << std::endl;
//							    if(key==27 || key == 0xffff) break;
//
//							    switch(key)
//							    {
//							        case 'a':
//							        	if (shiftview < space.m_shiftSize-1)
//							        	{
//							        		++shiftview;
//							        		for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
//							        		for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
//							        		{
//							        			image.at<float>(angle1, angle2) = space.get(angle1, angle2, shiftview);
//							        		}
//							        	}
//							        	cvShowImage("Smile", &IplImage(image));
//							        	std::cout << "Viewing shift = " << shiftview << "/" << space.m_shiftSize-1 << std::endl;
//							            break;
//							        case 'z':
//							        	if (shiftview > 0)
//							        	{
//							        		--shiftview;
//							        		for (int angle1 = 0; angle1 < space.m_angleSize; angle1 += 1)
//							        		for (int angle2 = 0; angle2 < space.m_angleSize; angle2 += 1)
//							        		{
//							        			image.at<float>(angle1, angle2) = space.get(angle1, angle2, shiftview);
//							        		}
//							        	}
//							        	cvShowImage("Smile", &IplImage(image));
//							        	std::cout << "Viewing shift = " << shiftview << "/" << space.m_shiftSize-1 << std::endl;
//							            break;
//							    }
//							}
//
//
//							cvDestroyWindow( "Smile" );
		/////////////////////////////////////////////////////////////////////////////////////////////////
		used.resize(planes.size(), false);
		planes.clear();
		used.clear();

		aux.clear();
		counts.clear();
		space.findMaxima(planes, 0.7);
//		for (unsigned int i = 0; i < aux.size(); ++i)
//
//		if (not used[i])
//		{
//			used[i] = true;
//			Plane<float> final(aux[i].a, aux[i].b, aux[i].c, aux[i].d);
//			int count = 1;
//			for (unsigned int j = i+1; j < aux.size(); ++j)
//			if (not used[j] && aux[i].isSimilar(aux[j], 0.01, 0.01))
//			{
//				used[j] = true;
//				final.a += aux[j].a;
//				final.b += aux[j].b;
//				final.c += aux[j].c;
//				final.d += aux[j].d;
//				++count;
//			}
//
//			final.a /= count;
//			final.b /= count;
//			final.c /= count;
//			final.d /= count;
//			planes.push_back(final);
//			counts.push_back(count);
//			total_count += count;
//		}

		std::cout << "Found : " << planes.size() << " planes." << std::endl;
	}

	void SceneModel::clearNoise(double minValue)
	{
		ParameterSpaceHierarchyFullIterator it(&space);

		// pass all non null points
		double val;
		while (not it.end)
		{
			val = it.getVal();
			if (val != 0.0 && val < minValue)
			{
				it.setVal(0.0);
			}
			++it;
		}
	}

	void SceneModel::AddNext(Normals &normals)
	{
		ParameterSpaceHierarchy cache_space(m_angle_min, m_angle_max, m_shift_min, m_shift_max, m_angle_res, m_shift_res);
		current_space.clear();

		int maxi = normals.m_points.rows;
		int maxj = normals.m_points.cols;

		Vec3f point;
		Vec4f plane;
		indexFactor = 1.0;

		scene_cloud->clear();
		// pass all points and write them into init space
		for (int i = 0; i < maxi; ++i)
		for (int j = 0; j < maxj; ++j)
		{
			point = normals.m_points.at<Vec3f>(i, j);
			plane = normals.m_planes.at<Vec4f>(i, j);

//			// skip all which is farer than 3m
//			if (point[2] < m_depth)
			if (plane[0] != 0.0 || plane[1] != 0.0 || plane[2] != 0.0)
			{
				// signed angle atan2(b.y,b.x) - atan2(a.y,a.x)
				// angle on XZ plane with X
				float a1, a2;
				ParameterSpace::toAngles(plane[0], plane[1], plane[2], a1, a2);


				PointXYZRGB rgbpoint(255, 255, 255);
				rgbpoint.x = point[0];
				rgbpoint.y = point[1];
				rgbpoint.z = point[2];
				rgbpoint.r = (plane[0]+1)*128;
				rgbpoint.g = (plane[1]+1)*128;
				rgbpoint.b = (plane[2]+1)*128;
				scene_cloud->push_back(rgbpoint);

				int i, j, k;
				cache_space.getIndex(a1, a2, plane[3], i, j, k);
				if (i < cache_space.m_angleSize && j < cache_space.m_angleSize && k < cache_space.m_shiftSize &&
					i >= 0 && j >= 0 && k >= 0)
					cache_space.set(i, j, k, cache_space.get(i, j, k) + 1);
			}
		}

		// fire up the iterator on cache space
		ParameterSpaceHierarchyFullIterator it(&cache_space);
		int i, j, k;
		double val;
		// for each point in cache space which is not zero, write a multiplied gauss into the HT
		while (not it.end)
		{
			val = it.getVal();
			if (val > 0.0)
			{
				current_space.fromIndex(it.currentI, i, j, k);
				current_space.addVolume(gauss, i, j, k, val);
			}
			++it;
		}

		std::cout << "New parameter space size: " << (double)current_space.getSize()*sizeof(double) / 1000000.0 << " MB" << std::endl;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Function adds a depth map with computed normals into existing Hough space
	// @param depth Depth image
	// @param cam_info Camera info object
	// @param normals Computed normals of depth image
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SceneModel::AddNext(Mat &depth, const sensor_msgs::CameraInfoConstPtr& cam_info, Normals &normals)
	{
		ParameterSpaceHierarchy cache_space(m_angle_min, m_angle_max, m_shift_min, m_shift_max, m_angle_res, m_shift_res);
		current_space.clear();

		int maxi = normals.m_points.rows;
		int maxj = normals.m_points.cols;

		Vec3f point;
		Vec4f plane;
		//Regions reg(&normals);
		//reg.watershedRegions(depth, cam_info, WatershedType::DepthDiff, 1, 2, 20);
		//double min, max;
		//minMaxLoc(reg.m_regionMatrix, &min, &max);
		indexFactor = 1.0;

		//if (max !=0)
		//	indexFactor /= (double)max;

		scene_cloud->clear();
		// pass all points and write them into init space
		for (int i = 0; i < maxi; ++i)
			for (int j = 0; j < maxj; ++j)
			{
				point = normals.m_points.at<Vec3f>(i, j);
				plane = normals.m_planes.at<Vec4f>(i, j);

				// skip all which is farer than 3m
				//if (point[2] < m_depth)
				//{
					// signed angle atan2(b.y,b.x) - atan2(a.y,a.x)
					// angle on XZ plane with X
					float a1, a2;
					ParameterSpace::toAngles(plane[0], plane[1], plane[2], a1, a2);


					PointXYZRGB rgbpoint(255, 255, 255);
					//if (reg.m_regionMatrix.at<int>(i, j) > 0)
					//	rgbpoint.rgb = indexFactor *reg.m_regionMatrix.at<int>(i, j);

					rgbpoint.x = point[0];
					rgbpoint.y = point[1];
					rgbpoint.z = point[2];
					scene_cloud->push_back(rgbpoint);

					int i, j, k;
					cache_space.getIndex(a1, a2, plane[3], i, j, k);
					cache_space.set(i, j, k, cache_space.get(i, j, k) + 1);
				//}
			}

			// fire up the iterator on cache space
			ParameterSpaceHierarchyFullIterator it(&cache_space);
			int i, j, k;
			double val;
			// for each point in cache space which is not zero, write a multiplied gauss into the HT
			while (not it.end)
			{
				val = it.getVal();
				if (val > 0.0)
				{
					current_space.fromIndex(it.currentI, i, j, k);
					current_space.addVolume(gauss, i, j, k, val);
				}
				++it;
			}

			std::cout << "New parameter space size: " << (double)current_space.getSize()*sizeof(double) / 1000000.0 << " MB" << std::endl;

//			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//			// Control visualisaation - uncoment to see HT space
//			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//					min = 99999999999.0;
//					max = -99999999999.0;
//					for (int shift = 0; shift < current_space.m_shiftSize; shift += 1)
//					for (int angle1 = 0; angle1 < current_space.m_angleSize; angle1 += 1)
//					for (int angle2 = 0; angle2 < current_space.m_angleSize; angle2 += 1)
//					{
//						float value = current_space.get(angle1, angle2, shift);
//						if (value < min) min = value;
//						if (value > max) max = value;
//
//			//			if (space(angle1, angle2, shift) > 0.0)
//			//			{
//			//			PointXYZI pt;
//			//			pt.x = (float)angle1;
//			//			pt.y = (float)angle2;
//			//			pt.z = (float)shift;
//			//			pt.intensity = space(angle1, angle2, shift);
//			//			current_hough_cloud->push_back(pt);
//			//			}
//					}
//
//					for (int shift = 0; shift < current_space.m_shiftSize; shift += 1)
//					for (int angle1 = 0; angle1 < current_space.m_angleSize; angle1 += 1)
//					for (int angle2 = 0; angle2 < current_space.m_angleSize; angle2 += 1)
//					{
//						if (current_space.get(angle1, angle2, shift) != 0)
//							current_space.set(angle1, angle2, shift, 255.0*((current_space.get(angle1, angle2, shift) - min) / (max - min)));
//					}
//
//					cv::Mat image = cv::Mat::zeros(cvSize(current_space.m_angleSize, current_space.m_angleSize), CV_32FC1);
//					int shiftview = current_space.m_shiftSize/2;
//					for (int angle1 = 0; angle1 < current_space.m_angleSize; angle1 += 1)
//					for (int angle2 = 0; angle2 < current_space.m_angleSize; angle2 += 1)
//					{
//						image.at<float>(angle1, angle2) = current_space.get(angle1, angle2, shiftview);
//					}
//
//
//
//					//create a new window & display the image
//					cvNamedWindow("Smile", 1);
//					cvShowImage("Smile", &IplImage(image));
//					std::cout << "Viewing shift = " << shiftview << std::endl;
//					//wait for key to close the window
//					int key = 0;
//					while(1)
//					{
//					    key = cvWaitKey();
//					    key &= 0x0000ffff;
//					    std::cout << key << std::endl;
//					    if(key==27 || key == 0xffff) break;
//
//					    switch(key)
//					    {
//					        case 'a':
//					        	if (shiftview < current_space.m_shiftSize-1)
//					        	{
//					        		++shiftview;
//					        		for (int angle1 = 0; angle1 < current_space.m_angleSize; angle1 += 1)
//					        		for (int angle2 = 0; angle2 < current_space.m_angleSize; angle2 += 1)
//					        		{
//					        			image.at<float>(angle1, angle2) = current_space.get(angle1, angle2, shiftview);
//					        		}
//					        	}
//					        	cvShowImage("Smile", &IplImage(image));
//					        	std::cout << "Viewing shift = " << shiftview << "/" << current_space.m_shiftSize-1 << std::endl;
//					            break;
//					        case 'z':
//					        	if (shiftview > 0)
//					        	{
//					        		--shiftview;
//					        		for (int angle1 = 0; angle1 < current_space.m_angleSize; angle1 += 1)
//					        		for (int angle2 = 0; angle2 < current_space.m_angleSize; angle2 += 1)
//					        		{
//					        			image.at<float>(angle1, angle2) = current_space.get(angle1, angle2, shiftview);
//					        		}
//					        	}
//					        	cvShowImage("Smile", &IplImage(image));
//					        	std::cout << "Viewing shift = " << shiftview << "/" << current_space.m_shiftSize-1 << std::endl;
//					            break;
//					    }
//					}
//
//
//					cvDestroyWindow( "Smile" );
	}

	void SceneModel::AddNext(Mat &depth, const sensor_msgs::CameraInfoConstPtr& cam_info, Normals &normals, tf::StampedTransform &sensorToWorldTf, tf::StampedTransform &worldToSensorTf)
	{
//		ParameterSpaceHierarchy cache_space(m_angle_min, m_angle_max, m_shift_min, m_shift_max, m_angle_res, m_shift_res);
//		current_space.clear();
//
//		int maxi = normals.m_points.rows;
//		int maxj = normals.m_points.cols;
//
//		Vec3f point;
//		Vec4f plane;
//		Regions reg(&normals);
//		reg.watershedRegions(depth, cam_info, WatershedType::DepthDiff, 1, 2, 20);
//		double min, max;
//		minMaxLoc(reg.m_regionMatrix, &min, &max);
//		indexFactor = 1.0;
//
//		if (max !=0)
//			indexFactor /= (double)max;
//
//		scene_cloud->clear();
//		// pass all points and write them into init space
//		for (int i = 0; i < maxi; ++i)
//			for (int j = 0; j < maxj; ++j)
//			{
//				point = normals.m_points.at<Vec3f>(i, j);
//				plane = normals.m_planes.at<Vec4f>(i, j);
//
//				///////////////////////////////////////////////////////////////////
////				tf::Transformer t;
////				t.setTransform(sensorToWorldTf);
////				tf::Stamped<btVector3> originalPoint, transformedPoint;
////				originalPoint.setX(point[0]);
////				originalPoint.setY(point[1]);
////				originalPoint.setZ(point[2]);
////				originalPoint.frame_id_ = "/head_cam3d_link";
////				t.transformPoint("/map", originalPoint, transformedPoint);
////
//////				//t.setTransform(sensorToWorldTf);
////				tf::Stamped<btVector3> originalNormal, transformedNormal;
//////				transformedNormal.setX(plane[0]);
//////				transformedNormal.setY(plane[1]);
//////				transformedNormal.setZ(plane[2]);
//////				transformedNormal.frame_id_ = "/head_cam3d_link";
//////				t.transformVector("/map", originalNormal, transformedNormal);
//				///////////////////////////////////////////////////////////////////
//				if (i%100 == 0 && j % 100 == 0)
//				{
//					std::cout << point[0]*plane[0] + point[1]*plane[1] + point[2]*plane[2] << "   " << plane[3] << std::endl;
//					std::cout << transformedNormal.getX()*transformedPoint.getX() +
//								 transformedNormal.getY()*transformedPoint.getY() +
//								 transformedNormal.getZ()*transformedPoint.getZ() << std::endl << std::endl;
//				}
//				// skip all which is farer than 3m
//				if (point[2] < m_depth)
//				{
//					// signed angle atan2(b.y,b.x) - atan2(a.y,a.x)
//					// angle on XZ plane with X
//					float a1, a2;
//					ParameterSpace::toAngles(plane[0], plane[1], plane[2], a1, a2);
//
//
//					PointXYZRGB rgbpoint(255, 255, 255);
//					if (reg.m_regionMatrix.at<int>(i, j) > 0)
//						rgbpoint.rgb = indexFactor *reg.m_regionMatrix.at<int>(i, j);
//
//					rgbpoint.x = point[0];
//					rgbpoint.y = point[1];
//					rgbpoint.z = point[2];
//					scene_cloud->push_back(rgbpoint);
//
//					int i, j, k;
//					cache_space.getIndex(a1, a2, plane[3], i, j, k);
//					cache_space.set(i, j, k, cache_space.get(i, j, k) + 1);
//				}
//			}
//
//			// fire up the iterator on cache space
//			ParameterSpaceHierarchyFullIterator it(&cache_space);
//			int i, j, k;
//			double val;
//			// for each point in cache space which is not zero, write a multiplied gauss into the HT
//			while (not it.end)
//			{
//				val = it.getVal();
//				if (val > 0.0)
//				{
//					current_space.fromIndex(it.currentI, i, j, k);
//					current_space.addVolume(gauss, i, j, k, val);
//				}
//				++it;
//			}
//
//			std::cout << "New parameter space size: " << (double)current_space.getSize()*sizeof(double) / 1000000.0 << " MB" << std::endl;
	}
} // but_plane_detector

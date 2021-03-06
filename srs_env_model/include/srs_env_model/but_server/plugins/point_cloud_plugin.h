/******************************************************************************
 * \file
 *
 * $Id:$
 *
 * Copyright (C) Brno University of Technology
 *
 * This file is part of software developed by dcgm-robotics@FIT group.
 *
 * Author: Vit Stancl (stancl@fit.vutbr.cz)
 * Supervised by: Michal Spanel (spanel@fit.vutbr.cz)
 * Date: dd/mm/2012
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
#pragma once
#ifndef PointCloudPlugin_H_included
#define PointCloudPlugin_H_included

#include <srs_env_model/but_server/server_tools.h>

#include <message_filters/subscriber.h>
#include <sensor_msgs/PointCloud2.h>
#include <tf/message_filter.h>

#include <message_filters/sync_policies/exact_time.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/pass_through.h>

// PCL includes
#include <pcl/segmentation/extract_polygonal_prism_data.h>


namespace srs_env_model
{

    class CPointCloudPlugin : public CServerPluginBase, public COctomapCrawlerBase<tButServerOcTree::NodeType>, public CDataHolderBase< tPointCloud >
    {
    public:
    	//! Incomming pointcloud type
    	typedef sensor_msgs::PointCloud2 tIncommingPointCloud;

    public:
        /// Constructor
        CPointCloudPlugin(const std::string & name, bool subscribe = true );

        /// Destructor
        virtual ~CPointCloudPlugin();

        //! Enable or disable publishing
        void enable( bool enabled ){ m_publishPointCloud = enabled; }

        //! Should plugin publish data?
        bool shouldPublish();

        //! Initialize plugin - called in server constructor
        virtual void init(ros::NodeHandle & node_handle);

        //! Initialize plugin - called in server constructor, enable or disable subscription.
        virtual void init(ros::NodeHandle & node_handle, bool subscribe){ m_bSubscribe = subscribe; init(node_handle); }

        //! Called when new scan was inserted and now all can be published
        virtual void onPublish(const ros::Time & timestamp);

        //! Set used octomap frame id and timestamp
        virtual void onFrameStart( const SMapParameters & par );

        /// hook that is called when traversing occupied nodes of the updated Octree (does nothing here)
        virtual void handleOccupiedNode(tButServerOcTree::iterator& it, const SMapParameters & mp);

        /// Called when all nodes was visited.
        virtual void handlePostNodeTraversal(const SMapParameters & mp);

    protected:


        /**
        * @brief Insert point cloud callback
        *
        * @param cloud Input point cloud
        */
        void insertCloudCallback(const tIncommingPointCloud::ConstPtr& cloud);

        /**
         * Test if incomming pointcloud2 has rgb part
         */
        bool isRGBCloud( const tIncommingPointCloud::ConstPtr& cloud );

    protected:
        //! Is publishing enabled?
        bool m_publishPointCloud;

        //! Point cloud publisher name
        std::string m_pcPublisherName;

        //! Point cloud subscriber name
        std::string m_pcSubscriberName;

        /// Subscriber - point cloud
        message_filters::Subscriber<tIncommingPointCloud> *m_pcSubscriber;

        //! Message filter (we only want point cloud 2 messages)
        tf::MessageFilter<tIncommingPointCloud> *m_tfPointCloudSub;

        /// Point cloud publisher
        ros::Publisher m_pcPublisher;

        //! Should this plugin subscribe to some publishing topic?
        bool m_bSubscribe;


        //! Transform listener
        tf::TransformListener m_tfListener;

        //
        bool m_latchedTopics;

        //! Used frame id (point cloud will be transformed to it)
        std::string m_pcFrameId;

        /// Crawled octomap frame id
        std::string m_ocFrameId;

        //! Do pointcloud filtering?
        bool m_bFilterPC;

        //! Minimal Z value
        double m_pointcloudMinZ;

        //! Maximal Z value
        double m_pointcloudMaxZ;

        //! Counter
        long counter;

        //! Has input cloud rgb data?
        bool m_bUseRGB;

        //! Input type was set by parameter
        bool m_bRGB_byParameter;

        //! Pointcloud working mode
        bool m_bAsInput;

    }; // class CPointCloudPlugin

    /// Declare holder object - partial specialization of the default holder with predefined connection settings
    template< class tpOctomapPlugin >
    struct SPointCloudPluginHolder : public  CCrawlingPluginHolder< CPointCloudPlugin, tpOctomapPlugin >
    {
    protected:
        /// Define holder type
        typedef CCrawlingPluginHolder< CPointCloudPlugin, tpOctomapPlugin > tHolder;

    public:
        /// Create holder
        SPointCloudPluginHolder( const std::string & name )
        : tHolder(  name,  tHolder::ON_START | tHolder::ON_OCCUPIED | tHolder::ON_STOP)
        {

        }

    }; // struct SPointCloudPluginHolder

} // namespace srs_env_model


// PointCloudPubPlugin_H_included
#endif


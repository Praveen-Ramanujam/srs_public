

#include "ros/ros.h"

// services
#include <srs_body_detector/getBodyDetections.h>


class BodyDetector {
 
 public:
    ros::NodeHandle nh_;
    // services
        ros::ServiceServer  service_server_detect_bodies_;

//constructor
    BodyDetector(ros::NodeHandle nh) :
      nh_(nh)
     {

      //services
      service_server_detect_bodies_ = nh_.advertiseService("detect_bodies", &BodyDetector::detectBodiesCallback, this);


     }

 


// calback for the DetectLegs service        
        bool detectBodiesCallback(srs_body_detector::getBodyDetections::Request &req, srs_body_detector::getBodyDetections::Response &res)
        {
        

         res.bodies_list.header.stamp = ros::Time::now();


         geometry_msgs::Point32 pt1,pt2,pt3,pt4;

         pt1.x=2.0; pt1.y=2.0;
         pt2.x=-2.0; pt2.y=2.0;
         pt3.x=2.0; pt3.y=-2.0;
         pt4.x=-2.0; pt4.y=-2.0;
          
         //res.leg_list.points = detected_legs;
         res.bodies_list.points.push_back(pt1);	
         res.bodies_list.points.push_back(pt2);
         res.bodies_list.points.push_back(pt3);
         res.bodies_list.points.push_back(pt4);
    
         return true;
        }


};



int main(int argc, char **argv)
{
  ros::init(argc, argv, "srs_body_detector");  
  ros::NodeHandle nh;

  BodyDetector bd(nh);

  ros::spin();

  return 0;
}

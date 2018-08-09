#include <ros/ros.h>
#include <iostream>
#include <stdlib.h>
#include <math.h> 
#include "image_segmentation_node/ImageSet.h"

#include <image_transport/image_transport.h>
#include <opencv2/highgui/highgui.hpp>
#include <cv_bridge/cv_bridge.h>

#include <sensor_msgs/LaserScan.h>

#include <sensor_msgs/point_cloud_conversion.h>
#include <pcl/point_types.h>
#include <boost/foreach.hpp>
#include <pcl/filters/passthrough.h>
#include <pcl/point_cloud.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/common/centroid.h>

#include <pointcloud_msgs/PointCloud2_Segments.h>
#include <image_msgs/Image_Segments.h>

ros::Publisher pub;
image_transport::Publisher tpub;


std::vector<std::pair<double,double>> pair_vector;
sensor_msgs::Image latest_frame;

int got_message = 0;
const double PI = 3.141592653589793;

std::pair<double,double> angle_calculation(double angle_l, double angle_r){
	//conversion to center-based angles			!A,B,C,D are the quadrants, starting from upper-left quadrant and moving clockwise.
		double c_angle_l, c_angle_r;
		if(angle_l > PI/2 && angle_l < PI){			//A
			c_angle_l= - ( angle_l - PI/2 );
		}
		else if(angle_l > -PI/2 && angle_l < PI/2){	//B,C
			c_angle_l= PI/2 - angle_l;
		}
		else if(angle_l > -PI && angle_l < -PI/2){	//D
			c_angle_l= -( 3*PI/2 + angle_l );
		}
		else if(angle_l == PI/2){			
			c_angle_l= 0;
		}
		else if(angle_l == PI || angle_l == -PI){			
			c_angle_l= -PI/2;
		}
		else if(angle_l == -PI/2){			
			c_angle_l= PI;
		}


		if(angle_r > PI/2 && angle_r < PI){			//A
			c_angle_r= - ( angle_r - PI/2 );
		}
		else if(angle_r > -PI/2 && angle_r < PI/2){	//B,C
			c_angle_r= PI/2 - angle_r;
		}
		else if(angle_r > -PI && angle_r < -PI/2){	//D
			c_angle_r= -( 3*PI/2 +angle_r );
		}
		else if(angle_r == PI/2){			
			c_angle_r= 0;
		}
		else if(angle_r == PI || angle_r == -PI){			
			c_angle_r= -PI/2;
		}
		else if(angle_r == -PI/2){			
			c_angle_r= PI;
		}
		std::pair<double,double> pair(c_angle_l, c_angle_r);
		return pair;

}

// typedef pcl::PointCloud<pcl::PointXYZ> PointCloud;

// void pcl_Callback(const PointCloud::ConstPtr& msg)
// {
//   printf ("Cloud: width = %d, height = %d\n", msg->width, msg->height);
//   BOOST_FOREACH (const pcl::PointXYZ& pt, msg->points)
//     printf ("\t(%f, %f, %f)\n", pt.x, pt.y, pt.z);
// }



void pcl_seg_Callback(const pointcloud_msgs::PointCloud2_Segments& msg){
	//pointcloud_msgs::PointCloud2_Segments msg_out;

	// while(got_message == 1){
	// 	1;
	// 	std::cout << "In seg loop..." << std::endl;
	// }
	pair_vector.clear();
	//std::vector<std::pair<double,double>> msg_vector;
	double angle_min = msg.angle_min;
	double angle_max = msg.angle_max;
	double angle_increment = msg.angle_increment;   

    // Eigen::Vector4f cluster_centroid;
    // pcl::compute3DCentroid ( pc , cluster_centroid);	// (x,y,z,1)
    // //cluster_centroid_vec.push_back( cluster_centroid );

    // std::cout << "Centroid of first cluster is:\nx= " << cluster_centroid(0) << "\ny= " << cluster_centroid(1) << "\nz= " << cluster_centroid(2) << "\nlast= " << cluster_centroid(3) << "\n\n\n\n";

	// //pcl1 test
	// Eigen::Vector4f cluster_centroid;
	// sensor_msgs::PointCloud conv_msg;
	// convertPointCloud2ToPointCloud(msg.clusters[0], conv_msg);

 //    Eigen::Vector4f cluster_centroid;
	// convertPointCloud2ToPointCloud( msg.clusters[0], conv_msg);
 //    pcl::compute3DCentroid( cloud2 , cluster_centroid);

	// while(i < msg.cluster_id.size()){
	// 	std::cout << "i= " << i << ": " << msg.cluster_id[i] << std::endl << std::endl;
	// 	i++;
	// }

	//NEW WAY SEG**************************************

	cv_bridge::CvImagePtr cv_ptr;
	cv_ptr = cv_bridge::toCvCopy(latest_frame, "bgr8");
	cv::imshow("view",cv_ptr->image);
    cv::waitKey(30);

	for (int j=0; j < msg.clusters.size(); j++){		//for every cluster
		double angle_l, angle_r, c_angle_l, c_angle_r;
		std::pair<double,double> angle_pair(0,0);

		pcl::PCLPointCloud2 pc2;
    	pcl_conversions::toPCL ( msg.clusters[j] , pc2 );	//from sensor_msgs::pointcloud2 to pcl::pointcloud2

    	pcl::PointCloud<pcl::PointXYZ> pc;
    	pcl::fromPCLPointCloud2 ( pc2 , pc );	//from pcl::pointcloud2 to pcl::pointcloud
    	//pc= clusters[j] in pointcloud format

    	pcl::PointXYZ min_point(pc.points[0]);
    	pcl::PointXYZ max_point(pc.points[0]);

    	std::cout << "\n\n\nSTART*****\noriginal min,max y: " << min_point.y << std::endl;

		for (int i=1; i < pc.points.size(); i++){		//for every point in the cluster	
			//std::cout << "x= " << pc.points[i].x << std::endl << "y= " << pc.points[i].y << std::endl << "z= " << pc.points[i].z << "\n\n\n";
			if(pc.points[i].y < min_point.y){
				min_point.x= pc.points[i].x;
				min_point.y= pc.points[i].y;
				min_point.z= pc.points[i].z;
			}
		 	if(pc.points[i].y > max_point.y){
				max_point.x= pc.points[i].x;
				max_point.y= pc.points[i].y;
				max_point.z= pc.points[i].z;
			}
		}
		// std::cout << "MAX y= " << max_point.y << std::endl << "x= " << max_point.x << std::endl << "z= " << max_point.z << std::endl << std::endl;
		// std::cout << "MIN y= " << min_point.y << std::endl << "x= " << min_point.x << std::endl << "z= " << min_point.z << std::endl << std::endl;

		std::cout << "Min y is: " << min_point.y << "\nMax y is: " << max_point.y << std::endl;

		//angle calculation in rads [0,2pi] starting from upper-left quadrant. 		! x,y are reversed because of laserscan's reversed x,y axes
		angle_l= atan2(min_point.x, min_point.y);
		angle_r= atan2(max_point.x, max_point.y);

		std::cout << "Not centered angle_l is: " << angle_l << "\nNot centered angle_r is: " << angle_r << std::endl;

		angle_pair= angle_calculation(angle_l, angle_r);	//conversion to center-based angles

		std::cout << "Centered angle_l is: " << angle_pair.first << "\nCentered angle_r is: " << angle_pair.second << std::endl;

		//msg_vector.push_back( angle_calculation(angle_l, angle_r) );
		pair_vector.push_back( angle_calculation(angle_l, angle_r) );	

		//TRYING IT INSIDE THE FOR LOOP
		double a_l= pair_vector.at(j).first;	//first: center-based left angle, second: center_based right angle
		double a_r= pair_vector.at(j).second;
		std::cout << "ANGLES:\nLEFT: " << a_l << "\nRIGHT: " << a_r << std::endl;

		double cam_min= -PI/6, cam_max= PI/6;	// Orbbec Astra Pro wideness: PI/3 (60 degrees) total
		std::cout << "Image.cols (width)= " << cv_ptr->image.cols << "\nabs(cam_min)= " << abs(cam_min) << "\nabs(cam_max)= " << abs(cam_max) << std::endl;
		int ratio= (cv_ptr->image.cols) / (abs(cam_min)+abs(cam_max)) ;	//	(width pixels) / (wideness)
		std::cout << "Ratio:\t" << ratio << std::endl;
		int center = (cv_ptr->image.cols)/2;
		int x_l, x_r;

		// a_r not necessarily greater than a_l
		if( a_l < a_r ){
			if( (a_l < cam_min && a_r < cam_min) || (a_l > cam_max && a_r > cam_max) ){	//out of range
				std::cout << "out of range!" << std::endl;
				continue;
			}
			else if( a_l < cam_min && a_r > cam_max ){	//bigger than image size
				std::cout << "bigger than image size!" << std::endl;
				x_l= -center;
				x_r= center;
			}
			else if( a_l < cam_min && a_r > cam_min && a_r < cam_max ){	//left side out of range
				std::cout << "left side out of range!" << std::endl;
				x_l= -center;
				x_r= a_r*ratio;	//x_r, x_l: pixel distance from center of image (can be either positive or negative)
			}
			else if( a_r > cam_max && a_l > cam_min && a_l < cam_max ){	//right side out of range
				std::cout << "right side out of range!" << std::endl;
				x_r= center;
				x_l= a_l*ratio;
			}
			else if( a_l > cam_min && a_l < cam_max && a_r > cam_min && a_r < cam_max ){	//in range
				std::cout << "in range!" << std::endl;
				x_l= a_l*ratio;
				x_r= a_r*ratio;
			}
			else{
				std::cout << "??????" << std::endl;
				continue;
			}
		}
		else{
			std::cout << "angle mistake" << std::endl;
			continue;
		}

		int width_pixels= x_r - x_l;
		int offset= center + x_l;
		std::cout << "\n\nx_l= " << x_l << "\nx_r= " << x_r << "\nwidth_pixels= " << width_pixels << "\noffset= " << offset << "\n\n\n\n";
		cv::Rect myROIseg(offset, 0, width_pixels, cv_ptr->image.rows); 
	    cv::Mat roiseg = cv::Mat(cv_ptr->image,myROIseg);
		cv::imshow("cluster1",roiseg);
	    cv::waitKey(30);	

		//std::cout << "min: " << angle_l*180/PI << std::endl;
		//std::cout << "center based min: " << angle_pair.first*180/PI << std::endl;
		//std::cout << "center based min in msg_vector: " << msg_vector.at(j).first*180/PI << std::endl << std::endl;
		//std::cout << "center based min in global vector: " << pair_vector.at(j).first*180/PI << std::endl << std::endl;

		//std::cout << "max: " << angle_r*180/PI << std::endl;
		//std::cout << "center based max in msg_vector: " << msg_vector.at(j).second*180/PI << std::endl;

		//std::cout << "New Angle Pair!!!" << std::endl;

		//std::cout << "center based max: " << angle_pair.second*180/PI << std::endl;
		//std::cout << "center based max in global vector: " << pair_vector.at(j).second*180/PI << std::endl<< std::endl<< std::endl<< std::endl;
		
	}

	// std::cout << "HEADER INFO BELOW:\n\n" << msg.header<< std::endl;
	// std::cout << "FIRST STAMP:\n\n" << msg.first_stamp << std::endl;

	//std::cout << "DATA OF FIRST CLUSTER: "<< std::endl << std::endl << msg.clusters[0] << std::endl << std::endl;
	//std::cout << "DATA OF SECOND CLUSTER: "<< std::endl << std::endl << msg.clusters[1] << std::endl << std::endl;

	
	//sensor_msgs::PointCloud conv_msg;
	//convertPointCloud2ToPointCloud(msg.clusters[0], conv_msg);

	// int i=0;
	// while(i< msg.clusters.size()){
	// 	std::cout << conv_msg.points[i] << std::endl;
	// 	i++;
	// }

}


// void imageCallback(const sensor_msgs::ImageConstPtr& msg)
// {
//   std_msgs::Header h = msg->header;
//   try
//   {
//     if(h.seq%3==0){
//       cv::imshow("view", cv_bridge::toCvShare(msg, "bgr8")->image);
//       cv::waitKey(30);
//     }
//     else if(h.seq%3==1){
//       cv::imshow("view2", cv_bridge::toCvShare(msg, "bgr8")->image);
//       cv::waitKey(30);
//     }
//     else{
//       cv::imshow("view3", cv_bridge::toCvShare(msg, "bgr8")->image);
//       cv::waitKey(30);
//     }
    
//   }
//   catch (cv_bridge::Exception& e)
//   {
//     ROS_ERROR("Could not convert from '%s' to 'bgr8'.", msg->encoding.c_str());
//   }
// }


void videoCallback(const sensor_msgs::ImageConstPtr& msg){
	// while( got_message == 0 ){
	// 	1;
	// 	//std::cout << "In video loop..." << std::endl;
	// }
	std_msgs::Header h = msg->header;
	sensor_msgs::Image new_msg;


	//new way...
	latest_frame= *msg;


  try
  {
    //cv::imshow("view", cv_bridge::toCvShare(msg, "bgr8")->image);
    //cv::waitKey(30);
    cv_bridge::CvImagePtr cv_ptr;

    //std::cout << "New Video Frame!" << std::endl;

    try{
      cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
      // cv::imshow("view",cv_ptr->image);
      // cv::waitKey(30);
    }
    catch (cv_bridge::Exception& e){
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }

    int center = (cv_ptr->image.cols)/2;

    //cut a rectangle
    //cv::Rect myROI(0, 0, (cv_ptr->image.cols)/3, cv_ptr->image.rows); 
    //cv::Mat roi = cv::Mat(cv_ptr->image,myROI);

    cv::Rect myROI2(cv_ptr->image.cols/3, 0, cv_ptr->image.cols/3,cv_ptr->image.rows); 
    cv::Mat roi2 = cv::Mat(cv_ptr->image,myROI2);

    cv::Rect myROI3(2*(cv_ptr->image.cols/3),0,cv_ptr->image.cols/3,cv_ptr->image.rows);
    cv::Mat roi3= cv::Mat(cv_ptr->image,myROI3);


    cv::Rect myROI4(0, 0, 150, 150); 
    cv::Mat roi4 = cv::Mat(cv_ptr->image,myROI4);
    // cv::imshow("view5", roi4);
    // cv::waitKey(30);

    image_segmentation_node::ImageSet set;

    cv::Rect myROI(0, 0, (cv_ptr->image.cols)/3, cv_ptr->image.rows); 
    cv::Mat roi = cv::Mat(cv_ptr->image,myROI);
    sensor_msgs::ImagePtr msg1 = cv_bridge::CvImage(std_msgs::Header(), "bgr8", roi).toImageMsg();
    set.data[0]= *msg1;

 //    for (int i=0; i < pair_vector.size(); i++){
    	
 //    	// cv::Rect myROI(0, 0, 50, 50); 
 //    	// cv::Mat roi = cv::Mat(cv_ptr->image,myROI);

	// 	//convert to sensor_msgs/Image
 //    	sensor_msgs::ImagePtr msg1 = cv_bridge::CvImage(std_msgs::Header(), "bgr8", roi).toImageMsg();
 //    	set.data[i]= *msg1;

 //    	//...
 //    	// msg1 = cv_bridge::CvImage(std_msgs::Header(), "bgr8", roi2).toImageMsg();
 //    	// set.data[1]= *msg1;
 //    	// msg1 = cv_bridge::CvImage(std_msgs::Header(), "bgr8", roi3).toImageMsg();
 //    	// set.data[2]= *msg1;
 //    	//new_msg=set.data[0];

	// }

    // cv::imshow("view2", roi);
    // cv::waitKey(30);

    // cv::imshow("view3", roi2);
    // cv::waitKey(30);

    // cv::imshow("view4", roi3);
    // cv::waitKey(30);

    //tpub.publish(msg1);

    //tpub.publish(new_msg);

    //pub.publish(set);	//PUBLISH
    got_message = 0;
    
  }
  catch (cv_bridge::Exception& e)
  {
    ROS_ERROR("Could not convert from '%s' to 'bgr8'.", msg->encoding.c_str());
  }


}

void laserCallback(const sensor_msgs::LaserScan::ConstPtr& msg){
	// ROS_INFO("angle_min is: %f\n",msg->angle_min);
	// ROS_INFO("angle_max is: %f\n",msg->angle_max);
	// ROS_INFO("angle_increment is: %f\n",msg->angle_increment);
	// ROS_INFO("time_increment is: %f\n",msg->time_increment);
	// ROS_INFO("scan_time is: %f\n",msg->scan_time);
	// ROS_INFO("range_min is: %f\n",msg->range_min);
	// ROS_INFO("range_max is: %f\n",msg->range_max);
	// unsigned long int i=0;
	// ROS_INFO("Size of ranges[] = %lu",msg->ranges.size()); //size=360
	// std::cout << "[" << std::endl;
	// while(i<msg->ranges.size()){
	// 	std::cout << i << ": " << msg->ranges[i]<< " ";
	// 	i++;
	// }
	// std::cout << "]" << std::endl;
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "image_segmentation_node");
  ros::NodeHandle nh;

  //ROS_INFO("angle_max is 2.345\n");

  cv::namedWindow("view");
  std::cout << "New Window: view" << std::endl;
  // cv::namedWindow("view2");
  // cv::namedWindow("view3");
  // cv::namedWindow("view4");
  //cv::namedWindow("view5");
  cv::namedWindow("cluster1");
  //std::cout << "New Window: view5" << std::endl;
  image_transport::ImageTransport it(nh);

  //advertise to output topic
  //image_transport::Publisher pub = it.advertise("seg_images", 2);
  pub = nh.advertise<image_segmentation_node::ImageSet>("seg_images", 2);
  tpub = it.advertise("normal_image", 2);

  // image_transport::Subscriber image_sub = it.subscribe("camera/image", 50, imageCallback);
  // ros::Subscriber pcl_sub = nh.subscribe<PointCloud>("points2", 1, pcl_Callback);

  std::cout << "reached subscribers point" << std::endl;
  ros::Subscriber pcl_seg_sub = nh.subscribe<const pointcloud_msgs::PointCloud2_Segments&>("pointcloud2_cluster_tracking/clusters", 1, pcl_seg_Callback);
  image_transport::Subscriber video_sub = it.subscribe("camera/rgb/image_raw", 50, videoCallback);		//  usb_cam/image_raw gia to rosbag me to video mono.
  ros::Subscriber laser_sub = nh.subscribe("scan",50, laserCallback);

  ros::Rate loop_rate(0.5);
  while (nh.ok()){
    // pub.publish(msg);
    // pub.publish(msg2);
    // pub.publish(msg3);
    ros::spinOnce();
    loop_rate.sleep();
  }
  cv::destroyWindow("view");
  // cv::destroyWindow("view2");
  // cv::destroyWindow("view3");
  // cv::destroyWindow("view4");
  //cv::destroyWindow("view5");
  cv::namedWindow("cluster1");
}
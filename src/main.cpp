#include <ros/ros.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl/io/auto_io.h>
#include <pcl/compression/octree_pointcloud_compression.h>
#include <fstream>
#include <iostream>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/io/png_io.h>
#include <pcl/range_image/range_image.h>
#include <pcl/range_image/range_image_spherical.h>
#include <boost/thread/thread.hpp>
#include <pcl/visualization/common/float_image_utils.h>
#include <pcl/compression/libpng_wrapper.h>
#include <pcl/compression/organized_pointcloud_compression.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <stdio.h>
#include <chrono>


#include "prediction.h"
#include "packager.h"
#include "top_encoding.h"

using namespace cv;
using namespace std;
using namespace std::chrono;

Packager packager("/home/portilha/catkin_ws/src/h264/output_bitstream/out.h264");

ofstream rimage_file("txt/rimage_time.txt", ios::out);
ofstream png_file("txt/png_time.txt", ios::out);
ofstream mb_file("txt/mb_time.txt", ios::out);
ofstream pred_file("txt/pred_time.txt", ios::out);
ofstream transf_file("txt/trf_time.txt", ios::app);
ofstream code_file("txt/code_time.txt", ios::out);
ofstream pack_file("txt/pack_time.txt", ios::out);

void receiver_cb(const sensor_msgs::PointCloud2ConstPtr& input)
{
    static int counter=0;
    static int encode_flag=0;
    char file_name[70];
    
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromROSMsg(*input, *cloud);    // We now want to create a range image from the above point cloud, with a 1deg angular resolution
    
    float angularResolution_x = (float) (0.2f * (M_PI/180.0f));  //   1.0 degree in radians
    float angularResolution_y = (float) (0.2f* (M_PI/180.0f));  //   1.0 degree in radians
    float maxAngleWidth     = (float) (360.0f * (M_PI/180.0f));  // 360.0 degree in radians
    float maxAngleHeight    = (float) (26.8f * (M_PI/180.0f));  // 180.0 degree in radians
    
    Eigen::Affine3f sensorPose = (Eigen::Affine3f)Eigen::Translation3f(0.0f, 0.0f, 0.0f);
    pcl::RangeImage::CoordinateFrame coordinate_frame = pcl::RangeImage::LASER_FRAME;
    
    float noiseLevel=0.00;
    float minRange = 0.0f;
    int borderSize = 0;    
      
    auto start_0 = high_resolution_clock::now();
    pcl::RangeImage rangeImage;

    rangeImage.createFromPointCloud(*cloud, angularResolution_x, angularResolution_y, maxAngleWidth, maxAngleHeight,
                                     sensorPose, coordinate_frame, noiseLevel, minRange, borderSize);
    
    auto stop_0 = high_resolution_clock::now();
    auto duration_0 = duration_cast<microseconds>(stop_0 - start_0);
    rimage_file << duration_0.count() << endl;                           
    
    //std::cout << rangeImage << "\n";
    
    //std::cout << rangeImage.size() * (sizeof(int) + 3 * ) << "\n";    
    
    
    auto start_1 = high_resolution_clock::now();
    float* ranges = rangeImage.getRangesArray(); 
    unsigned char* rgb_image = pcl::visualization::FloatImageUtils::getVisualImage (ranges, rangeImage.width, rangeImage.height);    
    snprintf (file_name, sizeof file_name, "./images/rosbag_%d.png", counter);
    pcl::io::saveRgbPNGFile(file_name, rgb_image, rangeImage.width, rangeImage.height);
    auto stop_1 = high_resolution_clock::now();
    auto duration_1 = duration_cast<microseconds>(stop_1 - start_1);
    png_file << duration_1.count() << endl;
    
         
    /*
        The output YUV image has ONE channel and a number of rows equivalent to 
            3/2 * number of rows of the input RGB image (nrows) and has the same number of columns (ncols)
        The first 2/2*nrows rows represent the luma component
        The last 1/2*nrows rows represent the chroma component, which are interlaced, divided this way:
            The U component is in the first 1/6*nrows (the first half of the last 1/2 rows) rows, being the 
            first half of the rows the even rows, and the last half of the rows the odd rows.
            The last 1/6*nrows rows represent the V component, and are divided the same way.
    */

    // The default setting with cv::imread will create a CV_8UC3 matrix
    // 8-bit 3-channel color image
    //Mat image =  imread(argv[1]), paddedImage, yuv;
    
    auto start_2 = high_resolution_clock::now(); 
    Mat image =  imread(file_name), paddedImage, yuv;

    // Padding width and height to multiple of 16
    int hPad = image.cols % 16;
    int vPad = image.rows % 16;
    if(hPad || vPad)
    {
        copyMakeBorder(image, paddedImage, 0, (16-vPad) & 0x0F, 0 , (16-hPad) & 0x0F, BORDER_REPLICATE);
        //imshow("Padded RGB image", paddedImage);
        //waitKey(0);
        cvtColor(paddedImage, yuv, COLOR_BGR2YUV_I420);     // convert image
    }
    else
        // opencv internally stores channels as BGR
        cvtColor(image, yuv, COLOR_BGR2YUV_I420);       // convert image

    //imshow("YUV image", yuv);
    //waitKey(0); // Wait for a keystroke in the window
    
    // cout << "RGB image:\nChannels: " << image.channels() << endl << "Rows x Cols: " 
    //     << image.rows << " x " << image.cols << endl << endl;
    // cout << "Padded RGB image:\nChannels: " << paddedImage.channels() << endl << "Rows x Cols: " 
    //     << paddedImage.rows << " x " << paddedImage.cols << endl << endl;
    // cout << "YUV image:\nChannels: " << yuv.channels() << endl << "Rows x Cols: " 
    // << yuv.rows << " x " << yuv.cols << endl;

    Frame yuvFrame(yuv);
    auto stop_2 = high_resolution_clock::now();
    auto duration_2 = duration_cast<microseconds>(stop_2 - start_2);
    mb_file << duration_2.count() << endl;

    if(!encode_flag)
    {
        packager.write_SPS(yuvFrame.width, yuvFrame.width, 76);  // 1 frame for testing
        packager.write_PPS();   // 1 PPS for the whole slice
        encode_flag=1;
        printf("SPS and PPS done\n");
    }
   
    transf_file << "Start frame " << frame_cnt << endl;
    auto start_3 = high_resolution_clock::now(); 
    encode_I_frame(yuvFrame);
    auto stop_3 = high_resolution_clock::now();
    auto duration_3 = duration_cast<microseconds>(stop_3 - start_3);
    pred_file << duration_3.count() << endl;

    printf("Prediction and Transform %d\n",counter);

    auto start_4 = high_resolution_clock::now(); 
    vlc_frame(yuvFrame);
    auto stop_4 = high_resolution_clock::now();
    auto duration_4 = duration_cast<microseconds>(stop_4 - start_4);
    code_file << duration_4.count() << endl;
    transf_file << "End frame " <<  frame_cnt << endl;

    
    printf("Entropy coding %d\n",counter);

    auto start_5 = high_resolution_clock::now(); 
    packager.write_slice(counter, yuvFrame);
    auto stop_5 = high_resolution_clock::now();
    auto duration_5 = duration_cast<microseconds>(stop_5 - start_5);
    pack_file << duration_5.count() << endl;    

    printf("Packing %d\n",counter);

    counter++;
}

int main (int argc, char** argv)
{
  
//   remove("txt/16x16_Y_predictors.txt");
//   remove("txt/4x4_Y_predictors.txt");
//   remove("txt/16x16_Y_pred_mode.txt");
//   remove("txt/4x4_Y_pred_mode.txt"); 
//   remove("txt/8x8_Cb_predictors.txt");
//   remove("txt/8x8_Cr_predictors.txt");
//   remove("txt/8x8_CbCr_pred_mode.txt");
//   remove("txt/4x4_Y_residual.txt");
//   remove("txt/16x16_Y_residual.txt");
//   remove("txt/8x8_CbCr_residual.txt");

  // Initialize ROS
  ros::init (argc, argv, "image_process_node");
  ros::NodeHandle nh;

  // Create a ROS subscriber for the input point cloud
  ros::Subscriber sub = nh.subscribe ("/kitti/velo/pointcloud", 100, receiver_cb);
  //ros::Subscriber sub = nh.subscribe ("/autonomoose/velo/pointcloud", 1, receiver_cb);

  // Spin
  ros::spin ();
}

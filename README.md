# Project2_H264
Software Implementation of a LiDAR Point Cloud Data Compression System based on the H.264/AVC Standard developed for the DIGILENT<sup>®</sup> ZYBO Z7-10.

The aplication is integrated within a ROS environment. **pointcloud_h264** is the name of the catkin package created for this application, and **pointcloud_h264_node** the compression node.

When executed, this node subscribes to the **/kitti/velo/pointcloud** ROS topic, where the *PointCloud2* messages must be published.

**ZYBO_Z7-10** folder contains all files required to boot a compatible Linux image with ROS and the compression node integrated.

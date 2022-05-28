#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <stdio.h>

#include "prediction.h"
#include "packager.h"
#include "top_encoding.h"

using namespace cv;
using namespace std;

int main(int argc, char* argv[])
{
    remove("txt/16x16_Y_predictors.txt");
    remove("txt/4x4_Y_predictors.txt");
    remove("txt/16x16_Y_pred_mode.txt");
    remove("txt/4x4_Y_pred_mode.txt");
    remove("txt/8x8_Cb_predictors.txt");
    remove("txt/8x8_Cr_predictors.txt");
    remove("txt/8x8_CbCr_pred_mode.txt");
    remove("txt/4x4_Y_residual.txt");
    remove("txt/16x16_Y_residual.txt");
    remove ("txt/8x8_CbCr_residual.txt");

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
    Mat image =  imread(argv[1]), paddedImage, yuv;
    // Mat image =  imread("png_test1.png"), paddedImage, yuv;

    // Padding width and height to multiple of 16
    int hPad = image.cols % 16;
    int vPad = image.rows % 16;
    if(hPad || vPad)
    {
        copyMakeBorder(image, paddedImage, 0, (16-vPad) & 0x0F, 0 , (16-hPad) & 0x0F, BORDER_REPLICATE);
        imshow("Padded RGB image", paddedImage);
        waitKey(0);
        cvtColor(paddedImage, yuv, COLOR_BGR2YUV_I420);     // convert image
    }
    else
        // opencv internally stores channels as BGR
        cvtColor(image, yuv, COLOR_BGR2YUV_I420);       // convert image

    imshow("YUV image", yuv);
    waitKey(0); // Wait for a keystroke in the window
    
    cout << "RGB image:\nChannels: " << image.channels() << endl << "Rows x Cols: " 
        << image.rows << " x " << image.cols << endl << endl;
    cout << "Padded RGB image:\nChannels: " << paddedImage.channels() << endl << "Rows x Cols: " 
        << paddedImage.rows << " x " << paddedImage.cols << endl << endl;
    cout << "YUV image:\nChannels: " << yuv.channels() << endl << "Rows x Cols: " 
    << yuv.rows << " x " << yuv.cols << endl;

    Frame yuvFrame(yuv);
    Packager packager("/home/manuale97/Documents/University/ESRG_2nd/Projects/PI/H264_Simple_encoder/output_bitstream/out.h264");

    packager.write_SPS(yuvFrame.width, yuvFrame.width, 20);  // 1 frame for testing
    packager.write_PPS();   // 1 PPS for the whole slice

    encode_I_frame(yuvFrame);
    vlc_frame(yuvFrame);

    for(int i = 0; i < 20; i++)
        packager.write_slice(i, yuvFrame);

/*=================================== CAVLC TESTING =================================*/
    // int test[16] = {0,3,-1,0,0,-1,1,0,1,0,0,0,0,0,0,0};
    // int test[16] = {-2,4,0,-1,3,0,0,0,-3,0,0,0,0,0,0,0};
    // int test[16] = {0,0,1,0,0,0,0,0,1,0,0,0,-1,0,0,0};
    // Block4x4 test_block(test[0], test[1], test[2], test[3], test[4], test[5], test[6], test[7], test[8],
    //                     test[9], test[10], test[11], test[12], test[13], test[14], test[15]);
    // cavlc_block4x4(test_block, 0, 16);
/*===================================================================================*/

    return 0;
}
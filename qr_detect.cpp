#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include "zbar.h"
#include "zbar/Image.h"
#include "zbar/Decoder.h"
#include "opencv2/core/core.hpp"
#include <iostream>

using namespace cv;
using namespace std;
using namespace zbar;

typedef struct
{
  string type;
  string data;
  vector <Point> location;
} decodedObject;


// Find and decode barcodes and QR codes
void decode(Mat &im, vector<decodedObject>&decodedObjects)
{    
    // Create zbar scanner
    ImageScanner scanner;

    // Configure scanner
    scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
     
    // Convert image to grayscale
    Mat imGray;
    cvtColor(im, imGray, COLOR_BGR2GRAY);

    // Wrap image data in a zbar image
    Image image(im.cols, im.rows, "Y800", (uchar *)imGray.data, im.cols * im.rows);

    // Scan the image for barcodes and QRCodes
    int n = scanner.scan(image);
     
    // Print results
    for(Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol)
    {
        decodedObject obj;
         
        obj.type = symbol->get_type_name();
        obj.data = symbol->get_data();
         
        // Print type and data
        cout << "Type : " << obj.type << endl;
        cout << "Data : " << obj.data << endl << endl;
         
        // Obtain location
        for(int i = 0; i< symbol->get_location_size(); i++)
        {
            obj.location.push_back(Point(symbol->get_location_x(i),symbol->get_location_y(i)));
        }
         
        decodedObjects.push_back(obj);
    }
}


int showImg(string name, Mat img)
{
	namedWindow( name, WINDOW_NORMAL );
	resizeWindow( name, 400, 400);
	imshow(name, img);	
}

void drawLine(Vec2f line, Mat &img, Scalar rgb = CV_RGB(0,0,255))
{
    if(line[1]!=0)
    {
        float m = -1/tan(line[1]);

        float c = line[0]/sin(line[1]);

        cv::line(img, Point(0, c), Point(img.size().width, m*img.size().width+c), rgb);
    }
    else
    {
        cv::line(img, Point(line[0], 0), Point(line[0], img.size().height), rgb);
    }

}

void findBlob(Mat outerBox)
{
	int count=0;
    int max=-1;

    Point maxPt;

    for(int y=0;y<outerBox.size().height;y++)
    {
        uchar *row = outerBox.ptr(y);
        for(int x=0;x<outerBox.size().width;x++)
        {
            if(row[x]>=128)
            {

                 int area = floodFill(outerBox, Point(x,y), CV_RGB(0,0,64));

                 if(area>max)
                 {
                     maxPt = Point(x,y);
                     max = area;
                 }
            }
        }

    }

    floodFill(outerBox, maxPt, CV_RGB(255,255,255));

    for(int y=0;y<outerBox.size().height;y++)
    {
        uchar *row = outerBox.ptr(y);
        for(int x=0;x<outerBox.size().width;x++)
        {
            if(row[x]==64 && x!=maxPt.x && y!=maxPt.y)
            {
                int area = floodFill(outerBox, Point(x,y), CV_RGB(0,0,0));
            }
        }
    }

    vector<Vec2f> lines;
    HoughLines(outerBox, lines, 1, CV_PI/180, 200);

    for(int i=0;i<lines.size();i++)
    {
        float slope = abs(lines[i][1] / lines[i][0]);

        if (slope < 0.001)
            drawLine(lines[i], outerBox, CV_RGB(0,0,128));
    }
}


int main(int argc, char** argv)
{
	Mat sudoku;

	if (argc == 1)
		sudoku = imread("data/test-a/center.jpg", 0);

	else
		sudoku = imread(argv[1], 0);
	
	Mat outerBox = Mat(sudoku.size(), CV_8UC1);
	Mat outerBoxGauss;

	showImg("Original", sudoku);

	// Blur image a bit to make line extraction easier
	GaussianBlur(sudoku, sudoku, Size(11,11), 0);
	// showImg("Blur", sudoku);

	// Perform adaptive threshold with 
	adaptiveThreshold(sudoku, outerBox, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 5, 2);
	adaptiveThreshold(sudoku, outerBoxGauss, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 2);
	showImg("Threshold", outerBox);
	showImg("Threshold GAUSSIAN", outerBoxGauss);

	bitwise_not(outerBox, outerBox);
	// showImg("Bitwise", outerBox);

	Mat kernel = (Mat_<uchar>(3,3) << 0,1,0,1,1,1,0,1,0);
	dilate(outerBox, outerBox, kernel);

	// namedWindow( "OuterBox", WINDOW_NORMAL );
	// resizeWindow( "OuterBox", 400, 400);
	// imshow("OuterBox", outerBox);
	showImg("OuterBox", outerBox);

	findBlob(outerBox);
	showImg("Blob", outerBox);

	// erode(outerBox, outerBox, kernel);
	// showImg("After Erode", outerBox);
	waitKey(0);
	return 0;
}
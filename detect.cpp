#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <stdlib.h>

using namespace cv;
using namespace std;

int showImg(string name, Mat img)
{
	namedWindow( name, WINDOW_NORMAL );
	resizeWindow( name, 400, 400);
	imshow(name, img);	
}

void printImageFeatures( const Mat &imagem )
{
    cout << endl;

    cout << "Numero de linhas : " << imagem.size().height << endl;

    cout << "Numero de colunas : " << imagem.size().width << endl;

    cout << "Numero de canais : " << imagem.channels() << endl;

    cout << "Numero de bytes por pixel : " << imagem.elemSize() << endl;

    cout << endl;
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

int findMainGrid(Mat src)
{
    int largest_area=0;
    int largest_contour_index=0;
    Rect bounding_rect;

    Mat thr;
    // cvtColor( src, thr, COLOR_BGR2GRAY ); //Convert to gray
    threshold( src, thr, 125, 255, THRESH_BINARY ); //Threshold the gray
    showImg( "THE", thr );
    // printImageFeatures(thr);

    vector<vector<Point> > contours; // Vector for storing contours

    findContours( thr, contours, RETR_CCOMP, CHAIN_APPROX_SIMPLE ); // Find the contours in the image
    for( size_t i = 0; i< contours.size(); i++ ) // iterate through each contour.
    {
        double area = contourArea( contours[i] );  //  Find the area of contour

        if( area > largest_area )
        {
            cout <<  "largest: " <<  area << endl;

            largest_area = area;
            largest_contour_index = i;               //Store the index of largest contour
            bounding_rect = boundingRect( contours[i] ); // Find the bounding rectangle for biggest contour
        }
    }

    drawContours( src, contours,largest_contour_index, Scalar( 0, 255, 0 ), 2 ); // Draw the largest contour using previously stored index.

    showImg( "COUNTOUR", src );
    waitKey();
}

int findContainer(Mat src)
{
    Mat gray;
    if (src.channels() == 3)
    {
        cvtColor(src, gray, COLOR_BGR2GRAY);
    }
    else
    {
        gray = src;
    }

    // showImg("Original", sudoku);
    // Show gray image
    showImg("gray", gray);

    // Apply adaptiveThreshold at the bitwise_not of gray, notice the ~ symbol
    Mat bw;
    adaptiveThreshold(~gray, bw, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, -2);
    // Show binary image
    showImg("binary", bw);

    // Create the images that will use to extract the horizontal and vertical lines
    Mat horizontal = bw.clone();
    Mat vertical = bw.clone();

    // Specify size on horizontal axis
    int horizontalsize = horizontal.cols / 30;
    // Create structure element for extracting horizontal lines through morphology operations
    Mat horizontalStructure = getStructuringElement(MORPH_RECT, Size(horizontalsize,1));
    // Apply morphology operations
    erode(horizontal, horizontal, horizontalStructure, Point(-1, -1));
    dilate(horizontal, horizontal, horizontalStructure, Point(-1, -1));

    // Show extracted horizontal lines
    showImg("horizontal", horizontal);

    // Specify size on vertical axis
    int verticalsize = vertical.rows / 30;
    // Create structure element for extracting vertical lines through morphology operations
    Mat verticalStructure = getStructuringElement(MORPH_RECT, Size( 1,verticalsize));

    // Apply morphology operations
    erode(vertical, vertical, verticalStructure, Point(-1, -1));
    dilate(vertical, vertical, verticalStructure, Point(-1, -1));

    // Show extracted vertical lines
    showImg("vertical", vertical);

    Mat s = horizontal + vertical;
    showImg("RESULT", s);

    showImg("BLOB", s);

    // findMainGrid(s);
}


int main(int argc, char** argv)
{
	Mat sudoku;

	if (argc == 1)
		sudoku = imread("data/test-a/center.jpg", 0);

	else
		sudoku = imread(argv[1], 0);
    
    printImageFeatures(sudoku);	
	Mat outerBox = Mat(sudoku.size(), CV_8UC1);
	Mat outerBoxGauss;
    findMainGrid(sudoku);
    // findContainer(sudoku);

	// showImg("Original", sudoku);

	// Blur image a bit to make line extraction easier
	// GaussianBlur(sudoku, sudoku, Size(11,11), 0);
	// showImg("Blur", sudoku);

	// Perform adaptive threshold with 
	// adaptiveThreshold(sudoku, outerBox, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 5, 2);
	// adaptiveThreshold(sudoku, outerBoxGauss, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 2);
	// showImg("Threshold", outerBox);
	// showImg("Threshold GAUSSIAN", outerBoxGauss);

	// bitwise_not(outerBox, outerBox);
	// showImg("Bitwise", outerBox);

	// Mat kernel = (Mat_<uchar>(3,3) << 0,1,0,1,1,1,0,1,0);
	// dilate(outerBox, outerBox, kernel);

	// namedWindow( "OuterBox", WINDOW_NORMAL );
	// resizeWindow( "OuterBox", 400, 400);
	// imshow("OuterBox", outerBox);
	// showImg("OuterBox", outerBox);

	// findBlob(outerBox);
	// showImg("Blob", outerBox);

	// erode(outerBox, outerBox, kernel);
	// showImg("After Erode", outerBox);
	waitKey(0);
	return 0;
}

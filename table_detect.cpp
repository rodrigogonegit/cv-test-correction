#include <iostream>
#include <opencv2/opencv.hpp>
#include <iomanip>
#include <vector>
#include <iterator>
// #include "opencv2/core/core.hpp"

using namespace std;
using namespace cv;

template<typename T>
std::vector<T> slice(std::vector<T> const &v, int m, int n)
{
    auto first = v.cbegin() + m;
    auto last = v.cbegin() + n + 1;
 
    std::vector<T> vec(first, last);
    return vec;
}

template<typename T>
void printVector(const T& t) {
    std::copy(t.cbegin(), t.cend(), std::ostream_iterator<typename T::value_type>(std::cout, ", "));
}

vector<vector<Rect>> sortRectangles(vector<Rect> rects)
{
    vector<vector<Rect>> sorted_cells;

    std::sort(rects.begin(), rects.end(), [](Rect a, Rect b){
        return (a.tl().y < b.tl().y);
    });

    for (int i = 0; i < rects.size(); i += 18)
    {
        vector<Rect> row(rects.begin()+i, rects.begin()+i+18);

        std::sort(row.begin(), row.end(), [](Rect a, Rect b){
            return (a.tl().x < b.tl().x);
        });

        sorted_cells.push_back(row);
    }

    return sorted_cells;
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

vector<int> getSelectedAnswer(vector<Rect> answers, Mat base_img, int question_number)
{
    vector<Mat> img_answers;
    // int current_most_confident_idx = -1;

    // 4 values will hold the value for each possible answer
    // 1 means selected, 0 means empty, -1 means scrubbed
    vector<int> rtn_answers;

    for (int i=1; i < answers.size(); i++)
    {
        Mat cell = base_img(answers[i]).clone();
        img_answers.push_back(cell);

        // Find black points
        vector<Vec2f> lines;
        // imshow("CELL", cell);

        cvtColor( cell, cell, COLOR_BGR2GRAY ); //Convert to gray

        // Find black points
        Mat cell_binary;
        threshold( cell, cell_binary, 180, 255, THRESH_BINARY );

        // imshow("BIN", cell_binary);

        // findNonZero(cell, black_pixels);
        int black_pixels = cell.total() - countNonZero(cell_binary);
        double ratio = (double)black_pixels / (double)cell.total();

        cout.precision(2);
        cout << question_number << "=> " << "Black pixels count: " << black_pixels << "Total: " << cell.total()  << " Ratio: " <<  std::fixed << ratio << endl;

        Canny(cell, cell, 50, 200, 3);

        // blur(~cell, cell, Size(2, 2));
        // GaussianBlur(cell, cell, Size(3,3), 0); 
        // imshow("Cell", cell);
        // waitKey(0);

        HoughLines(cell, lines, 4, CV_PI/300, 80);

        if (ratio > 0.40)
        {
            // cout << "PUSHED -1" << endl;
            rtn_answers.push_back(-1);
        }

        else if (lines.size() > 0)
        {
            rtn_answers.push_back(1);                
        }
        else
        {
            rtn_answers.push_back(0);
        }

        // cout << "Lines: " << lines.size() << endl;

        for(int i=0;i<lines.size();i++)
        {
            float slope = abs(lines[i][1] / lines[i][0]);
            drawLine(lines[i], cell, CV_RGB(255,0,0));
        }

        // imshow("Cell", cell);
    }

    // cout << "Answer: " << current_most_confident_idx << endl;
    // waitKey(0);

    return rtn_answers;
}

void drawRectsQuestion(vector<Rect> cells, vector<int> answers, Mat& base_img)
{
    // cout << "SIZE OF CELLS: " << cells.size() << " --- SIZE OF ANSWERS: " << answers.size() << endl;
    // cout << "ANSWERS VECTOR:" << endl;
    // printVector(answers);
    // cout << endl;
    int already_answered = 0;


    for (int i = 1; i < cells.size(); i++)
    {
        if (answers[i-1] == 1)
        {
            if (already_answered == 1)
            {
                rectangle( base_img, cells[i].tl(), cells[i].br(), Scalar(255, 0, 0), 2, 8, 0 );
            }
            else
            {
                rectangle( base_img, cells[i].tl(), cells[i].br(), Scalar(0, 255, 0), 2, 8, 0 );
                already_answered = 1;
            }
        }
        else if (answers[i-1] == -1)
        {
            rectangle( base_img, cells[i].tl(), cells[i].br(), Scalar(0, 0, 255), 2, 8, 0 );
        }

    }
}

int main(int argc, char** argv)
{
    string filename = "data/j-test/test1.jpg";

    if (argc > 1)    
        filename = argv[1];

    // Load source image
    Mat src = imread(filename);

    // Check if image is loaded fine
    if(!src.data)
        cerr << "Problem loading image!!!" << endl;

    //    // Show source image
    //    imshow("src", src);

    // resizing for practical reasons
    Mat rsz;
    Size size(800, 900);
    resize(src, rsz, size);

    // imshow("rsz", rsz);

    // Transform source image to gray if it is not
    Mat gray;

    if (rsz.channels() == 3)
    {
        cvtColor(rsz, gray, COLOR_BGR2GRAY);
    }
    else
    {
        gray = rsz;
    }

    // Show gray image
    // imshow("gray", gray);

    // Apply adaptiveThreshold at the bitwise_not of gray, notice the ~ symbol
    Mat bw;
    adaptiveThreshold(~gray, bw, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, -2);

    // Show binary image
    // imshow("binary", bw);

    // Create the images that will use to extract the horizonta and vertical lines
    Mat horizontal = bw.clone();
    Mat vertical = bw.clone();

    int scale = 35; // play with this variable in order to increase/decrease the amount of lines to be detected

    // Specify size on horizontal axis
    int horizontalsize = horizontal.cols / scale;

    // Create structure element for extracting horizontal lines through morphology operations
    Mat horizontalStructure = getStructuringElement(MORPH_RECT, Size(horizontalsize,1));

    // Apply morphology operations
    erode(horizontal, horizontal, horizontalStructure, Point(-1, -1));
    dilate(horizontal, horizontal, horizontalStructure, Point(-1, -1));
    //    dilate(horizontal, horizontal, horizontalStructure, Point(-1, -1)); // expand horizontal lines

    // Show extracted horizontal lines
    // imshow("horizontal", horizontal);
    // Specify size on vertical axis
    int verticalsize = vertical.rows / scale;

    // Create structure element for extracting vertical lines through morphology operations
    Mat verticalStructure = getStructuringElement(MORPH_RECT, Size( 1,verticalsize));

    // Apply morphology operations
    erode(vertical, vertical, verticalStructure, Point(-1, -1));
    dilate(vertical, vertical, verticalStructure, Point(-1, -1));
    //dilate(vertical, vertical, verticalStructure, Point(-1, -1)); // expand vertical lines

    // Show extracted vertical lines
    // imshow("vertical", vertical);

    // create a mask which includes the tables
    Mat mask = horizontal + vertical;
    // imshow("mask", mask);

    // find the joints between the lines of the tables, we will use this information in order to descriminate tables from pictures (tables will contain more than 4 joints while a picture only 4 (i.e. at the corners))
    Mat joints;
    bitwise_and(horizontal, vertical, joints);
    // imshow("joints", joints);

    // Find external contours from the mask, which most probably will belong to tables or to images
    vector<Vec4i> hierarchy;
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(mask, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE, Point(0, 0));

    vector<vector<Point> > contours_poly( contours.size() );
    vector<Rect> boundRect;
    vector<Mat> rois;
    int ignored_contours = 0;

    for (size_t i = 0; i < contours.size(); i++)
    {
        approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
        Rect temp = boundingRect( Mat(contours_poly[i]) );

        // Cells must fall within area size of 1600 < x < 2500 [This range should be more precise]
        if(temp.area() < 1600 || temp.area() > 2500)
        {
            ignored_contours++;
            // boundRect.erase(boundRect.begin() + i);
            continue;
        }

        boundRect.push_back(temp);

        // find the number of joints that each table has
        // Mat roi = joints(boundRect[i]);

        // vector<vector<Point> > joints_contours;
        // findContours(roi, joints_contours, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

        // if the number is not more than 5 then most likely it not a table
        // if(joints_contours.size() <= 4)
        //     continue;

        // rois.push_back(rsz(temp).clone());
        // rectangle( rsz, temp.tl(), temp.br(), Scalar(0, 255, 0), 1, 8, 0 );    
    }

    vector<vector<Rect>> res = sortRectangles(boundRect);

    cout << "Total contours: " << contours.size() << endl;
    cout << "Ignored contours (under 2000 area): " << ignored_contours << endl;
    cout << "Number of cells found: " << boundRect.size() << endl;
    cout << "Number of rows: " << res.size() << endl;
    imshow("contours", rsz);

    // Ignore first row
    for(int i = 1; i < res.size(); ++i)
    {
        // Slice vectors to corresponding length in the row
        vector<Rect> question_1 = slice(res[i], 0, 4);
        vector<Rect> question_2 = slice(res[i], 5, 9);
        vector<Rect> question_3 = slice(res[i], 10, 14);
        vector<Rect> question_tf;

        // Process cells from each question
        vector<int> r_1 = getSelectedAnswer(question_1, rsz, i);
        vector<int> r_2 = getSelectedAnswer(question_2, rsz, i + 14);
        vector<int> r_3 = getSelectedAnswer(question_3, rsz, i + 28);
        vector<int> r_tf;

        // T/F questions only present in the first 12 rows (this should be dynamically detected)
        if (i < 13)
        {
            question_tf = slice(res[i], 15, 17);
            r_tf = getSelectedAnswer(question_tf, rsz, i);
        }

        // Print values
        cout << "Question " << i << ": ";
        printVector(r_1);
        cout << endl; 

        drawRectsQuestion(question_1, r_1, rsz);
        drawRectsQuestion(question_2, r_2, rsz);
        drawRectsQuestion(question_3, r_3, rsz);

        cout << "Question " << i + 14 << ": ";
        printVector(r_2);
        cout << endl; 

        cout << "Question " << i + 28<< ": ";
        printVector(r_3);
        cout << endl;

        if (i < 13)
        {
            cout << "Question T/F" << i << ": ";
            printVector(r_tf);
            cout << endl;

            drawRectsQuestion(question_tf, r_tf, rsz);

        }
        imshow("contours", rsz);

        // waitKey(0);
    }

    return 0;
}

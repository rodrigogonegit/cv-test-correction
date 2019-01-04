qrcode:	
	g++ qrcode.cpp -o qrcode -I /usr/include/opencv4/ -L /usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lzbar
main:
	g++ main.cpp -o main -I /usr/include/opencv4/ -L /usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lzbar
sudoku:	
	g++ detect.cpp -o detect -I /usr/include/opencv4/ -L /usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lzbar

qr_detect:
	g++ qr_detect.cpp -o qr_detect -I /usr/include/opencv4/ -L /usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lzbar

detect_table:
	g++ table_detect.cpp -o table_detect -I /usr/include/opencv4/ -L /usr/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc

all: qrcode main sudoku qr_detect detect_table

#include "opencv2/opencv.hpp"
#include <iostream>
#include <ctime>

using namespace cv;
using namespace std;

struct Ball {
    Point position;
    int radius;
    int touchCount;
};

Point getRandomPosition(int w, int h, int r) {
    return Point(rand()%(w-2*r)+r, rand()%(h-2*r)+r);
}

//터치 부분에 이미지 오버레이
void overlayImage(Mat& frame, Mat& img, Point pos)
{
    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {

            int fx = pos.x + x;
            int fy = pos.y + y;

            if (fx < 0 || fy < 0 || fx >= frame.cols || fy >= frame.rows) continue;

            Vec4b p = img.at<Vec4b>(y, x);
            if (p[3] == 0) continue;

            frame.at<Vec3b>(fy, fx) = Vec3b(p[0], p[1], p[2]);
        }
    }
}

void mProject()
{
    srand((unsigned)time(0));
    VideoCapture cap(0);
    if (!cap.isOpened()) return;

    int width = (int)cap.get(CAP_PROP_FRAME_WIDTH);
    int height = (int)cap.get(CAP_PROP_FRAME_HEIGHT);

    Mat img1 = imread("start.png", IMREAD_UNCHANGED);
    Mat img2 = imread("end.png", IMREAD_UNCHANGED);

    if (img1.empty() || img2.empty()) return;

    Ball ball;
    ball.radius = 30;
    ball.position = getRandomPosition(width, height, ball.radius);
    ball.touchCount = 0;

    Mat prevGray;

    while (true)
    {
        Mat frame, gray, diff, thresh;
        cap >> frame;
        if (frame.empty()) break;

        flip(frame, frame, 1);  //거울 이미지로 반전

        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(15, 15), 0);

        if (prevGray.empty()) {
            gray.copyTo(prevGray);
            continue;
        }

        //프레임 차이 잡기
        absdiff(prevGray, gray, diff);
        threshold(diff, thresh, 25, 255, THRESH_BINARY);

        Rect r(ball.position.x - ball.radius,
            ball.position.y - ball.radius,
            ball.radius * 2, ball.radius * 2);
        r &= Rect(0, 0, width, height);

        if (countNonZero(thresh(r)) > r.area() * 0.1) {
            ball.touchCount++;
            ball.position = getRandomPosition(width, height, ball.radius);
        }

        // 이미지 처리
        Mat current;

        if (ball.touchCount <= 5) {
            current = img1.clone(); //5회까진 기본
        }
        else if (ball.touchCount <= 10) {
            flip(img1, current, 0); //10회까지 좌우 반전
        }
        else if (ball.touchCount <= 15) {
            flip(img1, current, 1);
            resize(current, current, Size(150, 150)); //15회까지 크기변환
        }
        else if (ball.touchCount <= 20) {
            flip(img1, current, 1);
            resize(current, current, Size(100, 100));
            current.forEach<Vec4b>([](Vec4b& p, const int*) {
                p[2] = 255; // 빨간색으로 변환
                });
        }
        else if (ball.touchCount <= 24) {
            double alpha = (ball.touchCount - 21) / 3.0; //0% ,33%, 66%, 100%
            resize(img1, img1, Size(60, 60));
            resize(img2, img2, Size(60, 60)); // 크기 맞추기
            addWeighted(img1, 1 - alpha, img2, alpha, 0, current);
        } 
        else {current = img2.clone();}

        //resize(current, current, Size(ball.radius * 2, ball.radius * 2));

        Point topLeft(ball.position.x - ball.radius,
            ball.position.y - ball.radius);

        overlayImage(frame, current, topLeft);

        string stageName;
        Scalar textColor;

        if (ball.touchCount <= 5) {
            stageName = "BASIC";
            textColor = Scalar(255, 255, 255); //흰색
        }
        else if (ball.touchCount <= 10) {
            stageName = "FLIP";
            textColor = Scalar(0, 255, 0); //녹색
        }
        else if (ball.touchCount <= 15) {
            stageName = "SIZE";
            textColor = Scalar(255, 255, 0); //노랑색
        }
        else if (ball.touchCount <= 20) {
            stageName = "COLOR";
            textColor = Scalar(0, 0, 255); //빨강색
        }
        else if (ball.touchCount <= 24) {
            stageName = "MORPHING";
            textColor = Scalar(255, 0, 255); //보라색
        }
        else {
            stageName = "FINAL";
            textColor = Scalar(0, 255, 255); //하늘색
        }

        putText(frame,
            "Touch : " + to_string(ball.touchCount) + " (" + stageName + ")",
            Point(20, 40),
            FONT_HERSHEY_SIMPLEX,
            1.0,
            textColor,
            2);
        imshow("GAME", frame);
        gray.copyTo(prevGray);
        if (waitKey(10) == 27) break;
    }

    cap.release();
    destroyAllWindows();
}

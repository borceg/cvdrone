#include "ardrone/ardrone.h"
#include <opencv2/legacy/legacy.hpp>
#include <opencv2/stitching/stitcher.hpp>

#define KEY_DOWN(key) (GetAsyncKeyState(key) & 0x8000)
#define KEY_PUSH(key) (GetAsyncKeyState(key) & 0x0001)

// --------------------------------------------------------------------------
// main(Number of arguments, Argument values)
// Description  : This is the entry point of the program.
// Return value : SUCCESS:0  ERROR:-1
// --------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // AR.Drone class
    ARDrone ardrone;

    // Initialize
    if (!ardrone.open()) {
        printf("Failed to initialize.\n");
        return -1;
    }

    // Snapshots
    std::vector<cv::Mat> snapshots;

    // Key frame
    IplImage *last = cvCloneImage(ardrone.getImage());

    // ORB detector/descriptor
    cv::OrbFeatureDetector detector;
    cv::OrbDescriptorExtractor extractor;

    // Detect key points
    cv::Mat descriptorsA;
    std::vector<cv::KeyPoint> keypointsA;
    detector.detect(cv::Mat(last), keypointsA);
    extractor.compute(cv::Mat(last), keypointsA, descriptorsA);

    // Main loop
    while (!GetAsyncKeyState(VK_ESCAPE)) {
        // Update
        if (!ardrone.update()) break;

        // Get an image
        IplImage *image = ardrone.getImage();

        // Detect key points
        cv::Mat descriptorsB;
        std::vector<cv::KeyPoint> keypointsB;
        detector.detect(cv::Mat(image), keypointsB);
        extractor.compute(cv::Mat(image), keypointsB, descriptorsB);

        // Match key points
        std::vector<cv::DMatch> matches;
        cv::BruteForceMatcher<cv::Hamming> matcher;
        matcher.match(descriptorsA, descriptorsB, matches);

        // Count matches
        int count = 0;
        for (int i = 0; i < (int)matches.size(); i++) {
            if (matches[i].queryIdx == matches[i].trainIdx) count++; 
        }

        // Take a snapshot when scene changed
        if (count == 0) {
            cvCopy(image, last);
            snapshots.push_back(cv::Mat(image, true));
            detector.detect(cv::Mat(last), keypointsA);
            extractor.compute(cv::Mat(last), keypointsA, descriptorsA);
        }

        // Display the image
        cvShowImage("camera", image);
        cvWaitKey(1);
    }

    // Stiching
    cv::Mat result;
    cv::Stitcher stitchImg = cv::Stitcher::createDefault();
    printf("Stitching images...\n");
    if (stitchImg.stitch(snapshots, result) == cv::Stitcher::OK) {
        cv::imshow("stitching", result);
        cv::imwrite("result.jpg", result);
        cvWaitKey(0);
    }

    // Release the image
    cvReleaseImage(&last);

    // See you
    ardrone.close();

    return 0;
}
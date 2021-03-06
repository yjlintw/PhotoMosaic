//
//  main.cpp
//  PhotoMosaic
//
//  Created by peacedove on 8/1/12.
//  Copyright (c) 2012 peacedove. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sstream>
#include "opencv2/opencv.hpp"

static int TILE_WIDTH = 20;
static int TILE_HEIGHT = 15;
static int OUT_RES_W = 16000;
static int OUT_RES_H = 12000;
bool debugFlag = true;

//  Not used in this project
bool fileExists(std::string filename)
{
    std::ifstream file(filename.c_str());
    return file.is_open();
}

int to_string(const char* value) {
    std::stringstream s(value);
    int result;
    s >> result;
    return result;
}

/*  list all files in the input directory
 *  TODO: 
 *      1. Traverse sub-directory
 *      2. reaname the function as list All files
*/
std::vector<std::string> listAllImageFiles(std::string dirPath)
{
    std::vector<std::string> imgList;
    DIR *dp;
    struct dirent *ep;
    dp = opendir(dirPath.c_str());
    
    if (dp != NULL) {
        while ((ep = readdir(dp))) {
            if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0 || strcmp(ep->d_name, ".DS_Store") == 0) {
                // bypass curent folder, parent folder and MAC index file.
                continue;
            }
            imgList.push_back(dirPath + "/" + ep->d_name);
        }
        (void)closedir(dp);
    } else {
        std::cout << "Couldn't open the directory" << std::endl;
    }
    return imgList;
}



void loadImage(std::vector<cv::Mat>& images, std::vector<cv::Scalar>& colors, std::vector<std::string>imgList)
{
    images.clear();
    colors.clear();
    
    
    for (std::vector<std::string>::iterator it = imgList.begin(); it != imgList.end(); it++) {
        std::string imgName = *it;
        //puts(imgName.c_str());
        
        cv::Mat mat = cv::imread(imgName);
        
        if (mat.data == 0)
            continue;
        
        cv::Scalar color;
        color = cv::mean(mat);
        
        colors.push_back(color);
        images.push_back(mat);
    }
    
}

/*void loadImage(std::vector<cv::Mat>& images, std::vector<cv::Scalar>& colors, std::ifstream& imgListFile, std::ofstream& outFile)
{
    images.clear();
    colors.clear();
    while (!imgListFile.eof()) {
        std::string imgName;
        std::getline(imgListFile, imgName);
        
        cv::Mat mat = cv::imread(imgName);
        
        if (mat.data == 0)
            continue;
        
        cv::Scalar color;
        color = cv::mean(mat);
        
        colors.push_back(color);
        images.push_back(mat);
    }
}*/

void computeTileAvgRGB(cv::Mat mat, cv::Mat& tileMat)
{
    int colNumber = mat.cols / TILE_WIDTH;
    int rowNumber = mat.rows / TILE_HEIGHT;
    
    tileMat = cv::Mat(rowNumber, colNumber, CV_8UC3);
    cv::Mat outTileMat(OUT_RES_W, OUT_RES_H, CV_8UC3);
    for (int j = 0; j < rowNumber; j++) {
        for (int i = 0; i < colNumber; i++) {
            cv::Rect roiRect(i * TILE_WIDTH, j * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT);
            cv::Mat roiMat = mat(roiRect);
            //cv::imshow("roiMat", roiMat);
            //cv::waitKey(0);
            cv::Scalar color = cv::mean(roiMat);
            //std::cout << color[0] << "," << color[1] << "," << color[2] << std::endl;
            
            int blue = color[0];
            int green = color[1];
            int red = color[2];
            
            tileMat.at<cv::Vec3b>(j,i)[0] = blue;
            tileMat.at<cv::Vec3b>(j,i)[1] = green;
            tileMat.at<cv::Vec3b>(j,i)[2] = red;
        }
    }

    cv::resize(tileMat, outTileMat, cv::Size(800, 600), 0 ,0, CV_INTER_NN);
    
    cv::imshow("tileMap", outTileMat);
}

int findNearestImage(cv::Scalar color, std::vector<cv::Scalar>colors)
{
    int index = -1;
    int minDistance = 100000000;
    for (int i = 0; i < colors.size(); i++) {
        cv::Scalar baseColor = colors[i];
        
        int diff1 = abs(color[0] - baseColor[0]);
        int diff2 = abs(color[1] - baseColor[1]);
        int diff3 = abs(color[2] - baseColor[2]);
        
        int diffTotal = diff1 + diff2 + diff3;
        if (diffTotal < minDistance) {
            minDistance = diffTotal;
            index = i;
        }
    }
    return index;
}

cv::Mat tileMatchImage(cv::Mat& matchList, cv::Mat& processMat, cv::Mat tileMat, std::vector<cv::Mat> images, std::vector<cv::Scalar> colors)
{
    matchList = cv::Mat(tileMat.rows, tileMat.cols, CV_8UC1);
    int t_Width = OUT_RES_W / tileMat.cols;
    int t_Height = OUT_RES_H / tileMat.rows;
    
//    std::cout << t_Width << std::endl;
//    std::cout << t_Height << std::endl;

    cv::Mat resultMat(OUT_RES_H, OUT_RES_W, CV_8UC3);
    
//    std::cout << resultMat.cols << std::endl;
//    std::cout << resultMat.rows << std::endl;
    for (int j = 0; j < tileMat.rows; j++)
    {
        for (int i = 0; i < tileMat.cols; i++)
        {
            cv::Scalar color;
            color[0] = tileMat.at<cv::Vec3b>(j, i)[0];
            color[1] = tileMat.at<cv::Vec3b>(j, i)[1];
            color[2] = tileMat.at<cv::Vec3b>(j, i)[2];
            
            // For loop find nearest image
            // use another function
            int index = findNearestImage(color, colors);
            
            matchList.at<uchar>(j, i) = index;
            cv::Mat smallMat = images[index].clone();
            cv::resize(smallMat, smallMat, cv::Size(t_Width,t_Height), CV_INTER_NN);
//            std::cout << i << "," << j << std::endl;

            cv::Mat roiMat = resultMat(cv::Rect(i*t_Width, j*t_Height, t_Width, t_Height));
            
            smallMat.copyTo(roiMat);
            
        }
    }
    return resultMat;
}

int main(int argc, const char * argv[])
{
    //listAllImageFiles("/Users/peacedove/Pictures/Wallpaper");
    //return 0;
    // insert code here...
    std::ifstream imgListFile;
    //std::ofstream outFile;
    std::string inputfileName = "./abstract.png";
    std::string outfileName = "./result.png";
    std::string imgLibPath = "/Users/peacedove/Pictures/Wallpaper";
    std::string imgListName = "imageList.txt";
    std::string filename = "test.txt";
    
    if (argc < 8) {
        puts("./PhotoMosaic inputfilePath imageLibPath outputFileName TILE_WIDTH TILE_HEIGHT OUT_RES_W OUT_RES_H");
        return 0;
    }
    
    inputfileName = argv[1];
    imgLibPath = argv[2];
    outfileName = argv[3];
    TILE_WIDTH = to_string(argv[4]);
    TILE_HEIGHT = to_string(argv[5]);
    OUT_RES_W = to_string(argv[6]);
    OUT_RES_H = to_string(argv[7]);
    
    
    
    imgListFile.open(imgListName.c_str());
    //outFile.open(filename.c_str(), std::ios_base::app);
    
    std::vector<cv::Mat> images;
    std::vector<cv::Scalar> colors;
    
    puts("-get all image files in directory");

    std::vector<std::string> imgList = listAllImageFiles(imgLibPath);
    
    puts("*done");
    
    puts("-processing all image get from the directory");
    //loadImage(images, colors, imgListFile, outFile);
    loadImage(images, colors, imgList);
    
    puts("*done");
    
    puts("-finding suitable images");
    
    cv::Mat baseMat = cv::imread(inputfileName);
    cv::Mat processMat = baseMat.clone();
    cv::Mat resultMat = baseMat.clone();
    
    if (debugFlag)
    {
        for (int i = 0; i < resultMat.cols; i += TILE_WIDTH)
        {
            cv::line(resultMat, cv::Point(i,0), cv::Point(i, resultMat.rows), cv::Scalar(100,100,100));
        }
        
        for (int i = 0; i < resultMat.rows; i += TILE_HEIGHT)
        {
            cv::line(resultMat, cv::Point(0, i), cv::Point(resultMat.cols, i), cv::Scalar(100,100,100));
        }
    }
    
    cv::Mat tileMat;
    
    computeTileAvgRGB(processMat, tileMat);
    
    cv::Mat matchList;
    resultMat = tileMatchImage(matchList, processMat, tileMat, images, colors);
    
    puts("*done");
    
    cv::imwrite(outfileName, resultMat);
    
    puts("----debug output----");
    puts("press ctrl-c in terminal to end the process");
    puts("press esc on result preview window to end the process");
    
    cv::resize(processMat, processMat, cv::Size(800,600));
    cv::imshow("Source", processMat);
    
    cv::resize(resultMat, resultMat, cv::Size(800,600));
    cv::imshow("Result", resultMat);
    
    
    
    cv::waitKey(0);
    
    
    
    return 0;
}


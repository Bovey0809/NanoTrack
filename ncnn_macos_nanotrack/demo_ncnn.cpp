#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>

#ifdef __APPLE__
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "nanotrack.hpp"

void cxy_wh_2_rect(const cv::Point& pos, const cv::Point2f& sz, cv::Rect &rect) 
{   
    rect.x = max(0, pos.x - int(sz.x / 2));
    rect.y = max(0, pos.y - int(sz.y / 2));
    rect.width = int(sz.x);   
    rect.height = int(sz.y);    
}

void track(NanoTrack *siam_tracker, const char *video_path)

{
    // Read video 
    cv::VideoCapture capture; 
    bool ret;
    if (strlen(video_path)==1)
        ret = capture.open(atoi(video_path));  
    else
        ret = capture.open(video_path); 

    // Exit if video not opened.
    if (!ret) 
        std::cout << "Open cap failed!" << std::endl;

    // Read first frame. 
    cv::Mat frame; 
    
    bool ok = capture.read(frame);
    if (!ok)
    {
        std::cout<< "Cannot read video file" << std::endl;
        return; 
    }
    
    // Select a rect.
    cv::namedWindow("demo"); 
    cv::Rect trackWindow = cv::selectROI("demo", frame); // 手动选择
    //cv::Rect trackWindow =cv::Rect(244,161,74,70);         // 固定值 
    
    // Initialize tracker with first frame and rect.
    State state; 
    
    //   frame, (cx,cy) , (w,h), state
    siam_tracker->init(frame, trackWindow);
    std::cout << "==========================" << std::endl;
    std::cout << "Init done!" << std::endl; 
    std::cout << std::endl; 
    cv::Mat init_window;
    frame(trackWindow).copyTo(init_window); 

    bool paused = false;
    for (;;)
    {
        if (!paused) {
            // Read a new frame
            capture >> frame;
            if (frame.empty())
                break;
        }

        // Start timer
        double t = (double)cv::getTickCount();
        
        // Update tracker if not paused
        if (!paused) {
            siam_tracker->track(frame);
        }
        
        // Calculate Frames per second (FPS)
        double fps = cv::getTickFrequency() / ((double)cv::getTickCount() - t);
        
        // Result to rect
        cv::Rect rect;
        cxy_wh_2_rect(siam_tracker->state.target_pos, siam_tracker->state.target_sz, rect);

        // Boundary judgment
        if (0 <= rect.x && 0 <= rect.width && rect.x + rect.width <= frame.cols && 
            0 <= rect.y && 0 <= rect.height && rect.y + rect.height <= frame.rows)
        {
            cv::rectangle(frame, rect, cv::Scalar(0, 255, 0));
        }

        // Display FPS and instructions
        std::cout << "FPS: " << fps << std::endl;
        std::cout << "按空格键暂停/继续" << std::endl;
        std::cout << "暂停时按 r 键重新选择区域" << std::endl;
        std::cout << "按 q 键退出" << std::endl;
        std::cout << std::endl;

        // Display result
        cv::imshow("demo", frame);
        
        // Handle key events
        char key = (char)cv::waitKey(paused ? 0 : 20);
        if (key == 'q' || key == 'Q')
            break;
        else if (key == ' ') // 空格键切换暂停状态
            paused = !paused;
        else if ((key == 'r' || key == 'R') && paused) {
            // 重新选择区域并初始化跟踪器
            cv::Rect newTrackWindow = cv::selectROI("demo", frame);
            siam_tracker->init(frame, newTrackWindow);
        }
    }

    cv::destroyWindow("demo");
    capture.release();
}

// 添加文件选择函数
std::string openFileDialog() {
#ifdef __APPLE__
    __block std::string result = "";
    dispatch_sync(dispatch_get_main_queue(), ^{
        @autoreleasepool {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setCanChooseFiles:YES];
            [panel setCanChooseDirectories:NO];
            [panel setAllowsMultipleSelection:NO];
            [panel setAllowedFileTypes:@[@"mp4", @"avi", @"mkv", @"mov"]];
            
            // 设置对话框标题和按钮
            [panel setTitle:@"选择视频文件"];
            [panel setPrompt:@"选择"];
            
            // 设置为模态窗口
            [panel setLevel:NSModalPanelWindowLevel];
            [panel setFloatingPanel:YES];
            
            NSModalResponse response = [panel runModal];
            
            if (response == NSModalResponseOK) {
                NSURL* url = [[panel URLs] objectAtIndex:0];
                NSString* path = [url path];
                if (path) {
                    result = std::string([[url path] UTF8String]);
                    NSLog(@"Selected file: %@", path);
                }
            }
        }
    });
    return result;
#else
    return "";
#endif
}

int main(int argc, char** argv)
{
    // Get model path
    std::string backbone_model = "./model/ncnn/nanotrack_backbone_sim-opt";
    std::string head_model = "./model/ncnn/nanotrack_head_sim-opt";

    // 添加文件检查
    std::cout << "正在检查模型文件..." << std::endl;
    std::ifstream backbone_file(backbone_model + ".param");
    std::ifstream head_file(head_model + ".param");
    if (!backbone_file.good() || !head_file.good()) {
        std::cout << "错误：无法找到模型文件！" << std::endl;
        std::cout << "请确保以下文件存在：" << std::endl;
        std::cout << backbone_model << ".param/.bin" << std::endl;
        std::cout << head_model << ".param/.bin" << std::endl;
        return -1;
    }

    // Build tracker 
    std::cout << "正在初始化追踪器..." << std::endl;
    NanoTrack *siam_tracker = new NanoTrack(); 
    siam_tracker->load_model(backbone_model, head_model);
    
    std::string video_path;
    
    // 如果有命令行参数，直接使用指定的视频文件
    if (argc > 1) {
        video_path = argv[1];
    } else {
        // 否则使用文件选择对话框
        int max_attempts = 3;
        int attempts = 0;
        
        do {
            video_path = openFileDialog();
            if (!video_path.empty()) {
                break;
            }
            attempts++;
            if (attempts < max_attempts) {
                std::cout << "请重新选择视频文件（还剩 " << max_attempts - attempts << " 次机会）" << std::endl;
            }
        } while (attempts < max_attempts);
    }

    // 验证文件是否存在
    std::ifstream video_file(video_path);
    if (!video_file.good()) {
        std::cout << "无法访问选择的视频文件：" << video_path << std::endl;
        delete siam_tracker;
        return -1;
    }
    video_file.close();

    std::cout << "开始处理视频：" << video_path << std::endl;
    track(siam_tracker, video_path.c_str());

    delete siam_tracker;
    return 0;
}

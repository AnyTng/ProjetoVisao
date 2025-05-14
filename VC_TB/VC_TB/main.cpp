#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

extern "C" {
#include "vc.h"
}

int main(void) {
    // Caminho para o arquivo de v�deo
#ifdef _WIN32      //Windows
    const char *videofile = "..\\..\\Assets\\video2.mp4";
#elif defined(__APPLE__) && defined(__MACH__)   // macOS
    const char *videofile = "../../Assets/video2.mp4";
#else                               // Linux
    const char *videofile = "../Assets/video2.mp4";
#endif

    cv::VideoCapture capture;
    struct {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;

    std::string str;
    int key = 0;

    // Abre v�deo a partir de um arquivo
    capture.open(videofile);

    // Alternativa: capturar da webcam
    // capture.open(0, cv::CAP_DSHOW);

    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir o ficheiro de video!\n";
        return 1;
    }
    else {
        std::cout << "Ficheiro de video aberto com sucesso!\n";
    }

    // Informa��es sobre o v�deo
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    // Cria janela para exibir o v�deo
    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);
    
    cv::namedWindow("VC - VIDEO2", cv::WINDOW_AUTOSIZE);

    cv::Mat frame;
    cv::Mat frame2;
    const int TEXT_X = 20;
    int textY;
    const int LINE_SPACING = 25;

    IVC *image0 = vc_image_new(video.width, video.height, 3, 255);
    IVC *image1 = vc_image_new(video.width, video.height, 3, 255);
    IVC *image2 = vc_image_new(video.width, video.height, 1, 255);
    IVC *image3 = vc_image_new(video.width, video.height, 1, 255);
    IVC *image4 = vc_image_new(video.width, video.height, 3, 255);
    IVC *image5 = vc_image_new(video.width, video.height, 3, 255);
    IVC *image6 = vc_image_new(video.width, video.height, 1, 255);
    OVC *blobs;
    int nblobs = 0;
    
    
    while (key != 'q') {
        // Le uma frame
        capture.read(frame);
        capture.read(frame2);

        if (frame.empty()) {
            std::cerr << "Fim do video ou erro ao ler frame.\n";
            break;
        }
        if (frame2.empty()) {
            std::cerr << "Fim do video ou erro ao ler frame.\n";
            break;
        }


        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        textY = 25;
        str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);

        textY += LINE_SPACING;
        str = "TOTAL DE FRAMES: " + std::to_string(video.ntotalframes);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);

            textY += LINE_SPACING;
            str = "FRAME RATE: " + std::to_string(video.fps);
            cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
            cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);
            
            textY += LINE_SPACING;
            str = "N. DA FRAME: " + std::to_string(video.nframe);
            cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
            cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);
            
            // Integra��o com biblioteca vc (descomentando se necess�rio)
            
        memcpy(image0->data, frame.data, video.width * video.height * 3);
        
        vc_bgr_to_rgb(image0);

        vc_rgb_to_hsv(image0, image1);
        vc_hsv_segmentation(image1, image2, 0, 360, 0, 25, 55, 100);
        vc_binary_close2(image2, image3, 7, 5);

        //vc_image_channels_change(image3, image4);
        
        vc_image_alter_mask(image1, image3, image5);
        
        vc_hsv_segmentation(image5, image2, 10, 40, 20, 100, 5, 45);//cobre
        vc_binary_open2(image2, image3, 9, 13);//cobre
        
        //vc_gray_negative(image3);

        //vc_image_alter_mask(image1, image3, image4);
        
        blobs = vc_binary_blob_labelling(image3, image6, &nblobs);

        vc_binary_blob_info(image3, blobs, nblobs);

        
        vc_image_channels_change(image6, image4);
        
        memcpy(frame.data, image4->data, video.width * video.height * 3);
        
        
        // Exibe frame
        cv::imshow("VC - VIDEO", frame);
        
        cv::imshow("VC - VIDEO2", frame2);
        
        key = cv::waitKey(1);
    }
    
    for (int i = 0; i < nblobs; i++) {
        std::cout << "Blob " << i + 1 << ": Label: " << blobs[i].label
            << ", Area: " << blobs[i].area
            << ", Perimeter: " << blobs[i].perimeter
            << std::endl;
    }

    vc_image_free(image0);
    vc_image_free(image1);
    vc_image_free(image2);
    vc_image_free(image3);
    vc_image_free(image4);
    vc_image_free(image5);
    vc_image_free(image6);
    free(blobs);

    cv::destroyWindow("VC - VIDEO");
    cv::destroyWindow("VC - VIDEO2");

    capture.release();

    return 0;
}

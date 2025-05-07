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
#ifdef _WIN32                       // Any flavour of Windows (32? or 64?bit)
    const char *videofile = "..\\Assets\\video2.mp4";
#elif defined(__APPLE__) && defined(__MACH__)   // macOS
    const char *videofile = "../../Assets/video2.mp4";
#else                               // Linux / BSD / anything else
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
        std::cerr << "Erro ao abrir o ficheiro de v�deo!\n";
        return 1;
    }

    // Informa��es sobre o v�deo
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    // Cria janela para exibir o v�deo
    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

    cv::Mat frame;
    const int TEXT_X = 20;
    int textY;
    const int LINE_SPACING = 25;

    while (key != 'q') {
        // L� uma frame
        capture.read(frame);

        if (frame.empty()) {
            std::cerr << "Fim do v�deo ou erro ao ler frame.\n";
            break;
        }

        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        textY = 25;
        str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);

        textY += LINE_SPACING;
        str = "TOTAL DE FRAMES: " + std::to_string(video.ntotalframes);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);

        textY += LINE_SPACING;
        str = "FRAME RATE: " + std::to_string(video.fps);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);

        textY += LINE_SPACING;
        str = "N. DA FRAME: " + std::to_string(video.nframe);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);

        // Integra��o com biblioteca vc (descomentando se necess�rio)
        /*
        IVC *image = vc_image_new(video.width, video.height, 3, 255);
        memcpy(image->data, frame.data, video.width * video.height * 3);
        vc_rgb_get_green(image);
        memcpy(frame.data, image->data, video.width * video.height * 3);
        vc_image_free(image);
        */

        // Exibe frame
        cv::imshow("VC - VIDEO", frame);

        key = cv::waitKey(1);
    }

    cv::destroyWindow("VC - VIDEO");
    capture.release();

    return 0;
}

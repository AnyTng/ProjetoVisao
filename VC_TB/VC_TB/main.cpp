#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

extern "C" {
#include "vc.h"
}

struct TrackedBlob {
    int id;
    cv::Point centroid;
};

struct Blobs
{
    int id;
    int area;
    Blobs *next;
};

int insereBlob (Blobs **head, int area, int id)
{   
    if (area < 0) return 0;
    if (id < 0) return 0;

    // Verifica se o blob já existe
    Blobs *current = *head;
    while (current != NULL) {
        if (current->id == id) {
            int maior = MAX(current->area, area);
            current->area = maior;
            return 1; // Blob já existe
        }
        current = current->next;
    }
    // Insere o novo blob
    Blobs *newBlob = (Blobs *)malloc(sizeof(Blobs));
    if (!newBlob) return 0;

    newBlob->id = id;
    newBlob->area = area;
    newBlob->next = *head;
    *head = newBlob;

    return 1;
}


int next_id = 1;
std::vector<TrackedBlob> tracked_blobs;

int find_closest_blob(cv::Point centroid) {
    int min_dist = 140;  // Distância máxima para considerar o mesmo blob
    int best_id = -1;
    for (auto& blob : tracked_blobs) {
        int dist = cv::norm(centroid - blob.centroid);
        if (dist < min_dist) {
            min_dist = dist;
            best_id = blob.id;
            blob.centroid = centroid;  // Atualiza a posição do blob
        }
    }
    return best_id;
}


int main(void) {
#ifdef _WIN32
    const char *videofile = "..\\..\\Assets\\video1.mp4";
#elif defined(__APPLE__) && defined(__MACH__)
    const char *videofile = "../../Assets/video2.mp4";
#else
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

    // Abrir vídeo
    capture.open(videofile);
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir o ficheiro de video!\n";
        return 1;
    } else {
        std::cout << "Ficheiro de video aberto com sucesso!\n";
    }

    // Info do vídeo
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    //cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("VC - VIDEO2", cv::WINDOW_AUTOSIZE);

    cv::Mat frame, frame2;
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
    Blobs *lista;
    int nblobs = 0;
    
    std::vector<cv::Point> previous_centroids;
    
    while (key != 'q') {
        capture.read(frame);
        capture.read(frame2);
        
        if (frame.empty() || frame2.empty()) {
            std::cerr << "Fim do video ou erro ao ler frame.\n";
            break;
        }
        
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);
        textY = 25;
        
        str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        
        textY += LINE_SPACING;
        str = "TOTAL DE FRAMES: " + std::to_string(video.ntotalframes);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        
        textY += LINE_SPACING;
        str = "N. DA FRAME: " + std::to_string(video.nframe);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
        cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);
            
        textY += LINE_SPACING;
        str = "N. DE BLOBS: " + std::to_string(nblobs);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);    
            
        memcpy(image0->data, frame.data, video.width * video.height * 3);
        vc_bgr_to_rgb(image0);
        vc_rgb_to_hsv(image0, image1);
            
        vc_hsv_segmentation(image1, image2, 0, 360, 0, 25, 55, 100);
            
        vc_binary_close2(image2, image3, 7, 7);
        vc_image_alter_mask(image1, image3, image5);
            
        vc_hsv_segmentation(image5, image2, 10, 40, 20, 100, 5, 45); // cobre
        vc_binary_open2(image2, image3, 19, 19);
                 
        blobs = vc_binary_blob_labelling(image3, image6, &nblobs);
        vc_binary_blob_info(image6, blobs, nblobs);

        for (int i = 0; i < nblobs; i++) {
            cv::Point centroid(blobs[i].xc, blobs[i].yc);
            int id = find_closest_blob(centroid);

            if (id == -1) {
                id = next_id++;
                tracked_blobs.push_back({id, centroid});
            }
            
            insereBlob(&lista, blobs[i].area, id);
            
            cv::circle(frame2, centroid, 5, cv::Scalar(0, 255, 0), -1);
            cv::rectangle(frame2, cv::Rect(blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height), cv::Scalar(0, 0, 255), 2);
            cv::putText(frame2, "ID: " + std::to_string(id), centroid, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 2);
            cv::putText(frame2, "Area: " + std::to_string(blobs[i].area), cv::Point(blobs[i].x, blobs[i].y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
        }
        
        vc_image_channels_change(image6, image4);
        
        //memcpy(frame.data, image4->data, video.width * video.height * 3);
        
        //cv::imshow("VC - VIDEO", frame);
        cv::imshow("VC - VIDEO2", frame2);
        
        key = cv::waitKey(1);
    }
    

    for (Blobs *current = lista; current != NULL; ) {
        Blobs *next = current->next;
        std::cout << "Blob ID: " << current->id << ", Area: " << current->area << std::endl;
        current = next;
    }

    for (Blobs *current = lista; current != NULL; ) {
        Blobs *next = current->next;
        free(current);
        current = next;
    }
    vc_image_free(image0);
    vc_image_free(image1);
    vc_image_free(image2);
    vc_image_free(image3);
    vc_image_free(image4);
    vc_image_free(image5);
    vc_image_free(image6);
    free(blobs);
    cv::destroyAllWindows();
    capture.release();

    std::cout << "Pressione qualquer tecla para sair...\n";
    std::cin.get();

    return 0;
}

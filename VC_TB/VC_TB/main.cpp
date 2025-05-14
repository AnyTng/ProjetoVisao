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

constexpr int MAX_MISS = 5;     // nº máx. de frames sem detecção
constexpr int MIN_DIST = 140;

struct TrackedBlob {
    int id;
    cv::Point centroid;
    int miss = 0;
};

struct Blobs
{
    int id;
    int area;
    Blobs *next;
};

int insereBlob (Blobs **head, int area, int id)
{   
    if (area < 0 || id < 0) return 0;

    // Verifica se o blob já existe
    for (Blobs *current = *head; current; current = current->next) {  // já existe?
        if (current->id == id) { 
            current->area = MAX(current->area, area); return 1; 
        }
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


static int next_id = 1;
static std::vector<TrackedBlob> tracked_blobs;

int find_closest_blob(cv::Point centroid) {
    int min_dist = MIN_DIST;  // Distância máxima para considerar o mesmo blob
    int best_id = -1;
    for (size_t i = 0; i < tracked_blobs.size(); ++i) {
        int dist = cv::norm(centroid - tracked_blobs[i].centroid);
        if (dist < min_dist) { min_dist = dist; best_id = (int)i; }
    }
    /*
    for (auto& blob : tracked_blobs) {
        int dist = cv::norm(centroid - blob.centroid);
        if (dist < min_dist) {
            min_dist = dist;
            best_id = blob.id;
            blob.centroid = centroid;  // Atualiza a posição do blob
            blob.age = 0;
        }
    }
    */
    return best_id;
}

/*
int update_blob_ages() {
    for (auto& blob : tracked_blobs) {
        blob.age++;
    }
    return 1;
}
*/

/*
void remove_old_blobs() {
    tracked_blobs.erase(std::remove_if(tracked_blobs.begin(), tracked_blobs.end(), [](const TrackedBlob& blob) {
        return blob.age > 5;
    }), tracked_blobs.end());
}
*/



int main(void) {
#ifdef _WIN32
    const char *videofile = "..\\..\\Assets\\video2.mp4";
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
    //IVC *image4 = vc_image_new(video.width, video.height, 3, 255);
    IVC *image5 = vc_image_new(video.width, video.height, 3, 255);
    IVC *image6 = vc_image_new(video.width, video.height, 1, 255);
    OVC *blobs = nullptr;
    Blobs *lista = nullptr;
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
        vc_binary_open2(image2, image3, 17, 17);
        
        nblobs = 0;
        blobs = vc_binary_blob_labelling(image3, image6, &nblobs);
        vc_binary_blob_info(image6, blobs, nblobs);

        //update_blob_ages();
        for (TrackedBlob &tb : tracked_blobs) tb.miss++; 

        for (int i = 0; i < nblobs; i++) {
            cv::Point centroid(blobs[i].xc, blobs[i].yc);
            int idx = find_closest_blob(centroid);

            if (idx == -1) {                                 // novo
                tracked_blobs.push_back({ next_id++, centroid, 0 });
                idx = (int)tracked_blobs.size() - 1;
            } else {                                         // visto
                tracked_blobs[idx].centroid = centroid;
                tracked_blobs[idx].miss = 0;
            }
            int id = tracked_blobs[idx].id;

            insereBlob(&lista, blobs[i].area, id);
            
            cv::circle(frame2, centroid, 5, cv::Scalar(0, 255, 0), -1);
            cv::rectangle(frame2, cv::Rect(blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height), cv::Scalar(0, 0, 255), 2);
            cv::putText(frame2, "ID: " + std::to_string(id), centroid, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 2);
            cv::putText(frame2, "Area: " + std::to_string(blobs[i].area), cv::Point(blobs[i].x, blobs[i].y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
        }
        
        //vc_image_channels_change(image6, image4);
        
        //memcpy(frame.data, image4->data, video.width * video.height * 3);
        
        //remove_old_blobs();
        tracked_blobs.erase(
            std::remove_if(tracked_blobs.begin(), tracked_blobs.end(),
                           [](const TrackedBlob &b){ return b.miss > MAX_MISS; }),
            tracked_blobs.end());


        //cv::imshow("VC - VIDEO", frame);
        cv::imshow("VC - VIDEO2", frame2);
        
        free(blobs);
        key = cv::waitKey(1);
    }
    
    int count1=0, count2=0, count5=0;
    for (Blobs *current = lista; current != NULL; ) {
        Blobs *next = current->next;
        
        if (current->area > 3000 && current->area < 12000) count1++;
        if (current->area > 12010 && current->area < 15990) count2++;
        if (current->area > 16000 && current->area < 20000) count5++;
        
        std::cout << "Blob ID: " << current->id << ", Area: " << current->area << std::endl;
        current = next;
    }
    std::cout << "\nMoedas 0,01: " << count1 << "\nMoedas 0,02: " << count2 << "\nMoedas 0,05: " << count5 << std::endl;

    for (Blobs *current = lista; current != NULL; ) {
        Blobs *next = current->next;
        free(current);
        current = next;
    }
    vc_image_free(image0);
    vc_image_free(image1);
    vc_image_free(image2);
    vc_image_free(image3);
    //vc_image_free(image4);
    vc_image_free(image5);
    vc_image_free(image6);
    free(blobs);
    cv::destroyAllWindows();
    capture.release();

    std::cout << "Pressione qualquer tecla para sair...\n";
    std::cin.get();

    return 0;
}

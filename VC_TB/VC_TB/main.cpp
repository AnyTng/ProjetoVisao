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

constexpr int MAX_MISS = 6;     // nº máx. de frames sem detecção
constexpr int MIN_DIST = 120;
constexpr int MIN_DISTD = 130;

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


static int nextIdC = 1;
static int nextIdD = 1;
static std::vector<TrackedBlob> tracked_blobs;

int find_closest_blob(const std::vector<TrackedBlob>& tracked_blobs, cv::Point centroid) {
    int min_dist = MIN_DIST; 
    int best_id = -1;
    for (size_t i = 0; i < tracked_blobs.size(); ++i) {
        int dist = cv::norm(centroid - tracked_blobs[i].centroid);
        if (dist < min_dist) { min_dist = dist; best_id = (int)i; }
    }
    return best_id;
}
int find_closest_blob_D(const std::vector<TrackedBlob>& tracked_blobs, cv::Point centroid) {
    int min_dist = MIN_DISTD;
    int best_id = -1;
    for (size_t i = 0; i < tracked_blobs.size(); ++i) {
        int dist = cv::norm(centroid - tracked_blobs[i].centroid);
        if (dist < min_dist) { min_dist = dist; best_id = (int)i; }
    }
    return best_id;
}

void exportVideo(const std::string& filename,
    const cv::Size& frameSize,
    const std::vector<cv::Mat>& frames,
    double fps = 30.0)
{
    // Define o codec: aqui usamos MJPG (bastante compatível)
    int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    cv::VideoWriter writer(filename, fourcc, fps, frameSize, true);
    if (!writer.isOpened()) {
        throw std::runtime_error("Não foi possível abrir o arquivo de saída: " + filename);
    }
    for (const auto& f : frames) {
        writer.write(f);   // escreve cada frame
    }
    writer.release();
}

int main() {
#ifdef _WIN32
    const char* videofile = "..\\Assets\\video2.mp4";
#elif defined(__APPLE__) && defined(__MACH__)
    const char* videofile = "../../Assets/video2.mp4";
#else
    const char* videofile = "../Assets/video2.mp4";
#endif


    cv::VideoCapture capture;
    struct {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;

    std::string str;

    capture.open(videofile);
    if (!capture.isOpened()) {
        std::cerr << "Erro ao abrir o ficheiro de video!\n";
        return 1;
    }
    else {
        std::cout << "Ficheiro de video aberto com sucesso!\n";
    }
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    
    IVC* image0 = vc_image_new(video.width, video.height, 3, 255);
    IVC* image1 = vc_image_new(video.width, video.height, 3, 255);
    IVC* image2 = vc_image_new(video.width, video.height, 1, 255);
    IVC* image3 = vc_image_new(video.width, video.height, 1, 255);
    IVC* image5 = vc_image_new(video.width, video.height, 3, 255);
    IVC* image6 = vc_image_new(video.width, video.height, 1, 255);

    cv::Mat frame, frame2;
    const int TEXT_X = 20;
    int textY;
    const int LINE_SPACING = 25;

    std::vector<TrackedBlob> tracked_blobs_cobre;
    std::vector<TrackedBlob> tracked_blobs_dourado;
    int nextIdC = 1, nextIdD = 1;
    Blobs* listaC = nullptr, * listaD = nullptr;

    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);
    /////////////////
    std::vector<cv::Mat> processedFrames;
    /////////////////
    int key = 0;

    while (key != 'q') {
        if (!capture.read(frame)) break;
        frame.copyTo(frame2);
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


        memcpy(image0->data, frame.data, video.width * video.height * 3);
        vc_bgr_to_rgb(image0);
        vc_rgb_to_hsv(image0, image1);
        vc_hsv_segmentation(image1, image2, 0, 360, 0, 25, 55, 100); 
        vc_binary_close2(image2, image3, 7, 7);
        vc_image_alter_mask(image1, image3, image5);

        // ---------------- Zona Cobre ----------------
        vc_hsv_segmentation(image5, image2, 10, 40, 20, 100, 5, 45);
        vc_binary_open2(image2, image3, 17, 17);
        int nblobs = 0;
        OVC* blobs = vc_binary_blob_labelling(image3, image6, &nblobs);
        vc_binary_blob_info(image6, blobs, nblobs);

        for (TrackedBlob& tb : tracked_blobs_cobre) tb.miss++;

        for (int i = 0; i < nblobs; ++i) {
            if (blobs[i].area > 3000) {
                cv::Point centroid(blobs[i].xc, blobs[i].yc);
                int idx = find_closest_blob(tracked_blobs_cobre, centroid);
                if (idx == -1) {
                    tracked_blobs_cobre.push_back({ nextIdC++, centroid, 0 });
                    idx = static_cast<int>(tracked_blobs_cobre.size()) - 1;
                }
                else {
                    tracked_blobs_cobre[idx].centroid = centroid;
                    tracked_blobs_cobre[idx].miss = 0;
                }
                int id = tracked_blobs_cobre[idx].id;
                insereBlob(&listaC, blobs[i].area, id);
                cv::circle(frame2, centroid, 5, cv::Scalar(0, 255, 0), -1);
                cv::rectangle(frame2, cv::Rect(blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height), cv::Scalar(0, 0, 255), 2);
                cv::putText(frame2, "ID_C: " + std::to_string(id), centroid, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 2);
            }
        }

        tracked_blobs_cobre.erase(
            std::remove_if(tracked_blobs_cobre.begin(), tracked_blobs_cobre.end(),
                [](const TrackedBlob& b) { return b.miss > MAX_MISS; }),
            tracked_blobs_cobre.end());
        free(blobs);

        // ---------------- Zona Dourado ----------------
        vc_hsv_segmentation(image5, image2, 40, 80, 20, 90, 10, 70);
        vc_binary_open2(image2, image3, 17, 21);
        nblobs = 0;
        blobs = vc_binary_blob_labelling(image3, image6, &nblobs);
        vc_binary_blob_info(image6, blobs, nblobs);

        for (TrackedBlob& tb : tracked_blobs_dourado) tb.miss++;

        for (int i = 0; i < nblobs; ++i) {
            if (blobs[i].area > 3000) {
                cv::Point centroid(blobs[i].xc, blobs[i].yc);
                int idx = find_closest_blob_D(tracked_blobs_dourado, centroid);
                if (idx == -1) {
                    tracked_blobs_dourado.push_back({ nextIdD++, centroid, 0 });
                    idx = static_cast<int>(tracked_blobs_dourado.size()) - 1;
                }
                else {
                    tracked_blobs_dourado[idx].centroid = centroid;
                    tracked_blobs_dourado[idx].miss = 0;
                }
                int id = tracked_blobs_dourado[idx].id;
                insereBlob(&listaD, blobs[i].area, id);
                cv::circle(frame2, centroid, 5, cv::Scalar(0, 255, 0), -1);
                cv::rectangle(frame2, cv::Rect(blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height), cv::Scalar(255, 0, 0), 2);
                cv::putText(frame2, "ID_D: " + std::to_string(id), centroid, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
            }
        }

        tracked_blobs_dourado.erase(
            std::remove_if(tracked_blobs_dourado.begin(), tracked_blobs_dourado.end(),
                [](const TrackedBlob& b) { return b.miss > MAX_MISS; }),
            tracked_blobs_dourado.end());
        free(blobs);

        ////////////////////
        processedFrames.push_back(frame2.clone());
        ////////////////

        // Mostrar
        cv::imshow("VC - VIDEO", frame2);
        key = cv::waitKey(1);
    }

    exportVideo("saida2.avi",
        cv::Size(video.width, video.height),
        processedFrames,
        30.0);
    std::cout << "Video exportado em saida.avi\n";

    int count1=0, count2=0, count5=0;
    for (Blobs *current = listaC; current != NULL; ) {
        Blobs *next = current->next;

        if (current->area > 3000 && current->area < 12000) count1++;
        if (current->area > 12010 && current->area < 15990) count2++;
        if (current->area > 16000 && current->area < 20000) count5++;

        std::cout << "Blob ID: " << current->id << ", Area: " << current->area << std::endl;
        current = next;
    }
    std::cout << "\n\nMoedas 0,01: " << count1 << "\nMoedas 0,02: " << count2 << "\nMoedas 0,05: " << count5 << std::endl;

    int count10=0, count20=0, count50=0, count100=0, count200=0;

    for (Blobs *current = listaD; current != NULL; ) {
        Blobs *next = current->next;

        if (current->area > 16906 && current->area < 19999) count10++;
        if (current->area > 20000 && current->area < 23599) count20++;
        if (current->area > 24000) count50++;
        if (current->area > 3000 && current->area < 14599) count100++;
        if (current->area > 14600 && current->area <= 16905) count200++;

        std::cout << "Blob ID: " << current->id << ", Area: " << current->area << std::endl;
        current = next;
    }
    std::cout << "\n\nMoedas 0,10: " << count10 << "\nMoedas 0,20: " << count20 << "\nMoedas 0,50: " << count50 << "\nMoedas 1,00: " << count100 << "\nMoedas 2,00: " << count200 << std::endl;

    for (Blobs *current = listaC; current != NULL; ) {
        Blobs *next = current->next;
        free(current);
        current = next;
    }
    for (Blobs *current = listaD; current != NULL; ) {
        Blobs *next = current->next;
        free(current);
        current = next;
    }
    vc_image_free(image0);
    vc_image_free(image1);
    vc_image_free(image2);
    vc_image_free(image3);
    vc_image_free(image5);
    vc_image_free(image6);

    cv::destroyAllWindows();
    capture.release();

    std::cout << "Pressione qualquer tecla para sair...\n";
    std::cin.get();

    return 0;
}


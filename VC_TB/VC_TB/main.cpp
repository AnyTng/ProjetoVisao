//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VISAO POR COMPUTADOR
//
//     [  Fábio R. G. Costa       - a22997@alunos.ipca.pt  ]
//     [  Lino E. O. Azevedo      - a23015@alunos.ipca.pt  ]
//     [  Goncalo T. M. Goncalves - a23020@alunos.ipca.pt  ]
//     [  Hugo F. Baptista        - a23279@alunos.ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

// Inclusão da biblioteca VC 
extern "C" {
#include "vc.h"
}

constexpr int MAX_MISS = 6; // Nº máximo de frames que um objeto pode não aparecer até sem ser removido
constexpr int MIN_DIST = 120; // Distância mínima para considerar que blobs são o mesmo (zona cobre)
constexpr int MIN_DISTD = 130; // Distância mínima para considerar que blobs são o mesmo (zona dourada)

//Estrutura dos blobs detetados
struct TrackedBlob {
    int id;
    cv::Point centroid;
    int miss = 0;
};

// Estrutura de lista ligada para armazenar blobs detectados e suas áreas
struct Blobs
{
    int id;
    int area;
	float perimeter;
    Blobs *next;
};

// Função que insere ou atualiza um blob na lista ligada

int insereBlob (Blobs **head, int area, int id, float perimeter)
{
    if (area < 0 || id < 0) return 0;

	// Verifica se o blob já está na lista
    for (Blobs *current = *head; current; current = current->next) {  
        if (current->id == id) { //Caso já exista um blob com o mesmo id
            current->area = MAX(current->area, area); // Atualiza com a maior área 
			current->perimeter = MAX(current->perimeter, perimeter);
            return 1;
        }
    }
    // Aloca memória para um blob
    Blobs *newBlob = (Blobs *)malloc(sizeof(Blobs));
    if (!newBlob) return 0;

    newBlob->id = id;
    newBlob->area = area;
	newBlob->perimeter = perimeter;
    newBlob->next = *head;
    
    // Atualiza a lista
    *head = newBlob;

    return 1;
}


static int nextIdC = 1;
static int nextIdD = 1;
static std::vector<TrackedBlob> tracked_blobs;

//Função para encontrar o blob mais proximo (cobre)
int find_closest_blob(const std::vector<TrackedBlob>& tracked_blobs, cv::Point centroid) {
    int min_dist = MIN_DIST; 
    int best_id = -1;
    for (size_t i = 0; i < tracked_blobs.size(); ++i) {
        int dist = cv::norm(centroid - tracked_blobs[i].centroid);
        if (dist < min_dist) { min_dist = dist; best_id = (int)i; }
    }
    return best_id;
}

//Função para encontrar o blob mais proximo (dourado)
int find_closest_blob_D(const std::vector<TrackedBlob>& tracked_blobs, cv::Point centroid) {
    int min_dist = MIN_DISTD;
    int best_id = -1;
    for (size_t i = 0; i < tracked_blobs.size(); ++i) {
        int dist = cv::norm(centroid - tracked_blobs[i].centroid);
        if (dist < min_dist) { min_dist = dist; best_id = (int)i; }
    }
    return best_id;
}

//Função para exportar o vídeo processado
void exportVideo(const std::string& filename, const cv::Size& frameSize, const std::vector<cv::Mat>& frames,double fps = 30.0)
{
    int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    cv::VideoWriter writer(filename, fourcc, fps, frameSize, true); // true para cores
    if (!writer.isOpened()) {
        throw std::runtime_error("Não foi possível abrir o arquivo de saída: " + filename);
    }
    for (const auto& f : frames) {
		writer.write(f);  // Adiciona o frame ao vídeo 
    }
    writer.release();
}

int main() {
// Caminho para o vídeo em diversos sistemas operativos
#ifdef _WIN32
    const char* videofile = "..\\Assets\\video1.mp4";
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
	// Obtém propriedades do vídeo
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    
    // Criação das imagens IVC
    IVC* image0 = vc_image_new(video.width, video.height, 3, 255);
    IVC* image1 = vc_image_new(video.width, video.height, 3, 255);
    IVC* image2 = vc_image_new(video.width, video.height, 1, 255);
    IVC* image3 = vc_image_new(video.width, video.height, 1, 255);
    IVC* image5 = vc_image_new(video.width, video.height, 3, 255);
    IVC* image6 = vc_image_new(video.width, video.height, 1, 255);

    // Variáveis auxiliares para escrita
    cv::Mat frame, frame2;
    const int TEXT_X = 20;
    int textY;
    const int LINE_SPACING = 25;

	// Vetor para armazenar os blobs cobre
    std::vector<TrackedBlob> tracked_blobs_cobre;
    // Vetor para armazenar os blobs dourado
    std::vector<TrackedBlob> tracked_blobs_dourado;

    //IDs dos blobs
    int nextIdC = 1, nextIdD = 1;
    //Listas de blobs
    Blobs* listaC = nullptr, * listaD = nullptr;

    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

    /////////////////

	// vetor para armazenar os frames processados
    std::vector<cv::Mat> processedFrames;
    /////////////////

    int key = 0;

    // Loop principal de leitura e processamento dos frames
    while (key != 'q') {
        if (!capture.read(frame)) break;
        frame.copyTo(frame2);
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);
        textY = 25;

        // Escreve no ecrã a resolução do vídeo
        str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        // Escreve no ecrã o total dos frames do vídeo
        textY += LINE_SPACING;
        str = "TOTAL DE FRAMES: " + std::to_string(video.ntotalframes);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        // Escreve no ecrã o número do frame atual
        textY += LINE_SPACING;
        str = "N. DA FRAME: " + std::to_string(video.nframe);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);
        cv::putText(frame2, str, cv::Point(TEXT_X, textY), cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(255, 255, 255), 1);


        // Preparação das imagens. Copia o frame para a imagem 0
        memcpy(image0->data, frame.data, video.width * video.height * 3);
        // Passa de a imagem de BGR (Blue, Green, Red) para RGB (Reed, Green,)
        vc_bgr_to_rgb(image0);
        // Passa a imagem 0 para a imagem 1, em HSV 
        vc_rgb_to_hsv(image0, image1);
        // Segmentação da imagem 1 para a imagem 2, para poder remover o fundo. 
        vc_hsv_segmentation(image1, image2, 0, 360, 0, 25, 55, 100); 
        // "Fecha" (Dilata e erode) a imagem segmentada com um kernel de tamanho 7
        vc_binary_close2(image2, image3, 7, 7);
        // Remove da imagem 1 tudo o que está a branco na mascára
        vc_image_alter_mask(image1, image3, image5);

        //+++++++++++++++++++++++++++++++++++++++++++++
        // ---------------- Zona Cobre ----------------
        //+++++++++++++++++++++++++++++++++++++++++++++

        //Segmenta a imagem, para encontrar zonas que correspondem ao cobre das moedas de 0.1, 0.2 e 0.5 cent.
        vc_hsv_segmentation(image5, image2, 10, 40, 20, 100, 5, 45);

        // "Abre" (Erode e Dilata) a imagem segmentada com um kernel de tamanho 17
        vc_binary_open2(image2, image3, 17, 17);
        
        //Define os blobs encontrados no frame como 0 (reset, para o frame atual)
        int nblobs = 0;

        // Labelling de blobs encontrados na imagem segmentada
        OVC* blobs = vc_binary_blob_labelling(image3, image6, &nblobs);

        // Mete a informação dos blobs
        vc_binary_blob_info(image6, blobs, nblobs);

        
        // Loop que percorre os blobs seguidos de cobre, aumenta o contador de misses.
        for (TrackedBlob& tb : tracked_blobs_cobre) tb.miss++;

        // Loop que percorre os blobs
        for (int i = 0; i < nblobs; ++i) {
            // Verifica se a área do blob é maior que 3000, para evitar falsos positivos
            if (blobs[i].area > 3000) {
                //Calcula o centro de massa do blob
                cv::Point centroid(blobs[i].xc, blobs[i].yc);
                 // Procura o blob rastreado mais próximo do centro de massa atual
                int idx = find_closest_blob(tracked_blobs_cobre, centroid);
                if (idx == -1) {
                     // Se não encontrar nenhum blob próximo, cria um novo blob rastreado
                    tracked_blobs_cobre.push_back({ nextIdC++, centroid, 0 });
                    idx = static_cast<int>(tracked_blobs_cobre.size()) - 1;
                }
                else {
                    // Se encontrar, atualiza o centro de massa e dá reset ao contador de "misses"
                    tracked_blobs_cobre[idx].centroid = centroid;
                    tracked_blobs_cobre[idx].miss = 0;
                }
                // Obtém o ID do blob rastreado
                int id = tracked_blobs_cobre[idx].id;

                // Insere ou atualiza o blob na lista ligada de blobs de cobre
                insereBlob(&listaC, blobs[i].area, id, blobs[i].perimeter);
				
                // Desenha no ecrã o centro de massa do blob
                cv::circle(frame2, centroid, 5, cv::Scalar(0, 255, 0), -1);
                // Desenha no ecrã a bounding box do blob 
                cv::rectangle(frame2, cv::Rect(blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height), cv::Scalar(0, 0, 255), 2);
                //Escreve o ID do blob no ecrã
                cv::putText(frame2, "ID_C: " + std::to_string(id), centroid, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 2);
            }
        }

        // Remove blobs que "Desaparecem" 
        tracked_blobs_cobre.erase(
            std::remove_if(tracked_blobs_cobre.begin(), tracked_blobs_cobre.end(),
                [](const TrackedBlob& b) { return b.miss > MAX_MISS; }),
            tracked_blobs_cobre.end());
        free(blobs);

        //+++++++++++++++++++++++++++++++++++++++++++++++
        // ---------------- Zona Dourado ----------------
        //+++++++++++++++++++++++++++++++++++++++++++++++

		//Segmenta a imagem, para encontrar zonas que correspondem ao dourado das moedas de 10 cent., 20 cent., 50 cent., 1 euro e 2 euros
        vc_hsv_segmentation(image5, image2, 40, 80, 20, 90, 10, 70);
        // "Abre" (Erode e Dilata) a imagem segmentada com um kernel de tamanho 17
        vc_binary_open2(image2, image3, 17, 21);

		//Define os blobs encontrados no frame como 0 (reset, para o frame atual)
        nblobs = 0;

		// Labelling de blobs encontrados na imagem segmentada
        blobs = vc_binary_blob_labelling(image3, image6, &nblobs);

		// Mete a informação dos blobs 
        vc_binary_blob_info(image6, blobs, nblobs);

        //De maneira similar à zona cobre, percorre os blobs seguidos de dourado, aumentando o contador de misses.
        for (TrackedBlob& tb : tracked_blobs_dourado) tb.miss++;
        
        // Loop que percorre os blobs
        for (int i = 0; i < nblobs; ++i) {
            // Verifica se a área do blob é maior que 3000, para evitar falsos positivos
            if (blobs[i].area > 3000) {
                //Calcula o centro de massa do blob
                cv::Point centroid(blobs[i].xc, blobs[i].yc);
                // Procura o blob rastreado mais próximo do centro de massa atual
                int idx = find_closest_blob_D(tracked_blobs_dourado, centroid);
                if (idx == -1) {
                    // Se não encontrar nenhum blob próximo, cria um novo blob rastreado
                    tracked_blobs_dourado.push_back({ nextIdD++, centroid, 0 });
                    idx = static_cast<int>(tracked_blobs_dourado.size()) - 1;
                }
                else {
                    // Se encontrar, atualiza o centro de massa e dá reset ao contador de "misses"
                    tracked_blobs_dourado[idx].centroid = centroid;
                    tracked_blobs_dourado[idx].miss = 0;
                }
                // Obtém o ID do blob rastreado
                int id = tracked_blobs_dourado[idx].id;
                // Insere ou atualiza o blob na lista ligada de blobs dourados
                insereBlob(&listaD, blobs[i].area, id, blobs[i].perimeter);

                // Desenha no ecrã o centro de massa do blob
                cv::circle(frame2, centroid, 5, cv::Scalar(0, 255, 0), -1);
                // Desenha no ecrã a bounding box do blob 
                cv::rectangle(frame2, cv::Rect(blobs[i].x, blobs[i].y, blobs[i].width, blobs[i].height), cv::Scalar(255, 0, 0), 2);
                //Escreve o ID do blob no ecrã
                cv::putText(frame2, "ID_D: " + std::to_string(id), centroid, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
            }
        }

		// Remove blobs que desapareceram
        tracked_blobs_dourado.erase(
            std::remove_if(tracked_blobs_dourado.begin(), tracked_blobs_dourado.end(),
                [](const TrackedBlob& b) { return b.miss > MAX_MISS; }),
            tracked_blobs_dourado.end());
        free(blobs);

        ////////////////
        // Adiciona o frame processado ao vetor de frames processados
        processedFrames.push_back(frame2.clone());
        ////////////////
        
        // Mostra o frame na janela do vídeo
        cv::imshow("VC - VIDEO", frame2);
        key = cv::waitKey(1);
    }

	// Exporta vídeo processado
    exportVideo("saida2.avi",
        cv::Size(video.width, video.height),
        processedFrames,
        30.0);
    std::cout << "Video exportado em saida.avi\n";
    

	// Zona Calculo de areas e quantidades de moedas

    // Com base na área 
	// Contadores para as moedas
    int count1=0, count2=0, count5=0;
    for (Blobs *current = listaC; current != NULL; current = current->next) {
        // Determina qual a moeda pela área do blob
        if (current->area > 3000 && current->area < 12000) count1++;
        if (current->area > 12010 && current->area < 15990) count2++;
        if (current->area > 16000 && current->area < 20000) count5++;

        std::cout << "Blob ID_C: " << current->id << ", Area: " << current->area << ", Perimetro: " << current->perimeter << std::endl;
    }

    //Inicar a 0 contadores para as moedas de 10 cent., 20 cent., 50 cent., 1 euro e 2 euros
    int count10=0, count20=0, count50=0, count100=0, count200=0;
    for (Blobs *current = listaD; current != NULL; current = current->next) {
        // Determina qual a moeda pela área do blob
        if (current->area > 16906 && current->area < 19999) count10++;
        if (current->area > 20000 && current->area < 23599) count20++;
        if (current->area > 24000) count50++;
        if (current->area > 3000 && current->area < 14599) count100++;
        if (current->area > 14600 && current->area <= 16905) count200++;

        std::cout << "Blob ID_D: " << current->id << ", Area: " << current->area << ", Perimetro: " << current->perimeter << std::endl;
    }

    std::cout << "\n\nMoedas 0,01: " << count1 << "\nMoedas 0,02: " << count2 << "\nMoedas 0,05: " << count5 << std::endl;
    std::cout << "Moedas 0,10: " << count10 << "\nMoedas 0,20: " << count20 << "\nMoedas 0,50: " << count50 << "\nMoedas 1,00: " << count100 << "\nMoedas 2,00: " << count200 << std::endl;

	int totalMoedas = count1 + count2 + count5 + count10 + count20 + count50 + count100 + count200;
	std::cout << "\n\nTotal de moedas: " << totalMoedas << std::endl;

	// Zona Liberta memória 
    for (Blobs *current = listaC; current != NULL; current = current->next) {free(current);}
    for (Blobs *current = listaD; current != NULL; current = current->next) {free(current);}

	// Libertação de imagens
    vc_image_free(image0);
    vc_image_free(image1);
    vc_image_free(image2);
    vc_image_free(image3);
    vc_image_free(image5);
    vc_image_free(image6);
    // Fim Zona Liberta memória 


	cv::destroyAllWindows();// Fecha todas as janelas abertas
	capture.release();// Liberta o video

    std::cout << "Pressione qualquer tecla para sair...\n";
	std::cin.get();// Espera por uma tecla antes de sair

    return 0; // Termina o programa 
}


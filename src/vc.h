//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define VC_DEBUG


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    DEFINI��O DE MACROS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define MAX(a,b) ((a > b) ? a : b)
#define MIN(a,b) ((a < b) ? a : b)
#define MAX3(a,b,c) (a > b ? (a > c ? a : c) : (b > c ? b : c))
#define MIN3(a,b,c) (a < b ? (a < c ? a : c) : (b < c ? b : c))
#define SQUARE(a) ((a) * (a))


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			// Bin�rio/Cinzentos=1; RGB=3
	int levels;				// Bin�rio=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	int x, y, width, height;  // Bounding Box
	int area;  			 // �rea
	int xc, yc;  		 // Centro-de-massa
	int perimeter;  	 // Per�metro
	int label;  		 // Etiqueta

	unsigned char *mask;  // M�scara
	unsigned char *data;  // Dados
	int channels;  		 // Bin�rio/Cinzentos=1; RGB=3
	int levels;  		 // Bin�rio [0,1]; Cinzentos [0,255]; RGB [0,255]
} OVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROT�TIPOS DE FUN��ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);
int vc_image_copy(IVC *src, IVC *dst);

// FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);
int vc_count_white(IVC *src);
int vc_gray_negative(IVC *srcdst);
int vc_rgb_negative(IVC *srcdst);
int vc_rgb_get_red(IVC *srcdst);
int vc_rgb_get_green(IVC *srcdst);
int vc_rgb_get_blue(IVC *srcdst);
int vc_rgb_to_gray(IVC *src, IVC *dst);
int vc_rgb_to_hsv(IVC *src, IVC *dst);
int vc_hsv_segmentation(IVC *src, IVC *dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);
int vc_scale_gray_to_color_palette(IVC *src, IVC *dst);
int vc_gray_to_binary(IVC *src, IVC *dst, int threshold);
int vc_gray_to_binary_global_mean(IVC *src, IVC *dst);
int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel);
int vc_gray_to_binary_bernsen(IVC *src, IVC *dst, int kernel, int c);
int vc_gray_to_binary_niblack(IVC *src, IVC *dst, int kernel, float k);
int vc_binary_dilate(IVC *src, IVC *dst, int kernel);
int vc_binary_erode(IVC *src, IVC *dst, int kernel);
int vc_binary_open(IVC *src, IVC *dst, int kernel);
int vc_binary_open2(IVC *src, IVC *dst, int kernel, int kernel2);
int vc_binary_close(IVC *src, IVC *dst, int kernel);
int vc_binary_close2(IVC *src, IVC *dst, int kernel, int kernel2);
int vc_image_subtract(IVC *src1, IVC *src2, IVC *dst);
int vc_image_remove_mask(IVC *src, IVC *mask, IVC *dst);
int vc_gray_edge_prewitt(IVC *src, IVC *dst, int th);
int vc_gray_edge_sobel(IVC *src, IVC *dst, int th);
int vc_gray_lowpass_mean(IVC *src, IVC *dst, int kernelsize);
int vc_gray_lowpass_median(IVC *src, IVC *dst, int kernelsize);
int vc_gray_lowpass_gaussian_filter(IVC *src, IVC *dst);

// FUN��ES: BLOBS
OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels);
int vc_binary_blob_labelling_int(IVC *src, IVC* dst);
int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs);
int vc_label_coloring(IVC *src, IVC *dst, OVC *blobs, int nblobs);
int vc_label_coloring_rgb(IVC *src, IVC *dst, OVC *blobs, int nblobs);
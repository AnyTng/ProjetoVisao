//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
	}
	
	*t = 0;
	
	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);
				
				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;
	
	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}
		
		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}
	
	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}
		
		fclose(file);

		return 1;
	}
	
	return 0;
}

int vc_image_copy(IVC *src, IVC *dst)
{
	if(src == NULL || dst == NULL) return 0;
	if(src->width != dst->width || src->height != dst->height || src->channels != dst->channels) return 0;

	memcpy(dst->data, src->data, src->width * src->height * src->channels * sizeof(char));

	return 1;
}

int vc_image_channels_change(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int width = dst->width;
	int height = dst->height;
	int x = 0, y = 0, pos = 0, pos_dst = 0;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 3)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			datadst[pos_dst] = datasrc[pos];
			datadst[pos_dst + 1] = datasrc[pos];
			datadst[pos_dst + 2] = datasrc[pos];
		}
	}
}

int vc_count_white(IVC *src)
{
	unsigned char *data = (unsigned char *) src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x,y;
	long int pos;
	int count = 0;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if(channels != 1) return 0;

	//conta o número de pixeis brancos
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = y * bytesperline + x * channels;
			if(data[pos] == 255) count++;
		}
	}
	return count;
}

int vc_gray_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *) srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x,y;
	long int pos;

	//verificação de erros
	if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if(channels != 1) return 0;

	//inverte a imagem Gray

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
		}
	}
	return 1;
}

int vc_rgb_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *) srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x,y;
	long int pos;

	//verificação de erros
	if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if(channels != 3) return 0;

	//inverte a imagem RGB
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
			data[pos+1] = 255 - data[pos+1];
			data[pos+2] = 255 - data[pos+2];
		}
	}
	return 1;
}

int vc_rgb_get_red(IVC *srcdst)
{
	unsigned char *data = (unsigned char *) srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x,y;
	long int pos;

	//verificação de erros
	if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if(channels != 3) return 0;

	//extrai a componente Red

	for(y=0; y<height;y++)
	{
		for(x=0; x<width;x++)
		{
			pos = y* bytesperline + x* channels;
			data[pos] = 0;
			data[pos + 1] = data[pos + 1]; //Green 1
			data[pos + 2] = data[pos + 2]; //Blue 2
		}
	}
	return 1;
}

int vc_rgb_get_green(IVC *srcdst)
{
	unsigned char *data = (unsigned char *) srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x,y;
	long int pos;

	//verificação de erros
	if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if(channels != 3) return 0;

	//extrai a componente Green

	for(y=0; y<height;y++)
	{
		for(x=0; x<width;x++)
		{
			pos = y* bytesperline + x* channels;
			data[pos] = data[pos]; //Red 0
			data[pos + 1] = 0; //Green 1
			data[pos + 2] = data[pos + 2]; //Blue 2
		}
	}
	return 1;
}

int vc_rgb_get_blue(IVC *srcdst)
{
	unsigned char *data = (unsigned char *) srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x,y;
	long int pos;

	//verificação de erros
	if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if(channels != 3) return 0;

	//extrai a componente Blue

	for(y=0; y<height;y++)
	{
		for(x=0; x<width;x++)
		{
			pos = y* bytesperline + x* channels;
			data[pos] = data[pos]; //Red 0
			data[pos + 1] = data[pos + 1]; //Green 1
			data[pos + 2] = 0; //Blue 2
		}
	}
	return 1;
}

int vc_rgb_to_gray(IVC *src , IVC *dst)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	long int pos_src, pos_dst;
	float rf,gf,bf;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 3) || (dst->channels != 1)) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			rf = (float) datasrc[pos_src];
			gf = (float) datasrc[pos_src + 1];
			bf = (float) datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char) (0.299 * rf + 0.587 * gf + 0.114 * bf);
		}
	}
	return 1;
}

int vc_rgb_to_hsv(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	long int pos_src, pos_dst;
	float rf,gf,bf;
	float h,s,v;
	float min, max, delta;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 3) || (dst->channels != 3)) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			rf = (float) datasrc[pos_src];
			gf = (float) datasrc[pos_src + 1];
			bf = (float) datasrc[pos_src + 2];

			// Normaliza os valores RGB
			rf /= 255;
			gf /= 255;
			bf /= 255;

			// Calcula valores HSV
			min = MIN3(rf, gf, bf);
			max = MAX3(rf, gf, bf);

			delta = max - min;

			v = max;

			if (v == 0) 
				h, s = 0;
			else
			{
				s = delta / v;
				if (s == 0) h = 0;
				else
				{
					if (min == max) h = 0;
					else
					{
						if (max == rf && gf >= bf) h = 60 * (gf - bf) / delta;
						else if (max == rf && gf < bf) h = 360 + 60 * (gf - bf) / delta;
						else if (max == gf) h = 120 + 60 * (bf - rf) / delta;
						else if (max == bf) h = 240 + 60 * (rf - gf) / delta;
					}
				}
			}
			h = (h / 360) * 255;
			datadst[pos_dst] = (unsigned char) h;
			datadst[pos_dst + 1] = (unsigned char) (s*255);
			datadst[pos_dst + 2] = (unsigned char) (v*255);
		}
	}
	return 1;
}

int vc_rgb_to_bgr(IVC *srcdst)
{
	unsigned char *data = (unsigned char *) srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x,y;
	long int pos;

	if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if(channels != 3) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = data[pos + 2];
			data[pos + 1] = data[pos + 1];
			data[pos + 2] = data[pos];
		}
	}
	return 1;
}

int vc_bgr_to_rgb(IVC *srcdst)
{
	unsigned char *data = (unsigned char *) srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x,y;
	long int pos;

	if((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if(channels != 3) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = data[pos + 2];
			data[pos + 1] = data[pos + 1];
			data[pos + 2] = data[pos];
		}
	}
	return 1;
}

/*!
 * \brief Função que segmenta uma imagem em tons de cinzento.
 * \param src Imagem de entrada.
 * \param dst Imagem de saída.
 * \param hmin Matiz mínimo [0,360].
 * \param hmax Matiz máximo [0,360].
 * \param smin Saturação mínima [0,100].
 * \param smax Saturação máxima [0,100].
 * \param vmin Luminosidade mínima [0,100].
 * \param vmax Luminosidade máxima [0,100].
 * \return Retorna 1 se a segmentação foi realizada com sucesso e 0 caso contrário.
 */

int vc_hsv_segmentation(IVC *src, IVC *dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	long int pos_src, pos_dst;
	float h,s,v;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 3) || (dst->channels != 1)) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			h = (float) datasrc[pos_src];
			s = (float) datasrc[pos_src + 1];
			v = (float) datasrc[pos_src + 2];

			h = (h / 255) * 360;
			s = (s / 255) * 100;
			v = (v / 255) * 100;

			if((h >= hmin) && (h <= hmax) && (s >= smin) && (s <= smax) && (v >= vmin) && (v <= vmax))
			{
				datadst[pos_dst] = 255;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = 0;
				datadst[pos_dst + 2] = 0;
			}
		}
	}
	return 1;
}	

int vc_scale_gray_to_color_palette(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	long int pos_src, pos_dst;
	int i;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL)) return 0;
	if((src->channels != 1) || (dst->channels != 3)) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			i = (int) datasrc[pos_src];

			if ((i >= 0) && (i <= 63))
			{
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = i * 4;
				datadst[pos_dst + 2] = 255;
			}
			else if ((i >= 64) && (i <= 127))
			{
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 255-(i-64)*4;
			}
			else if ((i >= 128) && (i <= 191))
			{
				datadst[pos_dst] = (i-128)*4;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 0;
			}
			else if ((i >= 192) && (i <= 255))
			{
				datadst[pos_dst] = 255;
				datadst[pos_dst + 1] = 255-(i-192)*4;
				datadst[pos_dst + 2] = 0;
			}
		}
	}
	return 1;
}

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	long int pos_src, pos_dst;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 1) || (dst->channels != 1) || (dst->levels != 1)) return 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			datadst[pos_dst] = (datasrc[pos_src] > threshold) ? 255 : 0;
		}
	}
	return 1;
}

int vc_gray_to_binary_global_mean(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	long int pos_src, pos_dst;
	float media = 0;
	int totalpixels = width * height;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 1) || (dst->channels != 1) || (dst->levels != 1)) return 0;

	// Calcula a média global
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			media += (float) datasrc[pos_src];
		}
	}
	media = media / totalpixels;
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			datadst[pos_dst] = (datasrc[pos_src] > media) ? 255 : 0;
		}
	}
	return 1;
}

int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	int xx, yy;
	long int pos_src, pos_dst, posk;
	int max, min;
	int offset = (kernel - 1) / 2;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 1) || (dst->channels != 1) || (dst->levels != 1)) return 0;

	// Vizinhos
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			max = datasrc[pos_src];
			min = datasrc[pos_src];

			for(yy=-offset; yy<=offset; yy++)
			{
				for(xx=-offset; xx<=offset; xx++)
				{
					if((y+yy >= 0) && (y+yy < height) && (x+xx >= 0) && (x+xx < width))
					{
						posk = (y+yy) * bytesperline + (x+xx) * channels;
						if(datasrc[posk] > max) max = datasrc[posk];
						if(datasrc[posk] < min) min = datasrc[posk];
					}
				}
			}

			float threshold = (min + max) / 2;
			datadst[pos_dst] = (datasrc[pos_src] > threshold) ? 255 : 0;

		}
	}
	return 1;
}

int vc_gray_to_binary_bernsen(IVC *src, IVC *dst,int kernel, int c)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int levels = src->levels;
	unsigned char *datadst = (unsigned char *) dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channelsdst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x,y;
	int xx, yy;
	long int pos_src, pos_dst, posk;
	int max, min;
	int offset = (kernel - 1) / 2;
	float threshold;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 1) || (dst->channels != 1) || (dst->levels != 1)) return 0;

	// Vizinhos
	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos_src = y * bytesperline + x * channels;
			pos_dst = y * bytesperline_dst + x * channelsdst;

			max = datasrc[pos_src];
			min = datasrc[pos_src];

			for(yy = -offset; yy <= offset; yy++)
			{
				for(xx = -offset; xx <= offset; xx++)
				{
					if((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
					{
						posk = (y + yy) * bytesperline + (x + xx) * channels;
						if(datasrc[posk] > max) max = datasrc[posk];
						if(datasrc[posk] < min) min = datasrc[posk];
					}
				}
			}
			threshold = (max-min) < c ? levels / 2 : (max + min) / 2;
			datadst[pos_dst] = (datasrc[pos_src] > threshold) ? 255 : 0;
		}
	}
	return 1;
}

int vc_gray_to_binary_niblack(IVC *src, IVC *dst, int kernel, float k)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	unsigned char *datadst = (unsigned char *) dst->data;
	int height = src->height;
	int width = src->width;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x,y;
	int xx, yy;
	long int pos, posk;
	unsigned char threshold;
	int offset = (kernel - 1) / 2;
	int counter;
	float mean, sdeviation = 0.0f;

	//verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 1) || (dst->channels != 1) || (dst->levels != 1)) return 0;

	// Vizinhos
	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			mean = 0.0f;
			sdeviation = 0.0f;
			for(counter = 0,yy = -offset; yy <= offset; yy++)
			{
				for(xx=-offset; xx <= offset; xx++)
				{
					if((y + yy > 0) && (y + yy < height) && (x + xx > 0) && (x + xx < width))
					{	
						posk = (y + yy) * bytesperline + (x + xx) * channels;

						mean += (float)datasrc[posk];	
						counter++;
					}
					
				}
			}
		
			mean /= counter;

			for(counter = 0,yy = -offset; yy <= offset; yy++)
			{
				for(xx = -offset; xx <= offset; xx++)
				{
					if((y + yy > 0) && (y + yy < height) && (x + xx > 0) && (x + xx < width))
					{
						posk = (y + yy) * bytesperline + (x + xx) * channels;

						sdeviation += SQUARE((float)(datasrc[posk]) - mean); 
						counter++;
					}
				}
			}
			sdeviation = sqrt(sdeviation / counter);

			threshold = mean + k * sdeviation;
			
			datadst[pos] = (datasrc[pos] > threshold) ? 255 : 0;
		}
	}
}


int vc_binary_dilate(IVC *src, IVC *dst, int kernel)
{
    unsigned char *datasrc = (unsigned char *) src->data;
    unsigned char *datadst = (unsigned char *) dst->data;
    int height = src->height;
    int width = src->width;
    int bytesperline = src->width * src->channels;
    int channels = src->channels;
    int x, y;
    int xx, yy;
    long int pos, posk;
    int offset = (kernel - 1) / 2;
    int max;

    // Verificação de erros
    if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if((src->width != dst->width) || (src->height != dst->height)) return 0;
    if((src->channels != 1) || (dst->channels != 1)) return 0;

    // Dilatação
    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;

            max = 0;

            for(yy = -offset; yy <= offset; yy++)
            {
                for(xx = -offset; xx <= offset; xx++)
                {
                    if((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
                    {
                        posk = (y + yy) * bytesperline + (x + xx) * channels;

                        if(datasrc[posk] > max) max = datasrc[posk];
                    }
                }
            }

            datadst[pos] = max;
        }
    }

    return 1;
}

int vc_binary_erode(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	unsigned char *datadst = (unsigned char *) dst->data;
	int height = src->height;
	int width = src->width;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	int xx, yy;
	long int pos, posk;
	int offset = (kernel - 1) / 2;
	int min;

	// Verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 1) || (dst->channels != 1)) return 0;

	// Erosão
	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			min = 255;

			for(yy = -offset; yy <= offset; yy++)
			{
				for(xx = -offset; xx <= offset; xx++)
				{
					if((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
					{
						posk = (y + yy) * bytesperline + (x + xx) * channels;

						if(datasrc[posk] < min) min = datasrc[posk];
					}
				}
			}

			datadst[pos] = min;
		}
	}

	return 1;
}

int vc_binary_close(IVC *src, IVC *dst, int kernel)
{
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if(tmp == NULL) return 0;

	if(!vc_binary_dilate(src, tmp, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	if(!vc_binary_erode(tmp, dst, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);

	return 1;
}

int vc_binary_close2(IVC *src, IVC *dst, int kernel, int kernel2)
{
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if(tmp == NULL) return 0;

	if(!vc_binary_dilate(src, tmp, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	if(!vc_binary_erode(tmp, dst, kernel2))
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);

	return 1;
}

int vc_binary_open(IVC *src, IVC *dst, int kernel)
{
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if(tmp == NULL) return 0;

	if(!vc_binary_erode(src, tmp, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	if(!vc_binary_dilate(tmp, dst, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);

	return 1;
}

int vc_binary_open2(IVC *src, IVC *dst, int kernel, int kernel2)
{
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if(tmp == NULL) return 0;

	if(!vc_binary_erode(src, tmp, kernel))
	{
		vc_image_free(tmp);
		return 0;
	}

	if(!vc_binary_dilate(tmp, dst, kernel2))
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);

	return 1;
}

int vc_image_subtract(IVC *src1, IVC *src2, IVC *dst)
{
	unsigned char *datasrc1 = (unsigned char *) src1->data;
	unsigned char *datasrc2 = (unsigned char *) src2->data;
	unsigned char *datadst = (unsigned char *) dst->data;
	int height = src1->height;
	int width = src1->width;
	int bytesperline = src1->width * src1->channels;
	int channels = src1->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if((src1->width <= 0) || (src1->height <= 0) || (src1->data == NULL)) return 0;
	if((src2->width != src1->width) || (src2->height != src1->height) || (src2->channels != src1->channels)) return 0;
	if((dst->width != src1->width) || (dst->height != src1->height) || (dst->channels != src1->channels)) return 0;

	// Subtração
	for(y = 0; y < height; y++)
	{
		for(x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			datadst[pos] = datasrc1[pos] - datasrc2[pos];
		}
	}

	return 1;
}

int vc_image_remove_mask(IVC *src, IVC *mask, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *) src->data;
	unsigned char *datamask = (unsigned char *) mask->data;
	unsigned char *datadst = (unsigned char *) dst->data;
	int height = src->height;
	int width = src->width;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((mask->width != src->width) || (mask->height != src->height) || (mask->channels != 1)) return 0;
	if((dst->width != src->width) || (dst->height != src->height) || (dst->channels != src->channels)) return 0;

	// Remoção
	for (int i = 0; i < src->height * src->width; i++)
    {
        if (datamask[i] == 0)
        {
            datadst[i] = 0;
        }
    }

	return 1;
}


// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC *blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i<size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y<height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x<width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a<label - 1; a++)
	{
		for (b = a + 1; b<label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a<label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a<(*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}

int vc_binary_blob_labelling_int(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD;
	int label = 1;

	if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if((src->width != dst->width) || (src->height != dst->height)) return 0;
	if((src->channels != 1) || (dst->channels != src->channels)) return 0;

	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			posX = y * bytesperline + x;
			posA = (y-1) * bytesperline + (x-1);
			posB = (y-1) * bytesperline + x;
			posC = (y-1) * bytesperline + (x+1);
			posD = y * bytesperline + (x-1);
			if (datasrc[posX] == 255) {
				if ((y - 1) >= 0 && (x - 1) >= 0 && (y + 1) < height && (x + 1) < width) {
					if (datasrc[posA] == 0 && datasrc[posB] == 0 && datasrc[posC] == 0 && datasrc[posD] == 0) {
						datadst[posX] = label;
						label++;
					} else {
						int dataA = datadst[posA] == 0 ? 255 : (int)datadst[posA];
						int dataB = datadst[posB] == 0 ? 255 : (int)datadst[posB];
						int dataC = datadst[posC] == 0 ? 255 : (int)datadst[posC];
						int dataD = datadst[posD] == 0 ? 255 : (int)datadst[posD];
						int menor = dataA;
						int aux = MIN3(dataB,dataC,dataD);
						menor = MIN(aux, menor);
						datadst[posX] = menor;
					}
				} else datadst[posX] = 0;
			} else datadst[posX] = 0;

		}
	}
}

int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta área de cada blob
	for (i = 0; i<nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y<height - 1; y++)
		{
			for (x = 1; x<width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_label_coloring(IVC *src, IVC *dst, OVC *blobs, int nblobs)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	// Copia imagem para a imagem destino
	memcpy(datadst, datasrc, bytesperline * height);

	// Verifica cada blob
	for (i = 0; i<nblobs; i++)
	{

		// Pinta o blob de uma cor (random) na imagem destino
		unsigned char color = (rand() % 255);

		for (y = 0; y<height; y++)
		{
			for (x = 0; x<width; x++)
			{
				pos = y * bytesperline + x * channels;

				if (datadst[pos] == blobs[i].label)
				{
					datadst[pos] = color;
				}
			}
		}
		printf("\nLabel %d : Cor %d", blobs[i].label, color);

	}

	return 1;
}

int vc_label_coloring_rgb(IVC *src, IVC *dst, OVC *blobs, int nblobs)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int dst_bytesperline = dst->bytesperline;
	int channels = src->channels;
	int dst_channels = dst->channels;
	int x, y, i;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if (channels != 1) return 0;

	printf("\nSRC Channels %d", channels);
	printf("\nDST Channels %d", dst_channels);
	printf("\nSRC Bytes per line %d", bytesperline);
	printf("\nDST Bytes per line %d", dst_bytesperline);
	printf("\nWidth %d", width);
	printf("\nHeight %d", height);
	printf("\nDST Width %d", dst->width);
	printf("\nDST Height %d", dst->height);
	printf("\nNBlobs %d", nblobs);

	// Copia imagem para a imagem destino
	memcpy(datadst, datasrc, bytesperline * height);

	// Verifica cada blob
	for (i = 0; i<nblobs; i++)
	{

		// Pinta o blob de uma cor (random) na imagem destino
		unsigned char color[3] = { (rand() % 255), (rand() % 255), (rand() % 255) };

		for (y = 0; y<height; y++)
		{
			for (x = 0; x<width; x++)
			{
				pos = y * bytesperline + x * channels;

				if (datadst[pos] == blobs[i].label)
				{
					datadst[pos] = color[0];
					datadst[pos + 1] = color[1];
					datadst[pos + 2] = color[2];
				}
			}
		}
		printf("\nLabel %d : Cor %d %d %d", blobs[i].label, color[0], color[1], color[2]);

	}

	return 1;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: HISTOGRAMAS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


IVC* vc_histogram(IVC* src)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, max;
	int pixels[256];
	int pos,dpos;

	max = 0;

	for (int i = 0; i<256; i++)
	{
		pixels[i]=0;
	}
	

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for(x=0; x<width; x++)
	{
		for(y=0; y<height; y++)
		{
			pos = y * bytesperline + x * channels;
			pixels[datasrc[pos]] += 1;
		}
	}

	for (int i = 0; i<256; i++)
	{
		max = MAX(pixels[i],max);
	}

	IVC* dst = vc_image_new(256,max,1,1);
	unsigned char *datadst = (unsigned char *)dst->data;

	for(x=0; x < dst->width; x++)
	{
		for (y = 0; y < max; y++)
		{
			dpos = y * dst->bytesperline + x * dst->channels;
			if(y <= pixels[x])
				datadst[dpos] = 0;
			else
				datadst[dpos] = 1;
		}
	}
	
	return dst;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: FILTRAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_gray_edge_prewitt(IVC *src, IVC *dst, int th) // th = [0, 255]
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH = 0;
	int sumx, sumy = 0;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			// PosA PosB PosC
			// PosD PosX PosE
			// PosF PosG PosH

			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			// PosA*(-1) PosB*0 PosC*(1)
			// PosD*(-1) PosX*0 PosE*(1)
			// PosF*(-1) PosG*0 PosH*(1)

			//sumx = (datasrc[posC] + datasrc[posE] + datasrc[posH]) - (datasrc[posA] + datasrc[posD] + datasrc[posF]);

			sumx = datasrc[posA] * (-1);
			sumx += datasrc[posD] * (-1);
			sumx += datasrc[posF] * (-1);

			sumx += datasrc[posC] * (1);
			sumx += datasrc[posE] * (1);
			sumx += datasrc[posH] * (1);

			sumx = (int)(sumx / 3);
			
			// PosA*(-1) PosB*(-1) PosC*(-1)
			// PosD*0    PosX*0    PosE*0
			// PosF*(1)  PosG*(1)  PosH*(1)

			//sumy = (datasrc[posF] + datasrc[posG] + datasrc[posH]) - (datasrc[posA] + datasrc[posB] + datasrc[posC]);

			sumy = datasrc[posA] * (-1);
			sumy += datasrc[posB] * (-1);
			sumy += datasrc[posC] * (-1);

			sumy += datasrc[posF] * (1);
			sumy += datasrc[posG] * (1);
			sumy += datasrc[posH] * (1);

			sumy = (int)(sumy / 3);

			float magn = sqrt(SQUARE(sumx) + SQUARE(sumy));
			if (magn >= th) datadst[posX] = 255;
			else datadst[posX] = 0;
		}
	}
	return 1;
}

int vc_gray_lowpass_mean(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	int offset = (kernelsize - 1) / 2;
	int xx, yy;
	long int pos, posk;
	int counter;
	float mean;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			mean = 0.0f;
			counter = 0;

			for (yy = -offset; yy <= offset; yy++)
			{
				for (xx = -offset; xx <= offset; xx++)
				{
					if ((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
					{
						posk = (y + yy) * bytesperline + (x + xx) * channels;

						mean += (float)datasrc[posk];
						counter++;
					}
				}
			}

			mean /= counter;
			datadst[pos] = (unsigned char)mean;
		}
	}
}


int vc_gray_lowpass_median(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	int offset = (kernelsize - 1) / 2;
	int xx, yy;
	long int pos, posk;
	int counter;
	unsigned char *values;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;
	if (kernelsize % 2 == 0) return 0; // o kernel deve ser ímpar
	values = (unsigned char *)malloc(kernelsize * kernelsize * sizeof(unsigned char));
	if (values == NULL) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			counter = 0;

			for (yy = -offset; yy <= offset; yy++)
			{
				for (xx = -offset; xx <= offset; xx++)
				{
					if ((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
					{
						posk = (y + yy) * bytesperline + (x + xx) * channels;
						values[counter] = datasrc[posk];
						counter++;
					}
				}
			}

			for (int i = 0; i < counter - 1; i++)
			{
				for (int j = i + 1; j < counter; j++)
				{
					if (values[i] > values[j])
					{
						unsigned char k = values[i];
						values[i] = values[j];
						values[j] = k;
					}
				}
			}

			unsigned char median = values[counter / 2];
			datadst[pos] = median;
		}
	}
	free(values);
}

int vc_gray_lowpass_gaussian_filter(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int pos, posk;
	int kernel[5][5] = {
		{ 1,  4,  7,  4, 1 },
		{ 4, 16, 26, 16, 4 },
		{ 7, 26, 41, 26, 7 },
		{ 4, 16, 26, 16, 4 },
		{ 1,  4,  7,  4, 1 }
	};

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			int sum = 0;

			for (int j = -2; j <= 2; j++)
			{
				for (int i = -2; i <= 2; i++)
				{
					if ((y + j >= 0) && (y + j < height) && (x + i >= 0) && (x + i < width))
					{
						posk = (y + j) * bytesperline + (x + i) * channels;
						sum += datasrc[posk] * kernel[j + 2][i + 2];
					}
				}
			}
			datadst[pos] = (unsigned char)(sum / 273);
		}
	}
}

//Função que aplica um filtro passa-alto a uma imagem grayscale
//src: Imagem de entrada
//dst: Imagem de saída
//type: Tipo de filtro a aplicar (1, 2 ou 3)
int vc_gray_highpass_filter(IVC *src, IVC *dst, int type)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, xx, yy;
	long int pos, posk;
	int i, j;
	int kernel1[3][3] = {
		{  0, -1,  0 },
		{ -1,  4, -1 },
		{  0, -1,  0 }
	};
	int kernel2[3][3] = {
		{ -1, -1, -1 },
		{ -1,  8, -1 },
		{ -1, -1, -1 }
	};
	int kernel3[3][3] = {
		{ -1, -2, -1 },
		{ -2, 12, -2 },
		{ -1, -2, -1 }
	};
	int sum = 0;
	int k;
	int offset = 1;


	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels != 1) return 0;

	// Filtro passa-alto
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			pos = y * bytesperline + x * channels;

			for (yy = 0; yy <= 2; yy++)
			{
				for (xx = 0; xx <= 2; xx++)
				{
					if ((y + yy >= 0) && (y + yy < height) && (x + xx >= 0) && (x + xx < width))
					{
						posk = (y + yy - 1) * bytesperline + (x + xx - 1) * channels;
						switch (type)
						{
							case 1:
								sum += datasrc[posk] * kernel1[yy][xx];
								k = 6;
								break;
							case 2:
								sum += datasrc[posk] * kernel2[yy][xx];
								k = 9;
								break;
							case 3:
								sum += datasrc[posk] * kernel3[yy][xx];
								k = 16;
								break;
						}
					}

				}
			}

			datadst[pos] = (unsigned char)((float)abs(sum) / (float)k);	
			sum = 0;
		}
	}

}
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

typedef enum {
        EXIF_BYTE       = 0x01,  
        EXIF_ASCII      = 0x02,  
        EXIF_SHORT      = 0x03,  
        EXIF_LONG       = 0x04,  
        EXIF_RATIONAL   = 0x05,  
        EXIF_SBYTE      = 0x06,  
        EXIF_UNDEFINED  = 0x07,  
        EXIF_SSHORT     = 0x08,  
        EXIF_SLONG      = 0x09,  
        EXIF_SRATIONAL  = 0x10,  
        EXIF_FLOAT      = 0x11,  
        EXIF_DOUBLE     = 0x12   
} ExifFormats;

typedef enum {
        EXIF_MAKE = (0x01<<8) | 0x0f,
        EXIF_MODEL = (0x01<<8) | 0x10,
        EXIF_EXPOSURE = (0x82<<8) | 0x9a,
        EXIF_FNUMBER = (0x82<<8) | 0x9d,
        EXIF_ISOSPEEDRATING = (0x82<<8) | 0x9d,
        EXIF_USERCOMMENT = (0x92<<8) | 0x86,
        EXIF_GPSLATITUDE = (0x00<<8) | 0x02,
        EXIF_GPSLONGITUDE = (0x00<<8) | 0x04,
        EXIF_GPSLATITUDEREF = (0x00<<8) | 0x01,
        EXIF_GPSLONGITUDEREF = (0x00<<8) | 0x03,
        EXIF_DATETIME = (0x01<<8) | 0x32,
        EXIF_IMAGEDESCRIPTION = (0x01<<8) | 0x0e
} ExifTags;

typedef struct {
        ExifFormats format;
        unsigned char* data;
        u_int32_t dataLen;
        int isSigned;
} ExifData;

typedef struct {
        char* exifName;
        u_int32_t exifNameLen;
        ExifTags tagType;
        ExifData* tagData;  
} ExifInfo;


/*

        ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

*/

int writeHelpMessage(char* execName) {
	printf("Использование: %s {--update, --add, --delete, --read, --help} [{Опции}]\n", execName);
	printf("Опции: \n");
	printf("--filename <имя_файла> - Название файла с которым будет проводиться работа \n");
        printf("{--atime, --ctime, --mtime} ГГГГ-ММ-ДД ЧЧ:мм:сс - Изменение системных меток(если нет флага оставляет прошлые)-H\n");
        printf("\t atime - Дата последнего доступа \n");
        printf("\t ctime - Дата изменения метаданных \n");
        printf("\t mtime - Дата последнего изменения \n");
	printf("--header <Заголовок> - Заголовок метаданных (комментария) при изменении, удалении и добавлении \n");
	printf("--data <Данные> - Метаданные(комментарий), которые будут добавлены или на которые будет произведена подмена\n");
        printf("\nPNG\n");
        printf("\t--width <Пиксели> - Ширина в пикселях(только в режиме --add и --update)\n");
        printf("\t--height <Пиксели> - Высота в пикселях(только в режиме --add и --update)\n");
        printf("\t--horizontal <Число> - Горизонтальное разрешение(только в режиме --add и --update)\n");
        printf("\t--vertical <Число> - Вертикальное разрешение(только в режиме --add и --update)\n");
        printf("\t--measure {0,1} - Единица измерения разрешения, 0 - соотношение сторон, 1 - метры(только в режиме --add и --update)\n");
        printf("\nJPEG\n");
        printf("\tВ разработке\n");
        printf("\nGIF\n");
        printf("\tВ разработке\n");
        printf("\nTIF\n");
        printf("\tВ разработке\n");
        return 1;
}

int getSystemMarks(int argc, char** argv, char* flag, char* buffer) {
        for (int i = 2; i < argc; i++ ) {
                if (strcmp(argv[i],flag) == 0) {
                        
                        if (argc <= i+2 || strcmp(argv[i+1],"") == 0 || strcmp(argv[i+2],"") == 0) {
                                printf("Ошибка: Не указано значение метаданных \n");
                                writeHelpMessage(argv[0]);
                                return 1;
                        }
                        snprintf(buffer, 19, "%s %s", argv[i + 1], argv[i + 2]);
                        return 0;               
                }
        }
        return 1;
}

unsigned int crc32b(unsigned char *message, int len) {
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   int counter = 0;
   while (counter < len) {
      byte = message[i];         
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {  
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
      counter++;
   }
   return ~crc;
}

int isValidTag(ExifTags tag) {
        if (tag!=EXIF_MAKE && tag!=EXIF_MODEL && tag!=EXIF_EXPOSURE && tag!=EXIF_FNUMBER && tag!=EXIF_ISOSPEEDRATING && tag!=EXIF_GPSLATITUDE && tag!=EXIF_GPSLONGITUDE && tag !=EXIF_GPSLATITUDEREF && tag!=EXIF_GPSLONGITUDEREF && tag!=EXIF_DATETIME && tag!=EXIF_IMAGEDESCRIPTION) {
                return 0;
        }
        return 1;
}

/*

	МОДУЛЬ ЧТЕНИЯ

*/

int readMetadataPNG(FILE* fp_in) {
    unsigned char lastByte;
    unsigned char currentByte;
    if (!fp_in) {
        return 1;
    }
    while (fread(&currentByte, 1, 1, fp_in) == 1){
        if (currentByte == 0x74){
            unsigned char bytes[3] = {0};
            fread(bytes, 3 ,1, fp_in);
            if (bytes[0]!=0x45 || bytes[1]!=0x58 || bytes[2] != 0x74){
                fseek(fp_in, -3, SEEK_CUR);
            } else {
                fseek(fp_in, -8, SEEK_CUR);
                unsigned char lengthBytes[4] = {0};
                fread(lengthBytes,4,1,fp_in);
                fseek(fp_in, 4, SEEK_CUR);
		u_int32_t remain = (lengthBytes[0] << 24) | (lengthBytes[1] << 16) | (lengthBytes[2] << 8) | lengthBytes[3];
		u_int32_t tempRemain = remain;
		unsigned char* metaData = (unsigned char*) malloc((remain + 1) * sizeof(char));
		while (remain>0 && fp_in) {
		    unsigned char metaDataByte;
		    fread(&metaDataByte, 1, 1, fp_in);
		    if (metaDataByte == 0x00) {
			metaData[tempRemain-remain] = ':';
			remain--;
			continue;
		    }
		    metaData[tempRemain-remain] = metaDataByte;
		    remain--;
		}
		metaData[tempRemain+1] = '\0';
		for (int i = 0; i < tempRemain; i++){
    		    printf("%c",metaData[i]);
		}
		printf("\n");
		free(metaData);
	    }
        } else if (currentByte == 0x49){
	    unsigned char bytes[3] = {0};
	    fread(bytes, 3 ,1, fp_in);
            if (bytes[0]==0x44 || bytes[1]==0x41 || bytes[2] == 0x54){
		break;
            }
            if (bytes[0] == 0x48 && bytes[1] == 0x44 && bytes[2] == 0x52){
                unsigned char widthBytes[4] = {0};
                fread(widthBytes, 4 ,1, fp_in);
                u_int32_t width = (u_int32_t)((widthBytes[0] << 24) |
                                 (widthBytes[1] << 16) |
                                 (widthBytes[2] << 8)  |
                                 (widthBytes[3]));
                printf("Ширина изображения: %u\n",width);
                unsigned char heightBytes[4] = {0};
                fread(heightBytes, 4 ,1, fp_in);
                u_int32_t height = (u_int32_t)((heightBytes[0] << 24) |
                                 (heightBytes[1] << 16) |
                                 (heightBytes[2] << 8)  |
                                 (heightBytes[3]));
                
                printf("Высота изображения: %u\n",height);
                unsigned char depth = 0;
                fread(&depth, 1 ,1, fp_in);
                u_int8_t dephtInt = (u_int8_t)(depth);
                printf("Глубина цвета: %u\n",dephtInt);

                unsigned char colorType = 0;
                fread(&colorType, 1 ,1, fp_in);
                switch (colorType) {
                        case 0x00:
                                printf("Цветовой тип: Оттенки серого\n");
                                break;
                        case 0x02:
                                printf("Цветовой тип: RGB\n");
                                break;
                        case 0x03:
                                printf("Цветовой тип: Палитровые цвета\n");
                                break;
                        case 0x04:
                                printf("Цветовой тип: Оттенки серого с альфа-каналом\n");
                                break;
                        case 0x06:
                                printf("Цветовой тип: RGB с альфа-каналом\n");
                                break;
                        default:
                               printf("Цветовой тип: Неизвестно\n"); 
                               break;

                }


                unsigned char compressionType = 0;
                fread(&compressionType, 1 ,1, fp_in);
                switch (compressionType) {
                        case 0x00:
                                printf("Тип сжатия: Deflate самый быстрый\n");
                                break;
                        case 0x01:
                                printf("Тип сжатия: Deflate быстрый\n");
                                break;
                        case 0x02:
                                printf("Тип сжатия: Deflate по умолчанию\n");
                                break;
                        case 0x03:
                                printf("Тип сжатия: Deflate максимальное сжатие\n");
                                break;
                        default:
                               printf("Тип сжатия: Неизвестно\n"); 
                               break;
                }

                unsigned char filterMethod = 0;
                fread(&filterMethod, 1 ,1, fp_in);
                switch (filterMethod) {
                        case 0x00:
                                printf("Метод фильрации: Нет\n");
                                break;
                        case 0x01:
                                printf("Метод фильрации: Вычитание левого(Sub)\n");
                                break;
                        case 0x02:
                                printf("Метод фильрации: Вычитание верхнего(Up)\n");
                                break;
                        case 0x03:
                                printf("Метод фильрации: Вычитание среднего(Average)\n");
                                break;
                        case 0x04:
                                printf("Метод фильрации: алгоритм Paeth\n");
                                break;
                        default:
                               printf("Метод фильтрации: Неизвестно\n"); 
                               break;
                }

                unsigned char interlace = 0;
                fread(&interlace, 1 ,1, fp_in);
                switch (interlace) {
                        case 0x00:
                                printf("Interlace: Нет\n");
                                break;
                        case 0x01:
                                printf("Interlace: Adam7\n");
                                break;
                        default:
                               printf("Interlace: Неизвестно\n"); 
                               break;
                }
                continue;
            }
	    fseek(fp_in, -3, SEEK_CUR);
	} else if (currentByte == 0x70) {
            unsigned char bytes[3] = {0};
	    fread(bytes, 3 ,1, fp_in);
            if (bytes[0]==0x48 || bytes[1]==59 || bytes[2] == 0x73){
                unsigned char horizontalBytes[4] = {0};
                fread(horizontalBytes, 4 ,1, fp_in);
                u_int32_t horizontal = (u_int32_t)((horizontalBytes[0] << 24) |
                                 (horizontalBytes[1] << 16) |
                                 (horizontalBytes[2] << 8)  |
                                 (horizontalBytes[3]));
                printf("Горизонтальное разрешение: %u\n",horizontal);
                unsigned char verticalBytes[4] = {0};
                fread(verticalBytes, 4 ,1, fp_in);
                u_int32_t vertical = (u_int32_t)((verticalBytes[0] << 24) |
                                 (verticalBytes[1] << 16) |
                                 (verticalBytes[2] << 8)  |
                                 (verticalBytes[3]));
                printf("Вертикальное разрешение: %u\n",vertical);
                unsigned char measure = 0;
                fread(&measure, 1 ,1, fp_in);

                switch (measure){
                        case 0:
                                printf("Единицы измерения: Не указано(предположительно соотношение сторон)\n");
                                break;
                        case 1:
                                printf("Единицы измерения: Метры\n");
                                break;
                        default:
                                printf("Единицы измерения: Неизвестно\n");
                                break;
                }
                continue;
            }
            fseek(fp_in, -3, SEEK_CUR);
        }
	lastByte = currentByte;
    }
    return 0;
}

ExifData* parseExifField(FILE* fp_in, long startTIFF) {
        unsigned char type = 0x00;
        fseek(fp_in,1,SEEK_CUR);
        int result = fread(&type,1,1,fp_in);
        if (result<1 || !fp_in) {
                return NULL;
        }
        if (type==0x00 || type>0x12) {
                return NULL;
        }
        unsigned char countBytes[4] = {0x00,0x00,0x00,0x00};
        result = fread(countBytes,1,4,fp_in);
        if (result < 4 || !fp_in) {
                return NULL;
        }
        ExifFormats format = (ExifFormats)type;
        u_int32_t count = (countBytes[0]<<24) | (countBytes[1]<<16) | (countBytes[2]<<8) | countBytes[3];
        u_int32_t countMultiple = 0;
        int isSigned = 0;
        switch (format) {
                case EXIF_BYTE:
                        countMultiple = 1;
                        break;
                case EXIF_ASCII:
                        countMultiple = 1;
                        break;
                case EXIF_SHORT:
                        countMultiple = 2;
                        break;
                case EXIF_LONG:
                        countMultiple = 4;
                        break;
                case EXIF_RATIONAL:
                        countMultiple = 8;
                        break;
                case EXIF_SBYTE:
                        countMultiple = 1;
                        isSigned = 1;
                        break;
                case EXIF_UNDEFINED:
                        countMultiple = 1;
                        break;
                case EXIF_SSHORT:
                        countMultiple = 2;
                        isSigned = 1;
                        break;
                case EXIF_SLONG:
                        countMultiple = 4;
                        isSigned = 1;
                        break;
                case EXIF_SRATIONAL:
                        countMultiple = 8;
                        isSigned = 1;
                        break;
                case EXIF_FLOAT:
                        countMultiple = 4;
                        isSigned = 1;
                        break;
                case EXIF_DOUBLE:
                        countMultiple = 8;
                        isSigned = 1;
                        break;
        }
        u_int32_t bytesForRead = count * countMultiple;
        ExifData tagData;
        tagData.format = format;
        tagData.isSigned = isSigned;

        if (bytesForRead <= 4){
                tagData.data = (unsigned char*)malloc(4);
                tagData.dataLen = 4;
                result = fread(tagData.data,1,4,fp_in);
                if (result!=4 || ! fp_in) {
                        free(tagData.data);
                        return NULL;
                }
                return &tagData;
        }

        unsigned char offsetBytes[4] = {0x00,0x00,0x00,0x00};
        result = fread(offsetBytes,1,4,fp_in);
        if (result != 4 || !fp_in) {
                return NULL;
        }
        u_int32_t offset = (offsetBytes[0]<<24) | (offsetBytes[1]<<16) | (offsetBytes[2]<<8) | offsetBytes[3];
        long currentOffset = ftell(fp_in);
        tagData.data = (unsigned char*)malloc(bytesForRead);
        tagData.dataLen = bytesForRead;
        fseek(fp_in, startTIFF+offset, SEEK_SET);

        u_int32_t readResult = fread(tagData.data, 1, tagData.dataLen, fp_in);
        if (!fp_in) {
                free(tagData.data);
                return NULL;
        }
        fseek(fp_in,currentOffset,SEEK_SET);
        if (readResult!=tagData.dataLen) {
                free(tagData.data);
                return NULL;
        }
        return &tagData;
}

int parseJPEGAPPTag(FILE* fp_in, u_int16_t length) {
        long tiffStart = ftell(fp_in);
        unsigned char currentBytes[2];
        u_int32_t result = 0;
        for (long counter = 0; counter < (long)length; counter++) {
                result = fread(currentBytes,1,2,fp_in);
                if (result!=2 || !fp_in) {
                        continue;
                }
                ExifTags tag = (ExifTags)((currentBytes[0]<<8)|currentBytes[1]);
                if (isValidTag(tag) == 0){
                        fseek(fp_in,-1,SEEK_CUR);
                        continue;
                }
        }
        //unsigned char currentByte = 0x00;
        //if (!fp_in){
        //        return 1;
        //}
	//double longitude[3] = {-200.0, -200.0, -200.0};
	//double latitude[3] = {-200.0, -200.0, -200.0};
	//int isLongitudeEast = 0;
	//int isLatitudeSouth = 0;
	//int result = 0;
        //for (long counter = 0; counter < (long)length; counter++){
	//	result = fread(&currentByte,1,1,fp_in);
	//	if (result<=0 || !fp_in){
	//		break;
	//	}
	//	//ImageDescription
	//	if (currentByte == 0x01){
	//		counter++;
	//		fread(&currentByte,1,1,fp_in);
	//		//0x0e - imDescr, 0x0f - camera maker, 0x10 - cameraModel, 0x32 - DateTime
	//		if (currentByte != 0x0e && currentByte!=0x0f && currentByte!=0x10 && currentByte!=0x32){
	//			fseek(fp_in,-1,SEEK_CUR);
	//			counter--;
	//			continue;
	//		}
	//		int isCamera = 0;
	//		if (currentByte == 0x0f){
	//			isCamera = 1;
	//		}
	//		if (currentByte == 0x10){
	//			isCamera = -1;
	//		}
	//		if (currentByte == 0x32) {
	//			isCamera = 2;
	//		}
	//		counter+=2;
	//		fseek(fp_in,2,SEEK_CUR);
	//		unsigned char tagLengthBytes[4] = {0};
	//		result = fread(tagLengthBytes,1,4,fp_in);
	//		if (result!=4 || !fp_in){
	//			fseek(fp_in,-4,SEEK_CUR);
	//			continue;
	//		}
	//		counter+=4;
	//		u_int32_t tagLength = (tagLengthBytes[0]<<24) | (tagLengthBytes[1]<<16) | (tagLengthBytes[2]<<8)|tagLengthBytes[3];
	//		unsigned char tagShiftBytes[4] = {0};
	//		result = fread(tagShiftBytes,1,4,fp_in);
	//		if (result!=4 || !fp_in){
	//			fseek(fp_in,-4,SEEK_CUR);
	//			continue;
	//		}
	//		counter+=4;
	//		u_int32_t tagShift = (tagShiftBytes[0]<<24) | (tagShiftBytes[0]<<16) | (tagShiftBytes[2]<<8) | tagShiftBytes[3];
	//		long curPos = ftell(fp_in);
	//		
	//		fseek(fp_in,tiffStart,SEEK_SET);
	//		fseek(fp_in,tagShift,SEEK_CUR);
//
	//		unsigned char* imageDescription = (unsigned char*)malloc(tagLength+1);
	//		fread(imageDescription,1,tagLength,fp_in);
	//		imageDescription[tagLength] = '\0';
	//		if (isCamera == 0) {
	//			printf("Описание изображения: %s\n",imageDescription);
	//			printf("Описание изображения в байтах:");
	//		} else if (isCamera == 1) {
	//			printf("Производитель: %s\n",imageDescription);
	//			printf("Производитель в байтах:");
	//		} else if (isCamera == -1) {
	//			printf("Модель: %s\n",imageDescription);
	//			printf("Модель в байтах:");
	//		} else if (isCamera == 2) {
	//			printf("Дата и время: %s\n",imageDescription);
	//			printf("Дата и время в байтах:");
	//		}
	//		for (int i = 0; i < tagLength-1; i++) {
	//			printf(" %02x", imageDescription[i]);
	//		}
	//		printf("\n");
	//		free(imageDescription);
//
	//		fseek(fp_in,curPos,SEEK_SET);
	//		continue;
	//	}
//
	//	//UserComment
	//	if (currentByte=0x92){
	//		counter++;
	//		fread(&currentByte,1,1,fp_in);
	//		if (currentByte != 0x86){
	//			fseek(fp_in,-1,SEEK_CUR);
	//			counter--;
	//			continue;
	//		}
	//		counter+=2;
	//		fseek(fp_in,2,SEEK_CUR);
	//		unsigned char tagLengthBytes[4] = {0};
	//		result = fread(tagLengthBytes,1,4,fp_in);
	//		if (result!=4 || !fp_in){
	//			fseek(fp_in,-4,SEEK_CUR);
	//			continue;
	//		}
	//		counter+=4;
	//		u_int32_t tagLength = (tagLengthBytes[0]<<24) | (tagLengthBytes[1]<<16) | (tagLengthBytes[2]<<8)|tagLengthBytes[3];
	//		unsigned char tagShiftBytes[4] = {0};
	//		result = fread(tagShiftBytes,1,4,fp_in);
	//		if (result!=4 || !fp_in){
	//			fseek(fp_in,-4,SEEK_CUR);
	//			continue;
	//		}
	//		counter+=4;
	//		u_int32_t tagShift = (tagShiftBytes[0]<<24) | (tagShiftBytes[0]<<16) | (tagShiftBytes[2]<<8) | tagShiftBytes[3];
	//		long curPos = ftell(fp_in);
	//		
	//		fseek(fp_in,tiffStart,SEEK_SET);
	//		fseek(fp_in,tagShift,SEEK_CUR);
//
	//		unsigned char* userComment = (unsigned char*)malloc(tagLength+1-8);
	//		unsigned char commentType[8];
	//		fread(commentType,1,8,fp_in);
	//		commentType[8] = '\0';
	//		fread(userComment,1,tagLength-8,fp_in);
	//		printf("Формат пользовательского комментария: %s\n", commentType);
	//		printf("Пользовательский комментарий: %s\n",userComment);
	//		printf("Пользовательский комментарий в байтах:");
	//		for (int i = 0; i < tagLength-8; i++) {
	//			printf(" %02x", userComment[i]);
	//		}
	//		printf("\n");
	//		free(userComment);
//
	//		fseek(fp_in,curPos,SEEK_SET);
	//		continue;
	//	}
//
	//	//ExposureTime и Aperture
	//	if (currentByte == 0x82) {
	//		counter++;
	//		fread(&currentByte,1,1,fp_in);
	//		//0x9a - exposure, 0x9d - aperture
	//		if (currentByte != 0x9a && currentByte != 0x9d){
	//			fseek(fp_in,-1,SEEK_CUR);
	//			counter--;
	//			continue;
	//		}
	//		int isExposure = 1;
	//		if (currentByte == 0x9d) {
	//			isExposure = 0;
	//		}
	//		counter+=2;
	//		fseek(fp_in,2,SEEK_CUR);
	//		unsigned char tagLengthBytes[4] = {0};
	//		result = fread(tagLengthBytes,1,4,fp_in);
	//		if (result!=4 || !fp_in){
	//			fseek(fp_in,-4,SEEK_CUR);
	//			continue;
	//		}
	//		counter+=4;
	//		u_int32_t tagLength = (tagLengthBytes[0]<<24) | (tagLengthBytes[1]<<16) | (tagLengthBytes[2]<<8)|tagLengthBytes[3];
	//		unsigned char tagShiftBytes[4] = {0};
	//		result = fread(tagShiftBytes,1,4,fp_in);
	//		if (result!=4 || !fp_in){
	//			fseek(fp_in,-4,SEEK_CUR);
	//			continue;
	//		}
	//		counter+=4;
	//		u_int32_t tagShift = (tagShiftBytes[0]<<24) | (tagShiftBytes[0]<<16) | (tagShiftBytes[2]<<8) | tagShiftBytes[3];
	//		long curPos = ftell(fp_in);
	//		
	//		fseek(fp_in,tiffStart,SEEK_SET);
	//		fseek(fp_in,tagShift,SEEK_CUR);
//
	//		unsigned char exposureTimeBytes[8];
	//		fread(exposureTimeBytes,1,8,fp_in);
//
	//		u_int32_t numerator = (exposureTimeBytes[0]<<24) | (exposureTimeBytes[1]<<16) | (exposureTimeBytes[2]<<8) | exposureTimeBytes[3];
	//		u_int32_t denominator = (exposureTimeBytes[4]<<24) | (exposureTimeBytes[5]<<16) | (exposureTimeBytes[6]<<8) | exposureTimeBytes[7];
	//		if (isExposure == 1) {
	//			printf("Время экспозиции: %.2f секунд\n",numerator/(denominator*1.0));
	//		} else if (isExposure == 0) {
	//			printf("Апертура: %.2f\n",numerator/(denominator*1.0));
	//		}
//
	//		free(exposureTimeBytes);
//
	//		fseek(fp_in,curPos,SEEK_SET);
	//		continue;
	//	}
	//	if (currentByte == 0x88){
	//		result = fread(&currentByte,1,1,fp_in);
	//		if (!fp_in || result != 1) {
	//			continue;
	//		}
	//		if (currentByte!=0x27) {
	//			fseek(fp_in,-1,SEEK_CUR);
	//			continue;
	//		}
	//		fseek(fp_in,6,SEEK_CUR);
	//		unsigned char isoFormatBytes[4] = {0x00};
	//		result = fread(isoFormatBytes,1,4,fp_in);
	//		if (!fp_in || result!=4) {
	//			continue;
	//		}
	//		u_int32_t isoFormat = (isoFormatBytes[0]<<24) | (isoFormatBytes[1]<<16) | (isoFormatBytes[2]<<8) | isoFormatBytes[3];
	//		printf("ISO чувствительность: ISO %d\n",isoFormat);
	//		continue;
	//	}
//
	//	//Gelocation
	//	if (currentByte == 0x00){
	//		counter++;
	//		fread(&currentByte,1,1,fp_in);
	//		//longitude and latitude
	//		if (currentByte == 0x02 || currentByte != 0x04) {
	//			int isLat = 0;
	//			if (currentByte == 0x02) {
	//				isLat = 1;
	//			}
	//			counter+=2;
	//			fseek(fp_in,2,SEEK_CUR);
	//			unsigned char tagLengthBytes[4] = {0};
	//			result = fread(tagLengthBytes,1,4,fp_in);
	//			if (result!=4 || !fp_in){
	//				fseek(fp_in,-4,SEEK_CUR);
	//				continue;
	//			}
	//			counter+=4;
	//			u_int32_t tagLength = (tagLengthBytes[0]<<24) | (tagLengthBytes[1]<<16) | (tagLengthBytes[2]<<8)|tagLengthBytes[3];
	//			unsigned char tagShiftBytes[4] = {0};
	//			result = fread(tagShiftBytes,1,4,fp_in);
	//			if (result!=4 || !fp_in){
	//				fseek(fp_in,-4,SEEK_CUR);
	//				continue;
	//			}
	//			counter+=4;
	//			u_int32_t tagShift = (tagShiftBytes[0]<<24) | (tagShiftBytes[0]<<16) | (tagShiftBytes[2]<<8) | tagShiftBytes[3];
	//			long curPos = ftell(fp_in);
//
	//			fseek(fp_in,tiffStart,SEEK_SET);
	//			fseek(fp_in,tagShift,SEEK_CUR);
//
	//			unsigned char coordinatesBytes[24] = {0x00};
	//			fread(coordinatesBytes,1,24,fp_in);
//
	//			u_int32_t numeratorGrad = (coordinatesBytes[0]<<24) | (coordinatesBytes[1]<<16) | (coordinatesBytes[2]<<8) | coordinatesBytes[3];
	//			u_int32_t denominatorGrad = (coordinatesBytes[4]<<24) | (coordinatesBytes[5]<<16) | (coordinatesBytes[6]<<8) | coordinatesBytes[7];
	//			u_int32_t numeratorMin = (coordinatesBytes[8]<<24) | (coordinatesBytes[9]<<16) | (coordinatesBytes[10]<<8) | coordinatesBytes[11];
	//			u_int32_t denominatorMin = (coordinatesBytes[12]<<24) | (coordinatesBytes[13]<<16) | (coordinatesBytes[14]<<8) | coordinatesBytes[15];
	//			u_int32_t numeratorSec = (coordinatesBytes[16]<<24) | (coordinatesBytes[17]<<16) | (coordinatesBytes[18]<<8) | coordinatesBytes[19];
	//			u_int32_t denominatorSec = (coordinatesBytes[20]<<24) | (coordinatesBytes[21]<<16) | (coordinatesBytes[22]<<8) | coordinatesBytes[23];
	//			
	//			if (isLat == 1) {
	//				latitude[0] = numeratorGrad/(denominatorGrad*1.0);
	//				latitude[1] = numeratorMin/(denominatorMin*1.0);
	//				latitude[2] = numeratorSec/(denominatorSec*1.0);
	//			} else {
	//				longitude[0] = numeratorGrad/(denominatorGrad*1.0);
	//				longitude[1] = numeratorMin/(denominatorMin*1.0);
	//				longitude[2] = numeratorSec/(denominatorSec*1.0);
	//			}
	//			
	//			fseek(fp_in,curPos,SEEK_SET);
	//			continue;	
	//		}
	//		//refs
	//		if (currentByte == 0x01 || currentByte == 0x03) {
	//			int isLat = 0;
	//			if (currentByte == 0x01) {
	//				isLat = 1;
	//			}
	//			counter+=2;
	//			unsigned char tagFormat[2];
	//			fread(tagFormat,1,2,fp_in);
	//			if (tagFormat[1] == 0x03 || tagFormat[1] == 0x04){
	//				unsigned char tagLengthBytes[4] = {0};
	//				result = fread(tagLengthBytes,1,4,fp_in);
	//				if (result!=4 || !fp_in){
	//					fseek(fp_in,-4,SEEK_CUR);
	//					continue;
	//				}
	//				unsigned char value[4];
	//				result = fread(value,1,4,fp_in);
	//				if (result!=4 || !fp_in){
	//					fseek(fp_in,-4,SEEK_CUR);
	//					continue;
	//				}
	//				if (isLat) {
	//					if (value[3] == 0x4e) {
	//						isLatitudeSouth = -1;
	//					} else {
	//						isLatitudeSouth = 1;
	//					}
	//				} else {
	//					if (value[3] == 0x57) {
	//						isLongitudeEast = -1;
	//					} else {
	//						isLongitudeEast = 1;
	//					}
	//				}
	//				continue;
	//			}
	//			unsigned char tagLengthBytes[4] = {0};
	//			result = fread(tagLengthBytes,1,4,fp_in);
	//			if (result!=4 || !fp_in){
	//				fseek(fp_in,-4,SEEK_CUR);
	//				continue;
	//			}
	//			counter+=4;
	//			u_int32_t tagLength = (tagLengthBytes[0]<<24) | (tagLengthBytes[1]<<16) | (tagLengthBytes[2]<<8)|tagLengthBytes[3];
	//			unsigned char tagShiftBytes[4] = {0};
	//			result = fread(tagShiftBytes,1,4,fp_in);
	//			if (result!=4 || !fp_in){
	//				fseek(fp_in,-4,SEEK_CUR);
	//				continue;
	//			}
	//			counter+=4;
	//			u_int32_t tagShift = (tagShiftBytes[0]<<24) | (tagShiftBytes[0]<<16) | (tagShiftBytes[2]<<8) | tagShiftBytes[3];
	//			long curPos = ftell(fp_in);
//
	//			fseek(fp_in,tiffStart,SEEK_SET);
	//			fseek(fp_in,tagShift,SEEK_CUR);
//
	//			unsigned char refBute = 0x00;
	//			fread(&refBute,1,1,fp_in);
	//			if (isLat) {
	//				if (refBute == 0x4e) {
	//					isLatitudeSouth = -1;
	//				} else {
	//					isLatitudeSouth = 1;
	//				}
	//			} else {
	//				if (refBute == 0x57) {
	//					isLongitudeEast = -1;
	//				} else {
	//					isLongitudeEast = 1;
	//				}
	//			}
//
	//			fseek(fp_in,curPos,SEEK_SET);
	//			
	//			continue;
	//		}
	//		fseek(fp_in,-1,SEEK_CUR);
	//		counter--;
	//		continue;
	//		
	//	}
	//	
        //}
//
	//if (longitude[0]!=-200.0 || longitude[1]!=-200.0 || longitude[2]!=-200.0) {
	//	printf("Долгота: %.0f градусов %.0f минут %.0f секунд",longitude[0],longitude[1],longitude[2]);
	//	if (isLongitudeEast == 1) {
	//		printf(" восточной долготы");
	//	} else if (isLongitudeEast == -1) {
	//		printf(" западной долготы");
	//	}
	//	printf("\n");
	//}
	//if (latitude[0]!=-200.0 || latitude[1]!=-200.0 || latitude[2]!=-200.0) {
	//	printf("Широта: %.0f градусов %.0f минут %.0f секунд",latitude[0],latitude[1],latitude[2]);
	//	if (isLatitudeSouth == 1) {
	//		printf(" южной широты");
	//	} else if (isLatitudeSouth == -1) {
	//		printf(" северной широты");
	//	}
	//	printf("\n");
	//}
        //return 0;
}

int readMetadataJPEG(FILE* fp_in) {
        unsigned char currentByte = 0x00;
        if (!fp_in) {
                return 1;
        }
        while(fread(&currentByte,1,1,fp_in) == 1) {
                if (currentByte == 0x4a) {
                        unsigned char bytesFIF[3] = {0};
                        int readRes = fread(bytesFIF,1,3,fp_in);
                        if (readRes!=3) {
                                continue;
                        }
                        if (bytesFIF[0]!=0x46 || bytesFIF[1]!=0x49 || bytesFIF[2]!=0x46) {
                                fseek(fp_in,-3, SEEK_CUR);
                                continue;
                        }
                        unsigned char infoBytes[7] = {0};
                        readRes = fread(infoBytes,1,7,fp_in);
                        if (readRes!=7) {
                                continue;
                        }
                        u_int16_t jfifVersion = (infoBytes[0]<<8) | infoBytes[1];
                        printf("Версия JFIF: 1.%02d\n",jfifVersion);
                        switch (infoBytes[2]) {
                                case 0x00:
                                        printf("Единица измерения разрешения: Нет\n");
                                        break;
                                case 0x01:
                                        printf("Единица измерения разрешения: Точки на дюйм\n");
                                        break;
                                case 0x02:
                                        printf("Единица измерения разрешения: Точки на сантиметр\n");
                                        break;
                                default:
                                        printf("Единица измерения разрешения: Неизвестно\n");
                                        break;
                        }
                        u_int16_t horizontalResolution = (infoBytes[3]<<8) | infoBytes[4];
                        u_int16_t verticalResolution = (infoBytes[5]<<8) | infoBytes[6];
                        printf("Горизонтальное разрешение: %d\n", horizontalResolution);
                        printf("Вертикальное разрешение: %d\n", verticalResolution);
                        continue;
                }
                if (currentByte != 0xff) {
                        continue;
                }
                fread(&currentByte,1,1,fp_in);
                if (currentByte == 0xe1){
                        unsigned char appTagLenBytes[2] = {0};
                        fread(appTagLenBytes,1,2,fp_in);
                        u_int16_t appTagLen = (appTagLenBytes[0]<<8) | appTagLenBytes[1];
                        appTagLen = appTagLen-2;
                        unsigned char exifBytes[4] = {0};
                        int result = fread(exifBytes,1,4,fp_in);
                        if (exifBytes[0]!=0x45 || exifBytes[1]!=0x78 || exifBytes[2]!=0x69 || exifBytes[3]!=0x66){
                                continue;
                        }
                        fseek(fp_in,2,SEEK_CUR);
                        parseJPEGAPPTag(fp_in,appTagLen-4-2);
                        continue;
                }
                if (currentByte == 0xfe) {
                        unsigned char lenghtBytes[2] = {0x00};
                        int result = fread(lenghtBytes,1,2,fp_in);
                        if (result!=2) {
                                continue;
                        }
                        u_int16_t length = (lenghtBytes[0]<<8) | lenghtBytes[1];
                        length = length - 2;
                        unsigned char* comment = (unsigned char*)malloc(length+1);
                        fread(comment,1,length,fp_in);
                        comment[length] = '\0';
                        printf("Найден комментарий: %s\n",comment);
                        free(comment);
                }
                if (currentByte == 0xc0) {
                        printf("Тип маркера: SOF0 (Baseline DCT)\n");
                } else if (currentByte == 0xc1) {
                        printf("Тип маркера: SOF1 (Extended Sequential DCT)\n");
                } else if (currentByte == 0xc2) {
                        printf("Тип маркера: SOF2 (Progressive DCT)\n");
                } else if (currentByte == 0xc3) {
                        printf("Тип маркера: SOF3 (Lossless)\n");
                } else {
                        fseek(fp_in,-1,SEEK_CUR);
                        continue;
                }
                fseek(fp_in,2,SEEK_CUR);
                unsigned char imageInfo[5] = {0};
                int readResult = fread(imageInfo,1,5,fp_in);
                if (readResult != 5) {
                        continue;
                }
                u_int8_t accuracy = imageInfo[0];
                u_int16_t height = (imageInfo[1] << 8) | imageInfo[2];
                u_int16_t width = (imageInfo[3] << 8) | imageInfo[4];
                printf("Точность: %d бит на компонент\n", accuracy);
                printf("Ширина изображения: %d\n", width);
                printf("Высота изображения: %d\n", height);
        }
        return 0;
}

int readMetadata(char* filename, int argc, char** argv){
    printf("Системная информация о файле\n");
    printf("----------------------------\n");
    struct stat fileInfo;
    char atimeBufferInput[20];
    char ctimeBufferInput[20];
    char mtimeBufferInput[20];
    int changeAtime = getSystemMarks(argc, argv, "--atime",atimeBufferInput);
    int changeCtime = getSystemMarks(argc, argv, "--ctime",ctimeBufferInput);
    int changeMtime = getSystemMarks(argc, argv, "--mtime",mtimeBufferInput);
    if (stat(filename, &fileInfo) == 0) {
        printf("Размер: %ld Байт\n", fileInfo.st_size);
        char* absolutePath = realpath(filename, NULL);
        if (absolutePath != NULL) {
            printf("Местоположение: %s\n", absolutePath);
            free(absolutePath);
        }
        
        char buffer[20];

        time_t mtime = fileInfo.st_mtime;
        struct tm *tm_info_m = localtime(&mtime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_m);
        
        printf("Дата изменения: %s\n", buffer);
        if (changeMtime != 0) {
            strcpy(mtimeBufferInput, buffer);
        }
        time_t atime = fileInfo.st_atime;
        struct tm *tm_info_a = localtime(&atime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_a);

        printf("Дата последнего доступа: %s\n", buffer);
        if (changeAtime != 0) {
            strcpy(atimeBufferInput, buffer);
        }

        time_t ctime = fileInfo.st_ctime;
        struct tm *tm_info_c = localtime(&ctime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_c);
        
        printf("Дата изменения метаданных: %s\n", buffer);
        if (changeCtime != 0) {
            strcpy(ctimeBufferInput, buffer);
        }

        printf("Права доступа: %o\n", fileInfo.st_mode & 0777);
    } else {
        printf("Не удалось получить системную информацию.\n");
    }
    printf("\n");
    FILE* fp_in = fopen(filename,"rb");
    if (!fp_in) {
        printf("Ошибка открытия файла\n");
        return 1;
    }
    unsigned char headerBytes[4] = {0};
    fread(headerBytes, 1, 4, fp_in);
    if (headerBytes[0] == 0x89 && headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
        printf("\nВнутренние данные файла\n");
        printf("-----------------------\n");
        printf("Тип файла: PNG \n");
        printf("MIME Тип: image/png\n");
        readMetadataPNG(fp_in);
    } else if (headerBytes[0] == 0xff && headerBytes[1] == 0xd8 && headerBytes[2] == 0xff && headerBytes[3] == 0xe0) {
        printf("\nВнутренние данные файла\n");
        printf("-----------------------\n");
        printf("Тип файла: JPEG \n");
        printf("MIME Тип: image/jpeg\n");
        readMetadataJPEG(fp_in);
    } else {
        printf("Неподдерживаемый формат файла\n");
        fclose(fp_in);
    }
    fclose(fp_in);
    
    struct timespec new_times[2];
    struct timespec current_time;
    if (clock_gettime(CLOCK_REALTIME, &current_time)) {
        printf("\nОшибка при получении текущего системного времени\n");
        return 1;
    }
    
    struct tm tm_atime = {0}, tm_ctime = {0}, tm_mtime = {0};
    strptime(atimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_atime);
    strptime(ctimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_ctime);
    strptime(mtimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_mtime);


    struct timespec new_system_time;
    new_system_time.tv_sec = mktime(&tm_ctime);
    new_system_time.tv_nsec = 0;
    if (clock_settime(CLOCK_REALTIME, &new_system_time)) {
        perror("\nОшибка при установке системного времени\n");
        return 1;
    }
    
    new_times[0].tv_sec = mktime(&tm_atime); 
    new_times[1].tv_sec = mktime(&tm_mtime);  
    
    if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
        printf("\nОшибка при изменении временных меток файла\n");
        return 1;
    }
    printf("\nВременные метки файла успешно изменены.\n");
    
    if (clock_settime(CLOCK_REALTIME, &current_time)) {
        perror("Ошибка при восстановлении системного времени");
        return 1;
    }

    return 0;
}

/*

	МОДУЛЬ УДАЛЕНИЯ

*/
int deleteMetadataPNG(FILE* fp_in, char* header, char* filename){
        int foundMetadata = -1;
        unsigned char currentByte = 0x00;
        fseek(fp_in, 0, SEEK_SET);
        printf("Копирование исходного файла в %s_delete_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_delete_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_delete_copy");
        
        FILE* fp_out = fopen(new_filename, "wb");
        if (!fp_out) {
                printf("Ошибка открытия файла для резервного копирования\n");
                free(new_filename);
                return 1;
        }
        while(fread(&currentByte, 1, 1, fp_in) == 1){
                fwrite(&currentByte, 1, 1, fp_out);
        }
        fclose(fp_out);
        fclose(fp_in);
        FILE* fp_copy = fopen(new_filename, "rb");
        FILE* fp_delete = fopen(filename, "wb");
        if (!fp_copy || !fp_delete) {
                perror("Ошибка открытия файлов для удаления\n");
                if(fp_copy){
                        fclose(fp_copy);
                }
                if(fp_delete){
                        fclose(fp_delete);
                }
                return 1;
        }
        int headerLen = strlen(header);
        unsigned char currentBytes[8] = {0x00};
        int result = 0;
        currentByte = 0x00;
        while(1){
                result = fread(currentBytes,1, 8, fp_copy);
                if (result == 0){
                        break;
                }
                if (result<8){
                        fwrite(currentBytes,result, 1, fp_delete);
                        break;
                }
                if (currentBytes[4]!=0x74){
                        fwrite(currentBytes, 1, 1,fp_delete);
                        fseek(fp_copy,-7,SEEK_CUR);
                        continue;
                }
                if (currentBytes[5]!=0x45 || currentBytes[6]!=0x58 || currentBytes[7]!=0x74){
                        fwrite(currentBytes, 1, 1,fp_delete);
                        fseek(fp_copy,-7,SEEK_CUR);
                        continue;
                }
                u_int32_t length = (currentBytes[3] << 0)  |
                                   (currentBytes[2] << 8)  |
                                   (currentBytes[1] << 16) |
                                   (currentBytes[0] << 24);
                fread(&currentByte,1,1,fp_copy);
                if (currentByte != header[0]){
                        fwrite(currentBytes,1,1,fp_delete);
                        fseek(fp_copy, -1-4-3,SEEK_CUR);
                        continue;
                }
                int currentTab = 0;
                int thisHeader = 1;
                for (int i = 1; i < headerLen; i++) {
                        fread(&currentByte, 1, 1, fp_copy);
                        currentTab++;
                        if (currentByte != header[i]){
                                thisHeader = -1;
                                break;
                        }
                }
                if (thisHeader == -1) {
                        fwrite(currentBytes,1,1,fp_delete);
                        fseek(fp_copy, -currentTab-1-4-3,SEEK_CUR);
                        continue;
                }
                fread(&currentByte, 1, 1, fp_copy);
                fseek(fp_copy,-1,SEEK_CUR);
                if (currentByte != 0x00) {
                        fwrite(currentBytes,1,1,fp_delete);
                        fseek(fp_copy, -currentTab-1-4-3,SEEK_CUR);
                        continue;  
                }
                // 00 00 00 24 t E X t h e a d e r
                unsigned char fileMetaLen[4] = {0};
                fseek(fp_copy,-currentTab-1-4-4, SEEK_CUR);
                //4 - длина, 4 - tEXt, 4- CRC32
                fseek(fp_copy, 4+4+4+length, SEEK_CUR);
                foundMetadata = 1;
        }
        fclose(fp_copy);
        fclose(fp_delete);
        if (foundMetadata == -1) {
                printf("Метаданные с таки заголовком не найдены!");
        } else {
                printf("Успешно!");
        }
        return 1;
}

int deleteMetadata(char* filename, char* header, int argc, char** argv) {
        struct stat fileInfo;
        char atimeBufferInput[20];
        char ctimeBufferInput[20];
        char mtimeBufferInput[20];
        int changeAtime = getSystemMarks(argc, argv, "--atime",atimeBufferInput);
        int changeCtime = getSystemMarks(argc, argv, "--ctime",ctimeBufferInput);
        int changeMtime = getSystemMarks(argc, argv, "--mtime",mtimeBufferInput);
        if (stat(filename, &fileInfo) == 0) {
                char buffer[20];

                time_t mtime = fileInfo.st_mtime;
                struct tm *tm_info_m = localtime(&mtime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_m);
                if (changeMtime != 0) {
                    strcpy(mtimeBufferInput, buffer);
                }
                time_t atime = fileInfo.st_atime;
                struct tm *tm_info_a = localtime(&atime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_a);

                if (changeAtime != 0) {
                    strcpy(atimeBufferInput, buffer);
                }

                time_t ctime = fileInfo.st_ctime;
                struct tm *tm_info_c = localtime(&ctime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_c);

                if (changeCtime != 0) {
                    strcpy(ctimeBufferInput, buffer);
                }
        }
        FILE* fp_in = fopen(filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
        unsigned char headerBytes[4] = {0};
        fread(headerBytes, 1, 4, fp_in);
        if (headerBytes[0] == 0x89 && headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
                int result = deleteMetadataPNG(fp_in, header, filename);
        } else {
                printf("Неподдерживаемый формат файла\n");
                fclose(fp_in);
        }
        struct timespec new_times[2];
        struct timespec current_time;
        if (clock_gettime(CLOCK_REALTIME, &current_time)) {
                printf("\nОшибка при получении текущего системного времени\n");
                return 1;
        }
    
        struct tm tm_atime = {0}, tm_ctime = {0}, tm_mtime = {0};
        strptime(atimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_atime);
        strptime(ctimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_ctime);
        strptime(mtimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_mtime);


        struct timespec new_system_time;
        new_system_time.tv_sec = mktime(&tm_ctime);
        new_system_time.tv_nsec = 0;
        if (clock_settime(CLOCK_REALTIME, &new_system_time)) {
            perror("\nОшибка при установке системного времени\n");
            return 1;
        }
        
        new_times[0].tv_sec = mktime(&tm_atime); 
        new_times[1].tv_sec = mktime(&tm_mtime);  
        
        if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
            printf("\nОшибка при изменении временных меток файла\n");
            return 1;
        }
        printf("\nВременные метки файла успешно изменены.\n");
        
        if (clock_settime(CLOCK_REALTIME, &current_time)) {
            perror("Ошибка при восстановлении системного времени");
            return 1;
        }

        return 0;
}


/*

	МОДУЛЬ ДОБАВЛЕНИЯ

*/
int getArgumentSize(int argc, char** argv, char* flag) {
        for (int i = 2; i < argc; i++ ) {
                if (strcmp(argv[i],flag) == 0) {
                        
                        if (argc <= i+1 || strcmp(argv[i+1],"") == 0) {
                                printf("Ошибка: Не указано значение метаданных \n");
                                writeHelpMessage(argv[0]);
                                return -1;
                        }
                        
                        if (strcmp(argv[i+1],"0") == 0){
                                return 0;
                        }
                        int result = atoi(argv[i+1]);
                        if (result == 0){
                                return -1;
                        }
                        return result;
                }
        }
        return -1;
}

int addMetadataPNG(FILE* fp_in, char* header, char* data, char* filename, int argc, char** argv) {
        printf("Изменение заголовка PNG\n");
        printf("-----------------------\n");
        int width = getArgumentSize(argc, argv, "--width");
        int height = getArgumentSize(argc, argv, "--height");
        
        int widthChanged = 1;
        int heightChanged = 1;
        int depthChanged = 1;
        int colorTypeChanged = 1;
        int compressionChanged = 1;
        int filterChanged = 1;
        int interlaceChanged = 1;

        if (width < 0) {
                printf("Изменение ширины не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                widthChanged = -1;
        }
        if (height < 0){
                printf("Изменение высоты не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                heightChanged = -1;
        }
        unsigned char oldHeader[17] = {0};
        int headerFound = 0;
        if (fp_in) {
                unsigned char currentByte = 0x00;
                while (fread(&currentByte,1,1,fp_in) == 1){
                        if (currentByte == 0x49) {
                                unsigned char bytesHDR[3] = {0};
                                fread(bytesHDR,3,1,fp_in);
                                if (bytesHDR[0]!=0x48 || bytesHDR[1]!=0x44 || bytesHDR[2]!=0x52){
                                        fseek(fp_in, -3, SEEK_CUR);
                                        continue;
                                }
                                headerFound = 1;
                                fread(oldHeader,17,1,fp_in);
                                break;
                        }
                }
        }
        if (headerFound == 0) {
                printf("Ошибка: У файла не найден IHDR\n");
                fclose(fp_in);
                return 1;
        }
        unsigned char newHeader[17] = {0};
        newHeader[0] = 0x49;
        newHeader[1] = 0x48;
        newHeader[2] = 0x44;
        newHeader[3] = 0x52;
        if (widthChanged==-1 && heightChanged==-1 && depthChanged==-1 && colorTypeChanged==-1 && compressionChanged==-1 && filterChanged==-1 && interlaceChanged==-1){
                printf("Итог изменения заголовка: Нечего изменять\n\n");
        } else{
                printf("Итог изменения заголовка: Данные изменения заголовка принять в обработку\n\n");
        }
        if (widthChanged!=-1) {
                newHeader[4] = (width >> 24) & 0xFF; 
                newHeader[5] = (width >> 16) & 0xFF; 
                newHeader[6] = (width >> 8)  & 0xFF; 
                newHeader[7] = width & 0xFF;
        } else {
                newHeader[4] = oldHeader[0];
                newHeader[5] = oldHeader[1];
                newHeader[6] = oldHeader[2];
                newHeader[7] = oldHeader[3];
        }
        if (heightChanged!=-1) {
                newHeader[8] = (height >> 24) & 0xFF; 
                newHeader[9] = (height >> 16) & 0xFF; 
                newHeader[10] = (height >> 8)  & 0xFF; 
                newHeader[11] = height & 0xFF; 
        } else {
                newHeader[8] =  oldHeader[0];
                newHeader[9] =  oldHeader[1];
                newHeader[10] =  oldHeader[2];
                newHeader[11] =  oldHeader[3];
        }
        
        newHeader[12] = oldHeader[8];
        newHeader[13] = oldHeader[9];
        newHeader[14] = oldHeader[10];
        newHeader[15] = oldHeader[11];
        newHeader[16] = oldHeader[12];
       
        unsigned int newCRC32 = crc32b(newHeader, 17);
        unsigned char newCRC32CharIHDR[4] = {0};
        newCRC32CharIHDR[0] = (newCRC32 >> 24) & 0xFF; 
        newCRC32CharIHDR[1] = (newCRC32 >> 16) & 0xFF; 
        newCRC32CharIHDR[2] = (newCRC32 >> 8)  & 0xFF; 
        newCRC32CharIHDR[3] = newCRC32 & 0xFF;

        printf("Изменение PHYS PNG\n");
        printf("------------------\n");
        int horizontalResolution = getArgumentSize(argc, argv, "--horizontal");
        int verticalResolution = getArgumentSize(argc, argv, "--vertical");
        int measure = getArgumentSize(argc,argv,"--measure");
        if (horizontalResolution == -1) {
                printf("Изменение горизонтального разрешения не было произведено(указаны некорректные параметры или параметры не указаны)\n");
        }
        if (verticalResolution == -1) {
                printf("Изменение вертикального разрешения не было произведено(указаны некорректные параметры или параметры не указаны)\n");                
        }
        if (measure!=0 && measure!=1){
                printf("Изменение меры измерения разрещения не было произведено(указаны некорректные параметры или параметры не указаны)\n");
        }
        if (horizontalResolution == -1 && verticalResolution == -1 && measure!=0 && measure!=1) {
                printf("Итог изменения PHYS: Нечего изменть\n\n");
        } else {
                printf("Итог изменения PHYS: Данные приняты в обработку\n\n");
        }
        
        int physFound = -1;
        unsigned char oldPhys[13] = {0};
        if (fp_in) {
                unsigned char currentByte = 0x00;
                while (fread(&currentByte,1,1,fp_in) == 1){
                        if (currentByte == 0x70) {
                                unsigned char bytesHDR[3] = {0};
                                fread(bytesHDR,3,1,fp_in);
                                if (bytesHDR[0]!=0x48 || bytesHDR[1]!=0x59 || bytesHDR[2]!=0x73){
                                        fseek(fp_in, -3, SEEK_CUR);
                                        continue;
                                }
                                physFound = 1;
                                fread(oldPhys,13,1,fp_in);
                                break;
                        } 
                }
        }
        
        unsigned char newPhys[13] = {0};
        newPhys[0] = 0x70;
        newPhys[1] = 0x48;
        newPhys[2] = 0x59;
        newPhys[3] = 0x73;
        if (horizontalResolution != -1) {
                newPhys[4] = (horizontalResolution >> 24) & 0xFF; 
                newPhys[5] = (horizontalResolution >> 16) & 0xFF; 
                newPhys[6] = (horizontalResolution >> 8)  & 0xFF; 
                newPhys[7] = horizontalResolution & 0xFF;
        } else {
                newPhys[4] = oldPhys[0];
                newPhys[5] = oldPhys[1];
                newPhys[6] = oldPhys[2];
                newPhys[7] = oldPhys[3];
        }
        if (verticalResolution != -1) {
                newPhys[8] = (verticalResolution >> 24) & 0xFF; 
                newPhys[9] = (verticalResolution >> 16) & 0xFF; 
                newPhys[10] = (verticalResolution >> 8)  & 0xFF; 
                newPhys[11] = verticalResolution & 0xFF; 
        } else {
                newPhys[8] =  oldPhys[4];
                newPhys[9] =  oldPhys[5];
                newPhys[10] =  oldPhys[6];
                newPhys[11] =  oldPhys[7];
        }
        if (measure != -1) {
                newPhys[12] = (unsigned char)measure;
        } else {
                newPhys[12] = oldPhys[8];
        }
        newCRC32 = crc32b(newPhys, 13);
        unsigned char newCRC32CharPHYS[4] = {0};
        newCRC32CharPHYS[0] = (newCRC32 >> 24) & 0xFF; 
        newCRC32CharPHYS[1] = (newCRC32 >> 16) & 0xFF; 
        newCRC32CharPHYS[2] = (newCRC32 >> 8)  & 0xFF; 
        newCRC32CharPHYS[3] = newCRC32 & 0xFF;
        if (physFound == -1&&!(horizontalResolution == -1 && verticalResolution == -1 && measure!=0 && measure!=1)) {
                printf("Ошибка: У файла не найден PHYS\n\n");
        }

        fclose(fp_in);

        printf("Копирование исходного файла в %s_add_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_add_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_add_copy");

        FILE* fp_in_copy = fopen(filename, "rb");
        FILE* fp_out_copy = fopen(new_filename, "wb");
        
        if (fp_in_copy && fp_out_copy) {
                char currentByte = 0x00;
                while (fread(&currentByte, 1, 1, fp_in_copy) == 1) {
                        fwrite(&currentByte, 1, 1, fp_out_copy);
                }
        }

        if (fp_in_copy) fclose(fp_in_copy);
        if (fp_out_copy) fclose(fp_out_copy);   

        printf("Процесс изменения исходного файла:\n");
        FILE* fp_out = fopen(filename, "wb");
        FILE* fp_in_copied = fopen(new_filename, "rb");
        if (fp_in_copied && fp_out) {
             unsigned char currentByte = 0x00;
             while (fread(&currentByte, 1, 1, fp_in_copied) == 1) {
                if (currentByte == 0x49) {
                        unsigned char bytesHDR[3] = {0};
                        fread(bytesHDR,3,1,fp_in_copied);
                        if (bytesHDR[0]!=0x48 || bytesHDR[1]!=0x44 || bytesHDR[2]!=0x52){
                                fseek(fp_in_copied, -3, SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        if (!(widthChanged==-1 && heightChanged==-1 && depthChanged==-1 && colorTypeChanged==-1 && compressionChanged==-1 && filterChanged==-1 && interlaceChanged==-1)){
                                fwrite(newHeader,17,1,fp_out);
                                fwrite(newCRC32CharIHDR,4,1,fp_out);
                                fseek(fp_in_copied, 17, SEEK_CUR);
                                printf("Изменение IHDR: Успешно\n\n");          
                        } else {
                                fseek(fp_in_copied, -3, SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                for (int i = 0; i < 20; i++) {
                                        unsigned char innerByte = 0x00;
                                        fread(&innerByte,1,1,fp_in_copied);
                                        fwrite(&innerByte,1,1,fp_out);
                                }
                        }
                        if (header!=NULL && data!=NULL) {
                                u_int32_t length = strlen(header) + strlen(data) + 1;
                                u_int8_t bytes[4];
                                bytes[0] = (length >> 24) & 0xFF; 
                                bytes[1] = (length >> 16) & 0xFF;
                                bytes[2] = (length >> 8) & 0xFF;
                                bytes[3] = (length >> 0) & 0xFF;

                                unsigned char* newMetadata = (unsigned char*)malloc(length+4);
                                newMetadata[0] = 0x74;
                                newMetadata[1] = 0x45;
                                newMetadata[2] = 0x58;
                                newMetadata[3] = 0x74;
                                strcpy(newMetadata + 4, header);
                                newMetadata[4 + strlen(header)] = 0x00;
                                strcpy(newMetadata + 4 + strlen(header) + 1, data);
                                newCRC32 = crc32b(newMetadata, length);
                                unsigned char newCRC32CharTXT[4] = {0};
                                newCRC32CharTXT[0] = (newCRC32 >> 24) & 0xFF; 
                                newCRC32CharTXT[1] = (newCRC32 >> 16) & 0xFF; 
                                newCRC32CharTXT[2] = (newCRC32 >> 8)  & 0xFF; 
                                newCRC32CharTXT[3] = newCRC32 & 0xFF;
                                fwrite(bytes, 4, 1, fp_out);
                                fwrite(newMetadata, length + 4, 1, fp_out);
                                fwrite(newCRC32CharTXT,4,1,fp_out);
                                printf("Добавление новых метаданных: Успешно\n\n");
                                free(newMetadata);
                        }
                        continue;
                }
                if (currentByte == 0x70) {
                        unsigned char bytesHys[3] = {0};
                        fread(bytesHys,3,1,fp_in_copied);
                        if (bytesHys[0]!=0x48 || bytesHys[1]!=0x59 || bytesHys[2]!=0x73){
                                fseek(fp_in_copied, -3, SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        if (!(horizontalResolution == -1 && verticalResolution == -1 && measure!=0 && measure!=1)) {
                                fwrite(newPhys,13,1,fp_out);
                                fwrite(newCRC32CharPHYS,4,1,fp_out);
                                fseek(fp_in_copied, 13, SEEK_CUR);
                                printf("Изменение PHYS: Успешно\n\n");   
                                continue; 
                        }
                        fwrite(&currentByte,1,1,fp_out);
                        fseek(fp_in_copied, -3, SEEK_CUR);
                        for (int i = 0; i < 12; i++) {
                                unsigned char innerByte = 0x00;
                                fread(&innerByte,1,1,fp_in_copied);
                                fwrite(&innerByte,1,1,fp_out);
                        }
                        continue;
                }
                fwrite(&currentByte,1,1,fp_out);
        }   
        }
        
        printf("Изменение исходного файла: Успешно\n\n");
        fclose(fp_in_copied);
        fclose(fp_out);
        free(new_filename);
}
int addMetadata(char* filename, char* header, char* data, int argc, char** argv) {
        struct stat fileInfo;
        char atimeBufferInput[20];
        char ctimeBufferInput[20];
        char mtimeBufferInput[20];
        int changeAtime = getSystemMarks(argc, argv, "--atime",atimeBufferInput);
        int changeCtime = getSystemMarks(argc, argv, "--ctime",ctimeBufferInput);
        int changeMtime = getSystemMarks(argc, argv, "--mtime",mtimeBufferInput);
        if (stat(filename, &fileInfo) == 0) {
                char buffer[20];

                time_t mtime = fileInfo.st_mtime;
                struct tm *tm_info_m = localtime(&mtime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_m);
                if (changeMtime != 0) {
                    strcpy(mtimeBufferInput, buffer);
                }
                time_t atime = fileInfo.st_atime;
                struct tm *tm_info_a = localtime(&atime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_a);

                if (changeAtime != 0) {
                    strcpy(atimeBufferInput, buffer);
                }

                time_t ctime = fileInfo.st_ctime;
                struct tm *tm_info_c = localtime(&ctime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_c);

                if (changeCtime != 0) {
                    strcpy(ctimeBufferInput, buffer);
                }
        }
        FILE* fp_in = fopen(filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
        unsigned char headerBytes[4] = {0};
        fread(headerBytes, 1, 4, fp_in);
        if (headerBytes[0] == 0x89 && headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
                int result = addMetadataPNG(fp_in, header, data, filename, argc, argv);
        } else {
                printf("Неподдерживаемый формат файла\n");
                fclose(fp_in);
        }
        struct timespec new_times[2];
        struct timespec current_time;
        if (clock_gettime(CLOCK_REALTIME, &current_time)) {
                printf("\nОшибка при получении текущего системного времени\n");
                return 1;
        }
    
        struct tm tm_atime = {0}, tm_ctime = {0}, tm_mtime = {0};
        strptime(atimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_atime);
        strptime(ctimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_ctime);
        strptime(mtimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_mtime);


        struct timespec new_system_time;
        new_system_time.tv_sec = mktime(&tm_ctime);
        new_system_time.tv_nsec = 0;
        if (clock_settime(CLOCK_REALTIME, &new_system_time)) {
            perror("\nОшибка при установке системного времени\n");
            return 1;
        }
        
        new_times[0].tv_sec = mktime(&tm_atime); 
        new_times[1].tv_sec = mktime(&tm_mtime);  
        
        if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
            printf("\nОшибка при изменении временных меток файла\n");
            return 1;
        }
        printf("\nВременные метки файла успешно изменены.\n");
        
        if (clock_settime(CLOCK_REALTIME, &current_time)) {
            perror("Ошибка при восстановлении системного времени");
            return 1;
        }

        return 0;
}


/*

	МОДУЛЬ ОБНОВЛЕНИЯ

*/
int updateMetadataPNG(FILE* fp_in, char* header, char* data, char* filename, int argc, char** argv){
	printf("Изменение заголовка PNG\n");
        printf("-----------------------\n");
        int width = getArgumentSize(argc, argv, "--width");
        int height = getArgumentSize(argc, argv, "--height");
        
        int widthChanged = 1;
        int heightChanged = 1;
        int depthChanged = 1;
        int colorTypeChanged = 1;
        int compressionChanged = 1;
        int filterChanged = 1;
        int interlaceChanged = 1;

        if (width < 0) {
                printf("Изменение ширины не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                widthChanged = -1;
        }
        if (height < 0){
                printf("Изменение высоты не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                heightChanged = -1;
        }
        unsigned char oldHeader[17] = {0};
        int headerFound = 0;
        if (fp_in) {
                unsigned char currentByte = 0x00;
                while (fread(&currentByte,1,1,fp_in) == 1){
                        if (currentByte == 0x49) {
                                unsigned char bytesHDR[3] = {0};
                                fread(bytesHDR,3,1,fp_in);
                                if (bytesHDR[0]!=0x48 || bytesHDR[1]!=0x44 || bytesHDR[2]!=0x52){
                                        fseek(fp_in, -3, SEEK_CUR);
                                        continue;
                                }
                                headerFound = 1;
                                fread(oldHeader,17,1,fp_in);
                                break;
                        }
                }
        }
        if (headerFound == 0) {
                printf("Ошибка: У файла не найден IHDR\n");
                fclose(fp_in);
                return 1;
        }
        headerFound = -1;
        unsigned char newHeader[17] = {0};
        newHeader[0] = 0x49;
        newHeader[1] = 0x48;
        newHeader[2] = 0x44;
        newHeader[3] = 0x52;
        if (widthChanged==-1 && heightChanged==-1){
                printf("Итог изменения заголовка: Нечего изменять\n\n");
        } else{
                printf("Итог изменения заголовка: Данные изменения заголовка принять в обработку\n\n");
        }
        if (widthChanged!=-1) {
                newHeader[4] = (width >> 24) & 0xFF; 
                newHeader[5] = (width >> 16) & 0xFF; 
                newHeader[6] = (width >> 8)  & 0xFF; 
                newHeader[7] = width & 0xFF;
        } else {
                newHeader[4] = oldHeader[0];
                newHeader[5] = oldHeader[1];
                newHeader[6] = oldHeader[2];
                newHeader[7] = oldHeader[3];
        }
        if (heightChanged!=-1) {
                newHeader[8] = (height >> 24) & 0xFF; 
                newHeader[9] = (height >> 16) & 0xFF; 
                newHeader[10] = (height >> 8)  & 0xFF; 
                newHeader[11] = height & 0xFF; 
        } else {
                newHeader[8] =  oldHeader[0];
                newHeader[9] =  oldHeader[1];
                newHeader[10] =  oldHeader[2];
                newHeader[11] =  oldHeader[3];
        }
        
        newHeader[12] = oldHeader[8];
        newHeader[13] = oldHeader[9];
        newHeader[14] = oldHeader[10];
        newHeader[15] = oldHeader[11];
        newHeader[16] = oldHeader[12];
       
        unsigned int newCRC32 = crc32b(newHeader, 17);
        unsigned char newCRC32CharIHDR[4] = {0};
        newCRC32CharIHDR[0] = (newCRC32 >> 24) & 0xFF; 
        newCRC32CharIHDR[1] = (newCRC32 >> 16) & 0xFF; 
        newCRC32CharIHDR[2] = (newCRC32 >> 8)  & 0xFF; 
        newCRC32CharIHDR[3] = newCRC32 & 0xFF;

        printf("Изменение PHYS PNG\n");
        printf("------------------\n");
        int horizontalResolution = getArgumentSize(argc, argv, "--horizontal");
        int verticalResolution = getArgumentSize(argc, argv, "--vertical");
        int measure = getArgumentSize(argc,argv,"--measure");
        if (horizontalResolution == -1) {
                printf("Изменение горизонтального разрешения не было произведено(указаны некорректные параметры или параметры не указаны)\n");
        }
        if (verticalResolution == -1) {
                printf("Изменение вертикального разрешения не было произведено(указаны некорректные параметры или параметры не указаны)\n");                
        }
        if (measure!=0 && measure!=1){
                printf("Изменение меры измерения разрещения не было произведено(указаны некорректные параметры или параметры не указаны)\n");
        }
        if (horizontalResolution == -1 && verticalResolution == -1 && measure!=0 && measure!=1) {
                printf("Итог изменения PHYS: Нечего изменть\n\n");
        } else {
                printf("Итог изменения PHYS: Данные приняты в обработку\n\n");
        }
        
        int physFound = -1;
        unsigned char oldPhys[13] = {0};
        if (fp_in) {
                unsigned char currentByte = 0x00;
                while (fread(&currentByte,1,1,fp_in) == 1){
                        if (currentByte == 0x70) {
                                unsigned char bytesHDR[3] = {0};
                                fread(bytesHDR,3,1,fp_in);
                                if (bytesHDR[0]!=0x48 || bytesHDR[1]!=0x59 || bytesHDR[2]!=0x73){
                                        fseek(fp_in, -3, SEEK_CUR);
                                        continue;
                                }
                                physFound = 1;
                                fread(oldPhys,13,1,fp_in);
                                break;
                        } 
                }
        }
        
        unsigned char newPhys[13] = {0};
        newPhys[0] = 0x70;
        newPhys[1] = 0x48;
        newPhys[2] = 0x59;
        newPhys[3] = 0x73;
        if (horizontalResolution != -1) {
                newPhys[4] = (horizontalResolution >> 24) & 0xFF; 
                newPhys[5] = (horizontalResolution >> 16) & 0xFF; 
                newPhys[6] = (horizontalResolution >> 8)  & 0xFF; 
                newPhys[7] = horizontalResolution & 0xFF;
        } else {
                newPhys[4] = oldPhys[0];
                newPhys[5] = oldPhys[1];
                newPhys[6] = oldPhys[2];
                newPhys[7] = oldPhys[3];
        }
        if (verticalResolution != -1) {
                newPhys[8] = (verticalResolution >> 24) & 0xFF; 
                newPhys[9] = (verticalResolution >> 16) & 0xFF; 
                newPhys[10] = (verticalResolution >> 8)  & 0xFF; 
                newPhys[11] = verticalResolution & 0xFF; 
        } else {
                newPhys[8] =  oldPhys[4];
                newPhys[9] =  oldPhys[5];
                newPhys[10] =  oldPhys[6];
                newPhys[11] =  oldPhys[7];
        }
        if (measure != -1) {
                newPhys[12] = (unsigned char)measure;
        } else {
                newPhys[12] = oldPhys[8];
        }
        newCRC32 = crc32b(newPhys, 13);
        unsigned char newCRC32CharPHYS[4] = {0};
        newCRC32CharPHYS[0] = (newCRC32 >> 24) & 0xFF; 
        newCRC32CharPHYS[1] = (newCRC32 >> 16) & 0xFF; 
        newCRC32CharPHYS[2] = (newCRC32 >> 8)  & 0xFF; 
        newCRC32CharPHYS[3] = newCRC32 & 0xFF;
        if (physFound == -1&&!(horizontalResolution == -1 && verticalResolution == -1 && measure!=0 && measure!=1)) {
                printf("Ошибка: У файла не найден PHYS\n\n");
        }

        fclose(fp_in);

        printf("Копирование исходного файла в %s_update_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_update_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_update_copy");

        FILE* fp_in_copy = fopen(filename, "rb");
        FILE* fp_out_copy = fopen(new_filename, "wb");
        
        if (fp_in_copy && fp_out_copy) {
                unsigned char currentByte = 0x00;
                while (fread(&currentByte, 1, 1, fp_in_copy) == 1) {
                        fwrite(&currentByte, 1, 1, fp_out_copy);
                }
        }

        if (fp_in_copy) fclose(fp_in_copy);
        if (fp_out_copy) fclose(fp_out_copy);   
        printf("Процесс изменения исходного файла:\n");
        FILE* fp_out = fopen(filename, "wb");
        FILE* fp_in_copied = fopen(new_filename, "rb");
        unsigned char currentBytes[8] = {0};
        int result = 0;
        while (1) {
                result = fread(currentBytes,1,8, fp_in_copied);
                if (result == 0) {
                        break;
                }
                if (result<8){
                        fwrite(currentBytes,result, 1, fp_out);
                        break;
                }
                if ((currentBytes[4]!=0x74 || header==NULL || data==NULL) && (currentBytes[4]!=0x49||(widthChanged==-1 && heightChanged==-1)) && (currentBytes[4]!=0x70||(horizontalResolution == -1 && verticalResolution == -1 && measure!=0 && measure!=1))){
                        fwrite(currentBytes, 1, 1,fp_out);
                        fseek(fp_in_copied,-7,SEEK_CUR);
                        continue;
                }
                u_int32_t length = (currentBytes[3] << 0)  |
                                   (currentBytes[2] << 8)  |
                                   (currentBytes[1] << 16) |
                                   (currentBytes[0] << 24);
                if (currentBytes[4] == 0x49) {
                        if (currentBytes[5]!=0x48 || currentBytes[6]!=0x44 || currentBytes[7]!=0x52){
                                fwrite(currentBytes, 1, 1,fp_out);
                                fseek(fp_in_copied,-7,SEEK_CUR);
                                continue;
                        }
                        fseek(fp_in_copied,13+4,SEEK_CUR);
                        fwrite(currentBytes,4,1,fp_out);
                        fwrite(newHeader,17,1,fp_out);
                        fwrite(newCRC32CharIHDR,4,1,fp_out);
                        printf("Заголовок обновлён!\n");
                        continue;
                }
                if (currentBytes[4] == 0x70){
                        if (currentBytes[5]!=0x48 || currentBytes[6]!=0x59 || currentBytes[7]!=0x73){
                                fwrite(currentBytes, 1, 1,fp_out);
                                fseek(fp_in_copied,-7,SEEK_CUR);
                                continue;
                        }
                        fwrite(currentBytes,4,1,fp_out);
                        fwrite(newPhys,13,1,fp_out);
                        fwrite(newCRC32CharPHYS,4,1,fp_out);
                        fseek(fp_in_copied, 9+4, SEEK_CUR);
                        printf("Изменение PHYS: Успешно\n\n");   
                        continue; 
                }
                if (currentBytes[5]!=0x45 || currentBytes[6]!=0x58 || currentBytes[7]!=0x74){
                        fwrite(currentBytes, 1, 1,fp_out);
                        fseek(fp_in_copied,-7,SEEK_CUR);
                        continue;
                }
                int currentTab = 0;
                unsigned char currentByte = 0x00;
                int otherText = -1;
                int headerLen = strlen(header);
                for (int i = 0; i < headerLen; i++){
                        fread(&currentByte,1,1,fp_in_copied);
                        currentTab++;
                        if (header[i]!=currentByte) {
                                otherText = 1;
                                break;
                        }
                }
                if (otherText == 1){
                        fwrite(currentBytes, 1, 1,fp_out);
                        fseek(fp_in_copied,-7-currentTab,SEEK_CUR);
                        continue; 
                }
                fread(&currentByte,1,1,fp_in_copied);
                fseek(fp_in_copied,-1,SEEK_CUR);
                if (currentByte != 0x00){
                        fwrite(currentBytes, 1, 1,fp_out);
                        fseek(fp_in_copied,-7-currentTab,SEEK_CUR);
                }
                fseek(fp_in_copied,-currentTab,SEEK_CUR);
                fseek(fp_in_copied, length+4, SEEK_CUR);
                
                u_int32_t newLength = strlen(header) + strlen(data) + 1;
                u_int8_t bytes[4];
                bytes[0] = (newLength >> 24) & 0xFF; 
                bytes[1] = (newLength >> 16) & 0xFF;
                bytes[2] = (newLength >> 8) & 0xFF;
                bytes[3] = (newLength >> 0) & 0xFF;
                unsigned char* newMetadata = (unsigned char*)malloc(newLength+4);
                newMetadata[0] = 0x74;
                newMetadata[1] = 0x45;
                newMetadata[2] = 0x58;
                newMetadata[3] = 0x74;
                strcpy(newMetadata + 4, header);
                newMetadata[4 + strlen(header)] = 0x00;
                strcpy(newMetadata + 4 + strlen(header) + 1, data);
                newCRC32 = crc32b(newMetadata, newLength);
                unsigned char newCRC32CharTXT[4] = {0};
                newCRC32CharTXT[0] = (newCRC32 >> 24) & 0xFF; 
                newCRC32CharTXT[1] = (newCRC32 >> 16) & 0xFF; 
                newCRC32CharTXT[2] = (newCRC32 >> 8)  & 0xFF; 
                newCRC32CharTXT[3] = newCRC32 & 0xFF;
                fwrite(bytes, 4, 1, fp_out);
                fwrite(newMetadata, newLength + 4, 1, fp_out);
                fwrite(newCRC32CharTXT,4,1,fp_out);
                printf("Изменение метаданных: Успешно\n\n");
                headerFound = 1;
                free(newMetadata);
        }
        if (fp_in_copied) fclose(fp_in_copied);
        if(fp_out) fclose(fp_out);
        if (header!=NULL && data!=NULL && headerFound == -1){
                printf("Изменение метаданных: Не найдены метаданные с заданным заголовком\n\n");
        }
        return 0;
}

int updateMetadata(char* filename, char* header, char* data, int argc, char** argv) {
        struct stat fileInfo;
        char atimeBufferInput[20];
        char ctimeBufferInput[20];
        char mtimeBufferInput[20];
        int changeAtime = getSystemMarks(argc, argv, "--atime",atimeBufferInput);
        int changeCtime = getSystemMarks(argc, argv, "--ctime",ctimeBufferInput);
        int changeMtime = getSystemMarks(argc, argv, "--mtime",mtimeBufferInput);
        if (stat(filename, &fileInfo) == 0) {
                char buffer[20];

                time_t mtime = fileInfo.st_mtime;
                struct tm *tm_info_m = localtime(&mtime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_m);
                if (changeMtime != 0) {
                    strcpy(mtimeBufferInput, buffer);
                }
                time_t atime = fileInfo.st_atime;
                struct tm *tm_info_a = localtime(&atime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_a);

                if (changeAtime != 0) {
                    strcpy(atimeBufferInput, buffer);
                }

                time_t ctime = fileInfo.st_ctime;
                struct tm *tm_info_c = localtime(&ctime);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_c);

                if (changeCtime != 0) {
                    strcpy(ctimeBufferInput, buffer);
                }
        }
        FILE* fp_in = fopen(filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
        unsigned char headerBytes[4] = {0};
        fread(headerBytes, 1, 4, fp_in);
        if (headerBytes[0] == 0x89 && headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
                int result = updateMetadataPNG(fp_in, header, data, filename, argc, argv);
        } else {
                printf("Неподдерживаемый формат файла\n");
                fclose(fp_in);
        }
        struct timespec new_times[2];
        struct timespec current_time;
        if (clock_gettime(CLOCK_REALTIME, &current_time)) {
                printf("\nОшибка при получении текущего системного времени\n");
                return 1;
        }
    
        struct tm tm_atime = {0}, tm_ctime = {0}, tm_mtime = {0};
        strptime(atimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_atime);
        strptime(ctimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_ctime);
        strptime(mtimeBufferInput, "%Y-%m-%d %H:%M:%S", &tm_mtime);


        struct timespec new_system_time;
        new_system_time.tv_sec = mktime(&tm_ctime);
        new_system_time.tv_nsec = 0;
        if (clock_settime(CLOCK_REALTIME, &new_system_time)) {
            perror("\nОшибка при установке системного времени\n");
            return 1;
        }
        
        new_times[0].tv_sec = mktime(&tm_atime); 
        new_times[1].tv_sec = mktime(&tm_mtime);  
        
        if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
            printf("\nОшибка при изменении временных меток файла\n");
            return 1;
        }
        printf("\nВременные метки файла успешно изменены.\n");
        
        if (clock_settime(CLOCK_REALTIME, &current_time)) {
            perror("Ошибка при восстановлении системного времени");
            return 1;
        }

        return 0;
}


/*

	МОДУЛЬ ОБРАБОТКИ ВВОДА

*/

char* getFileName(int argc, char** argv) {
	for (int i = 2; i < argc; i++ ) {
		if (strcmp(argv[i],"--filename") == 0) {
			if (argc <= i+1 || strcmp(argv[i+1],"") == 0) {
				printf("Ошибка: Не указано имя файла \n");
				writeHelpMessage(argv[0]);
				return NULL;
			}
			return argv[i+1];
		}
	}
        printf("Ошибка: Не указано имя файла \n");
	writeHelpMessage(argv[0]);
	return NULL;
}

char* getHeader(int argc, char** argv) {
	for (int i = 2; i < argc; i++ ) {
                if (strcmp(argv[i],"--header") == 0) {
                        if (argc <= i+1 || strcmp(argv[i+1],"") == 0) {
                                return NULL;
                        }
                        return argv[i+1];
                }
        }
        return NULL;
}

char* getData(int argc, char** argv) {
        for (int i = 2; i < argc; i++ ) {
                if (strcmp(argv[i],"--data") == 0) {
                        if (argc <= i+1 || strcmp(argv[i+1],"") == 0) {
                                return NULL;
                        }
                        return argv[i+1];
                }
        }
        return NULL;
}



int main(int argc, char** argv) {
	if (argc < 3 ){
		writeHelpMessage(argv[0]);
		return 0;
	}
	if (strcmp(argv[1],"--help") == 0) {
                writeHelpMessage(argv[0]);
                return 0;
        }
        if (strcmp(argv[1],"--read") == 0) {
		char* filename = getFileName(argc,argv);
		if (filename == NULL){
			return 1;
		}
		readMetadata(filename, argc, argv);
                return 0;
        }
        if (strcmp(argv[1],"--update") == 0) {
		char* filename = getFileName(argc,argv);
                if (filename == NULL){
                        return 1;
                }
		char* header = getHeader(argc,argv);
                if (header == NULL){
                        return 1;
                }
		char* data = getData(argc,argv);
		if (data == NULL){
                        return 1;
                }
		updateMetadata(filename, header, data, argc, argv);
                return 0;
        }
        if (strcmp(argv[1],"--add") == 0) {
                char* filename = getFileName(argc,argv);
                if (filename == NULL){
                        return 1;
                }
		char* header = getHeader(argc,argv);
		char* data = getData(argc,argv);
                if ((header!=NULL && data == NULL) || (header==NULL&&data!=NULL)){
                        return 1;
                }
		addMetadata(filename, header, data, argc, argv);
                return 0;
        }
        if (strcmp(argv[1],"--delete") == 0) {
                char* filename = getFileName(argc,argv);
                if (filename == NULL){
                        return 1;
                }
		char* header = getHeader(argc,argv);
		if (header == NULL){
			return 1;
		}
		deleteMetadata(filename, header, argc, argv);
		return 0;
        }
	writeHelpMessage(argv[0]);
	return 1;
}

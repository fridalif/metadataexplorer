#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

/*

        ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

*/

int writeHelpMessage(char* execName) {
	printf("Использование: %s {--update, --add, --delete, --read, --help} [{Опции}]\n", execName);
	printf("Опции: \n");
	printf("--filename <имя_файла> - Название файла с которым будет проводиться работа \n");
        printf("{--atime, --ctime, --mtime} ГГГГ-ММ-ДД ЧЧ:мм:сс - Изменение системных меток(если нет флага оставляет прошлые)\n");
        printf("\t atime - Дата последнего доступа \n");
        printf("\t ctime - Дата изменения метаданных \n");
        printf("\t mtime - Дата последнего изменения \n");
	printf("--header <Заголовок> - Заголовок метаданных (комментария) при изменении, удалении и добавлении \n");
	printf("--data <Данные> - Метаданные (комментарий), которые будут добавлены или на которые будет произведена подмена\n");
        printf("\nPNG\n");
        printf("\t--width <Пиксели> - Ширина в пикселях\n");
        printf("\t--height <Пиксели> - Высота в пикселях\n");
        printf("\t--horizontal <Число> - Горизонтальное разрешение\n");
        printf("\t--vertical <Число> - Вертикальное разрешение\n");
        printf("\t--measure {0,1} - Единица измерения разрешения, 0 - соотношение сторон, 1 - метры\n");
        printf("\t--interlace {0,1} - Разрвёртка, 0 - нет, 1 - Adam7\n");
        printf("\t--compression {0,1,2,3} - Cжатие\n");
        printf("\t\t 0 - Deflate самый быстрый\n");
        printf("\t\t 1 - Deflate быстрый\n");
        printf("\t\t 2 - Deflate по умолчанию\n");
        printf("\t\t 3 - Deflate максимальное сжатие\n");
        printf("\t--filter {0,1,2,3,4} - Метод фильтрации\n");
        printf("\t\t 0 - Нет\n");
        printf("\t\t 1 - Вычитание левого(Sub)\n");
        printf("\t\t 2 - Вычитание верхнего(Up)\n");
        printf("\t\t 3 - Вычитание среднего(Average)\n");
        printf("\t\t 4 - Алгоритм Paeth\n");
        printf("\t--colorType {0,2,3,4,6} - Цветовой тип\n");
        printf("\t\t 0 - Оттенки серого\n");
        printf("\t\t 2 - RGB\n");
        printf("\t\t 3 - Палитровые цвета\n");
        printf("\t\t 4 - Оттенки серого с альфа-каналом\n");
        printf("\t\t 6 - RGB с альфа-каналом\n");
        printf("\t--depth {0-15} - Глубина цвета\n");
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


/*

	МОДУЛЬ ЧТЕНИЯ

*/


int readMetadataPNG(FILE* fp_in) {
    char lastByte;
    char currentByte;
    while (fp_in){
        fread(&currentByte, 1, 1, fp_in);
        if (currentByte == 0x74){
            char bytes[3] = {0};
            fread(&bytes, 3 ,1, fp_in);
            if (bytes[0]!=0x45 || bytes[1]!=0x58 || bytes[2] != 0x74){
                fseek(fp_in, -3, SEEK_CUR);
            } else {
		int remain = lastByte - 0x00;
		int tempRemain = remain;
		char* metaData = (char*) malloc((remain + 1) * sizeof(char));
		while (remain>0 && fp_in) {
		    char metaDataByte;
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
	    char bytes[3] = {0};
	    fread(&bytes, 3 ,1, fp_in);
            if (bytes[0]==0x44 || bytes[1]==0x41 || bytes[2] == 0x54){
		break;
            }
            if (bytes[0] == 0x48 && bytes[1] == 0x44 && bytes[2] == 0x52){
                unsigned char widthBytes[4] = {0};
                fread(&widthBytes, 4 ,1, fp_in);
                u_int32_t width = (u_int32_t)((widthBytes[0] << 24) |
                                 (widthBytes[1] << 16) |
                                 (widthBytes[2] << 8)  |
                                 (widthBytes[3]));
                printf("Ширина изображения: %u\n",width);
                unsigned char heightBytes[4] = {0};
                fread(&heightBytes, 4 ,1, fp_in);
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
            char bytes[3] = {0};
	    fread(&bytes, 3 ,1, fp_in);
            if (bytes[0]==0x48 || bytes[1]==59 || bytes[2] == 0x73){
                unsigned char horizontalBytes[4] = {0};
                fread(&horizontalBytes, 4 ,1, fp_in);
                u_int32_t horizontal = (u_int32_t)((horizontalBytes[0] << 24) |
                                 (horizontalBytes[1] << 16) |
                                 (horizontalBytes[2] << 8)  |
                                 (horizontalBytes[3]));
                printf("Горизонтальное разрешение: %u\n",horizontal);
                unsigned char verticalBytes[4] = {0};
                fread(&verticalBytes, 4 ,1, fp_in);
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
    char headerBytes[4] = {0};
    fread(&headerBytes, 4, 1, fp_in);
    if (headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
        printf("\nВнутренние данные файла\n");
        printf("-----------------------\n");
        printf("Тип файла: PNG \n");
        printf("MIME Тип: image/png\n");
        readMetadataPNG(fp_in);
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
        int headerLen = strlen(header);
        int position = 0;
        u_int16_t byteLen = 0;
        while (fp_in){
                char currentByte;
                fread(&currentByte, 1, 1, fp_in);
                if (currentByte == 0x49){
                        char bytes[3] = {0};
                        fread(&bytes,3,1,fp_in);
                        if ((bytes[0] == 0x45 && bytes[1] == 0x4e && bytes[2] == 0x44)||(bytes[0] == 0x44 && bytes[1] == 0x41 && bytes[2] == 0x54)){
                                printf("Не найдено метаданных с таким заголовком");
                                fclose(fp_in);
                                return 1;
                        }
                        fseek(fp_in, -3, SEEK_CUR);
                }
                if (currentByte == 0x74){
                        char bytes[3] = {0};
                        fread(&bytes,3,1,fp_in);
                        if (bytes[0]==0x45 && bytes[1]==0x58 && bytes[2] == 0x74){
                                fseek(fp_in,-6,SEEK_CUR);
                                char tempLen[2] = {0};
                                fread(&tempLen, 2, 1, fp_in);
                                fseek(fp_in,4,SEEK_CUR);
                                u_int16_t tempLenInt = (tempLen[0] << 8) | tempLen[1];
                                if (tempLenInt<=headerLen){
                                        fseek(fp_in,-3,SEEK_CUR);
                                        position++;
                                        continue;
                                }
                                int notFound = 0;
                                for (int i = 0; i < headerLen; i++){
                                        char fileByteHeader = 0x00;
                                        fread(&fileByteHeader,1,1,fp_in);
                                        if (fileByteHeader!=header[i]){
                                                position++;
                                                fseek(fp_in,-4-i,SEEK_CUR);
                                                notFound = 1;
                                                break;
                                        }
                                }
                                if (notFound == 1){
                                        continue;
                                }
                                position-=3;
                                byteLen = tempLenInt;
                                break;
                        }
                }
                position++;
        }
        if (!fp_in){
                printf("Не нашлось метаданных с таким заголовком\n");
                fclose(fp_in);
                return 1;
        }
        fseek(fp_in, 0L, SEEK_SET);
        fseek(fp_in, position+6, SEEK_CUR);
        char nullBytes[2] = {};
        fread(&nullBytes,2,1,fp_in);
        char lenghtBytes[2] = {};
        fread(&lenghtBytes,2,1,fp_in);
        char textBytes[4] = {};
        fread(&textBytes,4,1,fp_in);
        char* headerAndDataBytes = (char*)malloc(byteLen);
        for (int i = 0; i < byteLen; i++){
                char currentByte = 0x00;
                fread(&currentByte, 1, 1, fp_in);
                headerAndDataBytes[i] = currentByte;
        }
        char crc32Bytes[4] = {};
        fread(&crc32Bytes,4,1,fp_in);
        printf("Null Bytes: %x %x\n", nullBytes[0], nullBytes[1]);
        printf("Length Bytes: %x %x\n", lenghtBytes[0], lenghtBytes[1]);
        printf("Text Bytes: %x %x %x %x\n",textBytes[0],textBytes[1],textBytes[2],textBytes[3]);
        printf("Header And Data Bytes:");
        for (int i = 0; i < byteLen; i++){
                printf(" %x", headerAndDataBytes[i]);
        }
        printf("\n");
        printf("CRC32 Bytes: %x %x %x %x\n",crc32Bytes[0],crc32Bytes[1],crc32Bytes[2],crc32Bytes[3]);
        free(headerAndDataBytes);
        fclose(fp_in);
	return 0;
}

int deleteMetadata(char* filename, char* header, int argc, char** argv) {
        printf("deleteMetadata %s %s\n",filename, header);
        FILE* fp_in = fopen(filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
	char headerBytes[4] = {0};
    	fread(&headerBytes, 4, 1, fp_in);
    	if (headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
        	printf("File Type: PNG \n");
        	return deleteMetadataPNG(fp_in, header, filename);
    	}
        fclose(fp_in);
        return 0;
}


/*

	МОДУЛЬ ДОБАВЛЕНИЯ

*/
//int addMetadataPNG(FILE* fp_in, char* header, char* data, char* filename) {
//	int headerLen = strlen(header);
//        int dataLen = strlen(data);
//
//        //+1 потому что между заголовком и данными 0x00
//        u_int16_t resultLen = headerLen + dataLen + 1;
//        char byte1 = (resultLen >> 8);
//        char byte2 = resultLen & 0xFF;
//
//        printf("%x %x", byte1, byte2);
//        //+6 потому что и 2 байт - длина, и tEXt
//        int writeLen = (6+resultLen);
//        char* bytesArray = (char*)malloc(writeLen*sizeof(char));
//        bytesArray[0] = (char)byte1;
//        bytesArray[1] = (char)byte2;
//        bytesArray[2] = 't';
//        bytesArray[3] = 'E';
//        bytesArray[4] = 'X';
//        bytesArray[5] = 't';
//        for (int i = 0; i < headerLen; i++){
//                bytesArray[i+6] = header[i];
//        }
//        bytesArray[headerLen+6] = 0x00;
//        for (int i = 0; i < dataLen; i++){
//                bytesArray[headerLen+7+i] = data[i];
//        }
//        int position = 0;
//        while (fp_in){
//                char currentByte;
//                fread(&currentByte,1,1,fp_in);
//                if (currentByte == 0x49){
//                        char bytes[3] = {0};
//                        fread(&bytes, 3 ,1, fp_in);
//                        if (bytes[0]==0x44 || bytes[1]==0x41 || bytes[2] == 0x54){
//                                break;
//                        }
//                        fseek(fp_in, -3, SEEK_CUR);
//                }
//                position++;
//        }
//        if (!fp_in){
//                printf("В картинке не найдено поле IDAT. Невозможно добавить метаданные.");
//                return 1;
//        }
//        fseek(fp_in, 0L, SEEK_SET);
//        char* newFilename = malloc(strlen(filename) + 6);
//        snprintf(newFilename, strlen(filename) + 6, "%s_copy", filename);
//        FILE* fp_out = fopen(newFilename, "wb");
//        free(newFilename);
//        if (!fp_out) {
//                printf("Ошибка открытия файла\n");
//                return 1;
//        }
//        while (fp_in) {
//                if (position == 0){
//                        char endChank[2] = {0x00,0x00};
//                        fwrite(&endChank,2,1,fp_out);
//                        for (int i = 0; i< writeLen; i++){
//                                fwrite(bytesArray+i,1,1,fp_out);
//                        }
//                        int result = crc32b(bytesArray+2,writeLen-2);
//                        unsigned char crc32Bytes[4];
//                        crc32Bytes[0] = (unsigned char)((result >> 24) & 0xFF);
//                        crc32Bytes[1] = (unsigned char)((result >> 16) & 0xFF);
//                        crc32Bytes[2] = (unsigned char)((result >> 8) & 0xFF);
//                        crc32Bytes[3] = (unsigned char)(result & 0xFF);
//                        fwrite(&crc32Bytes,4,1,fp_out);
//                        printf("%02x %02x %02x %02x\n",crc32Bytes[0],crc32Bytes[1],crc32Bytes[2],crc32Bytes[3]);
//                        
//                }
//                if (position!=0){
//                        char srcByte;
//                        fread(&srcByte,1,1,fp_in);
//                        if (srcByte == 0x49){
//	                        char bytes[3] = {0};
//	                        fread(&bytes, 3 ,1, fp_in);
//                                if (bytes[0]==0x45 || bytes[1]==0x4e || bytes[2] == 0x44){
//		                        char iendTag[4] = {'I','E','N','D'};
//                                        char crc[4] ={0};
//                                        fread(&crc,4,1,fp_in);
//                                        fwrite(&iendTag,4,1,fp_out);
//                                        fwrite(&crc,4,1,fp_out);
//                                        break;
//                                }
//	                        fseek(fp_in, -3, SEEK_CUR);
//	                }
//                        fwrite(&srcByte,1,1,fp_out);
//                }
//                position--;
//        }
//        fclose(fp_out);
//        free(bytesArray);
//
//        return 0;
//}

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
                }
        }
        return -1;
}
int addMetadataPNG(FILE* fp_in, char* header, char* data, char* filename, int argc, char** argv) {
        printf("Изменение заголовка PNG\n");
        printf("-----------------------\n");
        int width = getArgumentSize(argc, argv, "--width");
        int height = getArgumentSize(argc, argv, "--height");
        int depth = getArgumentSize(argc, argv, "--depth");
        int colorType = getArgumentSize(argc, argv, "--colorType");
        int compression = getArgumentSize(argc, argv, "--compression");
        int filter = getArgumentSize(argc,argv,"--filter");
        int interlace = getArgumentSize(argc, argv, "--interlace");
        
        bool widthChanged = true;
        bool heightChanged = true;
        bool depthChanged = true;
        bool colorTypeChanged = true;
        bool compressionChanged = true;
        bool filterChanged = true;
        bool interlaceChanged = true;

        if (width < 0) {
                printf("Изменение ширины не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                widthChanged = false;
        }
        if (height < 0){
                printf("Изменение высоты не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                heightChanged = false;
        }
        if (depth<0 || depth>15) {
                printf("Изменение глубины не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                depthChanged = false;
        }
        if (colorType!=0 && colorType != 2 && colorType!=3 && colorType!=4 && colorType!=6){
                printf("Изменение цветового типа не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                colorTypeChanged = false;
        }
        if (compression<0 || compression>3){
                printf("Изменение сжатия не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                compressionChanged = false;
        }
        if (filter<0 || filter>4){
                printf("Изменение метода фильтрации не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                filterChanged = false;
        }
        if (interlace!=0 && interlace!=1){
                printf("Изменение развёртки не было произведено(указаны некорректные параметры или параметры не указаны)\n");
                interlaceChanged = false;
        }
        unsigned char oldHeader[17] = {0};
        while (fp_in) {
                char currentByte = 0x00;
                fread(&currentByte,1,1,fp_in);
                if (currentByte == 0x49) {
                        char bytesHDR[3] = {0};
                        fread(&bytesHDR,3,1,fp_in);
                        if (bytesHDR[0]!=0x48 || bytesHDR[1]!=0x44 || bytesHDR[2]!=0x52){
                                fseek(fp_in, -3, SEEK_CUR);
                                continue;
                        }
                        fread(oldHeader,17,1,fp_in);
                }
        }
        if (!fp_in) {
                printf("Ошибка: У файла не найден IHDR\n");
                return 1;
        }
        unsigned char newHeader[17] = {0};
        newHeader[0] = 0x49;
        newHeader[1] = 0x48;
        newHeader[2] = 0x44;
        newHeader[3] = 0x52;
        if (!widthChanged && !heightChanged && !depthChanged && !colorTypeChanged && !compressionChanged && !filterChanged && !interlaceChanged){
                printf("Итог изменения заголовка: Нечего изменять\n\n");
        }
        if (widthChanged) {
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
        if (heightChanged) {
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
        if (depthChanged) {
                newHeader[12] = (unsigned char)depth;
        } else {
                newHeader[12] = oldHeader[8];
        }
        if (colorTypeChanged) {
                newHeader[13] = (unsigned char)colorType;
        } else {
                newHeader[13] = oldHeader[9];
        }
        if (compressionChanged) {
                newHeader[14] = (unsigned char)compression;
        } else {
                newHeader[14] = oldHeader[10];
        }
        if (filterChanged) {
                newHeader[15] = (unsigned char)filter;
        } else {
                newHeader[15] = oldHeader[11];
        }
        if (interlaceChanged) {
                newHeader[16] = (unsigned char)interlace;
        } else {
                newHeader[16] = oldHeader[12];
        }

        unsigned int newCRC32 = crc32b(newHeader, 17);
        unsigned char newCRC32CharIHDR[4] = {0};
        newCRC32CharIHDR[0] = (newCRC32 >> 24) & 0xFF; 
        newCRC32CharIHDR[1] = (newCRC32 >> 16) & 0xFF; 
        newCRC32CharIHDR[2] = (newCRC32 >> 8)  & 0xFF; 
        newCRC32CharIHDR[3] = newCRC32 & 0xFF;



        int horizontalResolution = getArgumentSize(argc, argv, "--horizontal");
        int verticalResolution = getArgumentSize(argc, argv, "--vertical");
        int measure = getArgumentSize(argc,argv,"--measure");
        if (horizontalResolution == 0) {
                printf("Изменение горизонтального разрешения не было произведено(указаны некорректные параметры или параметры не указаны)\n");
        }
        if (verticalResolution == 0) {
                printf("Изменение вертикального разрешения не было произведено(указаны некорректные параметры или параметры не указаны)\n");                
        }


}
int addMetadata(char* filename, char* header, char* data, int argc, char** argv) {
        char atimeBufferInput[20];
        char ctimeBufferInput[20];
        char mtimeBufferInput[20];
        int changeAtime = getSystemMarks(argc, argv, "--atime",atimeBufferInput);
        int changeCtime = getSystemMarks(argc, argv, "--ctime",ctimeBufferInput);
        int changeMtime = getSystemMarks(argc, argv, "--mtime",mtimeBufferInput);
    
        FILE* fp_in = fopen(filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
        char headerBytes[4] = {0};
        fread(&headerBytes, 4, 1, fp_in);
        if (headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
                int result = addMetadataPNG(fp_in, header, data, filename, argc, argv);
                fclose(fp_in);
                return result;
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

	МОДУЛЬ ОБНОВАЛЕНИЯ

*/
int updateMetadataPNG(FILE* fp_in, char* header, char* data){
	printf("updateMetadataPNG %s %s\n", header, data);
        return 0;
}

int updateMetadata(char* filename, char* header, char* data, int argc, char** argv) {
        printf("updateMetadata %s %s %s\n", filename, header, data);
        FILE* fp_in = fopen(filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
        char headerBytes[4] = {0};
        fread(&headerBytes, 4, 1, fp_in);
        if (headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
                printf("File Type: PNG \n");
                updateMetadataPNG(fp_in, header, data);
                return 0;
        }
        fclose(fp_in);
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
                                printf("Ошибка: Не указан заголовок метаданных \n");
                                writeHelpMessage(argv[0]);
                                return NULL;
                        }
                        return argv[i+1];
                }
        }
	printf("Ошибка: Не указан заголовок метаданных \n");
        writeHelpMessage(argv[0]);
        return NULL;
}

char* getData(int argc, char** argv) {
        for (int i = 2; i < argc; i++ ) {
                if (strcmp(argv[i],"--data") == 0) {
                        if (argc <= i+1 || strcmp(argv[i+1],"") == 0) {
                                printf("Ошибка: Не указано значение метаданных \n");
                                writeHelpMessage(argv[0]);
                                return NULL;
                        }
                        return argv[i+1];
                }
        }
        printf("Ошибка: Не указано значение метаданных \n");
        writeHelpMessage(argv[0]);
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
                if (header == NULL){
                        return 1;
                }
		char* data = getData(argc,argv);
                if (data == NULL){
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

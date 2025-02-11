#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/*

        ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

*/


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
	    fseek(fp_in, -3, SEEK_CUR);
	}
	lastByte = currentByte;
    }
    return 0;
}

int readMetadata(char* filename){
    printf("Системная информация о файле\n");
    printf("----------------------------\n");
    struct stat fileInfo;
    if (stat(filename, &fileInfo) == 0) {
        printf("Размер: %ld Байт\n", fileInfo.st_size);
        
        char buffer[80];

        time_t mtime = fileInfo.st_mtime;
        struct tm *tm_info_m = localtime(&mtime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_m);
        
        printf("Дата изменения: %s\n", buffer);

        time_t atime = fileInfo.st_atime;
        struct tm *tm_info_a = localtime(&atime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_a);

        printf("Дата последнего доступа: %s\n", buffer);

        time_t ctime = fileInfo.st_ctime;
        struct tm *tm_info_c = localtime(&ctime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info_c);
        printf("Дата изменения метаданных: %s\n", buffer);

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
        printf("File Type: PNG \n");
        readMetadataPNG(fp_in);
        return 0;
    }
    fclose(fp_in);
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

int deleteMetadata(char* filename, char* header) {
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
int addMetadataPNG(FILE* fp_in, char* header, char* data, char* filename) {
	int headerLen = strlen(header);
        int dataLen = strlen(data);

        //+1 потому что между заголовком и данными 0x00
        u_int16_t resultLen = headerLen + dataLen + 1;
        char byte1 = (resultLen >> 8);
        char byte2 = resultLen & 0xFF;

        printf("%x %x", byte1, byte2);
        //+6 потому что и 2 байт - длина, и tEXt
        int writeLen = (6+resultLen);
        char* bytesArray = (char*)malloc(writeLen*sizeof(char));
        bytesArray[0] = (char)byte1;
        bytesArray[1] = (char)byte2;
        bytesArray[2] = 't';
        bytesArray[3] = 'E';
        bytesArray[4] = 'X';
        bytesArray[5] = 't';
        for (int i = 0; i < headerLen; i++){
                bytesArray[i+6] = header[i];
        }
        bytesArray[headerLen+6] = 0x00;
        for (int i = 0; i < dataLen; i++){
                bytesArray[headerLen+7+i] = data[i];
        }
        int position = 0;
        while (fp_in){
                char currentByte;
                fread(&currentByte,1,1,fp_in);
                if (currentByte == 0x49){
                        char bytes[3] = {0};
                        fread(&bytes, 3 ,1, fp_in);
                        if (bytes[0]==0x44 || bytes[1]==0x41 || bytes[2] == 0x54){
                                break;
                        }
                        fseek(fp_in, -3, SEEK_CUR);
                }
                position++;
        }
        if (!fp_in){
                printf("В картинке не найдено поле IDAT. Невозможно добавить метаданные.");
                return 1;
        }
        fseek(fp_in, 0L, SEEK_SET);
        char* newFilename = malloc(strlen(filename) + 6);
        snprintf(newFilename, strlen(filename) + 6, "%s_copy", filename);
        FILE* fp_out = fopen(newFilename, "wb");
        free(newFilename);
        if (!fp_out) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
        while (fp_in) {
                if (position == 0){
                        char endChank[2] = {0x00,0x00};
                        fwrite(&endChank,2,1,fp_out);
                        for (int i = 0; i< writeLen; i++){
                                fwrite(bytesArray+i,1,1,fp_out);
                        }
                        int result = crc32b(bytesArray+2,writeLen-2);
                        unsigned char crc32Bytes[4];
                        crc32Bytes[0] = (unsigned char)((result >> 24) & 0xFF);
                        crc32Bytes[1] = (unsigned char)((result >> 16) & 0xFF);
                        crc32Bytes[2] = (unsigned char)((result >> 8) & 0xFF);
                        crc32Bytes[3] = (unsigned char)(result & 0xFF);
                        fwrite(&crc32Bytes,4,1,fp_out);
                        printf("%02x %02x %02x %02x\n",crc32Bytes[0],crc32Bytes[1],crc32Bytes[2],crc32Bytes[3]);
                        
                }
                if (position!=0){
                        char srcByte;
                        fread(&srcByte,1,1,fp_in);
                        if (srcByte == 0x49){
	                        char bytes[3] = {0};
	                        fread(&bytes, 3 ,1, fp_in);
                                if (bytes[0]==0x45 || bytes[1]==0x4e || bytes[2] == 0x44){
		                        char iendTag[4] = {'I','E','N','D'};
                                        char crc[4] ={0};
                                        fread(&crc,4,1,fp_in);
                                        fwrite(&iendTag,4,1,fp_out);
                                        fwrite(&crc,4,1,fp_out);
                                        break;
                                }
	                        fseek(fp_in, -3, SEEK_CUR);
	                }
                        fwrite(&srcByte,1,1,fp_out);
                }
                position--;
        }
        fclose(fp_out);
        free(bytesArray);

        return 0;
}

int addMetadata(char* filename, char* header, char* data) {
        FILE* fp_in = fopen(filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла\n");
                return 1;
        }
        char headerBytes[4] = {0};
        fread(&headerBytes, 4, 1, fp_in);
        if (headerBytes[1] == 0x50 && headerBytes[2] == 0x4e && headerBytes[3] == 0x47) {
                printf("File Type: PNG \n");
                int result = addMetadataPNG(fp_in, header, data, filename);
                fclose(fp_in);
                return result;
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

int updateMetadata(char* filename, char* header, char* data) {
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
int writeHelpMessage(char* execName) {
	printf("Использование: %s {--update, --add, --delete, --read, --help} [{Опции}]\n", execName);
	printf("Опции: \n");
	printf("--filename <имя_файла> - Название файла с которым будет проводиться работа \n");
	printf("--header <Заголовок> - Заголовок метаданных при изменении, удалении и добавлении \n");
	printf("--data <Данные> - Метаданные, которые будут добавлены или на которые будет произведена подмена\n");
	return 1;
}

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
		readMetadata(filename);
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
		updateMetadata(filename, header, data);
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
		addMetadata(filename, header, data);
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
		deleteMetadata(filename, header);
		return 0;
        }
	writeHelpMessage(argv[0]);
	return 1;
}

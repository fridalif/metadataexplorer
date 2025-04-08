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
        TIFF_IMAGE_WIDTH = (0x01<<8)|0x00,
        TIFF_IMAGE_LENGTH = (0x01<<8)|0x01,
        TIFF_BITS_PER_SAMPLE = (0x01<<8)|0x02,
        TIFF_COMPRESSION = (0x01<<8)|0x03,
        TIFF_PHOTOMETRIC_INETRPRETATION = (0x01<<8)|0x06,
        TIFF_SAMPLES_PER_PIXEL = (0x01<<8)|0x15,
        TIFF_STRIP_OFFSETS = (0x01<<8)|0x11,
        TIFF_FILL_ORDER = (0x01<<8)|0x0A,
        //TIFF_SAMPLES_PER_PIXEL = (0x01<<8)|0x12,
        TIFF_ROWS_PER_STRIP = (0x01<<8)|0x16,
        TIFF_STRIP_BYTE_COUNT = (0x01<<8)|0x17,
        TIFF_XRESOLUTION = (0x01<<8)|0x1A,
        TIFF_YRESOLUTION = (0x01<<8)|0x1B,
        TIFF_RESOLUTION_UNIT = (0x01<<8)|0x28,
        TIFF_MAKE = (0x01<<8)|0x0F,
        TIFF_MODEL = (0x01<<8)|0x10,
        TIFF_EXPOSURETIME = (0x82<<8)|0x9A,
        TIFF_FNUMBER = (0x82<<8)|(0x9D),
        TIFF_SOFTWARE = (0x01<<8)|0x31,
        TIFF_ISOSPEEDRATING = (0x88<<8)|0x27,
        TIFF_USERCOMENT = (0x92<<8)|0x86,
        TIFF_LATITUDEREF = (0x00<<8)|(0x01),
        TIFF_LATITUDE = (0x00<<8)|(0x02),
        TIFF_LONGITUDEREF = (0x00<<8)|(0x03),
        TIFF_LONGITUDE = (0x00<<8)|(0x04),
        TIFF_DATETIME = (0x01<<8)|0x32,
        TIFF_IMAGEDESCRIPTION = (0x01)|0x0e
} TIFFTags;


typedef enum {
        EXIF_MAKE = (0x01<<8) | 0x0f,
        EXIF_MODEL = (0x01<<8) | 0x10,
        EXIF_EXPOSURE = (0x82<<8) | 0x9a,
        EXIF_FNUMBER = (0x82<<8) | 0x9d,
        EXIF_ISOSPEEDRATING = (0x88<<8) | 0x27,
        EXIF_USERCOMMENT = (0x92<<8) | 0x86,
        EXIF_GPSLATITUDE = (0x00<<8) | 0x02,
        EXIF_GPSLONGITUDE = (0x00<<8) | 0x04,
        EXIF_GPSLATITUDEREF = (0x00<<8) | 0x01,
        EXIF_GPSLONGITUDEREF = (0x00<<8) | 0x03,
        EXIF_DATETIME = (0x01<<8) | 0x32,
        EXIF_IMAGEDESCRIPTION = (0x01<<8) | 0x0e
} ExifTags;

typedef struct ExifInfo ExifInfo;

typedef struct TIFFInfo TIFFInfo;

struct TIFFInfo {
        TIFFInfo* next;
        ExifFormats format;
        TIFFTags tagType;
        unsigned char* data;
        u_int32_t structuresCount;
        u_int16_t ifdNumber;
        u_int32_t dataLen;
};

struct ExifInfo {
        ExifInfo* next;
        char* exifName;
        int exifNumber;
        int isLittleEndian;
        u_int32_t exifNameLen;
        ExifTags tagType;
        ExifFormats format;
        unsigned char* data;
        u_int32_t dataLen;
        u_int32_t structuresCount;
};

TIFFInfo* initTiffInfo() {
        TIFFInfo* tiffInfo = (TIFFInfo*)malloc(sizeof(TIFFInfo));
        tiffInfo->data = NULL;
        tiffInfo->next = NULL;
        tiffInfo->format = 0;
        tiffInfo->tagType = 0;
        tiffInfo->structuresCount = 0;
        return tiffInfo;
}

void appendTiff(TIFFInfo* start, TIFFInfo* node) {
        TIFFInfo* temp = start;
        while(temp->next!=NULL) {
                temp = temp->next;
        }
        temp->next = node;
}

void clearTIFFInfo(TIFFInfo* start) {
        TIFFInfo* temp = start;
        int counter = 0;
        while(temp!=NULL) {
                TIFFInfo* nextNode = temp->next;
                if (temp->data!=NULL) {
                        free(temp->data);
                }
                free(temp);
                temp = nextNode;
                counter++;
        }
}

void deleteTiffTag(TIFFInfo* start, TIFFTags tag) {
        TIFFInfo* tempTag = start;
        TIFFInfo* tempTagNext = start->next;
        while (tempTagNext!=NULL) {
                if (tempTagNext->tagType == tag) {
                        tempTag->next = tempTagNext->next;
                        tempTagNext->next = NULL;
                        clearTIFFInfo(tempTagNext);
                        tempTagNext = tempTag->next;
                        break;
                }
                tempTag = tempTagNext;
                tempTagNext = tempTagNext->next;
        }
}



int getLastIFD(TIFFInfo* start) {
        TIFFInfo* temp = start;
        while(temp->next!=NULL) {
                temp = temp->next;
        }
        return temp->ifdNumber;
}

void printTIFFTags(TIFFInfo* tag, int isLittleEndian, const char* name) {
        printf("%s в байтах:", name);
        for (int i = 0; i < tag->dataLen; i++) {
                printf(" %02x", tag->data[i]);
        }
        printf("\n");
        printf("%s: ", name);
        u_int16_t shortInt = 0;
        u_int32_t longInt = 0;
        int16_t sshortInt = 0;
        int32_t slongInt = 0;
        u_int32_t ratFirst = 0;
        u_int32_t ratSecond = 1;
        int32_t sratFirst = 0;
        int32_t sratSecond = 1;
        float floatRes = 0;
        double doubleRes = 0;
        char byteArray[4] = {0};
        char bigByteArray[8] = {0};
        for (int i = 0; i < tag->structuresCount; i++) {
                if (tag->format == EXIF_BYTE || tag->format == EXIF_ASCII || tag->format == EXIF_SBYTE || tag->format == EXIF_DATETIME || tag->format ==EXIF_UNDEFINED) {
                        if (isLittleEndian == 1) {
                                for (int j = 0; j < tag->dataLen; j++) {
                                        if (tag->data[j] == 0x00) {
                                                continue;
                                        }
                                        printf("%c", tag->data[j]);
                                }
                        } else {
                                for (int j = 0; j < tag->dataLen; j++) {
                                        printf("%c", tag->data[j]);  
                                }
                        }
                        break;
                }
                switch (tag->format) {
                        case EXIF_SHORT:
                                if (isLittleEndian == 1) {
                                        shortInt = (tag->data[i*2+1]<<8)|tag->data[i*2];
                                } else {
                                        shortInt = (tag->data[i*2]<<8)|tag->data[i*2+1];
                                }
                                printf(" %d", shortInt);
                                break;   
                        case EXIF_SSHORT:
                                if (isLittleEndian == 1) {
                                        sshortInt = (tag->data[i*2+1]<<8)|tag->data[i*2];
                                } else {
                                        sshortInt = (tag->data[i*2]<<8)|tag->data[i*2+1];
                                }
                                printf(" %d", sshortInt);
                                break;
                        case EXIF_LONG:
                                 if (isLittleEndian == 1) {
                                        longInt = (tag->data[i*2+3]<<24)|(tag->data[i*2+2]<<16)|(tag->data[i*2+1]<<8)|tag->data[i*2];
                                } else {
                                        longInt = (tag->data[i*2]<<24)|(tag->data[i*2+1]<<16)|(tag->data[i*2+2]<<8)|(tag->data[i*2+3]);
                                }
                                printf(" %d", longInt);
                                break;
                        case EXIF_SLONG:
                                if (isLittleEndian == 1) {
                                        slongInt = (tag->data[i*2+3]<<24)|(tag->data[i*2+2]<<16)|(tag->data[i*2+1]<<8)|tag->data[i*2];
                                } else {
                                        slongInt = (tag->data[i*2]<<24)|(tag->data[i*2+1]<<16)|(tag->data[i*2+2]<<8)|(tag->data[i*2+3]);
                                }
                                printf(" %d", slongInt);
                                break;
                        case EXIF_RATIONAL:
                                if (isLittleEndian == 1) {
                                       ratSecond = (tag->data[i*2+3]<<24)|(tag->data[i*2+2]<<16)|(tag->data[i*2+1]<<8)|tag->data[i*2];
                                       ratFirst = (tag->data[i*2+7]<<24)|(tag->data[i*2+6]<<16)|(tag->data[i*2+5]<<8)|tag->data[i*2+4]; 
                                } else {
                                        ratFirst = (tag->data[i*2]<<24)|(tag->data[i*2+1]<<16)|(tag->data[i*2+2]<<8)|(tag->data[i*2+3]);
                                        ratSecond = (tag->data[i*2+4]<<24)|(tag->data[i*2+5]<<16)|(tag->data[i*2+6]<<8)|(tag->data[i*2+7]);
                                }
                                printf(" %d/%d", ratSecond, ratFirst);
                                
                                break;
                        case EXIF_SRATIONAL:
                                if (isLittleEndian == 1) {
                                       sratSecond = (tag->data[i*2+3]<<24)|(tag->data[i*2+2]<<16)|(tag->data[i*2+1]<<8)|tag->data[i*2];
                                       sratFirst = (tag->data[i*2+7]<<24)|(tag->data[i*2+6]<<16)|(tag->data[i*2+5]<<8)|tag->data[i*2+4]; 
                                } else {
                                        sratFirst = (tag->data[i*2]<<24)|(tag->data[i*2+1]<<16)|(tag->data[i*2+2]<<8)|(tag->data[i*2+3]);
                                        sratSecond = (tag->data[i*2+4]<<24)|(tag->data[i*2+5]<<16)|(tag->data[i*2+6]<<8)|(tag->data[i*2+7]);
                                }
                                
                                printf(" %d/%d", sratSecond, sratFirst);
                                
                                break;
                        case EXIF_FLOAT:
                                if (isLittleEndian) {
                                        byteArray[0] = tag->data[i+3];
                                        byteArray[1] = tag->data[i+2];
                                        byteArray[2] = tag->data[i+1];
                                        byteArray[3] = tag->data[i];

                                } else {
                                        byteArray[0] = tag->data[i];
                                        byteArray[1] = tag->data[i+1];
                                        byteArray[2] = tag->data[i+2];
                                        byteArray[3] = tag->data[i+3];
                                }
                                floatRes = *((float*)byteArray);
                                printf(" %.2f", floatRes);
                                break;
                        case EXIF_DOUBLE:
                                if (isLittleEndian) {
                                        bigByteArray[0] = tag->data[i+7];
                                        bigByteArray[1] = tag->data[i+6];
                                        bigByteArray[2] = tag->data[i+5];
                                        bigByteArray[3] = tag->data[i+4];
                                        bigByteArray[4] = tag->data[i+3];
                                        bigByteArray[5] = tag->data[i+2];
                                        bigByteArray[6] = tag->data[i+1];
                                        bigByteArray[7] = tag->data[i];

                                } else {
                                        bigByteArray[0] = tag->data[i];
                                        bigByteArray[1] = tag->data[i+1];
                                        bigByteArray[2] = tag->data[i+2];
                                        bigByteArray[3] = tag->data[i+3];
                                        bigByteArray[4] = tag->data[i+4];
                                        bigByteArray[5] = tag->data[i+5];
                                        bigByteArray[6] = tag->data[i+6];
                                        bigByteArray[7] = tag->data[i+7];
                                }
                                doubleRes = *((double*)bigByteArray);
                                printf(" %.2lf", doubleRes);
                                break;
                }
                
        }
        printf("\n");
}

void printTIFFInfo(TIFFInfo* start, int isLittleEndian) {
        TIFFInfo* temp = start;
        int ifdNumber = 0;
        while(temp!=NULL) {
                if (temp->data==NULL) {
                        temp = temp->next;
                        continue;
                }
                if (temp->ifdNumber != ifdNumber) {
                        ifdNumber = temp->ifdNumber;
                        printf("\n\nIFD %d\n", ifdNumber);
                        
                }
                char* name = NULL;
                switch (temp->tagType) {
                        case TIFF_IMAGE_WIDTH:
                                name = "Ширина изображения";
                                break;
                        case TIFF_BITS_PER_SAMPLE:
                                name = "Количество бит на пиксель";
                                break;
                        case TIFF_PHOTOMETRIC_INETRPRETATION:
                                name = "Формат изображения";
                                break;
                        case TIFF_STRIP_OFFSETS:
                                name = "Смещение полос";
                                break;
                        case TIFF_SAMPLES_PER_PIXEL:
                                name = "Количество каналов";
                                break;
                        case TIFF_FILL_ORDER:
                                name = "Порядок заполнения";
                                break;
                        case TIFF_IMAGE_LENGTH:
                                name = "Высота изображения";
                                break;
                        case TIFF_ROWS_PER_STRIP:
                                name = "Количество строк в одной полосе";
                                break;
                        case TIFF_STRIP_BYTE_COUNT:
                                name = "Количество байтов в полосе";
                                break;
                        case TIFF_XRESOLUTION:
                                name = "Разрешение по X";
                                break;
                        case TIFF_YRESOLUTION:
                                name = "Разрешение по Y";
                                break;
                        case TIFF_RESOLUTION_UNIT:
                                name = "Единица измерения";
                                break;
                        case TIFF_MAKE:
                                name = "Производитель";
                                break;
                        case TIFF_MODEL:
                                name = "Модель";
                                break;
                        case TIFF_EXPOSURETIME:
                                name = "Время экспозиции";
                                break;
                        case TIFF_FNUMBER:
                                name = "Апертура";
                                break;
                        case TIFF_ISOSPEEDRATING:
                                name = "ISO-чувствительность";
                                break;
                        case TIFF_USERCOMENT:
                                name = "Комментарий";
                                break;
                        case TIFF_DATETIME:
                                name = "Дата и время";
                                break;
                        case TIFF_IMAGEDESCRIPTION:
                                name = "Описание";
                                break;
                        case TIFF_LATITUDE:
                                name = "Широта";
                                break;
                        case TIFF_LONGITUDE:
                                name = "Долгота";
                                break;
                        case TIFF_LATITUDEREF:
                                name = "Широта направление";
                                break;
                        case TIFF_LONGITUDEREF:
                                name = "Долгота направление";
                                break;
                        case TIFF_SOFTWARE:
                                name = "Программа";
                                break;
                        default:
                                char buffname[100];
                                sprintf(buffname,"Unknown tag %d", temp->tagType);
                                name = buffname;
                                break;
                }
                printTIFFTags(temp,isLittleEndian, name);
                temp = temp->next;
        }        
}

int writeHelpMessage(char* execName) {
	printf("Использование: %s {--update, --add, --delete, --read, --help} [{Опции}]\n", execName);
	printf("Опции: \n");
	printf("--filename <имя_файла> - Название файла с которым будет проводиться работа \n");
        printf("{--atime, --ctime, --mtime} ГГГГ-ММ-ДД ЧЧ:мм:сс - Изменение системных меток(если нет флага оставляет прошлые)-H\n");
        printf("\t atime - Дата последнего доступа \n");
        printf("\t ctime - Дата изменения метаданных \n");
        printf("\t mtime - Дата последнего изменения \n");
	printf("--header <Заголовок> - Заголовок метаданных (комментария) при изменении, удалении и добавлении (для PNG, JPEG) \n");
	printf("--data <Данные> - Метаданные(комментарий), которые будут добавлены или на которые будет произведена подмена (для PNG, JPEG)\n");
	
        printf("\nPNG\n");
        printf("\t--width <Пиксели> - Ширина в пикселях(только в режиме --add и --update) НЕБЕЗОПАСНО!\n");
        printf("\t--height <Пиксели> - Высота в пикселях(только в режиме --add и --update) НЕБЕЗОПАСНО!\n");
        printf("\t--horizontal <Число> - Горизонтальное разрешение(только в режиме --add и --update)\n");
        printf("\t--vertical <Число> - Вертикальное разрешение(только в режиме --add и --update)\n");
        printf("\t--measure {0,1} - Единица измерения разрешения, 0 - соотношение сторон, 1 - метры(только в режиме --add и --update)\n");
        printf("\nJPEG\n");
        printf("\t--jfifVersion <Число> - Версия JFIF\n");
        printf("\t--initNewExif - Добавить новый exif блок(только для --add)\n");
        printf("\t--make \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Фирма камеры (только для --add и --update)\n");
        printf("\t--make - Фирма камеры (только для --delete)\n");
        printf("\t--model \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Модель камеры (только для --add и --update)\n");
        printf("\t--model - Модель камеры (только для --delete)\n");
        printf("\t--exposure \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Время экспозиции (только для --add и --update)\n");
        printf("\t--exposure - Время экспозиции (только для --delete)\n");
        printf("\t--fnumber \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Апертура (только для --add и --update)\n");
        printf("\t--fnumber - Апертура (только для --delete)\n");
        printf("\t--lat \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Широта (только для --add и --update)\n");
        printf("\t--lat - Широта (только для --delete)\n");
        printf("\t--lon \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Долгота (только для --add и --update)\n");
        printf("\t--lon - Долгота (только для --delete)\n");
        printf("\t--latRef \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Направление широты (обычно буквы N(North) или S(South)) (только для --add и --update)\n");
        printf("\t--latRef - Направление широты (только для --delete)\n");
        printf("\t--lonRef\" <Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Направление долготы (обычно буквы E(East) или W(West))(только для --add и --update)\n");
        printf("\t--lonRef - Направление долготы (только для --delete)\n");
        printf("\t--dt \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Дата и время (только для --add и --update)\n");
        printf("\t--dt - Дата и время (только для --delete)\n");
        printf("\t--imageDescription \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Описание изображения (только для --add и --update)\n");
        printf("\t--imageDescription - Описание изображения (только для --delete)\n");
        printf("\tИдентификаторы типов:\n");
        printf("\t\t1 - Байт\n");
        printf("\t\t2 - ASCII строка\n");
        printf("\t\t3 - 2-х байтовое беззнаковое число\n");
        printf("\t\t4 - 4-х байтовое беззнаковое число\n");
        printf("\t\t5 - Беззнаковое рациональное число (значение записывается как <Число>/<Число>)\n");
        printf("\t\t6 - Знаковый байт\n");
        printf("\t\t7 - Неизвестный тип(интерпретируется как строка)\n");
        printf("\t\t8 - 2-х байтовое знаковое число\n");
        printf("\t\t9 - 4-х байтовое беззнаковое число\n");
        printf("\t\t10 - Знаковое рациональное число (значение записывается как <Число>/<Число>)\n");
        printf("\t\t11 - Float (значение записывается как <Целая_часть>.<Дробная_часть>)\n");
        printf("\t\t12 - Double (значение записывается как <Целая_часть>.<Дробная_часть>)\n");
        printf("\nTIFF\n");
        printf("\t--make \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Фирма камеры (только для --add и --update)\n");
        printf("\t--make - Фирма камеры (только для --delete)\n");
        printf("\t--model \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Модель камеры (только для --add и --update)\n");
        printf("\t--model - Модель камеры (только для --delete)\n");
        printf("\t--exposure \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Время экспозиции (только для --add и --update)\n");
        printf("\t--exposure - Время экспозиции (только для --delete)\n");
        printf("\t--fnumber \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Апертура (только для --add и --update)\n");
        printf("\t--fnumber - Апертура (только для --delete)\n");
        printf("\t--lat \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Широта (только для --add и --update)\n");
        printf("\t--lat - Широта (только для --delete)\n");
        printf("\t--lon \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Долгота (только для --add и --update)\n");
        printf("\t--lon - Долгота (только для --delete)\n");
        printf("\t--latRef \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Направление широты (обычно буквы N(North) или S(South)) (только для --add и --update)\n");
        printf("\t--latRef - Направление широты (только для --delete)\n");
        printf("\t--lonRef\" <Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Направление долготы (обычно буквы E(East) или W(West))(только для --add и --update)\n");
        printf("\t--lonRef - Направление долготы (только для --delete)\n");
        printf("\t--dt \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Дата и время (только для --add и --update)\n");
        printf("\t--dt - Дата и время (только для --delete)\n");
        printf("\t--imageDescription \"<Значение>(;<Значение>;<Значение>...)\" --type <Идентификатор_типа> - Описание изображения (только для --add и --update)\n");
        printf("\t--imageDescription - Описание изображения (только для --delete)\n");
        printf("\tИдентификаторы типов:\n");
        printf("\t\t1 - Байт\n");
        printf("\t\t2 - ASCII строка\n");
        printf("\t\t3 - 2-х байтовое беззнаковое число\n");
        printf("\t\t4 - 4-х байтовое беззнаковое число\n");
        printf("\t\t5 - Беззнаковое рациональное число (значение записывается как <Число>/<Число>)\n");
        printf("\t\t6 - Знаковый байт\n");
        printf("\t\t7 - Неизвестный тип(интерпретируется как строка)\n");
        printf("\t\t8 - 2-х байтовое знаковое число\n");
        printf("\t\t9 - 4-х байтовое беззнаковое число\n");
        printf("\t\t10 - Знаковое рациональное число (значение записывается как <Число>/<Число>)\n");
        printf("\t\t11 - Float (значение записывается как <Целая_часть>.<Дробная_часть>)\n");
        printf("\t\t12 - Double (значение записывается как <Целая_часть>.<Дробная_часть>)\n");
        printf("\nGIF\n");
        printf("\t--oldData <Данные> - Метаданные(комментарий), которые будут удалены или заменены\n");
        printf("\t--newData <Данные> - Метаданные(комментарий), которые будут добавлены или на которые будет произведена замена\n");
        printf("\t--delay <Число> - Задержка между кадрами в сотых секунды(только для --update)\n");
        return 1;
        
}

u_int16_t getIFDInnerCount(TIFFInfo* start, int ifdNumber) {
        TIFFInfo* current = start;
        u_int16_t counter = 0;
        while (current) {
                if (current->ifdNumber == ifdNumber) {
                        counter++;
                }
                current = current->next;
        }
        return counter;
}

int rewriteTIFF(TIFFInfo* start, char* filename, char* operation, FILE* fp_in) {
        if (fp_in) {
                fclose(fp_in);
        }
        char* new_filename = (char*)malloc(strlen(filename) + strlen(operation) + strlen("_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, operation);
        strcat(new_filename, "_copy");
        fp_in = fopen(filename, "rb");
        FILE* fp_out = fopen(new_filename, "wb");
        if (!fp_in || !fp_out) {
                printf("Ошибка: не удалось открыть файлы при копировании\n");
                free(new_filename);
                return 1;
        }
        long bytesCount = 0;
        unsigned char currentChar = 0x00;
        while (fread(&currentChar,1,1,fp_in) == 1 && fp_in) {
                bytesCount++;
                fwrite(&currentChar,1,1,fp_out);
        }
        if (fp_in) {
                fclose(fp_in);
        }
        if (fp_out) {
                fclose(fp_out);
        }
        unsigned char* bytesWithScipMap = (unsigned char*)malloc(bytesCount);
        fp_in = fopen(new_filename,"rb");
        fp_out = fopen(filename, "wb");

        for (int i = 0; i < bytesCount; i++) {
                bytesWithScipMap[i] = 0x00;
        }
        printf("Копирование исходного файла: Успешно\n\n");
        unsigned char byteFormat[2] = {0x00,0x00};
        fread(byteFormat,1,2,fp_in);
        int isLittleEndian = -1;
        if (byteFormat[0] == 0x49 && byteFormat[1] == 0x49) {
                isLittleEndian = 1;
        }
        fseek(fp_in,2,SEEK_CUR);
        unsigned char nextIFDOffset[4] = {0x00,0x00,0x00,0x00}; 
        u_int32_t nextIFDOffsetNumber = 0;
        while (fp_in && fread(nextIFDOffset,1,4,fp_in) == 4) {
                long currentPosNext = ftell(fp_in)-4;
                bytesWithScipMap[currentPosNext] = 0x01;
                bytesWithScipMap[currentPosNext+1] = 0x01;
                bytesWithScipMap[currentPosNext+2] = 0x01;
                bytesWithScipMap[currentPosNext+3] = 0x01;
                if (nextIFDOffset[0] == 0x00 && nextIFDOffset[1] == 0x00 && nextIFDOffset[2] == 0x00 && nextIFDOffset[3] == 0x00) {
                        break;
                }
                nextIFDOffsetNumber = (nextIFDOffset[0]<<24)|(nextIFDOffset[1]<<16)|(nextIFDOffset[2]<<8)|nextIFDOffset[3];
                if (isLittleEndian == 1) {
                        nextIFDOffsetNumber = (nextIFDOffset[3]<<24)|(nextIFDOffset[2]<<16)|(nextIFDOffset[1]<<8)|nextIFDOffset[0];
                }
                fseek(fp_in,nextIFDOffsetNumber,SEEK_SET);
                unsigned char tagCounterBytes[2] = {0x00,0x00};
                bytesWithScipMap[nextIFDOffsetNumber] = 0x01;
                bytesWithScipMap[nextIFDOffsetNumber+1] = 0x01;
                fread(tagCounterBytes,1,2,fp_in);
                u_int16_t tagCounter = (tagCounterBytes[0]<<8)|tagCounterBytes[1];
                if (isLittleEndian == 1) {
                        tagCounter = (tagCounterBytes[1]<<8)|tagCounterBytes[0];
                }
                for (u_int16_t i = 0; i < tagCounter; i++) {
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+2] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+3] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+4] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+5] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+6] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+7] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+8] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+9] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+10] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+11] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+12] = 0x01;
                        bytesWithScipMap[nextIFDOffsetNumber+12*i+13] = 0x01;
                        fseek(fp_in,2,SEEK_CUR);
                        unsigned char tagFormatBytes[2] = {0x00,0x00};
                        fread(tagFormatBytes,1,2,fp_in);
                        ExifFormats tagFormat = (ExifFormats)(tagFormatBytes[0]<<8|tagFormatBytes[1]);
                        if (isLittleEndian == 1) {
                                tagFormat = (ExifFormats)(tagFormatBytes[1]<<8|tagFormatBytes[0]);
                        }
                        unsigned char tagCountBytes[4] = {0x00,0x00,0x00,0x00};
                        fread(tagCountBytes,1,4,fp_in);
                        u_int32_t tagCount = (tagCountBytes[0]<<24)|(tagCountBytes[1]<<16)|(tagCountBytes[2]<<8)|tagCountBytes[3];
                        if (isLittleEndian == 1) {
                                tagCount = (tagCountBytes[3]<<24)|(tagCountBytes[2]<<16)|(tagCountBytes[1]<<8)|tagCountBytes[0];
                        }
                        u_int32_t resultLen = 0;
                        switch (tagFormat) {
                                case EXIF_BYTE:
                                case EXIF_ASCII:
                                case EXIF_UNDEFINED:
                                case EXIF_SBYTE:
                                        resultLen = tagCount;
                                        break;
                                case EXIF_SHORT:
                                case EXIF_SSHORT:
                                        resultLen = tagCount*2;
                                        break;
                                case EXIF_LONG:
                                case EXIF_SLONG:
                                case EXIF_FLOAT:
                                        resultLen = tagCount*4;
                                        break;
                                case EXIF_RATIONAL:
                                case EXIF_SRATIONAL:
                                case EXIF_DOUBLE:
                                        resultLen = tagCount*8;
                                        break;
                                default:
                                        break;
                        }
                        if (resultLen <= 4) {
                                fseek(fp_in,4,SEEK_CUR);
                                continue;
                        }
                        unsigned char offsetBytes[4] = {0x00,0x00,0x00,0x00};
                        fread(offsetBytes,1,4,fp_in);
                        u_int32_t offset = (offsetBytes[0]<<24)|(offsetBytes[1]<<16)|(offsetBytes[2]<<8)|offsetBytes[3];
                        if (isLittleEndian == 1) {
                                offset = (offsetBytes[3]<<24)|(offsetBytes[2]<<16)|(offsetBytes[1]<<8)|offsetBytes[0];
                        }
                        for (int j = 0; j < resultLen; j++) {
                                bytesWithScipMap[offset+j] = 0x01;
                        }
                }
        }

        printf("Карта построена: Успешно\n\n");
        if (!fp_in) {
                fp_in = fopen(new_filename, "rb");
        }
        fseek(fp_in, 0, SEEK_SET);
        unsigned char magicBytes[4] = {0x00,0x00,0x00,0x00};
        fread(magicBytes, 1, 4, fp_in);
        fwrite(magicBytes, 1, 4, fp_out);
        u_int32_t currentCounter = 4;
        
        //Запись нового IFD
        TIFFInfo* tempInfo = start->next;
        unsigned char currentOffsetChar[4] = {0x00,0x00,0x00,0x00};
        currentCounter += 4;
        currentOffsetChar[0] = currentCounter >> 24 & 0xFF;
        currentOffsetChar[1] = currentCounter >> 16 & 0xFF;
        currentOffsetChar[2] = currentCounter >> 8 & 0xFF;
        currentOffsetChar[3] = currentCounter & 0xFF;
        if (isLittleEndian == 1) {
                currentOffsetChar[0] = currentCounter & 0xFF;
                currentOffsetChar[1] = currentCounter >> 8 & 0xFF;
                currentOffsetChar[2] = currentCounter >> 16 & 0xFF;
                currentOffsetChar[3] = currentCounter >> 24 & 0xFF;
        }
        fwrite(currentOffsetChar, 1, 4, fp_out);
        while (tempInfo != NULL) {
                int currentIfdNumber = tempInfo->ifdNumber;
                TIFFInfo* tempInfo2 = tempInfo;
                u_int16_t counter = 0;
                while (tempInfo2 != NULL && tempInfo2->ifdNumber == currentIfdNumber) {
                
                        counter++;
                        tempInfo2 = tempInfo2->next;
                }
                unsigned char counterBytes[2] = {0x00,0x00};
                counterBytes[0] = counter >> 8 & 0xFF;
                counterBytes[1] = counter & 0xFF;
                if (isLittleEndian == 1) {
                        counterBytes[0] = counter & 0xFF;
                        counterBytes[1] = counter >> 8 & 0xFF;
                }
                fwrite(counterBytes, 1, 2, fp_out);
                currentCounter += 2;
                tempInfo2 = tempInfo;
                currentCounter += 12*counter+4;
                while (tempInfo2 != NULL && tempInfo2->ifdNumber == currentIfdNumber) {
                        unsigned char tagId[2] = {0x00,0x00};
                        tagId[0] = tempInfo2->tagType >> 8 & 0xFF;
                        tagId[1] = tempInfo2->tagType & 0xFF;
                        if (isLittleEndian == 1) {
                                tagId[0] = tempInfo2->tagType & 0xFF;
                                tagId[1] = tempInfo2->tagType >> 8 & 0xFF;
                        }
                        unsigned char tagFormat[2] = {0x00,0x00};
                        tagFormat[0] = tempInfo2->format >> 8 & 0xFF;
                        tagFormat[1] = tempInfo2->format & 0xFF;
                        
                        if (isLittleEndian == 1) {
                                tagFormat[0] = tempInfo2->format & 0xFF;
                                tagFormat[1] = tempInfo2->format >> 8 & 0xFF;
                        }
                        unsigned char tagCounter[4] = {0x00,0x00,0x00,0x00};
                        tagCounter[0] = (tempInfo2->structuresCount >> 24) & 0xFF;
                        tagCounter[1] = (tempInfo2->structuresCount >> 16) & 0xFF;
                        tagCounter[2] = (tempInfo2->structuresCount >> 8) & 0xFF;
                        tagCounter[3] = tempInfo2->structuresCount & 0xFF;
                        if (isLittleEndian == 1) {
                                tagCounter[0] = tempInfo2->structuresCount & 0xFF;
                                tagCounter[1] = (tempInfo2->structuresCount >> 8) & 0xFF;
                                tagCounter[2] = (tempInfo2->structuresCount >> 16) & 0xFF;
                                tagCounter[3] = (tempInfo2->structuresCount >> 24) & 0xFF;
                        }
                        fwrite(tagId, 1, 2, fp_out);
                        fwrite(tagFormat, 1, 2, fp_out);
                        fwrite(tagCounter, 1, 4, fp_out);

                        if (tempInfo2->dataLen <= 4) {
                                
                                if (isLittleEndian == 1) {
                                        fwrite(tempInfo2->data, 1, tempInfo2->dataLen, fp_out);
                                        if (tempInfo2->dataLen < 4) {
                                                unsigned char* nullBytesData = (unsigned char*)malloc(4-tempInfo2->dataLen);
                                                for (int i = 0; i < 4-tempInfo2->dataLen; i++) {
                                                        nullBytesData[i] = 0x00;
                                                }
                                                fwrite(nullBytesData, 1, 4-tempInfo2->dataLen, fp_out);
                                                free(nullBytesData);
                                        }
                                } else {
                                        if (tempInfo2->dataLen < 4) {
                                                unsigned char* nullBytesData = (unsigned char*)malloc(4-tempInfo2->dataLen);
                                                for (int i = 0; i < 4-tempInfo2->dataLen; i++) {
                                                        nullBytesData[i] = 0x00;
                                                }
                                                fwrite(nullBytesData, 1, 4-tempInfo2->dataLen, fp_out);
                                                free(nullBytesData);
                                        }
                                        fwrite(tempInfo2->data, 1, tempInfo2->dataLen, fp_out);   
                                }
                                tempInfo2 = tempInfo2->next;
                                continue;        
                        }

                        unsigned char metadataOfsset[4] = {0x00,0x00,0x00,0x00};
                        metadataOfsset[0] = (currentCounter >> 24) & 0xFF;
                        metadataOfsset[1] = (currentCounter >> 16) & 0xFF;
                        metadataOfsset[2] = (currentCounter >> 8) & 0xFF;
                        metadataOfsset[3] = currentCounter & 0xFF;
                        if (isLittleEndian == 1) {
                                metadataOfsset[0] = currentCounter & 0xFF;
                                metadataOfsset[1] = (currentCounter >> 8) & 0xFF;
                                metadataOfsset[2] = (currentCounter >> 16) & 0xFF;
                                metadataOfsset[3] = (currentCounter >> 24) & 0xFF;
                        }
                        fwrite(metadataOfsset, 1, 4, fp_out);
                        currentCounter += tempInfo2->dataLen;
                        tempInfo2 = tempInfo2->next;       
                }
                if (tempInfo2 == NULL) {
                        unsigned char nullBytes[4] = {0x00,0x00,0x00,0x00};
                        fwrite(nullBytes, 1, 4, fp_out);
                } else {
                        unsigned char nextOffset[4] = {0x00,0x00,0x00,0x00};
                        nextOffset[0] = (currentCounter >> 24) & 0xFF;
                        nextOffset[1] = (currentCounter >> 16) & 0xFF;
                        nextOffset[2] = (currentCounter >> 8) & 0xFF;
                        nextOffset[3] = currentCounter & 0xFF;
                        if (isLittleEndian == 1) {
                                nextOffset[0] = currentCounter & 0xFF;
                                nextOffset[1] = (currentCounter >> 8) & 0xFF;
                                nextOffset[2] = (currentCounter >> 16) & 0xFF;
                                nextOffset[3] = (currentCounter >> 24) & 0xFF;
                        }
                        fwrite(nextOffset, 1, 4, fp_out);
                }
                tempInfo2 = tempInfo;
                while (tempInfo2 != NULL && tempInfo2->ifdNumber == currentIfdNumber) {
                        if (tempInfo2->dataLen > 4) {
                                fwrite(tempInfo2->data, 1, tempInfo2->dataLen, fp_out);
                        }
                        tempInfo2 = tempInfo2->next;
                }
                if (tempInfo2 == NULL) {
                        break;
                }
                tempInfo = tempInfo2;
        }
        printf("IFD записан\n");
        currentCounter = 4;
        if (!fp_in) {
                fp_in = fopen(new_filename, "rb");
        }
        fseek(fp_in, 4, SEEK_SET);
        //Запись остального
        while (fp_in && fread(&currentChar, 1, 1, fp_in)) {
                if (bytesWithScipMap[currentCounter] != 0x01) {
                        fwrite(&currentChar, 1, 1, fp_out);
                }
                currentCounter++;
        }
        free(new_filename);
        free(bytesWithScipMap);
        return 0;
}

int append(ExifInfo* start, ExifInfo* appendingItem) {
    if (start == NULL) {
        appendingItem->next = NULL;
        return 1; 
    }

    ExifInfo* tempPointer = start;
    while (tempPointer->next != NULL) {
        tempPointer = tempPointer->next;
    }
    tempPointer->next = appendingItem;
    return 1;
}

void clearExifInfo(ExifInfo* info) {
        if (info == NULL) {
            return;
        }
        if (info->next != NULL) {
            clearExifInfo(info->next);
        }

        if (info->exifName != NULL) {
                free(info->exifName);
                info->exifName = NULL;
        }
        free(info);
}

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

typedef struct {
        char* header;
        unsigned char* data;
        ExifFormats format;
} CLIExifArgument;

int getExifArgumentFromCLI(CLIExifArgument* argument, int argc, char** argv) {
        for (int i = 1; i < argc; i++) {
                if (strcmp(argument->header,argv[i]) == 0) {
                        if (argc<i+4) {
                                continue;
                        }
                        if (strcmp("--type",argv[i+2])!=0) {
                                continue;
                        }
                        int dataType = atoi(argv[i+3]);
                        if (dataType<=0 || dataType>12) {
                                continue;
                        }
                        unsigned char* dynamicData = (unsigned char*)malloc((u_int32_t)strlen(argv[i+1])+1);
                        memcpy(dynamicData,argv[i+1],(u_int32_t)strlen(argv[i+1]));
                        dynamicData[strlen(argv[i+1])] = '\0';
                        argument->data = dynamicData;
                        argument->format = (ExifFormats)((unsigned char)dataType);
                        return 1;
                }
        }
        return -1;
}

CLIExifArgument constructorCLIExifArgument(char* header) {
        CLIExifArgument newCLIExifArg;
        newCLIExifArg.data = NULL;
        newCLIExifArg.header = header;
        return newCLIExifArg;
}

int getJFIFVersionArgument(int argc, char** argv, char* buffer) {
        for (int i = 0; i < argc; i++) {
                if (strcmp("--jfifVersion",argv[i]) == 0 && argc > i+1) {
                        int jfifVersion = atoi(argv[i+1]);
                        u_int16_t jfifUnsigned = (u_int16_t)jfifVersion;
                        unsigned char highByte = (jfifUnsigned >> 8) & 0xFF;
                        unsigned char lowByte = jfifUnsigned & 0xFF;
                        buffer[0] = highByte;
                        buffer[1] = lowByte;
                        return 0;
                }
        }
        return -1;
}


int argumentsCount(char* data) {
        int dataLen = strlen(data);
        int result = 1;
        for (int i = 0; i < dataLen; i++) {
                if (data[i] == ';') {
                        result++;
                }
        }
        return result;
}

int parseShortCLI(char* data, u_int16_t* numbersArray) {
        int realCounter = 0;
        u_int16_t tempNumber = 0;
        int dataLen = strlen(data);
        for (int i = 0; i < dataLen; i++) {
                if (data[i] == ';') {
                        numbersArray[realCounter] = tempNumber;
                        tempNumber = 0;
                        realCounter++;
                        continue;
                }
                tempNumber*=10;
                char currentElement[1] = {data[i]};
                if (currentElement[0] == '0') {
                        continue;
                }
                int currentNumber = atoi(currentElement);
                tempNumber+=(u_int16_t)currentNumber;
        }
        numbersArray[realCounter] = tempNumber;
        realCounter++;
        return realCounter;
}

int parseSShortCLI(char* data, int16_t* numbersArray) {
        int realCounter = 0;
        int16_t tempNumber = 0;
        int dataLen = strlen(data);
        int isNegative = 1;
        for (int i = 0; i < dataLen; i++) {
                if (data[i] == '-') {
                        isNegative = -1;
                        continue;
                }
                if (data[i] == ';') {
                        numbersArray[realCounter] = tempNumber*isNegative;
                        isNegative = 1;
                        tempNumber = 0;
                        realCounter++;
                        continue;
                }
                tempNumber*=10;
                char currentElement[1] = {data[i]};
                if (currentElement[0] == '0') {
                        continue;
                }
                int currentNumber = atoi(currentElement);
                tempNumber+=(int16_t)currentNumber;
        }
        numbersArray[realCounter] = tempNumber;
        realCounter++;
        return realCounter;
}



int parseLongCLI(char* data, u_int32_t* numbersArray) {
        int realCounter = 0;
        u_int32_t tempNumber = 0;
        int dataLen = strlen(data);
        for (int i = 0; i < dataLen; i++) {
                if (data[i] == ';') {
                        numbersArray[realCounter] = tempNumber;
                        tempNumber = 0;
                        realCounter++;
                        continue;
                }
                tempNumber*=10;
                char currentElement[1] = {data[i]};
                if (currentElement[0] == '0') {
                        continue;
                }
                int currentNumber = atoi(currentElement);
                tempNumber+=(u_int32_t)currentNumber;
        }
        numbersArray[realCounter] = tempNumber;
        realCounter++;
        return realCounter;
}

int parseSLongCLI(char* data, int32_t* numbersArray) {
        int realCounter = 0;
        int32_t tempNumber = 0;
        int dataLen = strlen(data);
        int isNegative = 1;
        for (int i = 0; i < dataLen; i++) {
                if (data[i] == '-') {
                        isNegative = -1;
                        continue;
                }
                if (data[i] == ';') {
                        numbersArray[realCounter] = tempNumber*isNegative;
                        isNegative = 1;
                        tempNumber = 0;
                        realCounter++;
                        continue;
                }
                tempNumber*=10;
                char currentElement[1] = {data[i]};
                if (currentElement[0] == '0') {
                        continue;
                }
                int currentNumber = atoi(currentElement);
                tempNumber+=(int32_t)currentNumber;
        }
        numbersArray[realCounter] = tempNumber;
        realCounter++;
        return realCounter;
}

int parseRationalCLI(char* data, u_int32_t* numbersArray) {
        int realCounter = 0;
        u_int32_t tempNumber = 0;
        int dataLen = strlen(data);
        int isDelim = -1;
        for (int i = 0; i < dataLen; i++) {
                if (data[i] == ';') {
                        numbersArray[realCounter] = tempNumber;
                        tempNumber = 0;
                        if (isDelim == -1) {
                                numbersArray[realCounter+1] = 1;
                                realCounter++;
                        }
                        isDelim = -1;
                        realCounter++;
                        continue;
                } 
                if (data[i] == '/') {
                        numbersArray[realCounter] = tempNumber;
                        tempNumber = 0;
                        isDelim = 1;
                        realCounter++;
                        continue;  
                }
                tempNumber*=10;
                char currentElement[1] = {data[i]};
                if (currentElement[0] == '0') {
                        continue;
                }
                int currentNumber = atoi(currentElement);
                tempNumber+=(u_int32_t)currentNumber;
        }
        numbersArray[realCounter] = tempNumber;
        if (isDelim == -1) {
                numbersArray[realCounter+1] = 1;
                realCounter++;
        }
        realCounter++;
        return realCounter;
}


int parseSRationalCLI(char* data, int32_t* numbersArray) {
        int realCounter = 0;
        int32_t tempNumber = 0;
        int dataLen = strlen(data);
        int isDelim = -1;
        int isNegative = 1;
        for (int i = 0; i < dataLen; i++) {
                if (data[i] == '-') {
                        isNegative = -1;
                        continue;
                }
                if (data[i] == ';') {
                        numbersArray[realCounter] = tempNumber*isNegative;
                        isNegative = 1;
                        tempNumber = 0;
                        if (isDelim == -1) {
                                numbersArray[realCounter+1] = 1;
                                realCounter++;
                        }
                        isDelim = -1;
                        realCounter++;
                        continue;
                } 
                if (data[i] == '/') {
                        numbersArray[realCounter] = tempNumber;
                        tempNumber = 0;
                        isDelim = 1;
                        realCounter++;
                        continue;  
                }
                tempNumber*=10;
                char currentElement[1] = {data[i]};
                if (currentElement[0] == '0') {
                        continue;
                }
                int currentNumber = atoi(currentElement);
                tempNumber+=(int32_t)currentNumber;
        }
        numbersArray[realCounter] = tempNumber;
        if (isDelim == -1) {
                numbersArray[realCounter+1] = 1;
                realCounter++;
        }
        realCounter++;
        return realCounter;
}


int parseFloatCLI(char* data, float* numbersArray) {
        int realCounter = 0;
        int tempCounter = 0;
        int prevPos = 0;
        int dataLen = strlen(data);
        for (int i=0; i < dataLen; i++) {
                if (data[i]!=';') {
                        tempCounter++;
                        continue;
                }
                tempCounter++;
                char* currentString = (char*)malloc(tempCounter);
                for (int j = prevPos+1;j<prevPos+tempCounter;j++) {
                        currentString[j-prevPos-1] = data[j];
                }
                currentString[tempCounter] = '\0';
                prevPos = i;
                tempCounter = 0;
                float result = (float)atof(currentString);
                numbersArray[realCounter] = result;
                realCounter++;
                free(currentString);
        }
        char* tempString = (char*)malloc(dataLen-prevPos);
        for (int i=prevPos+1;i < dataLen; i++) {
                tempString[i-prevPos-1] = data[i];
        }
        tempString[dataLen-prevPos-1] = '\0';
        float result = (float)atof(tempString);
        numbersArray[realCounter] = result;
        realCounter++;
        free(tempString);
        return realCounter;
}


int parseDoubleCLI(char* data, double* numbersArray) {
        int realCounter = 0;
        int tempCounter = 0;
        int prevPos = 0;
        int dataLen = strlen(data);
        for (int i=0; i < dataLen; i++) {
                if (data[i]!=';') {
                        tempCounter++;
                        continue;
                }
                tempCounter++;
                char* currentString = (char*)malloc(tempCounter);
                for (int j = prevPos+1;j<prevPos+tempCounter;j++) {
                        currentString[j-prevPos-1] = data[j];
                }
                currentString[tempCounter] = '\0';
                prevPos = i;
                tempCounter = 0;
                double result = atof(currentString);
                numbersArray[realCounter] = result;
                realCounter++;
                free(currentString);
        }
        char* tempString = (char*)malloc(dataLen-prevPos);
        for (int i=prevPos+1;i < dataLen; i++) {
                tempString[i-prevPos-1] = data[i];
        }
        tempString[dataLen-prevPos-1] = '\0';
        double result = atof(tempString);
        numbersArray[realCounter] = result;
        realCounter++;
        free(tempString);
        return realCounter;
}

void fillTIFFInfoFromCLI(CLIExifArgument argument, TIFFInfo* newNode, TIFFTags tagType, int isLittleEndian) {
        newNode->next = NULL;
        newNode->tagType = tagType;
        int count = 0;
        int realCounter = 0;
        unsigned char* dataArray = NULL;
        char* reversedBytes = NULL;
        switch (argument.format) {
                case EXIF_BYTE:
                case EXIF_ASCII:
                case EXIF_SBYTE:
                case EXIF_UNDEFINED:
                        newNode->data = argument.data;
                        newNode->dataLen = strlen(argument.data);
                        newNode->structuresCount = strlen(argument.data);
                        if (isLittleEndian == 1) {
                                reversedBytes = (char*)malloc(newNode->dataLen);
                                for (int j = newNode->dataLen-1; j >= 0; j--) {
                                        reversedBytes[newNode->dataLen-1-j] = newNode->data[newNode->dataLen-1-j];
                                }
                                newNode->data = reversedBytes;
                        } else {
                                reversedBytes = (char*)malloc(newNode->dataLen);
                                for (int j = newNode->dataLen-1; j >= 0; j--) {
                                        reversedBytes[newNode->dataLen-1-j] = newNode->data[newNode->dataLen-1-j];
                                }
                                newNode->data = reversedBytes;
                        }
                        break;
                case EXIF_SHORT:
                        count = argumentsCount(argument.data);
                        u_int16_t* elementsArrayUI16 = (u_int16_t*)malloc(count*2);
                        realCounter = parseShortCLI(argument.data,elementsArrayUI16);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*2); 
                        for (int i = 0; i < realCounter; i++) {
                                dataArray[2*i] = elementsArrayUI16[i]>>8 & 0xff;
                                dataArray[2*i+1] = elementsArrayUI16[i] & 0xff;
                                if (isLittleEndian == 1) {
                                        dataArray[2*i+1] = elementsArrayUI16[i]>>8 & 0xff;
                                        dataArray[2*i] = elementsArrayUI16[i] & 0xff;    
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter;
                        newNode->dataLen = 2*realCounter;
                        free(elementsArrayUI16);
                        break;
                case EXIF_LONG:
                        count = argumentsCount(argument.data);
                        u_int32_t* elementsArrayUI32 = (u_int32_t*)malloc(count*4);
                        realCounter = parseLongCLI(argument.data,elementsArrayUI32);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*4); 
                        for (int i = 0; i < realCounter; i++) {
                                dataArray[4*i] = elementsArrayUI32[i]>>24 & 0xff;
                                dataArray[4*i+1] = elementsArrayUI32[i]>>16 & 0xff;
                                dataArray[4*i+2] = elementsArrayUI32[i]>>8 & 0xff;
                                dataArray[4*i+3] = elementsArrayUI32[i] & 0xff;
                                if (isLittleEndian == 1) {
                                        dataArray[4*i+3] = elementsArrayUI32[i]>>24 & 0xff;
                                        dataArray[4*i+2] = elementsArrayUI32[i]>>16 & 0xff;
                                        dataArray[4*i+1] = elementsArrayUI32[i]>>8 & 0xff;
                                        dataArray[4*i] = elementsArrayUI32[i] & 0xff;
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter;
                        newNode->dataLen = 4*realCounter;
                        free(elementsArrayUI32);
                        break;
                case EXIF_RATIONAL:
                        count = argumentsCount(argument.data);
                        u_int32_t* elementsArrayRAT = (u_int32_t*)malloc(count*8);
                        realCounter = parseRationalCLI(argument.data,elementsArrayRAT);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*4); 
                        for (int i = 0; i < realCounter; i++) {
                                dataArray[4*i] = elementsArrayRAT[i]>>24 & 0xff;
                                dataArray[4*i+1] = elementsArrayRAT[i]>>16 & 0xff;
                                dataArray[4*i+2] = elementsArrayRAT[i]>>8 & 0xff;
                                dataArray[4*i+3] = elementsArrayRAT[i] & 0xff;
                                if (isLittleEndian == 1) {
                                        dataArray[4*i+3] = elementsArrayRAT[i]>>24 & 0xff;
                                        dataArray[4*i+2] = elementsArrayRAT[i]>>16 & 0xff;
                                        dataArray[4*i+1] = elementsArrayRAT[i]>>8 & 0xff;
                                        dataArray[4*i] = elementsArrayRAT[i] & 0xff;
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter/2;
                        newNode->dataLen = 4*realCounter;
                        free(elementsArrayRAT);
                
                case EXIF_SSHORT:
                        count = argumentsCount(argument.data);
                        int16_t* elementsArrayI16 = (int16_t*)malloc(count*2);
                        realCounter = parseSShortCLI(argument.data,elementsArrayI16);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*2); 
                        for (int i = 0; i < realCounter; i++) {
                                dataArray[2*i] = elementsArrayI16[i]>>8 & 0xff;
                                dataArray[2*i+1] = elementsArrayI16[i] & 0xff;
                                if (isLittleEndian == 1) {
                                        dataArray[2*i+1] = elementsArrayI16[i]>>8 & 0xff;
                                        dataArray[2*i] = elementsArrayI16[i] & 0xff;
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter;
                        newNode->dataLen = 2*realCounter;
                        free(elementsArrayI16);
                        break;
                case EXIF_SLONG:
                        count = argumentsCount(argument.data);
                        int32_t* elementsArrayI32 = (int32_t*)malloc(count*4);
                        realCounter = parseSLongCLI(argument.data,elementsArrayI32);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*4); 
                        for (int i = 0; i < realCounter; i++) {
                                dataArray[4*i] = elementsArrayI32[i]>>24 & 0xff;
                                dataArray[4*i+1] = elementsArrayI32[i]>>16 & 0xff;
                                dataArray[4*i+2] = elementsArrayI32[i]>>8 & 0xff;
                                dataArray[4*i+3] = elementsArrayI32[i] & 0xff;
                                if (isLittleEndian == 1) {
                                        dataArray[4*i+3] = elementsArrayI32[i]>>24 & 0xff;
                                        dataArray[4*i+2] = elementsArrayI32[i]>>16 & 0xff;
                                        dataArray[4*i+1] = elementsArrayI32[i]>>8 & 0xff;
                                        dataArray[4*i] = elementsArrayI32[i] & 0xff;
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter;
                        newNode->dataLen = 4*realCounter;
                        free(elementsArrayI32);
                        break;
                case EXIF_SRATIONAL:
                        count = argumentsCount(argument.data);
                        int32_t* elementsArraySRAT = (int32_t*)malloc(count*8);
                        realCounter = parseSRationalCLI(argument.data,elementsArraySRAT);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*4); 
                        for (int i = 0; i < realCounter; i++) {
                                dataArray[4*i] = elementsArraySRAT[i]>>24 & 0xff;
                                dataArray[4*i+1] = elementsArraySRAT[i]>>16 & 0xff;
                                dataArray[4*i+2] = elementsArraySRAT[i]>>8 & 0xff;
                                dataArray[4*i+3] = elementsArraySRAT[i] & 0xff;
                                if (isLittleEndian == 1) {
                                        dataArray[4*i+3] = elementsArraySRAT[i]>>24 & 0xff;
                                        dataArray[4*i+2] = elementsArraySRAT[i]>>16 & 0xff;
                                        dataArray[4*i+1] = elementsArraySRAT[i]>>8 & 0xff;
                                        dataArray[4*i] = elementsArraySRAT[i] & 0xff;
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter/2;
                        newNode->dataLen = 4*realCounter;
                        free(elementsArraySRAT);
                        break;
                case EXIF_FLOAT:
                        count = argumentsCount(argument.data);
                        float* elementsArrayFloat = (float*)malloc(count*4);
                        realCounter = parseFloatCLI(argument.data,elementsArrayFloat);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*4); 
                        for (int i = 0; i < realCounter; i++) {
                                u_int8_t bytes[sizeof(float)]; 
                                memcpy(bytes, &elementsArrayFloat[i], sizeof(float));
                                dataArray[4*i] = bytes[0];
                                dataArray[4*i+1] = bytes[1];
                                dataArray[4*i+2] = bytes[2];
                                dataArray[4*i+3] = bytes[3];
                                if (isLittleEndian == 1) {
                                        dataArray[4*i+3] = bytes[0];
                                        dataArray[4*i+2] = bytes[1];
                                        dataArray[4*i+1] = bytes[2];
                                        dataArray[4*i] = bytes[3];
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter;
                        newNode->dataLen = 4*realCounter;
                        free(elementsArrayFloat);
                        break;
                case EXIF_DOUBLE:
                        count = argumentsCount(argument.data);
                        double* elementsArrayDouble = (double*)malloc(count*8);
                        realCounter = parseDoubleCLI(argument.data,elementsArrayDouble);
                        free(argument.data);
                        dataArray = (unsigned char*)malloc(realCounter*8); 
                        for (int i = 0; i < realCounter; i++) {
                                u_int8_t bytes[sizeof(double)]; 
                                memcpy(bytes, &elementsArrayDouble[i], sizeof(double));
                                dataArray[8*i] = bytes[0];
                                dataArray[8*i+1] = bytes[1];
                                dataArray[8*i+2] = bytes[2];
                                dataArray[8*i+3] = bytes[3];
                                dataArray[8*i+4] = bytes[4];
                                dataArray[8*i+5] = bytes[5];
                                dataArray[8*i+6] = bytes[6];
                                dataArray[8*i+7] = bytes[7];
                                if (isLittleEndian == 1) {
                                        dataArray[8*i+7] = bytes[0];
                                        dataArray[8*i+6] = bytes[1];
                                        dataArray[8*i+5] = bytes[2];
                                        dataArray[8*i+4] = bytes[3];
                                        dataArray[8*i+3] = bytes[4];
                                        dataArray[8*i+2] = bytes[5];
                                        dataArray[8*i+1] = bytes[6];
                                        dataArray[8*i] = bytes[7];
                                }
                        }
                        newNode->data = dataArray;
                        newNode->structuresCount = realCounter;
                        newNode->dataLen = 8*realCounter;
                        free(elementsArrayDouble);
                        break;
        }
        newNode->format = argument.format;
}




u_int16_t countExifTags(ExifInfo* startNode) {
        u_int16_t result = 0;
        ExifInfo* next = startNode->next;
        while (next!=NULL) {
                result++;
                next = next->next;
        }
        return result;
}


/*

        ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

*/

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
        if (tag!=EXIF_USERCOMMENT && tag!=EXIF_MAKE && tag!=EXIF_MODEL && tag!=EXIF_EXPOSURE && tag!=EXIF_FNUMBER && tag!=EXIF_ISOSPEEDRATING && tag!=EXIF_GPSLATITUDE && tag!=EXIF_GPSLONGITUDE && tag !=EXIF_GPSLATITUDEREF && tag!=EXIF_GPSLONGITUDEREF && tag!=EXIF_DATETIME && tag!=EXIF_IMAGEDESCRIPTION) {
                return 0;
        }
        return 1;
}

/*

	МОДУЛЬ ЧТЕНИЯ

*/

int readMetadataPNG(FILE* fp_in) {
        fseek(fp_in, 8, SEEK_SET);
        unsigned char lengthBytes[4] = {0x00,0x00,0x00,0x00};
        unsigned char tagName[4] = {0x00,0x00,0x00,0x00};
        int result = 0;
        u_int32_t  length = 0;
        while(fp_in && fread(lengthBytes,1,4,fp_in) == 4) {
                result = fread(tagName,1,4,fp_in);
                if (result!=4 || !fp_in) {
                        break;
                }
        
                length = (lengthBytes[0]<<24)|(lengthBytes[1]<<16)|(lengthBytes[2]<<8)|(lengthBytes[3]);
                if ((tagName[0] == 'I' && tagName[1] == 'D' && tagName[2] == 'A' && tagName[3] == 'T')||(tagName[0] == 'I' && tagName[1] == 'E' && tagName[2] == 'N' && tagName[3] == 'D')) {
                        fseek(fp_in,length+4,SEEK_CUR);
                        continue;
                }
                unsigned char* tagData = (unsigned char*)malloc(length);
                if (tagData == NULL) {
                        fseek(fp_in,length+4,SEEK_CUR);
                        continue;
                }
                result = fread(tagData,1,length,fp_in);
                if (result<length || !fp_in) {
                        free(tagData);
                        if (fp_in) {
                                fseek(fp_in,4,SEEK_CUR);
                        }
                        continue;
                }
                if (tagName[0] == 'I' && tagName[1] == 'H' && tagName[2] == 'D' && tagName[3] == 'R') {
                        if (length<13) {
                                printf("Предупреждение: Найден IHDR с длиной меньше 13 байт. Не был считан в стандартном формате.\n");
                                printf("\tДанные этого IHDR в байтах:");
                                for (int i=0; i<length;i++) {
                                        printf(" %02x", tagData[i]);
                                }
                                printf("\n");
                                free(tagData);
                                fseek(fp_in,4,SEEK_CUR);
                                continue;
                        }
                        printf("IHDR:\n");
                        u_int32_t width = (tagData[0]<<24)|(tagData[1]<<16)|(tagData[2]<<8)|(tagData[3]);
                        u_int32_t height = (tagData[4]<<24)|(tagData[5]<<16)|(tagData[6]<<8)|(tagData[7]);
                        printf("\tШирина изображения в байтах: %02x %02x %02x %02x\n", tagData[0],tagData[1], tagData[2], tagData[3]);
                        printf("\tШирина изображения: %d\n", width);
                        printf("\tВысота изображения в байтах: %02x %02x %02x %02x\n", tagData[4],tagData[5], tagData[6], tagData[7]);
                        printf("\tВысота изображения: %d\n", height);
                        u_int8_t dephtInt = (u_int8_t)(tagData[8]);
                        printf("\tГлубина цвета в байтах: %02x\n", tagData[8]);
                        printf("\tГлубина цвета: %u\n",dephtInt);
                        printf("\tЦветовой тип в байтах: %02x\n", tagData[9]);
                        switch (tagData[9]) {
                                case 0x00:
                                        printf("\tЦветовой тип: Оттенки серого\n");
                                        break;
                                case 0x02:
                                        printf("\tЦветовой тип: RGB\n");
                                        break;
                                case 0x03:
                                        printf("\tЦветовой тип: Палитровые цвета\n");
                                        break;
                                case 0x04:
                                        printf("\tЦветовой тип: Оттенки серого с альфа-каналом\n");
                                        break;
                                case 0x06:
                                        printf("\tЦветовой тип: RGB с альфа-каналом\n");
                                        break;
                                default:
                                       printf("\tЦветовой тип: Неизвестно\n"); 
                                       break;

                        }

                        printf("\tТип сжатия в байтах: %02x\n", tagData[10]);
                        switch (tagData[10]) {
                                case 0x00:
                                        printf("\tТип сжатия: Deflate самый быстрый\n");
                                        break;
                                case 0x01:
                                        printf("\tТип сжатия: Deflate быстрый\n");
                                        break;
                                case 0x02:
                                        printf("\tТип сжатия: Deflate по умолчанию\n");
                                        break;
                                case 0x03:
                                        printf("\tТип сжатия: Deflate максимальное сжатие\n");
                                        break;
                                default:
                                       printf("\tТип сжатия: Неизвестно\n"); 
                                       break;
                        }

                        printf("\tМетод фильрации в байтах: %02x\n", tagData[11]);
                        switch (tagData[11]) {
                                case 0x00:
                                        printf("\tМетод фильрации: Нет\n");
                                        break;
                                case 0x01:
                                        printf("\tМетод фильрации: Вычитание левого(Sub)\n");
                                        break;
                                case 0x02:
                                        printf("\tМетод фильрации: Вычитание верхнего(Up)\n");
                                        break;
                                case 0x03:
                                        printf("\tМетод фильрации: Вычитание среднего(Average)\n");
                                        break;
                                case 0x04:
                                        printf("\tМетод фильрации: алгоритм Paeth\n");
                                        break;
                                default:
                                       printf("\tМетод фильтрации: Неизвестно\n"); 
                                       break;
                        }
                        printf("\tInterlace в байтах: %02x\n",tagData[12]);
                        switch (tagData[12]) {
                                case 0x00:
                                        printf("\tInterlace: Нет\n");
                                        break;
                                case 0x01:
                                        printf("\tInterlace: Adam7\n");
                                        break;
                                default:
                                       printf("\tInterlace: Неизвестно\n"); 
                                       break;
                        }
                        if (length>13) {
                                printf("\tОстальные байты относящиеся к IHDR:");
                                for (int i = 13; i < length; i++) {
                                        printf(" %02x", tagData[i]);
                                }
                                printf("\n");
                        }
                } else if (tagName[0] == 'p' && tagName[1] == 'H' && tagName[2] == 'Y' && tagName[3] == 's') {
                        if (length<9) {
                                printf("Предупреждение: Найден pHYs с длиной меньше 9 байт. Не был считан в стандартном формате.\n");
                                printf("\tДанные этого pHYs в байтах:");
                                for (int i=0; i<length;i++) {
                                        printf(" %02x", tagData[i]);
                                }
                                printf("\n");
                                free(tagData);
                                fseek(fp_in,4,SEEK_CUR);
                                continue;
                        }
                        printf("pHYs:\n");
                        u_int32_t horizontal = (tagData[0]<<24)|(tagData[1]<<16)|(tagData[2]<<8)|(tagData[3]);
                        printf("\tГоризонтальное разрешение в байтах: %02x %02x %02x %02x\n", tagData[0], tagData[1], tagData[2], tagData[3]);
                        printf("\tГоризонтальное разрешение: %d\n", horizontal);
                        u_int32_t vertical = (tagData[4]<<24)|(tagData[5]<<16)|(tagData[6]<<8)|(tagData[7]);
                        printf("\tВертикальное разрешение в байтах: %02x %02x %02x %02x\n", tagData[4], tagData[5], tagData[6], tagData[7]);
                        printf("\tВертикальное разрешение: %d\n", vertical);
                        printf("\tЕдиницы измерения в байтах: %02x\n", tagData[8]);
                        switch (tagData[8]){
                                case 0:
                                        printf("\tЕдиницы измерения: Не указано(предположительно соотношение сторон)\n");
                                        break;
                                case 1:
                                        printf("\tЕдиницы измерения: Метры\n");
                                        break;
                                default:
                                        printf("\tЕдиницы измерения: Неизвестно\n");
                                        break;
                        }
                        if (length>9) {
                                printf("Остальные байты относящиеся к pHYs:");
                                for (int i = 9; i < length; i++) {
                                        printf(" %02x", tagData[i]);
                                }
                                printf("\n");
                        }
                } else if (tagName[0] == 't' && tagName[1] == 'E' && tagName[2] == 'X' && tagName[3] == 't') {
                        printf("Найден комментарий:\n");
                        printf("\tЗначение комментария в байтах:");
                        for (int i = 0; i < length; i++) {
                                printf(" %02x", tagData[i]);
                        }
                        printf("\n");
                        printf("\tКомментарий: ");
                        for (int i = 0; i < length; i++) {
                                if (tagData[i] == 0x00) {
                                        printf(":");
                                        continue;
                                }
                                printf("%c", tagData[i]);
                        }
                        printf("\n");
                } else {
                        printf("Предупреждение! Найден неизвестный тег:\n");
                        printf("\t Название тега в байтах: %02x %02x %02x %02x\n", tagName[0], tagName[1], tagName[2], tagName[3]);
                        printf("\t Название тега: %c %c %c %c\n", tagName[0], tagName[1], tagName[2], tagName[3]);
                        printf("\t Данные тега в байтах:");
                        for (int i = 0; i < length; i++) {
                                printf(" %02x", tagData[i]);
                        }
                        printf("\n");
                }
                free(tagData);
                if (fp_in) {
                        fseek(fp_in,4,SEEK_CUR);
                }
                
        }
        if (fp_in) fclose(fp_in);
}

int readMetadataGIF(FILE* fp_in) {
        unsigned char width[2] = {0x00};
        unsigned char height[2] = {0x00};
        unsigned char flags = 0x00;
        unsigned char colorIndex = 0x00;
        unsigned char resolution = 0x00;
        fread(width, 1, 2, fp_in);
        fread(height, 1, 2, fp_in);
        fread(&flags, 1, 1, fp_in);
        fread(&colorIndex,1,1,fp_in);
        fread(&resolution,1,1,fp_in);
        printf("Ширина холста в байтах: %02x %02x\n", width[0],width[1]);
        printf("Ширина холста: %d\n", (u_int16_t)(width[0]|(width[1]<<8)));
        printf("Высота холста в байтах: %02x %02x\n", height[0],height[1]);
        printf("Высота холста: %d\n", (u_int16_t)(height[0]|(height[1]<<8)));
        printf("Флаги: %02x\n", flags);
        printf("Индекс цвета в таблице в байтах: %02x\n", colorIndex);
        printf("Индекс цвета в таблице: %d\n", (u_int8_t)colorIndex);
        printf("Соотношение сторон пикселя в байтах: %02x\n", resolution);
        printf("Соотношение сторон пикселя: %d\n", (u_int8_t)resolution);
        unsigned char currentByte = 0x00;
        while(fp_in && fread(&currentByte, 1, 1, fp_in) == 1) {
                if (currentByte != 0x21) {
                        continue;
                }
                unsigned char nextByte = 0x00;
                fread(&nextByte, 1, 1, fp_in);
                if (nextByte != 0xF9 && nextByte!=0x01 && nextByte!=0xfe && nextByte!=0xff) {
                        continue;
                }
                if (nextByte == 0xfe) {
                        u_int8_t length = 0x00;
                        fread(&length, 1, 1, fp_in);
                        unsigned char* buffer = (unsigned char*)malloc(length);
                        fread(buffer, 1, length, fp_in);
                        printf("Найден комментарий в байтах:");
                        for (int i = 0; i < length; i++) {
                                printf(" %02x", buffer[i]);
                        }
                        printf("\n");
                        printf("Найден комментарий: %s\n", buffer);
                        free(buffer);
                        continue;
                }
                if (nextByte == 0x01) {
                        u_int8_t length = 0x00;
                        fread(&length, 1, 1, fp_in);
                        unsigned char* buffer = (unsigned char*)malloc(length);
                        fread(buffer, 1, length, fp_in);
                        printf("Найдены текстовые метаданные:\n");
                        printf("\tМестоположение в байтах: %02x %02x от левого края %02x %02x от верхнего края\n", buffer[0],buffer[1],buffer[2],buffer[3]);
                        printf("\tМестоположение: %d от левого края %d от верхнего края\n",(u_int16_t)(buffer[0]|buffer[1]<<8),(u_int16_t)(buffer[2]|buffer[3]<<8));
                        printf("\tШирина области в байтах: %02x %02x\n", buffer[4], buffer[5]);
                        printf("\tШирина области: %d\n", (u_int16_t)(buffer[4]|buffer[5]<<8));
                        printf("\tВысота области в байтах: %02x %02x\n", buffer[6], buffer[7]);
                        printf("\tВысота области: %d\n", (u_int16_t)(buffer[6]|buffer[7]<<8));
                        printf("\tШирина и высота текста в байтах: %02x %02x\n", buffer[8],buffer[9]);
                        printf("\tШирина и высота текста: %dx%d\n", (u_int8_t)(buffer[8]),(u_int8_t)(buffer[9]));
                        printf("\tЦвет текста в байтах: %02x\n", buffer[10]);
                        printf("\tЦвет текста: %d\n",(u_int8_t)buffer[10]);
                        printf("\tЦвет фона в байтах: %02x\n", buffer[11]);
                        printf("\tЦвет фона: %d\n",(u_int8_t)buffer[11]);
                        free(buffer);
                        fread(&length,1,1,fp_in);
                        unsigned char* textBuffer = (unsigned char*)malloc(length);
                        fread(textBuffer, 1, length, fp_in);
                        printf("\tТекст в байтах:");
                        for (int i = 0; i < length; i++) {
                                printf(" %02x", buffer[i]);
                        }
                        printf("\n");
                        printf("\tТекст: %s\n", textBuffer);
                        free(textBuffer);
                        continue;
                }
                if (nextByte == 0xff) {
                        printf("Найдены данные приложения:\n");
                        u_int8_t length = 0;
                        fread(&length,1,1,fp_in);
                        unsigned char* buffer = (unsigned char*)malloc(length);
                        printf("\tДанные о приложении в байтах:");
                        for (int i = 0; i < length; i++) {
                                printf(" %02x", buffer[i]);
                        }
                        printf("\n");
                        printf("\tДанные о приложении: %s\n", buffer);
                        free(buffer);
                        fread(&length,1,1,fp_in);
                        if (length == 0) {
                                continue;
                        }
                        buffer = (unsigned char*)malloc(length);
                        printf("\tДанные в байтах:");
                        for (int i = 0; i < length; i++) {
                                printf(" %02x", buffer[i]);
                        }
                        printf("\n");
                        printf("\tДанные: %s\n", buffer);
                        free(buffer);
                        continue;
                }
                u_int8_t length = 0;
                fread(&length,1,1,fp_in);
                unsigned char* buffer = (unsigned char*)malloc(length);
                fread(buffer, 1, length, fp_in);
                printf("Найден Graphic Control Extension:");
                printf("\tФлаги в байтах: %02x\n", buffer[0]);
                printf("\tЗадержка в сотых секунды в байтах: %02x %02x\n", buffer[1], buffer[2]);
                printf("\tЗадержка в сотых: %d\n", (u_int16_t)(buffer[1]|buffer[2]<<8));
                printf("\tИндекс прозрачного цвета в байтах: %02x\n", buffer[3]);
                printf("\tИндекс прозрачного цвета: %d\n", (u_int8_t)buffer[3]);
                free(buffer);
                
        }
}

int readMetadataTIFF(FILE* fp_in, int isLittleEndian, int withClear, TIFFInfo* start) {
        if (!fp_in) {
                printf("Ошибка: не удалось считать из файла\n");
                return 1;
        }
        if (start == NULL) start = initTiffInfo();
        int ifdCounter = 0;
        unsigned char nextIFDOffsetBytes[4] = {0x00, 0x00, 0x00, 0x00};
        while (fread(nextIFDOffsetBytes,1,4,fp_in) == 4) {
                if (nextIFDOffsetBytes[0] == 0x00 && nextIFDOffsetBytes[1] == 0x00 && nextIFDOffsetBytes[2] == 0x00 && nextIFDOffsetBytes[3] == 0x00) {
                        break;
                }
                ifdCounter++;
                u_int32_t nextIFDOffset = (nextIFDOffsetBytes[0]<<24)|(nextIFDOffsetBytes[1]<<16)|(nextIFDOffsetBytes[2]<<8)|nextIFDOffsetBytes[3];
                if (isLittleEndian == 1) {
                        nextIFDOffset = (nextIFDOffsetBytes[3]<<24)|(nextIFDOffsetBytes[2]<<16)|(nextIFDOffsetBytes[1]<<8)|nextIFDOffsetBytes[0];
                }
                fseek(fp_in,nextIFDOffset,SEEK_SET);
                unsigned char tagCounter[2] = {0x00,0x00};
                int result = fread(tagCounter,1,2,fp_in);
                if (result < 2 || !fp_in) {
                        break;
                }
                u_int16_t tagCount = (tagCounter[0]<<8)|tagCounter[1];
                if (isLittleEndian == 1) {
                        tagCount = (tagCounter[1]<<8)|tagCounter[0];
                }
                long ifdLen = 0;
                long ifdStart = ftell(fp_in);
                for (u_int16_t i = 0; i < tagCount; i++) {
                        TIFFInfo* tagInfo = initTiffInfo();
                        unsigned char readTag[2] = {0x00,0x00};
                        result = fread(readTag,1,2,fp_in);
                        if (result < 2 || !fp_in) {
                                free(tagInfo);
                                continue;
                        }
                        u_int16_t tag = (readTag[0]<<8)|readTag[1];
                        if (isLittleEndian == 1) {
                                tag = (readTag[1]<<8)|readTag[0];
                        }
                        
                        tagInfo->tagType = (TIFFTags)tag;
                        unsigned char tagFormat[2] = {0x00,0x00};
                        result = fread(tagFormat,1,2,fp_in);
                        if (result < 2 || !fp_in) {
                                free(tagInfo);
                                continue;
                        }
                        u_int16_t format = (tagFormat[0]<<8)|tagFormat[1];
                        if (isLittleEndian == 1) {
                                format = (tagFormat[1]<<8)|tagFormat[0];
                        }
                        tagInfo->format = (ExifFormats)format;
                        unsigned char tagLength[4] = {0x00,0x00,0x00,0x00};
                        result = fread(tagLength,1,4,fp_in);
                        if (result < 4 || !fp_in) {
                                free(tagInfo);
                                continue;
                        }
                        u_int32_t length = (tagLength[0]<<24)|(tagLength[1]<<16)|(tagLength[2]<<8)|tagLength[3];
                        
                        if (isLittleEndian == 1) {
                                length = (tagLength[3]<<24)|(tagLength[2]<<16)|(tagLength[1]<<8)|tagLength[0];
                        }
                        tagInfo->structuresCount = length;
                        u_int32_t resultLenght = length;
                        switch (tagInfo->format) {
                                case EXIF_ASCII:
                                case EXIF_BYTE:
                                case EXIF_UNDEFINED:
                                case EXIF_SBYTE:
                                case EXIF_DATETIME:
                                        break;
                                case EXIF_SHORT:
                                case EXIF_SSHORT:
                                        resultLenght = length*2;
                                        break;
                                case EXIF_LONG:
                                case EXIF_SLONG:
                                case EXIF_FLOAT:
                                        resultLenght = length*4;
                                        break;
                                case EXIF_RATIONAL:
                                case EXIF_SRATIONAL:
                                case EXIF_DOUBLE:
                                        resultLenght = length*8;
                                        break;
                                default:
                                        resultLenght = 0;
                                        break;
                        }
                        if (resultLenght == 0) {
                                free(tagInfo);
                                continue;
                        }
                        ifdLen += 12;
                        tagInfo->dataLen = resultLenght;
                        unsigned char* data = (unsigned char*)malloc(resultLenght);
                        if (resultLenght <= 4) {
                                result = fread(data,1,resultLenght,fp_in);
                                if (resultLenght<4) {
                                        fseek(fp_in,4-resultLenght,SEEK_CUR);
                                }
                        } else {
                               unsigned char offsetBytes[4] = {0x00};
                               result = fread(offsetBytes,1,4,fp_in);
                               long currentPos = ftell(fp_in);
                               u_int32_t offset = (offsetBytes[0]<<24)|(offsetBytes[1]<<16)|(offsetBytes[2]<<8)|offsetBytes[3];
                               if (isLittleEndian == 1) {
                                       offset = (offsetBytes[3]<<24)|(offsetBytes[2]<<16)|(offsetBytes[1]<<8)|offsetBytes[0];
                               }
                               fseek(fp_in,offset,SEEK_SET);
                               result = fread(data,1,resultLenght,fp_in);
                               fseek(fp_in,currentPos,SEEK_SET);
                        }
                        if (result < resultLenght || !fp_in) {
                                free(tagInfo);
                                free(data);
                                continue;
                        }
                        tagInfo->data = data;
                        tagInfo->ifdNumber = ifdCounter;
                        appendTiff(start,tagInfo);
                               
                }
                fseek(fp_in,ifdStart+ifdLen,SEEK_SET);
                ifdCounter++;
        }
        if (withClear) printTIFFInfo(start, isLittleEndian);
        if (withClear) clearTIFFInfo(start);
        if (withClear) fclose(fp_in);
        return 0;
}


int readMetadataJPEG(FILE* fp_in, char* filename) {
        unsigned char currentByte = 0x00;
        if (!fp_in) {
                return 1;
        }
        int tiffCounter = 0;
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
                        tiffCounter++;
                        fseek(fp_in,2,SEEK_CUR);
                        appTagLen = appTagLen - 6;
                        printf("Найден EXIF %d-й маркер\n",tiffCounter);
                        char* newFilename = (char*)malloc(strlen(filename)+strlen("_exif_copy")+1);
                        strcpy(newFilename,filename);
                        strcat(newFilename,"_exif_copy");
                        FILE* tempFile = fopen(newFilename,"wb");
                        TIFFInfo* start = initTiffInfo();
                        unsigned char isLittleEndianBytes[2] = {0};
                        fread(isLittleEndianBytes,1,2,fp_in);
                        int isLittleEndian = 0;
                        if (isLittleEndianBytes[0] == 0x49 && isLittleEndianBytes[1] == 0x49) {
                                isLittleEndian = 1;
                        }
                        fwrite(isLittleEndianBytes,1,2,tempFile);
                        for (int i = 2; i < appTagLen; i++) {
                                fread(&currentByte,1,1,fp_in);
                                fwrite(&currentByte,1,1,tempFile);
                        }
                        if (tempFile) fclose(tempFile);
                        tempFile = fopen(newFilename,"rb");
                        fseek(tempFile,4,SEEK_SET);
                        readMetadataTIFF(tempFile, isLittleEndian, 0, start);
                        printTIFFInfo(start, isLittleEndian);
                        printf("\n");
                        clearTIFFInfo(start);
                        if (tempFile) fclose(tempFile);
                        remove(newFilename);
                        free(newFilename);
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
                        printf("Найден комментарий: ");
                        for (int i = 0; i< length; i++) {
                                if (comment[i] == '\0') {
                                        continue;
                                }
                                printf("%c",comment[i]);
                        }
                        if (comment!=NULL) {
                                free(comment);
                                comment = NULL;
                        }
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
        readMetadataJPEG(fp_in,filename);
    } else if (headerBytes[0] == 0x49 && headerBytes[1] == 0x49 && headerBytes[2] == 0x2a && headerBytes[3] == 0x00) {
        printf("\nВнутренние данные файла\n");
        printf("-----------------------\n");
        printf("Тип файла: TIFF \n");
        printf("MIME Тип: image/tiff\n");
        printf("Байтовый порядок: Little-endian\n");
        readMetadataTIFF(fp_in, 1, 1, NULL);
    } else if (headerBytes[0] == 0x4d && headerBytes[1] == 0x4d && headerBytes[2] == 0x00 && headerBytes[3] == 0x2a) {
        printf("\nВнутренние данные файла\n");
        printf("-----------------------\n");
        printf("Тип файла: TIFF \n");
        printf("MIME Тип: image/tiff\n");
        printf("Байтовый порядок: Big-endian\n");
        readMetadataTIFF(fp_in, 0,1, NULL);
    } else if (headerBytes[0] == 0x47 && headerBytes[1] == 0x49 && headerBytes[2] == 0x46) {
        printf("\nВнутренние данные файла\n");
        printf("-----------------------\n");
        printf("Тип файла: GIF \n");
        printf("MIME Тип: image/GIF\n");
        printf("Версия: %c", headerBytes[3]);
        unsigned char otherVersionBytes[2] = {0x00};
        fread(otherVersionBytes,1,2,fp_in);
        printf("%c%c\n", otherVersionBytes[0], otherVersionBytes[1]);
        readMetadataGIF(fp_in);
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

    new_times[0].tv_sec = mktime(&tm_atime);
    new_times[0].tv_nsec = 0;
    new_times[1].tv_sec = mktime(&tm_mtime);
    new_times[1].tv_nsec = 0;
    struct timespec new_system_time;
    new_system_time.tv_sec = mktime(&tm_ctime);
    new_system_time.tv_nsec = 0;
    if (clock_settime(CLOCK_REALTIME, &new_system_time)) {
        perror("\nОшибка при установке системного времени\n");
        return 1;
    }
     
    if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
        printf("\nОшибка при изменении временных меток файла\n");
    } else {
        printf("\nВременные метки файла успешно изменены.\n");
    }
    if (clock_settime(CLOCK_REALTIME, &current_time)) {
        perror("Ошибка при восстановлении системного времени");
        return 1;
    }

    return 0;
}




/*

	МОДУЛЬ УДАЛЕНИЯ

*/
void deleteFromExifInfo(ExifInfo* startNode, ExifTags tag) {
        ExifInfo* nextNode = startNode->next;
        ExifInfo* curNode = startNode;
        while (nextNode!=NULL) {
                if (nextNode->tagType == tag) {
                        curNode->next = nextNode->next;
                        nextNode->next = NULL;
                        clearExifInfo(nextNode);
                        nextNode = curNode->next;
                        continue;
                }
                curNode = nextNode;
                nextNode = nextNode->next;
        }
}
int foundExifTagInCLI(int argc, char** argv, char* header) {
        for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], header) == 0) {
                        return 1;
                }
        } 
        return 0;
}

int deleteMetadataTIFF(FILE* fp_in, char* header, char* filename, int argc, char** argv, int isLittleEndian) {
        TIFFInfo* startPoint = initTiffInfo();
        int make = foundExifTagInCLI(argc,argv,"--make");
        int model = foundExifTagInCLI(argc,argv,"--model");
        int exposure = foundExifTagInCLI(argc,argv,"--exposure");
        int FNumber = foundExifTagInCLI(argc,argv,"--fnumber");
        int ISR = foundExifTagInCLI(argc,argv,"--isr");
        int userComment = foundExifTagInCLI(argc,argv,"--usercomment");
        int latitude = foundExifTagInCLI(argc,argv,"--lat");
        int longitude = foundExifTagInCLI(argc,argv,"--lon");
        int latitudeRef = foundExifTagInCLI(argc,argv,"--latRef");
        int longitudeRef = foundExifTagInCLI(argc,argv,"--lonRef");
        int datetime = foundExifTagInCLI(argc,argv,"--dt");
        int imageDescription = foundExifTagInCLI(argc,argv,"--imageDescription");
        fseek(fp_in,4,SEEK_SET);
        readMetadataTIFF(fp_in, isLittleEndian, 0, startPoint);

        if (make != 0) {
                deleteTiffTag(startPoint, TIFF_MAKE);
        }
        if (model != 0) {
                deleteTiffTag(startPoint, TIFF_MODEL);
        }
        if (exposure != 0) {
                deleteTiffTag(startPoint, TIFF_EXPOSURETIME);
        }
        if (FNumber != 0) {
                deleteTiffTag(startPoint, TIFF_FNUMBER);
        }
        if (ISR != 0) {
                deleteTiffTag(startPoint, TIFF_ISOSPEEDRATING);
        }
        if (userComment != 0) {
                deleteTiffTag(startPoint, TIFF_USERCOMENT);
        }
        if (latitude != 0) {
                deleteTiffTag(startPoint, TIFF_LATITUDE);
        }
        if (longitude != 0) {
                deleteTiffTag(startPoint, TIFF_LONGITUDE);
        }
        if (latitudeRef != 0) {
                deleteTiffTag(startPoint, TIFF_LATITUDEREF);
        }
        if (longitudeRef != 0) {
                deleteTiffTag(startPoint, TIFF_LONGITUDEREF);
        }
        if (datetime != 0) {
                deleteTiffTag(startPoint, TIFF_DATETIME);
        }
        if (imageDescription != 0) {
                deleteTiffTag(startPoint, TIFF_IMAGEDESCRIPTION);
        }
        rewriteTIFF(startPoint, filename, "delete", fp_in);
        clearTIFFInfo(startPoint);
}

int deleteMetadataJPEG(FILE* fp_in, char* header, char* filename, int argc, char** argv){
        int make = foundExifTagInCLI(argc,argv,"--make");
        int model = foundExifTagInCLI(argc,argv,"--model");
        int exposure = foundExifTagInCLI(argc,argv,"--exposure");
        int FNumber = foundExifTagInCLI(argc,argv,"--fnumber");
        int ISR = foundExifTagInCLI(argc,argv,"--isr");
        int userComment = foundExifTagInCLI(argc,argv,"--usercomment");
        int latitude = foundExifTagInCLI(argc,argv,"--lat");
        int longitude = foundExifTagInCLI(argc,argv,"--lon");
        int latitudeRef = foundExifTagInCLI(argc,argv,"--latRef");
        int longitudeRef = foundExifTagInCLI(argc,argv,"--lonRef");
        int datetime = foundExifTagInCLI(argc,argv,"--dt");
        int imageDescription = foundExifTagInCLI(argc,argv,"--imageDescription");
        int alreadyDeleted = -1;
        int alreadyDeletedComment = -1;
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
                return 1;
        }
        while(fread(&currentByte, 1, 1, fp_in) == 1){
                fwrite(&currentByte, 1, 1, fp_out);
        }
        fclose(fp_out);
        fclose(fp_in);
        printf("Копирование исходного файла в %s_delete_copy завершено\n", filename);
        printf("Удаление метаданных\n");
        fp_in = fopen(new_filename, "rb");
        if (!fp_in) {
                printf("Ошибка открытия файла для чтения\n");
                return 1;
        }
        fp_out = fopen(filename, "wb");
        if (!fp_out) {
                printf("Ошибка открытия файла для записи\n");
                return 1;
        }
        while (fp_in && fread(&currentByte,1,1,fp_in) == 1) {
                if (currentByte == 0xff) {
                        unsigned char nextByte = 0x00;
                        int result = fread(&nextByte,1,1,fp_in);
                        if (result!=1 || !fp_in) {
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        if (nextByte == 0xfe && header!=NULL && alreadyDeletedComment==-1) {
                                unsigned char markerLenBytes[2] = {0x00,0x00};
                                fread(markerLenBytes,1,2,fp_in);
                                u_int16_t markerLen = (markerLenBytes[0]<<8) | (markerLenBytes[1]);
                                markerLen-=2;
                                if (strlen(header)>=markerLen) {
                                        fwrite(&currentByte,1,1,fp_out);
                                        fseek(fp_in,-3,SEEK_CUR);
                                        continue;
                                }
                                unsigned char* headerFromFile = (unsigned char*)malloc(strlen(header)+1);
                                fread(headerFromFile,1,strlen(header),fp_in);
                                headerFromFile[strlen(header)] = 0x00;
                                if (strcmp(headerFromFile,header)!=0) {
                                        printf("Хедеры не равны\n");
                                        free(headerFromFile);
                                        fwrite(&currentByte,1,1,fp_out);
                                        fseek(fp_in,-3-strlen(header),SEEK_CUR);
                                        continue;
                                }
                                free(headerFromFile);
                                unsigned char mustBeNull = 0x00;
                                fread(&mustBeNull,1,1,fp_in);
                                if (mustBeNull!=0x00) {
                                        fwrite(&currentByte,1,1,fp_out);
                                        fseek(fp_in,-3-strlen(header)-1,SEEK_CUR);
                                        continue;   
                                }
                                fseek(fp_in,markerLen-strlen(header)-1,SEEK_CUR);
                                continue;
                        }
                        if (nextByte!=0xe1 || (nextByte == 0xe1 && alreadyDeleted!=-1) || (nextByte == 0xfe && (alreadyDeletedComment!=-1||header==NULL))) {
                                fwrite(&currentByte,1,1,fp_out);
                                fseek(fp_in,-1,SEEK_CUR);
                                continue;
                        }
                        unsigned char exifLenBytes[2];
                        result = fread(exifLenBytes,1,2,fp_in);
                        if (result!=2 || !fp_in) {
                                fseek(fp_in,-3,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        u_int16_t exifLen = (exifLenBytes[0]<<8)|exifLenBytes[1];
                        unsigned char exifBytes[6];
                        result = fread(exifBytes,1,6,fp_in);
                        if (result!=6 || !fp_in) {
                                fseek(fp_in,-3,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        if (exifBytes[0]!=0x45 || exifBytes[1]!=0x78 || exifBytes[2]!=0x69 || exifBytes[3]!=0x66 || exifBytes[4]!=0x00 || exifBytes[5]!=0x00) {
                                fseek(fp_in,-6,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        char* newFilenameTemp = (char*)malloc(strlen(filename)+strlen("_exif_copy")+1);
                        strcpy(newFilenameTemp,filename);
                        strcat(newFilenameTemp,"_exif_copy");
                        FILE* tempFile = fopen(newFilenameTemp,"wb");
                        unsigned char endianBytes[2];
                        result = fread(endianBytes,1,2,fp_in);
                        int isLittleEndian = 0;
                        if (endianBytes[0]==0x49 && endianBytes[1]==0x49) {
                                isLittleEndian = 1;
                        }
                        fwrite(endianBytes,1,2,tempFile);
                        for (int i = 2; i < exifLen; i++) {
                                fread(&currentByte,1,1,fp_in);
                                fwrite(&currentByte,1,1,tempFile);
                        }
                        if (tempFile) fclose(tempFile);
                        tempFile = fopen(newFilenameTemp,"rb");
                        char* filenameForRemove = (char*)malloc(strlen(newFilenameTemp)+strlen("delete_copy")+1);
                        strcpy(filenameForRemove,newFilenameTemp);
                        strcat(filenameForRemove,"delete_copy");
                        deleteMetadataTIFF(tempFile, NULL, newFilenameTemp, argc, argv, isLittleEndian);
                        if (!tempFile) {
                                tempFile = fopen(newFilenameTemp,"rb");
                        }
                        fseek(tempFile,0,SEEK_SET);
                        u_int16_t tagLen = 2+6;
                        long currentPos = ftell(fp_out);
                        while (tempFile && fread(&currentByte,1,1,tempFile) == 1) {
                                fwrite(&currentByte,1,1,fp_out);
                                tagLen++;
                        }
                        if (tempFile) fclose(tempFile);
                        fseek(fp_out,currentPos,SEEK_SET);
                        unsigned char markerBytes[2] = {0xff,0xe1};
                        fwrite(markerBytes,1,2,fp_out);
                        fwrite(&tagLen,1,2,fp_out);
                        unsigned char tagBytes[6] = {0x45,0x78,0x69,0x66,0x00,0x00};
                        fwrite(tagBytes,1,6,fp_out);
                        fseek(fp_out,-8+tagLen,SEEK_CUR);
                        if (tempFile) fclose(tempFile);
                        remove(newFilenameTemp);
                        remove(filenameForRemove);
                        free(filenameForRemove);
                        free(newFilenameTemp);
                        continue;
                }
                fwrite(&currentByte,1,1,fp_out);
        }
        if (fp_in) {
                fclose(fp_in);
        }
        if (fp_out) {
                fclose(fp_out);
        }
        free(new_filename);
        return 0;
}
int deleteMetadataPNG(FILE* fp_in, char* header, char* filename){
        if (header == NULL) {
                printf("Ошибка: Не указан заголовок\n");
                if (fp_in) fclose(fp_in);
                return 1;
        }
        int foundMetadata = -1;
        unsigned char currentByte = 0x00;
        fseek(fp_in, 0, SEEK_SET);
        printf("Копирование исходного файла в %s_delete_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_delete_copy") + 1);
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
        if (fp_out) fclose(fp_out);
        if (fp_in) fclose(fp_in);
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
        if (fp_copy) fclose(fp_copy);
        if (fp_delete) fclose(fp_delete);
        if (foundMetadata == -1) {
                printf("Метаданные с таки заголовком не найдены!");
        } else {
                printf("Успешно!");
        }
        free(new_filename);
        return 1;
}
int deleteMetadataGIF(FILE* fp_in, char* header,char* filename,int argc,char** argv) {
        fclose(fp_in);
        printf("Копирование исходного файла в %s_delete_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_delete_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_delete_copy");
        fp_in = fopen(filename, "rb");
        FILE* fp_out = fopen(new_filename, "wb");
        if (!fp_out) {
                printf("Ошибка открытия файла для резервного копирования\n");
                free(new_filename);
                return 1;
        }
        unsigned char currentByte = 0x00;
        while(fread(&currentByte, 1, 1, fp_in) == 1){
                fwrite(&currentByte, 1, 1, fp_out);
        }
        if (fp_out) fclose(fp_out);
        if (fp_in) fclose(fp_in);
        printf("Удаление метаданных\n");
        fp_in = fopen(new_filename, "rb");
        fp_out = fopen(filename, "wb");
        unsigned char* oldData = NULL;
        for (int i = 0; i < argc; i++) {
                if (strcmp(argv[i], "--oldData") == 0) {
                        if (argc <= i+1 || strcmp(argv[i+1],"") == 0) {
                                continue;
                        }
                        oldData = argv[i+1];
                }
        }
        int found = -1;
        while(fread(&currentByte, 1, 1, fp_in) == 1){
                if (oldData == NULL || found == 1) {
                        fwrite(&currentByte, 1, 1, fp_out);
                        continue;   
                }
                if (currentByte != 0x21) {
                        fwrite(&currentByte, 1, 1, fp_out);
                        continue;
                }
                unsigned char nextByte = 0x00;
                fread(&nextByte, 1, 1, fp_in);
                if (nextByte != 0xfe) {
                        fwrite(&currentByte, 1, 1, fp_out);
                        fwrite(&nextByte, 1, 1, fp_out);
                        continue;
                }
                u_int8_t length = 0x00;
                fread(&length, 1, 1, fp_in);
                if (length != strlen(oldData)) {
                        fwrite(&currentByte, 1, 1, fp_out);
                        fwrite(&nextByte, 1, 1, fp_out);
                        fwrite(&length, 1, 1, fp_out);
                        continue;
                }
                char* currentData = (char*)malloc(length + 1);
                fread(currentData, length+1, 1, fp_in);
                if (strcmp(currentData, oldData) == 0) {
                        found = 1;
                        continue;
                }
                fwrite(&currentByte, 1, 1, fp_out);
                fwrite(&nextByte, 1, 1, fp_out);
                fwrite(&length, 1, 1, fp_out);
                fwrite(currentData, length+1, 1, fp_out);
                free(currentData);
        }
        if (fp_out) fclose(fp_out);
        if (fp_in) fclose(fp_in);
        free(new_filename);
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
        } else if (headerBytes[0] == 0xff && headerBytes[1] == 0xd8 && headerBytes[2] == 0xff && headerBytes[3] == 0xe0) {
                int result = deleteMetadataJPEG(fp_in, header, filename,argc,argv);
        } else if (headerBytes[0] == 0x49 && headerBytes[1] == 0x49 && headerBytes[2] == 0x2a && headerBytes[3] == 0x00) {
                deleteMetadataTIFF(fp_in, header, filename, argc,argv, 1);
        } else if (headerBytes[0] == 0x4d && headerBytes[1] == 0x4d && headerBytes[2] == 0x00 && headerBytes[3] == 0x2a) {
                deleteMetadataTIFF(fp_in, header, filename, argc,argv, 0);
        } else if (headerBytes[0] == 0x47 && headerBytes[1] == 0x49 && headerBytes[2] == 0x46)  {
                deleteMetadataGIF(fp_in, header, filename, argc,argv);
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
        new_times[0].tv_nsec = 0;
        new_times[1].tv_sec = mktime(&tm_mtime);
        new_times[1].tv_nsec = 0;
        
        if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
            printf("\nОшибка при изменении временных меток файла\n");
            
        } else {
            printf("\nВременные метки файла успешно изменены.\n");
        }
        if (clock_settime(CLOCK_REALTIME, &current_time)) {
            perror("Ошибка при восстановлении системного времени");
            return 1;
        }

        return 0;
}


/*

	МОДУЛЬ ДОБАВЛЕНИЯ

*/
int addMetadataTIFF(FILE* fp_in, char* header, char* data, char* filename, int argc, char** argv, int isLittleEndian) {
        if (!fp_in){
                return -1;
        }
        fseek(fp_in,0,SEEK_SET);
        CLIExifArgument make = constructorCLIExifArgument("--make");
        CLIExifArgument model = constructorCLIExifArgument("--model");
        CLIExifArgument exposure = constructorCLIExifArgument("--exposure");
        CLIExifArgument FNumber = constructorCLIExifArgument("--fnumber");
        CLIExifArgument ISR = constructorCLIExifArgument("--isr");
        CLIExifArgument userComment = constructorCLIExifArgument("--usercomment");
        CLIExifArgument latitude = constructorCLIExifArgument("--lat");
        CLIExifArgument longitude = constructorCLIExifArgument("--lon");
        CLIExifArgument latitudeRef = constructorCLIExifArgument("--latRef");
        CLIExifArgument longitudeRef = constructorCLIExifArgument("--lonRef");
        CLIExifArgument datetime = constructorCLIExifArgument("--dt");
        CLIExifArgument imageDescription = constructorCLIExifArgument("--imageDescription");
        getExifArgumentFromCLI(&make, argc, argv);
        getExifArgumentFromCLI(&model, argc, argv);
        getExifArgumentFromCLI(&exposure, argc, argv);
        getExifArgumentFromCLI(&FNumber, argc, argv);
        getExifArgumentFromCLI(&ISR, argc, argv);
        getExifArgumentFromCLI(&userComment, argc, argv);
        getExifArgumentFromCLI(&latitude, argc, argv);
        getExifArgumentFromCLI(&latitudeRef, argc, argv);
        getExifArgumentFromCLI(&longitude, argc, argv);
        getExifArgumentFromCLI(&longitudeRef, argc, argv);
        getExifArgumentFromCLI(&datetime, argc, argv);
        getExifArgumentFromCLI(&imageDescription, argc, argv);
        TIFFInfo* tiffInfo = initTiffInfo();
        fseek(fp_in,4,SEEK_SET);
        readMetadataTIFF(fp_in,isLittleEndian, 0,tiffInfo);
        if (make.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(make, newNode, TIFF_MAKE, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (model.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(model, newNode, TIFF_MODEL, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (exposure.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(exposure, newNode, TIFF_EXPOSURETIME, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (FNumber.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(FNumber, newNode, TIFF_FNUMBER, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (ISR.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(ISR, newNode, TIFF_ISOSPEEDRATING, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (userComment.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(userComment, newNode, TIFF_USERCOMENT, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (latitude.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                fillTIFFInfoFromCLI(latitude, newNode, TIFF_LATITUDE, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (latitudeRef.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(latitudeRef, newNode, TIFF_LATITUDEREF, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (longitude.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(longitude, newNode, TIFF_LONGITUDE, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (longitudeRef.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(longitudeRef, newNode, TIFF_LONGITUDEREF, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (datetime.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(datetime, newNode, TIFF_DATETIME, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (imageDescription.data != NULL) {
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(imageDescription, newNode, TIFF_IMAGEDESCRIPTION, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        rewriteTIFF(tiffInfo, filename, "add", fp_in);
        clearTIFFInfo(tiffInfo);

}

int addMetadataJPEG(FILE* fp_in, char* header, char* data, char* filename, int argc, char** argv) {
        if (!fp_in){
                return -1;
        }
        fseek(fp_in,0,SEEK_SET);
        CLIExifArgument make = constructorCLIExifArgument("--make");
        CLIExifArgument model = constructorCLIExifArgument("--model");
        CLIExifArgument exposure = constructorCLIExifArgument("--exposure");
        CLIExifArgument FNumber = constructorCLIExifArgument("--fnumber");
        CLIExifArgument ISR = constructorCLIExifArgument("--isr");
        CLIExifArgument userComment = constructorCLIExifArgument("--usercomment");
        CLIExifArgument latitude = constructorCLIExifArgument("--lat");
        CLIExifArgument longitude = constructorCLIExifArgument("--lon");
        CLIExifArgument latitudeRef = constructorCLIExifArgument("--latRef");
        CLIExifArgument longitudeRef = constructorCLIExifArgument("--lonRef");
        CLIExifArgument datetime = constructorCLIExifArgument("--dt");
        CLIExifArgument imageDescription = constructorCLIExifArgument("--imageDescription");
        getExifArgumentFromCLI(&make, argc, argv);
        getExifArgumentFromCLI(&model, argc, argv);
        getExifArgumentFromCLI(&exposure, argc, argv);
        getExifArgumentFromCLI(&FNumber, argc, argv);
        getExifArgumentFromCLI(&ISR, argc, argv);
        getExifArgumentFromCLI(&userComment, argc, argv);
        getExifArgumentFromCLI(&latitude, argc, argv);
        getExifArgumentFromCLI(&latitudeRef, argc, argv);
        getExifArgumentFromCLI(&longitude, argc, argv);
        getExifArgumentFromCLI(&longitudeRef, argc, argv);
        getExifArgumentFromCLI(&datetime, argc, argv);
        getExifArgumentFromCLI(&imageDescription, argc, argv);
        int alreadyAdded = -1;
        int initNewExif = -1;
        for (int i = 0; i < argc; i++) {
                if (strcmp(argv[i],"--initNewExif") == 0) {
                        initNewExif = 1;
                        break;
                }
        }
        int exifCounter = 0;
        unsigned char jfifBuffer[2] = {0x00,0x00};
        int hasJFIFCLI = getJFIFVersionArgument(argc, argv, jfifBuffer);
        unsigned char currentByte = 0x00;
        printf("Копирование исходного файла в %s_add_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_add_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_add_copy");

        FILE* fp_out = fopen(new_filename,"wb");
        while(fp_in && fread(&currentByte,1,1,fp_in) == 1) {
                fwrite(&currentByte,1,1,fp_out);
        }
        if (fp_in) {
                fclose(fp_in);
        }
        if (fp_out){
                fclose(fp_out);
        }
        printf("Изменение исходного файла: %s\n", filename);
        fp_in = fopen(new_filename,"rb");
        fp_out = fopen(filename,"wb");
        free(new_filename);
        while (fp_in && fread(&currentByte,1,1,fp_in) == 1) {
                if (currentByte == 0xff) {
                        unsigned char nextByte = 0x00;
                        int result = fread(&nextByte,1,1,fp_in);
                        if (result!=1 || !fp_in) {
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        if (nextByte!=0xe1 && nextByte!=0xe0) {
                                fwrite(&currentByte,1,1,fp_out);
                                fseek(fp_in,-1,SEEK_CUR);
                                continue;
                        }
                        if (nextByte == 0xe0) {
                                unsigned char jfifBytes[4] = {0x00,0x00,0x00,0x00};
                                unsigned char jfifLenBytes[2] = {0x00,0x00};
                                result = fread(jfifLenBytes,1,2,fp_in);
                                if (result!=2 || !fp_in) {
                                        fseek(fp_in,-3,SEEK_CUR);
                                        fwrite(&currentByte,1,1,fp_out);
                                        continue;
                                }
                                u_int16_t jfifLen = (jfifLenBytes[0]<<8) | jfifLenBytes[1];
                                fread(jfifBytes,1,4,fp_in);
                                
                                unsigned char versionBytes[2] = {0x00,0x00};
                                if (hasJFIFCLI == -1) {
                                        fread(versionBytes,1,2,fp_in);
                                } else {
                                        versionBytes[0] = jfifBuffer[0];
                                        versionBytes[1] = jfifBuffer[1];
                                }
                                fwrite(&currentByte,1,1,fp_out);
                                fwrite(&nextByte,1,1,fp_out);
                                fwrite(jfifLenBytes,1,2,fp_out);
                                fwrite(jfifBytes,1,4,fp_out);
                                fwrite(versionBytes,1,2,fp_out);
                                //-2 len, -2 vers, -4 JFIF
                                for (int i=0;i<jfifLen-8;i++) {
                                        fread(&currentByte,1,1,fp_in);
                                        fwrite(&currentByte,1,1,fp_out);
                                }
                                if (header!=NULL && data!=NULL) {
                                        u_int16_t dataLen = strlen(header)+ strlen(data)+3;
                                        unsigned char tag[2] = {0xff, 0xfe};
                                        fwrite(tag,1,2,fp_out);
                                        unsigned char lenBytes[2] = {dataLen>>8 & 0xff, dataLen & 0xff};
                                        unsigned char nullByte = 0x00;
                                        fwrite(lenBytes,1,2,fp_out);
                                        fwrite(header,1,strlen(header),fp_out);
                                        fwrite(&nullByte,1,1,fp_out);
                                        fwrite(data, 1, strlen(data),fp_out);
                                }
                                
                                if (initNewExif == 1) {
                                        char* newFilenameTemp = (char*)malloc(strlen(filename)+strlen("_exif_copy")+1);
                                        strcpy(newFilenameTemp,filename);
                                        strcat(newFilenameTemp,"_exif_copy");
                                        FILE* tempFile = fopen(newFilenameTemp,"wb");
                                        unsigned char usingBytes[8] = {0x4d,0x4d,0x00,0x2a,0x00,0x00,0x00,0x08};
                                        fwrite(usingBytes,1,8,tempFile);
                                        if (tempFile) fclose(tempFile);
                                        tempFile = fopen(newFilenameTemp,"rb");
                                        char* filenameForRemove = (char*)malloc(strlen(newFilenameTemp)+strlen("add_copy")+1);
                                        strcpy(filenameForRemove,newFilenameTemp);
                                        strcat(filenameForRemove,"add_copy");
                                        addMetadataTIFF(tempFile, NULL, NULL, newFilenameTemp, argc, argv, 0);
                                        if (tempFile) {
                                                fclose(tempFile);
                                        }
                                        tempFile = fopen(newFilenameTemp,"rb");
                                        u_int16_t tagLen = 2+6;
                                        while (tempFile && fread(&currentByte,1,1,tempFile) == 1) {
                                                tagLen++;
                                        }
                                        if (tempFile) tempFile = fopen(newFilenameTemp,"rb");
                                        unsigned char markerBytes[2] = {0xff,0xe1};
                                        fwrite(markerBytes,1,2,fp_out);
                                        unsigned char tagLenBytes[2];
                                        tagLenBytes[0] = tagLen>>8&0xff;
                                        tagLenBytes[1] = tagLen&0xff;
                                        fwrite(tagLenBytes,1,2,fp_out);
                                        unsigned char tagBytes[6] = {0x45,0x78,0x69,0x66,0x00,0x00};
                                        fwrite(tagBytes,1,6,fp_out);
                                        while (tempFile && fread(&currentByte,1,1,tempFile) == 1) {
                                                fwrite(&currentByte,1,1,fp_out);
                                        }
                                        if (tempFile) fclose(tempFile);
                                        //remove(newFilenameTemp);
                                        remove(filenameForRemove);
                                        free(filenameForRemove);
                                        free(newFilenameTemp);
                                        alreadyAdded = 1;
                        
                                }
                                continue;
                        }
                        if (alreadyAdded == 1) {
                                fwrite(&currentByte,1,1,fp_out);
                                fwrite(&nextByte,1,1,fp_out);
                                continue;
                        }
                        unsigned char exifLenBytes[2];
                        result = fread(exifLenBytes,1,2,fp_in);
                        if (result!=2 || !fp_in) {
                                fseek(fp_in,-3,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        u_int16_t exifLen = (exifLenBytes[0]<<8)|exifLenBytes[1];
                        unsigned char exifLetters[6];
                        result = fread(exifLetters,1,6,fp_in);
                        if (result!=6 || !fp_in) {
                                fseek(fp_in,-8,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                fwrite(&nextByte,1,1,fp_out);
                                continue;
                        }
                        if (strncmp(exifLetters,"Exif",4)!=0) {
                                fseek(fp_in,-8,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                fwrite(&nextByte,1,1,fp_out);
                                continue;
                        }
                        char* newFilenameTemp = (char*)malloc(strlen(filename)+strlen("_exif_copy")+1);
                        strcpy(newFilenameTemp,filename);
                        strcat(newFilenameTemp,"_exif_copy");
                        FILE* tempFile = fopen(newFilenameTemp,"wb");
                        unsigned char endianBytes[2];
                        result = fread(endianBytes,1,2,fp_in);
                        int isLittleEndian = 0;
                        if (endianBytes[0]==0x49 && endianBytes[1]==0x49) {
                                isLittleEndian = 1;
                        }
                        fwrite(endianBytes,1,2,tempFile);
                        for (int i = 2; i < exifLen; i++) {
                                fread(&currentByte,1,1,fp_in);
                                fwrite(&currentByte,1,1,tempFile);
                        }
                        if (tempFile) fclose(tempFile);
                        tempFile = fopen(newFilenameTemp,"rb");
                        char* filenameForRemove = (char*)malloc(strlen(newFilenameTemp)+strlen("add_copy")+1);
                        strcpy(filenameForRemove,newFilenameTemp);
                        strcat(filenameForRemove,"add_copy");
                        addMetadataTIFF(tempFile, NULL, NULL, newFilenameTemp, argc, argv, isLittleEndian);
                        if (tempFile) {
                                fclose(tempFile);
                        }
                        tempFile = fopen(newFilenameTemp,"rb");
                        u_int16_t tagLen = 2+6;
                        while (tempFile && fread(&currentByte,1,1,tempFile) == 1) {
                                tagLen++;
                        }
                        if (tempFile) tempFile = fopen(newFilenameTemp,"rb");
                        unsigned char markerBytes[2] = {0xff,0xe1};
                        fwrite(markerBytes,1,2,fp_out);
                        unsigned char tagLenBytes[2];
                        tagLenBytes[0] = tagLen>>8&0xff;
                        tagLenBytes[1] = tagLen&0xff;
                        fwrite(tagLenBytes,1,2,fp_out);
                        unsigned char tagBytes[6] = {0x45,0x78,0x69,0x66,0x00,0x00};
                        fwrite(tagBytes,1,6,fp_out);
                        while (tempFile && fread(&currentByte,1,1,tempFile) == 1) {
                                fwrite(&currentByte,1,1,fp_out);
                        }
                        if (tempFile) fclose(tempFile);
                        remove(newFilenameTemp);
                        remove(filenameForRemove);
                        free(filenameForRemove);
                        free(newFilenameTemp);
                        alreadyAdded = 1;
                        continue;
                }
                fwrite(&currentByte,1,1,fp_out);
        }
        return 0;
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
        if (width>=0 || height>=0) {
                char response;
                while (response!='Y' && response!='y' && response!='N' && response!='n') {
                        printf("Изменение размеров изображения небезопасно! Продолжить?(Y/n): ");
                        scanf(" %c", &response);
                }
                if (response=='N' || response=='n') {
                        widthChanged = -1;
                        heightChanged = -1;
                        width = -1;
                        height = -1;
                } 
        }
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
                                
                                newCRC32 = crc32b(newMetadata, length+4);
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
        if (new_filename!=NULL){
                free(new_filename);
                new_filename = NULL;
        }
}

int addMetadataGIF(FILE* fp_in, char* filename, char* header, char* data, int argc, char** argv) {
        fclose(fp_in);

        printf("Копирование исходного файла в %s_add_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_add_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_add_copy");
        FILE* fp_out = fopen(new_filename, "wb");
        fp_in = fopen(filename, "rb");
        unsigned char currentByte = 0x00;
        while (fp_in && fread(&currentByte,1,1,fp_in)) {
                fwrite(&currentByte,1,1,fp_out);
        }
        if (fp_in) {
                fclose(fp_in);
        }
        if (fp_out) fclose(fp_out);
        printf("Копирование исходного файла: Успешно\n\n");
        fp_in = fopen(new_filename, "rb");
        fp_out = fopen(filename, "wb");
        unsigned char* newData = NULL;
        for (int i = 0; i< argc; i++) {
                if (strcmp(argv[i],"--newData") == 0) {
                        if (argc <= i+1 || strcmp(argv[i+1],"") == 0) {
                                continue;
                        }
                        newData = argv[i+1];
                        break;
                }
        }
        unsigned char startBytes[13] = {0};
        fread(startBytes,1,13,fp_in);
        fwrite(startBytes,13,1,fp_out);
        if (newData != NULL) {
                unsigned char extensionId[2] = {0x21, 0xfe};
                fwrite(extensionId, 2, 1, fp_out);
                u_int8_t blockLength = strlen(newData);
                fwrite(&blockLength, 1, 1, fp_out);
                unsigned char nullByte = 0x00;
                fwrite(newData, strlen(newData), 1, fp_out);
                fwrite(&nullByte, 1, 1, fp_out);
                printf("Добавление новых метаданных: Успешно\n\n");
        }
        while (fp_in && fread(&currentByte,1,1,fp_in)) {
                fwrite(&currentByte,1,1,fp_out);
        }
        if (fp_in) {
                fclose(fp_in);
        }
        if (fp_out) fclose(fp_out);
        free(new_filename);
        return 1;
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
        } else if (headerBytes[0] == 0xff && headerBytes[1] == 0xd8 && headerBytes[2] == 0xff && headerBytes[3] == 0xe0) {
                int result = addMetadataJPEG(fp_in,header,data, filename,argc,argv);
        } else if (headerBytes[0] == 0x49 && headerBytes[1] == 0x49 && headerBytes[2] == 0x2a && headerBytes[3] == 0x00) {
                addMetadataTIFF(fp_in, header, data, filename, argc,argv, 1);
        } else if (headerBytes[0] == 0x4d && headerBytes[1] == 0x4d && headerBytes[2] == 0x00 && headerBytes[3] == 0x2a) {
                addMetadataTIFF(fp_in, header, data, filename, argc,argv, 0);
        } else if (headerBytes[0] == 0x47 && headerBytes[1] == 0x49 && headerBytes[2] == 0x46) {
                addMetadataGIF(fp_in, filename, header, data, argc, argv);
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
        new_times[0].tv_nsec = 0;
        new_times[1].tv_sec = mktime(&tm_mtime);
        new_times[1].tv_nsec = 0; 
        
        if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
            printf("\nОшибка при изменении временных меток файла\n");
        } else {
            printf("\nВременные метки файла успешно изменены.\n");
        }
        if (clock_settime(CLOCK_REALTIME, &current_time)) {
            perror("Ошибка при восстановлении системного времени");
            return 1;
        }

        return 0;
}


/*

	МОДУЛЬ ОБНОВЛЕНИЯ

*/
int updateMetadataTIFF(FILE* fp_in, char* header, char* data, char* filename, int argc, char** argv, int isLittleEndian) {
        if (!fp_in){
                return -1;
        }
        fseek(fp_in,0,SEEK_SET);
        CLIExifArgument make = constructorCLIExifArgument("--make");
        CLIExifArgument model = constructorCLIExifArgument("--model");
        CLIExifArgument exposure = constructorCLIExifArgument("--exposure");
        CLIExifArgument FNumber = constructorCLIExifArgument("--fnumber");
        CLIExifArgument ISR = constructorCLIExifArgument("--isr");
        CLIExifArgument userComment = constructorCLIExifArgument("--usercomment");
        CLIExifArgument latitude = constructorCLIExifArgument("--lat");
        CLIExifArgument longitude = constructorCLIExifArgument("--lon");
        CLIExifArgument latitudeRef = constructorCLIExifArgument("--latRef");
        CLIExifArgument longitudeRef = constructorCLIExifArgument("--lonRef");
        CLIExifArgument datetime = constructorCLIExifArgument("--dt");
        CLIExifArgument imageDescription = constructorCLIExifArgument("--imageDescription");
        getExifArgumentFromCLI(&make, argc, argv);
        getExifArgumentFromCLI(&model, argc, argv);
        getExifArgumentFromCLI(&exposure, argc, argv);
        getExifArgumentFromCLI(&FNumber, argc, argv);
        getExifArgumentFromCLI(&ISR, argc, argv);
        getExifArgumentFromCLI(&userComment, argc, argv);
        getExifArgumentFromCLI(&latitude, argc, argv);
        getExifArgumentFromCLI(&latitudeRef, argc, argv);
        getExifArgumentFromCLI(&longitude, argc, argv);
        getExifArgumentFromCLI(&longitudeRef, argc, argv);
        getExifArgumentFromCLI(&datetime, argc, argv);
        getExifArgumentFromCLI(&imageDescription, argc, argv);
        TIFFInfo* tiffInfo = initTiffInfo();
        fseek(fp_in,4,SEEK_SET);
        readMetadataTIFF(fp_in,isLittleEndian, 0,tiffInfo);
        if (make.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_MAKE);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(make, newNode, TIFF_MAKE, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (model.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_MODEL);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(model, newNode, TIFF_MODEL, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (exposure.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_EXPOSURETIME);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(exposure, newNode, TIFF_EXPOSURETIME, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (FNumber.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_FNUMBER);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(FNumber, newNode, TIFF_FNUMBER, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (ISR.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_ISOSPEEDRATING);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(ISR, newNode, TIFF_ISOSPEEDRATING, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (userComment.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_USERCOMENT);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(userComment, newNode, TIFF_USERCOMENT, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (latitude.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_LATITUDE);
                TIFFInfo* newNode = initTiffInfo();
                fillTIFFInfoFromCLI(latitude, newNode, TIFF_LATITUDE, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (latitudeRef.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_LATITUDEREF);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(latitudeRef, newNode, TIFF_LATITUDEREF, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (longitude.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_LONGITUDE);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(longitude, newNode, TIFF_LONGITUDE, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (longitudeRef.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_LONGITUDEREF);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(longitudeRef, newNode, TIFF_LONGITUDEREF, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (datetime.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_DATETIME);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(datetime, newNode, TIFF_DATETIME, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        if (imageDescription.data != NULL) {
                deleteTiffTag(tiffInfo, TIFF_IMAGEDESCRIPTION);
                TIFFInfo* newNode = initTiffInfo();
                newNode->ifdNumber = getLastIFD(tiffInfo);
                fillTIFFInfoFromCLI(imageDescription, newNode, TIFF_IMAGEDESCRIPTION, isLittleEndian);
                appendTiff(tiffInfo, newNode);
        }
        rewriteTIFF(tiffInfo, filename, "update", fp_in);
        clearTIFFInfo(tiffInfo);
}

int updateMetadataJPEG(FILE* fp_in, char* header, char* data, char* filename, int argc, char** argv) {
        if (!fp_in){
                return -1;
        }
        fseek(fp_in,0,SEEK_SET);
        CLIExifArgument make = constructorCLIExifArgument("--make");
        CLIExifArgument model = constructorCLIExifArgument("--model");
        CLIExifArgument exposure = constructorCLIExifArgument("--exposure");
        CLIExifArgument FNumber = constructorCLIExifArgument("--fnumber");
        CLIExifArgument ISR = constructorCLIExifArgument("--isr");
        CLIExifArgument userComment = constructorCLIExifArgument("--usercomment");
        CLIExifArgument latitude = constructorCLIExifArgument("--lat");
        CLIExifArgument longitude = constructorCLIExifArgument("--lon");
        CLIExifArgument latitudeRef = constructorCLIExifArgument("--latRef");
        CLIExifArgument longitudeRef = constructorCLIExifArgument("--lonRef");
        CLIExifArgument datetime = constructorCLIExifArgument("--dt");
        CLIExifArgument imageDescription = constructorCLIExifArgument("--imageDescription");
        getExifArgumentFromCLI(&make, argc, argv);
        getExifArgumentFromCLI(&model, argc, argv);
        getExifArgumentFromCLI(&exposure, argc, argv);
        getExifArgumentFromCLI(&FNumber, argc, argv);
        getExifArgumentFromCLI(&ISR, argc, argv);
        getExifArgumentFromCLI(&userComment, argc, argv);
        getExifArgumentFromCLI(&latitude, argc, argv);
        getExifArgumentFromCLI(&latitudeRef, argc, argv);
        getExifArgumentFromCLI(&longitude, argc, argv);
        getExifArgumentFromCLI(&longitudeRef, argc, argv);
        getExifArgumentFromCLI(&datetime, argc, argv);
        getExifArgumentFromCLI(&imageDescription, argc, argv);
        int exifCounter = 0;
        unsigned char jfifBuffer[2] = {0x00,0x00};
        int hasJFIFCLI = getJFIFVersionArgument(argc, argv, jfifBuffer);
        unsigned char currentByte = 0x00;
        printf("Копирование исходного файла в %s_update_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_update_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_update_copy");

        FILE* fp_out = fopen(new_filename,"wb");
        while(fp_in && fread(&currentByte,1,1,fp_in) == 1) {
                fwrite(&currentByte,1,1,fp_out);
        }
        if (fp_in) {
                fclose(fp_in);
        }
        if (fp_out){
                fclose(fp_out);
        }
        printf("Изменение исходного файла: %s\n", filename);
        fp_in = fopen(new_filename,"rb");
        fp_out = fopen(filename,"wb");
        free(new_filename);
        while (fp_in && fread(&currentByte,1,1,fp_in) == 1) {
                if (currentByte == 0xff) {
                        unsigned char nextByte = 0x00;
                        int result = fread(&nextByte,1,1,fp_in);
                        if (result!=1 || !fp_in) {
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }

                        if (nextByte == 0xfe && header!=NULL && data!=NULL) {
                                unsigned char markerLenBytes[2] = {0x00,0x00};
                                fread(markerLenBytes,1,2,fp_in);
                                u_int16_t markerLen = (markerLenBytes[0]<<8) | (markerLenBytes[1]);
                                markerLen-=2;
                                if (strlen(header)>=markerLen) {
                                        fwrite(&currentByte,1,1,fp_out);
                                        fseek(fp_in,-3,SEEK_CUR);
                                        continue;
                                }
                                unsigned char* headerFromFile = (unsigned char*)malloc(strlen(header));
                                fread(headerFromFile,1,strlen(header),fp_in);
                                if (strcmp(headerFromFile,header)!=0) {
                                        free(headerFromFile);
                                        fwrite(&currentByte,1,1,fp_out);
                                        fseek(fp_in,-3-strlen(header),SEEK_CUR);
                                        continue;
                                }
                                free(headerFromFile);
                                unsigned char mustBeNull = 0x00;
                                fread(&mustBeNull,1,1,fp_in);
                                if (mustBeNull!=0x00) {
                                        fwrite(&currentByte,1,1,fp_out);
                                        fseek(fp_in,-3-strlen(header)-1,SEEK_CUR);
                                        continue;   
                                }
                                fseek(fp_in,markerLen-strlen(header)-1,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                fwrite(&nextByte,1,1,fp_out);
                                u_int16_t newLen = strlen(header)+2+strlen(data)+1;
                                unsigned char newLenBytes[2] = {newLen>>8 & 0xff, newLen & 0xff};
                                fwrite(newLenBytes,1,2,fp_out);
                                fwrite(header,1,strlen(header),fp_out);
                                unsigned char nullByte = 0x00;
                                fwrite(&nullByte,1,1,fp_out);
                                fwrite(data,1,strlen(data),fp_out);
                                continue;
                        }

                        if (nextByte!=0xe1) {
                                fwrite(&currentByte,1,1,fp_out);
                                fseek(fp_in,-1,SEEK_CUR);
                                continue;
                        }
                        unsigned char exifLenBytes[2];
                        result = fread(exifLenBytes,1,2,fp_in);
                        if (result!=2 || !fp_in) {
                                fseek(fp_in,-3,SEEK_CUR);
                                fwrite(&currentByte,1,1,fp_out);
                                continue;
                        }
                        u_int16_t exifLen = (exifLenBytes[0]<<8)|exifLenBytes[1];
                        unsigned char exifBytes[6] = {0x00};
                        fread(exifBytes,1,6,fp_in);
                        if (exifBytes[0]!=0x45 || exifBytes[1]!=0x78 || exifBytes[2]!=0x69 || exifBytes[3]!=0x66 || exifBytes[4]!=0x00 || exifBytes[5]!=0x00) {
                                fwrite(&currentByte,1,1,fp_out);
                                fseek(fp_in,-9,SEEK_CUR);
                                continue;
                        }
                        char* newFilenameTemp = (char*)malloc(strlen(filename)+strlen("_exif_copy")+1);
                        strcpy(newFilenameTemp,filename);
                        strcat(newFilenameTemp,"_exif_copy");
                        FILE* tempFile = fopen(newFilenameTemp,"wb");
                        unsigned char endianBytes[2];
                        result = fread(endianBytes,1,2,fp_in);
                        int isLittleEndian = 0;
                        if (endianBytes[0]==0x49 && endianBytes[1]==0x49) {
                                isLittleEndian = 1;
                        }
                        fwrite(endianBytes,1,2,tempFile);
                        for (int i = 2; i < exifLen; i++) {
                                fread(&currentByte,1,1,fp_in);
                                fwrite(&currentByte,1,1,tempFile);
                        }
                        if (tempFile) fclose(tempFile);
                        tempFile = fopen(newFilenameTemp,"rb");
                        char* filenameForRemove = (char*)malloc(strlen(newFilenameTemp)+strlen("update_copy")+1);
                        strcpy(filenameForRemove,newFilenameTemp);
                        strcat(filenameForRemove,"update_copy");
                        updateMetadataTIFF(tempFile, NULL, NULL, newFilenameTemp, argc, argv, isLittleEndian);
                        if (!tempFile) {
                                tempFile = fopen(newFilenameTemp,"rb");
                        }
                        fseek(tempFile,0,SEEK_SET);
                        u_int16_t tagLen = 2+6;
                        long currentPos = ftell(fp_out);
                        while (tempFile && fread(&currentByte,1,1,tempFile) == 1) {
                                fwrite(&currentByte,1,1,fp_out);
                                tagLen++;
                        }
                        if (tempFile) fclose(tempFile);
                        fseek(fp_out,currentPos,SEEK_SET);
                        unsigned char markerBytes[2] = {0xff,0xe1};
                        fwrite(markerBytes,1,2,fp_out);
                        fwrite(&tagLen,1,2,fp_out);
                        unsigned char tagBytes[6] = {0x45,0x78,0x69,0x66,0x00,0x00};
                        fwrite(tagBytes,1,6,fp_out);
                        fseek(fp_out,-8+tagLen,SEEK_CUR);
                        if (tempFile) fclose(tempFile);
                        remove(newFilenameTemp);
                        remove(filenameForRemove);
                        free(filenameForRemove);
                        free(newFilenameTemp);
                        continue;
                }
                fwrite(&currentByte,1,1,fp_out);
        }
        return 0;
}

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
        if (width>=0 || height>=0) {
                char response;
                while (response!='Y' && response!='y' && response!='N' && response!='n') {
                        printf("Изменение размеров изображения небезопасно! Продолжить?(Y/n): ");
                        scanf(" %c", &response);
                }
                if (response=='N' || response=='n') {
                        widthChanged = -1;
                        heightChanged = -1;
                        width = -1;
                        height = -1;
                } 
        }
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
                newCRC32 = crc32b(newMetadata, newLength+4);
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
                if (newMetadata!=NULL){
                        free(newMetadata);
                        newMetadata = NULL;
                }
        }
        if (fp_in_copied) fclose(fp_in_copied);
        if(fp_out) fclose(fp_out);
        if (header!=NULL && data!=NULL && headerFound == -1){
                printf("Изменение метаданных: Не найдены метаданные с заданным заголовком\n\n");
        }
        return 0;
}
int updateMetadataGIF(FILE* fp_in, char* header,char* filename,int argc,char** argv) {
        fclose(fp_in);

        printf("Копирование исходного файла в %s_update_copy\n", filename);
        char* new_filename = (char*)malloc(strlen(filename) + strlen("_update_copy") + 1);
        if (new_filename == NULL) {
                printf("Ошибка выделения памяти\n");
                return 1;
        }
        strcpy(new_filename, filename);
        strcat(new_filename, "_update_copy");
        fp_in = fopen(filename, "rb");
        if (fp_in == NULL) {
                printf("Ошибка открытия файла %s\n", filename);
                free(new_filename);
                return 1;
        }
        FILE* fp_out = fopen(new_filename, "wb");
        if (fp_out == NULL) {
                printf("Ошибка открытия файла %s_update_copy\n", filename);
                free(new_filename);
                return 1;
        }
        unsigned char currentByte = 0x00;
        while(fread(&currentByte, 1, 1, fp_in) == 1){
                fwrite(&currentByte, 1, 1, fp_out);
        }
        fclose(fp_in);
        fp_in = fopen(new_filename, "rb");
        if (fp_in == NULL) {
                printf("Ошибка открытия файлов для изменения метаданных\n");
                free(new_filename);
                return 1;
        }
        fp_out = fopen(filename, "wb");
        unsigned char* oldData = NULL;
        unsigned char* newData = NULL;
        int delay = 0;
        for (int i = 0; i < argc; i++){
                if (i+1 >= argc){
                        continue;
                }
                if (strcmp(argv[i],"--delay") == 0){
                        
                        delay = atoi(argv[i+1]);
                        continue;
                }
                if (strcmp(argv[i],"--oldData") == 0){
                        oldData = argv[i+1];
                        continue;
                }
                if (strcmp(argv[i],"--newData") == 0){
                        newData = argv[i+1];
                        continue;
                }
        }
        int headerFound = 0;
        int found = -1;
        while(fread(&currentByte, 1, 1, fp_in) == 1){
                if (currentByte != 0x21) {
                        fwrite(&currentByte, 1, 1, fp_out);
                        continue;
                }
                unsigned char nextByte = 0x00;
                fread(&nextByte, 1, 1, fp_in);
                if (nextByte != 0xf9 && nextByte != 0xfe) {
                        fwrite(&currentByte, 1, 1, fp_out);
                        fwrite(&nextByte, 1, 1, fp_out);
                        continue;
                }
                if (nextByte == 0xfe) {
                        if (found == 1) {
                                continue;
                        }
                        u_int8_t length = 0x00;
                        fread(&length, 1, 1, fp_in);
                        if (length != strlen(oldData)) {
                                fwrite(&currentByte, 1, 1, fp_out);
                                fwrite(&nextByte, 1, 1, fp_out);
                                fwrite(&length, 1, 1, fp_out);
                                continue;
                        }
                        char* currentData = (char*)malloc(length + 1);
                        fread(currentData, length+1, 1, fp_in);
                        if (strcmp(currentData, oldData) == 0) {
                                found = 1;
                                fwrite(&currentByte, 1, 1, fp_out);
                                fwrite(&nextByte, 1, 1, fp_out);
                                u_int8_t newLength = strlen(newData);
                                fwrite(&newLength, 1, 1, fp_out);
                                fwrite(newData, 1, newLength, fp_out);
                                unsigned char nullByte = 0x00;
                                fwrite(&nullByte, 1, 1, fp_out);
                                free(currentData);
                                continue;
                        }
                        fwrite(&currentByte, 1, 1, fp_out);
                        fwrite(&nextByte, 1, 1, fp_out);
                        fwrite(&length, 1, 1, fp_out);
                        fwrite(currentData, length+1, 1, fp_out);
                        free(currentData);
                        continue;
                }
                if (delay<=0) {
                        fwrite(&currentByte, 1, 1, fp_out);
                        fwrite(&nextByte, 1, 1, fp_out);
                        continue;
                }
                unsigned char notImportantBytes[2] = {0x00, 0x00};
                fread(notImportantBytes, 2, 1, fp_in);
                fwrite(&currentByte, 1, 1, fp_out);
                fwrite(&nextByte, 1, 1, fp_out);
                fwrite(notImportantBytes, 2, 1, fp_out);
                fseek(fp_in, 2, SEEK_CUR);
                unsigned char delayBytes[2] = {(u_int16_t)delay & 0xff, (u_int16_t)delay>>8 & 0xff};
                fwrite(&delayBytes, 2, 1, fp_out);
                fread(&currentByte,1,1,fp_in);
                fwrite(&currentByte, 1, 1, fp_out);
                delay = -1;
        }
        printf("Метаданные изменены\n");
        free(new_filename);
        fclose(fp_in);
        fclose(fp_out);
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
        } else if (headerBytes[0] == 0xff && headerBytes[1] == 0xd8 && headerBytes[2] == 0xff && headerBytes[3] == 0xe0) {
                updateMetadataJPEG(fp_in,header,data,filename,argc, argv);
        } else if (headerBytes[0] == 0x49 && headerBytes[1] == 0x49 && headerBytes[2] == 0x2a && headerBytes[3] == 0x00) {
                updateMetadataTIFF(fp_in, header, data, filename, argc,argv, 1);
        } else if (headerBytes[0] == 0x4d && headerBytes[1] == 0x4d && headerBytes[2] == 0x00 && headerBytes[3] == 0x2a) {
                updateMetadataTIFF(fp_in, header, data, filename, argc,argv, 0);
        } else if (headerBytes[0] == 0x47 && headerBytes[1] == 0x49 && headerBytes[2] == 0x46)  {
                updateMetadataGIF(fp_in, header, filename, argc,argv);
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
        new_times[0].tv_nsec = 0;
        new_times[1].tv_sec = mktime(&tm_mtime);
        new_times[1].tv_nsec = 0; 
        
        if (utimensat(AT_FDCWD, filename, new_times, 0) == -1) {
            printf("\nОшибка при изменении временных меток файла\n");
        } else {
            printf("\nВременные метки файла успешно изменены.\n");
        }
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
        printf("\n\n\n");
        printf("\t\t\t              __            \n");
        printf("\t\t\t             /\\ \\           \n");
        printf("\t\t\t  ___ ___    \\_\\ \\     __   \n");
        printf("\t\t\t/' __` __`\\  /'_` \\  /'__`\\ \n");
        printf("\t\t\t/\\ \\/\\ \\/\\ \\/\\ \\L\\ \\/\\  __/ \n");
        printf("\t\t\t\\ \\_\\ \\_\\ \\_\\ \\___,_\\ \\____\\\n");
        printf("\t\t\t \\/_/\\/_/\\/_/\\/__,_ /\\/____/\n\n");
        printf("\n\n\n");

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
		char* data = getData(argc,argv);
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
		
		deleteMetadata(filename, header, argc, argv);
		return 0;
        }
	writeHelpMessage(argv[0]);
	return 1;
}

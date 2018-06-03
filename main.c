#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<stdint.h>
#include<unistd.h>
#include<sys/time.h>
#include<math.h>
#define Swap(ll) (((ll)>>56)|\
                 (((ll)&(0x00ff000000000000))>>40)|\
                 (((ll)&(0x0000ff0000000000))>>24)|\
                 (((ll)&(0x000000ff00000000))>>8)|\
                 (((ll)&(0x00000000ff000000))<<8)|\
                 (((ll)&(0x0000000000ff0000))<<24)|\
                 (((ll)&(0x000000000000ff00))<<40)|\
                 (((ll)<<56)))

#define MAXLENGTHFILENAME (150)
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

int strongfilewrite(FILE*fp, char* buf, int length) {
    int re = 0;
    if (fp == NULL) {
        printf("Bad file description!!\n");
        return -1;
    }

    if (buf == NULL || length <= 0) {
        printf("No buf need to Write\n");
        return 0;
    }
    int local = 0;
    while (1) {
        local = fwrite(&buf[re], sizeof(char), length - re, fp);
        if(re > length -re ) {
            printf("ERROR write ERROR !!!\n");
            return -1;
        }
        else if(local < length - re) {
            if(!ferror(fp))
                re += local;
        } else {
            return 0;
        }
    }
}

int strongfileread(FILE*fp, char* buf, int length) {
    int re = 0;
    if (fp == NULL) {
        printf("Bad file description!!! \n");
    }
    if (buf == NULL || length <=0 ) {
        printf("no buf or no need to read!!!");
        return -1;
    }
    int local = 0;
    while (1) {
        local = fread(&buf[re], sizeof(char), length -re, fp);
        if (local > length - re) {
            printf("Read ERROR !!!");
            return -1;
        } else if (local < length-re) {
            if (feof(fp)) {
                printf("Reach the end of file\n");
                return -1;
            } if(!ferror(fp))
               re = re + local;
        } else {
            return 0;
        }
    }
}

int ParseArgv(int argc, char** argv, char* srchcifilepath, \
        char* generatefiledirector, int* maxlength) {
    int commandoptions = 0;
    if(argc < 2) {
        printf("too few parments. use -h to get help\n");
        return -1;
    }
    while ((commandoptions = getopt(argc, argv, "hd:s:l:")) != -1) {
        switch (commandoptions) {
            case 'h':
                printf("\
splite btsnoop_hci.log\n\
splithci -h  for help\n\
splithci -s[src file path] -d[des file dir] -l[MAX file length]\
\n");
                return -1;
            case 's':
                snprintf(srchcifilepath, MAXLENGTHFILENAME - 1, "%s", optarg);
                if (access(srchcifilepath, F_OK)) {
                    printf("no src file %s and errno is %d\n", srchcifilepath, errno);
                    return -1;
                }
                printf("src is %s\n", srchcifilepath);
                break;
            case 'd':
                snprintf(generatefiledirector, MAXLENGTHFILENAME - 1,"%s",optarg);
                if (access(generatefiledirector, 0)) {
                    printf("no dst file!!!%s \n", generatefiledirector);
                    return -1;
                }
                break;
            case 'l':
                *maxlength = atoi(optarg);
                if (*maxlength > 5 || *maxlength < 0) {
                    printf("maxlength is better set 0 - 5\n");
                    return -1;
                }
                break;
            default:
                printf("Knowed option:%c\n", commandoptions);
        }
    }
    if (!strlen(srchcifilepath) || !strlen(generatefiledirector)) {
        printf("worry parments please use -h for help\n");
        return -1 ;
    }
    printf("splite %s into dst %s, and the max length is %d MB",srchcifilepath, \
            generatefiledirector, *maxlength);
    return 0;
}

int main(int argc, char ** argv) {

    FILE *srchcifilepathfilepointer = NULL;
    FILE *generatefiledirectorfilepointer = NULL;
    char srchcifilepath[MAXLENGTHFILENAME] = {0};
    char generatefiledirector[MAXLENGTHFILENAME] = {0};
    char contenttransferbuf[102400] = {0};
    int maxlengthofhcifile = 50;
    char splitedfilename[100] = {0}; //文件名
    unsigned int generatehcifile = 0;        //组建文件名
    char filelengthraw[4] = {0};    //数据包中的文件长度字段
    unsigned long long filelengthnum = 0;

    if (ParseArgv(argc, argv, srchcifilepath, generatefiledirector, \
                &maxlengthofhcifile)) {
        printf("configure failed!!!\n");
        return -1;
    }

    srchcifilepathfilepointer = fopen(srchcifilepath, "rt+");
    fseek(srchcifilepathfilepointer, 16, SEEK_CUR);
    while (!feof(srchcifilepathfilepointer)) {
        int write_file_cur = 0;
        sprintf(splitedfilename, "%s/%d", generatefiledirector, generatehcifile++);
        generatefiledirectorfilepointer = fopen(splitedfilename, "wb");
        fwrite("btsnoop\0\0\0\0\1\0\0\x3\xea", sizeof(unsigned char), 16, \
                generatefiledirectorfilepointer);
        while (write_file_cur < (MAXLENGTHFILENAME * 1024) \
                && !feof(srchcifilepathfilepointer)) {
            int lll = 0;
            filelengthnum = 0;
            fread(filelengthraw, sizeof(unsigned char), 4, srchcifilepathfilepointer);
            while (lll < 4) {
                filelengthnum = filelengthnum * 16 * 16 + \
                                (unsigned char)filelengthraw[lll++];
            }
            if (filelengthnum < 0) {
                printf("ERROR!!! length is error\n");
                return -1;
            }
            printf("=======,length=%llu \n", filelengthnum);

            fseek(srchcifilepathfilepointer, -4, SEEK_CUR);
            memset(&contenttransferbuf, 0, sizeof(contenttransferbuf));
            int cur = 0;
            while (1) {
                int local = (filelengthnum - cur + 24 > 1024) ? 1024 : \
                            (filelengthnum + 24 - cur);
                memset(&contenttransferbuf, 0, sizeof(contenttransferbuf));
                if (strongfileread(srchcifilepathfilepointer, contenttransferbuf, local)) {
                    return -1;
                }
                cur += local;
                if (strongfilewrite(generatefiledirectorfilepointer, contenttransferbuf, \
                            local)) {
                    return -1;
                }
                memset(&contenttransferbuf, 0, sizeof(contenttransferbuf));
                if (cur >= filelengthnum + 24) {
                    break;
                }
                if (feof(generatefiledirectorfilepointer)) {
                    printf("finsh success!!!\n");
                    fclose(generatefiledirectorfilepointer);
                    return 0;
                }
            }
            write_file_cur = write_file_cur + filelengthnum;
            filelengthnum = 0;
            printf("finish one block!!!\n");
        }
        printf("finish write file %s\n", srchcifilepath);
        fclose(generatefiledirectorfilepointer);
        generatefiledirectorfilepointer = NULL;
        write_file_cur = 0;
    }
    if (generatefiledirectorfilepointer != NULL) {
        fclose(generatefiledirectorfilepointer);
        generatefiledirectorfilepointer = NULL;
    }
    if (srchcifilepathfilepointer != NULL) {
        fclose(srchcifilepathfilepointer);
        srchcifilepathfilepointer = NULL;
    }
    return 0;
}

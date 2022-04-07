#include <stdio.h>
#include <malloc.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define ALL_OK 0
#define DIR_NOT_EXIST 1
#define OTHER (14-12) 
#define INCORRECT_DIRECTORY 25

struct fileInfo {
    char* name;
    char* path;
    int size;
    int ind;
};

typedef struct fileInfoNode {
    struct fileInfo info;
    struct fileInfoNode* ptOnNext;
}TFILE_INFO_NODE;

int numLen(int ind){
    int length = 0;
    while(ind){
        length++;
        ind /= 10;
    }
    return length;
}

int compareSize(TFILE_INFO_NODE* p1, TFILE_INFO_NODE* p2)
{
    return p1->info.size > p2->info.size ? 1 : -1;
}


int compareName(TFILE_INFO_NODE* p1, TFILE_INFO_NODE* p2)
{
    return strcmp(p1->info.name, p2->info.name) > 0;
}

TFILE_INFO_NODE* mergeSortedNodes(TFILE_INFO_NODE* p1, TFILE_INFO_NODE* p2, int (*isP1GreaterP2) (TFILE_INFO_NODE*, TFILE_INFO_NODE*))
{
    TFILE_INFO_NODE* result;
    if (p1 == NULL)
        return p2;
    else if (p2 == NULL)
        return p1;

    if (isP1GreaterP2(p1, p2) > 0) {
        result = p2;
        p2->ptOnNext = mergeSortedNodes(p1, p2->ptOnNext, isP1GreaterP2);
    }
    else {
        result = p1;
        p1->ptOnNext = mergeSortedNodes(p1->ptOnNext, p2, isP1GreaterP2);
    }
    return result;
}

TFILE_INFO_NODE* splitToParts(TFILE_INFO_NODE* head, TFILE_INFO_NODE** start, TFILE_INFO_NODE** end)
{

    TFILE_INFO_NODE* p1 = head, * p2 = head->ptOnNext;
    while (p2 != NULL)
    {
        p2 = p2->ptOnNext;
        if (p2 != NULL) {
            p1 = p1->ptOnNext;
            p2 = p2->ptOnNext;
        }
    }

    *start = head;
    *end = p1->ptOnNext;
    p1->ptOnNext = NULL;
}


void sortList(TFILE_INFO_NODE** head, int (*isP1GreaterP2) (TFILE_INFO_NODE*, TFILE_INFO_NODE*))
{
    TFILE_INFO_NODE* p1, * p2;
    if ((*head == NULL) || ((*head)->ptOnNext == NULL)) return;

    splitToParts(*head, &p1, &p2);
    sortList(&p1, isP1GreaterP2);
    sortList(&p2, isP1GreaterP2);

    *head = mergeSortedNodes(p1, p2, isP1GreaterP2);
}

void freeNode(TFILE_INFO_NODE* node)
{
    free(node->info.name);
    free(node->info.path);
    free(node);
}

char* formFileName(char* fileName, int ind)
{
    
    if (ind != 0){

        int chunk = numLen(ind);
        int prevLen = strlen(fileName);
        fileName = realloc(fileName, prevLen + chunk + 2 + 2 + 1);
        if (fileName){
            fileName[prevLen] = '_';
            fileName[++prevLen] = '_';
            fileName[prevLen] = '(';
            prevLen += chunk;
            while (ind){
                fileName[prevLen--] = ind % 10 + '0';
                ind /= 10;
            }
            prevLen += chunk;
            fileName[++prevLen] = ')';
            fileName[++prevLen] = '\0';
        }

    }
    return fileName;
}

void checkFileNames(TFILE_INFO_NODE* head)
{
   
    while (head && head->ptOnNext)
    {
        TFILE_INFO_NODE* enumNode = head->ptOnNext;
        int ind = 0;
        while(!head->info.ind && enumNode){
            if (!strcmp(enumNode->info.name, head->info.name)) 
                enumNode->info.ind = ++ind;
            enumNode = enumNode->ptOnNext;
        }
        head = head->ptOnNext;
    }
    
}

char* formDirectoryName(char* path, char* file){
    int len = strlen(path) + strlen(file);
    char* res = (char*)malloc(len + 2);
    for (int i = 0; i <= strlen(path); i++)
        res[i] = path[i];
    len = strlen(path);
    res[len] = '/';
    for(int i = 0; i <= strlen(file); i++)
        res[i + len + 1] = file[i];

    return res;
}

TFILE_INFO_NODE* createNode(char* path, char* name, char* fullName, struct stat* statInfo)
{
    TFILE_INFO_NODE* res = (TFILE_INFO_NODE*)malloc(sizeof(TFILE_INFO_NODE));   
    if (res){
        res->info.name = (char*)malloc(strlen(name) + 1);
        res->info.name = strcpy(res->info.name, name);
        res->info.path = (char*)malloc(strlen(path) + 1);
        res->info.path = strcpy(res->info.path, path);
        res->info.size = statInfo->st_size;
        res->info.ind = 0;
    }
    return res;
}

int isItDirectory(char* path, struct stat* path_stat){
    stat(path, path_stat);
    return S_ISDIR(path_stat->st_mode);
}

int enumFiles(char* pathName, struct fileInfoNode** head, char* incorrPath)
{
    DIR* dir = opendir(pathName);
    int resCode = 0;
    if (!dir){ 
        int errorCode = errno;
        resCode = (errorCode == ENOTDIR || errorCode == ENOENT) ? DIR_NOT_EXIST : OTHER;
    }else{
        struct dirent* d;
        while (d = readdir(dir)){             
            if(strcmp(d->d_name, "..") && strcmp(d->d_name, ".")){
                char* c = formDirectoryName(pathName, d->d_name);
                struct stat fileStat;
                if (isItDirectory(c, &fileStat))
                    resCode = !strcmp(c, incorrPath) ? INCORRECT_DIRECTORY : enumFiles(c, head, incorrPath);
                else{
                    TFILE_INFO_NODE* node = createNode(pathName, d->d_name, c, &fileStat);
                    if (node){
                        node->ptOnNext = *head;
                        *head = node;
                    }
                }
                free(c);
            }
        }
    }

    closedir(dir); 
    return resCode;
}


int main(int argc, char** argv)
{
    if (argc != 4){
        fputs("Error! Incorrect count of arguments\n", stderr);
        return 1;
    }

    char* endptr;
    char* N = argv[2];

    int code = strtol(N, &endptr, 10);

    if ((errno == ERANGE && (code == LONG_MAX || code == LONG_MIN)) || (errno != 0 && code == 0)) {
        fprintf(stderr, "You entered wrong group size:\n");
        fprintf(stderr, "Out of range...\n");
        return -2;
    }

    if (endptr == N) {
        fprintf(stderr, "You entered wrong group size:\n");
        fprintf(stderr, "It should have int value>0, if you want output by groups of lines or it should be 0, if you want solid text\n");
        return -2;
    }

    if(code < 0){
        fprintf(stderr, "Invalid count of lines.\n");
        return 1;
    }

    int (*compFunc) (TFILE_INFO_NODE*, TFILE_INFO_NODE*);
    if (code == 1)
        compFunc = compareSize;
    else if (code == 2) 
        compFunc = compareName;
    else{
        fputs("Error! Incorrect value of the 2nd parameter\n", stderr);
        return 1;
    }

    TFILE_INFO_NODE* head = NULL;
     if ((code = enumFiles(argv[1], &head, argv[3])) != ALL_OK){
        switch (code)
        {
        case DIR_NOT_EXIST:
            fputs("Error! Directory doesn\'t exis\n", stderr);
            break;
        case INCORRECT_DIRECTORY:
            fputs("Error! Incorrect directory\n", stderr);
            break;

        default:
            fprintf(stderr, "Error code: %d", code);
            break;
        }

        while (head != NULL){
            TFILE_INFO_NODE* prev = head;
            head = head->ptOnNext;
            freeNode(prev);
        }
        return 1;
     }

    checkFileNames(head);   
    sortList(&head, compFunc);
    while (head != NULL){
        char* prevPath = formDirectoryName(head->info.path, head->info.name); 

        head->info.name = formFileName(head->info.name, head->info.ind);
        printf("\n%s", head->info.name);             
        if (compFunc == compareSize)
            printf(": %d", head->info.size);
        
        char* futPath = formDirectoryName(argv[3], head->info.name);
        symlink(prevPath, futPath);
        
        TFILE_INFO_NODE* prev = head;
        head = head->ptOnNext;
        freeNode(prev);
        free(prevPath);
        free(futPath);
    }
    return 0;
}

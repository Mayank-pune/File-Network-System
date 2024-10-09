#include "headers.h"

// void * nmhandler()
// {
//     data data;
//     recv(nmsockfd, &data, sizeof(data), 0);
// }


sem_t statelock;
int nmsockfd;
sem_t rwlock;
int numreaders;
pthread_mutex_t readmutex;
int clsockets[MAXCLIENTS];
char truecwd[MAXPATHLENGTH];
void removeallinside(char* path)
{
    printf("in func path %s\n", path);
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    chdir(truecwd);

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("cwd %s\n", cwd);
    if ((dir = opendir(path)) == NULL)
    {
        printf("Error opening %s\n", path);
        return;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        printf("in while\n");
        char tempp[1024];
        strcpy(tempp, path);
        strcat(tempp, "/");
        strcat(tempp, entry->d_name);
        printf("tempp %s\n", tempp);
        lstat(tempp, &statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0 || strcmp(".git", entry->d_name) == 0)
            {
                continue;
            }
            printf("REMOVING dir %s\n", entry->d_name);
            removeallinside(tempp);
            rmdir(tempp);
        }
        else if(S_ISREG(statbuf.st_mode))
        {
            printf("REMOVING file %s\n", entry->d_name);
            chdir(path);
            if(remove(entry->d_name) == 0){;}
            else{perror("errordeleting file");}
            chdir(cwd);
        }
        else{
            perror("Error getting file information");
        }

    }
    printf("done\n");
}


void copyfile(char *src, char *dest)
{
    FILE *fp, *fpcpy;
    fp = fopen(src, "r");
    fpcpy = fopen(dest, "w");
    printf("Copying file %s to %s ewuhfwh\n", src, dest);
    char c;
    while ((c = fgetc(fp)) != EOF)
    {
        fputc(c, fpcpy);
    }
    fclose(fp);
    fclose(fpcpy);
}

void copydir(char * src, char* dest, char* dirname)
{
    printf("in func\n");
    chdir(dest);
    mkdir(dirname, 0777);
    chdir(src);
    char parentpath[1024];
    strcpy(parentpath, src);
    strcat(parentpath, "/");
    strcat(parentpath, dirname);
    char copypath[1024];
    strcpy(copypath, dest);
    strcat(copypath, "/");
    strcat(copypath, dirname);
    printf("src:%s\ndest:%s\ncopypath:%s\ndirname:%s\n",src, dest, copypath, dirname);
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    if ((dir = opendir(parentpath)) == NULL)
    {
        printf("Error opening %s\n", src);
        return;
    }
    while ((entry = readdir(dir)) != NULL)
    {
        char tempp[1024];
        strcpy(tempp, parentpath);
        strcat(tempp, "/");
        strcat(tempp, entry->d_name);
        lstat(tempp, &statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0 || strcmp(".git", entry->d_name) == 0)
            {
                continue;
            }
            printf("called copydir for %s\n", entry->d_name);
            copydir(parentpath, copypath, entry->d_name);
        }
        else if(S_ISREG(statbuf.st_mode))
        {
            char parentpathfile[1024], copypathfile[1024];
            strcpy(parentpathfile, parentpath);
            strcpy(copypathfile, copypath);
            strcat(parentpathfile, "/");
            strcat(parentpathfile, entry->d_name);
            strcat(copypathfile, "/");
            strcat(copypathfile, entry->d_name);
            printf("Copying file %s\n", entry->d_name);
            copyfile(parentpathfile, copypathfile);
        }

    }
}


void getfulldir(char* dirname){
    char temp[1024];
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    strcpy(temp, cwd);
    if(dirname[0] != '\0'){strcat(temp, "/");}
    strcat(temp, dirname);  
    strcpy(dirname, temp);
    return;
}





void printserv(struct serverdata *server)
{
    printf("Serverinfo:\n");
    printf("Server IP: %s\n", server->ip);
    printf("Server Port: %d\n", server->portNM);
    printf("Server Directories:\n");
    for (int i = 0; i < server->numdirs; i++)
    {
        printf("%s\n", server->dirs[i]);
    }
    printf("Server Files:\n");
    for (int i = 0; i < server->numfiles; i++)
    {
        printf("%s\n", server->files[i]);
    }
}



void treesearch(struct serverdata *server, char *basepath)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    if ((dir = opendir(basepath)) == NULL)
    {
        printf("Error opening %s\n", basepath);
        return;
    }
    // print names of all files in dir
    while ((entry = readdir(dir)) != NULL)
    {

        char temppath[1024];
        strcpy(temppath, basepath);
        strcat(temppath, "/");
        strcat(temppath, entry->d_name);
        lstat(temppath, &statbuf);
        if (S_ISREG(statbuf.st_mode))
        {
            // printf("%*s%s\n", depth, "", entry->d_name);
            // send file name to server
            // rempve servercwd from name
            strcpy(server->files[server->numfiles], temppath + strlen(server->cwd) + 1);
            server->ogfile[server->numfiles] = 1;
            server->numfiles++;
        }
        else if (S_ISDIR(statbuf.st_mode))
        {

            // found a dir, but ignore . and ..
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0 || strcmp(".git", entry->d_name) == 0)
            {
                continue;
            }

            // remove servercwd from name

            strcpy(server->dirs[server->numdirs], temppath + strlen(server->cwd) + 1);
            server->ogdir[server->numdirs] = 1;
            server->numdirs++;
            treesearch(server, temppath);
        }
    }
}

void *handlenmsrequests(void *args)
{
    struct serverdata server;
    int sockfd = *(int *)args;
    while (1)
    {
        // listen for requests
        int request;
        printf("waiting for request\n");
        recv(sockfd, &request, sizeof(int), 0);
        printf("request: %d\n", request);
        sem_wait(&rwlock);
        switch (request)
        {
            char parentdir[MAXPATHLENGTH];
            char filename[MAXPATHLENGTH];
            char dirname[MAXPATHLENGTH];
            char filedestdir[MAXPATHLENGTH];    
            char c;
            FILE*   fp;

        case CREATEFILE:

            recv(sockfd, parentdir, sizeof(parentdir), 0);
            recv(sockfd, filename, sizeof(filename), 0);
            getfulldir(parentdir);
            printf("Creating file %s in dir %s\n", filename, parentdir);
            //go to parent dir
            chdir(parentdir);
            //create file
            fp = fopen(filename, "w");
            fclose(fp);
            break;
        case CREATEDIR:
            recv(sockfd, parentdir, sizeof(parentdir), 0);
            recv(sockfd, dirname, sizeof(dirname), 0);
            printf("Creating dir %s in dir %s\n", dirname, parentdir);
            getfulldir(parentdir);

            //go to parent dir
            chdir(parentdir);
            //create dir
            mkdir(dirname, 0777);
            break;
        
        case DELETEDIR:
            recv(sockfd, parentdir, sizeof(parentdir), 0);
            recv(sockfd, dirname, sizeof(dirname), 0);
            getfulldir(parentdir);
            printf("Deleting dir %s in dir %s\n", dirname, parentdir);
            //go to parent dir
            chdir(truecwd);
            chdir(parentdir);
            //delete dir
            removeallinside(dirname);
            printf("emptied dir\n");
            rmdir(dirname);
            break;

        case DELETEFILE:
            recv(sockfd, parentdir, sizeof(parentdir), 0);
            recv(sockfd, filename, sizeof(filename), 0);
            getfulldir(parentdir);
            printf("Deleting file %s in dir %s\n", filename, parentdir);
            //go to parent dir
            chdir(parentdir);
            //delete file
            remove(filename);
            break;
        case CPYFILE:
            int tosend;
            printf("cpyfile\n");
            recv(sockfd, &tosend, sizeof(tosend), 0);
            printf("tosend: %d\n", tosend);
            if(tosend)
            {
                recv(sockfd, parentdir, sizeof(parentdir), 0);
                recv(sockfd, filename, sizeof(filename), 0);
                getfulldir(parentdir);
                chdir(parentdir);
                //copy file
                fp = fopen(filename, "r");
                if(fp == NULL)
                {
                    printf("File not found\n");
                    break;
                }

                while ((c = fgetc(fp)) != EOF)
                {
                    printf("sent %c\n", c);
                    send(sockfd, &c, sizeof(char), 0);
                }
                c = EOF;
                send(sockfd, &c, sizeof(char), 0);
                printf("sent eof\n");
                fclose(fp);
                printf("sent file\n");
            }
            if(!tosend)
            {
                printf("recieving file\n");
                recv(sockfd, parentdir, sizeof(parentdir), 0);
                printf("recieved parentdir %s\n", parentdir);
                recv(sockfd, filename, sizeof(filename), 0);
                printf("recieved filename %s\n", filename);

                getfulldir(parentdir);
                chdir(parentdir);

                fp = fopen(filename, "w");
                recv(sockfd, &c, sizeof(char), 0);
                while (c != EOF)
                {
                    fputc(c, fp);
                    recv(sockfd, &c, sizeof(char), 0);
                }
                fclose(fp);
            }
            break;

        case HEARTBEAT:
            int hb = HEARTBEAT;
            send(sockfd, &hb, sizeof(int), 0);
            printf("throbbing\n");

        }
        int ackk = ACKNOWLEDGE;
        if(request!=HEARTBEAT)
        {send(sockfd, &ackk, sizeof(int), 0);printf("sent ack %d\n", ackk);}
        chdir(truecwd);
        sem_post(&rwlock);
        printf("done\n");
    }
    pthread_exit(NULL);
}

void* handleclrequests(void * args)
{
    printf("in\n");
    int sockfd = *(int *)args;
    int request;
    recv(sockfd, &request, sizeof(int), 0);
    printf("request: %d\n", request);
    switch (request)
    {
        char parentdir[MAXPATHLENGTH];
        char filename[MAXPATHLENGTH];
        char dirname[MAXPATHLENGTH];
        char filedestdir[MAXPATHLENGTH];
        FILE *fp;
        char c;
    case READ:
        printf("read request\n");
        pthread_mutex_lock(&readmutex);
        numreaders++;
        if(numreaders == 1)
        {
            sem_wait(&rwlock);
        }
        //read
        recv(sockfd, parentdir, sizeof(parentdir), 0);
        recv(sockfd, filename, sizeof(filename), 0);
        getfulldir(parentdir);
        chdir(parentdir);
        printf("Reading file %s in dir %s\n", filename, parentdir);
        fp = fopen(filename, "r");
        
        while ((c = fgetc(fp)) != EOF)
        {
            printf("sent %c\n", c);
            send(sockfd, &c, sizeof(char), 0);

        }
        c = EOF;
        send(sockfd, &c, sizeof(char), 0);
        numreaders--;
        if(numreaders == 0)
        {
            sem_post(&rwlock);
        }
        pthread_mutex_unlock(&readmutex);
        printf("read done\n");
        break;

    case WRITE:
        printf("write request\n");
        sem_wait(&rwlock);
        //write
        recv(sockfd, parentdir, sizeof(parentdir), 0);
        recv(sockfd, filename, sizeof(filename), 0);
        getfulldir(parentdir);
        chdir(parentdir);
        fp = fopen(filename, "w");
        while (1)
        {
            recv(sockfd, &c, sizeof(char), 0);
            if(c == EOF)
            {
                break;
            }
            fputc(c, fp);
        }
        fclose(fp);
        sem_post(&rwlock);
        break;
    case PERMISSIONS:
        printf("Permission request\n");
        pthread_mutex_lock(&readmutex);
        numreaders++;
        if(numreaders==1)
        {
            sem_wait(&rwlock);
        }
        recv(sockfd, parentdir, sizeof(parentdir), 0);
        recv(sockfd, filename, sizeof(filename), 0);
        getfulldir(parentdir);
        chdir(parentdir);
        printf("Getting permissions for file %s in dir %s\n", filename, parentdir);
        struct stat fileStat;
        if (stat(filename, &fileStat) == -1) {
            perror("Error getting file information");
            return NULL;
        }
        // Send file permissions back to client
        send(sockfd, &fileStat.st_mode, sizeof(mode_t), 0);
        numreaders--;
        if(numreaders==0)
        {
            sem_post(&rwlock);
        }
        pthread_mutex_unlock(&readmutex);
        break;
    }
    
    chdir(truecwd);
    close(sockfd);
    sem_post(&statelock);
    *(int*)args = -1;
    sem_post(&statelock);
    printf("cl done\n");
    pthread_exit(NULL);

}









int main()
{

    // get all rel inside current dir
    struct serverdata server;
    printf("enterserverno\n");
    scanf("%d", &server.serverid);
    printf("enter dir to serve as home\n");
    scanf("%s", server.cwd);
    chdir(server.cwd);
    getcwd(truecwd, MAXPATHLENGTH);
    server.portclient = clsockss + server.serverid;
    server.portNM = serverportNM;
    server.numdirs = 0;
    server.numfiles = 0;
    for (int i = 0; i < MAXNUMPATHS; i++)
    {
        strcpy(server.dirs[i], "");
        strcpy(server.files[i], "");
        server.ogfile[i] = true;
        server.ogdir[i] = true;
    }
    char ip[20] = "127.0.0.1";
    strcpy(server.ip, ip);
    getcwd(server.cwd, sizeof(server.cwd));
    treesearch(&server, server.cwd);
    // printserv(&server);
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server.portNM);
    serv_addr.sin_addr.s_addr = inet_addr(server.ip);
    printf("%d\n", server.portNM);  
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("Error connecting to server\n");
        exit(1);
    }
    send(sockfd, (void *)&server.serverid, sizeof(int), 0);
    printf("sent id %d\n", server.serverid);
    send(sockfd, (void *)&server, sizeof(struct serverdata), 0);
    pthread_t nmsrequesthandler;

    pthread_create(&nmsrequesthandler, NULL, handlenmsrequests, (void *)&sockfd);


    struct sockaddr_in cl_addr;
    int clsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clsocket < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }
    for(int i = 0; i < MAXCLIENTS; i++)
    {
        clsockets[i] = -1;
    }
    printf("clsocket:%d\n", clsocket);
    sem_init(&rwlock, 0, 1);
    sem_init(&statelock, 0, 1);
    cl_addr.sin_port = htons(server.portclient);
    cl_addr.sin_family = AF_INET;
    cl_addr.sin_addr.s_addr = INADDR_ANY;
    int claddrlen = sizeof(cl_addr);
    printf("cl_addr:%d\n", server.portclient);
    if (bind(clsocket, (struct sockaddr *)&cl_addr, sizeof(cl_addr)) == -1)
    {
        printf("Error binding socket\n");
        exit(1);
    }
    printf("bound\n");
    listen(clsocket, 5);
    printf("listening on port %d\n", server.portclient);
    while (1)
    {
        printf("waiting lock\n");
        sem_wait(&statelock);
        printf("got lock\n");
        int bhalu = -1;
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (clsockets[i] == -1)
            {
                bhalu = i;break;
            }
        }

        //accept connection
        clsockets[bhalu] = accept(clsocket, (struct sockaddr *)&cl_addr, &claddrlen);
        printf("accepted client\n");  
        pthread_t clrequesthandler;
        pthread_create(&clrequesthandler, NULL, handleclrequests, (void *)&clsockets[bhalu]);  
        sem_close(&statelock);        
    }
    close(sockfd);
}
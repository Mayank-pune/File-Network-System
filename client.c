#include "headers.h"

int main()
{
    int sockfd;
    char ip[10] = "127.0.0.1";
    printf("OH");
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(clientportNM);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    printf("%d\n", clientportNM);
    fflush(stdout);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("Error connecting to server\n");
        exit(1);
    }
    printf("Connection Successful\n");
    fflush(stdout);
    while(1){
        int requesttype;
        scanf("%d", &requesttype);
        if(requesttype == CREATEFILE || requesttype == DELETEFILE)
        {
            char parentdir[MAXPATHLENGTH];
            char filename[MAXPATHLENGTH];
            scanf("%s", parentdir);
            scanf("%s", filename);
            if(strcmp(parentdir, "home") == 0)
                strcpy(parentdir, "");
            else{
                for(int i =5; i<strlen(parentdir); i++)
                {
                    parentdir[i-5] = parentdir[i];
                }
                parentdir[strlen(parentdir)-5] = '\0';
            }
            send(sockfd, (void *)&requesttype, sizeof(int), 0);
            send(sockfd, (void *)&parentdir, sizeof(parentdir), 0);
            send(sockfd, (void *)&filename, sizeof(filename), 0);

            //receive acknowledgement
            int ack;
            recv(sockfd, (void *)&ack, sizeof(int), 0);
            if(ack == ACKNOWLEDGE)
                printf("Acknowledgement received\n");
            else
                printf("Acknowledgement not received\n");
        }
        
        else if(requesttype == CREATEDIR || requesttype == DELETEDIR)
        {
            char parentdir[MAXPATHLENGTH];
            char dirname[MAXPATHLENGTH];
            scanf("%s", parentdir);
            scanf("%s", dirname);
            if(strcmp(parentdir, "home") == 0)
                strcpy(parentdir, "");
            else{
                for(int i =5; i<strlen(parentdir); i++)
                {
                    parentdir[i-5] = parentdir[i];
                }
                parentdir[strlen(parentdir)-5] = '\0';
            }
            send(sockfd, (void *)&requesttype, sizeof(int), 0);
            send(sockfd, (void *)&parentdir, sizeof(parentdir), 0);
            send(sockfd, (void *)&dirname, sizeof(dirname), 0);
            int ack;
            recv(sockfd, (void *)&ack, sizeof(int), 0);
            if(ack == ACKNOWLEDGE)
                printf("Acknowledgement received\n");
            else
                printf("Acknowledgement not received\n");
        }
        
        else if(requesttype == CPYFILE)
        {
            char parentdir[MAXPATHLENGTH];
            char filename[MAXPATHLENGTH];
            char filedestdir[MAXPATHLENGTH];
            scanf("%s", parentdir);
            scanf("%s", filename);
            scanf("%s", filedestdir);
            if(strcmp(parentdir, "home") == 0)
                strcpy(parentdir, "");
            else{
                for(int i =5; i<strlen(parentdir); i++)
                {
                    parentdir[i-5] = parentdir[i];
                }
                parentdir[strlen(parentdir)-5] = '\0';
            }
            if(strcmp(filedestdir, "home") == 0)
                strcpy(filedestdir, "");
            else{
                for(int i =5; i<strlen(filedestdir); i++)
                {
                    filedestdir[i-5] = filedestdir[i];
                }
                filedestdir[strlen(filedestdir)-5] = '\0';
            }
            send(sockfd, (void *)&requesttype, sizeof(int), 0);
            send(sockfd, (void *)&parentdir, sizeof(parentdir), 0);
            send(sockfd, (void *)&filename, sizeof(filename), 0);
            send(sockfd, (void *)&filedestdir, sizeof(filedestdir), 0);

            int ack;
            recv(sockfd, (void *)&ack, sizeof(int), 0);
            if(ack == ACKNOWLEDGE)
                printf("Acknowledgement received\n");
            else
                printf("Acknowledgement not received\n");
        }

        else if(requesttype == CPYDIR)
        {
            char dirtoclonename[MAXPATHLENGTH];
            char dirtocloneparent[MAXPATHLENGTH];
            char dirtocloneparentdest[MAXPATHLENGTH];
            scanf("%s", dirtoclonename);
            scanf("%s", dirtocloneparent);
            scanf("%s", dirtocloneparentdest);
            if(strcmp(dirtocloneparent, "home") == 0)
                strcpy(dirtocloneparent, "");
            else{
                for(int i =5; i<strlen(dirtocloneparent); i++)
                {
                    dirtocloneparent[i-5] = dirtocloneparent[i];
                }
                dirtocloneparent[strlen(dirtocloneparent)-5] = '\0';
            }
            if(strcmp(dirtocloneparentdest, "home") == 0)
                strcpy(dirtocloneparentdest, "");
            else{
                for(int i =5; i<strlen(dirtocloneparentdest); i++)
                {
                    dirtocloneparentdest[i-5] = dirtocloneparentdest[i];
                }
                dirtocloneparentdest[strlen(dirtocloneparentdest)-5] = '\0';
            }
            send(sockfd, (void *)&requesttype, sizeof(int), 0);
            send(sockfd, (void *)&dirtoclonename, sizeof(dirtoclonename), 0);
            send(sockfd, (void *)&dirtocloneparent, sizeof(dirtocloneparent), 0);
            send(sockfd, (void *)&dirtocloneparentdest, sizeof(dirtocloneparentdest), 0);

            int ack;
            recv(sockfd, (void *)&ack, sizeof(int), 0);
            if(ack == ACKNOWLEDGE)
                printf("Acknowledgement received\n");
            else
                printf("Acknowledgement not received\n");
        }

        else if(requesttype == READ)
        {
            send(sockfd, (void *)&requesttype, sizeof(int), 0);
            char parentdir[MAXPATHLENGTH];
            char filename[MAXPATHLENGTH];
            scanf("%s", parentdir);
            scanf("%s", filename);
            if(strcmp(parentdir, "home") == 0)
                strcpy(parentdir, "");
            else{
                for(int i =5; i<strlen(parentdir); i++)
                {
                    parentdir[i-5] = parentdir[i];
                }
                parentdir[strlen(parentdir)-5] = '\0';
            }
            send(sockfd, (void *)&parentdir, sizeof(parentdir), 0);
            send(sockfd, (void *)&filename, sizeof(filename), 0);
            int servport;
            recv(sockfd, (void *)&servport, sizeof(int), 0);
            printf("Server port: %d\n", servport);
            int servsock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(servport);
            serv_addr.sin_addr.s_addr = inet_addr(ip);
            printf("%d\n", servport);
            if (connect(servsock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
            {
                printf("Error connecting to server\n");
                exit(1);
            }
            printf("Connection Successful\n");
            send(servsock, (void *)&requesttype, sizeof(int), 0);
            send(servsock, (void *)&parentdir, sizeof(parentdir), 0);
            send(servsock, (void *)&filename, sizeof(filename), 0);
            char c;
            while(1)
            {
                recv(servsock, (void *)&c, sizeof(char), 0);
                if(c == EOF)
                    break;
                printf("%c", c);
                int ack;
                recv(sockfd, (void *)&ack, sizeof(int), 0);
                if(ack == ACKNOWLEDGE)
                    printf("Acknowledgement received\n");
                    break;
            }
            
        }
        else if(requesttype==WRITE)
        {
            send(sockfd, (void *)&requesttype, sizeof(int), 0);
            char parentdir[MAXPATHLENGTH];
            char filename[MAXPATHLENGTH];
            scanf("%s", parentdir);
            scanf("%s", filename);
            if(strcmp(parentdir, "home") == 0)
                strcpy(parentdir, "");
            else{
                for(int i =5; i<strlen(parentdir); i++)
                {
                    parentdir[i-5] = parentdir[i];
                }
                parentdir[strlen(parentdir)-5] = '\0';
            }
            send(sockfd, (void *)&parentdir, sizeof(parentdir), 0);
            send(sockfd, (void *)&filename, sizeof(filename), 0);
            int servport[MAXSERVERS];
            int sockfds[MAXSERVERS];
            recv(sockfd, (void *)&servport, sizeof(servport), 0);
            for(int i =0; i<MAXSERVERS; i++)
            {
                if(servport[i] == -1){
                    sockfds[i] = -1;
                }
                else{
                    printf("Server port: %d\n", servport[i]);
                    sockfds[i] = socket(AF_INET, SOCK_STREAM, 0);
                    struct sockaddr_in serv_addr;
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(servport[i]);
                    serv_addr.sin_addr.s_addr = inet_addr(ip);
                    printf("%d\n", servport[i]);
                    if (connect(sockfds[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error connecting to server\n");
                        exit(1);
                    }
                    printf("Connection Successful\n");
                    send(sockfds[i], (void *)&requesttype, sizeof(int), 0);
                    send(sockfds[i], (void *)&parentdir, sizeof(parentdir), 0);
                    send(sockfds[i], (void *)&filename, sizeof(filename), 0);
                }
            }
            char c;
            while(1)
            {
                scanf("%c", &c);
                if(c == '*')
                    break;
                for(int i=0; i<MAXSERVERS; i++)
                {
                    if(servport[i] == -1)
                    {    break;}
                    send(sockfds[i], (void *)&c, sizeof(char), 0);
                } 
            }
            
            int ack;
            recv(sockfd, (void *)&ack, sizeof(int), 0);
            if(ack == ACKNOWLEDGE)
                printf("Acknowledgement received\n");
                break;
            c = EOF;
            for(int i=0; i<MAXSERVERS; i++)
            {
                if(servport[i] == -1)
                {    break;}
                send(sockfds[i], (void *)&c, sizeof(char), 0);
                close(sockfds[i]);
            }

        }
        else if(requesttype == PERMISSIONS)
        {
            send(sockfd, (void *)&requesttype, sizeof(int), 0);
            char parentdir[MAXPATHLENGTH];
            char filename[MAXPATHLENGTH];
            scanf("%s", parentdir);
            scanf("%s", filename);
            if(strcmp(parentdir, "home") == 0)
                strcpy(parentdir, "");
            else{
                for(int i =5; i<strlen(parentdir); i++)
                {
                    parentdir[i-5] = parentdir[i];
                }
                parentdir[strlen(parentdir)-5] = '\0';
            }
            send(sockfd, (void *)&parentdir, sizeof(parentdir), 0);
            send(sockfd, (void *)&filename, sizeof(filename), 0);
            int servport;
            recv(sockfd, (void *)&servport, sizeof(int), 0);
            printf("Server port: %d\n", servport);
            int servsock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(servport);
            serv_addr.sin_addr.s_addr = inet_addr(ip);
            printf("%d\n", servport);
            if (connect(servsock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
            {
                printf("Error connecting to server\n");
                exit(1);
            }
            printf("Connection Successful\n");
            send(servsock, (void *)&requesttype, sizeof(int), 0);
            send(servsock, (void *)&parentdir, sizeof(parentdir), 0);
            send(servsock, (void *)&filename, sizeof(filename), 0);
            char c;
            recv(servsock, (void *)&c, sizeof(char), 0);
            struct stat fileStat;
            recv(servsock, (void *)&fileStat, sizeof(fileStat), 0);

            //If want to print fileStat, enter printf statement here
            printf("Size: %lld bytes\n", (long long)fileStat.st_size);
            printf("Mode: %o\n", fileStat.st_mode);
            printf("UID: %u\n", fileStat.st_uid);
            printf("GID: %u\n", fileStat.st_gid);
            printf("Access time: %ld\n", fileStat.st_atime);
            printf("Modification time: %ld\n", fileStat.st_mtime);
            printf("Change time: %ld\n", fileStat.st_ctime);
            
            int ack;
            recv(sockfd, (void *)&ack, sizeof(int), 0);
            if(ack == ACKNOWLEDGE)
                printf("Acknowledgement received\n");
                break;
        }
        
        else
        {
            printf("Invalid request type\n");
        }
    }
    return 0;
}
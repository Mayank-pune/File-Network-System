#include "headers.h"

FILE *logfp;

int joined[MAXSERVERS];

LRUCache ServerCacheArray[MAXSERVERS];

sem_t joinlocks[MAXSERVERS];

int clsockets[MAXCLIENTS];

struct serverdata servers[MAXSERVERS];

struct hashedserverdata hashedservers[MAXSERVERS];

int sockfdarr[MAXSERVERS];

struct sockaddr_in serveraddr[MAXSERVERS];

pthread_t threads[MAXSERVERS];

sem_t writelocks[MAXSERVERS];

struct clientinfo client[MAXCLIENTS];

int numserversinit;

pthread_t trash;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t initnext = PTHREAD_COND_INITIALIZER;

pthread_cond_t initdone = PTHREAD_COND_INITIALIZER;

int intarray[MAXSERVERS];

sem_t copylock;

sem_t raidbitches;

void printserv(struct serverdata *server)
{
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
void printhashedserv(struct hashedserverdata hashed)
{
    for (int i = 0; i < HASH_MAX; i++)
    {
        if (hashed.dirs[i] != NULL)
        {
            printf("DIRS: %d\n", i);
            for (node *trav = hashed.dirs[i]; trav != NULL; trav = trav->next)
            {
                printf("%s\n", trav->name);
            }
        }
        if (hashed.files[i] != NULL)
        {
            printf("FILES: %d\n", i);
            for (node *trav = hashed.files[i]; trav != NULL; trav = trav->next)
            {
                printf("%s\n", trav->name);
            }
        }
    }
}

int clonethisshit(struct hashedserverdata *hs)
{

    struct serverdata *s = &hs->data;
    int sno = s->serverid;
    printf("cloning %d\n", sno);
    // pick 2 other servers
    int clones[2];
    clones[0] = -1;
    clones[1] = -1;
    int j = 0;
    for (int i = 0; i < MAXSERVERS; i++)
    {
        if (servers[i].status == 1 && i != sno)
        {
            clones[j] = i;
            j++;
        }
        if (j == 2)
        {
            break;
        }
    }
    if (clones[0] == -1 || clones[1] == -1)
    {
        printf("Not enough servers to clone\n");
        return -1;
    }
    printf("in %d %d\n", clones[0], clones[1]);
    struct serverdata *servclones[2];
    servclones[0] = &hashedservers[clones[0]].data;
    servclones[1] = &hashedservers[clones[0]].data;

    // acquire writelocks on all 3
    sem_wait(&writelocks[sno]);
    sem_wait(&writelocks[clones[0]]);
    sem_wait(&writelocks[clones[1]]);

    // iterate through all dirs in sno and if they aren't present in clone signal to create
    for (int i = 0; i < s->numdirs; i++)
    {
        if (!s->ogdir[i])
        {
            continue;
        }
        else
        {
            int creatdir = CREATEDIR;
            char pardir[MAXPATHLENGTH];
            char fname[MAXPATHLENGTH];
            getparent(s->dirs[i], pardir, fname);
            if (!find(s->dirs[i], hashedservers[clones[0]].dirs, clones[0]))
            {
                printf("cloning dir %s", s->dirs[i]);
                // send create dir request to clone 0
                send(sockfdarr[clones[0]], &creatdir, sizeof(int), 0);
                send(sockfdarr[clones[0]], pardir, sizeof(s->cwd), 0);
                send(sockfdarr[clones[0]], fname, sizeof(s->dirs[i]), 0);
                int ackk;
                recv(sockfdarr[clones[0]], &ackk, sizeof(int), 0);
                if (ackk == ACKNOWLEDGE)
                {
                    printf("request done\n");
                    create(s->dirs[i], hashedservers[clones[0]].dirs);
                    strcpy(servclones[0]->dirs[servclones[0]->numdirs], s->dirs[i]);
                    servclones[0]->ogdir[servclones[0]->numdirs] = false;
                    servclones[0]->numdirs++;
                }
            }

            if (!find(s->dirs[i], hashedservers[clones[1]].dirs,clones[1]))
            {
                printf("cloning dir %s", s->dirs[i]);
                // send create dir request to clone 0
                send(sockfdarr[clones[1]], &creatdir, sizeof(int), 0);
                send(sockfdarr[clones[1]], pardir, sizeof(s->cwd), 0);
                send(sockfdarr[clones[1]], fname, sizeof(fname), 0);
                int ackk;
                recv(sockfdarr[clones[1]], &ackk, sizeof(int), 0);
                if (ackk == ACKNOWLEDGE)
                {
                    printf("request done\n");
                    create(s->dirs[i], hashedservers[clones[1]].dirs);
                    strcpy(servclones[1]->dirs[servclones[1]->numdirs], s->dirs[i]);
                    servclones[1]->ogdir[servclones[1]->numdirs] = false;
                    servclones[1]->numdirs++;
                }
            }
        }
    }
    printf("all dirs done for %d\n", sno);
    // iterate thru al files and copy

    // filecopying
    for (int i = 0; i < s->numfiles; i++)
    {
        printf("file %d\n", i);
        if (!s->ogfile[i] || s->files[i][0] == '\0')
        {
            printf("reject\n");
            continue;
        }
        else
        {
            int req = CPYFILE;
            char pardir[MAXPATHLENGTH];
            char fname[MAXPATHLENGTH];
            int tosend = 1;
            getparent(s->files[i], pardir, fname);
            printf("copying file %s\n", s->files[i]);
            sem_wait(&copylock);
            send(sockfdarr[sno], &req, sizeof(int), 0);
            send(sockfdarr[sno], &tosend, sizeof(int), 0);
            send(sockfdarr[sno], pardir, sizeof(pardir), 0);
            send(sockfdarr[sno], fname, sizeof(fname), 0);
            FILE *fp = fopen("tempfile_nms", "w");
            char c;
            recv(sockfdarr[i], &c, sizeof(char), 0);
            int ackk;
            tosend = 0;
            while (c != EOF)
            {
                printf("putting %c\n", c);
                fputc(c, fp);
                recv(sockfdarr[i], &c, sizeof(char), 0);
            }
            printf("EOF recieved\n");
            recv(sockfdarr[sno], &ackk, sizeof(int), 0);
            printf("got ack uwu %d\n", ackk);
            tosend = 0;
            if (!find(s->files[i], hashedservers[clones[0]].files,clones[0]))
            {
                printf("cloning %s\n", s->files[i]);
                // send create dir request t sizeofo clone 0
                if (ackk == ACKNOWLEDGE)
                {
                    printf("in c1\n");
                    fclose(fp);
                    fp = fopen("tempfile_nms", "r");

                    send(sockfdarr[clones[0]], &req, sizeof(int), 0);
                    send(sockfdarr[clones[0]], &tosend, sizeof(int), 0);
                    send(sockfdarr[clones[0]], pardir, sizeof(pardir), 0);
                    send(sockfdarr[clones[0]], fname, sizeof(fname), 0);
                    printf("sent to %d : %d %d %s %s\n", clones[0], req, tosend, pardir, fname);
                    while ((c = fgetc(fp)) != EOF)
                    {
                        printf("putting %c\n", c);
                        send(sockfdarr[clones[0]], &c, sizeof(char), 0);
                    }
                    c = EOF;
                    send(sockfdarr[clones[0]], &c, sizeof(char), 0);
                    printf("sent eof\n");
                    fclose(fp);
                    recv(sockfdarr[clones[0]], &ackk, sizeof(int), 0);
                    printf("ack %d\n", ackk);
                    create(s->files[i], hashedservers[clones[0]].files);
                    strcpy(servclones[0]->files[servclones[0]->numfiles], s->files[i]);
                    servclones[0]->ogfile[servclones[0]->numfiles] = false;
                    servclones[0]->numfiles++;
                }
            }
            if (!find(s->files[i], hashedservers[clones[1]].files,clones[1]))
            {
                printf("cloning %s\n", s->files[i]);
                // send create dir request t sizeofo clone 0
                if (ackk == ACKNOWLEDGE)
                {
                    printf("in c2\n");
                    fp = fopen("tempfile_nms", "r");

                    send(sockfdarr[clones[1]], &req, sizeof(int), 0);
                    send(sockfdarr[clones[1]], &tosend, sizeof(int), 0);
                    send(sockfdarr[clones[1]], pardir, sizeof(pardir), 0);
                    send(sockfdarr[clones[1]], fname, sizeof(fname), 0);
                    printf("sent to %d : %d %d %s %s\n", clones[0], req, tosend, pardir, fname);
                    while ((c = fgetc(fp)) != EOF)
                    {
                        printf("putting %c\n", c);
                        send(sockfdarr[clones[0]], &c, sizeof(char), 0);
                    }
                    c = EOF;
                    send(sockfdarr[clones[1]], &c, sizeof(char), 0);
                    printf("sent eof\n");
                    fclose(fp);
                    recv(sockfdarr[clones[1]], &ackk, sizeof(int), 0);
                    printf("ack %d\n", ackk);
                    create(s->files[i], hashedservers[clones[1]].files);
                    strcpy(servclones[1]->files[servclones[1]->numfiles], s->files[i]);
                    servclones[1]->ogfile[servclones[1]->numfiles] = false;
                    servclones[1]->numfiles++;
                }
            }
        }
    }
    // release writelocks
    sem_post(&writelocks[sno]);
    sem_post(&writelocks[clones[0]]);
    sem_post(&writelocks[clones[1]]);
}

bool checkifallowed(char *name, int type) // check if oprn is allowed (all ss containg dups of name are present)
{
    bool isdir = false;
    if (type == 1)
    {
        isdir = true;
    }
    if (isdir)
    {
        for (int i = 0; i < MAXSERVERS; i++)
        {
            if (servers[i].status == -2)
            {
                if (find(name, hashedservers[i].dirs,i))
                {
                    return false;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < MAXSERVERS; i++)
        {
            if (servers[i].status == -2)
            {
                if (find(name, hashedservers[i].dirs,i))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

void *ackchecker(void *arg)
{
    int sno = *(int *)arg;
    int sockfd = sockfdarr[sno];
    int buf;
    recv(sockfd, &buf, sizeof(int), 0);
    sem_wait(&joinlocks[sno]);
    if (buf == HEARTBEAT)
    {
        joined[sno] = 1;
        sem_post(&joinlocks[sno]);
        pthread_exit(NULL);
    }
    else
    {
        printf("wtf %d\n", buf);
    }
    sem_post(&joinlocks[sno]);
}

void *throbbing_cocka(void *arg)
{
    int sno = *(int *)arg;
    int sockfd = sockfdarr[sno];
    int hb = HEARTBEAT;
    while (1)
    {
        sleep(10);
        sem_wait(&writelocks[sno]);
        // send msg and wait for ack in a thread
        // send msg
        send(sockfd, &hb, sizeof(int), 0);
        joined[sno] = 0;
        sem_post(&joinlocks[sno]);
        pthread_t thid;
        pthread_create(&thid, NULL, ackchecker, &sno);
        sleep(1);
        if (joined[sno] == 0)
        {
            printf("Server %d disconnected\n", sno);
            servers[sno].status = -2;
            sem_post(&writelocks[sno]);
            pthread_exit(NULL);
        }
        pthread_join(thid, NULL);
        sem_post(&writelocks[sno]);
    }
}

void *handleserverinit(void *fd)
{
    pthread_mutex_lock(&mutex);
    int clientfd = *(int *)fd;
    int sno;
    recv(clientfd, &sno, sizeof(int), 0);
    if (servers[sno].status == -2)
    {
        // reconnect handler
        printf("Reconnecting server %d\n", sno);
        servers[sno].status = 1;
    }
    printf("Initializing server %d\n", sno);
    sockfdarr[sno] = clientfd;
    recv(clientfd, &servers[sno], sizeof(servers[sno]), 0);
    // printserv(&servers[sno]);
    // do hashing here before lock is released

    inithashedserverdata(&servers[sno], &hashedservers[sno]);
    printserv(&servers[sno]);
    numserversinit++;
    if (numserversinit == 3)
    {
        sem_post(&raidbitches);
    }
    if (numserversinit == SERVERSINIT)
    {
        pthread_cond_signal(&initdone);
        printf("initdone\n");
    }
    servers[sno].serverid = sno;
    servers[sno].status = 1;
    printf("Server %d initialized\n", sno);
    printhashedserv(hashedservers[sno]);
    sem_init(&writelocks[sno], 0, 1);
    sem_init(&joinlocks[sno], 0, 0);
    pthread_t trsh;
    //pthread_create(&trsh, NULL, throbbing_cocka, &intarray[sno]);
    pthread_mutex_unlock(&mutex);
    sem_wait(&raidbitches);
    clonethisshit(&hashedservers[sno]);
    sem_post(&raidbitches);
    pthread_exit(NULL);
}
void getparent(char *name, char *parent, char *filename)
{
    if (strcmp(name, "") == 0)
    {
        strcpy(parent, "");
        return;
    }
    int i = strlen(name) - 1;
    while (name[i] != '/' && i > 0)
    {
        i--;
    }
    strcpy(parent, name);
    parent[i] = '\0';
    // copy rest of name to filename
    strcpy(filename, name + i + 1);
    if (i == 0)
    {
        strcpy(filename, name);
    }
}
void *handleclient(void *arg)
{
    // store client info the the client struct

    struct clientinfo *client = (struct clientinfo *)arg;
    // store client info
    while (1)
    {
        // receive client request
        int requesttype;
        recv(client->sockfd, &requesttype, sizeof(int), 0);
        printf("Request type: %d\n", requesttype);
        fflush(stdout);
        // fprintf("Client %d has requested %d\n", client->sockfd, requesttype);
        // fflsuh(logfp);
            char parentdir[MAXPATHLENGTH];
            char filename[MAXPATHLENGTH];
            char data[1024];
            char dirtoclonename[MAXPATHLENGTH];
            char dirtocloneparent[MAXPATHLENGTH];
            char dirtocloneparentdest[MAXPATHLENGTH];
            char filedestdir[MAXPATHLENGTH];
            char dirname[MAXPATHLENGTH];
            char nname[MAXPATHLENGTH];
            char filefull[MAXPATHLENGTH];
            char c;
            int fnd = 0;
            int servtocontact = -1;
            FILE *fp;
            node *n;
            node *prev;
            memset(parentdir, 0, sizeof(parentdir));
            memset(filename, 0, sizeof(filename));
            memset(data, 0, sizeof(data));
            memset(dirtoclonename, 0, sizeof(dirtoclonename));
            memset(dirtocloneparent, 0, sizeof(dirtocloneparent));
            memset(dirtocloneparentdest, 0, sizeof(dirtocloneparentdest));
            memset(filedestdir, 0, sizeof(filedestdir));
            memset(dirname, 0, sizeof(dirname));
            memset(nname, 0, sizeof(nname));
        switch (requesttype)
        {

        case CREATEFILE:
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, filename, sizeof(filename), 0);
            fprintf("Client %d wants to create file %s in directory %s\n", client->sockfd, filename, parentdir);
            fflsuh(logfp); 
            printf("Parent dir: %s\n", parentdir);
            printf("Filename: %s\n", filename);
            if (!checkifallowed(filename, 1))
            {
                printf("releavant server dead\n");
                break;
            }
            // iterate through the linked list at that hash for all servers
            for (int i = 0; i < MAXSERVERS; ++i)
            {
                if (servers[i].status > 0)
                {
                    int found = find(parentdir, hashedservers[i].dirs,i);
                    printf("searching in server %d \n", i);
                    if (found)
                    {
                        printf("Sending request to server %d\n", i);
                        sem_wait(&writelocks[i]);
                        struct serverdata concernedserver = servers[i];
                        send(sockfdarr[i], &requesttype, sizeof(int), 0);
                        send(sockfdarr[i], parentdir, sizeof(parentdir), 0);
                        send(sockfdarr[i], filename, sizeof(filename), 0);
                        int ackk;
                        recv(sockfdarr[i], &ackk, sizeof(int), 0);
                        if (ackk == ACKNOWLEDGE)
                        {
                            printf("request done\n");
                        }
                        // add file to hash
                        strcpy(nname, parentdir);
                        if (parentdir[0] != '\0')
                        {
                            strcat(nname, "/");
                        }
                        strcat(nname, filename);
                        create(nname, hashedservers[i].files);
                        strcpy(hashedservers[i].data.files[hashedservers[i].data.numfiles], nname);

                        sem_post(&writelocks[i]);
                        printf("i: %d\n", i);
                        //printhashedserv(hashedservers[i]);
                    }
                }
            }
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case CREATEDIR:
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, dirname, sizeof(dirname), 0);
            fprintf("Client %d wants to create directory %s in directory %s\n", client->sockfd, dirname, parentdir);
            fflsuh(logfp);
            if (!checkifallowed(filename, 1))
            {
                printf("releavant server dead\n");
                break;
            }
            for (int i = 0; i < MAXSERVERS; ++i)
            {
                if (servers[i].status > 0)
                {
                    int found = find(parentdir, hashedservers[i].dirs,i);
                    if (found)
                    {
                        sem_wait(&writelocks[i]);
                        struct serverdata concernedserver = servers[i];
                        // send request to that server
                        send(sockfdarr[i], &requesttype, sizeof(int), 0);
                        send(sockfdarr[i], parentdir, sizeof(parentdir), 0);
                        send(sockfdarr[i], dirname, sizeof(filename), 0);
                        int ackk;
                        recv(sockfdarr[i], &ackk, sizeof(int), 0);
                        if (ackk == ACKNOWLEDGE)
                        {
                            printf("request done\n");
                        }
                        strcpy(nname, parentdir);
                        if (parentdir[0] != '\0')
                        {
                            strcat(nname, "/");
                        }
                        strcat(nname, dirname);
                        create(nname, hashedservers[i].dirs);
                        strcpy(hashedservers[i].data.dirs[hashedservers[i].data.numdirs], nname);
                        sem_post(&writelocks[i]);
                        printf("i: %d\n", i);
                        printhashedserv(hashedservers[i]);
                    }
                }
            }
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case DELETEFILE:
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, filename, sizeof(filename), 0);
            fprintf("Client %d wants to delete file %s in directory %s\n", client->sockfd, filename, parentdir);
            fflsuh(logfp);
            char fullpth[MAXPATHLENGTH] = "";
            strcpy(fullpth, parentdir);
            if (parentdir[0] != '\0')
            {
                strcat(fullpth, "/");
            }
            strcat(fullpth, filename);
            printf("deleting %s\n", fullpth);
            if (!checkifallowed(filename, 0))
            {
                printf("releavant server dead\n");
                break;
            }
            for (int i = 0; i < MAXSERVERS; i++)
            {
                if (servers[i].status > 0)
                {
                    int found = find(fullpth, hashedservers[i].files,i);
                    if (found)
                    {
                        sem_wait(&writelocks[i]);
                        struct serverdata concernedserver = servers[i];
                        // send request to that server
                        send(sockfdarr[i], &requesttype, sizeof(int), 0);
                        send(sockfdarr[i], parentdir, sizeof(parentdir), 0);
                        send(sockfdarr[i], filename, sizeof(filename), 0);
                        int ackk;
                        recv(sockfdarr[i], &ackk, sizeof(int), 0);
                        if (ackk == ACKNOWLEDGE)
                        {
                            printf("request done\n");
                        }
                        // find node with filename
                        int twofind = get_hash(fullpth);
                        printf("2find%d\n", twofind);
                        n = hashedservers[i].files[twofind];
                        prev = NULL;
                        while (n != NULL || prev != NULL)
                        {
                            if (strcmp(n->name, fullpth) == 0)
                            {
                                break;
                            }
                            prev = n;
                            if (n == NULL)
                            {
                                break;
                            }
                            n = n->next;
                        }
                        if (n == NULL)
                        {
                            printf("File not found\n");
                            break;
                        }
                        destroy(prev);
                        // replace entry in files with empty
                        for (int i = 0; i < MAXNUMPATHS; i++)
                        {
                            if (strcmp(hashedservers[i].data.files[i], fullpth) == 0)
                            {
                                strcpy(hashedservers[i].data.files[i], "");
                                break;
                            }
                        }
                        sem_post(&writelocks[i]);
                    }
                }
            }
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case DELETEDIR:
            printf("delete dir request\n");
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, dirname, sizeof(dirname), 0);
            fprintf("Client %d wants to delete directory %s in directory %s\n", client->sockfd, dirname, parentdir);
            fflsuh(logfp);
            printf("deets:%s %s\n", parentdir, dirname);
            strcpy(fullpth, parentdir);
            if (fullpth[0] != '\0')
            {
                strcat(fullpth, "/");
            }
            strcat(fullpth, dirname);
            if (!checkifallowed(parentdir, 1))
            {
                printf("releavant server dead\n");
                break;
            }
            for (int i = 0; i < MAXSERVERS; i++)
            {
                if (servers[i].status > 0)
                {
                    int found = find(fullpth, hashedservers[i].dirs,i);
                    if (found)
                    {
                        printf("found in %d\n", i);
                        sem_wait(&writelocks[i]);
                        struct serverdata concernedserver = servers[i];
                        // send request to that server
                        send(sockfdarr[i], &requesttype, sizeof(int), 0);
                        send(sockfdarr[i], parentdir, sizeof(parentdir), 0);
                        send(sockfdarr[i], dirname, sizeof(filename), 0);
                        printf("sent %s %s\n", parentdir, dirname);
                        int ackk;
                        recv(sockfdarr[i], &ackk, sizeof(int), 0);
                        if (ackk == ACKNOWLEDGE)
                        {
                            printf("request done\n");
                        }
                        int twofind = get_hash(fullpth);
                        n = hashedservers[i].dirs[twofind];
                        prev = NULL;
                        while (n != NULL)
                        {
                            printf("wss\n");
                            if (strcmp(n->name, fullpth) == 0)
                            {
                                printf("found %s\n", n->name);
                                break;
                            }
                            prev =n;
                            n = n->next;
                            printf("wsssi\n");
                        }
                        
                        if(prev==NULL && n==NULL){printf("not found\n");break;}
                        else if(prev == NULL && n!=NULL){
                            hashedservers[i].dirs[twofind] = n->next;
                            free(n);
                        }
                        else{
                            printf("found 2\n");
                            destroy(prev);
                        }
                        twofind = get_hash_parent(fullpth);
                        printf("w %d\n", twofind);
                        n = hashedservers[i].files[twofind];
                        prev = NULL;
                        while (n != NULL)
                        {
                            printf("wss\n");
                            if (strstr(n->name, fullpth) != NULL)
                            {
                                if (prev == NULL)
                                {
                                    printf("o\n");
                                    hashedservers[i].files[twofind] = n->next;
                                    prev = n->next;
                                    free(n);
                                    n = prev;
                                    prev = NULL;
                                    printf("k\n");
                                }
                                else
                                {
                                    printf("oo\n");
                                    destroy(prev);
                                    printf("kk\n");
                                    n = prev->next;
                                }
                            }
                            else
                            {
                                printf("not removed file %s\n", n->name);
                                prev = n;
                                n = n->next;
                            }
                            printf("wsssi\n");

                        }
                        printf("wssssiiiii\n");
                        n = hashedservers[i].dirs[twofind];
                        prev = NULL;
                        while (n != NULL)
                        {
                            printf("wss\n");
                            if (strstr(n->name, fullpth) != NULL)
                            {
                                if (prev == NULL)
                                {
                                    printf("ooo\n");
                                    hashedservers[i].dirs[twofind] = n->next;
                                    prev = n->next;
                                    free(n);
                                    n = prev;
                                    prev = NULL;
                                    printf("kkk\n");
                                }
                                else
                                {
                                    printf("oooo\n");
                                    destroy(prev);
                                    printf("kkkk\n");
                                    n = prev->next;
                                }
                            }
                            else
                            {
                                printf("not removed file %s\n", n->name);
                                prev = n;
                                n = n->next;
                            }

                        }

                        for (int ii = 0; ii < MAXNUMPATHS; ii++)
                        {
                            if (ii<hashedservers[i].data.numfiles && strstr(hashedservers[i].data.files[ii], fullpth) != NULL)
                            {
                                strcpy(hashedservers[i].data.files[i], "");
                            }
                            printf("1\n");
                            if ( ii<hashedservers[i].data.numdirs ||strstr(hashedservers[i].data.dirs[ii], fullpth) != NULL)
                            {
                                strcpy(hashedservers[i].data.dirs[i], "");
                            }
                            printf("2\n");
                        }
                        sem_post(&writelocks[i]);
                    }
                }
                printf("i: %d\n", i);
                //printhashedserv(hashedservers[i]);
            }
            printf("done del\n");
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case CPYFILE:
            printf("copy request\n");
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, filename, sizeof(filename), 0);
            recv(client->sockfd, filedestdir, sizeof(filedestdir), 0);

            fprintf("Client %d wants to copy file %s in directory %s to directory %s\n", client->sockfd, filename, parentdir, filedestdir);
            fflsuh(logfp);
            
            printf("Parent dir: %s\n", parentdir);
            printf("Filename: %s\n", filename);
            printf("File destination dir: %s\n", filedestdir);
            char fullfilepath[MAXPATHLENGTH] = "";
            strcpy(fullfilepath, parentdir);
            if (parentdir[0] != '\0')
            {
                strcat(fullfilepath, "/");
            }
            strcat(fullfilepath, filename);
            bool fnded = false;
            for (int i = 0; i < MAXSERVERS; ++i)
            {
                if (servers[i].status > 0)
                {
                    if (find(fullfilepath, hashedservers[i].files,i))
                    {
                        sem_wait(&writelocks[i]);
                        struct serverdata concernedserver = servers[i];
                        // send request to that server
                        send(sockfdarr[i], &requesttype, sizeof(int), 0);
                        int tosend = 1;
                        send(sockfdarr[i], &tosend, sizeof(int), 0);
                        send(sockfdarr[i], parentdir, sizeof(parentdir), 0);
                        send(sockfdarr[i], filename, sizeof(filename), 0);
                        fp = fopen("tempfile_nms", "w");
                        recv(sockfdarr[i], &c, sizeof(char), 0);
                        while (c != EOF)
                        {
                            printf("putting %c\n", c);
                            fputc(c, fp);
                            recv(sockfdarr[i], &c, sizeof(char), 0);
                        }
                        int ackk;
                        recv(sockfdarr[i], &ackk, sizeof(int), 0);
                        {
                            printf("request done\n");
                        }
                        sem_post(&writelocks[i]);
                        fclose(fp);
                        fnded = true;
                        goto setup;
                    }
                }
            }
        setup:
            if (!fnded)
            {
                printf("File not found\n");
                fprintf("File requested by Client %d not found\n", client->sockfd);
                fflsuh(logfp);
                send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
                break;
            }
            else if (!checkifallowed(filedestdir, 0))
            {
                printf("releavant server dead\n");
                send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
                break;
            }
            else
            {
                fp = fopen("tempfile_nms", "r");
                for (int i = 0; i < MAXSERVERS; i++)
                {
                    if (servers[i].status > 0)
                    {
                        // look for folders containg destdir
                        if (find(filedestdir, hashedservers[i].dirs,i))
                        {
                            sem_wait(&writelocks[i]);
                            struct serverdata concernedserver = servers[i];
                            // send request to that server
                            send(sockfdarr[i], &requesttype, sizeof(int), 0);
                            int tosend = 0;
                            send(sockfdarr[i], &tosend, sizeof(int), 0);
                            send(sockfdarr[i], filedestdir, sizeof(filedestdir), 0);
                            send(sockfdarr[i], filename, sizeof(filename), 0);
                            while ((c = fgetc(fp)) != EOF)
                            {
                                send(sockfdarr[i], &c, sizeof(char), 0);
                            }
                            c = EOF;
                            send(sockfdarr[i], &c, sizeof(char), 0);
                            int ackk;
                            recv(sockfdarr[i], &ackk, sizeof(int), 0);
                            if (ackk == ACKNOWLEDGE)
                            {
                                printf("request done\n");
                            }
                            sem_post(&writelocks[i]);
                        }
                    }
                }
            }
            fclose(fp);
            remove("tempfile_nms");
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case READ:
            printf("read request\n");
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, filename, sizeof(filename), 0);
            fprintf("Client %d wants to read file %s in directory %s\n", client->sockfd, filename, parentdir);
            fflsuh(logfp);
            strcpy(filefull, parentdir);

            if (parentdir[0] != '\0')
            {
                strcat(filefull, "/");
            }
            strcat(filefull, filename);

            for (int i = 0; i < MAXSERVERS; i++)
            {
                fnd = find(filefull, hashedservers[i].files,i);
                printf("searching in server %d\n", i);
                if (fnd == 1)
                {
                    servtocontact = i;
                    send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
                    break;
                }
            }
            if (servtocontact == -1)
            {
                printf("File not found\n");
                send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
                break;
            }
            send(client->sockfd, &servers[servtocontact].portclient, sizeof(int), 0);
            fprintf(logfp,"Sent the relevant server port details to client\n");
            fflush(logfp);
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case WRITE:
            printf("write request\n");
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, filename, sizeof(filename), 0);
            fprintf("Client %d wants to write file %s in directory %s\n", client->sockfd, filename, parentdir);
            fflush(logfp);
            strcpy(filefull, parentdir);

            int portstocontact[MAXSERVERS];
            if (parentdir[0] != '\0')
            {
                strcat(filefull, "/");
            }
            strcat(filefull, filename);
            if(!checkifallowed(filefull, 0)){printf("not found/ critical server missing\n"); break;}
            for (int i = 0; i < MAXSERVERS; i++)
            {
                fnd = find(filefull, hashedservers[i].files,i);
                printf("searching in server %d\n", i);
                if (fnd == 1)
                {
                    portstocontact[i] = hashedservers[i].data.portclient;
                }
                else{
                    portstocontact[i] = -1;
                }
            }        
            send(client->sockfd, portstocontact, sizeof(portstocontact), 0);
            fprintf(logfp,"Sent the relevant server port details to client\n");
            fflush(logfp);
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case PERMISSIONS:
            printf("permissions request\n");
            recv(client->sockfd, parentdir, sizeof(parentdir), 0);
            recv(client->sockfd, filename, sizeof(filename), 0);
            fprintf("Client %d wants to read permissions of file %s in directory %s\n", client->sockfd, filename, parentdir);
            fflsuh(logfp);
            char filefull2[MAXPATHLENGTH] = "";
            strcpy(filefull2, parentdir);
            if(parentdir[0]!='\0')
            {
                strcat(filefull2, "/");
            }
            strcat(filefull2, filename);
            int fnd2 = 0;
            int servtocontact2 = -1;
            printf("%s\n",filefull2);
            for(int i =0; i < MAXSERVERS; i++)
            {
                fnd2 = find(filefull2, hashedservers[i].files,i);
                if(fnd2 == 1){servtocontact2 = i;break;}
            }
            if(servtocontact2 == -1){printf("File not found\n");break;}
            send(client->sockfd , &servers[servtocontact2].portclient, sizeof(int), 0);
            fprintf(logfp,"Sent the relevant server port details to client\n");
            fflush(logfp);
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            break;
        case CPYDIR:
            printf("copy dir request\n");
            recv(client->sockfd, dirtoclonename, sizeof(dirtoclonename), 0);
            recv(client->sockfd, dirtocloneparent, sizeof(dirtocloneparent), 0);
            recv(client->sockfd, dirtocloneparentdest, sizeof(dirtocloneparentdest), 0);
            fprintf("Client %d wants to copy directory %s in directory %s to directory %s\n", client->sockfd, dirtoclonename, dirtocloneparent, dirtocloneparentdest);
            fflsuh(logfp);
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
        case EXIT:
            fprintf("Client %d wants to quit\n", client->sockfd);
            fflsuh(logfp);
            send(client->sockfd, ACKNOWLEDGE, sizeof(int), 0);
            //close the client socket
            close(client->sockfd);
            pthread_exit(NULL);
            break;
        default:
            printf("Invalid request type\n");
        }
    end:
    }
}

void *listener(void *args)
{
    int sockfd = *(int *)args;
    listen(sockfd, MAXSERVERS);
    int temparr[MAXSERVERS];
    int i = 0;
    while (1)
    {

        int sz = sizeof(serveraddr[i]);
        temparr[i] = accept(sockfd, (struct sockaddr *)&serveraddr[i], &sz);
        if (temparr[i] < 0)
        {
            printf("Error accepting server\n");
            exit(1);
        }
        printf("Connected to server %d\n", i);
        pthread_create(&threads[i], NULL, handleserverinit, &temparr[i]);
        i++;
    }
}

int main()
{
    logfp = fopen("logs.txt", "a");
    if(logfp==NULL)
    {
        printf("Error opening log file\n");
        exit(1);
    }
    fprintf(logfp,"NMS Server is up & running");
    fflush(logfp);
    sem_init(&raidbitches, 0, 0);
    sem_init(&copylock, 0, 1);
    for (int i = 0; i < MAXSERVERS; i++)
    {
        intarray[i] = i;
        servers[i].status = -1;
        ServerCacheArray[i] = *createLRUCache(MAXCACHESIZE, i);
    }
    numserversinit = 0;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverportNM);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Error binding socket\n");
        exit(1);
    }
    fprintf(logfp, "All Servers Initialized\n");
    fflush(logfp);
    printf("notlistening\n");
    pthread_mutex_lock(&mutex);

    printf("listening\n");
    pthread_create(&trash, NULL, listener, (void *)&sockfd);
    pthread_cond_wait(&initdone, &mutex);
    pthread_mutex_unlock(&mutex);
    int client_init_sockfd;
    struct sockaddr_in client_init_addr;
    client_init_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_init_sockfd < 0)
    {
        printf("Error opening socket\n");
        exit(1);
    }

    client_init_addr.sin_family = AF_INET;
    client_init_addr.sin_port = htons(clientportNM);
    client_init_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(client_init_sockfd, (struct sockaddr *)&client_init_addr, sizeof(client_init_addr)) == -1)
    {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    listen(client_init_sockfd, MAXCLIENTS);
    int j = 0;
    while (1)
    {
        printf("Start\n");
        int clientlen = sizeof(client[j].client_addr);
        client[j].sockfd = accept(client_init_sockfd, (struct sockaddr *)&client[j].client_addr, &clientlen);
        if (client[j].sockfd < 0)
        {
            printf("Error accepting client\n");
            exit(1);
        }
        printf("Connected to client\n");
        fprintf(logfp, "Connected to Client %d\n", j);
        // Create a thread to handle this client's requests
        pthread_t thread;
        pthread_create(&thread, NULL, handleclient, (void *)&client[j]);
        j++;
    }
}
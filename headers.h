#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>

#define MAXPATHLENGTH 100
#define MAXNUMPATHS 100
#define MAXCLIENTS 1
#define MAXSERVERS 1
#define SERVERSINIT 2
#define HASH_MAX 10

#define MAXCACHESIZE 5

//request types ie send these types of messages b4 senging info eg. if you want to initalize servers first send INIT and then the details of the server
#define CREATEFILE 1
#define CREATEDIR 2
#define DELETEDIR 3
#define DELETEFILE 4
#define CPYFILE 5
#define CPYDIR 6
#define READ 7
#define WRITE 8
#define PERMISSIONS 9
#define HEARTBEAT 1919
#define ACKNOWLEDGE 15
#define EXIT 1769


#define MAXBUFFERLENGTH 1024
#define clientportNM 6970
#define serverportNM 8089
#define clsockss 9099   
//etc. define all these here

 extern int nmsip;

 typedef struct data{
 	char* data;
}data;

struct serverdata
{
    int status;
    int serverid;
    int portNM;
    int portclient;
    char ip[20];
	int numdirs;
    char dirs[MAXNUMPATHS][MAXPATHLENGTH];
    int numfiles;
    char files[MAXNUMPATHS][MAXPATHLENGTH];
    bool ogdir[MAXNUMPATHS];
    bool ogfile[MAXNUMPATHS];
    char cwd[MAXPATHLENGTH];
};

typedef struct node
{
    char *name;
    struct node *next;
} node;

struct  hashedserverdata
{
    node *files[HASH_MAX];
    node *dirs[HASH_MAX];
    struct serverdata data;
};

struct clientinfo
{
    int sockfd;
    struct sockaddr_in client_addr;
};

//Hashing Functions
int get_hash(char *s1);

int get_hash_parent(char *s1);

int create(char *a, node **hashtable);

void destroy(node *node_ptr);

int find(char *a, node **hashtable, int ssnum);

void inithashedserverdata(struct serverdata *toHash, struct hashedserverdata *hashed);

//DLL (LRU Finctions)
// Define a structure for a doubly linked list node
typedef struct Node {
    char *data;
    struct Node *prev;
    struct Node *next;
} Node;

// Define a structure for the LRU cache
typedef struct {
    int capacity;
    int size;
    Node *head;
    Node *tail;
} LRUCache;

extern LRUCache ServerCacheArray[MAXSERVERS];

Node *createNode(const char *data);

LRUCache *createLRUCache(int capacity, int ssnum);

void moveToFront(LRUCache *cache, Node *node);

void removeTail(LRUCache *cache);

void insertAtFront(LRUCache *cache, const char *data);

int get(LRUCache *cache, const char *data);

void put(LRUCache *cache, const char *data);

void displayLRUCache(LRUCache *cache);

void destroyLRUCache(LRUCache *cache);

//request types ie send these types of messages b4 senging info eg. if you want to initalize servers first send INIT and then the details of the server

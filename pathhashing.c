#include "headers.h"

char* getparentdir1(char *path)
{
    char *parentdir = malloc(sizeof(char) * MAXPATHLENGTH);
    strcpy(parentdir, path);
    int j = strlen(parentdir) - 1;
    while (parentdir[j] != '/' && j > 0)
    {
        j--;
    }
    parentdir[j] = '\0';
    return parentdir;
}

int get_hash_parent(char *s)
{
    long long p = 31, m = HASH_MAX;
    long long hash = 0;
    long long p_pow = 1;
    int n = strlen(s);
    for (int i = 0; i < n; i++)
    {
        hash = (hash + (s[i] + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash;
}


int get_hash(char *s1)
{
    char *s = getparentdir1(s1);
    long long p = 31, m = HASH_MAX;
    long long hash = 0;
    long long p_pow = 1;
    int n = strlen(s);
    for (int i = 0; i < n; i++)
    {
        hash = (hash + (s[i] + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash;
}

int create(char *a, node **hashtable)
{
    int n = get_hash(a);
    node *new_node = malloc(sizeof(node));
    if (new_node == NULL)
        exit(1);
    else
    {
        new_node->name = malloc(sizeof(char) * MAXPATHLENGTH);
        strcpy(new_node->name, a);
        new_node->next = hashtable[n];
        hashtable[n] = new_node;
    }
    return n;
}

void destroy(node *node_ptr)
{
    if (node_ptr == NULL)
        {return;}
    else
    if (node_ptr->next == NULL)
        printf("errm\n");
    else
    {
        node_ptr->next = node_ptr->next->next;
        printf("removing %s\n", node_ptr->next->name); 
        free(node_ptr->next);
    }
    return;
}

int find(char *a, node **hashtable, int ssnum)
{
    if(strcmp(a, "") == 0)
        return 1; //home in every directory
    int n = get_hash(a);

    //1 if found, -1 if not
    int LRUget = get( &ServerCacheArray[ssnum] , a );
    if(LRUget==1)
    {
        printf("Cache Hit\n");
        return LRUget;
    }
    //File not present inside cache
    if(LRUget==-1)
    {
        put(&ServerCacheArray[ssnum], a);
    }
    if (hashtable[n] != NULL)
    {
        for (node *trav = hashtable[n]; trav != NULL; trav = trav->next)
        {
            if (strcmp(trav->name, a) == 0)
                return 1;
        }
    }
    return 0;
}

void inithashedserverdata(struct serverdata *toHash, struct hashedserverdata *hashed)
{
    memcpy(&hashed->data, toHash, sizeof(struct serverdata));
    for (int i = 0; i < HASH_MAX; i++)
    {
        hashed->dirs[i] = NULL;
        hashed->dirs[i] = NULL;
    }
    for (int j = 0; j < toHash->numdirs; j++)
    {
        create(toHash->dirs[j], hashed->dirs);
    }
    for (int k = 0; k < toHash->numfiles; k++)
    {
        create(toHash->files[k], hashed->files);
    }
}

// int main()
// {
//     node *hashtable[HASH_MAX];
//     // set pointers to NULL!
//     for (int i = 0; i < HASH_MAX; i++)
//     {
//         hashtable[i] = NULL;
//     }

//     // get the band together
//     int x = create("John", hashtable);
//     int y = create("Paul", hashtable);
//     int z = create("Ringo", hashtable);
//     int c = create("George", hashtable);
//     fprintf(stdout, "%d %s\n%d %s\n%d %s\n%d %s\n", x, hashtable[x]->name, y, hashtable[y]->name, z,
//             hashtable[z]->name, c, hashtable[c]->name);

//     int yes = find("Ringo", hashtable);
//     int no = find("Jerry", hashtable);
//     fprintf(stdout, "%d\n%d\n", yes, no);

//     // let it be (free)
//     for (int j = 0; j < HASH_MAX; j++)
//     {
//         if (hashtable[j] != NULL)
//         {
//             destroy(hashtable[j]);
//         }
//     }
//     return 0;
// }
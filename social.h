/*
Copyright (c) 2003-2006 by ccert@tsinghua.edu.cn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

extern struct sockaddr_in fromSock;       //add
extern struct sockaddr_in6 fromSock6;     //add
extern int isSocialProxy;
extern unsigned char PrtPrxKey;

extern pthread_mutex_t mutex;




int insertClientProxy(char *buf,int i);   //humeng add 11.4.23

/*define a client proxy struct*/
typedef struct _cProxy                  //humeng add 11.4.23
{
    struct _cProxy *next;
    AtomPtr addr;
    AtomPtr key;
    unsigned char keyHash; 
    unsigned int delay;
}ClientProxyRec, *ClientProxyPtr;

extern ClientProxyPtr clientProxys;     //humeng add 11.4.23

/*define a parent proxy struct*/
typedef struct _pProxy                  //humeng add 11.5.10
{
    struct _pProxy *next;
    AtomPtr addr;
    AtomPtr port;
    AtomPtr key;
    unsigned char keyHash;
    unsigned int delay;    
    NetAddressPtr allowIP;
}ParentProxyRec, *ParentProxyPtr;

extern ParentProxyPtr ParentProxys;     //humeng add 11.5.10

typedef struct _Neighbor
{   int isSocialProxy;
    unsigned int key;
    //algorithm
 }NeighborRec, *NeighborPtr;

NeighborRec neighbor[1024*8];

int insertParentProxy(char *buf,int i);  //humeng add 11.5.10
//void insertParentProxy(ConfigVariablePtr var, AtomPtr av);   //add

//added by lidehu*****//
unsigned char keyHasher(char *key, unsigned short keylen);
void initNeighbor();
void checkAcceptClientProxy(struct sockaddr_in addr, int fd);
void findKeyAlg(int fd);
int encryData(char *buf, int n, void *key , int keylen, char *encrtbuf, int *len); 
int decryData(char *buf, int n, void *key ,int keylen, /*char *decrybuf,*/ int *len); 
int social_READ(int fd, char *buf, int n);
int social_READV(int fd, const struct iovec *vector, int count);
int social_WRITE(int fd, char *buf, int n);
int social_WRITEV(int fd, const struct iovec *vector, int count);
//added by lidehu****//
/*********add by humeng @ 2011.5.16-17 ******/
int delClientProxy(char *buf);
int addClientProxy(char *clientAddr, char *clientKey);
int eraseClientProxys();
int delParentProxy(char *parentAddr);
int addParentProxy(char *parentAddr,char *parentKey,char *srvIP,char *srvPort);
int eraseParentProxys();
int alternateParentProxys();
int setCurrentParentProxy(char *parentAddr);
int updateParentProxy(char *parentAddr,char *parentKey,char *srvIP);
/*********add by humeng @ 2011.5.16-17 ******/

int printProxys();  //add by humeng @ 2011.5.16-19  test thread
//void Print_Thread(void* arg);   //add by humeng @ 2011.5.16-19  test thread
//void Set_Thread(void* arg);   //add by humeng @ 2011.5.16-19  test thread

//Example:
    //setCurrentParentProxy("99.237.56.138");
    //setCurrentParentProxy("163.43.162.176");
    //alternateParentProxys();
    //addParentProxy("166.111.132.140","12345","59.66.24.177");
    //addParentProxy("163.43.162.176","123456","59.66.24.178");
    //eraseParentProxys();
    //delParentProxy("163.43.162.176");
    //delParentProxy("192.168.1.101");
    //delParentProxy("166.111.132.140");
    //delClientProxy("166.111.132.140");
    //eraseClientProxys();
    //addClientProxy("166.111.132.141","123456");
    //addClientProxy("166.111.132.140","123456789");



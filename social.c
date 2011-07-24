#include "polipo.h"
//#include <pthread.h>

NetAddressPtr clientRange;
AtomListPtr findProxyClient(char *line);
ClientProxyPtr clientProxys=NULL;   //humeng add 11.4.23
ParentProxyPtr ParentProxys=NULL;   //humeng add 11.5.10

static int skipBlank(char *buf,int i)   //humeng add 11.4.23
{
    while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r')
        i++;
    return i;
}
/*

typedef struct _Atom {
    unsigned int refcount;
    struct _Atom *next;
    unsigned short length;
    char string[1];
} AtomRec, *AtomPtr;*/
unsigned char keyHasher(char *key, unsigned short keylen)
{
   unsigned short i=0;
   unsigned char hash=0;
   if(key==NULL|keylen==0) return 0xff;
   for(i=0; i<keylen; i++)
   {
  	hash=hash^key[i];
   }
   return hash;
}
int insertClientProxy(char *buf,int i)  //humeng add 11.4.23
{
    ClientProxyPtr node,cltPrxs;
    AtomPtr temp,addr,key;
    int x,y,rc;
    if(!buf[i]=='=') return(0);
    i++;
    skipBlank(buf,i);
    x=i;
    while(!(buf[i] == '\n' || buf[i] == '\0' || buf[i] == '#'))
    {
        skipBlank(buf,i);
        i++;
    }
    if(i>x+1)
    temp=internAtomN(buf+x,i-x-1);
    rc = atomSplit(temp, ':', &addr, &key);
    if(rc <= 0) {
        do_log(L_ERROR, "Couldn't parse parentProxy.");
        return -1;
    }
    node=malloc(sizeof(ClientProxyRec));
    node->next=NULL;
    node->addr=addr;
    node->key=key;
    node->keyHash=keyHasher(key->string, key->length);
    if(clientProxys==NULL)
    {
        clientProxys=node;
        return 1;
    }else
    {
        cltPrxs=clientProxys;
        while(cltPrxs)
        {
             if(strcmp(cltPrxs->addr->string,node->addr->string)==0)
             {
                  cltPrxs->key=node->key;
                  cltPrxs->keyHash=node->keyHash;
                  return 1;
             }
             if(cltPrxs->next==NULL)
             {
                 cltPrxs->next=node;
                 return 1;
             }
             cltPrxs=cltPrxs->next;
        }
    }
        return 1;

}

int
getFromSock(int fd)
{
    int rc;
    unsigned int len;
    //struct sockaddr_in sain;
    //int s;
    //aaa=10;
    len = sizeof(fromSock);
    rc = getpeername(fd, (struct sockaddr*)&fromSock, &len);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't get peer name");
        return -1;
    }
    if(fromSock.sin_family == AF_INET) {
        return 1;
#ifdef HAVE_IPv6
    } else if(fromSock.sin_family == AF_INET6) {
        len = sizeof(fromSock6);
        rc = getpeername(fd, (struct sockaddr*)&fromSock6, &len);
        if(rc < 0) {
            do_log_error(L_ERROR, errno, "Couldn't get peer name");
            return -1;
        }
        if(fromSock6.sin6_family != AF_INET6) {
            do_log(L_ERROR, "Inconsistent peer name");
            return -1;
        }
        return 1;
#endif
    } else {
        do_log(L_ERROR, "Unknown address family %d\n", fromSock.sin_family);
        return -1;
    }
    return 0;
}

int updateParentProxy(char *parentAddr,char *parentKey,char *srvIP)  //add by humeng @2011.5.12
{
    ParentProxyPtr node,temp;
    AtomPtr allowIP;
    AtomListPtr tempList;
    node=malloc(sizeof(ParentProxyRec));
    node->next=NULL;
    node->addr=internAtom(parentAddr);
    node->key=internAtom(parentKey);
    node->keyHash=keyHasher(node->key->string, node->key->length);
    node->port=internAtom("8123");
    allowIP=internAtom(srvIP);
    tempList=findProxyClient(allowIP->string);
    if(tempList){
        node->allowIP=parseNetAddress(tempList);
    }
    if(ParentProxys==NULL)
    {
        ParentProxys=node;
        return 1;
    }else{
        temp=ParentProxys;     //add by humeng @ 2011.5.16
        node->next=ParentProxys->next;
        ParentProxys=node;
        free(temp);            //add by humeng @ 2011.5.16
    }
    return 1;
}

int delParentProxy(char *parentAddr)     //add by humeng @ 2011.5.17
{
     ParentProxyPtr prtPrxs,prev;
     prtPrxs=ParentProxys;
     prev=ParentProxys;
     if(ParentProxys==NULL)return(0);
     if(strcmp(prtPrxs->addr->string,parentAddr)==0){
           ParentProxys=prtPrxs->next;
           if(prtPrxs->allowIP)
               free(prtPrxs->allowIP);
           free(prtPrxs);
           prtPrxs=NULL;
           return(1);
     }
     prtPrxs=prtPrxs->next;
     while(prtPrxs){
          if(strcmp(prtPrxs->addr->string,parentAddr)==0){
               prev->next=prtPrxs->next;
               if(prtPrxs->allowIP)
                   free(prtPrxs->allowIP);
               free(prtPrxs);
               prtPrxs=NULL;
               return(1);
          }
          prev=prtPrxs;
          prtPrxs=prtPrxs->next;
     }
     return(0);
}

// int addParentProxy(char *parentAddr,char *parentKey,char *srvIP)     //add by humeng @ 2011.5.17
// {
     // ParentProxyPtr prtPrxs,node;
     // AtomPtr allowIP;
     // AtomListPtr tempList;
     // prtPrxs=ParentProxys;
     // allowIP=internAtom(srvIP);
     // tempList=findProxyClient(allowIP->string);
     // while(prtPrxs){
          // if(strcmp(prtPrxs->addr->string,parentAddr)==0){
               // prtPrxs->key=internAtom(parentKey);
               // prtPrxs->keyHash=keyHasher(prtPrxs->key->string, prtPrxs->key->length);
               // if(prtPrxs->allowIP){
                   // free(prtPrxs->allowIP);
                   // prtPrxs->allowIP=NULL;
               // }
               // if(tempList){
                   // prtPrxs->allowIP=parseNetAddress(tempList);
               // }
               // return(1);
          // }
          // if(prtPrxs->next==NULL)break;
          // prtPrxs=prtPrxs->next;
     // }
     // node=malloc(sizeof(ParentProxyRec));
     // node->next=NULL;
     // node->addr=internAtom(parentAddr);
     // node->key=internAtom(parentKey);
     // node->keyHash=keyHasher(node->key->string, node->key->length);
     // node->port=internAtom("8123");
     // if(tempList){
         // node->allowIP=parseNetAddress(tempList);
     // }
     // if(ParentProxys)
         // prtPrxs->next=node;
     // /*{
         // node->next=ParentProxys;
         // ParentProxys=node;
     // }*/
     // else
         // ParentProxys=node;
     // return(1);
// }

int addParentProxy(char *parentAddr,char *parentKey,char *srvIP,char *srvPort)     //modified by yangkun
{
     ParentProxyPtr prtPrxs,node;
     AtomPtr allowIP;
     AtomListPtr tempList;
     prtPrxs=ParentProxys;
     allowIP=internAtom(srvIP);
     tempList=findProxyClient(allowIP->string);
     while(prtPrxs){
          if(strcmp(prtPrxs->addr->string,parentAddr)==0){
               prtPrxs->key=internAtom(parentKey);
               prtPrxs->keyHash=keyHasher(prtPrxs->key->string, prtPrxs->key->length);
               if(prtPrxs->allowIP){
                   free(prtPrxs->allowIP);
                   prtPrxs->allowIP=NULL;
               }
               if(tempList){
                   prtPrxs->allowIP=parseNetAddress(tempList);
               }
               return(1);
          }
          if(prtPrxs->next==NULL)break;
          prtPrxs=prtPrxs->next;
     }
     node=malloc(sizeof(ParentProxyRec));
     node->next=NULL;
     node->addr=internAtom(parentAddr);
     node->key=internAtom(parentKey);
     node->keyHash=keyHasher(node->key->string, node->key->length);
     node->port=internAtom(srvPort);
     if(tempList){
         node->allowIP=parseNetAddress(tempList);
     }
     if(ParentProxys)
         prtPrxs->next=node;
     /*{
         node->next=ParentProxys;
         ParentProxys=node;
     }*/
     else
         ParentProxys=node;
     return(1);
}

int setCurrentParentProxy(char *parentAddr)        //add by humeng @ 2011.5.17
{
     ParentProxyPtr prtPrxs,prev;
     prtPrxs=ParentProxys;
     prev=ParentProxys;
     if(ParentProxys==NULL)return(0);
     if(strcmp(prtPrxs->addr->string,parentAddr)==0)
         return(1);
     prtPrxs=prtPrxs->next;
     while(prtPrxs){
          if(strcmp(prtPrxs->addr->string,parentAddr)==0){
               prev->next=prtPrxs->next;
               prtPrxs->next=ParentProxys;
               ParentProxys=prtPrxs;
               return(1);
          }
          prev=prtPrxs;
          prtPrxs=prtPrxs->next;
     }
     return(0);
}

int alternateParentProxys()        //add by humeng @ 2011.5.17
{
    ParentProxyPtr prtPrxs,node;
    prtPrxs=ParentProxys;
    node=ParentProxys;
    if(ParentProxys==NULL) return(0);
    if(ParentProxys->next==NULL) return(1);
    while(prtPrxs){
        if(prtPrxs->next==NULL)break;
        prtPrxs=prtPrxs->next;
    }
    ParentProxys=ParentProxys->next;
    node->next=prtPrxs->next;
    prtPrxs->next=node;
    return(1);
}
/* test thread  */
/*void Print_Thread(void* arg)
{
    while(1){
        pthread_mutex_lock(&mutex);
        printProxys();
        sleep(1);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return;
}

void Set_Thread(void* arg)
{
    while(1){
        pthread_mutex_lock(&mutex);
        setCurrentParentProxy("163.43.162.176");
        printf("Set Parent Proxy\n");
        sleep(1);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return;
}*/

int printProxys()        //add by humeng @ 2011.5.19 test
{
    ParentProxyPtr prtPrxs;
    ClientProxyPtr cltPrxs;
    cltPrxs=clientProxys;
    prtPrxs=ParentProxys;
    while(prtPrxs){
        printf("Parent Proxy:%s  key:%s\n",prtPrxs->addr->string,prtPrxs->key->string);
        prtPrxs=prtPrxs->next;
    }
    while(cltPrxs){
        printf("Client Proxy:%s  key:%s\n",cltPrxs->addr->string,cltPrxs->key->string);
        cltPrxs=cltPrxs->next;
    }
    return(1);
}

int eraseParentProxys()        //add by humeng @ 2011.5.17
{
    ParentProxyPtr prtPrxs,node;
    prtPrxs=ParentProxys;
    while(prtPrxs){
        node=prtPrxs;
        prtPrxs=prtPrxs->next;
        if(node->allowIP)
                   free(node->allowIP);
        free(node);
        node=NULL;
    }
    ParentProxys=NULL;
    return(1);
}

int delClientProxy(char *clientAddr)     //add by humeng @ 2011.5.16
{
     ClientProxyPtr cltPrxs,prev;
     cltPrxs=clientProxys;
     prev=clientProxys;
     if(clientProxys==NULL)return(0);
     if(strcmp(cltPrxs->addr->string,clientAddr)==0){
           clientProxys=cltPrxs->next;
           free(cltPrxs);
           cltPrxs=NULL;
           return(1);
     }
     cltPrxs=cltPrxs->next;
     while(cltPrxs){
          if(strcmp(cltPrxs->addr->string,clientAddr)==0){
               prev->next=cltPrxs->next;
               free(cltPrxs);
               cltPrxs=NULL;
               return(1);
          }
          prev=cltPrxs;
          cltPrxs=cltPrxs->next;
     }
     return(0);
}

int addClientProxy(char *clientAddr, char *clientKey)     //add by humeng @ 2011.5.16
{
     ClientProxyPtr cltPrxs,node;
     cltPrxs=clientProxys;
     while(cltPrxs){
          if(strcmp(cltPrxs->addr->string,clientAddr)==0){
               cltPrxs->key=internAtom(clientKey);
               cltPrxs->keyHash=keyHasher(cltPrxs->key->string, cltPrxs->key->length);
               return(1);
          }
          if(cltPrxs->next==NULL)break;
          cltPrxs=cltPrxs->next;
     }
    node=malloc(sizeof(ClientProxyRec));
    node->next=NULL;
    node->addr=internAtom(clientAddr);
    node->key=internAtom(clientKey);
    node->keyHash=keyHasher(node->key->string, node->key->length);
    if(clientProxys)
        cltPrxs->next=node;
    else
        clientProxys=node;
    return(1);
}

int eraseClientProxys()        //add by humeng @ 2011.5.16
{
    ClientProxyPtr node,cltPrxs;
    cltPrxs=clientProxys;
    while(cltPrxs){
        node=cltPrxs;
        cltPrxs=cltPrxs->next;
        free(node);
        node=NULL;
    }
    clientProxys=NULL;
    return(1);
}

int insertParentProxy(char *buf,int i)  //humeng add 11.5.10
{
    ParentProxyPtr node,prtPrxs;
    AtomPtr temp,addr,key,atom, port,allowIP;
    AtomListPtr tempList;
    int x,y,rc;
    if(!buf[i]=='=') return(0);
    i++;
    skipBlank(buf,i);
    x=i;
    while(!(buf[i] == '\n' || buf[i] == '\0' || buf[i] == '#'))
    {
        skipBlank(buf,i);
        i++;
    }
    if(i>x+1)
    temp=internAtomN(buf+x,i-x-1);
    rc = atomSplit(temp, ':', &addr, &temp);
    if(rc <= 0) {
        do_log(L_ERROR, "Couldn't parse parentProxy.");
        return -1;
    }
    node=malloc(sizeof(ParentProxyRec));
    node->addr=addr;
    rc = atomSplit(temp, ':', &port, &temp);
    if(rc <= 0) {
        do_log(L_ERROR, "Couldn't parse parentProxy.");
        return -1;
    }
    node->port=port;
    rc = atomSplit(temp, '-', &key, &allowIP);
    if(rc <= 0) {
        do_log(L_ERROR, "Couldn't parse parentProxy.");
        return -1;
    }
    node->key=key;
    node->keyHash=keyHasher(key->string, key->length);
    tempList=findProxyClient(allowIP->string);
    if(tempList){
        node->allowIP=parseNetAddress(tempList);
    }
    if(ParentProxys==NULL)
    {
        ParentProxys=node;
        //updateParentProxy("59.66.24.76","123","166.111.132.138");
        return 1;
    }else
    {
        prtPrxs=ParentProxys;
        while(prtPrxs)
        {
             if(strcmp(prtPrxs->addr->string,node->addr->string)==0)
             {
                  prtPrxs->port=node->port;
                  prtPrxs->key=node->key;
                  prtPrxs->keyHash=node->keyHash;
                  prtPrxs->allowIP=node->allowIP;
                  return 1;
             }
             if(prtPrxs->next==NULL)
             {
                 prtPrxs->next=node;
                 return 1;
             }
             prtPrxs=prtPrxs->next;
        }
    }
        return 1;

}

int getUsername(char *buf,int i)  //humeng add 11.5.10
{
    AtomPtr temp;
    int x;
    if(!buf[i]=='=') return -1;
    i++;
    x=skipBlank(buf,i);
    while(!(buf[i] == '\n' || buf[i] == '\0' || buf[i] == '#'))
    {
        skipBlank(buf,i);
        i++;
    }
    if(i>x+1)
    {
        temp = internAtomN(buf+x,i-x-1);
        username = temp->string;        
    }
    
    return 0;
}

int getPassword(char *buf,int i)  //humeng add 11.5.10
{
    AtomPtr temp;
    int x;
    if(!buf[i]=='=') return -1;
    i++;
    x=skipBlank(buf,i);
    while(!(buf[i] == '\n' || buf[i] == '\0' || buf[i] == '#'))
    {
        skipBlank(buf,i);
        i++;
    }
    if(i>x+1)
    {
        temp = internAtomN(buf+x,i-x-1);
        password = temp->string;
    }
    return 0;
}

/*void
insertParentProxy(ConfigVariablePtr var,AtomPcharKey=(unsigned char*)key;tr av)          //add
{
    AtomPtr atom;
    atom=*var->value.a;
    while(1){
       //if(!atom->next)break;
       if(strcmp(atom->string,av->string)==0)return;
       if(!atom->next)break;
       atom=atom->next;
    }
    av->next=atom->next;
    atom->next=av;
    //av->next=*var->value.a;
    //*var->value.a=av;
    return;
}*/
static
int
parseAtom1(char *buf, int offset, AtomPtr *value_return, int insensitive)
{
    int y0, i, j, k;
    AtomPtr atom;
    int escape = 0;
    char *s;

    i = offset;
    if(buf[i] != '\0') {
        y0 = i;
        i++;
        while(buf[i] != '\"' && buf[i] != '\n' && buf[i] != '\0' && buf[i]!=',') {
            if(buf[i] == '\\' && buf[i + 1] != '\0') {
                escape = 1;
                i += 2;
            } else
                i++;
        }
        //if(buf[i] != '\0')
            //return -1;
        j = i ;
    } else {
        y0 = i;
        while(letter(buf[i]) || digit(buf[i]) || 
              buf[i] == '_' || buf[i] == '-' || buf[i] == '~' ||
              buf[i] == '.' || buf[i] == ':' || buf[i] == '/')
            i++;
        j = i;
    }

    if(escape) {
        s = malloc(i - y0);
        if(buf == NULL) return -1;
        k = 0;
        j = y0;
        while(j < i) {
            if(buf[j] == '\\' && j <= i - 2) {
                s[k++] = buf[j + 1];
                j += 2;
            } else
                s[k++] = buf[j++];
        }
        if(insensitive)
            atom = internAtomLowerN(s, k);
        else
            atom = internAtomN(s, k);
        free(s);
        j++;
    } else {
        if(insensitive)
            atom = internAtomLowerN(buf + y0, i - y0);
        else
            atom = internAtomN(buf + y0, i - y0);
    }
    *value_return = atom;
    return j;
}

AtomListPtr
findProxyClient(char *line)
{
    AtomListPtr alv;
    AtomPtr name, value;
    int i=0;
    alv = makeAtomList(NULL, 0);
    if(alv == NULL) {
        do_log(L_ERROR, "couldn't allocate atom list.\n");
        return NULL;
    }
    while(1) {
        i = parseAtom1(line, i, &value,1);
        //if(i < 0) goto syntax;
        if(!value) {
            do_log(L_ERROR, "couldn't allocate atom.\n");
            return NULL;
        }
        //printf("%s\n",value->string);
        atomListCons(value, alv);
        i = skipWhitespace(line, i);
        if(line[i] == '\n' || line[i] == '\0' || line[i] == '#')
            break;
        if(line[i] != ',') {
            destroyAtomList(alv);
            //goto syntax;
        }
        i = skipWhitespace(line, i + 1);
    }
    return alv;
}

int
clientMatch(NetAddressPtr allowIP)
{
    int rc;
    unsigned int len;
    struct sockaddr_in sain;
#ifdef HAVE_IPv6
    struct sockaddr_in6 sain6;
#endif

    sain=fromSock;

    if(sain.sin_family == AF_INET) {
        return match(4, (unsigned char*)&sain.sin_addr, allowIP);
#ifdef HAVE_IPv6
    } else if(sain.sin_family == AF_INET6) {
        len = sizeof(sain6);
        sain6=fromSock6;
        if(sain6.sin6_family != AF_INET6) {
            do_log(L_ERROR, "Inconsistent peer name");
            return -1;
        }
        return match(6, (unsigned char*)&sain6.sin6_addr, clientRange);
#endif
    } else {
        do_log(L_ERROR, "Unknown address family %d\n", sain.sin_family);
        return -1;
    }
    return 0;
}


int
findParentProxy()                //modified by humeng @ 2011.5.11
{
    AtomPtr atom,host, port_atom,temp;
    ParentProxyPtr node,prtPrxs;
    NetAddressPtr allowIP;
    //AtomListPtr tempList;
    int port;
    prtPrxs=ParentProxys;
    //aaa=10;
    while(prtPrxs){
        /*rc = atomSplit(atom, ':', &host, &temp);
        if(rc <= 0) {
            do_log(L_ERROR, "Couldn't parse parentProxy.");
            return -1;
        }clientMatch
        rc = atomSplit(temp, '-', &port_atom, &allowIP);
        clientRange=NULL;*/
        allowIP=prtPrxs->allowIP;
        //if(rc > 0) {
            //tempList=findProxyClient(allowIP->string);
            //printf("%s\n",tempList->list[0]->string);
            //printf("%s\n",tempList->list[1]->string);
            //printf("%s\n",tempList->list[2]->string);
            //if(tempList){
                //clientRange=parseNetAddress(tempList);
        if(clientMatch(allowIP)<=0){
             prtPrxs=prtPrxs->next;
             continue;
        }
        //}
        //if(clientRange){tempList
        
        //}

        port = atoi(prtPrxs->port->string);
        if(port <= 0 || port >= 0x10000) {
            //releaseAtom(host);
            //releaseAtom(port_atom);
            do_log(L_ERROR, "Couldn't find parentProxy's port.");
            return -1;
        }
        parentHost = prtPrxs->addr;
        parentPort = port;
        PrtPrxKey=prtPrxs->keyHash;
        return 1;
        //}
        //atom=atom->next;
    }
    return -1;

}
void initNeighbor()
{//added by lidheu
  int i;
  for(i=0; i<1024*8; i++)
  {
     neighbor[i].isSocialProxy=0;
     neighbor[i].key=0;
  }
}
void checkAcceptClientProxy(struct sockaddr_in addr, int fd)
{//added by lidehu
	//char *childProxy="59.66.24.81";
    ClientProxyPtr temp=clientProxys;
    unsigned long temp_addr;
    neighbor[fd].key=0;
     while(temp){
       temp_addr=inet_addr(temp->addr->string);
   	if(temp_addr==addr.sin_addr.s_addr)
    	{   neighbor[fd].isSocialProxy=1;
            neighbor[fd].key=temp->keyHash;
	    break;
         	}
         temp=temp->next;
	}
 	if(temp==NULL) neighbor[fd].isSocialProxy=0;
        // printf("do_scheduled_accpt:%s neighbor[%d].isSocialProxy=%d\n",inet_ntoa(addr.sin_addr), fd, neighbor[fd].isSocialProxy);
        return;
}
void findKeyAlg(int fd)//added by lidehu
{}
int encryData(char *buf, int n, void *key , int keylen, char *encrybuf, int *len)
{   //added by lidehu/*void
    int i=0;
    unsigned int intKey=((*(unsigned char*)key)<<24)|((*(unsigned char*)key)<<16)|((*(unsigned char*)key)<<8)|((*(unsigned char*)key));
    
    unsigned char *charKey;
    if(!encrybuf) return -1;
    while(i+4<=n)
    {
       (*(unsigned int*)encrybuf)=(*(unsigned int*)buf)^intKey;
       i+=4;
       encrybuf+=4;
       buf+=4;
    }
    encrybuf-=i;buf-=i;
    if(i<n)
    {   charKey=(unsigned char*)(&intKey);
        for(i=i; i<n; i++)
        {
          encrybuf[i]=charKey[3+i-n]^buf[i];
        }
    }
    *len=n;
    return n; 
}

int decryData(char *buf, int n, void *key ,int keylen,  int *len)
{  //added by lidehu
    int i=0;
     unsigned int intKey=((*(unsigned char*)key)<<24)|((*(unsigned char*)key)<<16)|((*(unsigned char*)key)<<8)|((*(unsigned char*)key));
    unsigned char *charKey;
    if(!buf) return -1;
    while(i+4<=n)
    {
        (*(unsigned int*)buf)^=intKey;
        buf+=4;    
        i+=4;
    }
    buf-=i;
   if(i<n)
   {    charKey=(unsigned char*)(&intKey);
   	for(i=i; i<n; i++)
    	{    
            buf[i]=buf[i]^charKey[3+i-n];
        }
    }
    *len=n;
    return n;
} 

//int encryData(char *buf, int n, void *key , int keylen, char *encrtbuf, int *len); //added by lidehu
int social_WRITE(int fd, char *buf, int n)//added by lidheu
{   //added by lidehu
    int rc;
    char *encrybuf=malloc(n);
    if(encrybuf==NULL) printf("buf malloc error\n");
    int len=0;
    encryData(buf, n, &(neighbor[fd].key), 1, encrybuf, &len);
    // printf("++++++++++++++++++++++++++writing key: %X\n", neighbor[fd].key);
    rc = WRITE(fd, encrybuf, n);
    //added by lidehu
    if(rc != n) {
        return -1;
    }
    free(encrybuf);
    return rc;
}

int social_WRITEV(int fd, const struct iovec *vector, int count)//added by lidehu
{
    int rc;                    /* Return Code */
        int n = 0;              /* Total bytes to write */
        char *buf = 0;          /* Buffer to copy to before writing */
        int i;                  /* Counter var for looping over vector[] */
        int offset = 0;        /* Offset for copying to buf */

        /* Figure out the required buffer size */
        for(i = 0; i < count; i++) {
            n += vector[i].iov_len;
        }

        /* Allocate the buffer. If the allocation fails, bail out */
        buf = malloc(n);
        if(!buf) {
            errno = ENOMEM;
            return -1;
        }

        /* Copy the contents of the vector array to the buffer */
        for(i = 0; i < count; i++) {
            memcpy(&buf[offset], vector[i].iov_base, vector[i].iov_len);
            offset += vector[i].iov_len;
        }
        assert(offset == n);

        /* Write the entire buffer to the socket and free the allocation */
        rc = social_WRITE(fd, buf, n);
        free(buf);
    return rc;
}
//int decryData(char *buf, int n, void *key ,int keylen, char *decrybuf, int *len); //added by lidehu
int social_READ(int fd, char *buf, int n)//added by lidehu
{   //added by lidehu
    int len=0;
    int rc=READ(fd, buf, n);
    decryData(buf, rc, &(neighbor[fd].key), 1, &len);
    // printf("++++++++++++++++++++++++++++++reading key: %X\n", neighbor[fd].key);
    //added by lidehu
    return rc;
}
int social_READV(int fd, const struct iovec *vector, int count)//added by lidehu
{
    int ret = 0;                     /* Return value */
    int i;
    for(i = 0; i < count; i++) {
        int n = vector[i].iov_len;
        int rc = social_READ(fd, vector[i].iov_base, n);
        if(rc == n) {
            ret += rc;
        } else {
            if(rc < 0) {
                ret = (ret == 0 ? rc : ret);
            } else {
                ret += rc;
            }
            break;
        }
    }
    return ret;
}
/*int social_READV(int fd, const struct iovec *vector, int count)//added by lidehu
{
    int rc = 0, len=0;
    int n=0;                   
    int i;
    int templen[count];
    char *buf = 0;          
    int offset = 0;       
    for(i = 0; i < count; i++) 
    {
        rc=READ(fd, vector[i].iov_base, vector[i].iov_len);
        templen[i]=rc;
        if(rc==vector[i].iov_len)
	{
          n+=rc; }
        else{
             if(rc<0){ n=(n==0?rc : n); }
             else{
                   n+=rc; }
             }
       printf("in social_readv:vector[%d].iov_len=%d, rc=%d, templen[%d]=%d\n",i, vector[i].iov_len, rc, i, rc);
     }  
   
    printf("in social_readv: n=%d\n", n);
    buf = malloc(n);
    if(!buf) {
    	errno = ENOMEM;
    	return -1;
    }
    for(i = 0; i < count; i++) {
        if(templen[i]>0)
       {
        memcpy(&buf[offset], vector[i].iov_base, templen[i]);
        offset += templen[i]; }
       else break;
    }
    assert(offset == n);
   
    decryData(buf, n, &(neighbor[fd].key), 4, &len);
    offset=0;
    for(i = 0; i < count; i++) {
        if(templen[i]>0)
	{
        memcpy(vector[i].iov_base, &buf[offset], templen[i]);
        offset += templen[i]; }
        else break;
    }
    assert(offset == n);
    free(buf);
    return n;
}*/
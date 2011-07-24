/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

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

#include "polipo.h"
AtomPtr configFile = NULL;
AtomPtr pidFile = NULL;
int daemonise = 0;
// global variables added by Yangkun

const char *username = NULL;
const char *password = NULL;
const char *ip_address = NULL;
const char *network_interface = NULL;
int wel_msg = 1;

const char * social_error_message[10];
FriendListRec friendListRec;
FriendListPtr friendList;

struct sockaddr_in fromSock;       //add
struct sockaddr_in6 fromSock6;     //add
int isSocialProxy;
unsigned char PrtPrxKey;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;    //add by humeng @ 2011.5.19 Thread lock

CURL *curl;
char *reply;

int debug_mode;
//time_t last_heartbeat_time;

static void
usage(char *argv0)
{
    fprintf(stderr, 
            "%s [ -h ] [ -v ] [ -x ] [ -c filename ] [ -- ] [ var=val... ]\n",
            argv0);
    fprintf(stderr, "  -h: display this message.\n");
    fprintf(stderr, "  -v: display the list of configuration variables.\n");
    fprintf(stderr, "  -x: perform expiry on the disk cache.\n");
    fprintf(stderr, "  -c: specify the configuration file to use.\n");
    //fprintf(stderr, "  -i: interface.\n");
    //fprintf(stderr, "  -u: username.\n");
    //fprintf(stderr, "  -p: password.\n");
    
}

unsigned int addr_to_unsigned(unsigned int in)
{
    unsigned int out = 0;
    int i = 0;
    for(i = 0; i < 4; i++) 
    { 
      out <<= 8;
      out |= in & 0x000000FF;
      in >>= 8;
    }    
    return out;
}


int
main(int argc, char **argv)
{

    
    
    // added by yangkun
    social_error_message[0] = "No error";
    social_error_message[1] = "Failed to parse HTTP reply";
    social_error_message[2] = "Failed to allocate memory";
    social_error_message[3] = "Reply format error";
    social_error_message[4] = "Database error";
    social_error_message[5] = "Password error";
    social_error_message[6] = "Invalid parameters";
    social_error_message[7] = "User not exists";
    social_error_message[8] = "IP address error";
    social_error_message[9] = "No error";

    debug_mode = 0;
    
    friendList = &friendListRec;
    
    // initialize friend list
    friendList->list = NULL;
    friendList->size = 0;
    
    const char *address = "http://social.sec.ccert.edu.cn/heartbeat.php";

    // initialize thread variables
    pthread_t tid;
    int rc = 0;

    FdEventHandlerPtr listener;
    int i;
    int rc0;
    int expire = 0, printConfig = 0;
    
    initAtoms();
    CONFIG_VARIABLE(daemonise, CONFIG_BOOLEAN, "Run as a daemon");
    CONFIG_VARIABLE(pidFile, CONFIG_ATOM, "File with pid of running daemon.");

    preinitChunks();
    preinitLog();
    preinitObject();
    preinitIo();
    preinitDns();
    preinitServer();
    preinitHttp();
    preinitDiskcache();
    preinitLocal();
    preinitForbidden();
    preinitSocks();

    i = 1;
    while(i < argc) {
        if(argv[i][0] != '-')
            break;
        if(strcmp(argv[i], "--") == 0) {
            i++;
            break;
        } else if(strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            exit(0);
        } else if(strcmp(argv[i], "-v") == 0) {
            printConfig = 1;
            i++;
        } else if(strcmp(argv[i], "-x") == 0) {
            expire = 1;
            i++;
        } else if(strcmp(argv[i], "-c") == 0) {
            i++;
            if(i >= argc) {
                usage(argv[0]);
                exit(1);
            }
            if(configFile)
                releaseAtom(configFile);
            configFile = internAtom(argv[i]);
            i++;
        } else if(strcmp(argv[i], "-d") == 0) {
            debug_mode = 1;
            i++;
        // } else if(strcmp(argv[i], "-u") == 0) {
            // i++;
            // if(i >= argc) {
                // usage(argv[0]);
                // exit(1);
            // }
            // username = argv[i];
            // i++;
        // } else if(strcmp(argv[i], "-p") == 0) {
            // i++;
            // if(i >= argc) {
                // usage(argv[0]);
                // exit(1);
            // }
            // password = argv[i];
            // i++;
        } else if(strcmp(argv[i], "-i") == 0) {
            i++;
            if(i >= argc) {
                usage(argv[0]);
                exit(1);
            }
            network_interface = argv[i];
            i++;
        } else {
            usage(argv[0]);
            exit(1);
        }
    }
    
   
    // get ip address for posix env
    if (network_interface && strcmp(network_interface, "lo") == 0)
    {
      usage(argv[0]);
      exit(1);
    }
    
    
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
	if (ifa->ifa->addr==NULL) continue;
        if (ifa ->ifa_addr->sa_family==AF_INET) {
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (network_interface)
            {
              if (strcmp(network_interface, ifa->ifa_name)==0)
              {
                ip_address = addressBuffer;
                
                
                // check ip address
                
                
                
                break;
              }
            }
            else
            {
            
            
              if (strcmp("lo", ifa->ifa_name) && strcmp("lo0", ifa->ifa_name))
              {
                ip_address = addressBuffer;
                network_interface = ifa->ifa_name;
                break;
              }
            }
             
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    
    
    
    // get ipaddress for win32  
    // char host_name[255];

    // if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR)
    // {
      // printf("Error %d when getting local host name.\n", WSAGetLastError());
      // exit(1);
    // }
    // struct hostent *phe = gethostbyname(host_name);
    // if (phe == 0)
    // {
      // printf("Bad host lookup.\n");
      // exit(1);
    // }

    // int j = 0;
    // for (; phe->h_addr_list[j] != 0; ++j)
    // {
      // struct in_addr addr;
      // memcpy(&addr, phe->h_addr_list[j], sizeof(struct in_addr));
      // unsigned int addr_unsigned = addr_to_unsigned(addr.s_addr);
      
      // if ((addr_unsigned >= 0x0A000000 && addr_unsigned <= 0x0AFFFFFF ) ||
         // (addr_unsigned >= 0xAC100000 && addr_unsigned <= 0xAC1FFFFF ) ||
         // (addr_unsigned >= 0xC0A80000 && addr_unsigned <= 0xC0A8FFFF )
         // )
      // {
        // continue;
      // }
      // if (strcmp(inet_ntoa(addr), "127.0.0.1"))
      // {
        // ip_address = inet_ntoa(addr);
        // break;
      // }
    // }
    
    if (ip_address == NULL)
    {
      printf("Public address not found.\n");
      exit(1);
    }
    
    if(configFile)
        configFile = expandTilde(configFile);

    if(configFile == NULL){
        configFile = expandTilde(internAtom("~/.polipo"));
        if(configFile)
            if(access(configFile->string, F_OK) < 0) {
                releaseAtom(configFile);
                configFile = NULL;
            }
    }

    if(configFile == NULL) {
        if(access("./config", F_OK) >= 0)
            configFile = internAtom("./config");
        if(configFile && access(configFile->string, F_OK) < 0) {
            releaseAtom(configFile);
            configFile = NULL;
        }
    }    
    
    rc = parseConfigFile(configFile);
    if(rc < 0)
        exit(1);

    while(i < argc) {
        rc = parseConfigLine(argv[i], "command line", 0, 0);
        if(rc < 0)
            exit(1);
        i++;
    }

    if (username == NULL || password == NULL)
    {
      printf("Please specify username&password in config file.\n");
      usage(argv[0]);
      exit(1);
    }

    printf("Your username: %s\n", username);
    // printf("password: %s\n", password);
    // printf("interface: %s\n", network_interface);
    printf("Your ip address: %s\n", ip_address);
     
    char post_form[100];
    char port_str[10];
    sprintf(port_str, "%d", proxyPort);
    
    post_form[0] = '\0';
    strcat(post_form, "username=");
    strcat(post_form, username);
    strcat(post_form, "&password=");
    strcat(post_form, password);
    strcat(post_form, "&ip_address=");
    strcat(post_form, ip_address);
    strcat(post_form, ":");
    strcat(post_form, port_str);
    
    //printf("post form = %s\n", post_form);
    
    
    
    // initialize libxml
    LIBXML_TEST_VERSION
    
    CURLcode curl_res;
    curl = curl_easy_init();
    
    if (curl == NULL)
    {
      printf("Failed to initialize curl.\n");
      free(reply);
      exit(1);
    }
    
    curl_res = curl_easy_setopt(curl, CURLOPT_URL, address);
    curl_res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_form);   
    curl_res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &process_data);
    curl_res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reply);
    
    
    initChunks();
    initLog();
    initObject();
    if(!expire && !printConfig)
        initEvents();
    initIo();
    initDns();
    initHttp();
    initServer();
    initDiskcache();
    initForbidden();
    initSocks();
    initNeighbor();//added by lidehu
    if(printConfig) {
        printConfigVariables(stdout, 0);
        exit(0);
    }

    if(expire) {
        expireDiskObjects();
        exit(0);
    }

    if(daemonise)
        do_daemonise(loggingToStderr());

    if(pidFile)
        writePid(pidFile->string);

    listener = create_listener(proxyAddress->string, 
                               proxyPort, httpAccept, NULL);
    if(!listener) {
        if(pidFile) unlink(pidFile->string);
        exit(1);
    }
    
    rc0 = pthread_create(&tid, NULL, heartbeat_thread, NULL);
    
    //addClientProxy("166.111.132.226", "AAAAAAAAA");
    //addParentProxy("166.111.132.226", "AAAAAAAAA", "127.0.0.1");
    eventLoop();

    if(pidFile) unlink(pidFile->string);
    return 0;
}

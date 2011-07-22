typedef struct _IPAddress {
    char *ip_address;
    struct _IPAddress *next;
} IPAddressRec, *IPAddressPtr;

typedef struct _Friend {
    int ip_list_size;
    char *username;
    IPAddressRec *ip_list;
    char *session_key;
    struct _Friend *next;
} FriendRec, *FriendPtr;

typedef struct _FriendList {
    int size;
    FriendRec *list;
} FriendListRec, *FriendListPtr;

void heartbeat_thread(void* arg);
size_t process_data(char *buffer, size_t size, size_t nmemb, char **replyp);
void print_friend_list();
void free_friend_list();
int process_heartbeat_reply(char *reply);



// added by Yangkun
extern const char *username;
extern const char *password;
extern const char *network_interface;
extern const char *ip_address;
extern const char * social_error_message[10];
extern FriendListRec friendListRec;
extern FriendListPtr friendList;
extern CURL *curl;
extern char *reply;
extern int wel_msg;


/*********add by elisen @ 2011.5.17 ******/
void printSocialList(FILE *out,char *mtubuf);
void printSocialVariables(FILE *out, int html);
/*********add by elisen @ 2011.5.17 ******/

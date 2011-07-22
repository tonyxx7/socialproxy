#include "polipo.h"
//
// heartbeat thread function
void heartbeat_thread(void* arg)
{   
    while(1)
    {
        //if (debug_mode) printProxys();       
        free_friend_list();
        
        reply = malloc(1);
        if (reply == NULL)
        {
          printf("Failed to allocate memory.\n");
          exit(1);
        }
        reply[0] = '\0';

        CURLcode curl_res = curl_easy_perform(curl);
        if (curl_res)
        {         
          free(reply);
          curl_easy_cleanup(curl);
          printf("HTTP request failed.\n");
          exit(1);
        } 
        // analyse reply
        int xml_res = process_heartbeat_reply(reply);
        if (xml_res)
        {         
          free(reply);
          curl_easy_cleanup(curl);
          printf("%s.\n", social_error_message[xml_res]);
          exit(1);
        }
        free(reply);
        update_proxys();
        if (wel_msg)
        {
          printf("-------- Login successfully! --------\n");
          wel_msg = 0;
        }
        if (debug_mode) print_friend_list();
        // if (debug_mode) print_friend_list();
        
        sleep(60);
    }
    return;
}


size_t process_data(char *buffer, size_t size, size_t nmemb, char **replyp)
{
  char *reply = *replyp;
  
  size_t extended_size = size * nmemb;
  size_t original_size = strlen(reply); 
  reply = realloc(reply, original_size + extended_size + 1);
  strncpy(reply + original_size, buffer, extended_size);
  reply[original_size + extended_size] = '\0';
  *replyp = reply;
  
  return extended_size;
}

void print_friend_list()
{
  if (friendList->list)
  {
    // print friend list
    printf("****************************************\n");
    FriendPtr friendIterator = friendList->list;
    while(friendIterator)
    {   
      if (friendIterator->username)
      {
        printf("username: %s\n", friendIterator->username);
      }
      if (friendIterator->ip_list)
      {
        printf("ip_list_size: %d\n", friendIterator->ip_list_size);
        IPAddressPtr ipAddressIterator = friendIterator->ip_list;
        
        while (ipAddressIterator)
        {
            printf("ip_address: %s\n", ipAddressIterator->ip_address);
            ipAddressIterator = ipAddressIterator->next;
        }
      }
      if (friendIterator->session_key)
      {
        printf("session_key: %s\n", friendIterator->session_key);
      }
      friendIterator = friendIterator->next;
    }
  }
  else
  {
    printf("Current friend list is empty.\n");
  }
}

void free_friend_list()
{
  if (friendList->list)
  {
    FriendPtr friendIterator = friendList->list;
    while(friendIterator)
    {   
      free(friendIterator->username);

      if (friendIterator->ip_list)
      {
        IPAddressPtr ipAddressIterator = friendIterator->ip_list;
        
        while (ipAddressIterator)
        {
          free(ipAddressIterator->ip_address);
          IPAddressPtr ipAddressTmp = ipAddressIterator;
          ipAddressIterator = ipAddressIterator->next;
          free(ipAddressTmp);
        }
      }
      free(friendIterator->session_key);
      FriendPtr friendTmp = friendIterator;
      friendIterator = friendIterator->next;
      free(friendTmp);
    }
  }
  friendList->list = NULL;
  friendList->size = 0;
}

void update_proxys()
{
  // save current proxy ip address
  char address_tmp[16];
  address_tmp[0] = '\0';
  if (ParentProxys)
  {
    strcpy(address_tmp, ParentProxys->addr->string);
    address_tmp[strlen(ParentProxys->addr->string)] = '\0';
  }
   
  pthread_mutex_lock(&mutex);  
  eraseParentProxys();
  eraseClientProxys();
  
  if (friendList->list)
  {
    FriendPtr friendIterator = friendList->list;
    while(friendIterator)
    {   
      if (friendIterator->ip_list)
      {
        IPAddressPtr ipAddressIterator = friendIterator->ip_list;       
        while (ipAddressIterator)
        {
          addClientProxy(ipAddressIterator->ip_address, friendIterator->session_key);
          addParentProxy(ipAddressIterator->ip_address, friendIterator->session_key, "127.0.0.1");
          ipAddressIterator = ipAddressIterator->next;
        }
      }
      friendIterator = friendIterator->next;
    }
  }
  if (strlen(address_tmp) > 0)
  {
    setCurrentParentProxy(address_tmp);
  }
  pthread_mutex_unlock(&mutex);

  
}



// function: use libxml to analyse HTTP reply
// error code:
// 0  OK.
// 1  Failed to parse HTTP reply.
// 2  Failed to allocate memory.
// 3  Reply format error.
// 4  Database error.
// 5  Password error.
// 6  Invalid parameters.
// 7  User not exists.
// 8  IP address error.

int process_heartbeat_reply(char *reply)
{
  xmlDocPtr doc = xmlReadMemory(reply, strlen(reply), "reply.xml", NULL, 0);
  if (doc == NULL)
  {
    xmlFreeDoc(doc);
    // printf("Failed to parse HTTP reply.\n");
    return 1;
  }
  
  xmlNodePtr reply_xml = xmlDocGetRootElement(doc);
  if (reply_xml == NULL)
  {
    xmlFreeDoc(doc);
    // printf("Failed to parse HTTP reply.\n");
    return 1;
  }
  xmlNodePtr friend = reply_xml->xmlChildrenNode;
  
  while (friend != NULL) 
  {
    if (xmlStrcmp(friend->name, (const xmlChar *)"friend"))
    {
      if (!xmlStrcmp(friend->name, (const xmlChar *)"error"))
      {
        // parse error code
        char* error_code_char = (char *)xmlNodeListGetString(doc, friend->xmlChildrenNode, 1);
        int error_code = atoi(error_code_char);
        
        
        if (error_code > 3 && error_code < 10)
        {
          xmlFreeDoc(doc);
          return error_code;
        }
        else
        {
          xmlFreeDoc(doc);          
          // printf("Reply format error.\n");
          return 3;  
        }
      }
      
      xmlFreeDoc(doc);
      // printf("Reply format error.\n");
      return 3;
    }
    // parse friend xml
    // initialize friend node
    FriendPtr newFriend = malloc(sizeof(FriendRec));
    if (newFriend == NULL)
    {
      xmlFreeDoc(doc);
      // printf("Failed to allocate memory.\n");
      return 2;
    }
    newFriend->ip_list = NULL;
    newFriend->next = NULL;
    newFriend->session_key = NULL;
    newFriend->username = NULL;
    newFriend->ip_list_size = 0;
                         
    xmlNodePtr friend_attr = friend->xmlChildrenNode;
    
    while (friend_attr != NULL)
    {
      // parse friend attributes
      if (!xmlStrcmp(friend_attr->name, (const xmlChar *)"username"))
      {
        newFriend->username = (char *)xmlNodeListGetString(doc, friend_attr->xmlChildrenNode, 1);
        // printf("username: %s\n", newFriend->username);
        friend_attr = friend_attr->next;
        continue;
      }
      
      if (!xmlStrcmp(friend_attr->name, (const xmlChar *)"ip_address"))
      {
        IPAddressPtr newIPAddress = malloc(sizeof(IPAddressRec));       
        if (newIPAddress == NULL)
        {
          free(newFriend);
          xmlFreeDoc(doc);
          // printf("Failed to allocate memory.\n");
          return 2;
        }
        newIPAddress->ip_address = (char *)xmlNodeListGetString(doc, friend_attr->xmlChildrenNode, 1);
        // printf("ip_address: %s\n", newIPAddress->ip_address);
                                    
        if (newFriend->ip_list)
        {
          newIPAddress->next = newFriend->ip_list;
        }
        else
        {
          newIPAddress->next = NULL;
        }
        newFriend->ip_list = newIPAddress;
        newFriend->ip_list_size++;
                                    
        friend_attr = friend_attr->next;
        continue;
      }
      
      if (!xmlStrcmp(friend_attr->name, (const xmlChar *)"session_key"))
      {
        newFriend->session_key = (char *)xmlNodeListGetString(doc, friend_attr->xmlChildrenNode, 1);
        // printf("session_key: %s\n", newFriend->session_key);

        friend_attr = friend_attr->next;
        continue;
      }
      
    }
    
    if (newFriend->username && newFriend->ip_list && newFriend->session_key)
    {
        // insert new friend
        if (friendList->list)
        {
            newFriend->next = friendList->list;
        }
        friendList->list = newFriend;
    }
    else
    {
      free(newFriend->username);
      
      IPAddressPtr ipAddressIterator = newFriend->ip_list;        
      while (ipAddressIterator)
      {
        free(ipAddressIterator->ip_address);
        IPAddressPtr ipAddressTmp = ipAddressIterator;
        ipAddressIterator = ipAddressIterator->next;
        free(ipAddressTmp);
      }        
      
      free(newFriend->session_key);
      free(newFriend);
      xmlFreeDoc(doc);
      // printf("Reply format error.\n");
      return 3;
    }

    friend = friend->next;
  }
  
  return 0;
}

// /*********add by elisen @ 2011.5.17 ******/
// void printSocialList(FILE *out,char *mtubuf)
// {
    // fprintf(out,
            // "<!DOCTYPE HTML PUBLIC "
            // "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
            // "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
            // "<html><head>\n"
            // "<title>Polipo Socialproxy</title>\n"
            // "</head><body>\n"
            // "<h1>Polipo Socialproxy</h1>\n");
    // printSocialVariables(out, 1);
    // fprintf(out, "<p><a href=\"/polipo/\">back</a></p>");
    // fprintf(out, "</body></html>\n");
// }
// void printSocialVariables(FILE *out, int html)
// {
   // int entryno = 0;
   // if(html) {
        // fprintf(out, "<table>\n");
        // fprintf(out, "<tbody>\n");
    // }

    // if(html) {
        // alternatingHttpStyle(out, "friendlist");
        // fprintf(out,
                // "<table id=friendlist>\n"
                // "<thead>\n"
                // "<tr>" 
                // "<th>No  </th>"
                // "<th>Ip  </th>"
                // "<th>Name  </th>"
                // "<th>Status  </th>"
                // //"<th>SetValue  </th>"
                // "<th>Description  </th>\n"
                // "</thead><tbody>\n");
    // }

    // ParentProxyPtr pvar,prev;
    // pvar=ParentProxys;

    // if(NULL == pvar)
       // return;
    // //while(1){
    // while(pvar != NULL) {
      // if(html) {
          // if(entryno % 2)
              // fprintf(out, "<tr class=odd>");
          // else
              // fprintf(out, "<tr class=even>");
          // fprintf(out, "<td>");
      // } 
      // entryno++;
      // //fprintf(out,out, "%s", pvar->name->string);
      // fprintf(out, "%d</td>", entryno);
      // fprintf(out, "<td>%s</td>",pvar->addr->string);
      // fprintf(out, "<td>%s</td>","    ");
      // if(1 == entryno)
         // fprintf(out, "<td>%s</td>", "active   ");
      // else
         // fprintf(out, "<td>%s</td>", "online   ");
      // //fprintf(out, "<td>");
      // //fprintf(out, "<form method=POST action=\"social?\">");
      // //fprintf(out, "<input type=\"submit\" value=\"%s\">\n",pvar->addr->string);
      // //fprintf(out, "</form></td>");
      // fprintf(out, "<td>%s","");
      // if(html)
	// fprintf(out, "</td></tr>\n");
      // else
	// fprintf(out, "\n");

      // //if(entryno >= 20) break;
      // pvar = pvar->next;
    // }
    // if(html) {
        // fprintf(out, "</tbody>\n");
        // fprintf(out, "</table>\n");
    // }

    // fprintf(out, "<p><form method=POST action=\"social?\">");
    // fprintf(out, "<select name=newparent >");
    // pvar=ParentProxys;
    // if(NULL == pvar)
       // return;
    // while(pvar != NULL) {
       // fprintf(out, "<option>%s</option>",pvar->addr->string);
       // pvar = pvar->next;
    // }

    // fprintf(out, "<input type=\"submit\" value=\"select\">\n");
    // fprintf(out, "</form></p>\n");
// }
// /*********add by elisen @ 2011.5.17 ******/

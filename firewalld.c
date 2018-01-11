#define LOG_TAG "firewalld"
#include <sys/socket.h>
//#include <cutils/sockets.h>
#include "../include/utils/log.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define SOCKET_PATH "firewalld"
#define BUFFER_MAX 1024
#define IPTABLES "/system/bin/iptables "

static int merg(const char* str1,const char* str2,char** out){
    int len1=str1==NULL?0:strlen(str1);
    int len2=str2==NULL?0:strlen(str2);
    
    if(*out != NULL){
      free(*out);
      *out = NULL;
    }
    *out = (char*)malloc(sizeof(char)*(len1+len2+1));
    if(*out == NULL){
      ALOGE("!@malloc memory fail.\n");
      return -1;
    }
    char * temp = *out;
    if(str1 != NULL){
      while(*str1 != '\0'){
          *temp=*str1;
          temp++;
          str1++;
      }
    }
    
    if(str2 != NULL){
      while(*str2 != '\0'){
          *temp=*str2;
          temp++;
          str2++;
      }
    }
    *temp = '\0';
    return 0;
}

static int exec(char* cmd,char** reply) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe){
       ALOGE("!@Open pipe fail!\n");
       return -1;
    }
    char result[1024]={0,};
    char * lastReply = NULL;
    while(!feof(pipe)) {
     if(fgets(result, 1024, pipe) != NULL){
       merg(lastReply,result,reply);
       if(lastReply != NULL){
        free(lastReply);
        lastReply = NULL;
       }
       lastReply = (char *)malloc(sizeof(char)*(strlen(*reply)+1));
       strcpy(lastReply,*reply);
      }
    }
    pclose(pipe);
    return 0;
}

static int readx(int s, void *_buf, int count)
{
    char *buf = _buf;
    int n = 0, r;
    if (count < 0) return -1;
    while (n < count) {
        r = read(s, buf + n, count - n);
        ALOGE("!@Read count:%d\n"+r);
        if (r < 0) {
            if (errno == EINTR) continue;
            ALOGE("!@read error: %s\n", strerror(errno));
            return -1;
        }
        if (r == 0) {
            ALOGE("eof\n");
            return -1;
        }
        n += r;
    }
    return 0;
}

static int writex(int s, const void *_buf, int count)
{
    const char *buf = _buf;
    int n = 0, r;
    if (count < 0) return -1;
    while (n < count) {
        r = write(s, buf + n, count - n);
        if (r < 0) {
            if (errno == EINTR) continue;
            ALOGE("!@write error: %s\n", strerror(errno));
            return -1;
        }
        n += r;
    }
    return 0;
}

int main(const int argc, const char *argv[]) {
    char buf[BUFFER_MAX];
    struct sockaddr addr;
    socklen_t alen;
    int lsocket,client;
    ALOGE("!@Create socket");
    lsocket = android_get_control_socket(SOCKET_PATH);
    if (lsocket < 0) {
        ALOGE("!@Failed to get socket from environment: %s\n", strerror(errno));
        exit(1);
    }
    ALOGE("!@Wait for client");
    if (listen(lsocket, 1)) {
        ALOGE("!@Listen on socket failed: %s\n", strerror(errno));
        exit(2);
    }
    fcntl(lsocket, F_SETFD, FD_CLOEXEC);
    
    
    for (;;) {
        alen = sizeof(addr);
        client = accept(lsocket, &addr, &alen);
        if (client < 0) {
            ALOGE("!@Accept failed: %s\n", strerror(errno));
            continue;
        }
        
        fcntl(client, F_SETFD, FD_CLOEXEC);

        ALOGE("!@new connection %d\n",client);
        for (;;) {
            unsigned short count;
            if (readx(client, &count, sizeof(count))) {
                ALOGE("!@failed to read size\n");
                exit(3);
                break;
            }
            
            ALOGE("!@count: %d\n", count);
            if ((count < 1) || (count >= BUFFER_MAX)) {
                ALOGE("!@invalid size %d\n", count);
                exit(4);
                break;
            }
            
            if (readx(client, buf, count)) {
                ALOGE("!@failed to read command\n");
                exit(5);
                break;
            }
            buf[count] = 0;
            
            char * result = NULL;
            char * cmd = NULL;
            merg(IPTABLES,buf,&cmd);
            if(exec(cmd,&result)==0){
              count = strlen(result)+1;
              writex(client,&count,sizeof(count));
              writex(client,result,count);
            }
        }
    }
    
}

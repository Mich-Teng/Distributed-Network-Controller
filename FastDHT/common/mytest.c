#include "fdht_global.h"
#include "fdht_types.h"
#include "../client/fdht_client.h"
#include "logger.h"
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

int main(int argc, const char *argv[])
{
    int res, expire;
    FDHTKeyInfo key;
    struct sigaction act;

        char buf[1024] = "initstr";
        int len = 1024;
    if(argc < 3)
    {
        printf("too few arguments\n");
        exit(1);
    }
    
    memset(&act, 0, sizeof(act));
	sigemptyset(&act.sa_mask);
	act.sa_handler = SIG_IGN;
	if(sigaction(SIGPIPE, &act, NULL) < 0)
	{
		printf("file: "__FILE__", line: %d, " \
			"call sigaction fail, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno;
	}
    
    log_init();
    res = fdht_client_init("/etc/fdhtd/fdht_client.conf");
    if(res != 0)
    {
        printf("failed to load fdht_client.conf");
        exit(1);
    }
   
    int succ_cnt, fail_cnt;
    if ((res=fdht_connect_all_servers(&g_group_array, true, \
		&succ_cnt, &fail_cnt)) != 0)
	{
		printf("fdht_connect_all_servers fail, " \
			"error code: %d, error info: %s\n", \
			res, strerror(res));
		return res;
	
    }
    memset(&key, 0, sizeof(FDHTKeyInfo));
    if(strcmp(argv[1], "set") == 0)
    {
        strcpy(key.szKey, argv[2]);
        key.key_len = strlen(argv[2]) + 1;
        //strcpy(key.szKey, "kiikkkkkkkkkkkkkkkk15");
        //key.key_len = strlen(key.szKey);
        res = fdht_set(&key, FDHT_EXPIRES_NEVER, argv[3], strlen(argv[3]));
        if(res != 0)
        {
            printf("fdht_set failed\n");
            goto CLEANUP;
        }
    }
    else if(strcmp(argv[1], "get") == 0)
    {
        char *pbuf = buf;
        strcpy(key.szKey, argv[2]);
        key.key_len = strlen(argv[2]) + 1;
        res = fdht_get(&key, &pbuf, &len);
        if(res != 0)
        {
            printf("fdht_get failed, %s\n", strerror(res));
            goto CLEANUP;
        }
        else
        {
            buf[len] = 0;
            printf("%s\n", buf);
        }
    }
    else
    {
        printf("unrecognized command %s\n", argv[1]);
        exit(1);
    }
CLEANUP:
    fdht_disconnect_all_servers(&g_group_array);
    fdht_client_destroy();
    
    return 0;
}

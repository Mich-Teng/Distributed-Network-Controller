#include <string>
#include "InterDomainTopo.h"
#include "LinkInfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "fdht_global.h"
#include "sockopt.h"
#include "logger.h"
#include "shared_func.h"
#include "fdht_types.h"
#include "fdht_proto.h"
#include "fdht_client.h"
#include "fdht_func.h"

using namespace std;
#define DEBUG 1
#define NO_SUCH_LINK 2
#define MAX_LINKS_NUM 20

int getDestDomainId(const LinkInfo& link)
{
	return link.des_port;
}
int InterDomainTopo::addLink(LinkInfo& link_info,uint32_t sdid,uint32_t ddid)
{
    int res, expire;
    FDHTKeyInfo key;
    char buf[1024] = "initstr";
    int len = 1024;
  	char tmp[1024]="";
	int count =0;
	  int error_num;	
  	int dest_domain_id = ddid;
    link_info.serialize(tmp);
//*****************initialize *******************************
#ifdef DEBUG
   printf("dest domain id = %d\n",dest_domain_id);
#endif

   
   log_init();
   res = fdht_client_init("/etc/fdhtd/fdht_client.conf");
   if(res != 0)
   {
       printf("failed to load fdht_client.conf");
       exit(1);
   }
   link_info.serialize(tmp);
   memset(&key, 0, sizeof(FDHTKeyInfo));
#ifdef DEBUG
   printf("addlink 46: get value from dht\n");
#endif
	
//************************* end initialize ************************
  char *pbuf = buf;
  key.namespace_len = sprintf(key.szNameSpace,"LinkInfo");
  key.obj_id_len = sprintf(key.szObjectId,"%d",sdid);
  key.key_len = sprintf(key.szKey,"%d",dest_domain_id);
#ifdef DEBUG
	printf("key = %s,key_len=%d\n",key.szKey,key.key_len);
#endif
  res = fdht_get(&key, &pbuf, &len);
    if(res != 0)
    {
#ifdef DEBUG
        printf("fdht_get failed, %s\n", strerror(res));
#endif
	   	memcpy(buf,tmp,sizeof(LinkInfo));
		count = 1;
    }
    else
    {
#ifdef DEBUG
        printf("current_value in dht = %s\n", buf);
#endif
		
		memcpy(buf+len,tmp,sizeof(LinkInfo));
		count = len/sizeof(LinkInfo);
		count ++;
    }
	LinkInfo ss;
	ss.deserialize(buf);
	printf("%d\n%d\n%d\n",ss.src,ss.des,ss.src_port);
	if( (error_num=fdht_set(&key,FDHT_EXPIRES_NEVER,buf,sizeof(LinkInfo)*count)) != 0)
	{
		printf("set error\n");
		return error_num;         // if failed, return error number
	}
CLEANUP:
    fdht_disconnect_all_servers(&g_group_array);
    fdht_client_destroy();
	return 0;   // if succeed, return 0;	
}


int InterDomainTopo::deleteLink(LinkInfo& link_info,uint32_t sdid,uint32_t ddid)
{
	int i=0;
	LinkInfo* link_info_list[MAX_LINKS_NUM];
    int res;
    FDHTKeyInfo key;
    char buf[sizeof(LinkInfo)*MAX_LINKS_NUM] = "initstr";
    int value_len = 1024;
  	char tmp[1024]="";
	int count =0;
	int error_num;	
  	int dest_domain_id = ddid;
    link_info.serialize(tmp);
//*****************initialize *******************************
#ifdef DEBUG
   printf("dest domain id = %d\n",dest_domain_id);
#endif

   
   log_init();
   res = fdht_client_init("/etc/fdhtd/fdht_client.conf");
   if(res != 0)
   {
       printf("failed to load fdht_client.conf");
       exit(1);
   }
   memset(&key, 0, sizeof(FDHTKeyInfo));
#ifdef DEBUG
   printf("Delete Link : get value from dht\n");
#endif
	
//************************* end initialize ************************
  char *pbuf = buf;
  key.namespace_len = sprintf(key.szNameSpace,"LinkInfo");
  key.obj_id_len = sprintf(key.szObjectId,"%d",sdid);
  key.key_len = sprintf(key.szKey,"%d",dest_domain_id);
	res = fdht_get(&key,&pbuf,&value_len);
	if(res == 0)
	{
		int links_num = value_len/sizeof(LinkInfo);
#ifdef DEBUG
		printf("link num = %d",links_num);
#endif
		if(links_num > 1)
		{
			for( i=0; i< links_num; i++)
			{
				link_info_list[i] = new LinkInfo();
				memcpy(tmp,pbuf+sizeof(LinkInfo)*i,sizeof(LinkInfo));
				link_info_list[i]->deserialize(tmp);
				if( sameLinkInfo(*link_info_list[i],link_info))
					break;
			}
			if( i == links_num )
			{
#ifdef DEBUG
				printf("No such link\n");
#endif
				return NO_SUCH_LINK;
			}
			int index = i*sizeof(LinkInfo);
			for(int i = index; i< value_len-sizeof(LinkInfo);i++)
				buf[i] = buf[i+sizeof(LinkInfo)];
			links_num-=1;
			if( error_num = fdht_set(&key,FDHT_EXPIRES_NEVER,pbuf,sizeof(LinkInfo)*links_num) != 0)
				return error_num;
		}
		else
		{
      		res = fdht_delete(&key);
    		if(res !=0)
    		{
      			printf("fdht_delete failed,%s\n",strerror(res));
     			goto CLEANUP;
     		}
		}
    }
	else
			return NO_SUCH_LINK;
CLEANUP:
    fdht_disconnect_all_servers(&g_group_array);
    fdht_client_destroy();
	return 0;   // if succeed, return 0;	
}

int InterDomainTopo::getLinkInfo(int src_domain_id,int dest_domain_id,LinkInfo* link_info_list[],int* num)
{
    int res, expire;
    FDHTKeyInfo key;
    char buf[1024] = "initstr";
    int len = 1024;
  	char tmp[1024]="";
	  int error_num;	
	int links_num;

//*****************initialize *******************************
#ifdef DEBUG
   printf("dest domain id = %d\n",dest_domain_id);
#endif

   
   log_init();
   res = fdht_client_init("/etc/fdhtd/fdht_client.conf");
   if(res != 0)
   {
       printf("failed to load fdht_client.conf");
       exit(1);
   }
   memset(&key, 0, sizeof(FDHTKeyInfo));
#ifdef DEBUG
   printf("getLink: get value from dht\n");
#endif
	
//************************* end initialize ************************
  char *value = buf;
  key.namespace_len = sprintf(key.szNameSpace,"LinkInfo");
  key.obj_id_len = sprintf(key.szObjectId,"%d",src_domain_id);
  key.key_len = sprintf(key.szKey,"%d",dest_domain_id);
	if( (error_num = fdht_get(&key,&value,&len) ) != 0 )
	{
		printf("can't find key\n");
		return error_num;
	}
#ifdef DEBUG
	printf("get value = %s len = %d,sizeof LinkInfo = %d\n",value,len,sizeof(LinkInfo));
#endif
	links_num = len/sizeof(LinkInfo);
	printf("num = %d\n",links_num);
	for(int i=0; i < links_num; i++ )
	{
		memcpy(tmp,value+sizeof(LinkInfo)*i,sizeof(LinkInfo));
#ifdef DEBUG
		printf("get from dht value = %s\n",tmp);
#endif
		link_info_list[i] = new LinkInfo();
		link_info_list[i]->deserialize(tmp); // desearialize here
#ifdef DEBUG
	printf("after deserialize: src = %s\tsrc_port = %d\tdes = %s\t des_port = %d\n", link_info_list[i]->src, link_info_list[i]->src_port, link_info_list[i]->des, link_info_list[i]->des_port);
#endif
	}
	(*num) = links_num;
CLEANUP:
    fdht_disconnect_all_servers(&g_group_array);
    fdht_client_destroy();
	return 0;   // if succeed, return 0;	
}

InterDomainTopo::InterDomainTopo(const string conf_filename)
{
//	dht_wrapper = dht_wrapper->getInstance(conf_filename);
}


InterDomainTopo::InterDomainTopo()
{
}

bool InterDomainTopo::sameLinkInfo(const LinkInfo& l1,const LinkInfo& l2)
{
	if(l1.src_port==l2.src_port&&l1.des_port==l2.des_port&&(l1.src==l2.src)&&(l1.des==l2.des))
		return true;
	return false;
}


/*
int main()
{
	int num;
	char str[200];
	InterDomainTopo p;
#ifdef DEBUG
	printf("initialization succeed\n");
#endif
	LinkInfo link_info;
	LinkInfo* link_info_list[MAX_LINKS_NUM];
	link_info.src_port = 1;
	link_info.des_port = 2;
	strcpy(link_info.src,"src_addr");
	strcpy(link_info.des,"des_addr");

#ifdef DEBUG
//	printf("prepare to add link\n");
#endif
//	p.addLink(link_info);
#ifdef DEBUG
//	printf("add link succeed, get link\n");
#endif
//	p.getLinkInfo(1,2,link_info_list,&num);
	printf("%d",num);
	p.deleteLink(link_info);
	return 0;

}
*/

#include "LinkInfo.h"


LinkInfo link_demo,link_new;
LinkInfo::LinkInfo()
{
	src[0] = 0;
	des[0] = 0;
	src_port = 0;
	des_port = 0;
}

void LinkInfo::serialize(char *series_buf)
{
	for (int i = 0; i < sizeof(LinkInfo); i++)
	{
		series_buf[i] = ((char*)this)[i];
	}
	((LinkInfo*) series_buf)->src_port = htonl(((LinkInfo*) series_buf)->src_port);
	((LinkInfo*) series_buf)->des_port = htonl(((LinkInfo*) series_buf)->des_port);

}

void LinkInfo::deserialize(char* series_buf)
{
	strcpy(src, ((LinkInfo*)series_buf)->src);
	strcpy(des, ((LinkInfo*)series_buf)->des);
	src_port =  ntohl(((LinkInfo*)series_buf)->src_port);
	des_port =  ntohl(((LinkInfo*)series_buf)->des_port);
}
	


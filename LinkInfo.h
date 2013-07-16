#ifndef LINKINFO_H
#define LINKINFO_H

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include "netinet++/datapathid.hh"

const int host_ID_len = 200;
typedef datapathid HostIDType ;
typedef uint32_t Port;
class LinkInfo
{
public:
	HostIDType	src;
	Port		src_port;
	HostIDType	des;
	Port		des_port;

	LinkInfo();
	void serialize(char *series_buf);
	void deserialize(char* series_buf);

};
#endif

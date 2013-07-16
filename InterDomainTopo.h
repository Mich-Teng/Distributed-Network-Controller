#ifndef INTERDOMAINTOPO_H
#define INTERDOMAINTOPO_H
#include "LinkInfo.h"
#include <string>
class InterDomainTopo
{
	public:
		InterDomainTopo(const std::string);
		InterDomainTopo();
		int addLink(LinkInfo& link_info,uint32_t sdid,uint32_t ddid);
		int deleteLink(LinkInfo& link_info,uint32_t sdid, uint32_t ddid);
		int getLinkInfo(int src_domain_id,int dest_domain_id,LinkInfo* link_info_list[],int* num);
	private:
		bool sameLinkInfo(const LinkInfo&,const LinkInfo&);
//		FDHTClientWrapper* dht_wrapper;
};

#endif

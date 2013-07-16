#include "LinkEventHandler.h"
#include "InterDomainTopo.h"
#include <iostream>

void LinkEventHandler::event_handler(Link_event* event)
{
  if( isExternal == INTERNAL)
    return ;
  InterDomainTopo *p = new InterDomainTopo();
  LinkInfo link_info;
  link_info.src = sdid ;
  link_info.src_port = sport ;
  link_info.des = ddid;
  link_info.des_port = dport ;
 
  switch(action)
  {
    case ADD:
      p->addLink(link_info);
      break;
    case REMOVE:
      p->deleteLink(link_info);
      break;
    default:
      std::cout << "undefined operation!" << endl;
      exit(1);

  }
  return ;
}

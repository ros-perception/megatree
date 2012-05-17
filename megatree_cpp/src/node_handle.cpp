#include <megatree/node_handle.h>
#include <megatree/node_file.h>


namespace megatree
{


  std::string NodeHandle::toString() const
  {
    std::stringstream ss;
    ss << "Node " << id.toString() << std::endl 
       << "  count " << node->getCount() << std::endl
       << "  point " << (unsigned)(node->point[0]) << ", " << (unsigned)(node->point[1]) << ", " << (unsigned)(node->point[2]) << std::endl
       << "  color " << (unsigned)(node->color[0]) << ", " << (unsigned)(node->color[1]) << ", " << (unsigned)(node->color[2]) << std::endl
       << "  children ";
  
    for (unsigned int i=0; i<8; i++)
      ss << (int)((node->children >> i)&1) << " ";
    ss << std::endl;
    return ss.str();
  }
  
}

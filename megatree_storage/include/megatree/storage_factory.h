#ifndef MEGATREE_STORAGE_FACTORY_H
#define MEGATREE_STORAGE_FACTORY_H

#include <megatree/storage.h>

namespace megatree {

enum { UNKNOWN_FORMAT, NORMAL_FORMAT, VIZ_FORMAT };

boost::shared_ptr<Storage> openStorage(const boost::filesystem::path &path, unsigned format=NORMAL_FORMAT);
boost::shared_ptr<TempDir> createTempDir(const boost::filesystem::path &parent, bool remove=true);

void removePath(const boost::filesystem::path &path);

}


#endif

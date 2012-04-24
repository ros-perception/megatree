#include <megatree/storage_factory.h>

#include <megatree/viz_storage.h>
#include <megatree/disk_storage.h>

#ifdef USE_HBASE
  #include <megatree/hbase_storage.h>
#endif

namespace megatree {

enum { UNKNOWN_STORAGE, DISK_STORAGE, HBASE_STORAGE, CLIENT_STORAGE };

int storageType(const boost::filesystem::path &path)
{
  if (path.string().substr(0, 8) == std::string("hbase://"))
    return HBASE_STORAGE;
  
  return DISK_STORAGE;
}

boost::shared_ptr<Storage> openStorage(const boost::filesystem::path &path, unsigned format)
{
  boost::shared_ptr<Storage> storage;
  int storage_type = storageType(path);

  switch (format)
  {
  case NORMAL_FORMAT:
    {
      switch (storage_type)
      {
      case DISK_STORAGE:
        storage.reset(new DiskStorage(path));
        break;

#ifdef USE_HBASE        
      case HBASE_STORAGE:
        storage.reset(new HbaseStorage(path));
        break;
#endif

      case UNKNOWN_STORAGE:
        fprintf(stderr, "Unknown storage type for format 1: %s\n", path.string().c_str());
        storage.reset();
        break;
      default:
        abort();
        break;
      }
    }
    break;

  default:
    {
      fprintf(stderr, "Unknown storage format: %d\n", format);
      abort();
    }
    break;
  }

  return storage;
}

boost::shared_ptr<TempDir> createTempDir(const boost::filesystem::path &parent, bool remove)
{
  boost::shared_ptr<TempDir> tempdir; 
  
  switch (storageType(parent))
  {
  case DISK_STORAGE:
    tempdir.reset(new DiskTempDir(parent, remove));
    break;
  case UNKNOWN_STORAGE:
    fprintf(stderr, "Unknown storage type: %s\n", parent.string().c_str());
    tempdir.reset();
    break;
  default:
    abort();
    break;
  }

  return tempdir;
}


void removePath(const boost::filesystem::path &path)
{
  switch (storageType(path))
  {
  case DISK_STORAGE:
    boost::filesystem::remove_all(path);
    break;

#ifdef USE_HBASE        
  case HBASE_STORAGE:
    removeHbasePath(path);
    break;
#endif

  case UNKNOWN_STORAGE:
    fprintf(stderr, "Unknown storage type: %s\n", path.string().c_str());
    break;
  default:
    abort();
    break;
  }
}

}

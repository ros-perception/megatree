#include <megatree/viz_storage.h>
#include <megatree/storage_factory.h>
#include <megatree/metadata.h>


namespace megatree
{
  VizStorage::VizStorage(const boost::filesystem::path &path): tree(path)
  {
    storage = openStorage(path, NORMAL_FORMAT);
    
    // get subtree width
    ByteVec data;
    storage->get("metadata.ini", data);
    MetaData metadata;
    metadata.deserialize(data);
    subtree_width = metadata.subtree_width;
  }

  void VizStorage::get(const boost::filesystem::path &path, ByteVec &result)
  {
    ByteVec tmp;
    storage->get(path, tmp);
    convert(tmp, result);
  }

  void VizStorage::getBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results)
  {
    std::vector<ByteVec> tmp;
    storage->getBatch(paths, tmp);

    results.resize(tmp.size());
    for (unsigned i=0; i<results.size(); i++)
      convert(tmp[i], results[i]);
  }

  void VizStorage::getAsync(const boost::filesystem::path &path, GetCallback callback)
  {
    if (path == "metadata.ini" || path == "views.ini")
      storage->getAsync(path, callback); 
    else
      storage->getAsync(path, boost::bind(&VizStorage::convertCb, this, path, callback, _1));
  }

  
void VizStorage::convertCb(const boost::filesystem::path &path, GetCallback cb, const ByteVec& data)
  {
    printf("Response for tree %s and file %s\n", tree.string().c_str(), path.string().c_str());

    ByteVec data_converted;
    convert(data, data_converted);
    cb(data_converted);
  }


  void VizStorage::convert(const ByteVec& data, ByteVec& res)
  {
    const static size_t STRIDE = 3 + 3;
    
    uint64_t num_nodes = (data.size()-1)/NODE_SIZE;
    res.resize(1+num_nodes*STRIDE);  // max possible size

    unsigned res_offset = 0;
    unsigned data_offset = 0;

    // children of file
    memcpy(&res[res_offset], &data[data_offset], 1);
    res_offset += 1;
    data_offset += 1;

    // node data
    while (data_offset < data.size())
    {
      // pre-read short id
      uint32_t short_id;
      memcpy(&short_id, &data[data_offset + POINT_SIZE + COLOR_SIZE + COUNT_SIZE + CHILDREN_SIZE], SHORT_ID_SIZE);

      // read point
      Point pnt[3];
      for (unsigned i=0; i<3; i++)
      {      
        //TODO: We're checking out how rendering from the center of a cell looks
        pnt[i] = 32768;
        //pnt[i] = 0;
        //memcpy(&pnt[i], &data[data_offset], POINT_SIZE / 3);
        data_offset += POINT_SIZE / 3;
      }

      // convert point from local node frame to frame of node file
      for (unsigned i=0; i<subtree_width; i++)
      {
        int which = (short_id >> (i*3)) & 07;
        pnt[0] = (pnt[0] >> 1) | ((which & (1<<X_BIT)) ? 1<<(8*POINT_SIZE/3-1) : 0);
        pnt[1] = (pnt[1] >> 1) | ((which & (1<<Y_BIT)) ? 1<<(8*POINT_SIZE/3-1) : 0);
        pnt[2] = (pnt[2] >> 1) | ((which & (1<<Z_BIT)) ? 1<<(8*POINT_SIZE/3-1) : 0);
      }

      // one byte per point component
      for (unsigned i=0; i<3; i++)
      {      
        res[res_offset] = pnt[i] >> 8*(POINT_SIZE / 3 - 1);
        res_offset  += 1;
      }
    
      // one byte per color component
      for (unsigned i=0; i<3; i++)
      {      
        memcpy(&res[res_offset], &data[data_offset], 1);
        data_offset += COLOR_SIZE / 3;
        res_offset += 1;
      }

      // Extracts the count
      Count count;
      memcpy(&count, &data[data_offset], COUNT_SIZE);
      data_offset += COUNT_SIZE;

      // skip children of node
      uint8_t children;
      memcpy(&children, &data[data_offset], 1);
      data_offset += CHILDREN_SIZE;
    
      // Stashes whether the node is a leaf or not into the color
      if (children == 0) {
        res[res_offset-1] &= (~1);
      }
      else {
        res[res_offset-1] |= 1;
      }

      // skip short id
      data_offset += SHORT_ID_SIZE;
    }    
  }
}

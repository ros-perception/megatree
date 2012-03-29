#include <megatree/disk_storage.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>


namespace megatree {




void DiskStorage::get(const boost::filesystem::path &path, ByteVec &result)
{
  assert(boost::filesystem::exists(root / path));

  // mmaps the file
  boost::iostreams::mapped_file_params params;
  params.path = (root / path).string();
  params.mode = std::ios_base::in;
  params.offset = 0;
  boost::iostreams::mapped_file file(params);
  
  // Copies data from the file
  result.resize(file.size());
  memcpy((void*)&result[0], file.const_data(), file.size());
}

void DiskStorage::getBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &results)
{
  // TODO: Execute the reads in parallel
  results.resize(paths.size());
  for (size_t i = 0; i < paths.size(); ++i)
    get(paths[i], results[i]);
}

void DiskStorage::putBatch(const std::vector<boost::filesystem::path> &paths, std::vector<ByteVec> &data)
{
  assert(paths.size() == data.size());
  for (size_t i = 0; i < paths.size(); ++i)
  {
    assert(data[i].size() > 0);
    boost::filesystem::path abspath = root / paths[i];

    //make sure thate the directory exists
    if (!boost::filesystem::exists(abspath.parent_path()))
      boost::filesystem::create_directories(abspath.parent_path());

    // mmaps the file
    boost::iostreams::mapped_file_params params;
    params.path = abspath.string();
    params.mode = std::ios_base::out;
    params.offset = 0;
    params.new_file_size = data[i].size();
    boost::iostreams::mapped_file file(params);

    // Writes the data to the file
    memcpy(file.data(), (void*)&data[i][0], data[i].size());
    file.close();
  }
}

void DiskStorage::readerFunction(const boost::filesystem::path &path, GetCallback callback)
{
  ByteVec result;
  get(path, result);
  callback(result);
}

void DiskStorage::getAsync(const boost::filesystem::path &path, GetCallback callback)
{
  function_caller.addFunction(boost::bind(&DiskStorage::readerFunction, this, path, callback));
}


void DiskStorage::writerFunction(const boost::filesystem::path &path, const ByteVec &data, PutCallback callback)
{
  put(path, data);
  callback();
}

void DiskStorage::putAsync(const boost::filesystem::path &path, const ByteVec &data, PutCallback callback)
{
  function_caller.addFunction(boost::bind(&DiskStorage::writerFunction, this, path, data, callback));
}



}

#include <cstdio>
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include <fcgiapp.h>

#include <megatree/metadata.h>
#include <megatree/tree_common.h>
#include <megatree/node_file.h>
#include <megatree/storage.h>
#include <megatree/storage_factory.h>

typedef std::map<std::string, boost::shared_ptr<megatree::Storage> > StorageMap;
StorageMap g_storage_map;


void emitPong(FCGX_Stream *out)
{
  FCGX_FPrintF(out, 
               "Content-Type: text/html\r\n"
               "\r\n"
               "files pong");
}

void emitTrees(FCGX_Stream *out, const std::vector<std::string>& trees)
{
  std::string tree_string;
  for (unsigned i=0; i<trees.size(); i++){
    tree_string += trees[i];
    if (i < trees.size()-1)
      tree_string += ",";
  }
  FCGX_FPrintF(out, 
               "Content-Type: text/html\r\n"
               "\r\n"
               "%s", tree_string.c_str());
}


#define NL "<br />\n"
//#define DUMP_VAR(x) FCGX_FPrintF(out, "%s: %s" NL, #x, FCGX_GetParam(#x, envp) ? FCGX_GetParam(#x, envp) : "<null>")
#define DUMP_VAR(x) FCGX_FPrintF(out, "<tr><td>%s</td><td>%s</td></tr>\n",   \
                                 #x, FCGX_GetParam(#x, envp) ? FCGX_GetParam(#x, envp) : "<null>")
void emitDump(FCGX_Stream *out, char **envp)
{
  FCGX_FPrintF(out, "Content-Type: text/html\r\n");
  FCGX_FPrintF(out, "\r\n");
  FCGX_FPrintF(out, "<html>\n");
  FCGX_FPrintF(out, "<head><title>Dump</title></head>");
  FCGX_FPrintF(out, "<body>");
  FCGX_FPrintF(out, "<h1>Dump</h1>\n");
  
  FCGX_FPrintF(out, "<table>\n");
  FCGX_FPrintF(out, "<tr><td>Compiled</td><td>%s  %s</td></tr>\n", __DATE__, __TIME__);
  FCGX_FPrintF(out, "<tr><td>Pid</td><td>%d</td></tr>\n", getpid());

  FCGX_FPrintF(out, "<tr><td></td><td></td></tr>\n");
  FCGX_FPrintF(out, "<tr><td><b>Env</b></td><td></td></tr>\n");
  DUMP_VAR(DOCUMENT_ROOT);
  DUMP_VAR(CONTENT_LENGTH);
  DUMP_VAR(HTTP_REFERER);
  DUMP_VAR(HTTP_USER_AGENT);
  DUMP_VAR(PATH_INFO);
  DUMP_VAR(PATH_TRANSLATED);
  DUMP_VAR(QUERY_STRING);
  DUMP_VAR(REMOTE_ADDR);
  DUMP_VAR(REMOTE_HOST);
  DUMP_VAR(REQUEST_METHOD);
  DUMP_VAR(SCRIPT_NAME);
  DUMP_VAR(SERVER_NAME);
  DUMP_VAR(HTTP_COOKIE);
  DUMP_VAR(HTTP_HOST);
  
  FCGX_FPrintF(out, "</table>\n");
  FCGX_FPrintF(out, "</body></html>\n");
}

void emitBox(FCGX_Stream *out)
{
  FCGX_FPrintF(out, "Content-Type: application/octet-stream\r\n");
  FCGX_FPrintF(out, "\r\n");

  // Cube, from [64,92], with min corner in red, others in white
  uint8_t coords[] = {
    0, // Children
    64, 64, 64,     255, 0, 0,
    192, 64, 64,    255, 255, 255,
    64, 192, 64,   255, 255, 255,
    192, 192, 64,   255, 255, 255,
    64, 64, 192,   255, 255, 255,
    192, 64, 192,   255, 255, 255,
    64, 192, 192,   255, 255, 255,
    192, 192, 192,  255, 255, 255
  };

  FCGX_PutStr((char*)coords, 1 + 8 * 6 * sizeof(uint8_t), out);
}


void sendResult(boost::shared_ptr<FCGX_Request> req, const std::string &filename, const megatree::ByteVec& file_data)
{
  if (file_data.empty()) {
    FCGX_FPrintF(req->out, "Status: 404 Not Found\r\n");
    FCGX_FPrintF(req->out, "Content-Type: text/html\r\n");
    FCGX_FPrintF(req->out, "\r\n");
    FCGX_FPrintF(req->out, "File not found: %s<br />\n", filename.c_str());
  }
  else{
    FCGX_FPrintF(req->out, "Content-Type: application/octet-stream\r\n");
    //FCGX_FPrintF(req->out, "X-Server-Id: " << num << "\r\n", num);
    FCGX_FPrintF(req->out, "\r\n");
    FCGX_PutStr((char*)&file_data[0], file_data.size(), req->out);
  }
  
  FCGX_Finish_r(req.get());
}

void emitFile(boost::shared_ptr<FCGX_Request> req, const std::string& tree, const std::string &filename)
{
  StorageMap::iterator it = g_storage_map.find(tree);
  if (it == g_storage_map.end())
    fprintf(stderr, "Server does not support tree %s\n", tree.c_str());
  else
    it->second->getAsync(filename, boost::bind(&sendResult, req, filename, _1));
}



std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  return split(s, delim, elems);
}


std::map<std::string, std::string> getParams(char* query_string)
{
  std::map<std::string, std::string> params;
  std::vector<std::string> params_vec = split(std::string(query_string), '&');
  for (unsigned i=0; i<params_vec.size(); i++){
    std::vector<std::string> param_vec = split(params_vec[i], '=');    
    if (param_vec.size() == 2)
      params[param_vec[0]] = param_vec[1].c_str();
  }
  return params;
}



int main(int argc, char** argv)
{
  using namespace boost::algorithm;
  
  int ret = FCGX_Init();
  if (ret != 0) {
    fprintf(stderr, "Unable to initialize fcgi\n");
    return 1;
  }

  std::vector<std::string> trees;
  //trees.push_back("chappes");
  //trees.push_back("bourges");
  //trees.push_back("tahoe_all");
  //trees.push_back("tahoe_roads");
  //trees.push_back("kendall_all");
  //trees.push_back("car");

  if (argc > 1)
    trees.push_back(argv[1]);

  for (unsigned i=0; i<trees.size(); i++)
    g_storage_map[trees[i]] = megatree::openStorage(trees[i], megatree::VIZ_FORMAT);


  while (true)
  {
    boost::shared_ptr<FCGX_Request> req(new FCGX_Request);
    ret = FCGX_InitRequest(req.get(), 0, 0);
    assert(ret == 0);
    ret = FCGX_Accept_r(req.get());
    assert(ret == 0);

    char* path_string = FCGX_GetParam("PATH_INFO", req->envp);
    if(path_string == NULL)
    {
      fprintf(stderr, "The path string is null!!!!\n");
    }

    
    std::string request_path(path_string ? path_string : "");
    if (request_path == "/ping") {
      emitPong(req->out);
      FCGX_Finish_r(req.get());
    }
    else if (request_path == "/box") {
      emitBox(req->out);
      FCGX_Finish_r(req.get());
    }
    else if (request_path == "/serve_new_tree") {
      char* query_string = FCGX_GetParam("QUERY_STRING", req->envp);
      std::map<std::string, std::string> params = getParams(query_string);      
      if (params.find("new_tree_name") != params.end()){
        std::string new_tree_name = params["new_tree_name"];
        std::vector<std::string> name_split = split(new_tree_name, '_');        
        if (name_split[0] == "proc" && name_split[1] == "tahoe"){
          trees.push_back(new_tree_name);
          g_storage_map[new_tree_name] = megatree::openStorage("hbase://wgsc3/" + new_tree_name, megatree::VIZ_FORMAT);
          printf("Now also serving tree %s\n", new_tree_name.c_str());
          FCGX_FPrintF(req->out, 
                       "Content-Type: text/html\r\n"
                       "\r\n"
                       "%s", new_tree_name.c_str());
        }
        else
          printf("Wrong tree name for serving new tree: %s\n", query_string);
      }
      else
        printf("Wrong parameters for serving new tree: %s\n", query_string);
      FCGX_Finish_r(req.get());
    }
    else if (request_path == "/supported_trees") {
      emitTrees(req->out, trees);
      FCGX_Finish_r(req.get());
    }
    else if (starts_with(request_path, "/")) {
      int slash = request_path.substr(1).find('/');
      std::string tree = request_path.substr(1, slash);
      std::string filename = request_path.substr(slash+2);
      printf("Received request for tree %s and file %s\n", tree.c_str(), filename.c_str());
      emitFile(req, tree, filename);
    }
    else {
      emitDump(req->out, req->envp);
      FCGX_Finish_r(req.get());
    }
  }
  
  return 0;
}

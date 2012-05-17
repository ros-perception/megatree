#include <cstdio>
#include <fstream>
#include <vector>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include <fcgiapp.h>

#include <megatree/metadata.h>
#include <megatree/tree_common.h>
#include <megatree/node_file.h>
#include <megatree/storage.h>
#include <megatree/storage_factory.h>
#include <megatree/process_box_executive.h>



double g_progress;
bool g_finished;

void progressCb(const std::string tree_name, double progress)
{
  g_progress = progress;
  printf("Tree %s is %f complete\n", tree_name.c_str(), progress*100);
}

void finishedCb(const std::string tree_name)
{
  g_finished = true;
  printf("Tree %s is fully generated\n", tree_name.c_str());
}


std::string detectRoads(FCGX_Stream *out, std::map<std::string, std::string>& p,
                        boost::shared_ptr<megatree::ProcessBoxExecutive> executive)
{
  // get parameters
  std::stringstream res;
  res << "Started road detection on tree "
      << p["tree_name"] << " in box (" 
      << p["l_x"] << ", " << p["l_y"] << ", " << p["l_z"] << "), ("
      << p["h_x"] << ", " << p["h_y"] << ", " << p["h_z"] << ")";
  printf("Detecting roads on %s\n", res.str().c_str());
  
  std::string source_tree_name = p["tree_name"];
  std::vector<double> min_corner(3), max_corner(3), tile_size(3);
  min_corner[0] = atof(p["l_x"].c_str());
  min_corner[1] = atof(p["l_y"].c_str());
  min_corner[2] = atof(p["l_z"].c_str());
  max_corner[0] = atof(p["h_x"].c_str());
  max_corner[1] = atof(p["h_y"].c_str());
  max_corner[2] = atof(p["h_z"].c_str());
  tile_size[0] = 1000;
  tile_size[1] = 1000;
  tile_size[2] = 1e6;
  double overlap = 0.0;
  
  // call road detection in box
  g_finished = false;
  g_progress = 0.0;
  std::string result_tree_name = executive->process(source_tree_name,
                                                    min_corner, max_corner, tile_size, overlap, 
                                                    boost::bind(&progressCb, _1, _2),
                                                    boost::bind(&finishedCb, _1));
  
  // successfully started road detection
  FCGX_FPrintF(out, 
               "Content-Type: text/html\r\n"
               "\r\n"
               "%s", result_tree_name.c_str());

  return result_tree_name;
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




void emitPong(FCGX_Stream *out)
{
  FCGX_FPrintF(out, 
               "Content-Type: text/html\r\n"
               "\r\n"
               "algorithm pong");
}



void getProgress(FCGX_Stream *out, const std::string& tree_name)
{
  printf("Get progress of tree %s\n", tree_name.c_str());
  FCGX_FPrintF(out, 
               "Content-Type: text/html\r\n"
               "\r\n"
               "%f", g_progress);
}

void isFinished(FCGX_Stream *out, const std::string& tree_name)
{
  printf("Get progress of tree %s\n", tree_name.c_str());
  FCGX_FPrintF(out, 
               "Content-Type: text/html\r\n"
               "\r\n"
               "%d", g_finished);
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





int main(int argc, char** argv)
{
  ros::init(argc, argv, "algorithm_server");

  using namespace boost::algorithm;
  
  int ret = FCGX_Init();
  if (ret != 0) {
    fprintf(stderr, "Unable to initialize fcgi\n");
    return 1;
  }

  if (argc != 2)
  {
    printf("Usage: ./algorithm_server num_clients\n");
    return -1;
  }
  printf("Starting up algorithm server with %s clients\n", argv[1]);
  boost::shared_ptr<megatree::ProcessBoxExecutive> executive(new megatree::ProcessBoxExecutive(atoi(argv[1])));


  printf("Running main server loop\n");
  while (ros::ok())
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
    // ping
    if (request_path == "/ping") {
      printf("Ping request\n");
      emitPong(req->out);
    }

    // road detection
    else if (request_path == "/road_detection") {
      char* query_string = FCGX_GetParam("QUERY_STRING", req->envp);
      std::map<std::string, std::string> params = getParams(query_string);      
      if (params.size() == 7){
        std::string tree_name = detectRoads(req->out, params, executive);
        printf("Road detection started, resulting tree %s\n", tree_name.c_str());
      }
      else
        printf("Wrong parameters for road detection: %s\n", query_string);
    }

    // progress
    else if (request_path == "/progress_monitor") {
      char* query_string = FCGX_GetParam("QUERY_STRING", req->envp);
      std::map<std::string, std::string> params = getParams(query_string);      
      if (params.find("tree_name") != params.end())
        getProgress(req->out, params["tree_name"]);
      else
        printf("Wrong parameters for progress monitor: %s\n", query_string);
    }

    // completion
    else if (request_path == "/completion_monitor") {
      char* query_string = FCGX_GetParam("QUERY_STRING", req->envp);
      std::map<std::string, std::string> params = getParams(query_string);      
      if (params.find("tree_name") != params.end())
        isFinished(req->out, params["tree_name"]);
      else
        printf("Wrong parameters for completion monitor: %s\n", query_string);
    }

    else {
      emitDump(req->out, req->envp);
    }
    FCGX_Finish_r(req.get());
  }
  
  return 0;
}

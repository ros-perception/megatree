#include <megatree/common.h>

#include <unistd.h>
#include <iostream>
#include <argp.h>
#include <float.h>

#include <liblas/liblas.hpp>
#include <fstream>
#include <iostream>

#include <boost/filesystem.hpp>

using namespace megatree;

class TempDirectory
{
public:
  TempDirectory(const std::string &prefix = "", bool remove = true)
    : remove_(remove)
  {
    std::string tmp_storage = prefix + "XXXXXX";
    char *tmp = mkdtemp(&tmp_storage[0]);
    assert(tmp);
    printf("Temporary directory: %s\n", tmp);

    path_ = tmp;
  }

  ~TempDirectory()
  {
    if (remove_)
      boost::filesystem::remove_all(path_);
  }

  const boost::filesystem::path &getPath() const
  {
    return path_;
  }

private:
  boost::filesystem::path path_;
  bool remove_;
};


// Returns, in lo and hi, the minimum and maximum bounds of the points in las_filenames.
void getExtents(const std::vector<std::string> &las_filenames, double lo[3], double hi[3])
{
  lo[0] = lo[1] = lo[2] = DBL_MAX;
  hi[0] = hi[1] = hi[2] = DBL_MIN;
  
  for (size_t i = 0; i < las_filenames.size(); ++i)
  {
    std::ifstream fin;
    fin.open(las_filenames[i].c_str(), std::ios::in | std::ios::binary);

    liblas::ReaderFactory f;
    liblas::Reader reader = f.CreateWithStream(fin);

    liblas::Header const& header = reader.GetHeader();
    lo[0] = std::min(lo[0], header.GetMinX());
    lo[1] = std::min(lo[1], header.GetMinY());
    lo[2] = std::min(lo[2], header.GetMinZ());
    hi[0] = std::max(hi[0], header.GetMaxX());
    hi[1] = std::max(hi[1], header.GetMaxY());
    hi[2] = std::max(hi[2], header.GetMaxZ());
  }
}

// http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/

// "Insert" a 0 bit after each of the 16 low bits of x
uint32_t Part1By1(uint32_t x)
{
  x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
  x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

// "Insert" two 0 bits after each of the 10 low bits of x
uint32_t Part1By2(uint32_t x)
{
  x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

uint32_t EncodeMorton2(uint32_t x, uint32_t y)
{
  return (Part1By1(y) << 1) + Part1By1(x);
}

uint32_t EncodeMorton3(uint32_t x, uint32_t y, uint32_t z)
{
  return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}

struct MortonPoint
{
  uint32_t morton;
  double x, y, z;
  double r, g, b;

  bool operator<(const MortonPoint &mp) const
  {
    return morton < mp.morton;
  }
};

std::string chunkFilename(unsigned int level, unsigned int num)
{
  char buf[255];
  snprintf(buf, 255, "%04u_%07u", level, num);
  return std::string(buf);
}

void writePoints(const std::vector<MortonPoint> &points, const boost::filesystem::path &path)
{
  FILE* f = fopen(path.string().c_str(), "wb");
  assert(f);
  for (size_t i = 0; i < points.size(); ++i)
  {
    fwrite(&points[i], sizeof(MortonPoint), 1, f);
  }
  fclose(f);
}

void mergeChunks(const boost::filesystem::path& inpath1,
                 const boost::filesystem::path& inpath2,
                 const boost::filesystem::path& outpath)
{
  printf("Merging %s and %s into ==> %s\n", inpath1.string().c_str(), inpath2.string().c_str(), outpath.string().c_str());
  FILE* fin1 = fopen(inpath1.string().c_str(), "rb");
  FILE* fin2 = fopen(inpath2.string().c_str(), "rb");
  FILE* fout = fopen(outpath.string().c_str(), "wb");

  assert(fin1);  assert(fin2);  assert(fout);

  MortonPoint p1, p2;
  if (fread(&p1, sizeof(MortonPoint), 1, fin1) < 1 ||
      fread(&p2, sizeof(MortonPoint), 1, fin2) < 1)
  {
    fprintf(stderr, "ERROR!  One of the files was empty!\n");
    abort();
  }

  bool file1_finished;
  while (true)
  {
    if (p1 < p2) {
      fwrite(&p1, sizeof(MortonPoint), 1, fout);
      size_t ret = fread(&p1, sizeof(MortonPoint), 1, fin1);
      if (ret < 1) {
        assert(feof(fin1));
        file1_finished = true;
        break;
      }
    }
    else {
      fwrite(&p2, sizeof(MortonPoint), 1, fout);
      size_t ret = fread(&p2, sizeof(MortonPoint), 1, fin2);
      if (ret < 1) {
        assert(feof(fin2));
        file1_finished = false;
        break;
      }
    }
  }

  // Copies over the rest of the data from whichever file didn't finish.
  if (file1_finished) {
    // File 2 didn't finish.
    while (true) {
      fwrite(&p2, sizeof(MortonPoint), 1, fout);
      size_t ret = fread(&p2, sizeof(MortonPoint), 1, fin2);
      if (ret < 1) {
        assert(feof(fin2));
        break;
      }
    }
  }
  else {
    // File 1 didn't finish.
    while (true) {
      fwrite(&p1, sizeof(MortonPoint), 1, fout);
      size_t ret = fread(&p1, sizeof(MortonPoint), 1, fin1);
      if (ret < 1) {
        assert(feof(fin1));
        break;
      }
    }
  }

  fclose(fin1);
  fclose(fin2);
  fclose(fout);
}


struct arguments_t {
  int chunk_size;
  char* tree;
  std::vector<std::string> las_filenames;
};

static int parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments_t *arguments = (arguments_t*)state->input;
  
  switch (key)
  {
  case 'c':
    arguments->chunk_size = parseNumberSuffixed(arg);
    break;
  case 't':
    arguments->tree = arg;
    break;
  case ARGP_KEY_ARG:
    arguments->las_filenames.push_back(arg);
    break;
  }
  return 0;
}


int main (int argc, char** argv)
{
  // Default command line arguments
  struct arguments_t arguments;
  arguments.chunk_size = 10000000;
  arguments.tree = 0;
  
  // Parses command line options
  struct argp_option options[] = {
    {"chunk-size", 'c', "SIZE",   0,     "Chunk size (for initial sorts)"},
    { 0 }
  };
  struct argp argp = { options, parse_opt };
  int parse_ok = argp_parse(&argp, argc, argv, 0, 0, &arguments);
  printf("Arguments parsed: %d\n", parse_ok);
  printf("Chunk size: %d\n", arguments.chunk_size);
  assert(parse_ok == 0);

  if (arguments.las_filenames.size() == 0) {
    fprintf(stderr, "No files given.\n");
    exit(1);
  }

  double lo[3], hi[3];
  getExtents(arguments.las_filenames, lo, hi);
  printf("Points extend from (%lf, %lf, %lf) to (%lf, %lf, %lf)\n", lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]);

  liblas::Header saved_header;

  TempDirectory scratch_dir("sorting", false);

  // ------------------------------------------------------------
  // Reads the las files into a set of sorted chunks of points.
  
  Tictoc timer;
  unsigned int chunk_number = 0;
  unsigned int point_index = 0;
  std::vector<MortonPoint> morton_points(arguments.chunk_size);
  for (size_t i = 0; i < arguments.las_filenames.size(); ++i)
  {
    // Opens this las file.
    printf("Loading las file: %s\n", arguments.las_filenames[i].c_str());
    std::ifstream fin;
    fin.open(arguments.las_filenames[i].c_str(), std::ios::in | std::ios::binary);

    liblas::ReaderFactory f;
    liblas::Reader reader = f.CreateWithStream(fin);

    liblas::Header const& header = reader.GetHeader();
    if (i == 0)
      saved_header = header;

    // Determines if the file contains color information, or just intensity.
    // TODO: I haven't seen a LAS file with color yet, so I don't know what fields to look for.
    bool has_color = false;  // !!header.GetSchema().GetDimension("R");

    // Adds each point into the tree
    std::vector<double> point(3, 0.0);
    std::vector<double> color(3, 0.0);
    while (reader.ReadNextPoint())
    {
      const liblas::Point& p = reader.GetPoint();
      point[0] = p.GetX();
      point[1] = p.GetY();
      point[2] = p.GetZ();
      if (has_color) {
        color[0] = p.GetColor().GetRed();
        color[1] = p.GetColor().GetGreen();
        color[2] = p.GetColor().GetBlue();
      }
      else {
        color[0] = color[1] = color[2] = double(p.GetIntensity()) / 65536.0 * 255.0; // TODO: is it [0,256) or [0,1]???
      }

      // TODO: uint32_t only gives 10 bits per coord.  Consider moving up to uint64 for fixed point coordinates

      // Converts the point to fixed-point coordinates.
      uint32_t fixed_coords[3];
      fixed_coords[0] = (uint32_t)( (point[0] - lo[0]) / (hi[0] - lo[0]) * (1<<10));
      fixed_coords[1] = (uint32_t)( (point[1] - lo[1]) / (hi[1] - lo[1]) * (1<<10));
      fixed_coords[2] = (uint32_t)( (point[2] - lo[2]) / (hi[2] - lo[2]) * (1<<10));

      // Computes the Morton code of the point.
      uint32_t morton = EncodeMorton3(fixed_coords[0], fixed_coords[1], fixed_coords[2]);
      //printf("(%lf, %lf, %lf) --> 0x%x\n", point[0]-lo[0], point[1]-lo[1], point[2]-lo[2], morton);

      morton_points[point_index].morton = morton;
      morton_points[point_index].x = point[0];
      morton_points[point_index].y = point[1];
      morton_points[point_index].z = point[2];
      morton_points[point_index].r = color[0];
      morton_points[point_index].g = color[1];
      morton_points[point_index].b = color[2];

      if (++point_index == (unsigned int)arguments.chunk_size)
      {
        // We've read in one chunk's worth of points.

        // Sorts the points.
        std::sort(morton_points.begin(), morton_points.end());

        // Writes out this chunk of points.
        writePoints(morton_points, scratch_dir.getPath() / chunkFilename(0, chunk_number));
        printf("Wrote chunk %6u\n", chunk_number);

        ++chunk_number;
        point_index = 0;
      }
    }
  }

  printf("All level 0 chunks written (%u chunks).  Moving onto the merging.\n", chunk_number);

  unsigned int merging_level = 0;
  unsigned int num_chunks = chunk_number;
  unsigned int num_chunks_next = 0;
  while (true)
  {
    for (unsigned int i = 0; i + 1 < num_chunks; i += 2)
    {
      printf("Level %u, combining %u and %u.\n", merging_level, i, i+1);
      mergeChunks(scratch_dir.getPath() / chunkFilename(merging_level, i),
                  scratch_dir.getPath() / chunkFilename(merging_level, i + 1),
                  scratch_dir.getPath() / chunkFilename(merging_level + 1, i / 2));
      // TODO: remove the old files.
      ++num_chunks_next;
    }

    // Is there an extra chunk with no partner?
    if (num_chunks % 2 == 1) {
      // Just copies up the extra chunk.
      printf("Copying chunk %u from level %u\n", num_chunks - 1, merging_level);
      boost::filesystem::copy_file(scratch_dir.getPath() / chunkFilename(merging_level, num_chunks - 1),
                                   scratch_dir.getPath() / chunkFilename(merging_level + 1, num_chunks / 2));
      ++num_chunks_next;
    }

    // Finished when we hit a single chunk.
    if (num_chunks_next == 1)
      break;
    
    num_chunks = num_chunks_next;
    num_chunks_next = 0;
    ++merging_level;
  }

  float t = timer.toc();
  printf("Sorting all files took %.3f seconds\n", t);

  boost::filesystem::path result_path = scratch_dir.getPath() / chunkFilename(merging_level + 1, 0);

  // Prepares a LAS file.
  std::ofstream ofs;
  ofs.open("sorted.las", std::ios::out | std::ios::binary);
  liblas::Header header;
  header.SetScale(saved_header.GetScaleX(), saved_header.GetScaleY(), saved_header.GetScaleZ());
  liblas::SpatialReference srs = saved_header.GetSRS();
  header.SetSRS(srs);
  header.SetSchema(saved_header.GetSchema());
  // TODO: may need to copy over more stuff from the header
  liblas::Writer writer(ofs, header);

  // Writes the result into a LAS file.
  FILE* fresult = fopen(result_path.string().c_str(), "rb");
  assert(fresult);
  while (true)
  {
    MortonPoint p;
    size_t ret = fread(&p, sizeof(MortonPoint), 1, fresult);
    if (ret < 1) {
      assert(feof(fresult));
      break;
    }

    liblas::Point point;
    point.SetCoordinates(p.x, p.y, p.z);
    writer.WritePoint(point);
  }
  fclose(fresult);

  t = timer.toc();
  printf("Finished after %.3f seconds (%.1f min).\n", t, t/60.0);
  return 0;
}

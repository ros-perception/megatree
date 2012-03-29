#include <megatree/common.h>

namespace megatree {

int parseNumberSuffixed(const char* s)
{
  char *end;
  float f = strtod(s, &end);
  switch (end[0]) {
  case '\0':
    break;
  case 'k':
  case 'K':
    f *= 1e3f;
  break;
  case 'm':
  case 'M':
    f *= 1e6f;
  break;
  case 'g':
  case 'G':
    f *= 1e9f;
  break;
  default:
    fprintf(stderr, "Weird suffix (%s) on number: %s\n", end, s);
    break;
  }

  return (int)f;
}

}

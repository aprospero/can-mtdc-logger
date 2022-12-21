#include <string.h>

int stricmp(const char * str1, const char * str2)
{
  int idx;
  while (*str1 != '\0' && *str2 != '\0')
  {
    idx = toupper(*str1) - toupper(*str2);
    if (idx)
    {
      return idx;
    }
    str1++; str2++;
  }
  return toupper(*str1) - toupper(*str2);
}

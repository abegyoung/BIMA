#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int findDataVersion(ini,dir)
char *ini, *dir;
{
  int ver;
  FILE  *fp;
  char  inBuf[256], buf[256];
  char  *cp, d[80];
 
  sprintf(d,"/home/data/versiontable.%3.3s", ini);
  if((fp = fopen(d,"r")) == NULL)
  {
    return(0);
  }
 
  while(fgets(inBuf,80,fp) != NULL)
  {
    strcpy(buf,inBuf);
  }

  fclose(fp);

  if((cp = strtok(buf," \n")))
  {
    ver = atoi(cp);
  }
  else
  {
    return(0);
  }

  if((cp = strtok(NULL," \n")))
  {
    strcpy(d, cp);
  }
  else
  {
    return(0);
  }

  strcpy(dir, d);

  return(ver);
}

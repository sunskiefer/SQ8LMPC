// writes bank <n> of a fresh library (factory banks, no backup) as an SQ8L bank file: savebank <n> <out>
#include "SoundLibrary.h"
#include <cstdio>
#include <cstdlib>
using namespace sq8l;
int main(int argc,char**argv){
  if(argc<3)return 2;
  SoundLibrary lib; auto v=lib.saveBank(atoi(argv[1]));
  FILE*f=fopen(argv[2],"wb"); fwrite(v.data(),1,v.size(),f); fclose(f);
  printf("%zu bytes\n",v.size());
}

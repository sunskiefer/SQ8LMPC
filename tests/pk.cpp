#include <cstdio>
#include <cstring>
extern "C" {
#include "engine.h"
}
int main(int argc,char**argv){
  const mpc_engine_t*E=mpc_engine(); void*I=E->create(argv[1]); char b[256]; int16_t o[256];
  E->render(I,o,128);
  E->get_param(I,"pack",b,256); printf("pack0 %s\n",b);
  E->set_param(I,"b_load","1"); E->render(I,o,128);
  E->get_param(I,"pack",b,256); printf("pack1 %s\n",b);
  for(int c=0;c<44;c++){char k[16];snprintf(k,16,"c0_%02d",c);E->get_param(I,k,b,256);} 
  E->destroy(I);}

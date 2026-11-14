#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
extern "C" {
#include "engine.h"
}
int main(int argc,char**argv){
  const mpc_engine_t*E=mpc_engine(); void*I=E->create(argv[1]); char b[256]; int16_t o[256];
  for(int i=0;i<4;i++)E->render(I,o,128);
  E->set_param(I,"program","3"); E->set_param(I,"pg_filt","1");E->set_param(I,"pg_filt","0");
  E->get_param(I,"k0_display",b,256); printf("A003 filter freq after reload: %s (expect 012)\n",b);
  E->get_param(I,"pname",b,256); printf("name %s\n",b);
  E->get_param(I,"pack",b,256); printf("pack %s\n",b);
  E->set_param(I,"b_load","1");E->set_param(I,"b_load","0");
  E->get_param(I,"pack",b,256); printf("after LOAD pack %s\n",b);
  E->destroy(I);return 0;}

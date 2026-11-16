// engine ABI: creates an instance on <dir>, renders, prints the two display lines and output peak
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
extern "C" {
#include "engine.h"
}
int main(int argc,char**argv){
  const mpc_engine_t*E=mpc_engine(); void*I=E->create(argv[1]); int16_t o[256]; int peak=0;
  uint8_t on[3]={0x90,60,100}; E->midi(I,on,3);
  for(int i=0;i<400;i++){E->render(I,o,128); for(int k=0;k<256;k++) if(abs(o[k])>peak) peak=abs(o[k]);}
  char b[64]; std::string rows[2];
  for(int r=0;r<2;r++) for(int c=0;c<44;c++){ char key[16]; snprintf(key,sizeof key,"c%d_%02d",r,c); b[0]=0; E->get_param(I,key,b,sizeof b); int fr=int(atof(b)*271.0+0.5); const char*cs=" \"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`{|}~"; int g=fr/4; rows[r]+= (g>=0&&g<(int)strlen(cs))?cs[g]:'?'; }
  printf("row0 [%s]\nrow1 [%s]\npeak %d\n",rows[0].c_str(),rows[1].c_str(),peak);
  E->set_param(I,"pg_filt","1"); E->set_param(I,"pg_filt","0"); E->render(I,o,128);
  E->set_param(I,"save","1"); E->set_param(I,"save","0"); for(int i=0;i<50;i++)E->render(I,o,128);
  E->destroy(I); printf("destroyed ok\n");
}

// prints "key value" for every key listed on stdin, after a scripted scenario (engine ABI, runs under qemu-arm)
#include <cstdio>
#include <cstring>
#include <string>
extern "C" {
#include "engine.h"
}
int main(int argc,char**argv){
  const mpc_engine_t*E=mpc_engine(); void*I=E->create(argv[1]); int16_t o[256];
  auto run=[&](int n){for(int i=0;i<n;i++)E->render(I,o,128);};
  auto set=[&](const char*k,const char*v){E->set_param(I,k,v);};
  auto press=[&](const char*k){set(k,"1");set(k,"0");run(1);};
  run(300);   // lets the welcome message time out
  set("program","256"); run(2); press("pg_filt"); set("k0","0.62"); set("k1","0.30"); run(2);
  char line[256],b[512];
  while(fgets(line,sizeof line,stdin)){ line[strcspn(line,"\n")]=0; if(!*line)continue; b[0]=0; E->get_param(I,line,b,sizeof b); printf("%s\t%s\n",line,b);}
  E->destroy(I);}

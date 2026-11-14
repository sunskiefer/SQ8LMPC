// offline driver for the engine ABI (runs the ARMv7 build under qemu-arm)
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
extern "C" {
#include "engine.h"
}
static const mpc_engine_t* E; static void* I;
static const char* kCh = " \"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`{|}~";
static std::string get(const char* k){char b[512]="";E->get_param(I,k,b,sizeof b);return b;}
static void set(const char* k,const char* v){E->set_param(I,k,v);}
static void press(const char* k){set(k,"1");set(k,"0");}
static void vfd(const char* title){
  printf("--- %s\n",title);
  for(int r=0;r<2;r++){std::string line;for(int c=0;c<44;c++){char k[16];snprintf(k,sizeof k,"c%d_%02d",r,c);double v=atof(get(k).c_str());int fr=(int)std::lround(v*271);int g=fr/4;line+=g<(int)strlen(kCh)?kCh[g]:'?';}printf("|%s|\n",line.c_str());}
  printf("prog %s  name '%s'  num %s%s%s%s  leds %s%s%s\n",get("program").c_str(),get("pname").c_str(),get("n0").c_str(),"","","",get("led_sync").c_str(),get("led_am").c_str(),get("led_mono").c_str());
  for(int k=0;k<10;k++){char a[8],b[16],c[16];snprintf(a,8,"k%d",k);snprintf(b,16,"k%d_name",k);snprintf(c,16,"k%d_display",k);printf(" %s [%s] %s = %s\n",a,get(b).c_str(),get(a).c_str(),get(c).c_str());}
}
static double peak(int n){int16_t o[256];double p=0;for(int i=0;i<n;i++){E->render(I,o,128);for(int j=0;j<256;j++)p=fmax(p,fabs(o[j]/32768.0));}return p;}
int main(int argc,char**argv){
  E=mpc_engine(); I=E->create(argv[1]);
  for(int i=0;i<10;i++)peak(10);
  vfd("start");
  set("program","256"); peak(4); vfd("program 256 (C000)");
  uint8_t on[3]={0x90,60,100}; E->midi(I,on,3); printf("peak %.3f\n",peak(300));
  press("pg_filt"); vfd("FILTER page");
  set("k0","0.80"); peak(2); vfd("after k0=0.8");
  press("b_pgdn"); vfd("page down");
  press("pg_osc1"); press("pg_wav"); vfd("wav");
  press("b_sync"); printf("sync led %s\n",get("led_sync").c_str());
  press("b_write"); peak(2); vfd("write on factory bank C"); 
  press("b_bank"); press("b_bank"); peak(2); vfd("bank A"); set("program","3"); peak(2);
  set("k0","0.3"); press("pg_filt"); set("k1","0.9"); press("b_write"); for(int i=0;i<600;i++)peak(1); vfd("written to A003");
  char buf[4096]; int n=E->get_param(I,"state",buf,sizeof buf); printf("state %d bytes: %.40s...\n",n,buf);
  std::string st(buf); set("program","10"); set("state",st.c_str()); printf("after restore program=%s name=%s\n",get("program").c_str(),get("pname").c_str());
  printf("rev %s\n",get("display_rev").c_str());
  printf("%s\n",get("program:300").c_str());
  E->destroy(I); return 0;}

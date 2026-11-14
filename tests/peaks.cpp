#include "Synth.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace sq8l;
int main(){
  Synth s(44100.f); s.setOutputCut(0);
  std::vector<float> L(128),R(128); std::vector<double> pk;
  for(int p=256;p<512;p++){ if(p>=384+40||(p>=256+128&&p<384))continue; if(p>=256+128&&p<384)continue;
    s.setProgram(p);
    int keys[6]={48,55,60,64,67,72};
    for(int k:keys){RawMidiEvent on{0,{0x90,(uint8_t)k,127},0};s.processEvents(&on,1);}
    float peak=0;for(int b=0;b<700;b++){s.process(L.data(),R.data(),128);for(int i=0;i<128;i++)peak=fmaxf(peak,fmaxf(fabsf(L[i]),fabsf(R[i])));}
    for(int k:keys){RawMidiEvent off{0,{0x80,(uint8_t)k,0},0};s.processEvents(&off,1);}
    for(int b=0;b<300;b++)s.process(L.data(),R.data(),128);
    pk.push_back(peak);}
  std::sort(pk.begin(),pk.end());
  printf("n=%zu min %.3f median %.3f p90 %.3f max %.3f; over 1.0: ",pk.size(),pk[0],pk[pk.size()/2],pk[pk.size()*9/10],pk.back());
  int over=0;for(double v:pk)if(v>1.0)over++;printf("%d\n",over);}

#include "Synth.h"
#include <cstdio>
#include <cmath>
#include <vector>
using namespace sq8l;
int main(int argc,char**argv){
  Synth s(44100.f);
  int progs[]={256,257,300,384,400,0};
  for(int p:progs){
    s.setProgram(p);
    RawMidiEvent on{0,{0x90,60,100},0};
    s.processEvents(&on,1);
    float peak=0;double sum=0;std::vector<float>L(128),R(128);
    for(int b=0;b<400;b++){s.process(L.data(),R.data(),128);for(int i=0;i<128;i++){peak=fmaxf(peak,fabsf(L[i]));sum+=L[i]*L[i];}}
    RawMidiEvent off{0,{0x80,60,0},0};s.processEvents(&off,1);
    for(int b=0;b<200;b++)s.process(L.data(),R.data(),128);
    printf("prog %d '%s' peak %.4f rms %.5f\n",p,s.programName().c_str(),peak,sqrt(sum/(400*128)));
  }
}

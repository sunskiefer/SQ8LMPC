// Renders a fixed scenario to raw 16-bit PCM: pcm <out.raw> [SQ80.ROM] [bankD.8XB]
// Built against the 0.1.0 objects (no ROM arg) it gives the reference; the file-ROM build must match it byte for byte.
#include "Synth.h"
#include "SoundLibrary.h"
#ifdef LOAD_ROM
#include "WaveRom.h"
#endif
#include <cstdio>
#include <cstdint>
#include <vector>
using namespace sq8l;
static std::vector<uint8_t> rd(const char*p){std::vector<uint8_t>v;FILE*f=fopen(p,"rb");if(!f)return v;int c;while((c=fgetc(f))!=EOF)v.push_back(uint8_t(c));fclose(f);return v;}
int main(int argc,char**argv){
  if(argc<2)return 2;
#ifdef LOAD_ROM
  if(argc>2){auto r=rd(argv[2]); if(!loadWaveRom(r.data(),r.size())){fprintf(stderr,"rom not loaded\n");}}
  fprintf(stderr,"rom loaded: %d\n",(int)waveRomLoaded());
#endif
  Synth s(44100.f);
  if(argc>3){auto b=rd(argv[3]); fprintf(stderr,"bank D file: %d\n",(int)s.library().loadBank(b.data(),b.size(),3));}
  FILE*o=fopen(argv[1],"wb");
  int progs[]={256,257,300,330,383,384,400,423,0,5};
  std::vector<float>L(128),R(128); std::vector<int16_t> pcm;
  for(int p:progs){
    s.setProgram(p);
    int notes[]={48,60,67}; for(int n:notes){RawMidiEvent on{0,{0x90,(uint8_t)n,100},0}; s.processEvents(&on,1);}
    for(int b=0;b<300;b++){s.process(L.data(),R.data(),128);for(int i=0;i<128;i++){pcm.push_back(int16_t(L[i]*32767));pcm.push_back(int16_t(R[i]*32767));}}
    for(int n:notes){RawMidiEvent off{0,{0x80,(uint8_t)n,0},0}; s.processEvents(&off,1);}
    for(int b=0;b<150;b++){s.process(L.data(),R.data(),128);for(int i=0;i<128;i++){pcm.push_back(int16_t(L[i]*32767));pcm.push_back(int16_t(R[i]*32767));}}
    fprintf(stderr,"prog %d %s\n",p,s.programName().c_str());
  }
  fwrite(pcm.data(),2,pcm.size(),o); fclose(o); return 0;
}

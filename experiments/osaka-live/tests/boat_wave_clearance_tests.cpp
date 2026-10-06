#include "kit/boat-on-water.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void require(bool b,const char* text){if(!b)throw std::runtime_error(text);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 Gpu gpu;Score score;Audio audio;Schedule schedule(1);std::vector<std::unique_ptr<Canvas>> canvases;
 Ctx c{gpu,0,audio,&score,1,&canvases,&schedule};double gap=0,maxLift=0,escape=0;
 if(argc>1 && std::string(argv[1])=="--gull"){
  BoatOnWaterParametersV1 p;p.gullVisits=true;schedule.print.gull.start=8;schedule.print.gull.duration=26;schedule.print.gullNext=50;
  c.t=11;const auto circling=BoatOnWaterV1::gullPose(c,p);require(circling.alpha>0 && !circling.landed,"missing circling approach");
  c.t=17;const auto perched=BoatOnWaterV1::gullPose(c,p);require(perched.landed && perched.alpha==1,"gull does not land");
  schedule.advance(c.t,{},score);require(BoatOnWaterV1::gullPose(c,p).landed,"quiet music ejects gull");
  Audio loud;loud.bassLevel=.5;loud.bands.fill(.4);score.bassHits={{17,.9,999}};schedule.advance(17,loud,score);
  require(schedule.print.gullTakeoff==17,"loud hit does not launch gull");c.t=18;
  const auto flight=BoatOnWaterV1::gullPose(c,p);require(!flight.landed && flight.at.y<perched.at.y-60,"takeoff is not visible");
  c.t=22;require(BoatOnWaterV1::gullPose(c,p).alpha==0,"gull fails to leave");
  std::cout<<"PASS: circling, attached bow landing, quiet perch, loud-hit takeoff and complete departure\n";return 0;
 }
 for(bool right:{false,true})for(int seed:{71,137,211}){
  BoatOnWaterParametersV1 p;p.ridesWave=true;p.x=right?1260:660;p.row=7.6;p.scale=.66;p.length=315;
  p.wave.baseHeight=340;p.wave.maxRise=650;p.wave.anchorRight=right;p.wave.x=right?1920:0;p.wave.seed=seed;
  schedule.print.wave={0,36,1,0};
  for(int i=0;i<=36*60;++i){c.t=i/60.;const auto b=BoatOnWaterV1::pose(c,p);const auto field=GreatWaveV1::field(c,p.wave);
   const auto set=GreatWaveV1::pose(c,p.wave);const double side=right?-1:1;
   double lip=-1e9,crest=1e9;for(int j=16;j<48;++j){lip=std::max(lip,side*field.face[j].x);crest=std::min(crest,field.face[j].y);}
   if(set.height>220){
    require(side*b.at.x-p.length*p.scale*.60>lip+20,"hull enters curl/lip exclusion");
    if(set.height>500)require(b.at.y-35*p.scale>crest+30,"hull floats above towering crest");
   }
   const double keel=b.at.y+26*p.scale*std::cos(b.tilt),surface=BoatOnWaterV1::surfaceY(c,p,b.at.x,b.row);
   gap=std::max(gap,surface-keel);if(surface-keel>=26){std::cerr<<"t "<<c.t<<" gap "<<surface-keel<<" x "<<b.at.x<<" y "<<b.at.y<<" tilt "<<b.tilt<<" height "<<set.height<<" right "<<right<<"\n";throw std::runtime_error("hull floats above local wave surface");}
   auto sea=p;sea.ridesWave=false;maxLift=std::max(maxLift,BoatOnWaterV1::pose(c,sea).at.y-b.at.y);
   escape=std::max(escape,std::abs(b.at.x-p.x));
  }
 }
 require(maxLift<190 && escape>250,"boat fails believable-height escape");
 std::cout<<"PASS: full-hull curl exclusion, below crest, keel contact; max gap "<<gap<<", lift "<<maxLift<<", escape "<<escape<<" px\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

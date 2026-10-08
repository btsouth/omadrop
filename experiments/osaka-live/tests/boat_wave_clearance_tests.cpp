#include "kit/boat-on-water.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
using namespace Journey;using namespace Journey::Kit;
void require(bool b,const char* text){if(!b)throw std::runtime_error(text);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 Gpu gpu;Score score;Audio audio;Schedule schedule(1);std::vector<std::unique_ptr<Canvas>> canvases;
 Ctx c{gpu,0,audio,&score,1,&canvases,&schedule};double gap=0,escape=0;
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
 WaveTrainParametersV2 params;params.groupPeriod=1800;params.groupWidth=1500;params.groupFloor=.94;params.heightScale=.88;params.waterline=965;params.surgeEnabled=true;params.travelScale=.2;
 Schedule::waveTrainParameters=std::make_shared<WaveTrainParametersV2>(params);
 double minLip=1e9,minCrest=1e9,maxContact=0,maxStep=0,minTilt=1e9,maxTilt=-1e9,minEdge=1e9;
 for(double row:{5.6,7.6})for(int seed:{71,137,211}) {
  BoatOnWaterParametersV1 p;p.ridesWave=true;p.ridesWaveTrain=true;p.waveTrain=params;p.wave.row=params.row;
  p.x=row<7?1140:660;p.row=row;p.scale=row<7?.5:.66;p.length=315;p.seed=seed;
  p.swell.amplitude=.52;p.swell.amplitudeGain=3.5;p.swell.bandGain=1.15;p.swell.liftGain=.5;p.swell.kickGain=.35;p.swell.surgeEnabled=true;
  schedule=Schedule(1);StaticGeometry geometry;c.staticGeometry=&geometry;score=Score{};V2 previous;bool first=true;
  for(int i=1;i<=90*60;++i){c.t=i/60.;audio.bands.fill(.05+.38*(.5+.5*std::sin(c.t*.19)));audio.bassLevel=audio.bands[0];
   audio.surge=sstep(18,19.8,c.t)*(1-sstep(23,29,c.t));score.advance(audio,c.t,1/60.);schedule.advance(c.t,audio,score);c.a=audio;
   const auto b=BoatOnWaterV1::pose(c,p);const auto& field=WaveTrainV2::worldProfile(c,params);
   const double radius=std::hypot((p.length*.5+26)*p.scale,26*p.scale);
   minEdge=std::min(minEdge,std::min(b.at.x-radius,1920-b.at.x-radius));
   require(minEdge>=16-1e-6,"hull clips screen edge");
   if(row>7){minTilt=std::min(minTilt,b.tilt);maxTilt=std::max(maxTilt,b.tilt);}
   for(const auto& crest:field.crests)if(crest.stage>1.15){double left=1e9,top=1e9;for(auto q:crest.boundary){left=std::min(left,q.x);top=std::min(top,q.y);}
    const double right=WaveTrainV2::exclusionRight(crest),half=p.length*p.scale*.60;
    const double margin=std::max(b.at.x-half-right,left-(b.at.x+half));minLip=std::min(minLip,margin);
    require(margin>20,"full hull enters lip, claws, curl or barrel exclusion");
    if(crest.stage>4){const double marginY=b.at.y-35*p.scale-top;minCrest=std::min(minCrest,marginY);require(marginY>30,"hull floats above towering crest");}
   }
   double contact=1e9;
   for(int j=0;j<=32;++j){const auto q=BoatOnWaterV1::keel(p,j/32.);const V2 at=b.at+V2(q.x*std::cos(b.tilt)-q.y*std::sin(b.tilt),q.x*std::sin(b.tilt)+q.y*std::cos(b.tilt));
    const double difference=BoatOnWaterV1::surfaceY(c,p,at.x,b.row)-at.y;
    require(difference>=-1e-5,"keel passes through shared water surface");contact=std::min(contact,difference);
    if(j==16){gap=std::max(gap,difference);if(difference>=26)std::cerr<<"t "<<c.t<<" row "<<row<<" seed "<<seed<<" gap "<<difference<<" x "<<b.at.x<<" y "<<b.at.y<<"\n";}
   }
   maxContact=std::max(maxContact,std::abs(contact));require(std::abs(contact)<1e-5,"hull loses shared water contact");require(gap<26,"hull floats above local surface");
   escape=std::max(escape,b.at.x-p.x);
   if(!first){const double step=(b.at-previous).len();if(step>8){std::cerr<<"t "<<c.t<<" row "<<row<<" seed "<<seed<<" step "<<step<<" previous "<<previous.x<<","<<previous.y<<" next "<<b.at.x<<","<<b.at.y<<" stage "<<schedule.waveTrain.pose().stage<<"\n";throw std::runtime_error("boat escape jumps between sets");}maxStep=std::max(maxStep,step);}previous=b.at;first=false;
  }
 }
 require(minTilt<-.02 && maxTilt>.02,"near boat fails to rock both ways");
 std::cout<<"near tilt "<<minTilt*180/Pi<<" to "<<maxTilt*180/Pi<<" deg, conservative edge margin "<<minEdge<<" px\n";
 require(escape>250,"boats fail to escape tall travelling sets");
 std::cout<<"max step "<<maxStep<<" px\n";
 require(maxStep<8,"boat escape jumps between sets");
 std::cout<<"PASS: W2d full-hull/detail exclusion, below crest, keel contact, continuous escape; min lip "<<minLip<<", crest "<<minCrest<<", max gap "<<gap<<", contact "<<maxContact<<", escape "<<escape<<", step "<<maxStep<<" px\n";

}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

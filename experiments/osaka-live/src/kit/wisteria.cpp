#include "onset.h"
#include "wisteria.h"
#include "palette.h"
#include "primitives.h"
#include "pane.h"
#include "haze.h"
#include "../rig.h"
#include "../osaka_shaders.h"
#include <cmath>
#include <vector>

namespace Journey::Kit {
using Life = OsakaEventState;
void OsakaWisteriaV1::draw(Ctx& c, const OsakaState& s, const Life& L) {
    GpuProfile::Group profileGroup(c.gpu.profile,"wisteria");
    if (s.wisteria <= 0) return;
    const double t = c.t, ox = -s.cam * 1.25;
    // Past the quay the whole canopy has left the screen. Bound crowns,
    // 420 px racemes, their wind displacement and the petal halos before
    // generating the many small live paths / blur layers.
    const double swayBound = 18.7 * (1 + 2.5 * std::abs(L.wind)) + 26 * std::abs(L.wind) + 2 * std::abs(c.a.bass);
    if (std::max(2034.0, 1970.0 + swayBound) + ox < -24) return;
    Canvas& cv = c.canvas();
    Canvas& e = c.canvas();
    cv.batchSpatially=true;cv.preserveRaster=e.preserveRaster=true;
    const Col dark(0.010f, 0.050f, 0.037f);
    struct Petal {double f,dx,y,r;Col color,light;bool glow;double rotation;Canvas::PreparedEllipse shape;};
    struct Raceme {double x,length;std::vector<Petal> petals;};
    struct Canopy {std::vector<std::array<double,3>> crowns;std::vector<Raceme> racemes;};
    auto makeCanopy=[&] {
        Canopy canopy;
        Rng rng(88);
        auto crown=[&](double x,double y,double r){canopy.crowns.push_back({x,y,r});};
        for(int i=0;i<15;++i)crown(1560+rng.uni()*400,rng.uni()*30-14,34+rng.uni()*40);
        std::vector<double> xs(30);
        for(double& x:xs)x=1575+std::pow(rng.uni(),0.85)*370;
        std::sort(xs.begin(),xs.end());
        for(int i=0;i<30;++i) {
            const double xw=xs[i];
            const double ln=(90+rng.uni()*120)+210*std::pow((xw-1575)/370,1.4)*(0.6+0.4*rng.uni());
            const double wmax=9+rng.uni()*5;
            const int n=int(ln/5);
            Raceme raceme{xw,ln,{}};
            for(int j=0;j<n;++j) {
                const double f=double(j)/n;
                const double wd=wmax*std::sin(std::min(1.0,f*2.6)*Pi/2)*std::pow(1-f,0.6)+0.8;
                for(int side=-1;side<=1;side+=2) {
                    const double dx=side*wd*(0.35+0.65*rng.uni()),y=6+ln*f+rng.normal()*1.6;
                    const double r=2.3+2.0*(1-f)*rng.uni();
                    const Col tone=mix(Col(0.03f,0.26f,0.18f),MINT,std::pow(f,1.3)*(0.6+0.4*rng.uni()));
                    const Col color=mix(dark,tone,0.35+0.65*f);
                    const bool glow=f>0.45 && rng.uni()<0.5;
                    const Col light=glow?(rng.uni()<0.88?MINT:MAG):MINT;
                    raceme.petals.push_back({f,dx,y,r,color,light,glow,side*0.35,Canvas::prepareEllipse(r*0.8,r*1.6,side*0.35,c.gpu.pixelScale())});
                }
            }
            canopy.racemes.push_back(std::move(raceme));
        }
        return canopy;
    };
    Canopy temporary;
    Canopy* canopy=nullptr;
    if(c.staticGeometry) {
        auto& layout=c.staticGeometry->layouts["wisteria"];
        if(!layout.has_value())layout=makeCanopy();
        canopy=&std::any_cast<Canopy&>(layout);
    } else {temporary=makeCanopy();canopy=&temporary;}
    for(const auto& crown:canopy->crowns)cv.disc(crown[0]+ox,crown[1],crown[2],dark);
    const double glint=1+0.9*onsetFlash(c,6);
    for(std::size_t i=0;i<canopy->racemes.size();++i) {
        const auto& raceme=canopy->racemes[i];
        const double x=raceme.x+ox,ln=raceme.length;
        const double sway=std::sin(t*0.9+i*0.7)*(4+ln*0.035)*(1+2.5*L.wind)
                          +L.wind*26*(0.7+0.3*std::sin(t*2.7+i))+2.0*c.a.bass;
        cv.line(x,-10,x+sway*0.1,ln*0.3,1.6,dark);
        for(const auto& p:raceme.petals) {
            const double px=(x+sway*p.f*p.f)+p.dx;
            cv.color(p.color);
            if(c.staticGeometry)cv.fillEllipsePrepared(px,p.y,p.shape);else {cv.ellipse(px,p.y,p.r*0.8,p.r*1.6,p.rotation);cv.fill();}
            if(p.glow)e.glow(px,p.y,p.r*2.4,p.light,std::min(1.0,0.55*p.f*glint));
        }
    }
    c.gpu.over(cv, 1, 1.3f);
    if (!e.empty()) {
        // Both original passes used the same resolved live petal image.
        const int petals = c.gpu.layer(e);
        const int soft = c.gpu.blurred(petals, 1.0f);
        c.gpu.composite(soft, Blend::Over, 1.15f);
        const int bloom = c.gpu.blurred(petals, 12);
        c.gpu.composite(bloom, Blend::Add, 0.35f);
    }
}

}

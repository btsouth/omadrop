#include "render_comparison.h"
#include <iostream>

int main() {
    std::vector<unsigned char> a(128, 100), b=a;
    int failures=0;
    auto check=[&](bool ok,const char* name) {
        std::cout<<(ok ? "PASS: " : "FAIL: ")<<name<<'\n'; failures+=!ok;
    };
    auto diff=Journey::compareRender(a,b);
    check(diff.exact() && diff.acceptable(), "identical channels are exact");
    for(int i=0;i<64;++i) b[i]+=i%2 ? 1 : -1;
    diff=Journey::compareRender(a,b);
    check(!diff.exact() && diff.changed==64 && diff.maximum==1 && diff.acceptable(),
          "64 channels at plus or minus 1 are accepted");
    b[64]+=1; diff=Journey::compareRender(a,b);
    check(diff.changed==65 && !diff.acceptable(), "65 changed channels fail");
    b=a; b[0]+=2; diff=Journey::compareRender(a,b);
    check(diff.changed==1 && diff.maximum==2 && !diff.acceptable(), "one channel at 2 fails");
    b=a; b.pop_back(); diff=Journey::compareRender(a,b);
    check(!diff.exact() && !diff.acceptable(), "different buffer sizes fail");
    return failures ? 1 : 0;
}

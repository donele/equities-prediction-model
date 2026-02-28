#include <gtlib_gui/BidAskFrame.h>
#include <TApplication.h>

int main(int argc, char **argv) {
    TApplication theApp("App",&argc,argv);
    new BidAskFrame(gClient->GetRoot(), 800, 400);
    theApp.Run();
    return 0;
}


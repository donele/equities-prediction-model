#include <gtlib_gui/MyMainFrame.h>
#include <TApplication.h>

void example()
{
    new MyMainFrame(gClient->GetRoot(), 200, 200);
}

int main(int argc, char **argv) {
    TApplication theApp("App",&argc,argv);
    example();
    theApp.Run();
    return 0;
}


#include "ofApp.h"
#include "ofMain.h"

//========================================================================
int main() {

  // Use ofGLFWWindowSettings for more options like multi-monitor fullscreen
  ofGLWindowSettings settings;
  settings.setSize(800, 600);
  settings.windowMode = OF_WINDOW; // can also be OF_FULLSCREEN

  auto window = ofCreateWindow(settings);

  ofRunApp(window, make_shared<ofApp>());
  ofRunMainLoop();
}

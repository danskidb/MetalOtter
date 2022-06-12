// UIApplicationMain()

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
//#import "Otter.hpp"

int main(int argc, char * argv[])
{
  @autoreleasepool {
      return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}

//class SandboxApp : public Otter::Application
//{
//public:
//    SandboxApp();
//
//    void OnStart() override;
//    void OnTick(float deltaTime) override;
//    void OnStop() override;
//};
//
//Otter::Application* Otter::CreateApplication()
//{
//    return new SandboxApp();
//}

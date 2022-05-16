#include "Otter/Core/Application.hpp"
#include <chrono>

namespace Otter {

	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run(int argc, char* argv[], char* envp[])
	{
#ifdef __APPLE__
        NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

        MyAppDelegate del;

        NS::Application* pSharedApplication = NS::Application::sharedApplication();
        pSharedApplication->setDelegate( &del );
        pSharedApplication->run();

        pAutoreleasePool->release();
#endif

		OnStart();

		float dt = 0.0f;
		while (true) {
			auto startTime = std::chrono::high_resolution_clock::now();

			OnTick(dt);

			auto stopTime = std::chrono::high_resolution_clock::now();
			dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
		}

		OnStop();
	}

#ifdef __APPLE__
    MyAppDelegate::~MyAppDelegate()
    {
    }

    NS::Menu* MyAppDelegate::createMenuBar()
    {
        NS::Menu* pMainMenu = NS::Menu::alloc()->init();
        NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
        NS::Menu* pAppMenu = NS::Menu::alloc()->init( NS::String::string( "Appname", NS::StringEncoding::UTF8StringEncoding ) );

        NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
        NS::String* quitItemName = NS::String::string( "Quit ", NS::StringEncoding::UTF8StringEncoding )->stringByAppendingString( appName );
        SEL quitCb = NS::MenuItem::registerActionCallback( "appQuit", [](void*,SEL,const NS::Object* pSender){
            auto pApp = NS::Application::sharedApplication();
            pApp->terminate( pSender );
        } );

        NS::MenuItem* pAppQuitItem = pAppMenu->addItem( quitItemName, quitCb, NS::String::string( "q", NS::StringEncoding::UTF8StringEncoding ) );
        pAppQuitItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
        pAppMenuItem->setSubmenu( pAppMenu );

        pMainMenu->addItem( pAppMenuItem );

        pAppMenuItem->release();
        pAppMenu->release();

        return pMainMenu->autorelease();
    }

    void MyAppDelegate::applicationWillFinishLaunching( NS::Notification* pNotification )
    {
        NS::Menu* pMenu = createMenuBar();
        NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
        pApp->setMainMenu( pMenu );
        pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
    }

    void MyAppDelegate::applicationDidFinishLaunching( NS::Notification* pNotification )
    {
        NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
        pApp->activateIgnoringOtherApps( true );
    }

    bool MyAppDelegate::applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender )
    {
        return true;
    }
#endif
}
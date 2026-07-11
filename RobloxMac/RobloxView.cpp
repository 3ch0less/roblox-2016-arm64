#include "RobloxView.h"
#include "Roblox.h"
#include <cstdio>

#include "GfxBase/ViewBase.h"
#include "v8datamodel/datamodel.h"
#include "v8datamodel/workspace.h"
#include "v8datamodel/camera.h"
#include "v8datamodel/game.h"
#include "v8datamodel/InputObject.h"
#include "v8datamodel/GuiService.h"
#include "FunctionMarshaller.h"
#include "Util/StandardOut.h"
#include "Util/FileSystem.h"
#include "rbx/SystemUtil.h"
#include "rbx/Tasks/Coordinator.h"
#include "UserInput.h"
#include "Util/IMetric.h"
#include "Util/Object.h"
#include "GfxBase/RenderSettings.h"
#include "GfxBase/FrameRateManager.h"
#include "v8datamodel/BaseRenderJob.h"
#include "v8datamodel/UserController.h"
#include "v8datamodel/UserInputService.h"
#include "Util/Statistics.h"
#include "v8datamodel/ContentProvider.h"
#include "util/ContentId.h"
#include "security/SecurityContext.h"
#include "script/ScriptContext.h"
#include "v8xml/Serializer.h"
#include "rbx/CEvent.h"
#include "GameVerbs.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "Util/RunStateOwner.h"
#include "Util/NavKeys.h"
#include "Util/UserInputBase.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/StarterPlayerService.h"
#include "v8datamodel/PlayerScripts.h"
#include "v8datamodel/UserController.h"
#include "v8datamodel/ReplicatedFirst.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/GuiObject.h"
#include "v8datamodel/ScreenGui.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/Sky.h"
#include "v8datamodel/SpawnLocation.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Decal.h"
#include "Humanoid/Humanoid.h"
#include "util/RcProbe.h"
#include "util/ContentId.h"
#include "util/TextureId.h"
#include "script/CoreScript.h"
#include "script/LuaVM.h"
#include "../ClientBase/RenderSettingsItem.h"

// Offline Visit Solo: keep WASD→Humanoid + follow-cam alive for the process lifetime.
// Native path is the Phase 3 fallback; Lua ControlScript/CameraScript may take over.
static rbx::signals::scoped_connection g_offlinePlayStepConnection;
static rbx::signals::scoped_connection g_offlineScriptsLoadedConnection;


#include <boost/iostreams/copy.hpp>

#include "V8DataModel/GameBasicSettings.h"

LOGGROUP(PlayerShutdownLuaTimeoutSeconds)

// This job calls ViewBase::render(), which needs to be done exclusive to the DataModel.
// This is why it has the RBX::DataModelJob::Render enum, which prevents concurrent writes to DataModel.
// It also needs to run in the view's thread for OpenGL
// TODO: Can Ogre be modified to not require the thread?
class RobloxView::RenderJob : public RBX::BaseRenderJob
	, public RBX::IMetric
{
	RBX::FunctionMarshaller* marshaller;
	weak_ptr<RBX::DataModel> dataModel;
	RBX::ViewBase* view;
	RBX::CEvent renderEvent;

public:
	RenderJob(RBX::ViewBase* view, RBX::FunctionMarshaller* marshaller, shared_ptr<RBX::DataModel> dataModel)
		:RBX::BaseRenderJob(CRenderSettingsItem::singleton().getMinFrameRate(), CRenderSettingsItem::singleton().getMaxFrameRate(), dataModel),
		view(view),
		dataModel(dataModel),
		marshaller(marshaller),
		renderEvent(false)
	{
        cyclicExecutive = true;
    }
    

	RBX::Time::Interval sleepTime(const Stats& stats)
	{
        if (isAwake)
            return computeStandardSleepTime(stats, CRenderSettingsItem::singleton().getMaxFrameRate());
        else
            return RBX::Time::Interval::max();
	}


    static void scheduleRenderPerform(const weak_ptr<RenderJob>& selfWeak, ViewBase* view, double timeJobStart)
    {
        if (shared_ptr<RenderJob> self = selfWeak.lock())
        {
            view->renderPerform(timeJobStart);
            self->wake();
        }
    }

    void doDataModelRenderStep(DataModel* dm, const float secondsElapsed)
    {
        dm->renderStep(secondsElapsed);
        isAwake = false;
        marshaller->Execute(boost::bind(&RBX::ViewBase::renderPrepare, view, this), &renderEvent);
    }

	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
        DataModel* dm = view->getDataModel();
		if (!dm)
			return RBX::TaskScheduler::Done;

        double timeJobStart = RBX::Time::nowFastSec();
        try
        {
            const float secondsElapsed = view->getFrameRateManager()->GetFrameTimeStats().getLatest() / 1000.f;
            lastRenderTime = RBX::Time::now<RBX::Time::Fast>();
            // TODO: Can we fix Ogre so that it can be called from this thread, rather than marshalled?
            
            RBX::DataModel::scoped_write_request request(dm);
            doDataModelRenderStep(dm, secondsElapsed);
            
            marshaller->Submit(boost::bind(&scheduleRenderPerform, weak_from(this), view, timeJobStart));
        }
        catch (std::exception& e)
        {
            RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
        }
        return RBX::TaskScheduler::Stepped;
	}
	
	void abortRender()
	{
		renderEvent.Set();
	}

	// IMetric 
	/*override*/ double getMetricValue(const std::string& metric) const
	{
        RBX::FrameRateManager* frm = view ? view->getFrameRateManager() : 0;

		if (metric == "Render FPS") {
			return averageStepsPerSecond();
		}

		if (metric == "Render Duty") {
			return averageDutyCycle();
		}
		
		if (metric == "Render Nominal FPS") {
			return frm ? 1000.0 / frm->GetRenderTimeAverage() : 0.0;
		}

		if(metric == "Delta Between Renders")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "Total Render")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "Present Time")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "GPU Delay")
		{
			return view->getMetricValue(metric);
		}
		else if (metric == "Video Memory")
        {
			return (double)RBX::SystemUtil::getVideoMemory();
		}
		
		return 0.0;
	}


	/*override*/ std::string getMetric(const std::string& metric) const
	{
		if (! view ) {
			return "No View";
		}

		if (metric == "Graphics Mode") {
			return ""; // RBX::Reflection::EnumDesc<CRenderSettings::GraphicsMode>::singleton().convertToString(graphicsMode);
		}

		RBX::FrameRateManager* frm = view ? view->getFrameRateManager() : 0;


		if (metric == "FRM") {
			return (frm && frm->IsBlockCullingEnabled()) ? "On" : "Off";
		}

		if (metric == "Anti-Aliasing") {
			return (frm && frm->getAntialiasingMode() == RBX::CRenderSettings::AntialiasingOn) ? "On" : "Off";
		}

		RBXASSERT(0);
		return "";
	}
};

 class RobloxView::UserInputJob : public RBX::DataModelJob
{
	RobloxView* wnd;
	RBX::DataModel* dataModel;
public:
	UserInputJob(RobloxView* wnd, shared_ptr<RBX::DataModel> dataModel)
		:RBX::DataModelJob("UserInput", RBX::DataModelJob::Write, true, dataModel, RBX::Time::Interval(0)),
		wnd(wnd),
		dataModel(dataModel.get())
	{
	}
	
	RBX::Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, 60);
	}

	virtual Job::Error error(const Stats& stats)
	{
		Job::Error result = computeStandardError(stats, 60);
		return result;
	}

	virtual RBX::TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		RBX::DataModel::scoped_write_request request(dataModel);
		if(wnd)
			wnd->processInput();
		return RBX::TaskScheduler::Stepped;
	}

};

static RBX::InputObject::UserInputType viewEventToUserInputType(RobloxView::EventType event)
{
    switch (event)
	{
		case RobloxView::MOUSE_MOVE:
			return RBX::InputObject::TYPE_MOUSEMOVEMENT;
		case RobloxView::MOUSE_LEFT_BUTTON_DOWN:
        case RobloxView::MOUSE_LEFT_BUTTON_UP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON1;
		case RobloxView::MOUSE_RIGHT_BUTTON_DOWN:
		case RobloxView::MOUSE_RIGHT_BUTTON_UP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON2;
		case RobloxView::MOUSE_MIDDLE_BUTTON_DOWN:
		case RobloxView::MOUSE_MIDDLE_BUTTON_UP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON3;
		case RobloxView::KEY_UP:
		case RobloxView::KEY_DOWN: // intentional fall thru
			return RBX::InputObject::TYPE_KEYBOARD;
		default:
			return RBX::InputObject::TYPE_NONE;
	}
}

static RBX::InputObject::UserInputState viewEventToUserInputState(RobloxView::EventType event)
{
    switch (event)
	{
		case RobloxView::MOUSE_MOVE:
			return RBX::InputObject::INPUT_STATE_CHANGE;
		case RobloxView::MOUSE_LEFT_BUTTON_DOWN:
		case RobloxView::MOUSE_RIGHT_BUTTON_DOWN:
		case RobloxView::MOUSE_MIDDLE_BUTTON_DOWN:
        case RobloxView::KEY_DOWN: // intentional fall thru
            return RBX::InputObject::INPUT_STATE_BEGIN;
        case RobloxView::MOUSE_LEFT_BUTTON_UP:
		case RobloxView::MOUSE_RIGHT_BUTTON_UP:
		case RobloxView::MOUSE_MIDDLE_BUTTON_UP:
        case RobloxView::KEY_UP: // intentional fall thru
			return RBX::InputObject::INPUT_STATE_END;
		default:
			return RBX::InputObject::INPUT_STATE_NONE;
	}
}

void RobloxView::processInput()
{
	/*if (userInput && userInput.get())
		if(userInput->getWrapMode() == RBX::UserInputBase::WRAP_HYBRID)
			userInput->ProcessUserInputMessage(RBX::UIEvent::NO_EVENT,0,0);*/
}

// Request a shutdown of the client
Boolean RobloxView::requestShutdownClient()
{
	if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
	{
        // give scripts a deadline to finish
        if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
            if (ScriptContext* scriptContext = game->getDataModel()->find<ScriptContext>())
                scriptContext->setTimeout(FLog::PlayerShutdownLuaTimeoutSeconds);
    
		DataModel::RequestShutdownResult requestResult = dataModel->requestShutdown();
		return requestResult == DataModel::CLOSE_REQUEST_HANDLED;
	}	
	
	return true;
}

void RobloxView::handleUserMessage(EventType event, unsigned int wParam, unsigned int lParam)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(viewEventToUserInputType(event), viewEventToUserInputState(event), wParam, lParam);
    }
}

void RobloxView::handleFocus(bool focus)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(RBX::InputObject::TYPE_FOCUS, focus ? RBX::InputObject::INPUT_STATE_BEGIN : RBX::InputObject::INPUT_STATE_END, 0, 0);
    }
}

void RobloxView::handleMouseInside(bool inside)
{
	if (userInput)
	{
		if (inside)
			userInput->onMouseInside();
		else
			userInput->onMouseLeave();
	}
}

void RobloxView::handleMouse(EventType event, int x, int y, unsigned int modifiers)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(viewEventToUserInputType(event), viewEventToUserInputState(event), modifiers, MAKEXYLPARAM((uint) x, (uint) y));
    }
}

void RobloxView::handleKey(EventType event, RBX::KeyCode keyCode, RBX::ModCode modifiers)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(viewEventToUserInputType(event), viewEventToUserInputState(event), keyCode, modifiers);
    }
}

void RobloxView::handleScrollWheel(float delta, int x, int y)
{
	if (userInput)
    {
        userInput->PostUserInputMessage(RBX::InputObject::TYPE_MOUSEWHEEL, RBX::InputObject::INPUT_STATE_CHANGE, delta, MAKEXYLPARAM((uint) x, (uint) y));
    }
}

void RobloxView::leaveGame()
{
	marshaller->Submit(boost::bind(&RobloxView::handleLeaveGame, this));
}

void RobloxView::handleLeaveGame()
{
	Roblox::handleLeaveGame(appWindow);
}

void RobloxView::shutdownClient()
{
	marshaller->Submit(boost::bind(&RobloxView::handleShutdownClient, this));
}

void RobloxView::handleShutdownClient()
{
	Roblox::handleShutdownClient(appWindow);
}

void RobloxView::toggleFullScreen()
{
    if(getDataModel())
        getDataModel()->submitTask(boost::bind(&RobloxView::handleToggleFullScreen, this), RBX::DataModelJob::Write);
}

void RobloxView::handleToggleFullScreen()
{
	Roblox::handleToggleFullScreen(appWindow);
	RBX::GameBasicSettings::singleton().setFullScreen(Roblox::inFullScreenMode(appWindow));
}


extern std::string macBundlePath();

static RBX::ViewBase* createGameWindow(void *wnd)
{
	RC_PROBE("createGameWindow: InitPluginModules...\n");
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&RBX::ViewBase::InitPluginModules, flag);
	RC_PROBE("createGameWindow: CreateView...\n");

	RBX::OSContext context;
    context.hWnd = wnd;
	context.width = 800;
	context.height = 600;

	RBX::CRenderSettings& settings = CRenderSettingsItem::singleton();
	RBX::CRenderSettings::GraphicsMode mode = RBX::CRenderSettings::OpenGL;
	RBX::ViewBase* rbxView =  RBX::ViewBase::CreateView(mode, &context, &settings);
	RC_PROBE("createGameWindow: initResources rbxView=%p...\n",(void*)rbxView);
	if (rbxView) rbxView->initResources();
	RC_PROBE("createGameWindow: done\n");

	return rbxView;
}

class DummyVerb : public RBX::Verb {
public:
	DummyVerb(VerbContainer* container, const char* name)
	:Verb(container, name)
	{
	}
	virtual bool isEnabled() const {return false;}
	
	virtual void doIt(RBX::IDataState* dataState) {}
};

RobloxView::RobloxView(void *viewwnd, void *appwnd, boost::shared_ptr<RBX::Game> game)
	:view(createGameWindow(viewwnd))
	,appWindow(appwnd)
	,viewWindow(viewwnd)
    ,userInput(new UserInput(this, game))
	,game(game)
	,leaveGameVerb(new LeaveGameVerb(this, game->getDataModel().get()))
	,fullscreenVerb(new ToggleFullscreenVerb(this, game->getDataModel().get(), NULL))
	,studioVerb(new DummyVerb(game->getDataModel().get(), "TogglePlayMode"))
	,screenshotVerb(new DummyVerb(game->getDataModel().get(), "Screenshot"))
	,shutdownClientVerb(new ShutdownClientVerb(this, game->getDataModel().get()))
	,marshaller(RBX::FunctionMarshaller::GetWindow())
{
    shared_ptr<RBX::DataModel> dataModel = game->getDataModel();
    
    placeIDChangeConnection = dataModel->propertyChangedSignal.connect(boost::bind(&RobloxView::onPlaceIDChanged, this, _1));
	
	// Bind to the Workspace
	bindToWorkspace();
	
	// Create the rendering jobs
	renderJob = shared_ptr<RenderJob>(new RenderJob(view.get(), marshaller, dataModel));
    
    // Create the user input job
    userInputJob = shared_ptr<UserInputJob>(new UserInputJob(this, dataModel));
	
	defineConcurrencyRules();

	RBX::TaskScheduler::singleton().add(renderJob);
    if (userInputJob)
    {
        RBX::TaskScheduler::singleton().add(userInputJob);
    }
}


void RobloxView::onPlaceIDChanged(const RBX::Reflection::PropertyDescriptor* desc)
{	
	bool placeIDChanged = desc->name=="PlaceId";
    
	if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
        if(placeIDChanged && dataModel->getPlaceID() > 0)
            Roblox::addBreakPadKeyValue("Place0", dataModel->getPlaceID());
}


void RobloxView::defineConcurrencyRules()
{
	{
		// Force viewUpdateJob and renderJob to happen serially
		boost::shared_ptr<RBX::Tasks::Coordinator> sequence(new RBX::Tasks::ExclusiveSequence());
		renderJob->addCoordinator(sequence);
	}

	if (CRenderSettingsItem::singleton().isSynchronizedWithPhysics)
	{
		// Force rendering and physics to happen in lock-step
		sequence.reset(new RBX::Tasks::Sequence());
		renderJob->addCoordinator(sequence);
        if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
            dataModel->create<RBX::RunService>()->getPhysicsJob()->addCoordinator(sequence);
	}
}

void RobloxView::doUnbindWorkspace()
{
    if(view)
        view->bindWorkspace(boost::shared_ptr<RBX::DataModel>());
}

void RobloxView::initializeInput()
{
    userInput.reset(new UserInput(this, game));

    if(userInput)
	{
		DataModel::LegacyLock lock(game->getDataModel(), DataModelJob::Write);
        
		ControllerService* service = ServiceProvider::create<ControllerService>(game->getDataModel().get());
		service->setHardwareDevice(userInput.get());
	}
}

void RobloxView::stopJobs()
{
    if (renderJob)
    {
        renderJob->abortRender();
        
        boost::function<void()> callback = boost::bind(&RBX::FunctionMarshaller::ProcessMessages, marshaller);
        RBX::TaskScheduler::singleton().removeBlocking(renderJob, callback);
    }
    
    if (userInputJob)
	{
		boost::function<void()> callback = boost::bind(&FunctionMarshaller::ProcessMessages, marshaller);
		TaskScheduler::singleton().removeBlocking(userInputJob, callback);
	}

    // RenderJob is sure to be completed at this point, since removeBlocking returned - but it might have marshalled
    // renderPerform asynchronously before exiting, which means that we might still have a callback that uses this view
    // in the marshaller queue.
    // This makes sure that all pending marshalled events are processed to avoid a use after free.
    marshaller->ProcessMessages();

    // All render processing is complete; it's safe to reset job pointers now
    renderJob.reset();
    userInputJob.reset();
}

void RobloxView::doShutdownDataModel()
{
    if(game)
    {
		{
			RBX::DataModel::LegacyLock lock(game->getDataModel(), RBX::DataModelJob::Write);
        
			marshaller->Submit(boost::bind(&RobloxView::doUnbindWorkspace, this));
			marshaller->ProcessMessages();
        
			if(shared_ptr<DataModel> dataModel = game->getDataModel())
			{
				if (RBX::RunService* runService = dataModel->find<RBX::RunService>())
					runService->stopTasks();
            
				if(RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(dataModel.get()))
					service->setHardwareDevice(NULL);
			}
        
			if (sequence)
			{
				if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
					if (RBX::RunService* rs = dataModel->find<RBX::RunService>())
						rs->getPhysicsJob()->removeCoordinator(sequence);
			}
        
			stopJobs();
        
			marshaller->ProcessMessages();
        
			if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
			{
				userInput.reset();
            
				leaveGameVerb.reset();
				fullscreenVerb.reset();
				studioVerb.reset();
				screenshotVerb.reset();
				shutdownClientVerb.reset();
			}

			game->shutdown();
		}
        
		 game->shutdown();
    }
    
    // marshaller has not finished unbinding workspace, wait until this is finished
    if(view->getDataModel())
    {
        while (view->getDataModel())
        {
            sleep(0.016667f);
        }
    }
    
    RBXASSERT(!view->getDataModel());
}

RobloxView::~RobloxView(void)
{
	if (sequence)
	{
        if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
            if (RBX::RunService* rs = dataModel->find<RBX::RunService>())
                rs->getPhysicsJob()->removeCoordinator(sequence);
	}
    
    stopJobs();

    if( shared_ptr<RBX::DataModel> dataModel = game->getDataModel() )
	{
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
	
		RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(dataModel.get());
		service->setHardwareDevice(NULL);
		
		view->bindWorkspace(boost::shared_ptr<RBX::DataModel>());

		userInput.reset();
		
		leaveGameVerb.reset();
		fullscreenVerb.reset();
		studioVerb.reset();
		screenshotVerb.reset();
		shutdownClientVerb.reset();
		
	}
	
	RBX::FunctionMarshaller::ReleaseWindow(marshaller);
	
	// First destroy the view before closing the DataModel
	view.reset();
	
	Roblox::relinquishGame(game);
}

void RobloxView::bindToWorkspace()
{
    shared_ptr<RBX::DataModel> dataModel = game->getDataModel();
    if(!dataModel)
        return;

	// Note that this code needs to be thread-sensitive
	RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);

	view->bindWorkspace(dataModel);
	view->buildGui();

    if (userInput)
    {
        RBX::ControllerService* service = RBX::ServiceProvider::create<RBX::ControllerService>(dataModel.get());
        service->setHardwareDevice(userInput.get());
    }
}

void RobloxView::setBounds(unsigned int width, unsigned int height)
{
	this->width = width; this->height = height;
	if (view)
	{
		view->onResize(width, height);
	}
}

static void executeScript(boost::shared_ptr<RBX::Game> game, std::string urlScript, bool isApp, bool isFromProtocolHandler)
{
	RC_PROBE("executeScript ENTER url=%s\n",urlScript.c_str());
	RBX::Security::Impersonator impersonate(RBX::Security::COM);

	std::ostringstream data;
	if (RBX::ContentProvider::isUrl(urlScript))
	{
		try
		{
			RBX::DataModel::LegacyLock lock(game->getDataModel(), RBX::DataModelJob::Write);
			std::auto_ptr<std::istream> stream(RBX::ServiceProvider::create<RBX::ContentProvider>(game->getDataModel().get())->getContent(RBX::ContentId(urlScript.c_str())));
			boost::iostreams::copy(*stream, data);
			RC_PROBE("executeScript getContent OK bytes=%d\n",(int)data.str().size());
		}
		catch (const std::exception& _e)
		{
			RC_PROBE("executeScript getContent EXCEPTION: %s\n",_e.what());
			return;
		}
	}
	else
	{
		RC_PROBE("executeScript: not a URL, returning\n");
		return;	// silent failure is harder to hack :)
	}

    RBX::ProtectedString verifiedSource;

	try
    {
        verifiedSource = ProtectedString::fromTrustedSource(data.str());
        ContentProvider::verifyScriptSignature(verifiedSource, true);
        RC_PROBE("executeScript verifySignature OK\n");
	}
	catch(std::bad_alloc&)
	{
		throw;
	}
	catch(std::exception& _e)
	{
		RC_PROBE("executeScript verifySignature EXCEPTION: %s\n",_e.what());
		return;
	}

	shared_ptr<RBX::DataModel> dm = game->getDataModel();

    if(!dm)
	{
		RC_PROBE("executeScript: dm is null\n");
        return;
	}

	RBX::DataModel::LegacyLock lock(dm, RBX::DataModelJob::Write);

	if (dm->isClosed())
	{
		RC_PROBE("executeScript: dm isClosed\n");
		return;
	}

	RC_PROBE("executeScript: checking join.ashx find=%lu\n",(unsigned long)urlScript.find("join.ashx"));
    if (urlScript.find("join.ashx"))
    {
        std::string jdata = verifiedSource.getSource();
        size_t firstNewLineIndex = jdata.find("\r\n");
        RC_PROBE("executeScript: firstNL=%lu jdata[NL+2]=%c\n",(unsigned long)firstNewLineIndex,(firstNewLineIndex+2 < jdata.size() ? jdata[firstNewLineIndex+2] : '?'));
        if (firstNewLineIndex != std::string::npos && firstNewLineIndex+2 < jdata.size() && jdata[firstNewLineIndex+2] == '{')
        {
            int launchMode = 0;
            if (isFromProtocolHandler)
                launchMode = 1;

            RC_PROBE("executeScript: calling configurePlayer json=%s\n",jdata.substr(firstNewLineIndex+2,80).c_str());
            try {
                game->configurePlayer(RBX::Security::COM, jdata.substr(firstNewLineIndex+2), launchMode);
                RC_PROBE("executeScript: configurePlayer OK\n");
            } catch (const std::exception& _e) {
                RC_PROBE("executeScript: configurePlayer std::exception: %s\n",_e.what());
            } catch (...) {
                RC_PROBE("executeScript: configurePlayer unknown exception\n");
            }
            RC_PROBE("executeScript: calling loadContent placeID=%d\n", dm->getPlaceID());
            try {
                dm->loadContent(RBX::ContentId(RBX::format("rbxassetid://%d", dm->getPlaceID())));
                RC_PROBE("loadContent DONE\n");
            } catch (const std::exception& _e) {
                RC_PROBE("loadContent EXCEPTION: %s\n", _e.what());
            }
            return;
        }
        RC_PROBE("executeScript: join.ashx branch: not JSON or bad NL, falling through\n");
    }

	RC_PROBE("executeScript: executeInNewThread\n");
	RBX::ScriptContext* context = dm->create<RBX::ScriptContext>();
	context->executeInNewThread(RBX::Security::COM, verifiedSource, "Start Game");
	RC_PROBE("executeScript: done\n");
}

boost::shared_ptr<RBX::Game> RobloxView::startGame(std::string urlScript, const bool isApp) {
	
	boost::shared_ptr<RBX::Game> game(Roblox::getpreloadedGame(isApp));
	boost::thread(boost::bind(&executeScript, game, urlScript, isApp, false));
	return game;
}

RobloxView *RobloxView::start_game(void *wnd, void *appwnd, std::string urlScript, const bool isApp)
{
    boost::shared_ptr<RBX::Game> game = startGame(urlScript, isApp);
    
    return new RobloxView(wnd, appwnd, game);
}

RobloxView *RobloxView::init_game(void *wnd, void* appwnd, const bool isApp)
{
    boost::shared_ptr<RBX::Game> game(Roblox::getpreloadedGame(isApp));
    return new RobloxView(wnd, appwnd, game);
}

// Native offline movement: WASD walks relative to camera, Space jumps.
// Fallback when ControlScript is missing or not yet loaded. If Camera is CUSTOM
// (Lua CameraScript), do not force FOLLOW, let Lua own the camera.
static void offlinePlayStep(weak_ptr<DataModel> weakDm, const Heartbeat& /*hb*/)
{
	shared_ptr<DataModel> dm = weakDm.lock();
	if (!dm)
		return;

	ModelInstance* character = Network::Players::findLocalCharacter(dm.get());
	Humanoid* humanoid = Humanoid::modelIsCharacter(character);
	if (!humanoid)
		return;

	Workspace* ws = dm->getWorkspace();
	Camera* cam = ws ? ws->getCamera() : NULL;
	if (!cam)
		return;

	const bool luaCamera = (cam->getCameraType() == Camera::CUSTOM_CAMERA);
	if (!luaCamera)
	{
		// Keep follow-cam glued to the humanoid (Studio free-fly uses FIXED + no subject).
		if (cam->getCameraType() != Camera::FOLLOW_CAMERA &&
			cam->getCameraType() != Camera::TRACK_CAMERA &&
			cam->getCameraType() != Camera::ATTACH_CAMERA)
		{
			cam->setCameraType(Camera::FOLLOW_CAMERA);
		}
		if (cam->getCameraSubject() != humanoid)
			cam->setCameraSubject(humanoid);
	}

	// If LocalPlayer has a ControlScript under PlayerScripts, still apply native
	// WASD as a safety net (Lua modules can fail offline without web flags).
	ControllerService* cs = ServiceProvider::find<ControllerService>(dm.get());
	const UserInputBase* hw = cs ? cs->getHardwareDevice() : NULL;
	if (!hw)
		return;

	NavKeys nav;
	hw->getNavKeys(nav, dm->getSharedSuppressNavKeys());

	// Camera-relative walk on the XZ plane (classic third-person).
	Vector3 look = cam->getCameraCoordinateFrame().lookVector();
	look.y = 0.0f;
	if (look.unitize() < 1e-4f)
		look = Vector3(0, 0, -1);
	Vector3 right = look.cross(Vector3::unitY());
	if (right.unitize() < 1e-4f)
		right = Vector3(1, 0, 0);

	Vector3 dir = Vector3::zero();
	if (nav.forward())
		dir += look;
	if (nav.backward())
		dir -= look;
	if (nav.left())
		dir -= right;
	if (nav.right())
		dir += right;

	if (dir.squaredLength() > 1e-6f)
		humanoid->setWalkDirection(dir.direction());
	else
		humanoid->setWalkDirection(Vector3::zero());

	if (nav.space)
		humanoid->setJump(true);
}

// Wire every shippable content/* asset the offline place can use without network.
static void offlineEnsureWorldAssets(DataModel* dm)
{
	if (!dm)
		return;

	ContentProvider* cp = ServiceProvider::create<ContentProvider>(dm);

	// --- Sky (content/sky JPGs always in content/; platform .tex also available) ---
	Lighting* lighting = ServiceProvider::create<Lighting>(dm);
	if (lighting)
	{
		Sky* sky = lighting->findFirstChildOfType<Sky>();
		if (!sky)
		{
			shared_ptr<Sky> newSky = Creatable<Instance>::create<Sky>();
			// Prefer classic plain sky faces shipped under content/sky/
			newSky->setSkyboxBk(TextureId(ContentId::fromAssets("sky/null_plainsky512_bk.jpg")));
			newSky->setSkyboxDn(TextureId(ContentId::fromAssets("sky/null_plainsky512_dn.jpg")));
			newSky->setSkyboxFt(TextureId(ContentId::fromAssets("sky/null_plainsky512_ft.jpg")));
			newSky->setSkyboxLf(TextureId(ContentId::fromAssets("sky/null_plainsky512_lf.jpg")));
			newSky->setSkyboxRt(TextureId(ContentId::fromAssets("sky/null_plainsky512_rt.jpg")));
			newSky->setSkyboxUp(TextureId(ContentId::fromAssets("sky/null_plainsky512_up.jpg")));
			newSky->setParent(lighting);
			sky = newSky.get();
			RC_PROBE("offlineAssets: created Sky with content/sky null_plainsky*\n");
		}
		else
		{
			RC_PROBE("offlineAssets: place already has Sky\n");
		}
		// Pleasant daytime offline defaults
		lighting->setTimeStr("14:00:00");
	}

	// --- Verify key content/* files resolve on disk (no CDN) ---
	{
		static const char* kPreload[] = {
			"textures/face.png",
			"textures/HurtOverlay.png",
			"textures/ui/TopBar/dropshadow.png",
			"textures/ui/Menu/Hamburger.png",
			"textures/ui/Chat/Chat.png",
			"textures/ui/Backpack/Backpack.png",
			"textures/ui/Health-BKG-Center.png",
			"sky/null_plainsky512_up.jpg",
			"sky/sun.jpg",
			"sky/moon.jpg",
			"sounds/action_footsteps_plastic.mp3",
			"sounds/action_jump.mp3",
			"sounds/uuhhh.mp3",
			"other/character3.rbxm",
			"other/humanoidSoundNewLocal.rbxmx",
			"other/humanoidAnimateLocalKeyframe2.rbxm",
			"other/humanoidHealthRegenScript.rbxm",
			NULL
		};
		int ok = 0, miss = 0;
		for (int i = 0; kPreload[i]; ++i)
		{
			std::string path = ContentProvider::findAsset(ContentId::fromAssets(kPreload[i]));
			if (!path.empty())
				++ok;
			else
				++miss;
		}
		RC_PROBE("offlineAssets: preload check ok=%d miss=%d\n", ok, miss);
		(void)cp;
	}

	// --- SpawnLocation if place has none (baseplate offline) ---
	if (Workspace* ws = dm->getWorkspace())
	{
		if (!ws->findFirstChildOfType<SpawnLocation>())
		{
			shared_ptr<SpawnLocation> spawn = Creatable<Instance>::create<SpawnLocation>();
			spawn->setName("SpawnLocation");
			// Sit above typical baseplate so character doesn't fall through
			CoordinateFrame cf;
			cf.translation = Vector3(0, 5, 0);
			spawn->setCoordinateFrame(cf);
			spawn->setPartSizeXml(Vector3(6, 1, 6));
			spawn->setAnchored(true);
			spawn->setCanCollide(true);
			// Classic-style spawn shield: a few seconds then gone (Duration prop).
			spawn->forcefieldDuration = 5;
			spawn->setParent(ws);
			RC_PROBE("offlineAssets: added SpawnLocation ffDuration=5\n");
		}
	}

	// Ensure StarterCharacterScripts container exists for character extras
	if (StarterPlayerService* sps = ServiceProvider::create<StarterPlayerService>(dm))
	{
		if (!sps->findFirstChildOfType<StarterCharacterScripts>())
		{
			shared_ptr<StarterCharacterScripts> scs = Creatable<Instance>::create<StarterCharacterScripts>();
			scs->setParent(sps);
			RC_PROBE("offlineAssets: created StarterCharacterScripts\n");
		}
	}
}

// When StarterPlayer default scripts finish loading offline, copy into LocalPlayer.
static void offlineDefaultScriptsReady(weak_ptr<DataModel> weakDm, weak_ptr<Network::Player> weakPlayer)
{
	shared_ptr<DataModel> dm = weakDm.lock();
	shared_ptr<Network::Player> player = weakPlayer.lock();
	if (!dm || !player)
		return;

	StarterPlayerService* starter = ServiceProvider::find<StarterPlayerService>(dm.get());
	StarterPlayerScripts* sps = starter ? starter->findFirstChildOfType<StarterPlayerScripts>() : NULL;
	if (!sps)
		return;

	PlayerScripts* ps = player->findFirstChildOfType<PlayerScripts>();
	if (!ps)
		return;

	ps->CopyStarterPlayerScripts(sps);

	int control = sps->findFirstChildByName("ControlScript") ? 1 : 0;
	int camera = sps->findFirstChildByName("CameraScript") ? 1 : 0;
	int psKids = (int)ps->numChildren();
	RC_PROBE("offlineDefaultScriptsReady: starter control=%d camera=%d playerScriptsKids=%d\n", control, camera, psKids);
}

// Phase 3/4: Visit Solo offline, LocalPlayer, character, Run, default PlayerScripts.
// Without NetworkClient/NetworkServer, both frontendProcessing and backendProcessing
// are true (play-solo), so LoadCharacter is allowed on this process.
static void startLocalPlay(shared_ptr<DataModel> dm)
{
	using namespace RBX::Network;

	RC_PROBE("startLocalPlay: begin canCompileScripts=%d\n", LuaVM::canCompileScripts() ? 1 : 0);

	// Not Studio edit: disables free-fly / edit camera interpolation paths.
	RBX::GameBasicSettings::singleton().setStudioMode(false);
	RBX::GameBasicSettings::singleton().setControlMode(RBX::GameBasicSettings::CONTROL_CLASSIC);

	// CoreScripts (StarterScript → Topbar, etc.) load from content/scripts when canCompile.
	// buildGui may already have called startCoreScripts; re-entry is gated by areCoreScriptsLoaded.
	try
	{
		dm->startCoreScripts(true /* buildInGameGui */);
		RC_PROBE("startLocalPlay: startCoreScripts ok\n");
	}
	catch (const std::exception& e)
	{
		RC_PROBE("startLocalPlay: startCoreScripts EXCEPTION %s\n", e.what());
	}

	// Sanity: can we still fetch StarterScript source from disk?
	{
		boost::optional<ProtectedString> ss = CoreScript::fetchSource("StarterScript");
		RC_PROBE("startLocalPlay: StarterScript fetch %s bytes=%d\n", ss ? "OK" : "MISS", ss ? (int)ss->getSource().size() : 0);
	}

	Players* players = ServiceProvider::create<Players>(dm.get());
	if (!players)
	{
		RC_PROBE("startLocalPlay: FAIL no Players service\n");
		return;
	}

	// Ensure StarterPlayerScripts exist (workspaceLoaded may already have created them).
	StarterPlayerScripts* starterScripts = NULL;
	if (StarterPlayerService* starter = ServiceProvider::create<StarterPlayerService>(dm.get()))
	{
		starterScripts = starter->findFirstChildOfType<StarterPlayerScripts>();
		if (!starterScripts)
		{
			shared_ptr<StarterPlayerScripts> scripts = Creatable<Instance>::create<StarterPlayerScripts>();
			scripts->setParent(starter);
			starterScripts = scripts.get();
		}
	}

	Player* localPlayer = players->getLocalPlayer();
	if (!localPlayer)
	{
		// userId=1 is a normal offline test id; name defaults to "Player"
		players->createLocalPlayer(1, false);
		localPlayer = players->getLocalPlayer();
	}
	if (!localPlayer)
	{
		RC_PROBE("startLocalPlay: FAIL createLocalPlayer\n");
		return;
	}
	RC_PROBE("startLocalPlay: localPlayer ok userId=%d name=%s\n", localPlayer->getUserID(), localPlayer->getName().c_str());

	// Sky, spawn, content preload, all from local content/ + PlatformContent
	offlineEnsureWorldAssets(dm.get());

	// Spawn classic R6 character (character3.rbxm from content/other)
	try
	{
		localPlayer->luaLoadCharacter(true /* inGame: non-blocking appearance (empty offline) */);
	}
	catch (const std::exception& e)
	{
		RC_PROBE("startLocalPlay: loadCharacter EXCEPTION %s\n", e.what());
		return;
	}

	ModelInstance* character = localPlayer->getCharacter();
	Humanoid* humanoid = character ? Humanoid::modelIsCharacter(character) : NULL;
	int charKids = character ? (int)character->numChildren() : 0;
	int hasSound = character && character->findFirstChildByName("Sound") ? 1 : 0;
	int hasAnimate = character && character->findFirstChildByName("Animate") ? 1 : 0;
	int hasHealthScr = character && character->findFirstChildByName("Health") ? 1 : 0;
	RC_PROBE("startLocalPlay: character=%p kids=%d humanoid=%p sound=%d animate=%d healthScr=%d\n",
		(void*)character, charKids, (void*)humanoid, hasSound, hasAnimate, hasHealthScr);

	// Follow-camera attached to Humanoid (native Camera::step path, not CUSTOM/Lua).
	if (Workspace* ws = dm->getWorkspace())
	{
		// Play-mode mouse (not Studio tools)
		ws->setDefaultMouseCommand();

		if (Camera* cam = ws->getCamera())
		{
			if (humanoid)
			{
				cam->setCameraSubject(humanoid);
				// FOLLOW: native step tracks subject; CUSTOM needs Lua CameraScript.
				cam->setCameraType(Camera::FOLLOW_CAMERA);
				// Frame behind character using torso position
				Vector3 torsoPos = Vector3(0, 5, 0);
				if (PartInstance* torso = humanoid->getTorsoSlow())
					torsoPos = torso->getCoordinateFrame().translation;
				Vector3 focus = torsoPos + Vector3(0, 2.0f, 0);
				Vector3 behind = focus + Vector3(0, 4.0f, 14.0f);
				cam->setCameraFocus(CoordinateFrame(focus));
				cam->setCameraCoordinateFrame(CoordinateFrame(behind));
				cam->setDistanceFromTarget(14.0f);
			}
			RC_PROBE("startLocalPlay: camera type=%d subject=%p\n", (int)cam->getCameraType(), (void*)cam->getCameraSubject());
		}
	}

	// Physics + scripts run loop
	RunService* runService = ServiceProvider::create<RunService>(dm.get());
	if (runService)
	{
		runService->run();

		// Phase 4: force default ControlScript/CameraScript load (no NetworkServer confirm path).
		if (starterScripts)
		{
			g_offlineScriptsLoadedConnection.disconnect();
			// If already loaded, copy immediately; else wait for async rbxmx load.
			if (starterScripts->areDefaultScriptsLoaded())
			{
				offlineDefaultScriptsReady(weak_ptr<DataModel>(dm), weak_from(localPlayer));
			}
			else
			{
				g_offlineScriptsLoadedConnection = starterScripts->defaultScriptsLoadedSignal.connect(
					boost::bind(&offlineDefaultScriptsReady, weak_ptr<DataModel>(dm), weak_from(localPlayer)));
				starterScripts->InitializeDefaultScripts();
				// Also drive the solo confirm path so PlayerScripts copies when ready.
				starterScripts->requestDefaultScriptsServer(shared_from(localPlayer));
			}
			RC_PROBE("startLocalPlay: defaultScripts requested loaded=%d\n", starterScripts->areDefaultScriptsLoaded() ? 1 : 0);
		}

		// Drive WASD → Humanoid every heartbeat (fallback if ControlScript absent/broken)
		g_offlinePlayStepConnection.disconnect();
		g_offlinePlayStepConnection = runService->heartbeatSignal.connect(
			boost::bind(&offlinePlayStep, weak_ptr<DataModel>(dm), _1));
		RC_PROBE("startLocalPlay: RunService run state=%d offlineControl=1\n", (int)runService->getRunState());
	}
	else
	{
		RC_PROBE("startLocalPlay: FAIL no RunService\n");
	}

	// Note: Topbar builds async after LocalPlayer; pixel check + RC_PROBE CoreGui
	// snapshot at DONE may still show topBar=0 for ~1s (expected).

	dm->gameLoaded();

	// Dismiss LoadingScript BlackFrame if it still exists (Phase 4 regression offline).
	// ReplicatedFirst may not finish the normal join/replication dismiss path.
	if (ReplicatedFirst* rf = ServiceProvider::create<ReplicatedFirst>(dm.get()))
	{
		if (!rf->getIsFinishedReplicating())
			rf->setAllInstancesHaveReplicated(); // play-solo: fires FinishedReplicating
		rf->doRemoveDefaultLoadingGui();
	}
	// Hard cleanup: LoadingScript parents ScreenGui → BlackFrame under CoreGui.
	if (CoreGuiService* coreGui = ServiceProvider::find<CoreGuiService>(dm.get()))
	{
		int removed = 0;
		if (Instance* bf = coreGui->findFirstChildByNameRecursive("BlackFrame"))
		{
			// Prefer destroying the ScreenGui that owns BlackFrame.
			Instance* doomed = bf->getParent() ? bf->getParent() : bf;
			if (doomed == static_cast<Instance*>(coreGui))
				doomed = bf;
			doomed->setParent(NULL);
			++removed;
		}
		// Probe 2016 CoreGui chrome (Topbar / health / backpack icons).
		int hasRobloxGui = coreGui->findFirstChildByName("RobloxGui") ? 1 : 0;
		int hasTopBar = coreGui->findFirstChildByNameRecursive("TopBarContainer") ? 1 : 0;
		int hasHealth = coreGui->findFirstChildByNameRecursive("HealthGui") ||
			coreGui->findFirstChildByNameRecursive("HealthContainer") ||
			coreGui->findFirstChildByNameRecursive("HealthFrame") ? 1 : 0;
		int hasBackpack = coreGui->findFirstChildByNameRecursive("Backpack") ? 1 : 0;
		int hasChat = coreGui->findFirstChildByNameRecursive("ChatWindow") ||
			coreGui->findFirstChildByNameRecursive("ChatBar") ||
			coreGui->findFirstChildByNameRecursive("ChatIcon") ? 1 : 0;
		int modules = 0;
		if (Instance* rg = coreGui->findFirstChildByName("RobloxGui"))
		{
			if (Instance* mods = rg->findFirstChildByName("Modules"))
				modules = (int)mods->numChildren();
		}
		RC_PROBE("startLocalPlay: CoreGui robloxGui=%d topBar=%d health=%d backpack=%d chat=%d modules=%d loadRemoved=%d\n",
			hasRobloxGui, hasTopBar, hasHealth, hasBackpack, hasChat, modules, removed);
	}

	RC_PROBE("startLocalPlay: DONE gameLoaded=1 followCam+wasd+corescripts\n");
}

static void loadPlaceFileTask(shared_ptr<DataModel> dm, std::string contentUrl)
{
	try
	{
		// Match Studio place open: elevated identity for restricted props (Debris MaxItems, etc.)
		RBX::Security::Impersonator impersonate(RBX::Security::COM);
		RC_PROBE("loadPlaceFile: WriteJob loadContent %s\n", contentUrl.c_str());
		dm->loadContent(RBX::ContentId(contentUrl));
		int kids = 0;
		if (Workspace* ws = dm->getWorkspace())
			kids = (int)ws->numChildren();
		RC_PROBE("loadPlaceFile: OK workspaceChildren=%d\n", kids);

		// Phase 3/4: local play loop after place is in the DataModel
		startLocalPlay(dm);
	}
	catch (const std::exception& e)
	{
		RC_PROBE("loadPlaceFile: EXCEPTION %s\n", e.what());
	}
	catch (...)
	{
		RC_PROBE("loadPlaceFile: UNKNOWN EXCEPTION\n");
	}
}

bool RobloxView::loadPlaceFile(const std::string& absolutePath)
{
	RC_PROBE("loadPlaceFile: path=%s\n", absolutePath.c_str());

	shared_ptr<DataModel> dm = getDataModel();
	if (!dm)
	{
		RC_PROBE("loadPlaceFile: no DataModel\n");
		return false;
	}

	// ContentId::isFile requires "file://" + path. Absolute Unix paths start with '/'.
	// IMPORTANT: do NOT LegacyLock on the main thread after RenderJob is running 
	// it deadlocks waiting for the write slot. Schedule on the Write job instead.
	const std::string contentUrl = "file://" + absolutePath;
	dm->submitTask(boost::bind(&loadPlaceFileTask, dm, contentUrl), RBX::DataModelJob::Write);

	RC_PROBE("loadPlaceFile: scheduled WriteJob\n");
	return true;
}

void RobloxView::executeJoinScript(const std::string& urlScript, const bool isApp, const bool isFromProtocolHandler)
{
    boost::thread(boost::bind(&executeScript, game, urlScript, isApp, isFromProtocolHandler));
}

void RobloxView::setUIMessage(const std::string& message)
{
    if (shared_ptr<DataModel> dm = game->getDataModel())
        dm->submitTask(boost::bind(&RobloxView::setUIMessage_, this, message), DataModelJob::Write);
}

void RobloxView::setUIMessage_(const std::string& message)
{
    if (shared_ptr<DataModel> dm = game->getDataModel())
    {
        if (message.length() > 0)
        {
            dm->setUiMessage(message);
        }
        else
        {
            dm->clearUiMessage();
        }
        
        if (GuiService*gs = dm->create<GuiService>())
            gs->setUiMessage(GuiService::UIMESSAGE_INFO, message);
    }
}



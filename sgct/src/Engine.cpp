#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "../include/sgct/Engine.h"
#include "../include/sgct/freetype.h"
#include "../include/sgct/FontManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/TextureManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/ShaderManager.h"
#include <math.h>
#include <sstream>

using namespace core_sgct;

sgct::Engine::Engine( int argc, char* argv[] )
{	
	//init pointers
	mNetworkConnections = NULL;
	mConfig = NULL;
	for( unsigned int i=0; i<3; i++)
		mFrustums[i] = NULL;

	//init function pointers
	mDrawFn = NULL;
	mPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mInitOGLFn = NULL;
	mClearBufferFn = NULL;
	mInternalRenderFn = NULL;
	mTerminate = false;

	localRunningMode = NetworkManager::NotLocal;

	// Initialize GLFW
	if( !glfwInit() )
	{
		mTerminate = true;
		return;
	}

	setClearBufferFunction( clearBuffer );
	nearClippingPlaneDist = 0.1f;
	farClippingPlaneDist = 100.0f;
	showInfo = false;
	showGraph = false;
	showWireframe = false;
	activeFrustum = Frustum::Mono;

	//parse needs to be before read config since the path to the XML is parsed here
	parseArguments( argc, argv );
}

bool sgct::Engine::init()
{
	if(mTerminate)
	{
		sgct::MessageHandler::Instance()->print("Failed to init GLFW.\n");
		return false;
	}
	
	mConfig = new ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		sgct::MessageHandler::Instance()->print("Error in xml config file parsing.\n");
		return false;
	}
	if( !initNetwork() )
	{
		sgct::MessageHandler::Instance()->print("Network init error.\n");
		return false;
	}

	if( !initWindow() )
	{
		sgct::MessageHandler::Instance()->print("Window init error.\n");
		return false;
	}

	initOGL();

	//
	// Add fonts
	//
	if( !FontManager::Instance()->AddFont( "Verdana", "verdana.ttf" ) )
		FontManager::Instance()->GetFont( "Verdana", 14 );

	return true;
}

bool sgct::Engine::initNetwork()
{
	mNetworkConnections = new NetworkManager(localRunningMode);

	//check in cluster configuration which it is
	if( localRunningMode == NetworkManager::NotLocal )
		for(unsigned int i=0; i<NodeManager::Instance()->getNumberOfNodes(); i++)
			if( mNetworkConnections->matchAddress( NodeManager::Instance()->getNodePtr(i)->ip ) )
			{
				NodeManager::Instance()->setThisNodeId(i);
				break;
			}

	if( NodeManager::Instance()->getThisNodeId() == -1 ||
		NodeManager::Instance()->getThisNodeId() >= static_cast<int>( NodeManager::Instance()->getNumberOfNodes() )) //fatal error
	{
		sgct::MessageHandler::Instance()->print("This computer is not a part of the cluster configuration!\n");
		mNetworkConnections->close();
		return false;
	}
	else
	{
		printNodeInfo( static_cast<unsigned int>(NodeManager::Instance()->getThisNodeId()) );
	}

	//Set message handler to send messages or not
	sgct::MessageHandler::Instance()->sendMessagesToServer( !mNetworkConnections->isComputerServer() );
	
	if(!mNetworkConnections->init())
		return false;

    sgct::MessageHandler::Instance()->print("Done\n");
	return true;
}

bool sgct::Engine::initWindow()
{
	NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->useQuadbuffer( NodeManager::Instance()->getThisNodePtr()->stereo == ReadConfig::Active );

	int antiAliasingSamples = NodeManager::Instance()->getThisNodePtr()->numberOfSamples;
	if( antiAliasingSamples > 1 ) //if multisample is used
		glfwOpenWindowHint( GLFW_FSAA_SAMPLES, antiAliasingSamples );

	if( !NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->openWindow() )
		return false;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  sgct::MessageHandler::Instance()->print("Error: %s\n", glewGetErrorString(err));
	  return false;
	}
	sgct::MessageHandler::Instance()->print("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->init( NodeManager::Instance()->getThisNodePtr()->lockVerticalSync );
	NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->setWindowTitle( getBasicInfo() );

	//Must wait until all nodes are running if using swap barrier
	if( NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->isUsingSwapGroups() )
	{
		while(!mNetworkConnections->areAllNodesConnected())
		{
			sgct::MessageHandler::Instance()->print("Waiting for all nodes to connect...\n");
			// Swap front and back rendering buffers
			glfwSleep(0.25);
			glfwSwapBuffers();
		}
	}

	return true;
}

void sgct::Engine::initOGL()
{
	//Get OpenGL version
	int version[3];
	glfwGetGLVersion( &version[0], &version[1], &version[2] );
	sgct::MessageHandler::Instance()->print("OpenGL version %d.%d.%d\n", version[0], version[1], version[2]);

	if (!GLEW_ARB_texture_non_power_of_two)
	{
		sgct::MessageHandler::Instance()->print("Warning! Only power of two textures are supported!\n");
	}

	if( mInitOGLFn != NULL )
		mInitOGLFn();

	calculateFrustums();

	switch( NodeManager::Instance()->getThisNodePtr()->stereo )
	{
	case ReadConfig::Active:
		mInternalRenderFn = &Engine::setActiveStereoRenderingMode;
		break;

	default:
		mInternalRenderFn = &Engine::setNormalRenderingMode;
		break;
	}

	//init swap group barrier when ready to render
	NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->setBarrier(true);
	NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->resetSwapGroupFrameNumber();
}

void sgct::Engine::clean()
{
	sgct::MessageHandler::Instance()->print("Cleaning up...\n");

	//de-init window and unbind swapgroups...
	NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->close();

	//close TCP connections
	if( mNetworkConnections != NULL )
	{
		delete mNetworkConnections;
		mNetworkConnections = NULL;
	}

	if( mConfig != NULL )
	{
		delete mConfig;
		mConfig = NULL;
	}

	for( unsigned int i=0; i<3; i++)
		if( mFrustums[i] != NULL )
		{
			delete mFrustums[i];
			mFrustums[i] = NULL;
		}
	// Destroy explicitly to avoid memory leak messages
	FontManager::Destroy();
	ShaderManager::Destroy();
	SharedData::Destroy();
	TextureManager::Destroy();
	NodeManager::Destroy();
	MessageHandler::Destroy();

	// Close window and terminate GLFW
	glfwTerminate();
}

void sgct::Engine::frameSyncAndLock(int stage)
{
	double t0 = glfwGetTime();
	
	if( stage == PreStage )
	{
		mNetworkConnections->sync();

		if( !mNetworkConnections->isComputerServer() ) //not server
			while(mNetworkConnections->isRunning() && mRunning)// && (glfwGetTime()-t0) < 0.015)
			{
				if( mNetworkConnections->isSyncComplete() )
					break;
			}

		mStatistics.setSyncTime(glfwGetTime() - t0);
	}
	else if( mNetworkConnections->isComputerServer() && localRunningMode == NetworkManager::NotLocal )//post stage
	{
		while(mNetworkConnections->isRunning() && mRunning)// && (glfwGetTime()-t0) < 0.015)
		{
			if( mNetworkConnections->isSyncComplete() )
					break;
		}

		mStatistics.addSyncTime(glfwGetTime() - t0);
	}
}

void sgct::Engine::render()
{
	mRunning = GL_TRUE;

	while( mRunning )
	{
		if( mPreDrawFn != NULL )
			mPreDrawFn();

		if( mNetworkConnections->isComputerServer() )
		{
			SGCTNetwork::setMutexState(true);
				sgct::SharedData::Instance()->encode();
			SGCTNetwork::setMutexState(false);
		}
		else
		{
			if( !mNetworkConnections->isRunning() ) //exit if not running
				break;
		}

		frameSyncAndLock(PreStage);		

		double startFrameTime = glfwGetTime();
		calcFPS(startFrameTime);

		glLineWidth(1.0);
		showWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		(this->*mInternalRenderFn)();

		//restore
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		mStatistics.setDrawTime(glfwGetTime() - startFrameTime);

		frameSyncAndLock(PostStage);

		if( mPostDrawFn != NULL )
			mPostDrawFn();

		if( showGraph )
			mStatistics.draw();
		if( showInfo )
			renderDisplayInfo();

		// Swap front and back rendering buffers
		glfwSwapBuffers();

		mNetworkConnections->swapData();

		// Check if ESC key was pressed or window was closed
		mRunning = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED ) && !mTerminate;
	}
}

void sgct::Engine::renderDisplayInfo()
{
	glColor4f(0.8f,0.8f,0.0f,1.0f);
	unsigned int lFrameNumber = 0;
	NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);
	
	glDrawBuffer(GL_BACK); //draw into both back buffers
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 120, "Node ip: %s (%s)",
		NodeManager::Instance()->getThisNodePtr()->ip.c_str(),
		mNetworkConnections->isComputerServer() ? "master" : "slave");
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 100, "Frame rate: %.3f Hz", mStatistics.getAvgFPS());
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 80, "Render time: %.2f ms", getDrawTime()*1000.0);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 60, "Sync time [%d]: %.2f ms", 
		SharedData::Instance()->getBufferSize(),
		getSyncTime()*1000.0);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 40, "Swap groups: %s and %s (%s) [frame: %d]",
		NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->isUsingSwapGroups() ? "Enabled" : "Disabled",
		NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->isBarrierActive() ? "active" : "not active",
		NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->isSwapGroupMaster() ? "master" : "slave",
		lFrameNumber);

	//if stereoscopic rendering
	if( NodeManager::Instance()->getThisNodePtr()->stereo != ReadConfig::None )
	{
		glDrawBuffer(GL_BACK_LEFT);
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 140, "Active eye: Left");
		glDrawBuffer(GL_BACK_RIGHT);
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 140, "Active eye:        Right");
		glDrawBuffer(GL_BACK);
	}
}

void sgct::Engine::setNormalRenderingMode()
{
	activeFrustum = Frustum::Mono;
	glViewport (0, 0, NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->getHResolution(),
		NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->getVResolution());

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(mFrustums[Frustum::Mono]->getLeft(),
		mFrustums[Frustum::Mono]->getRight(),
        mFrustums[Frustum::Mono]->getBottom(),
		mFrustums[Frustum::Mono]->getTop(),
        mFrustums[Frustum::Mono]->getNear(),
		mFrustums[Frustum::Mono]->getFar());

	//translate to user pos
	glTranslatef(-mConfig->getUserPos()->x, -mConfig->getUserPos()->y, -mConfig->getUserPos()->z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK); //draw into both back buffers
	mClearBufferFn(); //clear buffers
	glLoadIdentity();

	if( mDrawFn != NULL )
		mDrawFn();
}

void sgct::Engine::setActiveStereoRenderingMode()
{
	glViewport (0, 0, NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->getHResolution(),
		NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->getVResolution());
	activeFrustum = Frustum::StereoLeftEye;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(mFrustums[Frustum::StereoLeftEye]->getLeft(),
		mFrustums[Frustum::StereoLeftEye]->getRight(),
        mFrustums[Frustum::StereoLeftEye]->getBottom(),
		mFrustums[Frustum::StereoLeftEye]->getTop(),
        mFrustums[Frustum::StereoLeftEye]->getNear(),
		mFrustums[Frustum::StereoLeftEye]->getFar());

	//translate to user pos
	glTranslatef(-mUser.LeftEyePos.x , -mUser.LeftEyePos.y, -mUser.LeftEyePos.z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK_LEFT);
	mClearBufferFn(); //clear buffers
	glLoadIdentity();

	if( mDrawFn != NULL )
		mDrawFn();

	activeFrustum = Frustum::StereoRightEye;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(mFrustums[Frustum::StereoRightEye]->getLeft(),
		mFrustums[Frustum::StereoRightEye]->getRight(),
        mFrustums[Frustum::StereoRightEye]->getBottom(),
		mFrustums[Frustum::StereoRightEye]->getTop(),
        mFrustums[Frustum::StereoRightEye]->getNear(),
		mFrustums[Frustum::StereoRightEye]->getFar());

	//translate to user pos
	glTranslatef(-mUser.RightEyePos.x , -mUser.RightEyePos.y, -mUser.RightEyePos.z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK_RIGHT);
	mClearBufferFn(); //clear buffers
	glLoadIdentity();

	if( mDrawFn != NULL )
		mDrawFn();
}

void sgct::Engine::calculateFrustums()
{
	mUser.LeftEyePos.x = mConfig->getUserPos()->x - mConfig->getEyeSeparation()/2.0f;
	mUser.LeftEyePos.y = mConfig->getUserPos()->y;
	mUser.LeftEyePos.z = mConfig->getUserPos()->z;

	mUser.RightEyePos.x = mConfig->getUserPos()->x + mConfig->getEyeSeparation()/2.0f;
	mUser.RightEyePos.y = mConfig->getUserPos()->y;
	mUser.RightEyePos.z = mConfig->getUserPos()->z;

	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = nearClippingPlaneDist / (NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::LowerLeft ].z - mConfig->getUserPos()->z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;

	mFrustums[Frustum::Mono] = new Frustum(
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mConfig->getUserPos()->x)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::UpperRight ].x - mConfig->getUserPos()->x)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mConfig->getUserPos()->y)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::UpperRight ].y - mConfig->getUserPos()->y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[Frustum::StereoLeftEye] = new Frustum(
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mUser.LeftEyePos.x)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::UpperRight ].x - mUser.LeftEyePos.x)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mUser.LeftEyePos.y)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::UpperRight ].y - mUser.LeftEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[Frustum::StereoRightEye] = new Frustum(
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mUser.RightEyePos.x)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::UpperRight ].x - mUser.RightEyePos.x)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mUser.RightEyePos.y)*nearFactor,
		(NodeManager::Instance()->getThisNodePtr()->viewPlaneCoords[ ReadConfig::UpperRight ].y - mUser.RightEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);
}

void sgct::Engine::parseArguments( int argc, char* argv[] )
{
	//parse arguments
	sgct::MessageHandler::Instance()->print("Parsing arguments...");
	int i=0;
	while( i<argc )
	{
		if( strcmp(argv[i],"-config") == 0 )
		{
			configFilename.assign(argv[i+1]);
			i+=2;
		}
		else if( strcmp(argv[i],"--client") == 0 )
		{
			localRunningMode = NetworkManager::LocalClient;
			i++;
		}
		else if( strcmp(argv[i],"-local") == 0 )
		{
			localRunningMode = NetworkManager::LocalServer;
			int tmpi = -1;
			std::stringstream ss( argv[i+1] );
			ss >> tmpi;
			NodeManager::Instance()->setThisNodeId(tmpi);
			i+=2;
		}
		else
			i++; //iterate
	}

	sgct::MessageHandler::Instance()->print(" Done\n");
}

void sgct::Engine::setDrawFunction(void(*fnPtr)(void))
{
	mDrawFn = fnPtr;
}

void sgct::Engine::setPreDrawFunction(void(*fnPtr)(void))
{
	mPreDrawFn = fnPtr;
}

void sgct::Engine::setPostDrawFunction(void(*fnPtr)(void))
{
	mPostDrawFn = fnPtr;
}

void sgct::Engine::setInitOGLFunction(void(*fnPtr)(void))
{
	mInitOGLFn = fnPtr;
}

void sgct::Engine::setClearBufferFunction(void(*fnPtr)(void))
{
	mClearBufferFn = fnPtr;
}

void sgct::Engine::setExternalControlCallback(void(*fnPtr)(const char *, int, int))
{
	mNetworkCallbackFn = fnPtr;
}

void sgct::Engine::clearBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void sgct::Engine::printNodeInfo(unsigned int nodeId)
{
	sgct::MessageHandler::Instance()->print("This node has index %d.\n", nodeId);
}

void sgct::Engine::calcFPS(double timestamp)
{
	static double lastTimestamp = glfwGetTime();
	mStatistics.setFrameTime(timestamp - lastTimestamp);
	lastTimestamp = timestamp;
    static double renderedFrames = 0.0;
	static double tmpTime = 0.0;
	renderedFrames += 1.0;
	tmpTime += mStatistics.getFrameTime();
	if( tmpTime >= 1.0 )
	{
		mStatistics.setAvgFPS(renderedFrames / tmpTime);
		renderedFrames = 0.0;
		tmpTime = 0.0;

		//don't set if in full screen
		if(NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->getWindowMode() == GLFW_WINDOW)
			NodeManager::Instance()->getThisNodePtr()->getWindowPtr()->setWindowTitle( getBasicInfo() );
	}
}

const double & sgct::Engine::getDt()
{
	return mStatistics.getFrameTime();
}

const double & sgct::Engine::getDrawTime()
{
	return mStatistics.getDrawTime();
}

const double & sgct::Engine::getSyncTime()
{
	return mStatistics.getSyncTime();
}

void sgct::Engine::setNearAndFarClippingPlanes(float _near, float _far)
{
	nearClippingPlaneDist = _near;
	farClippingPlaneDist = _far;
	calculateFrustums();
}

void sgct::Engine::decodeExternalControl(const char * receivedData, int receivedLenght, int clientIndex)
{
	if(mNetworkCallbackFn != NULL)
		mNetworkCallbackFn(receivedData, receivedLenght, clientIndex);
}

void sgct::Engine::sendMessageToExternalControl(void * data, int lenght)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( data, lenght );
}

void sgct::Engine::sendMessageToExternalControl(const std::string msg)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( (void *)msg.c_str(), msg.size() );
}

void sgct::Engine::setExternalControlBufferSize(unsigned int newSize)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->setBufferSize(newSize);
}

const char * sgct::Engine::getBasicInfo()
{
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
	sprintf_s( basicInfo, sizeof(basicInfo), "Node: %s (%s) | fps: %.2f",
		localRunningMode == NetworkManager::NotLocal ? NodeManager::Instance()->getThisNodePtr()->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "server" : "slave",
		mStatistics.getAvgFPS());
    #else
    sprintf( basicInfo, "Node: %s (%s) | fps: %.2f",
		localRunningMode == NetworkManager::NotLocal ? NodeManager::Instance()->getThisNodePtr()->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "server" : "slave",
		mStatistics.getAvgFPS());
    #endif

	return basicInfo;
}

/****************************************************************************
*  OpenNI 1.1 Alpha                                                         *
*  Copyright (C) 2011 PrimeSense Ltd.                                       *
*                                                                           *
*  This file is part of OpenNI.                                             *
*                                                                           *
*  OpenNI is free software: you can redistribute it and/or modify           *
*  it under the terms of the GNU Lesser General Public License as published *
*  by the Free Software Foundation, either version 3 of the License, or     *
*  (at your option) any later version.                                      *
*                                                                           *
*  OpenNI is distributed in the hope that it will be useful,                *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
*  GNU Lesser General Public License for more details.                      *
*                                                                           *
*  You should have received a copy of the GNU Lesser General Public License *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.           *
*                                                                           *
****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <math.h>
#include <GL/freeglut.h>
#include "SceneDrawer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
xn::Context g_Context;
xn::ScriptNode g_scriptNode;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;
xn::Player g_Player;
xn::ImageGenerator g_ImageGenerator;

XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";
XnBool g_bDrawBackground = TRUE;
XnBool g_bDrawPixels = TRUE;
XnBool g_bDrawSkeleton = TRUE;
XnBool g_bPrintID = TRUE;
XnBool g_bPrintState = TRUE;

int count = 0;

//network with Raspberry Pi
struct hostent *host;
struct sockaddr_in me;
int sock0,sock;
int turnpoint, new_turn;

#ifndef USE_GLES
#if (XN_PLATFORM == XN_PLATFORM_MACOSX)
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif
#else
	#include "opengles.h"
#endif

#ifdef USE_GLES
static EGLDisplay display = EGL_NO_DISPLAY;
static EGLSurface surface = EGL_NO_SURFACE;
static EGLContext context = EGL_NO_CONTEXT;
#endif

#define GL_WIN_SIZE_X 720
#define GL_WIN_SIZE_Y 480
#define TIMELIMIT 30

XnBool g_bPause = false;
XnBool g_bRecord = false;

XnBool g_bQuit = false;

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

void CleanupExit()
{
	g_scriptNode.Release();
	g_DepthGenerator.Release();
	g_ImageGenerator.Release();
	g_UserGenerator.Release();
	g_Player.Release();
	g_Context.Release();

	exit (1);
}

// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("New User %d\n", nId);
	// New user found
	if (g_bNeedPose)
	{
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
	}
	else
	{
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}
// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("Lost user %d\n", nId);
}
// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
	printf("Pose %s detected for user %d\n", strPose, nId);
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}
// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
	printf("Calibration started for user %d\n", nId);
}
// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie)
{
	if (bSuccess)
	{
		// Calibration succeeded
		printf("Calibration complete, start tracking user %d\n", nId);
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		printf("Calibration failed for user %d\n", nId);
		if (g_bNeedPose)
		{
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}
		else
		{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}

void XN_CALLBACK_TYPE UserCalibration_CalibrationComplete(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie)
{
	if (eStatus == XN_CALIBRATION_STATUS_OK)
	{
		// Calibration succeeded
		printf("Calibration complete, start tracking user %d\n", nId);
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		printf("Calibration failed for user %d\n", nId);
		if (g_bNeedPose)
		{
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}
		else
		{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}

#define XN_CALIBRATION_FILE_NAME "UserCalibration.bin"

void Pos(XnUserID player,XnSkeletonJoint ejoint,XnPoint3D* point)
{
  XnSkeletonJointPosition jointx;

  g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,ejoint,jointx);

  if(jointx.fConfidence<0.5f){
    point->X=-9999.0f;
    point->Y=-9999.0f;
    point->Z=-9999.0f;
  }else{

    point->X=jointx.position.X;
    point->Y=jointx.position.Y;
    point->Z=jointx.position.Z;
  }
}

void gesture_ver1(XnPoint3D* Position){
    //printf("test %lf\n",Position[0].Y);
    
    char onof[16];
    const double middle_low = (Position[4+3].Y + Position[3+3].Y) / 2;
    const double middle_mid = (Position[3+3].Y + Position[2].Y) / 2;
    const double head = Position[2].Y - Position[3+3].Y;
    if(Position[4+3].Y < Position[5+3].Y){
	new_turn = 1;
	if(Position[5+3].Y < middle_low){
	    sprintf(onof,"%s","low1");
	    printf("%d,%lf,%lf,%lf,%lf,0,low1\n",count,Position[2].Y,Position[6].Y,Position[7].Y,Position[8].Y);
	    //printf("status: low1\n  hand : %lf\n  head : %lf\n  shoulder : %lf\n elbow : %lf\n",Position[5+3].Y,Position[2].Y,Position[6].Y,Position[7].Y);
	} else if(Position[5+3].Y > middle_low && Position[5+3].Y < Position[3+3].Y){
	    sprintf(onof,"%s","low2");
	    printf("%d,%lf,%lf,%lf,%lf,1,low2\n",count,Position[2].Y,Position[6].Y,Position[7].Y,Position[8].Y);
	    //printf("status: low2\n  hand : %lf\n  head : %lf\n  shoulder : %lf\n elbow : %lf\n    ",Position[5+3].Y,Position[2].Y,Position[6].Y,Position[7].Y);
	} else if(Position[5+3].Y < middle_mid){
	    sprintf(onof,"%s","mid1");
	    printf("%d,%lf,%lf,%lf,%lf,2,mid1\n",count,Position[2].Y,Position[6].Y,Position[7].Y,Position[8].Y);
	    //printf("status: mid1\n  hand : %lf\n  head : %lf\n  shoulder : %lf\n elbow : %lf\n    ",Position[5+3].Y,Position[2].Y,Position[6].Y,Position[7].Y);
	} else if(Position[5+3].Y > middle_mid && Position[5+3].Y < Position[2].Y){
	    sprintf(onof,"%s","mid2");
	    printf("%d,%lf,%lf,%lf,%lf,3,mid2\n",count,Position[2].Y,Position[6].Y,Position[7].Y,Position[8].Y);
	    //printf("status: mid2\n  hand : %lf\n  head : %lf\n  shoulder : %lf\n elbow : %lf\n    ",Position[5+3].Y,Position[2].Y,Position[6].Y,Position[7].Y);
	} else if(Position[5+3].Y < Position[2].Y + head){
	    sprintf(onof,"%s","hig1");
	    printf("%d,%lf,%lf,%lf,%lf,4,hig1\n",count,Position[2].Y,Position[6].Y,Position[7].Y,Position[8].Y);
	    //printf("status: hig1\n  hand : %lf\n  head : %lf\n  shoulder : %lf\n elbow : %lf\n    ",Position[5+3].Y,Position[2].Y,Position[6].Y,Position[7].Y);
	} else if(Position[5+3].Y > Position[2].Y + head){
	    sprintf(onof,"%s","hig2");
	    printf("%d,%lf,%lf,%lf,%lf,5,hig2\n",count,Position[2].Y,Position[6].Y,Position[7].Y,Position[8].Y);
	    //printf("status: hig2\n  hand : %lf\n  head : %lf\n  shoulder : %lf\n elbow : %lf\n    ",Position[5+3].Y,Position[2].Y,Position[6].Y,Position[7].Y);
	} 
    } else {
	sprintf(onof,"%s","no");
	new_turn = 0;
	    printf("%d,%lf,%lf,%lf,%lf,6,no\n",count,Position[2].Y,Position[6].Y,Position[7].Y,Position[8].Y);
	//printf("status: no\n  hand : %lf\n  head : %lf\n  shoulder : %lf\n elbow : %lf\n    ",Position[5+3].Y,Position[2].Y,Position[6].Y,Position[7].Y);
    }
    write(sock0,onof,4);
    
}

void glutDisplay (void){
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	xn::ImageMetaData imageMD;
	g_DepthGenerator.GetMetaData(depthMD);
	//g_ImageGenerator.GetMetaData(imageMD);
#ifndef USE_GLES
	//glOrtho(0, depthMD.XRes(), depthMD.YRes(), 0, -1.0, 1.0);
	glOrtho(0, depthMD.XRes(), depthMD.YRes(), 0, -20.0, 20.0);
#else
	glOrthof(0, depthMD.XRes(), depthMD.YRes(), 0, -1.0, 1.0);
#endif

	glDisable(GL_TEXTURE_2D);

	if (!g_bPause)
	{
		// Read next available data
	  g_Context.WaitOneUpdateAll(g_DepthGenerator);
	  g_Context.WaitAndUpdateAll();

	}
		// Process the data
		g_DepthGenerator.GetMetaData(depthMD);
		//g_ImageGenerator.GetMetaData(imageMD);
		g_UserGenerator.GetUserPixels(0, sceneMD);
		DrawDepthMap(depthMD, sceneMD);
		XnUserID aUsers[15];
		XnUInt16 nUsers = 15;
		XnPoint3D Position[15];
		XnPoint3D Real[15];
		g_UserGenerator.GetUsers(aUsers,nUsers);

		for (int i = 0; i < nUsers; ++i){
		    if(g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])){
		        Pos(aUsers[i],XN_SKEL_TORSO,&Position[0]);
		        Pos(aUsers[i],XN_SKEL_NECK,&Position[1]);
		        Pos(aUsers[i],XN_SKEL_HEAD,&Position[2]);

		        Pos(aUsers[i],XN_SKEL_LEFT_SHOULDER,&Position[3]);
		        Pos(aUsers[i],XN_SKEL_LEFT_ELBOW,&Position[4]);
		        Pos(aUsers[i],XN_SKEL_LEFT_HAND,&Position[5]);

		        Pos(aUsers[i],XN_SKEL_RIGHT_SHOULDER,&Position[6]);
		        Pos(aUsers[i],XN_SKEL_RIGHT_ELBOW,&Position[7]);
		        Pos(aUsers[i],XN_SKEL_RIGHT_HAND,&Position[8]);
/*
		        Pos(aUsers[i],XN_SKEL_LEFT_HIP,&Position[9]);
		        Pos(aUsers[i],XN_SKEL_LEFT_KNEE,&Position[10]);
		        Pos(aUsers[i],XN_SKEL_LEFT_FOOT,&Position[11]);

		        Pos(aUsers[i],XN_SKEL_RIGHT_HIP,&Position[12]);
		        Pos(aUsers[i],XN_SKEL_RIGHT_KNEE,&Position[13]);
		        Pos(aUsers[i],XN_SKEL_RIGHT_FOOT,&Position[14]);
			*/
		    }
		}
		gesture_ver1(Position);
	        count++;
#ifndef USE_GLES
	glutSwapBuffers();
#endif
}


#ifndef USE_GLES
void glutIdle (void)
{
	if (g_bQuit) {
		CleanupExit();
	}

	// Display the frame
	glutPostRedisplay();
}


void glInit (int * pargc, char ** argv)
{
        //glutInit(pargc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow ("Prime Sense User Tracker Viewer");
	//glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	//	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);

	glDisable(GL_DEPTH_TEST);
 	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}
#endif // USE_GLES

#define SAMPLE_XML_PATH "../Config/SamplesConfig.xml"

#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
		return nRetVal;												\
	}

void InitClient(int port, char *hostname){
    struct hostent *host;
    struct sockaddr_in serv;
    int s, n;
    host = gethostbyname(hostname);
    sock0 = socket(PF_INET, SOCK_STREAM, 0);
    bzero((char *)&serv, sizeof(serv));
    serv.sin_family = PF_INET;
    serv.sin_port = htons(port);
    bcopy(host->h_addr,(char *)&serv.sin_addr, host->h_length);
    if(connect(sock0, (struct sockaddr *)&serv, sizeof(serv)) < 0){
	fprintf(stderr,"cannot connect\n");
	exit(1);
    }
}

int main(int argc, char **argv)
{
        glutInit(&argc, argv); 
        srand((unsigned int)time(NULL));
	XnStatus nRetVal = XN_STATUS_OK;

	if (argc > 1)
	{
                int port;
                if((port = atoi(argv[1])) == 0){
	               printf("ERROR: 2nd augument is unavailble.\n ");
	        }
                InitClient(port,argv[2]);

		nRetVal = g_Context.Init();
		CHECK_RC(nRetVal, "Init");
		/*nRetVal = g_Context.OpenFileRecording(argv[1], g_Player);
		nRetVal = g_Context.OpenFileRecording(SAMPLE_XML_PATH);
		*/
		nRetVal = g_Context.InitFromXmlFile(SAMPLE_XML_PATH);
		/*if (nRetVal != XN_STATUS_OK)
		{
			printf("Can't open recording %s: %s\n", argv[1], xnGetStatusString(nRetVal));
			return 1;
		}*/
	}
	else
	{
		xn::EnumerationErrors errors;
		nRetVal = g_Context.InitFromXmlFile(SAMPLE_XML_PATH, g_scriptNode, &errors);
		if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
		{
			XnChar strError[1024];
			errors.ToString(strError, 1024);
			printf("%s\n", strError);
			return (nRetVal);
		}
		else if (nRetVal != XN_STATUS_OK)
		{
			printf("Open failed: %s\n", xnGetStatusString(nRetVal));
			return (nRetVal);
		}
	}

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(nRetVal, "Find depth generator");
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_UserGenerator.Create(g_Context);
		CHECK_RC(nRetVal, "Find user generator");
	}

	XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete, hPoseDetected, hCalibrationInProgress, hPoseInProgress;
	if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
		return 1;
	}
	nRetVal = g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
	CHECK_RC(nRetVal, "Register to user callbacks");
	nRetVal = g_UserGenerator.GetSkeletonCap().RegisterToCalibrationStart(UserCalibration_CalibrationStart, NULL, hCalibrationStart);
	CHECK_RC(nRetVal, "Register to calibration start");
	nRetVal = g_UserGenerator.GetSkeletonCap().RegisterToCalibrationComplete(UserCalibration_CalibrationComplete, NULL, hCalibrationComplete);
	CHECK_RC(nRetVal, "Register to calibration complete");

	if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		g_bNeedPose = TRUE;
		if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
			return 1;
		}
		nRetVal = g_UserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(UserPose_PoseDetected, NULL, hPoseDetected);
		CHECK_RC(nRetVal, "Register to Pose Detected");
		g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}

	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	nRetVal = g_UserGenerator.GetSkeletonCap().RegisterToCalibrationInProgress(MyCalibrationInProgress, NULL, hCalibrationInProgress);
	CHECK_RC(nRetVal, "Register to calibration in progress");

	nRetVal = g_UserGenerator.GetPoseDetectionCap().RegisterToPoseInProgress(MyPoseInProgress, NULL, hPoseInProgress);
	CHECK_RC(nRetVal, "Register to pose in progress");

	nRetVal = g_Context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGenerating");
		printf("head, shoulder, elbow, hand, status\n");

//#ifndef USE_GLES
	glInit(&argc, argv);
	glutMainLoop();
}   

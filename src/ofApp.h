#pragma once

#include "ofMain.h"
#include <Kinect.h>
#include <Kinect.Face.h>

class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

private:
	void	ProcessFaces();
	HRESULT	UpdateBodyData(IBody** ppBodies);
	void	DrawFaceFrameResult();

	static void ExtractFaceRotationInDegrees(const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll);

	IKinectSensor*		sensor;
	ICoordinateMapper*	coordinateMapper;
	IColorFrameReader*	colorFrameReader;
	IBodyFrameReader*	bodyFrameReader;
	IFaceFrameSource*	faceFrameSources[BODY_COUNT];
	IFaceFrameReader*	faceFrameReaders[BODY_COUNT];

	ofImage color;
	RectI*	faceRects[BODY_COUNT];
	PointF*	facePoints[BODY_COUNT][FacePointType::FacePointType_Count];
	DetectionResult* detectionResults[BODY_COUNT][FaceProperty::FaceProperty_Count];
	Vector4* faceRotations[BODY_COUNT];
	
	ofTrueTypeFont verdana30;
};

// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}
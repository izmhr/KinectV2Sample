#include "ofApp.h"

static const int        cColorWidth  = 1920;
static const int        cColorHeight = 1080;

//--------------------------------------------------------------
void ofApp::setup(){
	HRESULT hr = GetDefaultKinectSensor(&sensor);

	if(sensor)
	{
		IColorFrameSource* colorFrameSource = NULL;
		hr = sensor->Open();
		if(SUCCEEDED(hr))
		{
			hr = sensor->get_ColorFrameSource(&colorFrameSource);
		}

		if(SUCCEEDED(hr))
		{
			hr = colorFrameSource->OpenReader(&colorFrameReader);
		}
		
		SafeRelease(colorFrameSource);
	}

	if(!sensor || FAILED(hr) )
	{
		std::cout << "no kinect sensor found!" << std::endl;
	}

#ifdef USE_OF_IMAGE
	color.allocate(cColorWidth, cColorHeight, OF_IMAGE_COLOR_ALPHA);
#else
	colorBuf = new unsigned char[cColorWidth * cColorHeight * 4];
	colorTex.allocate(cColorWidth, cColorHeight, GL_RGBA);
#endif
}

//--------------------------------------------------------------
void ofApp::update(){
	if(!colorFrameReader)
	{
		return;
	}

	IColorFrame* colorFrame = NULL;
	HRESULT hr = colorFrameReader->AcquireLatestFrame(&colorFrame);

	if(SUCCEEDED(hr))
	{
		//INT64 nTime = 0;
		IFrameDescription* pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nBufferSize = 0;
#ifdef USE_OF_IMAGE
		BYTE *data = color.getPixels();
#endif

		//hr = colorFrame->get_RelativeTime(&nTime);

		if (SUCCEEDED(hr))
		{
			hr = colorFrame->get_FrameDescription(&pFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Width(&nWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Height(&nHeight);
		}

		//if (SUCCEEDED(hr))
		//{
		//	hr = colorFrame->get_RawColorImageFormat(&imageFormat);
		//}

		if (SUCCEEDED(hr))
		{
			nBufferSize = cColorWidth * cColorHeight * 4;
#ifdef USE_OF_IMAGE
			hr = colorFrame->CopyConvertedFrameDataToArray(nBufferSize, data, ColorImageFormat_Rgba);
#else
			hr = colorFrame->CopyConvertedFrameDataToArray(nBufferSize, colorBuf, ColorImageFormat_Rgba);
			colorTex.loadData(colorBuf, cColorWidth, cColorHeight, GL_RGBA);
#endif
		}
		if (SUCCEEDED(hr))
		{
#ifdef USE_OF_IMAGE
			color.update();
#endif
		}
	}

	SafeRelease(colorFrame);

	std::stringstream strm;
	strm << "fps: " << ofGetFrameRate();
	ofSetWindowTitle(strm.str());
}

//--------------------------------------------------------------
void ofApp::draw(){
#ifdef USE_OF_IMAGE
	color.draw(0, 0, 640, 360);
#else
	colorTex.draw(0, 0, 640, 360);
#endif
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

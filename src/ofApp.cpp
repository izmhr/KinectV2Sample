#include "ofApp.h"

static const int        cColorWidth  = 1920;
static const int        cColorHeight = 1080;
static const int		cDrawWidth	= 960;
static const int		cDrawHeight = 540;
static const float		cScale = (float)cDrawWidth / (float)cColorWidth;
static const double c_FaceRotationIncrementInDegrees = 5.0f;

// define the face frame features required to be computed by this application
static const DWORD c_FaceFrameFeatures =
FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
| FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
| FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
| FaceFrameFeatures::FaceFrameFeatures_Happy
| FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
| FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
| FaceFrameFeatures::FaceFrameFeatures_MouthOpen
| FaceFrameFeatures::FaceFrameFeatures_MouthMoved
| FaceFrameFeatures::FaceFrameFeatures_LookingAway
| FaceFrameFeatures::FaceFrameFeatures_Glasses
| FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;

//--------------------------------------------------------------
void ofApp::setup(){
	HRESULT hr = GetDefaultKinectSensor(&sensor);

	if(sensor)
	{
		IColorFrameSource*	colorFrameSource = NULL;
		IBodyFrameSource*	bodyFrameSource = NULL;

		hr = sensor->Open();
		if (SUCCEEDED(hr))
		{
			hr = sensor->get_CoordinateMapper(&coordinateMapper);
		}

		if (SUCCEEDED(hr))
		{
			hr = sensor->get_ColorFrameSource(&colorFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = colorFrameSource->OpenReader(&colorFrameReader);
		}

		if (SUCCEEDED(hr))
		{
			hr = sensor->get_BodyFrameSource(&bodyFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = bodyFrameSource->OpenReader(&bodyFrameReader);
		}

		if (SUCCEEDED(hr))
		{
			// create a face frame source + reader to track each body in the fov
			for (int i = 0; i < BODY_COUNT; i++)
			{
				if (SUCCEEDED(hr))
				{
					// create the face frame source by specifying the required face frame features
					hr = CreateFaceFrameSource(sensor, 0, c_FaceFrameFeatures, &faceFrameSources[i]);
				}
				if (SUCCEEDED(hr))
				{
					// open the corresponding reader
					hr = faceFrameSources[i]->OpenReader(&faceFrameReaders[i]);
				}
			}
		}
		
		SafeRelease(colorFrameSource);
		SafeRelease(bodyFrameSource);
	}

	if(!sensor || FAILED(hr) )
	{
		std::cout << "no kinect sensor found!" << std::endl;
	}

	color.allocate(cColorWidth, cColorHeight, OF_IMAGE_COLOR_ALPHA);
	for (int iFace = 0; iFace < BODY_COUNT; iFace++)
	{
		faceRects[iFace] = new RectI();
		for (int p = 0; p < FacePointType::FacePointType_Count; p++)
		{
			facePoints[iFace][p] = new PointF();
		}
		for (int p = 0; p < FaceProperty::FaceProperty_Count; p++)
		{
			detectionResults[iFace][p] = new DetectionResult();
		}
		faceRotations[iFace] = new Vector4();
	}

	ofTrueTypeFont::setGlobalDpi(72);

	verdana30.loadFont("verdana.ttf", 30, true, true);
	verdana30.setLineHeight(34.0f);
	verdana30.setLetterSpacing(1.035);
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
		BYTE *data = color.getPixels();

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
			hr = colorFrame->CopyConvertedFrameDataToArray(nBufferSize, data, ColorImageFormat_Rgba);
		}
		if (SUCCEEDED(hr))
		{
			color.update();
			ProcessFaces();
		}
	}

	SafeRelease(colorFrame);

	std::stringstream strm;
	strm << "fps: " << ofGetFrameRate();
	ofSetWindowTitle(strm.str());
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(255, 255, 255);
	color.draw(0, 0, cDrawWidth, cDrawHeight);
	
	DrawFaceFrameResult();
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

void ofApp::ProcessFaces()
{
	HRESULT hr;
	IBody* ppBodies[BODY_COUNT] = { 0 };
	bool bHaveBodyData = SUCCEEDED(UpdateBodyData(ppBodies));

	// iterate through each face reader
	for (int iFace = 0; iFace < BODY_COUNT; ++iFace)
	{
		// retrieve the latest face frame from this reader
		IFaceFrame* pFaceFrame = nullptr;
		hr = faceFrameReaders[iFace]->AcquireLatestFrame(&pFaceFrame);

		delete faceRects[iFace];
		faceRects[iFace] = new RectI();

		for (int p = 0; p < FacePointType::FacePointType_Count; p++)
		{
			delete facePoints[iFace][p];
			facePoints[iFace][p] = new PointF();
		}

		for (int p = 0; p < FaceProperty::FaceProperty_Count; p++)
		{
			delete detectionResults[iFace][p];
			detectionResults[iFace][p] = new DetectionResult();
		}

		delete faceRotations[iFace];
		faceRotations[iFace] = new Vector4();

		BOOLEAN bFaceTracked = false;
		if (SUCCEEDED(hr) && nullptr != pFaceFrame)
		{
			// check if a valid face is tracked in this face frame
			hr = pFaceFrame->get_IsTrackingIdValid(&bFaceTracked);
		}

		if (SUCCEEDED(hr))
		{
			if (bFaceTracked)
			{
				IFaceFrameResult* pFaceFrameResult = nullptr;
				//RectI faceBox = { 0 };
				PointF _facePoints[FacePointType::FacePointType_Count];
				//Vector4 faceRotation;
				DetectionResult _faceProperties[FaceProperty::FaceProperty_Count];
				//D2D1_POINT_2F faceTextLayout;

				hr = pFaceFrame->get_FaceFrameResult(&pFaceFrameResult);

				// need to verify if pFaceFrameResult contains data before trying to access it
				if (SUCCEEDED(hr) && pFaceFrameResult != nullptr)
				{
					//hr = pFaceFrameResult->get_FaceBoundingBoxInColorSpace(&faceBox);
					hr = pFaceFrameResult->get_FaceBoundingBoxInColorSpace(faceRects[iFace]);

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->GetFacePointsInColorSpace(FacePointType::FacePointType_Count, _facePoints);
						for (int p = 0; p < FacePointType::FacePointType_Count; p++)
						{
							*facePoints[iFace][p] = _facePoints[p];
						}
					}

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->get_FaceRotationQuaternion(faceRotations[iFace]);
					}

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->GetFaceProperties(FaceProperty::FaceProperty_Count, _faceProperties);
						for (int p = 0; p < FaceProperty::FaceProperty_Count; p++)
						{
							*detectionResults[iFace][p] = _faceProperties[p];
						}
					}

					if (SUCCEEDED(hr))
					{
						//hr = GetFaceTextPositionInColorSpace(ppBodies[iFace], &faceTextLayout);
					}

					if (SUCCEEDED(hr))
					{
						// draw face frame results
						//m_pDrawDataStreams->DrawFaceFrameResults(iFace, &faceBox, facePoints, &faceRotation, faceProperties, &faceTextLayout);
					}
				}

				SafeRelease(pFaceFrameResult);
			}
			else
			{
				// face tracking is not valid - attempt to fix the issue
				// a valid body is required to perform this step
				if (bHaveBodyData)
				{
					// check if the corresponding body is tracked 
					// if this is true then update the face frame source to track this body
					IBody* pBody = ppBodies[iFace];
					if (pBody != nullptr)
					{
						BOOLEAN bTracked = false;
						hr = pBody->get_IsTracked(&bTracked);

						UINT64 bodyTId;
						if (SUCCEEDED(hr) && bTracked)
						{
							// get the tracking ID of this body
							hr = pBody->get_TrackingId(&bodyTId);
							if (SUCCEEDED(hr))
							{
								// update the face frame source with the tracking ID
								faceFrameSources[iFace]->put_TrackingId(bodyTId);
							}
						}
					}
				}
			}
		}

		SafeRelease(pFaceFrame);
	}

	if (bHaveBodyData)
	{
		for (int i = 0; i < _countof(ppBodies); ++i)
		{
			SafeRelease(ppBodies[i]);
		}
	}
}

HRESULT ofApp::UpdateBodyData(IBody** ppBodies)
{
	HRESULT hr = E_FAIL;

	if (bodyFrameReader != nullptr)
	{
		IBodyFrame* pBodyFrame = nullptr;
		hr = bodyFrameReader->AcquireLatestFrame(&pBodyFrame);
		if (SUCCEEDED(hr))
		{
			hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, ppBodies);
		}
		SafeRelease(pBodyFrame);
	}

	return hr;
}

void ofApp::DrawFaceFrameResult()
{
	ofNoFill();
	ofSetColor(0, 255, 0);
	ofSetLineWidth(3);
	ofPushMatrix();
	ofScale(cScale, cScale);
	for (int iFace = 0; iFace < BODY_COUNT; iFace++)
	{
		RectI* rect = faceRects[iFace];
		if ((rect->Bottom != 0) && (rect->Left != 0))
		{
			ofRect(rect->Left, rect->Top, rect->Right - rect->Left, rect->Bottom - rect->Top);

			PointF** points = facePoints[iFace];
			for (int p = 0; p < FacePointType::FacePointType_Count; p++)
			{
				ofEllipse(points[p]->X, points[p]->Y, 10.0f, 10.0f);
			}

			std::string faceText = "";

			// extract each face property information and store it is faceText
			for (int iProperty = 0; iProperty < FaceProperty::FaceProperty_Count; iProperty++)
			{
				switch (iProperty)
				{
				case FaceProperty::FaceProperty_Happy:
					faceText += "Happy :";
					break;
				case FaceProperty::FaceProperty_Engaged:
					faceText += "Engaged :";
					break;
				case FaceProperty::FaceProperty_LeftEyeClosed:
					faceText += "LeftEyeClosed :";
					break;
				case FaceProperty::FaceProperty_RightEyeClosed:
					faceText += "RightEyeClosed :";
					break;
				case FaceProperty::FaceProperty_LookingAway:
					faceText += "LookingAway :";
					break;
				case FaceProperty::FaceProperty_MouthMoved:
					faceText += "MouthMoved :";
					break;
				case FaceProperty::FaceProperty_MouthOpen:
					faceText += "MouthOpen :";
					break;
				case FaceProperty::FaceProperty_WearingGlasses:
					faceText += "WearingGlasses :";
					break;
				default:
					break;
				}

				switch (*detectionResults[iFace][iProperty])
				{
				case DetectionResult::DetectionResult_Unknown:
					faceText += " UnKnown";
					break;
				case DetectionResult::DetectionResult_Yes:
					faceText += " Yes";
					break;

					// consider a "maybe" as a "no" to restrict 
					// the detection result refresh rate
				case DetectionResult::DetectionResult_No:
				case DetectionResult::DetectionResult_Maybe:
					faceText += " No";
					break;
				default:
					break;
				}

				faceText += "\n";
			}

			// extract face rotation in degrees as Euler angles

			int pitch, yaw, roll;
			ExtractFaceRotationInDegrees(faceRotations[iFace], &pitch, &yaw, &roll);

			faceText += "FaceYaw : " + std::to_string(yaw) + "\n";
			faceText += "FacePitch : " + std::to_string(pitch) + "\n";
			faceText += "FaceRoll : " + std::to_string(roll) + "\n";

			verdana30.drawString((std::string)faceText, rect->Left, rect->Bottom);
		}
	}
	ofPopMatrix();
}

/// <summary>
/// Converts rotation quaternion to Euler angles 
/// And then maps them to a specified range of values to control the refresh rate
/// </summary>
/// <param name="pQuaternion">face rotation quaternion</param>
/// <param name="pPitch">rotation about the X-axis</param>
/// <param name="pYaw">rotation about the Y-axis</param>
/// <param name="pRoll">rotation about the Z-axis</param>
void ofApp::ExtractFaceRotationInDegrees(const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll)
{
	double x = pQuaternion->x;
	double y = pQuaternion->y;
	double z = pQuaternion->z;
	double w = pQuaternion->w;

	// convert face rotation quaternion to Euler angles in degrees		
	double dPitch, dYaw, dRoll;
	dPitch = atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z) / PI * 180.0;
	dYaw = asin(2 * (w * y - x * z)) / PI * 180.0;
	dRoll = atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z) / PI * 180.0;

	// clamp rotation values in degrees to a specified range of values to control the refresh rate
	double increment = c_FaceRotationIncrementInDegrees;
	*pPitch = static_cast<int>(floor((dPitch + increment / 2.0 * (dPitch > 0 ? 1.0 : -1.0)) / increment) * increment);
	*pYaw = static_cast<int>(floor((dYaw + increment / 2.0 * (dYaw > 0 ? 1.0 : -1.0)) / increment) * increment);
	*pRoll = static_cast<int>(floor((dRoll + increment / 2.0 * (dRoll > 0 ? 1.0 : -1.0)) / increment) * increment);
}
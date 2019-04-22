#ifndef IDCardVeriDemo_H
#define IDCardVeriDemo_H

#include <QtWidgets/QMainWindow>

#include <mutex>

#include "ui_IDCardVeriDemo.h"
#include "cv.h"
#include "opencv/highgui.h"
#include "idcardreader.h"
#include "asvloffscreen.h"

//class IDCardReader;

class IDCardVeriDemo : public QMainWindow
{
	Q_OBJECT

public:
	explicit IDCardVeriDemo(QWidget *parent = 0);
	~IDCardVeriDemo();

	void	 frThreadFunc();

private:
	enum ShowType
	{
		NO_FACE,
		NO_IDCARD,
		CHECK_SUCCEED,
		CHECK_FAILED,
		CAMERA_DISCONNECT,
		CAMERA_RECONNECT
	};

signals:
	void signalShowText(ShowType);
	//slot
	public slots :
	void nextFrame();
	void openCamera();
	void openIdCardReader();
	void setImageInfo(CardInfo);
	void clearIDCardImageInfo();
	void showText(ShowType);

private:
	Ui::IDCardVeriDemoClass ui;

private:

	cv::Mat m_curFrame;
	cv::VideoCapture m_capture;
	QImage  m_curVideoImage;
	QTimer *m_videoCtrlTimer;
	double m_videoRate; //FPS
	MRECT m_curFaceRect;

	bool m_frThreadRun;

	IDCardReader* m_idCardReader = nullptr;
	QTimer* m_idCardInfoTimer;

	std::mutex m_videoFaceImgMutex;
	ASVLOFFSCREEN m_videoFaceImg;

	ASVLOFFSCREEN m_idCardFaceImage;
	MHandle m_hEngine = NULL;
};

QImage Mat2QImage(cv::Mat cvImg);
cv::Mat QImage2Mat(const QImage& qimage);
IplImage* QImageToIplImage(const QImage* qImage);
void getImageRect(IplImage* orgImage, CvRect rectInImage, IplImage* imgRect);
MRESULT faceFeatureCompare(MHandle& ficHandle, MFloat& pSimilarScore);

#endif // IDCardVeriDemo_H

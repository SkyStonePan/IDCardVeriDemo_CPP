#include "IDCardVeriDemo.h"
#include <QFileDialog>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QBuffer>

#include <thread>
#include <mutex>
#include <string>

#include "amcomdef.h"
#include "arcsoft_idcardveri.h"
#include "merror.h"

#ifdef _WIN64

#define APPID  ""
#define SDKKey ""

#else _WIN32

#define APPID  ""
#define SDKKey ""

#endif

#pragma comment(lib,"libarcsoft_idcardveri.lib")

using namespace cv;

#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480

std::mutex g_mutex;
bool g_newFace = false;
bool g_newIdCard = false;

IDCardVeriDemo::IDCardVeriDemo(QWidget *parent)
	: QMainWindow(parent),
	m_videoFaceImg({ 0 }),
	m_idCardFaceImage({ 0 }),
	m_curFaceRect({ 0, 0, 0, 0 }),
	m_frThreadRun(false)
{
	ui.setupUi(this);
	this->setWindowIcon(QIcon(":/arcsoft.ico"));

	QRegExp rx("0.[0-9][0-9]");
	QRegExpValidator *validator = new QRegExpValidator(rx, this);
	ui.thresholdEdit->setValidator(validator);

	//激活
	MRESULT res = ArcSoft_FIC_Activate(APPID, SDKKey);
	if (res != MOK)
	{
		qDebug() << "Activate SDK failed, error code: " << res;
		ui.msgLabel->setText(QStringLiteral("激活失败"));
	}
	else
	{
		ui.msgLabel->setText(QStringLiteral("激活成功"));
	}

	/* 初始化人证比对引擎*/
	res = ArcSoft_FIC_InitialEngine(&m_hEngine);
	if (res != MOK)
	{
		qDebug() << "Initial Engine failed, error code: " << res;
		ui.msgLabel->setText(QStringLiteral("初始化失败"));
	}
	else
	{
		ui.msgLabel->setText(QStringLiteral("初始化成功"));
	}

	//注意：视频图像内容后面传入，这里设置基本参数
	m_videoFaceImg.i32Width = CAMERA_WIDTH;
	m_videoFaceImg.i32Height = CAMERA_HEIGHT;
	m_videoFaceImg.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
	m_videoFaceImg.pi32Pitch[0] = m_videoFaceImg.i32Width * 3;
	m_videoFaceImg.ppu8Plane[0] = (MUInt8*)malloc(sizeof(MUInt8)*m_videoFaceImg.pi32Pitch[0] * m_videoFaceImg.i32Height);

	//注意：身份证图像内容后面传入，这里设置基本参数
	m_idCardFaceImage.i32Width = 100;
	m_idCardFaceImage.i32Height = 126;
	m_idCardFaceImage.u32PixelArrayFormat = ASVL_PAF_RGB24_B8G8R8;
	m_idCardFaceImage.pi32Pitch[0] = m_idCardFaceImage.i32Width * 3;


	m_idCardInfoTimer = new QTimer;
	m_idCardInfoTimer->setInterval(3000);
	connect(m_idCardInfoTimer, SIGNAL(timeout()), this, SLOT(clearIDCardImageInfo()));

	//
	openCamera();
	openIdCardReader();

	qRegisterMetaType<ShowType>("ShowType");

	QObject::connect(this, SIGNAL(signalShowText(ShowType)), this, SLOT(showText(ShowType)));
}

IDCardVeriDemo::~IDCardVeriDemo()
{
	m_frThreadRun = false;

	std::this_thread::sleep_for(std::chrono::seconds(1));

	MRESULT res = ArcSoft_FIC_UninitialEngine(m_hEngine);
	if (res != MOK)
	{
		qDebug() << "Uninitial Engine failed, error code: " << res;
	}

	if (m_idCardReader != nullptr)
	{
		m_idCardReader->~IDCardReader();
	}

	free(m_videoFaceImg.ppu8Plane[0]);
}

void IDCardVeriDemo::frThreadFunc()
{

	LPAFIC_FSDK_FACERES pFaceRes = (LPAFIC_FSDK_FACERES)malloc(sizeof(AFIC_FSDK_FACERES));

	while (m_frThreadRun)
	{
		MRESULT res = MOK;
		{
			std::lock_guard<std::mutex> locker(m_videoFaceImgMutex);

			m_curFaceRect = { 0, 0, 0, 0 };
			//提取视频帧特征
			res = ArcSoft_FIC_FaceDataFeatureExtraction(m_hEngine, 1, &m_videoFaceImg, pFaceRes);

			if (MOK != res || pFaceRes->nFace == 0)
			{
				qDebug() << "ArcSoft_FIC_FaceDataFeatureExtraction failed, error code: " << res;
				emit signalShowText(NO_FACE);
				continue;
			}

			m_curFaceRect = pFaceRes->rcFace;
		}

		MFloat pSimilarScore = 0.0f;
		if (g_newIdCard)
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			//compare
			if ((res = faceFeatureCompare(m_hEngine, pSimilarScore)) != MOK)
			{
				qDebug() << "faceFeatureCompare failed, error code: " << res;
				continue;
			}
		}
		else
		{
			emit signalShowText(NO_IDCARD);
			continue;
		}

		if (pSimilarScore > ui.thresholdEdit->text().toFloat())
		{
			emit signalShowText(CHECK_SUCCEED);
		}
		else
		{
			emit signalShowText(CHECK_FAILED);
		}
	}

	free(pFaceRes);
}

void IDCardVeriDemo::openCamera()
{
	if (!m_capture.isOpened())
	{
		m_capture.open(0);
		bool setRes = m_capture.set(CV_CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
		setRes = m_capture.set(CV_CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
	}

	if (m_capture.isOpened())
	{
		int width = m_capture.get(CV_CAP_PROP_FRAME_WIDTH);
		int height = m_capture.get(CV_CAP_PROP_FRAME_HEIGHT);
		m_videoRate = m_capture.get(CV_CAP_PROP_FPS);

		if (width != CAMERA_WIDTH)
		{
			QMessageBox msgBox;
			msgBox.setText(QStringLiteral("摄像头设置失败"));
			msgBox.exec();
			return;
		}

		if ((int)m_videoRate == 0)
			m_videoRate = 30;
		m_capture >> m_curFrame;

		if (!m_curFrame.empty())
		{
			//视频显示
			m_videoCtrlTimer = new QTimer(this);
			m_videoCtrlTimer->setInterval(1000 / m_videoRate);
			connect(m_videoCtrlTimer, SIGNAL(timeout()), this, SLOT(nextFrame()));
			m_videoCtrlTimer->start();

			m_frThreadRun = true;
		}
	}

	std::thread frThread([=]()
	{
		frThreadFunc();
	});

	frThread.detach();
}

void IDCardVeriDemo::nextFrame()
{
	m_capture >> m_curFrame;
	if (!m_curFrame.empty())
	{
		//传入视频帧内容
		{
			std::lock_guard<std::mutex> locker(m_videoFaceImgMutex);
			memcpy(m_videoFaceImg.ppu8Plane[0], m_curFrame.data,
				sizeof(MUInt8)*m_videoFaceImg.pi32Pitch[0] * m_videoFaceImg.i32Height);

			//画人脸框
			rectangle(m_curFrame, cvPoint(m_curFaceRect.left, m_curFaceRect.top),
				cvPoint(m_curFaceRect.right, m_curFaceRect.bottom), cvScalar(0, 0, 255));

		}

		m_curVideoImage = Mat2QImage(m_curFrame);

		QPixmap videoPixmap = QPixmap::fromImage(m_curVideoImage);
		videoPixmap.scaled(ui.labelVideo->size(), Qt::KeepAspectRatio);
		ui.labelVideo->setScaledContents(true);
		ui.labelVideo->setPixmap(videoPixmap);
	}
	else
	{
		if (!m_capture.open(0))
		{
			emit signalShowText(CAMERA_DISCONNECT);
			return;
		}
		else
		{
			emit signalShowText(CAMERA_RECONNECT);
		}
	}

}

void IDCardVeriDemo::openIdCardReader()
{
	if (m_idCardReader == NULL)
	{
		m_idCardReader = new IDCardReader();
	}

	connect(m_idCardReader, SIGNAL(signalReadFinished(CardInfo)), this, SLOT(setImageInfo(CardInfo)));

	if (m_idCardReader->initReader())
	{
		std::thread readThread(&IDCardReader::readCard, m_idCardReader);
		readThread.detach();
	}
	else
	{
		QMessageBox msgBox;
		msgBox.setText(QStringLiteral("初始化阅读器失败"));
		msgBox.exec();
	}
}

void IDCardVeriDemo::setImageInfo(CardInfo stu)
{
	qDebug() << QStringLiteral("收到信息了");
	ui.labelIdcardPic->setPixmap(stu.imagePixmap);
	ui.labelName->setText(stu.name);
	QString maskSerialNum = stu.ID.replace(6, 8, "********");
	ui.labelSerialNumber->setText(maskSerialNum);//做mask

	m_idCardInfoTimer->start();

	IplImage* temp = QImageToIplImage(&stu.imagePixmap.toImage());

	//以下是将图片转换成宽能被4整除的图片
	Rect rect(1, 0, temp->width&(~3), temp->height);
	IplImage* image_roi = cvCreateImage(cvSize(rect.width, rect.height), temp->depth, temp->nChannels);
	getImageRect(temp, rect, image_roi);

	//传入身份证照片内容
	m_idCardFaceImage.ppu8Plane[0] = (MUInt8*)image_roi->imageData;
	//提取身份证照片特征
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		MRESULT res = ArcSoft_FIC_IdCardDataFeatureExtraction(m_hEngine, &m_idCardFaceImage);

		if (MOK == res)
		{
			g_newIdCard = true;
		}
	}

	cvReleaseImage(&image_roi);
	cvReleaseImage(&temp);
}

void IDCardVeriDemo::clearIDCardImageInfo()
{
	ui.labelIdcardPic->setPixmap(QPixmap());
	ui.labelName->clear();
	ui.labelSerialNumber->clear();
	g_newIdCard = false;
}

void IDCardVeriDemo::showText(ShowType showType)
{
	switch (showType)
	{
	case IDCardVeriDemo::NO_FACE:
		ui.msgLabel->setStyleSheet("color:#ff0000;");
		ui.msgLabel->setText(QStringLiteral("请正对摄像机!"));
		break;
	case IDCardVeriDemo::NO_IDCARD:
		ui.msgLabel->setStyleSheet("color:#ff0000;");
		ui.msgLabel->setText(QStringLiteral("请放置身份证!"));
		break;
	case IDCardVeriDemo::CHECK_SUCCEED:
		ui.msgLabel->setStyleSheet("color:#00ff00;");
		ui.msgLabel->setText(QStringLiteral("人证核验成功!"));
		break;
	case IDCardVeriDemo::CHECK_FAILED:
		ui.msgLabel->setStyleSheet("color:#ff0000;");
		ui.msgLabel->setText(QStringLiteral("人证核验失败!"));
		break;
	case CAMERA_DISCONNECT:
		ui.labelVideo->clear();
		ui.msgLabel->setStyleSheet("color:#ff0000;");
		ui.msgLabel->setText(QStringLiteral("摄像头已断开!"));
		m_frThreadRun = false;
		break;

	case CAMERA_RECONNECT:
		ui.msgLabel->setText(QStringLiteral("摄像头已重连!"));

		m_frThreadRun = true;

		std::thread frThread([=]()
		{
			frThreadFunc();
		});

		frThread.detach();
		break;
	}
}

MRESULT faceFeatureCompare(MHandle& ficHandle, MFloat& pSimilarScore)
{
	/* 人证比对 */
	MInt32 pResult = 0;
	//参考阈值
	MFloat g_threshold = 0.82f;

	MRESULT res = ArcSoft_FIC_FaceIdCardCompare(ficHandle, g_threshold, &pSimilarScore, &pResult);
	if (res != MOK)
	{
		qDebug() << "ArcSoft_FIC_FaceIdCardCompare failed, error code: " << res;
		return -1;
	}

	qDebug("%.4f", pSimilarScore);

	return MOK;
}

QImage Mat2QImage(cv::Mat cvImg)
{
	QImage qImg;
	if (cvImg.channels() == 3)                             //3 channels color image
	{
		cv::cvtColor(cvImg, cvImg, CV_BGR2RGB);
		qImg = QImage((const unsigned char*)(cvImg.data),
			cvImg.cols, cvImg.rows,
			cvImg.cols*cvImg.channels(),
			QImage::Format_RGB888);
	}
	else if (cvImg.channels() == 1)                    //grayscale image
	{
		qImg = QImage((const unsigned char*)(cvImg.data),
			cvImg.cols, cvImg.rows,
			cvImg.cols*cvImg.channels(),
			QImage::Format_Indexed8);
	}
	else
	{
		qImg = QImage((const unsigned char*)(cvImg.data),
			cvImg.cols, cvImg.rows,
			cvImg.cols*cvImg.channels(),
			QImage::Format_RGB888);
	}

	return qImg;

}

Mat QImage2Mat(const QImage& qimage)
{
	cv::Mat mat = cv::Mat(qimage.height(), qimage.width(), CV_8UC4, (uchar*)qimage.bits(), qimage.bytesPerLine());
	cv::Mat mat2 = cv::Mat(mat.rows, mat.cols, CV_8UC3);
	int from_to[] = { 0, 0, 1, 1, 2, 2 };
	cv::mixChannels(&mat, 1, &mat2, 1, from_to, 3);
	return mat2;
}

IplImage *QImageToIplImage(const QImage* qImage)
{
	int width = qImage->width();
	int height = qImage->height();
	CvSize Size;
	Size.height = height;
	Size.width = width;
	IplImage *IplImageBuffer = cvCreateImage(Size, IPL_DEPTH_8U, 3);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			QRgb rgb = qImage->pixel(x, y);
			CV_IMAGE_ELEM(IplImageBuffer, uchar, y, x * 3 + 0) = qBlue(rgb);
			CV_IMAGE_ELEM(IplImageBuffer, uchar, y, x * 3 + 1) = qGreen(rgb);
			CV_IMAGE_ELEM(IplImageBuffer, uchar, y, x * 3 + 2) = qRed(rgb);
		}
	}
	return IplImageBuffer;
}

void getImageRect(IplImage* orgImage, CvRect rectInImage, IplImage* imgRect)
{
	IplImage *result = imgRect;
	CvSize size;
	size.width = rectInImage.width;
	size.height = rectInImage.height;
	cvSetImageROI(orgImage, rectInImage);
	cvCopy(orgImage, result);
	cvResetImageROI(orgImage);
}
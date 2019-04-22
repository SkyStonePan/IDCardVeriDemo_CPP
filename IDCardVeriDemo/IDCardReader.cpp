#include "idcardreader.h"


IDCardReader::IDCardReader(QObject *parent)
	: QObject(parent)
{

}

IDCardReader::IDCardReader()
{
	connect(this, SIGNAL(signalReadFinished()), this, SLOT(readCardInfo()));//绑定读取信息
	qRegisterMetaType<CardInfo>("CardInfo");

}

IDCardReader::~IDCardReader()
{

}


bool IDCardReader::initReader()
{
	//TODO
	//初始化

	return false;
}


bool IDCardReader::readCardInfo()
{
	//TODO 
	//读取身份证信息

	return false;
}

void IDCardReader::readCard()
{
	//TODO
	//读卡，尝试读，读失败尝试认证，认证成功再读一次

}

#ifndef IDCARDREADER_H
#define IDCARDREADER_H

#include <QObject>
#include <QPixmap>

//身份证信息
struct CardInfo
{
	QString name;
	QString ID;
	QPixmap imagePixmap;
};

class IDCardReader : public QObject
{
	Q_OBJECT

public:
	explicit IDCardReader(QObject *parent);
	IDCardReader();
	~IDCardReader();
	bool initReader();
	void readCard();
	
	private slots:
	bool readCardInfo();

signals:
	void signalReadFinished(CardInfo);
	void signalReadError();

};

#endif // IDCARDREADER_H

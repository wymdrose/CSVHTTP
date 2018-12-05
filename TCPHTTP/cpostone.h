#ifndef CPOSTONE_H
#define CPOSTONE_H

#include <QObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

class CPostOne : public QObject
{
	Q_OBJECT

public:
	CPostOne(QString tPath, QString tIsn, int descriptor, QObject *parent);
	~CPostOne();

signals:
	void isncheckSignal(int, const int, QString);

private slots:
	void replyFinished(QNetworkReply *);

public:
	void isncheck();
	void isncheckMsgEncode();
	void CPostOne::loginMsgEncode();
	void CPostOne::login();
	bool CPostOne::xmlIsnCheckDecode(QString xmlCode, QString& reInfo);

	QString tPath;

private:
	int descriptor;

	QNetworkAccessManager *manager;
	QNetworkReply *reply;
	
	QString ISN;
	QString upMsg;

	QString tDevice;
	QString tUrl = "http://sfisws.ch.casetekcorp.com/SFISWebService/SFISTSPWebService.asmx";
	QString tUser = "TSP_NLSPAD";

	QString reMsg;
	QString reInfo;
};

#endif // CPOSTONE_H

#ifndef TCPHTTP_H
#define TCPHTTP_H

#include <QtWidgets/QMainWindow>
#include "ui_tcphttp.h"

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#include <QSettings>
#include <QTimer>

#include "server.h"

class TCPHTTP : public QMainWindow
{
	Q_OBJECT

public:
	TCPHTTP(QWidget *parent = 0);
	~TCPHTTP();


signals:
	void tcpSignal(const QString, int);

private slots:
	void isncheckSlot(int, const int,QString);
	void stopSlot(int, QString);
	void onTimerlog();

public slots:
	void updateServer(QString, int, int);

//
signals:
	void stopSignal(int, const QString);
	void stepSignal(QString*);
	
	void pdcaSignal(const QString);

private slots:
	void replyFinished(QNetworkReply *);
	void tcpSlot(const QString, int);
	void stepSlot1(QString*);
	
	void macReSlot(const QString);

public:
	const QString tPath = qApp->applicationDirPath();
	const QString PASS = "1";
	const QString LOGIN_TWINCE = "Login Twice";
	const int CODE_PASSED = 1;
	const int ERROR_CODE_FAILED = 89;
	const int ERROR_CODE_LOGIN = 81;
	const int ERROR_CODE_NET = 80;
	const int ERROR_CODE_TIMEOUT = 99;

public:
	enum WTSP_LOGINOUT
	{
		LOG_IN = 1,
		LOG_OUT = 2
	};

	
	void loginout(WTSP_LOGINOUT inORout, QString device, QString tUser);
	void inputdata(QString device, QString data);
	void postTest();
	bool xmlLoginoutDecode(QString xmlCode);
	bool xmlInputDecode(QString xmlCode, QString& reInfo);

	bool TCPHTTP::curStateDecode(QString* pCurState, QString reMsg);

	void closeEvent(QCloseEvent *event);

private:
	int port;
	Server *server;

private:
	QString tUrl;
	QString tDevice;
	bool	bProErr = false;
	QString tUser;
	QString tErr;
	QString lastHsg;

	QStringList mStatusList;
	QStringList mDataList;
	int	mDataNum;
	
	QString* pCurState;

	QMutex m_mutex;

	int curErrorCode = 0;

	unsigned int recvISN = 0;
	unsigned int okISN = 0;
	unsigned int ngISN = 0;

	unsigned int recvNum = 0;
	unsigned int okNum = 0;
	unsigned int ngNum = 0;
	QTimer *logTimer;

private:
	int descriptor;
	QNetworkAccessManager *manager;
	QNetworkReply *reply;

	Ui::TCPHTTPClass ui;
};

#endif // TCPHTTP_H

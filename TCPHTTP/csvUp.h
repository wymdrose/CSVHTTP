#ifndef CSVUP_H
#define CSVUP_H

#include <QDialog>
#include "ui_csvUp.h"
#include <QThread>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>


class csvUp : public QDialog
{
	Q_OBJECT

public:
	csvUp(QWidget *parent = 0);
	~csvUp();


private slots:
	void replyFinished(QNetworkReply *);
	void uiShowSlot(QString);

public slots:
	void onFileTimer();

public:
	QString tPath = qApp->applicationDirPath();
	QString tDevice;
	QString tUrl = "http://sfisws.ch.casetekcorp.com/SFISWebService/SFISTSPWebService.asmx";
	QString tUser = "TSP_NLSPAD";

	const QString PASS = "1";
	const QString LOGIN_TWINCE = "Login Twice";
	const int CODE_PASSED = 1;
	const int ERROR_CODE_FAILED = 89;
	const int ERROR_CODE_LOGIN = 81;
	const int ERROR_CODE_NET = 80;
	const int ERROR_CODE_TIMEOUT = 99;
	

	bool  netFlag = false;
	bool  doneFlag = false;
	bool  passFlag = false;
	bool  lwmFlag = false;
	int tryTime = 0;
	QTimer *fileTimer;

	enum WTSP_LOGINOUT
	{
		LOG_IN = 1,
		LOG_OUT = 2
	};

	void loginout(WTSP_LOGINOUT inORout, QString device, QString tUser);
	void inputdata(QString device, QString data);
	void lwmdata(QString device, QString dataIsn, QString dataFixId, QString dataLwm, QString lwmType);
	void postTest();
	bool xmlLoginoutDecode(QString xmlCode);
	bool xmlInputDecode(QString xmlCode, QString& reInfo);
	bool xmlLwmDecode(QString xmlCode, QString& reInfo);
	bool curStateDecode(QString* pCurState, QString reMsg);
	
	void closeEvent(QCloseEvent *event);


private:


	bool	bProErr = false;
	QString tErr;
	QString lastHsg;

	int curErrorCode = 0;

	QNetworkAccessManager *p_manager;
	QNetworkReply *reply;

	Ui::csvUp ui;
};


class Worker : public QObject
{
	Q_OBJECT
public:
	Worker(csvUp* p_csvUp) : pm_csvUp(p_csvUp)
	{
		m_thread.start();
		this->moveToThread(&m_thread);
	}
	

signals:
	void uiShowSignal(QString);

	protected slots:
	void csvUpThread();
	//	void fun2();

public:
	QThread m_thread;
	csvUp* pm_csvUp;
};



#endif // CSVUP_H

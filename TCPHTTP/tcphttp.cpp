
#include "cpostone.h"
#include "tcphttp.h"
#include "QXmlStreamReader"
#include "QFile"
#include "QDir"
#include "QMessageBox"

TCPHTTP::TCPHTTP(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	//
//	ui.textBrowser->document()->setMaximumBlockCount(1000);

	Qt::WindowFlags flags = Qt::Dialog;
	flags |= Qt::WindowMinMaxButtonsHint;
	//	flags |= Qt::WindowCloseButtonHint;
	setWindowFlags(flags);

	//
	QSettings cfgIni(tPath + "/cfg.ini", QSettings::IniFormat);
	
	port = cfgIni.value("/TcpServer/PORT").toInt();


	mDataNum = cfgIni.value("/DATANUM/mDataNum").toInt();

	tUrl = cfgIni.value("/Url/SOAP_WSDL").toString();
	tDevice = cfgIni.value("/DEVICE/tDevice").toString();
	//
	mStatusList.append("WTSP_NULL");
	mStatusList.append("WTSP_LOG_IN");
	mStatusList.append("WTSP_INPUT_ERR");
	
	for (size_t i = 0; i < mDataNum; i++)
	{
		mStatusList.append(QString("WTSP_INPUT_DATA%1").arg(i + 1));
	}

	mStatusList.append("WTSP_LOG_OUT");

	mStatusList.append("FINISH");

	pCurState = &mStatusList.first();
	
	/*
	QPalette pal;
	pal = ui.textBrowser->palette();
	pal.setColor(QPalette::Base, QColor(0, 0, 255));//改变背景色
	*/
	connect(this, SIGNAL(stepSignal(QString*)), this, SLOT(stepSlot1(QString*)));
	
	manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

	//
	connect(this, SIGNAL(tcpSignal(const QString, int)), this, SLOT(tcpSlot(const QString, int)));
	connect(this, SIGNAL(stopSignal(int, QString)), this, SLOT(stopSlot(int, QString)));

	server = new Server(this, port);				//创建一个Server对象
	connect(server, SIGNAL(updateServer(QString, int, int)), this,
		SLOT(updateServer(QString, int, int)));	//(a)


	logTimer = new QTimer(this);
	connect(logTimer, SIGNAL(timeout()), this, SLOT(onTimerlog()));
	logTimer->start(1 * 1000 * 60);
}

TCPHTTP::~TCPHTTP()
{

}

void TCPHTTP::onTimerlog()
{
	QDateTime datetime = QDateTime::currentDateTime();
	if (datetime.toString("hh:mm") == "00:00")
	{
		//do something
		QString saveText = ui.textBrowser->toPlainText();

		QDateTime time = QDateTime::currentDateTime();
		QString txtName = time.toString("yyyy-MM-dd");

		//
		QDir *folder = new QDir;

		bool exist = folder->exists(tPath + "/Log");
		if (!exist)
		{
			folder->mkdir(tPath + "/Log");
		}

		QString savePath = tPath + "/Log/" + txtName + ".txt";

		QFile file(savePath);

		auto a = file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);

		QTextStream out(&file);

		out << saveText << "\n" << "\n";

		file.flush();
		file.close();

		ui.textBrowser->clear();
	}
}

void TCPHTTP::closeEvent(QCloseEvent *event)
{
	QString saveISN = ui.textBrowser_ISN->toPlainText();
	QString saveText = ui.textBrowser->toPlainText();

	QDateTime time = QDateTime::currentDateTime();
	QString txtName = time.toString("yyyy-MM-dd");

	//
	QDir *folder = new QDir;

	bool exist = folder->exists(tPath + "/Log");
	if (!exist)
	{	
		folder->mkdir(tPath + "/Log");
	}
	
	//
	QFile fileISN(tPath + "/Log/" + "ISN" + ".txt");
	fileISN.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);

	QTextStream ISN(&fileISN);

	ISN << saveISN << "\n" << "\n";

	//
	QString savePath = tPath + "/Log/" + txtName + ".txt";
	
	QFile file(savePath);
	
	auto a = file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);

	QTextStream out(&file);	

	out << saveText << "\n" << "\n";

	file.flush();
	file.close();
}

void TCPHTTP::updateServer(QString msg, int length, int descriptor)
{
	if (msg.right(15) != "Enter Chat Room")
	{
		tcpSignal(msg.left(length), descriptor);
	}

}

void TCPHTTP::stopSlot(int descriptor, QString stopMsg)
{
//	pCurState = &mStatusList.first();

	QString tQString;

	if ("11" == stopMsg)
	{
		okISN++;
		tQString = QString("okISN: %1").arg(okISN);
		ui.textBrowser_ISN->append(tQString);
	}
	else if ("22" == stopMsg)
	{
		ngISN++;
		tQString = QString("ngISN: %1").arg(ngISN);
		ui.textBrowser_ISN->append(tQString);
	}
	
	else if (QString("%1").arg(CODE_PASSED) == stopMsg )
	{
		okNum++;
		tQString = QString("okNum: %1").arg(okNum);	
		ui.textBrowser->append(tQString);
	}
	else
	{
		ngNum++;
		tQString = QString("ngNum: %1").arg(ngNum);
		ui.textBrowser->append(tQString);
	}
	
	//
	stopMsg = QString("11");

	//
	for (int i = 0; i< server->tcpClientSocketList.count(); i++)
	{
		QTcpSocket *item = server->tcpClientSocketList.at(i);
		
		auto a = item->socketDescriptor();
		if (item->socketDescriptor() == descriptor)
		{
			item->write(stopMsg.toLatin1());
		}
	}
}

void TCPHTTP::isncheckSlot(int descriptor, const int reVal, QString reInfo)
{
	ui.textBrowser_ISN->append(reInfo);

	QApplication::processEvents();

	stopSignal(descriptor, QString("%1").arg(reVal));
}

bool TCPHTTP::xmlLoginoutDecode(QString xmlCode)
{
	QXmlStreamReader reader(xmlCode);

	reader.setNamespaceProcessing(false);

	while (!reader.atEnd())
	{
		if (reader.isStartElement())
		{
			if (reader.name() == "WTSP_LOGINOUTResult")
			{
				auto re = reader.readElementText();
				auto a = re.mid(2, 11);
				if (PASS == re.left(1) || LOGIN_TWINCE == re.mid(2,11))
				{
					return true;
				}

			}

		}
		else if (reader.isEndElement() && reader.name() == "soap:Envelope")
		{
			
		}

		reader.readNext();
	}

	return false;
}

bool TCPHTTP::xmlInputDecode(QString xmlCode, QString& reInfo)
{
	QXmlStreamReader reader(xmlCode);

	reader.setNamespaceProcessing(false);

	while (!reader.atEnd())
	{
		if (reader.isStartElement())
		{
			if (reader.name() == "WTSP_SSD_INPUTDATAResult")
			{
				reInfo = reader.readElementText();

				if (reInfo.left(1) == PASS)
				{
					return true;
				}

			}

		}
		else if (reader.isEndElement() && reader.name() == "soap:Envelope")
		{

		}

		reader.readNext();
	}

	return false;
}

bool TCPHTTP::curStateDecode(QString* pCurState, QString reMsg)
{
	QString reInfo;

	if ("WTSP_LOG_IN" == *pCurState || "WTSP_LOG_OUT" == *pCurState)
	{
		if (true == xmlLoginoutDecode(reMsg))
		{
			ui.textBrowser->append("<font color = green> Success! </font>"); QApplication::processEvents();
			return true;
		}
		else
		{
			curErrorCode = ERROR_CODE_LOGIN;
		}
	}
	else
	{
	
		if (true == xmlInputDecode(reMsg, reInfo))
		{
			ui.textBrowser->append("<font color = green> Success! </font>"); QApplication::processEvents();
			return true;
		}
		else
		{
			curErrorCode = ERROR_CODE_FAILED;
			ui.textBrowser->append(reInfo); QApplication::processEvents();
		}
	}
	
	//if (reInfo == "")

	ui.textBrowser->append("<font color = red> Failed! </font>"); QApplication::processEvents();
	return false;
}

void TCPHTTP::replyFinished(QNetworkReply *reply)
{
	//DELAY
	if (reply->error() != QNetworkReply::NoError)
	{
		QByteArray bytes = reply->readAll();
		QString string = QString::fromUtf8(bytes);

		qDebug() << "handle errors here";
		QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
		//statusCodeV是HTTP服务器的相应码，reply->error()是Qt定义的错误码，可以参考QT的文档
		qDebug("found error ....code: %d %d\n", statusCodeV.toInt(), (int)reply->error());
		qDebug(qPrintable(reply->errorString()));

		stopSignal(this->descriptor, QString("%1").arg(ERROR_CODE_NET));
	
	}
	else
	{
		QByteArray bytes = reply->readAll();
		qDebug() << bytes;
		QString reMsg = QString::fromUtf8(bytes);



		if (false == curStateDecode(pCurState, reMsg))
		{
			stopSignal(this->descriptor, QString("%1").arg(curErrorCode));
		}
		else
		{
			if (false == bProErr && "WTSP_LOG_IN" == *pCurState)
			{
				pCurState++;
			}

			pCurState++;

			stepSignal(pCurState);

			if ("FINISH" == *pCurState)
			{
				stopSignal(this->descriptor, QString("%1").arg(CODE_PASSED));
			}
		}
	}

	reply->deleteLater();
}


static void loginoutMsgEncode(QString device, QString tUser, QString para, QString & msg)
{
	msg.clear();

	msg += "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"\r";
	msg += "xmlns:sfis=\"http://www.pegatroncorp.com/SFISWebService/\">\r";
	msg += "<soap:Header/>\r";
	msg += "<soap:Body>\r";
	msg += "<sfis:WTSP_LOGINOUT>\r";

	msg += "<!--Optional:-->\r";

	msg += "<sfis:programId>TSP_NLSPAD</sfis:programId>\r";			// = "TSP_NLSPAD"
	msg += "<!--Optional:-->\r";
	msg += "<sfis:programPassword>N3eaM;</sfis:programPassword>\r";
	msg += "<!--Optional:-->\r";

	msg += "<sfis:op>";
	msg += tUser;
	msg += "</sfis:op>\r";

	msg += "<!--Optional:-->\r";
	msg += "<sfis:password></sfis:password>\r";
	msg += "<!--Optional:-->\r";

	msg += "<sfis:device>";
	msg += device;
	msg += "</sfis:device>\r";      // DEVICE  由铠嘉提供

	msg += "<!--Optional:-->\r";
	msg += "<sfis:TSP>LINK</sfis:TSP>\r";

	msg += "<sfis:status>";
	msg += para;
	msg += "</sfis:status>\r";          //  1代表登录, 2代表登出

	msg += "</sfis:WTSP_LOGINOUT>\r";
	msg += "</soap:Body>\r";
	msg += "</soap:Envelope>\r";
}

static void inputdataMsgEncode(QString device, QString data, QString & msg)
{
	msg.clear();

	msg += "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"\r";
	msg += "xmlns:sfis=\"http://www.pegatroncorp.com/SFISWebService/\">\r";
	msg += "<soap:Header/>\r";
	msg += "<soap:Body>\r";
	msg += "<sfis:WTSP_SSD_INPUTDATA>\r";
	msg += "<!--Optional:-->\r";
	msg += "<sfis:programId>TSP_NLSPAD</sfis:programId>\r";
	msg += "<!--Optional:-->\r";
	msg += "<sfis:programPassword>N3eaM;</sfis:programPassword>\r";
	msg += "<!--Optional:-->\r";

	msg += "<sfis:device>";
	msg += device;
	msg += "</sfis:device>\r";      // DEVICE  由铠嘉提供

	msg += "<!--ISN:-->\r";

	msg += "<sfis:data>";
	msg += data;				// KJ180910001JKKX4X48
	msg += "</sfis:data>\r";		//DATA ：error / 线别 / ISN等

	msg += "<!--Optional:-->\r";
	msg += "<sfis:type>1</sfis:type>\r";
	msg += "</sfis:WTSP_SSD_INPUTDATA>\r";
	msg += "</soap:Body>\r";
	msg += "</soap:Envelope>\r";
}


void TCPHTTP::loginout(WTSP_LOGINOUT inORout, QString device, QString tUser)
{
	//POST
	QNetworkRequest request;

	request.setUrl(QUrl(tUrl));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Content-Type", "application/soap+xml;setchar-utf8");

	QString upMsg;

	loginoutMsgEncode(device, tUser, QString("%1").arg(inORout), upMsg);

	QByteArray  postData;
	postData.append(upMsg);

	reply = manager->post(request, postData);

	qDebug() << reply->error();
}

void TCPHTTP::inputdata(QString device, QString data)
{
	//POST
	QNetworkRequest request;

	request.setUrl(QUrl(tUrl));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Content-Type", "application/soap+xml;setchar-utf8");

	QString upMsg;

	inputdataMsgEncode(device, data, upMsg);

	QByteArray  postData;
	postData.append(upMsg);

	reply = manager->post(request, postData);

	qDebug() << reply->error();
}

void TCPHTTP::postTest()
{
	QSettings cfgIni(tPath + "/cfg.ini", QSettings::IniFormat);

	QString tUrl = cfgIni.value("/Url/SOAP_WSDL").toString();

	//POST
	QNetworkRequest request;

	request.setUrl(QUrl(tUrl));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//	request.setRawHeader("Content-Type", "application/soap+xml;setchar-utf8");

	QString upMsg = "userkey=ff9bf77c827b4a4f86f4bd6b20ea9989";

	QByteArray  postData;
	postData.append(upMsg);

	reply = manager->post(request, postData);

	qDebug() << reply->error();
}

void TCPHTTP::tcpSlot(const QString upData, int descriptor)
{
	QApplication::processEvents();
	
	//
	QDateTime time = QDateTime::currentDateTime();
	QString text = time.toString("yyyy-MM-dd hh:mm:ss");

	QStringList tList = upData.split("\n");

	ui.textBrowser_ISN->append("");
	ui.textBrowser_ISN->append("");
	recvISN++;
	ui.textBrowser_ISN->append("<font color = black>" + QString("No.%1").arg(recvISN) + "</font>");
	ui.textBrowser_ISN->append("<font color = black>" + text + "</font>");
	ui.textBrowser_ISN->append("<font color = blue>" + upData + "</font>");

	if (tList.size() > 2)
	{
		ui.textBrowser_ISN->append("not Support.");
		stopSignal(descriptor, QString("22"));
		return;
	}
	
	if (tList[0].size() < 25)
	{
		ui.textBrowser_ISN->append("QR Error.");
		stopSignal(descriptor, QString("22"));
		return;
	}

	if (1 == tList.size() || 2 == tList.size())
	{
	
		CPostOne* pPostOne = new CPostOne(tPath, tList[0].left(25), descriptor, this);

		connect(pPostOne, SIGNAL(isncheckSignal(int, const int, const QString)), this, SLOT(isncheckSlot(int, const int, const QString)));

		pPostOne->isncheck();

	}

	/*
	else
	{
		ui.textBrowser->append("");
		ui.textBrowser->append("");
		
		recvNum++;
		ui.textBrowser->append("<font color = black>" + QString("No.%1").arg(recvNum) + "</font>");
		ui.textBrowser->append("<font color = black>" + text + "</font>");
		ui.textBrowser->append("<font color = blue>" + upData + "</font>");
		
		if (tList.size() < mDataNum + 2 || tList.size() > 5)
		{
			ui.textBrowser->append("WRONG SIZE:89");
			stopSignal(descriptor, QString("%1").arg(ERROR_CODE_FAILED));
			return;
		}

		if (tList[2].size() < 10 || tList[3].size() < 20)
		{
			ui.textBrowser->append("WRONG QR:89");
			stopSignal(descriptor, QString("%1").arg(ERROR_CODE_FAILED));
			return;
		}
		
		//
		curErrorCode = 0;
		mDataList.clear();

		tUser = tList[0];
		tErr = "ERVA" + tList[1].right(2);
		
		if ("ERVA00" == tErr)
		{
			bProErr = false;
		}
		else
		{
			bProErr = true;
		}

		for (size_t i = 0; i < mDataNum; i++)
		{
			mDataList.append(tList[i + 2]);
		}

		pCurState = &mStatusList.first();

		pCurState++;

		//
		stepSignal(pCurState);
		this->descriptor = descriptor;
	}
	*/

}

void TCPHTTP::stepSlot1(QString* pCurState)
{
	if (*pCurState == "WTSP_NULL" || *pCurState == "FINISH")
	{
		return;
	}
	else if (*pCurState == "WTSP_LOG_IN")
	{
		ui.textBrowser->append("LOG_IN:" + tUser);
		loginout(LOG_IN, tDevice, tUser);
		return;
	}
	else if (*pCurState == "WTSP_INPUT_ERR")
	{
		ui.textBrowser->append("Err");
		inputdata(tDevice, tErr);
		return;
	}
	else if (*pCurState == "WTSP_LOG_OUT")
	{
		ui.textBrowser->append("LOG_OUT:");
		loginout(LOG_OUT, tDevice, tUser);
		return;
	}
	else
	{
		auto tState = *pCurState;
		ui.textBrowser->append(tState);
		int index = tState.right(1).toInt();
		inputdata(tDevice, mDataList.at(index - 1));
		return;
	}
}

void  TCPHTTP::macReSlot(const QString macRe)
{
	stopSignal(this->descriptor, macRe);
}

#include "csvUp.h"
#include <QTimer>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include "QXmlStreamReader"
#include <QDir>

Worker* work;

csvUp::csvUp(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	QSettings cfgIni(tPath + "/cfg.ini", QSettings::IniFormat);

	//
	Qt::WindowFlags flags = Qt::Dialog;
	flags |= Qt::WindowMinMaxButtonsHint;
//	flags |= Qt::WindowCloseButtonHint;
	setWindowFlags(flags);

	//

	tUrl = cfgIni.value("/Url/SOAP_WSDL").toString();
	tDevice = cfgIni.value("/DEVICE/tDevice").toString();

	//
	p_manager = new QNetworkAccessManager(this);
	connect(p_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

	//
	
	fileTimer = new QTimer();
	connect(fileTimer, SIGNAL(timeout()), this, SLOT(onFileTimer()));
	
	//

	work = new Worker(this);
	QTimer::singleShot(0, work, SLOT(csvUpThread()));

	connect(work, SIGNAL(uiShowSignal(QString)), this, SLOT(uiShowSlot(QString)));
}

csvUp::~csvUp()
{
	
}

void csvUp::onFileTimer()
{
	QDateTime datetime = QDateTime::currentDateTime();
	QSettings cfgIni(tPath + "/cfg.ini", QSettings::IniFormat);

	QString beginDate = cfgIni.value("/BEGIN/beginDate").toString();

	/*
	if (datetime.toString("hh:mm:ss") == "00:00:00")
	{
		
	}
	*/
	
	QString filePath = cfgIni.value("/PATH/filePath").toString();


	QString fileName = filePath + QStringLiteral("生产数据_") + datetime.toString("yyyy-MM-dd").left(10) + ".csv";
	
	if (beginDate != datetime.toString("yyyy-MM-dd") && true == QFile::exists(fileName))
	{
		cfgIni.setValue("/BEGIN/beginDate", datetime.toString("yyyy-MM-dd"));
		cfgIni.setValue("/BEGIN/beginLine", 0);		
	}

}

void csvUp::closeEvent(QCloseEvent *event)
{
	
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
	QString savePath = tPath + "/Log/" + "csv" + txtName + ".txt";

	QFile file(savePath);

	auto a = file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);

	QTextStream out(&file);

	out << saveText << "\n" << "\n";

	file.flush();
	file.close();

	work->m_thread.terminate();
	work->m_thread.wait();
	//
}

void csvUp::uiShowSlot(QString showMsg)
{
	ui.textBrowser->append(showMsg);
}

void csvUp::replyFinished(QNetworkReply *reply)
{
	//DELAY
	if (reply->error() != QNetworkReply::NoError)
	{
		netFlag = false;

		QByteArray bytes = reply->readAll();
		QString string = QString::fromUtf8(bytes);
	}
	else
	{
		netFlag = true;

		QByteArray bytes = reply->readAll();
		qDebug() << bytes;
		QString reMsg = QString::fromUtf8(bytes);
		
		QString reInfo;
		
		if (false == lwmFlag)
		{
			if (true == xmlInputDecode(reMsg, reInfo))
			{
				passFlag = true;
			}
		}
		else
		{
			if (true == xmlLwmDecode(reMsg, reInfo))
			{
				passFlag = true;
			}

		}	

		ui.textBrowser->append(reInfo);
	}

	doneFlag = true;
	QApplication::processEvents();

	reply->deleteLater();
}

bool csvUp::xmlLoginoutDecode(QString xmlCode)
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
				if (PASS == re.left(1) || LOGIN_TWINCE == re.mid(2, 11))
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

bool csvUp::xmlInputDecode(QString xmlCode, QString& reInfo)
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

bool csvUp::xmlLwmDecode(QString xmlCode, QString& reInfo)
{
	QXmlStreamReader reader(xmlCode);

	reader.setNamespaceProcessing(false);

	while (!reader.atEnd())
	{
		if (reader.isStartElement())
		{
			if (reader.name() == "WTSP_GETVERSIONResult")
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

bool csvUp::curStateDecode(QString* pCurState, QString reMsg)
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

static void LWM_MsgEncode(QString device, QString dataIsn, QString dataFixId, QString dataLwm, QString lwmType, QString & msg)
{
	msg.clear();

	msg += "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"\r";
	msg += "xmlns:sfis=\"http://www.pegatroncorp.com/SFISWebService/\">\r";
	msg += "<soap:Header/>\r";
	msg += "<soap:Body>\r";
	msg += "<sfis:WTSP_GETVERSION>\r";
	msg += "<!--Optional:-->\r";
	msg += "<sfis:programId>TSP_NLSPAD</sfis:programId>\r";
	msg += "<!--Optional:-->\r";
	msg += "<sfis:programPassword>N3eaM;</sfis:programPassword>\r";

	msg += "<!--Optional:-->\r";
	msg += "<sfis:ISN>";
	msg += dataIsn;
	msg += "</sfis:ISN>\r";		//DATA  ISN等

	msg += "<!--Optional:-->\r";
	msg += "<sfis:device>";
	msg += device;
	msg += "</sfis:device>\r";      // DEVICE  由铠嘉提供

	msg += "<!--Optional:-->\r";
	msg += "<sfis:type>WELDING_LWM</sfis:type>\r";	//WELDING_LWM

	msg += "<!--Optional:-->\r";
	msg += "<sfis:ChkData>";
	msg += dataFixId;
	msg += "</sfis:ChkData>\r";

	msg += "<!--Optional:-->\r";
	msg += "<sfis:ChkData2>";
	msg += dataLwm;
	msg += "+";
	msg += lwmType;
	msg += "</sfis:ChkData2>\r";

	msg += "</sfis:WTSP_GETVERSION>\r";
	msg += "</soap:Body>\r";
	msg += "</soap:Envelope>\r";
}

void csvUp::loginout(WTSP_LOGINOUT inORout, QString device, QString tUser)
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

	reply = p_manager->post(request, postData);

	qDebug() << reply->error();
}

void csvUp::inputdata(QString device, QString data)
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

	reply = p_manager->post(request, postData);

	qDebug() << reply->error();
}

void csvUp::lwmdata(QString device, QString dataIsn, QString dataFixId, QString dataLwm, QString lwmType)
{
	//POST
	QNetworkRequest request;

	request.setUrl(QUrl(tUrl));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Content-Type", "application/soap+xml;setchar-utf8");

	QString upMsg;

	LWM_MsgEncode(device, dataIsn, dataFixId, dataLwm, lwmType, upMsg);

	QByteArray  postData;
	postData.append(upMsg);

	reply = p_manager->post(request, postData);

	qDebug() << reply->error();
}

void Worker::csvUpThread()
{
	//
	const QString tPath = qApp->applicationDirPath();

	static int tTimerStart = 0;

	while (true)
	{		
		tTimerStart++;
		if (0 == tTimerStart % 5)
		{
			pm_csvUp->onFileTimer();

			tTimerStart = 0;
		}

		_sleep(1000);
	
		//
		QSettings cfgIni(tPath + "/cfg.ini", QSettings::IniFormat);

		QString beginDate = cfgIni.value("/BEGIN/beginDate").toString();
		int beginLine = cfgIni.value("/BEGIN/beginLine").toInt();

		QString filePath = cfgIni.value("/PATH/filePath").toString();

		QString fileName = filePath + QStringLiteral("生产数据_") + beginDate.left(10) + ".csv";

		QFile csvFile(fileName);

		QStringList CSVList;

		if (!csvFile.open(QIODevice::ReadOnly))
		{
			continue;
		}

		QTextStream stream(&csvFile);

		while (!stream.atEnd())
		{
			CSVList.push_back(stream.readLine());
		}
		csvFile.close();

		while (beginLine < CSVList.size())
		{
			_sleep(1000);

			QDateTime time = QDateTime::currentDateTime();
			QString text = time.toString("yyyy-MM-dd hh:mm:ss");

			uiShowSignal("");
			uiShowSignal(text);
			uiShowSignal(QString("Line: %1").arg(beginLine));
			uiShowSignal(CSVList[beginLine]);

			QStringList oneLine = CSVList[beginLine].split(" ,");
			
			//
			if (oneLine.size() < 10)
			{
				uiShowSignal("Info Error.");
				beginLine++;

				cfgIni.setValue("/BEGIN/beginLine", beginLine);
				continue;
			}

			//
			if (oneLine[0].size() < 25 || oneLine[1].size() < 16)
			{
				uiShowSignal("QR Error.");
				beginLine++;

				cfgIni.setValue("/BEGIN/beginLine", beginLine);
				continue;
			}

			QString HSG = oneLine[0].left(25);
			QString fixId = oneLine[1].left(16);
			QString LWM1 = oneLine[10];
			QString LWM2 = oneLine[12];
			
			//
			auto b = oneLine[3];

			//
			QString codeError;

			/*
			if (oneLine[3] != "OK")
			{
				codeError = "ERVA01";
			}
			else if (oneLine[5] != "OK")
			{
				codeError = "ERVA02";
			}
			else if (oneLine[6] != "OK" && oneLine[7] != "OK")
			{
				codeError = "ERVA03";
			}
			else if (oneLine[8] != "OK" && oneLine[9] != "OK")
			{
				codeError = "ERVA04";
			}
			else
			{
				codeError = "ERVA00";
			}
			*/


			if (!("OK" == oneLine[3] && "OK" == oneLine[4] && "OK" == oneLine[5]
				&& ("OK" == oneLine[6] || "OK" == oneLine[7]) && ("OK" == oneLine[8] || "OK" == oneLine[9])))
			{
				uiShowSignal("Not Up.");
				beginLine++;

				cfgIni.setValue("/BEGIN/beginLine", beginLine);
				continue;
			}
			
			/*
			else if (oneLine[8] != "OK" && oneLine[9] != "OK")
			{
				codeError = "ERVA04";
			}
			*/

			else
			{
				codeError = "ERVA00";
			}


			//
			pm_csvUp->doneFlag = false;
			pm_csvUp->loginout(pm_csvUp->LOG_IN, pm_csvUp->tDevice, pm_csvUp->tUser);
			while (!pm_csvUp->doneFlag) { _sleep(10); QApplication::processEvents(); }

			if (false == pm_csvUp->netFlag){	uiShowSignal("Net Failed...");	continue;}

			//
			if (codeError != "ERVA00")
			{
				uiShowSignal("Up codeError");
				pm_csvUp->doneFlag = false;
				pm_csvUp->inputdata(pm_csvUp->tDevice, codeError);
				while (!pm_csvUp->doneFlag && pm_csvUp->netFlag)	{ _sleep(10); QApplication::processEvents(); }
			
				if (false == pm_csvUp->netFlag){ uiShowSignal("Net Failed...");	continue; }

			}
			
			//
			uiShowSignal(fixId);
			pm_csvUp->doneFlag = false;
			pm_csvUp->inputdata(pm_csvUp->tDevice, fixId);
			while (!pm_csvUp->doneFlag) { _sleep(10); QApplication::processEvents(); }
			
			if (false == pm_csvUp->netFlag){ uiShowSignal("Net Failed...");	continue; }

			//
			uiShowSignal(HSG);
			pm_csvUp->doneFlag = false;
			pm_csvUp->passFlag = false;
			pm_csvUp->inputdata(pm_csvUp->tDevice, HSG);
			while (!pm_csvUp->doneFlag) { _sleep(10); QApplication::processEvents(); }
			
			if (false == pm_csvUp->netFlag){ uiShowSignal("Net Failed...");	continue; }
			
			
			//
			uiShowSignal(LWM1); pm_csvUp->lwmFlag = true;
			pm_csvUp->doneFlag = false;
			pm_csvUp->passFlag = false;
			pm_csvUp->lwmdata(pm_csvUp->tDevice, HSG, fixId, LWM1, "2");
			while (!pm_csvUp->doneFlag) { _sleep(10); QApplication::processEvents(); }

			if (false == pm_csvUp->netFlag){ uiShowSignal("Net Failed...");	continue; }

			//
			uiShowSignal(LWM2); pm_csvUp->lwmFlag = true;
			pm_csvUp->doneFlag = false;
			pm_csvUp->passFlag = false;
			pm_csvUp->lwmdata(pm_csvUp->tDevice, HSG, fixId, LWM2, "3");
			while (!pm_csvUp->doneFlag) { _sleep(10); QApplication::processEvents(); }

			if (false == pm_csvUp->netFlag){ uiShowSignal("Net Failed...");	continue; }

			//
			pm_csvUp->doneFlag = false; pm_csvUp->lwmFlag = false;
			pm_csvUp->loginout(pm_csvUp->LOG_OUT, pm_csvUp->tDevice, pm_csvUp->tUser);
			while (!pm_csvUp->doneFlag) { _sleep(10); QApplication::processEvents(); }
			
			if (false == pm_csvUp->netFlag){ uiShowSignal("Net Failed...");	continue; }

			//
			if (true == pm_csvUp->passFlag)
			{
				beginLine++;
			}	
			else
			{
				pm_csvUp->tryTime++;
			}
			
			if(pm_csvUp->tryTime > 1)
			{
				beginLine++;
				pm_csvUp->tryTime = 0;
			}
			

			cfgIni.setValue("/BEGIN/beginLine", beginLine);

		}
	}


}

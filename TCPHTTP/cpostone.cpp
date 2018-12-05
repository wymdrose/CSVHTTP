#include "cpostone.h"
#include "QXmlStreamReader"
#include <QSettings>

CPostOne::CPostOne(QString tPath, QString tIsn, int descriptor, QObject *parent)
	: QObject(parent), ISN(tIsn), descriptor(descriptor), tPath(tPath)
{
	manager = new QNetworkAccessManager(this);
	connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));


	QSettings cfgIni(tPath + "/cfg.ini", QSettings::IniFormat);

	tUrl = cfgIni.value("/Url/SOAP_WSDL").toString();
	tDevice = cfgIni.value("/DEVICE/tDevice").toString();

}

CPostOne::~CPostOne()
{
	delete manager;
}

void CPostOne::replyFinished(QNetworkReply *reply)
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

		isncheckSignal(this->descriptor, 22, "NET ERROR.");

	}
	else
	{
		QByteArray bytes = reply->readAll();
	//	qDebug() << bytes;
		reMsg = QString::fromUtf8(bytes);

		if (false == xmlIsnCheckDecode(reMsg, reInfo))
		{
			isncheckSignal(this->descriptor, 22, reInfo);
		}
		else
		{
			isncheckSignal(this->descriptor, 11, "");
		}		
	}

	reply->deleteLater();
	this->deleteLater();
}

bool CPostOne::xmlIsnCheckDecode(QString xmlCode, QString& reInfo)
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

				if (reInfo.left(1) == "1")
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

void CPostOne::isncheckMsgEncode()
{
	upMsg.clear();

	upMsg += "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"\r";
	upMsg += "xmlns:sfis=\"http://www.pegatroncorp.com/SFISWebService/\">\r";
	upMsg += "<soap:Header/>\r";
	upMsg += "<soap:Body>\r";
	upMsg += "<sfis:WTSP_GETVERSION>\r";
	upMsg += "<!--Optional:-->\r";
	upMsg += "<sfis:programId>TSP_NLSPAD</sfis:programId>\r";
	upMsg += "<!--Optional:-->\r";
	upMsg += "<sfis:programPassword>N3eaM;</sfis:programPassword>\r";
	upMsg += "<!--Optional:-->\r";
	
	upMsg += "<sfis:ISN>";
	upMsg += ISN;
	upMsg += "</sfis:ISN>\r";

	upMsg += "<sfis:device>";
	upMsg += tDevice;
	upMsg += "</sfis:device>\r";      
	
	upMsg += "<!--Optional:-->\r";
	
	upMsg += "<sfis:type>";
	upMsg += "YH_ROUTE_NSTEP";
	upMsg += "</sfis:type>\r";	

	upMsg += "<!--Optional:-->\r";
	upMsg += "<sfis:ChkData>SN</sfis:ChkData>\r";
	upMsg += "<sfis:ChkData2>?</sfis:ChkData2>\r";
	
	upMsg += "</sfis:WTSP_GETVERSION>\r";
	upMsg += "</soap:Body>\r";
	upMsg += "</soap:Envelope>\r";
}

void CPostOne::isncheck()
{
	//POST
	QNetworkRequest request;

	request.setUrl(QUrl(tUrl));

	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Content-Type", "application/soap+xml;setchar-utf8");

	isncheckMsgEncode();

	QByteArray  postData;
	postData.append(upMsg);

	reply = manager->post(request, postData);

	qDebug() << reply->error();
}

void CPostOne::loginMsgEncode()
{
	upMsg.clear();
	
	upMsg += "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"\r";
	upMsg += "xmlns:sfis=\"http://www.pegatroncorp.com/SFISWebService/\">\r";
	upMsg += "<soap:Header/>\r";
	upMsg += "<soap:Body>\r";
	upMsg += "<sfis:WTSP_LOGINOUT>\r";
	
	upMsg += "<!--Optional:-->\r";
	
	upMsg += "<sfis:programId>TSP_NLSPAD</sfis:programId>\r";			// = "TSP_NLSPAD"
	upMsg += "<!--Optional:-->\r";
	upMsg += "<sfis:programPassword>N3eaM;</sfis:programPassword>\r";
	upMsg += "<!--Optional:-->\r";
	
	upMsg += "<sfis:op>";
	upMsg += tUser;
	upMsg += "</sfis:op>\r";
	
	upMsg += "<!--Optional:-->\r";
	upMsg += "<sfis:password></sfis:password>\r";
	upMsg += "<!--Optional:-->\r";
	
	upMsg += "<sfis:device>";
	upMsg += tDevice;
	upMsg += "</sfis:device>\r";      // DEVICE  由铠嘉提供
	
	upMsg += "<!--Optional:-->\r";
	upMsg += "<sfis:TSP>LINK</sfis:TSP>\r";

	upMsg += "<sfis:status>";
	upMsg += "1";
	upMsg += "</sfis:status>\r";          //  1代表登录, 2代表登出
	
	upMsg += "</sfis:WTSP_LOGINOUT>\r";
	upMsg += "</soap:Body>\r";
	upMsg += "</soap:Envelope>\r";
}

void CPostOne::login()
{
	//POST
	QNetworkRequest request;

	request.setUrl(QUrl(tUrl));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Content-Type", "application/soap+xml;setchar-utf8");

	loginMsgEncode();

	QByteArray  postData;
	postData.append(upMsg);

	reply = manager->post(request, postData);

	qDebug() << reply->error();
}

